#include "TunaSweeperLunaSkirtPhysicsSetup.h"

#include "AnimGraphNode_ComponentToLocalSpace.h"
#include "AnimGraphNode_LocalRefPose.h"
#include "AnimGraphNode_LocalToComponentSpace.h"
#include "AnimGraphNode_RigidBody.h"
#include "AnimGraphNode_Root.h"
#include "Animation/AnimBlueprint.h"
#include "Animation/Skeleton.h"
#include "AnimationGraph.h"
#include "AnimationGraphSchema.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "Components/SkeletalMeshComponent.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "Editor.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "Factories/AnimBlueprintFactory.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "PhysicsAssetUtils.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "PhysicsEngine/PhysicsConstraintTemplate.h"
#include "PhysicsEngine/SkeletalBodySetup.h"
#include "Subsystems/EditorAssetSubsystem.h"

namespace
{
	const TCHAR* SkeletalMeshObjectPath = TEXT("/Game/Characters/Player/Luna/Skirt/Luna__Skirt_front.Luna__Skirt_front");
	const TCHAR* SkeletonObjectPath = TEXT("/Game/Characters/Player/Luna/Skirt/Luna__Skirt_front_Skeleton.Luna__Skirt_front_Skeleton");
	const TCHAR* PhysicsAssetObjectPath = TEXT("/Game/Characters/Player/Luna/Skirt/Luna__Skirt_front_PhysicsAsset.Luna__Skirt_front_PhysicsAsset");
	const TCHAR* AnimBlueprintPackagePath = TEXT("/Game/Characters/Player/Luna/Skirt/Animations");
	const TCHAR* AnimBlueprintObjectPath = TEXT("/Game/Characters/Player/Luna/Skirt/Animations/ABP_Luna_Skirt.ABP_Luna_Skirt");
	const TCHAR* PlayerBlueprintObjectPath = TEXT("/Game/Characters/Player/BP_TunaSweeperPlayerCharacter.BP_TunaSweeperPlayerCharacter");

	bool SaveAsset(UObject* Asset)
	{
		UEditorAssetSubsystem* AssetSubsystem = GEditor ? GEditor->GetEditorSubsystem<UEditorAssetSubsystem>() : nullptr;
		return AssetSubsystem && Asset && AssetSubsystem->SaveLoadedAsset(Asset, false);
	}

	bool IsSkirtChainBone(const FName BoneName)
	{
		return BoneName.ToString().StartsWith(TEXT("skirt_"));
	}

	bool IsAnchorBone(const USkeletalMesh* SkeletalMesh, const FName BoneName)
	{
		if (!SkeletalMesh || !IsSkirtChainBone(BoneName))
		{
			return false;
		}

		const FReferenceSkeleton& RefSkeleton = SkeletalMesh->GetRefSkeleton();
		const int32 BoneIndex = RefSkeleton.FindBoneIndex(BoneName);
		const int32 ParentIndex = BoneIndex == INDEX_NONE ? INDEX_NONE : RefSkeleton.GetParentIndex(BoneIndex);
		return ParentIndex != INDEX_NONE && RefSkeleton.GetBoneName(ParentIndex) == FName(TEXT("SkirtRoot"));
	}

	void LogReferenceSkeleton(const USkeletalMesh* SkeletalMesh)
	{
		const FReferenceSkeleton& RefSkeleton = SkeletalMesh->GetRefSkeleton();
		const TArray<FTransform>& LocalPose = RefSkeleton.GetRefBonePose();
		TArray<FTransform> ComponentPose;
		ComponentPose.SetNum(LocalPose.Num());

		for (int32 BoneIndex = 0; BoneIndex < LocalPose.Num(); ++BoneIndex)
		{
			const int32 ParentIndex = RefSkeleton.GetParentIndex(BoneIndex);
			ComponentPose[BoneIndex] = ParentIndex == INDEX_NONE
				? LocalPose[BoneIndex]
				: LocalPose[BoneIndex] * ComponentPose[ParentIndex];

			UE_LOG(
				LogTemp,
				Display,
				TEXT("Luna skirt bone %d: %s, parent=%s, local=%s, component=%s"),
				BoneIndex,
				*RefSkeleton.GetBoneName(BoneIndex).ToString(),
				ParentIndex == INDEX_NONE ? TEXT("None") : *RefSkeleton.GetBoneName(ParentIndex).ToString(),
				*LocalPose[BoneIndex].GetTranslation().ToCompactString(),
				*ComponentPose[BoneIndex].GetTranslation().ToCompactString());
		}
	}

	void ReplaceWithThinChainCollider(USkeletalBodySetup* BodySetup, const USkeletalMesh* SkeletalMesh)
	{
		if (!BodySetup || !SkeletalMesh)
		{
			return;
		}

		const FReferenceSkeleton& RefSkeleton = SkeletalMesh->GetRefSkeleton();
		const int32 BoneIndex = RefSkeleton.FindBoneIndex(BodySetup->BoneName);
		int32 ChildBoneIndex = INDEX_NONE;
		for (int32 CandidateIndex = BoneIndex + 1; CandidateIndex < RefSkeleton.GetRawBoneNum(); ++CandidateIndex)
		{
			if (RefSkeleton.GetParentIndex(CandidateIndex) == BoneIndex)
			{
				ChildBoneIndex = CandidateIndex;
				break;
			}
		}

		constexpr float DynamicRadius = 0.8f;
		constexpr float AnchorRadius = 0.9f;
		const float Radius = IsAnchorBone(SkeletalMesh, BodySetup->BoneName) ? AnchorRadius : DynamicRadius;
		BodySetup->AggGeom.EmptyElements();

		if (ChildBoneIndex != INDEX_NONE)
		{
			const FVector ChildOffset = RefSkeleton.GetRefBonePose()[ChildBoneIndex].GetTranslation();
			const float BoneDistance = ChildOffset.Size();
			if (BoneDistance > UE_SMALL_NUMBER)
			{
				FKSphylElem Capsule;
				Capsule.Radius = Radius;
				Capsule.Length = FMath::Max(0.1f, BoneDistance - (2.0f * Radius));
				Capsule.Center = ChildOffset * 0.5f;
				Capsule.Rotation = FQuat::FindBetweenNormals(FVector::UpVector, ChildOffset / BoneDistance).Rotator();
				BodySetup->AggGeom.SphylElems.Add(Capsule);
				return;
			}
		}

		FKSphereElem EndSphere;
		EndSphere.Radius = Radius;
		BodySetup->AggGeom.SphereElems.Add(EndSphere);
	}

	bool RebuildPhysicsAsset(USkeletalMesh* SkeletalMesh, UPhysicsAsset* PhysicsAsset)
	{
		if (!SkeletalMesh || !PhysicsAsset)
		{
			return false;
		}

		PhysicsAsset->Modify();
		while (PhysicsAsset->ConstraintSetup.Num() > 0)
		{
			FPhysicsAssetUtils::DestroyConstraint(PhysicsAsset, PhysicsAsset->ConstraintSetup.Num() - 1);
		}
		while (PhysicsAsset->SkeletalBodySetups.Num() > 0)
		{
			FPhysicsAssetUtils::DestroyBody(PhysicsAsset, PhysicsAsset->SkeletalBodySetups.Num() - 1);
		}

		FPhysAssetCreateParams CreateParams;
		CreateParams.MinBoneSize = 0.1f;
		CreateParams.MinWeldSize = KINDA_SMALL_NUMBER;
		CreateParams.GeomType = EFG_Sphyl;
		CreateParams.VertWeight = EVW_AnyWeight;
		CreateParams.bAutoOrientToBone = true;
		CreateParams.bCreateConstraints = true;
		CreateParams.bWalkPastSmall = false;
		CreateParams.bBodyForAll = true;
		CreateParams.bDisableCollisionsByDefault = true;
		CreateParams.AngularConstraintMode = ACM_Limited;

		FText ErrorMessage;
		if (!FPhysicsAssetUtils::CreateFromSkeletalMesh(
			PhysicsAsset,
			SkeletalMesh,
			CreateParams,
			ErrorMessage,
			false,
			false))
		{
			UE_LOG(LogTemp, Error, TEXT("Luna skirt PhysicsAsset generation failed: %s"), *ErrorMessage.ToString());
			return false;
		}

		for (int32 BodyIndex = PhysicsAsset->SkeletalBodySetups.Num() - 1; BodyIndex >= 0; --BodyIndex)
		{
			USkeletalBodySetup* BodySetup = PhysicsAsset->SkeletalBodySetups[BodyIndex];
			if (!BodySetup || !IsSkirtChainBone(BodySetup->BoneName))
			{
				FPhysicsAssetUtils::DestroyBody(PhysicsAsset, BodyIndex);
				continue;
			}

			BodySetup->Modify();
			BodySetup->PhysicsType = IsAnchorBone(SkeletalMesh, BodySetup->BoneName) ? PhysType_Kinematic : PhysType_Simulated;
			BodySetup->DefaultInstance.LinearDamping = 2.5f;
			BodySetup->DefaultInstance.AngularDamping = 5.0f;
			BodySetup->DefaultInstance.MassScale = 0.25f;
			BodySetup->DefaultInstance.SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics, false);
			ReplaceWithThinChainCollider(BodySetup, SkeletalMesh);
			BodySetup->InvalidatePhysicsData();
			BodySetup->CreatePhysicsMeshes();
		}

		for (UPhysicsConstraintTemplate* Constraint : PhysicsAsset->ConstraintSetup)
		{
			if (!Constraint)
			{
				continue;
			}

			Constraint->Modify();
			FConstraintInstance& Instance = Constraint->DefaultInstance;
			Instance.SetLinearXMotion(LCM_Locked);
			Instance.SetLinearYMotion(LCM_Locked);
			Instance.SetLinearZMotion(LCM_Locked);
			Instance.SetAngularSwing1Limit(ACM_Limited, 28.0f);
			Instance.SetAngularSwing2Limit(ACM_Limited, 38.0f);
			Instance.SetAngularTwistLimit(ACM_Limited, 14.0f);
			Instance.SetAngularDriveMode(EAngularDriveMode::SLERP);
			Instance.SetOrientationDriveSLERP(true);
			Instance.SetAngularDriveParams(18.0f, 2.5f, 0.0f);
			Instance.SetAngularDriveAccelerationMode(true);
			Constraint->SetDefaultProfile(Instance);
		}

		PhysicsAsset->PreviewSkeletalMesh = SkeletalMesh;
		PhysicsAsset->UpdateBodySetupIndexMap();
		PhysicsAsset->UpdateBoundsBodiesArray();
		PhysicsAsset->RefreshPhysicsAssetChange();
		PhysicsAsset->MarkPackageDirty();

		SkeletalMesh->Modify();
		SkeletalMesh->SetPhysicsAsset(PhysicsAsset);
		SkeletalMesh->MarkPackageDirty();

		if (!SaveAsset(PhysicsAsset) || !SaveAsset(SkeletalMesh))
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to save Luna skirt PhysicsAsset or SkeletalMesh."));
			return false;
		}

		int32 KinematicCount = 0;
		int32 SimulatedCount = 0;
		for (const USkeletalBodySetup* BodySetup : PhysicsAsset->SkeletalBodySetups)
		{
			if (BodySetup && BodySetup->PhysicsType == PhysType_Kinematic)
			{
				++KinematicCount;
			}
			else if (BodySetup && BodySetup->PhysicsType == PhysType_Simulated)
			{
				++SimulatedCount;
			}
		}

		UE_LOG(
			LogTemp,
			Display,
			TEXT("Luna skirt PhysicsAsset configured: %d kinematic bodies, %d simulated bodies, %d constraints."),
			KinematicCount,
			SimulatedCount,
			PhysicsAsset->ConstraintSetup.Num());
		return KinematicCount == 16 && SimulatedCount == 48 && PhysicsAsset->ConstraintSetup.Num() == 48;
	}

	UEdGraphPin* FindPosePin(UEdGraphNode* Node, EEdGraphPinDirection Direction)
	{
		if (!Node)
		{
			return nullptr;
		}

		for (UEdGraphPin* Pin : Node->Pins)
		{
			if (Pin && Pin->Direction == Direction && UAnimationGraphSchema::IsPosePin(Pin->PinType))
			{
				return Pin;
			}
		}
		return nullptr;
	}

	UAnimationGraph* FindAnimationGraph(UAnimBlueprint* AnimBlueprint)
	{
		for (UEdGraph* FunctionGraph : AnimBlueprint->FunctionGraphs)
		{
			if (UAnimationGraph* AnimationGraph = Cast<UAnimationGraph>(FunctionGraph))
			{
				if (AnimationGraph->GetFName() == FName(TEXT("AnimGraph")))
				{
					return AnimationGraph;
				}
			}
		}
		return nullptr;
	}

	UAnimBlueprint* EnsureAnimBlueprint(USkeleton* Skeleton, USkeletalMesh* SkeletalMesh)
	{
		UAnimBlueprint* AnimBlueprint = LoadObject<UAnimBlueprint>(nullptr, AnimBlueprintObjectPath);
		if (!AnimBlueprint)
		{
			UAnimBlueprintFactory* Factory = NewObject<UAnimBlueprintFactory>();
			Factory->BlueprintType = BPTYPE_Normal;
			Factory->TargetSkeleton = Skeleton;
			Factory->PreviewSkeletalMesh = SkeletalMesh;

			FAssetToolsModule& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
			AnimBlueprint = Cast<UAnimBlueprint>(AssetTools.Get().CreateAsset(
				TEXT("ABP_Luna_Skirt"),
				AnimBlueprintPackagePath,
				UAnimBlueprint::StaticClass(),
				Factory));
		}

		if (!AnimBlueprint)
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to create ABP_Luna_Skirt."));
			return nullptr;
		}

		AnimBlueprint->Modify();
		AnimBlueprint->TargetSkeleton = Skeleton;
		AnimBlueprint->SetPreviewMesh(SkeletalMesh);

		UAnimationGraph* AnimGraph = FindAnimationGraph(AnimBlueprint);
		UAnimGraphNode_Root* RootNode = AnimGraph ? FBlueprintEditorUtils::GetAnimGraphRoot(AnimGraph) : nullptr;
		if (!AnimGraph || !RootNode)
		{
			UE_LOG(LogTemp, Error, TEXT("ABP_Luna_Skirt has no usable AnimGraph root."));
			return nullptr;
		}

		AnimGraph->Modify();
		TArray<UEdGraphNode*> NodesToRemove;
		for (UEdGraphNode* GraphNode : AnimGraph->Nodes)
		{
			if (GraphNode != RootNode)
			{
				NodesToRemove.Add(GraphNode);
			}
		}
		for (UEdGraphNode* GraphNode : NodesToRemove)
		{
			AnimGraph->RemoveNode(GraphNode);
		}

		FGraphNodeCreator<UAnimGraphNode_LocalRefPose> RefPoseCreator(*AnimGraph);
		UAnimGraphNode_LocalRefPose* RefPoseNode = RefPoseCreator.CreateNode();
		RefPoseNode->NodePosX = -900;
		RefPoseNode->NodePosY = 0;
		RefPoseCreator.Finalize();

		FGraphNodeCreator<UAnimGraphNode_LocalToComponentSpace> LocalToComponentCreator(*AnimGraph);
		UAnimGraphNode_LocalToComponentSpace* LocalToComponentNode = LocalToComponentCreator.CreateNode();
		LocalToComponentNode->NodePosX = -650;
		LocalToComponentNode->NodePosY = 0;
		LocalToComponentCreator.Finalize();

		FGraphNodeCreator<UAnimGraphNode_RigidBody> RigidBodyCreator(*AnimGraph);
		UAnimGraphNode_RigidBody* RigidBodyNode = RigidBodyCreator.CreateNode();
		RigidBodyNode->NodePosX = -400;
		RigidBodyNode->NodePosY = 0;
		RigidBodyNode->Node.OverridePhysicsAsset = SkeletalMesh->GetPhysicsAsset();
		RigidBodyNode->Node.bDefaultToSkeletalMeshPhysicsAsset = true;
		RigidBodyNode->Node.bUseDefaultAsSimulated = false;
		RigidBodyNode->Node.SimulationSpace = ESimulationSpace::ComponentSpace;
		RigidBodyNode->Node.ComponentLinearAccScale = FVector(0.75f);
		RigidBodyNode->Node.ComponentLinearVelScale = FVector(0.05f);
		RigidBodyNode->Node.ComponentAppliedLinearAccClamp = FVector(500.0f);
		RigidBodyNode->Node.CachedBoundsScale = 1.25f;
		RigidBodyNode->Node.bEnableWorldGeometry = false;
		RigidBodyNode->Node.bTransferBoneVelocities = true;
		RigidBodyNode->Node.bForceDisableCollisionBetweenConstraintBodies = true;
		RigidBodyNode->Node.EvaluationResetTime = 0.5f;
		RigidBodyNode->Node.Alpha = 1.0f;
		RigidBodyCreator.Finalize();

		FGraphNodeCreator<UAnimGraphNode_ComponentToLocalSpace> ComponentToLocalCreator(*AnimGraph);
		UAnimGraphNode_ComponentToLocalSpace* ComponentToLocalNode = ComponentToLocalCreator.CreateNode();
		ComponentToLocalNode->NodePosX = -100;
		ComponentToLocalNode->NodePosY = 0;
		ComponentToLocalCreator.Finalize();

		RootNode->NodePosX = 150;
		RootNode->NodePosY = 0;

		UEdGraphPin* RefPoseOutput = FindPosePin(RefPoseNode, EGPD_Output);
		UEdGraphPin* LocalToComponentInput = FindPosePin(LocalToComponentNode, EGPD_Input);
		UEdGraphPin* LocalToComponentOutput = FindPosePin(LocalToComponentNode, EGPD_Output);
		UEdGraphPin* RigidBodyInput = FindPosePin(RigidBodyNode, EGPD_Input);
		UEdGraphPin* RigidBodyOutput = FindPosePin(RigidBodyNode, EGPD_Output);
		UEdGraphPin* ComponentToLocalInput = FindPosePin(ComponentToLocalNode, EGPD_Input);
		UEdGraphPin* ComponentToLocalOutput = FindPosePin(ComponentToLocalNode, EGPD_Output);
		UEdGraphPin* RootInput = FindPosePin(RootNode, EGPD_Input);
		const UEdGraphSchema* Schema = AnimGraph->GetSchema();

		if (!Schema ||
			!Schema->TryCreateConnection(RefPoseOutput, LocalToComponentInput) ||
			!Schema->TryCreateConnection(LocalToComponentOutput, RigidBodyInput) ||
			!Schema->TryCreateConnection(RigidBodyOutput, ComponentToLocalInput) ||
			!Schema->TryCreateConnection(ComponentToLocalOutput, RootInput))
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to connect ABP_Luna_Skirt AnimGraph nodes."));
			return nullptr;
		}

		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(AnimBlueprint);
		FKismetEditorUtilities::CompileBlueprint(AnimBlueprint);
		if (AnimBlueprint->Status == BS_Error || !AnimBlueprint->GeneratedClass)
		{
			UE_LOG(LogTemp, Error, TEXT("ABP_Luna_Skirt failed to compile."));
			return nullptr;
		}

		AnimBlueprint->MarkPackageDirty();
		if (!SaveAsset(AnimBlueprint))
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to save ABP_Luna_Skirt."));
			return nullptr;
		}

		UE_LOG(LogTemp, Display, TEXT("ABP_Luna_Skirt created and compiled with a Rigid Body node."));
		return AnimBlueprint;
	}

	bool AssignAnimBlueprintToPlayerSkirt(UAnimBlueprint* AnimBlueprint, USkeletalMesh* SkirtMesh)
	{
		UBlueprint* PlayerBlueprint = LoadObject<UBlueprint>(nullptr, PlayerBlueprintObjectPath);
		if (!PlayerBlueprint || !PlayerBlueprint->SimpleConstructionScript || !AnimBlueprint || !AnimBlueprint->GeneratedClass)
		{
			UE_LOG(LogTemp, Error, TEXT("Could not load the player Blueprint or Luna skirt AnimBP for assignment."));
			return false;
		}

		USkeletalMeshComponent* SkirtComponentTemplate = nullptr;
		for (USCS_Node* Node : PlayerBlueprint->SimpleConstructionScript->GetAllNodes())
		{
			USkeletalMeshComponent* Candidate = Node ? Cast<USkeletalMeshComponent>(Node->ComponentTemplate) : nullptr;
			if (!Candidate)
			{
				continue;
			}

			const bool bNamedSkirt = Node->GetVariableName() == FName(TEXT("Skirt"));
			const bool bUsesSkirtMesh = Candidate->GetSkeletalMeshAsset() == SkirtMesh;
			if (bNamedSkirt || bUsesSkirtMesh)
			{
				SkirtComponentTemplate = Candidate;
				break;
			}
		}

		if (!SkirtComponentTemplate)
		{
			UE_LOG(LogTemp, Error, TEXT("BP_TunaSweeperPlayerCharacter has no Skirt skeletal mesh component template."));
			return false;
		}

		PlayerBlueprint->Modify();
		SkirtComponentTemplate->Modify();
		SkirtComponentTemplate->SetSkeletalMesh(SkirtMesh);
		SkirtComponentTemplate->SetAnimationMode(EAnimationMode::AnimationBlueprint);
		SkirtComponentTemplate->SetAnimInstanceClass(AnimBlueprint->GeneratedClass);

		FBlueprintEditorUtils::MarkBlueprintAsModified(PlayerBlueprint);
		FKismetEditorUtilities::CompileBlueprint(PlayerBlueprint);
		if (PlayerBlueprint->Status == BS_Error || !PlayerBlueprint->GeneratedClass)
		{
			UE_LOG(LogTemp, Error, TEXT("BP_TunaSweeperPlayerCharacter failed to compile after skirt AnimBP assignment."));
			return false;
		}

		PlayerBlueprint->MarkPackageDirty();
		if (!SaveAsset(PlayerBlueprint))
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to save BP_TunaSweeperPlayerCharacter after skirt AnimBP assignment."));
			return false;
		}

		UE_LOG(LogTemp, Display, TEXT("Assigned ABP_Luna_Skirt to BP_TunaSweeperPlayerCharacter's Skirt component."));
		return true;
	}
}

namespace TunaSweeperLunaSkirtPhysicsSetup
{
	bool Run()
	{
		UE_LOG(LogTemp, Display, TEXT("Starting Luna skirt rigid-body PhysicsAsset and AnimBP setup."));

		USkeletalMesh* SkeletalMesh = LoadObject<USkeletalMesh>(nullptr, SkeletalMeshObjectPath);
		USkeleton* Skeleton = LoadObject<USkeleton>(nullptr, SkeletonObjectPath);
		UPhysicsAsset* PhysicsAsset = LoadObject<UPhysicsAsset>(nullptr, PhysicsAssetObjectPath);
		if (!SkeletalMesh || !Skeleton || !PhysicsAsset)
		{
			UE_LOG(LogTemp, Error, TEXT("Could not load one or more Luna skirt source assets."));
			return false;
		}

		LogReferenceSkeleton(SkeletalMesh);
		if (!RebuildPhysicsAsset(SkeletalMesh, PhysicsAsset))
		{
			return false;
		}

		UAnimBlueprint* AnimBlueprint = EnsureAnimBlueprint(Skeleton, SkeletalMesh);
		if (!AnimBlueprint || !AssignAnimBlueprintToPlayerSkirt(AnimBlueprint, SkeletalMesh))
		{
			return false;
		}

		UE_LOG(LogTemp, Display, TEXT("Luna skirt rigid-body PhysicsAsset and AnimBP setup completed."));
		return true;
	}
}
