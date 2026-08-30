#include "TunaSweeperLunaMk2SideTailPhysicsSetup.h"

#include "AnimGraphNode_ComponentToLocalSpace.h"
#include "AnimGraphNode_LocalToComponentSpace.h"
#include "AnimGraphNode_RigidBody.h"
#include "AnimGraphNode_Root.h"
#include "Animation/AnimBlueprint.h"
#include "AnimationGraph.h"
#include "AnimationGraphSchema.h"
#include "AssetToolsModule.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "Editor.h"
#include "Engine/SkeletalMesh.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "PhysicsAssetUtils.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "PhysicsEngine/PhysicsConstraintTemplate.h"
#include "PhysicsEngine/SkeletalBodySetup.h"
#include "Subsystems/EditorAssetSubsystem.h"

namespace
{
	const TCHAR* SkeletalMeshObjectPath = TEXT("/Game/Characters/Player/LunaMk2/SKM_LunaMk2.SKM_LunaMk2");
	const TCHAR* SourcePhysicsAssetObjectPath = TEXT("/Game/Characters/Player/LunaMk2/SKM_LunaMk2_PhysicsAsset.SKM_LunaMk2_PhysicsAsset");
	const TCHAR* SideTailPhysicsAssetPackagePath = TEXT("/Game/Characters/Player/LunaMk2");
	const TCHAR* SideTailPhysicsAssetName = TEXT("PA_LunaMk2_SideTail");
	const TCHAR* SideTailPhysicsAssetObjectPath = TEXT("/Game/Characters/Player/LunaMk2/PA_LunaMk2_SideTail.PA_LunaMk2_SideTail");
	const TCHAR* AnimBlueprintObjectPath = TEXT("/Game/Characters/Player/LunaMk2/Animations/ABP_LunaMk2.ABP_LunaMk2");

	const TArray<FName> LeftSideTailBones = {
		FName(TEXT("sidetail_L_01")),
		FName(TEXT("sidetail_L_002")),
		FName(TEXT("sidetail_L_003")),
		FName(TEXT("sidetail_L_004")),
		FName(TEXT("sidetail_L_005")),
		FName(TEXT("sidetail_L_006"))
	};

	const TArray<FName> RightSideTailBones = {
		FName(TEXT("sidetail_R_01")),
		FName(TEXT("sidetail_R_002")),
		FName(TEXT("sidetail_R_003")),
		FName(TEXT("sidetail_R_004")),
		FName(TEXT("sidetail_R_005")),
		FName(TEXT("sidetail_R_006"))
	};

	const TSet<FName> KinematicCollisionBones = {
		FName(TEXT("head")),
		FName(TEXT("spine_04")),
		FName(TEXT("spine_02"))
	};

	bool SaveAsset(UObject* Asset)
	{
		UEditorAssetSubsystem* AssetSubsystem = GEditor ? GEditor->GetEditorSubsystem<UEditorAssetSubsystem>() : nullptr;
		return AssetSubsystem && Asset && AssetSubsystem->SaveLoadedAsset(Asset, false);
	}

	bool IsSideTailBone(const FName BoneName)
	{
		return LeftSideTailBones.Contains(BoneName) || RightSideTailBones.Contains(BoneName);
	}

	TArray<FName> GetAllSideTailBones()
	{
		TArray<FName> BoneNames = LeftSideTailBones;
		BoneNames.Append(RightSideTailBones);
		return BoneNames;
	}

	bool ValidateSideTailHierarchy(const USkeletalMesh* SkeletalMesh)
	{
		if (!SkeletalMesh)
		{
			return false;
		}

		const FReferenceSkeleton& RefSkeleton = SkeletalMesh->GetRefSkeleton();
		for (const TArray<FName>* Chain : { &LeftSideTailBones, &RightSideTailBones })
		{
			for (int32 ChainIndex = 0; ChainIndex < Chain->Num(); ++ChainIndex)
			{
				const FName BoneName = (*Chain)[ChainIndex];
				const int32 BoneIndex = RefSkeleton.FindBoneIndex(BoneName);
				if (BoneIndex == INDEX_NONE)
				{
					UE_LOG(LogTemp, Error, TEXT("Luna Mk2 side-tail bone is missing: %s"), *BoneName.ToString());
					return false;
				}

				const int32 ParentIndex = RefSkeleton.GetParentIndex(BoneIndex);
				const FName ExpectedParent = ChainIndex == 0 ? FName(TEXT("head")) : (*Chain)[ChainIndex - 1];
				if (ParentIndex == INDEX_NONE || RefSkeleton.GetBoneName(ParentIndex) != ExpectedParent)
				{
					UE_LOG(
						LogTemp,
						Error,
						TEXT("Unexpected Luna Mk2 side-tail hierarchy at %s; expected parent %s."),
						*BoneName.ToString(),
						*ExpectedParent.ToString());
					return false;
				}
			}
		}
		return true;
	}

	UPhysicsAsset* EnsureSideTailPhysicsAsset(UPhysicsAsset* SourcePhysicsAsset)
	{
		UPhysicsAsset* SideTailPhysicsAsset = LoadObject<UPhysicsAsset>(nullptr, SideTailPhysicsAssetObjectPath);
		if (!SideTailPhysicsAsset && SourcePhysicsAsset)
		{
			FAssetToolsModule& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
			SideTailPhysicsAsset = Cast<UPhysicsAsset>(AssetTools.Get().DuplicateAsset(
				SideTailPhysicsAssetName,
				SideTailPhysicsAssetPackagePath,
				SourcePhysicsAsset));
		}

		if (!SideTailPhysicsAsset)
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to create PA_LunaMk2_SideTail."));
		}
		return SideTailPhysicsAsset;
	}

	void ReplaceWithSideTailCollider(USkeletalBodySetup* BodySetup, const USkeletalMesh* SkeletalMesh)
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
			if (RefSkeleton.GetParentIndex(CandidateIndex) == BoneIndex && IsSideTailBone(RefSkeleton.GetBoneName(CandidateIndex)))
			{
				ChildBoneIndex = CandidateIndex;
				break;
			}
		}

		BodySetup->AggGeom.EmptyElements();
		if (ChildBoneIndex != INDEX_NONE)
		{
			const FVector ChildOffset = RefSkeleton.GetRefBonePose()[ChildBoneIndex].GetTranslation();
			const float BoneDistance = ChildOffset.Size();
			if (BoneDistance > UE_SMALL_NUMBER)
			{
				const float Radius = FMath::Clamp(BoneDistance * 0.2f, 1.2f, 2.0f);
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
		EndSphere.Radius = 1.4f;
		BodySetup->AggGeom.SphereElems.Add(EndSphere);
	}

	void ConfigureConstraint(
		UPhysicsAsset* PhysicsAsset,
		UPhysicsConstraintTemplate* Constraint,
		const FName ChildBone,
		const FName ParentBone,
		const bool bRootConstraint)
	{
		Constraint->Modify();
		FConstraintInstance& Instance = Constraint->DefaultInstance;
		Instance.ConstraintBone1 = ChildBone;
		Instance.ConstraintBone2 = ParentBone;
		Instance.SnapTransformsToDefault(EConstraintTransformComponentFlags::All, PhysicsAsset);
		Instance.SetLinearXMotion(LCM_Locked);
		Instance.SetLinearYMotion(LCM_Locked);
		Instance.SetLinearZMotion(LCM_Locked);
		Instance.SetAngularSwing1Limit(ACM_Limited, bRootConstraint ? 18.0f : 30.0f);
		Instance.SetAngularSwing2Limit(ACM_Limited, bRootConstraint ? 22.0f : 36.0f);
		Instance.SetAngularTwistLimit(ACM_Limited, bRootConstraint ? 12.0f : 18.0f);
		Instance.SetAngularDriveMode(EAngularDriveMode::SLERP);
		Instance.SetOrientationDriveSLERP(true);
		Instance.SetAngularVelocityDriveSLERP(true);
		Instance.SetAngularDriveParams(bRootConstraint ? 55.0f : 35.0f, bRootConstraint ? 8.0f : 5.0f, 0.0f);
		Instance.SetAngularDriveAccelerationMode(true);
		Constraint->SetDefaultProfile(Instance);
	}

	bool ConfigureSideTailPhysicsAsset(USkeletalMesh* SkeletalMesh, UPhysicsAsset* PhysicsAsset)
	{
		if (!SkeletalMesh || !PhysicsAsset || !ValidateSideTailHierarchy(SkeletalMesh))
		{
			return false;
		}

		PhysicsAsset->Modify();
		while (PhysicsAsset->ConstraintSetup.Num() > 0)
		{
			FPhysicsAssetUtils::DestroyConstraint(PhysicsAsset, PhysicsAsset->ConstraintSetup.Num() - 1);
		}

		for (int32 BodyIndex = PhysicsAsset->SkeletalBodySetups.Num() - 1; BodyIndex >= 0; --BodyIndex)
		{
			USkeletalBodySetup* BodySetup = PhysicsAsset->SkeletalBodySetups[BodyIndex];
			if (!BodySetup || !KinematicCollisionBones.Contains(BodySetup->BoneName))
			{
				FPhysicsAssetUtils::DestroyBody(PhysicsAsset, BodyIndex);
				continue;
			}

			BodySetup->Modify();
			BodySetup->PhysicsType = PhysType_Kinematic;
			BodySetup->DefaultInstance.SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics, false);
		}

		for (const FName CollisionBone : KinematicCollisionBones)
		{
			if (PhysicsAsset->FindBodyIndex(CollisionBone) == INDEX_NONE)
			{
				UE_LOG(LogTemp, Error, TEXT("PA_LunaMk2_SideTail requires the source body %s."), *CollisionBone.ToString());
				return false;
			}
		}

		FPhysAssetCreateParams CreateParams;
		CreateParams.GeomType = EFG_Sphyl;
		CreateParams.bCreateConstraints = false;
		CreateParams.bDisableCollisionsByDefault = false;
		CreateParams.AngularConstraintMode = ACM_Limited;

		const TArray<FName> SideTailBones = GetAllSideTailBones();
		for (const FName BoneName : SideTailBones)
		{
			const int32 BodyIndex = FPhysicsAssetUtils::CreateNewBody(PhysicsAsset, BoneName, CreateParams);
			USkeletalBodySetup* BodySetup = PhysicsAsset->SkeletalBodySetups.IsValidIndex(BodyIndex)
				? PhysicsAsset->SkeletalBodySetups[BodyIndex]
				: nullptr;
			if (!BodySetup)
			{
				UE_LOG(LogTemp, Error, TEXT("Failed to create side-tail body for %s."), *BoneName.ToString());
				return false;
			}

			BodySetup->Modify();
			BodySetup->PhysicsType = PhysType_Simulated;
			BodySetup->DefaultInstance.LinearDamping = 0.8f;
			BodySetup->DefaultInstance.AngularDamping = 5.0f;
			BodySetup->DefaultInstance.MassScale = 0.2f;
			BodySetup->DefaultInstance.SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics, false);
			ReplaceWithSideTailCollider(BodySetup, SkeletalMesh);
			BodySetup->InvalidatePhysicsData();
			BodySetup->CreatePhysicsMeshes();
		}

		for (const TArray<FName>* Chain : { &LeftSideTailBones, &RightSideTailBones })
		{
			for (int32 ChainIndex = 0; ChainIndex < Chain->Num(); ++ChainIndex)
			{
				const FName ChildBone = (*Chain)[ChainIndex];
				const FName ParentBone = ChainIndex == 0 ? FName(TEXT("head")) : (*Chain)[ChainIndex - 1];
				const int32 ConstraintIndex = FPhysicsAssetUtils::CreateNewConstraint(PhysicsAsset, ChildBone);
				UPhysicsConstraintTemplate* Constraint = PhysicsAsset->ConstraintSetup.IsValidIndex(ConstraintIndex)
					? PhysicsAsset->ConstraintSetup[ConstraintIndex]
					: nullptr;
				if (!Constraint)
				{
					UE_LOG(LogTemp, Error, TEXT("Failed to create side-tail constraint for %s."), *ChildBone.ToString());
					return false;
				}

				ConfigureConstraint(PhysicsAsset, Constraint, ChildBone, ParentBone, ChainIndex == 0);
				PhysicsAsset->DisableCollision(
					PhysicsAsset->FindBodyIndex(ChildBone),
					PhysicsAsset->FindBodyIndex(ParentBone));
			}
		}

		for (int32 FirstIndex = 0; FirstIndex < SideTailBones.Num(); ++FirstIndex)
		{
			for (int32 SecondIndex = FirstIndex + 1; SecondIndex < SideTailBones.Num(); ++SecondIndex)
			{
				PhysicsAsset->DisableCollision(
					PhysicsAsset->FindBodyIndex(SideTailBones[FirstIndex]),
					PhysicsAsset->FindBodyIndex(SideTailBones[SecondIndex]));
			}
		}

		PhysicsAsset->PreviewSkeletalMesh = SkeletalMesh;
		PhysicsAsset->UpdateBodySetupIndexMap();
		PhysicsAsset->UpdateBoundsBodiesArray();
		PhysicsAsset->RefreshPhysicsAssetChange();
		PhysicsAsset->MarkPackageDirty();

		if (!SaveAsset(PhysicsAsset))
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to save PA_LunaMk2_SideTail."));
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
			TEXT("Luna Mk2 side-tail PhysicsAsset configured: %d kinematic bodies, %d simulated bodies, %d constraints."),
			KinematicCount,
			SimulatedCount,
			PhysicsAsset->ConstraintSetup.Num());
		return KinematicCount == KinematicCollisionBones.Num()
			&& SimulatedCount == SideTailBones.Num()
			&& PhysicsAsset->ConstraintSetup.Num() == SideTailBones.Num();
	}

	UEdGraphPin* FindPosePin(UEdGraphNode* Node, const EEdGraphPinDirection Direction)
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

	void ConfigureRigidBodyNode(UAnimGraphNode_RigidBody* RigidBodyNode, UPhysicsAsset* PhysicsAsset)
	{
		RigidBodyNode->Modify();
		RigidBodyNode->Node.OverridePhysicsAsset = PhysicsAsset;
		RigidBodyNode->Node.bDefaultToSkeletalMeshPhysicsAsset = false;
		RigidBodyNode->Node.bUseDefaultAsSimulated = false;
		RigidBodyNode->Node.SimulationSpace = ESimulationSpace::ComponentSpace;
		RigidBodyNode->Node.ComponentLinearAccScale = FVector::ZeroVector;
		RigidBodyNode->Node.ComponentLinearVelScale = FVector::ZeroVector;
		RigidBodyNode->Node.ComponentAppliedLinearAccClamp = FVector::ZeroVector;
		RigidBodyNode->Node.SimSpaceSettings.WorldAlpha = 0.8f;
		RigidBodyNode->Node.SimSpaceSettings.VelocityScaleZ = 0.75f;
		RigidBodyNode->Node.SimSpaceSettings.DampingAlpha = 1.0f;
		RigidBodyNode->Node.SimSpaceSettings.MaxLinearVelocity = 800.0f;
		RigidBodyNode->Node.SimSpaceSettings.MaxAngularVelocity = 10.0f;
		RigidBodyNode->Node.SimSpaceSettings.MaxLinearAcceleration = 3000.0f;
		RigidBodyNode->Node.SimSpaceSettings.MaxAngularAcceleration = 100.0f;
		RigidBodyNode->Node.CachedBoundsScale = 1.25f;
		RigidBodyNode->Node.bEnableWorldGeometry = false;
		RigidBodyNode->Node.bUseExternalClothCollision = false;
		RigidBodyNode->Node.bTransferBoneVelocities = true;
		RigidBodyNode->Node.bForceDisableCollisionBetweenConstraintBodies = true;
		RigidBodyNode->Node.EvaluationResetTime = 0.5f;
		RigidBodyNode->Node.Alpha = 1.0f;
		RigidBodyNode->NodeComment = TEXT("Luna Mk2 side-tail rigid-body simulation");
	}

	bool ConfigureAnimBlueprint(UAnimBlueprint* AnimBlueprint, UPhysicsAsset* PhysicsAsset)
	{
		if (!AnimBlueprint || !PhysicsAsset)
		{
			return false;
		}

		UAnimationGraph* AnimGraph = FindAnimationGraph(AnimBlueprint);
		UAnimGraphNode_Root* RootNode = AnimGraph ? FBlueprintEditorUtils::GetAnimGraphRoot(AnimGraph) : nullptr;
		if (!AnimGraph || !RootNode)
		{
			UE_LOG(LogTemp, Error, TEXT("ABP_LunaMk2 has no usable AnimGraph root."));
			return false;
		}

		AnimBlueprint->Modify();
		AnimGraph->Modify();
		for (UEdGraphNode* GraphNode : AnimGraph->Nodes)
		{
			if (UAnimGraphNode_RigidBody* ExistingRigidBodyNode = Cast<UAnimGraphNode_RigidBody>(GraphNode))
			{
				if (ExistingRigidBodyNode->Node.OverridePhysicsAsset == PhysicsAsset)
				{
					ConfigureRigidBodyNode(ExistingRigidBodyNode, PhysicsAsset);
					FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(AnimBlueprint);
					FKismetEditorUtilities::CompileBlueprint(AnimBlueprint);
					AnimBlueprint->MarkPackageDirty();
					return AnimBlueprint->Status != BS_Error && AnimBlueprint->GeneratedClass && SaveAsset(AnimBlueprint);
				}
			}
		}

		UEdGraphPin* RootInput = FindPosePin(RootNode, EGPD_Input);
		if (!RootInput || RootInput->LinkedTo.Num() != 1)
		{
			UE_LOG(LogTemp, Error, TEXT("ABP_LunaMk2 root pose must have exactly one incoming connection."));
			return false;
		}

		UEdGraphPin* ExistingPoseOutput = RootInput->LinkedTo[0];
		ExistingPoseOutput->BreakLinkTo(RootInput);

		FGraphNodeCreator<UAnimGraphNode_LocalToComponentSpace> LocalToComponentCreator(*AnimGraph);
		UAnimGraphNode_LocalToComponentSpace* LocalToComponentNode = LocalToComponentCreator.CreateNode();
		LocalToComponentNode->NodePosX = RootNode->NodePosX - 650;
		LocalToComponentNode->NodePosY = RootNode->NodePosY;
		LocalToComponentCreator.Finalize();

		FGraphNodeCreator<UAnimGraphNode_RigidBody> RigidBodyCreator(*AnimGraph);
		UAnimGraphNode_RigidBody* RigidBodyNode = RigidBodyCreator.CreateNode();
		RigidBodyNode->NodePosX = RootNode->NodePosX - 400;
		RigidBodyNode->NodePosY = RootNode->NodePosY;
		ConfigureRigidBodyNode(RigidBodyNode, PhysicsAsset);
		RigidBodyCreator.Finalize();

		FGraphNodeCreator<UAnimGraphNode_ComponentToLocalSpace> ComponentToLocalCreator(*AnimGraph);
		UAnimGraphNode_ComponentToLocalSpace* ComponentToLocalNode = ComponentToLocalCreator.CreateNode();
		ComponentToLocalNode->NodePosX = RootNode->NodePosX - 150;
		ComponentToLocalNode->NodePosY = RootNode->NodePosY;
		ComponentToLocalCreator.Finalize();

		UEdGraphPin* LocalToComponentInput = FindPosePin(LocalToComponentNode, EGPD_Input);
		UEdGraphPin* LocalToComponentOutput = FindPosePin(LocalToComponentNode, EGPD_Output);
		UEdGraphPin* RigidBodyInput = FindPosePin(RigidBodyNode, EGPD_Input);
		UEdGraphPin* RigidBodyOutput = FindPosePin(RigidBodyNode, EGPD_Output);
		UEdGraphPin* ComponentToLocalInput = FindPosePin(ComponentToLocalNode, EGPD_Input);
		UEdGraphPin* ComponentToLocalOutput = FindPosePin(ComponentToLocalNode, EGPD_Output);
		const UEdGraphSchema* Schema = AnimGraph->GetSchema();

		if (!Schema
			|| !Schema->TryCreateConnection(ExistingPoseOutput, LocalToComponentInput)
			|| !Schema->TryCreateConnection(LocalToComponentOutput, RigidBodyInput)
			|| !Schema->TryCreateConnection(RigidBodyOutput, ComponentToLocalInput)
			|| !Schema->TryCreateConnection(ComponentToLocalOutput, RootInput))
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to insert the Luna Mk2 side-tail Rigid Body chain into ABP_LunaMk2."));
			return false;
		}

		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(AnimBlueprint);
		FKismetEditorUtilities::CompileBlueprint(AnimBlueprint);
		if (AnimBlueprint->Status == BS_Error || !AnimBlueprint->GeneratedClass)
		{
			UE_LOG(LogTemp, Error, TEXT("ABP_LunaMk2 failed to compile after side-tail physics setup."));
			return false;
		}

		AnimBlueprint->MarkPackageDirty();
		if (!SaveAsset(AnimBlueprint))
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to save ABP_LunaMk2 after side-tail physics setup."));
			return false;
		}

		UE_LOG(LogTemp, Display, TEXT("ABP_LunaMk2 now evaluates PA_LunaMk2_SideTail through a Rigid Body node."));
		return true;
	}
}

namespace TunaSweeperLunaMk2SideTailPhysicsSetup
{
	bool Run()
	{
		UE_LOG(LogTemp, Display, TEXT("Starting Luna Mk2 side-tail PhysicsAsset and AnimBP setup."));

		USkeletalMesh* SkeletalMesh = LoadObject<USkeletalMesh>(nullptr, SkeletalMeshObjectPath);
		UPhysicsAsset* SourcePhysicsAsset = LoadObject<UPhysicsAsset>(nullptr, SourcePhysicsAssetObjectPath);
		UAnimBlueprint* AnimBlueprint = LoadObject<UAnimBlueprint>(nullptr, AnimBlueprintObjectPath);
		if (!SkeletalMesh || !SourcePhysicsAsset || !AnimBlueprint)
		{
			UE_LOG(LogTemp, Error, TEXT("Could not load one or more Luna Mk2 side-tail source assets."));
			return false;
		}

		UPhysicsAsset* SideTailPhysicsAsset = EnsureSideTailPhysicsAsset(SourcePhysicsAsset);
		if (!SideTailPhysicsAsset
			|| !ConfigureSideTailPhysicsAsset(SkeletalMesh, SideTailPhysicsAsset)
			|| !ConfigureAnimBlueprint(AnimBlueprint, SideTailPhysicsAsset))
		{
			return false;
		}

		UE_LOG(LogTemp, Display, TEXT("Luna Mk2 side-tail PhysicsAsset and AnimBP setup completed."));
		return true;
	}
}
