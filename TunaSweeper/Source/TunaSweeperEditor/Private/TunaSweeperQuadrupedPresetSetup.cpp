#include "TunaSweeperQuadrupedPresetSetup.h"

#include "AI/TunaSweeperQuadrupedEnemyCharacter.h"
#include "TunaSweeperEditorSetupShared.h"

#include "AnimGraphNode_ComponentToLocalSpace.h"
#include "AnimGraphNode_LocalToComponentSpace.h"
#include "AnimGraphNode_QuadrupedRobotIK.h"
#include "AnimGraphNode_TwoBoneIK.h"
#include "Animation/AnimBlueprint.h"
#include "Animation/Skeleton.h"
#include "AnimationGraph.h"
#include "AnimationGraphSchema.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "Editor.h"
#include "Engine/SkeletalMesh.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "PackageTools.h"
#include "QuadrupedCharacter.h"
#include "QuadrupedComponent.h"
#include "QuadrupedRigProfile.h"
#include "Subsystems/EditorAssetSubsystem.h"

namespace
{
	const TCHAR* AnimBlueprintObjectPath = TEXT("/Game/Characters/Robot/ABP_RobotDog.ABP_RobotDog");
	const TCHAR* BackupAnimBlueprintObjectPath = TEXT("/Game/Characters/Robot/ABP_RobotDog_LegacyTwoBone.ABP_RobotDog_LegacyTwoBone");
	const TCHAR* ProfilePackageName = TEXT("/Game/Characters/Robot/QRP_RobotDog_2Joint");
	const TCHAR* ProfileObjectPath = TEXT("/Game/Characters/Robot/QRP_RobotDog_2Joint.QRP_RobotDog_2Joint");
	const TCHAR* SkeletonObjectPath = TEXT("/Game/Characters/Robot/SKM_Robot_Skeleton.SKM_Robot_Skeleton");
	const TCHAR* SkeletalMeshObjectPath = TEXT("/Game/Characters/Robot/SKM_Robot.SKM_Robot");
	const TCHAR* SourceQuadrupedBlueprintObjectPath = TEXT("/Game/Blueprints/BP_QuadrupedDog.BP_QuadrupedDog");
	const TCHAR* QuadrupedEnemyAssetPath = TEXT("/Game/Blueprints");
	const TCHAR* QuadrupedEnemyAssetName = TEXT("BP_QuadrupedGunEnemy");

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
		TArray<UAnimationGraph*> AnimationGraphs;
		for (UEdGraph* FunctionGraph : AnimBlueprint->FunctionGraphs)
		{
			if (UAnimationGraph* AnimationGraph = Cast<UAnimationGraph>(FunctionGraph))
			{
				if (AnimationGraph->GetFName() == FName(TEXT("AnimGraph")))
				{
					return AnimationGraph;
				}
				AnimationGraphs.Add(AnimationGraph);
			}
		}

		return AnimationGraphs.Num() == 1 ? AnimationGraphs[0] : nullptr;
	}

	bool IsExpectedLegacyIKChain(
		UEdGraphPin* SourcePosePin,
		UEdGraphPin* DestinationPosePin,
		const TArray<UAnimGraphNode_TwoBoneIK*>& ExpectedNodes)
	{
		if (!SourcePosePin || !DestinationPosePin || ExpectedNodes.Num() != 4)
		{
			return false;
		}

		TSet<UAnimGraphNode_TwoBoneIK*> RemainingNodes;
		for (UAnimGraphNode_TwoBoneIK* ExpectedNode : ExpectedNodes)
		{
			RemainingNodes.Add(ExpectedNode);
		}
		UEdGraphPin* CurrentInputPin = DestinationPosePin;
		for (int32 ChainIndex = 0; ChainIndex < ExpectedNodes.Num(); ++ChainIndex)
		{
			if (!CurrentInputPin || CurrentInputPin->LinkedTo.Num() != 1)
			{
				return false;
			}

			UEdGraphPin* UpstreamOutputPin = CurrentInputPin->LinkedTo[0];
			if (!UpstreamOutputPin || UpstreamOutputPin->LinkedTo.Num() != 1)
			{
				return false;
			}

			UAnimGraphNode_TwoBoneIK* UpstreamNode = Cast<UAnimGraphNode_TwoBoneIK>(UpstreamOutputPin->GetOwningNode());
			if (!UpstreamNode || RemainingNodes.Remove(UpstreamNode) != 1)
			{
				return false;
			}

			CurrentInputPin = FindPosePin(UpstreamNode, EGPD_Input);
		}

		return RemainingNodes.IsEmpty() &&
			CurrentInputPin &&
			CurrentInputPin->LinkedTo.Num() == 1 &&
			CurrentInputPin->LinkedTo[0] == SourcePosePin &&
			SourcePosePin->LinkedTo.Num() == 1;
	}

	void RestoreAnimBlueprintFromDisk(UAnimBlueprint* AnimBlueprint)
	{
		if (!AnimBlueprint)
		{
			return;
		}

		FText ReloadError;
		TArray<UPackage*> PackagesToReload{AnimBlueprint->GetOutermost()};
		if (!UPackageTools::ReloadPackages(
			PackagesToReload,
			ReloadError,
			EReloadPackagesInteractionMode::AssumePositive))
		{
			UE_LOG(
				LogTemp,
				Error,
				TEXT("Quadruped preset setup also failed to restore ABP_RobotDog from disk: %s"),
				*ReloadError.ToString());
		}
	}

	bool SaveAsset(UObject* Asset)
	{
		UEditorAssetSubsystem* AssetSubsystem = GEditor ? GEditor->GetEditorSubsystem<UEditorAssetSubsystem>() : nullptr;
		return AssetSubsystem && Asset && AssetSubsystem->SaveLoadedAsset(Asset, false);
	}

	UQuadrupedRigProfile* EnsureRigProfile()
	{
		UQuadrupedRigProfile* Profile = LoadObject<UQuadrupedRigProfile>(nullptr, ProfileObjectPath);
		const bool bCreated = Profile == nullptr;
		if (bCreated)
		{
			UPackage* Package = CreatePackage(ProfilePackageName);
			Profile = NewObject<UQuadrupedRigProfile>(
				Package,
				TEXT("QRP_RobotDog_2Joint"),
				RF_Public | RF_Standalone | RF_Transactional);
		}

		USkeleton* Skeleton = LoadObject<USkeleton>(nullptr, SkeletonObjectPath);
		USkeletalMesh* SkeletalMesh = LoadObject<USkeletalMesh>(nullptr, SkeletalMeshObjectPath);
		if (!Profile || !Skeleton || !SkeletalMesh)
		{
			UE_LOG(LogTemp, Error, TEXT("Quadruped preset setup could not load the RobotDog profile dependencies."));
			return nullptr;
		}

		const bool bNeedsSave =
			bCreated ||
			Profile->TargetSkeleton != Skeleton ||
			Profile->PreviewMesh != SkeletalMesh;
		if (!bNeedsSave)
		{
			return Profile;
		}

		Profile->Modify();
		if (bCreated)
		{
			Profile->ResetToStandardTwoLinkRobot();
		}
		Profile->TargetSkeleton = Skeleton;
		Profile->PreviewMesh = SkeletalMesh;
		Profile->MarkPackageDirty();

		if (bCreated)
		{
			FAssetRegistryModule::AssetCreated(Profile);
		}

		if (!SaveAsset(Profile))
		{
			UE_LOG(LogTemp, Error, TEXT("Quadruped preset setup failed to save %s."), ProfileObjectPath);
			return nullptr;
		}

		return Profile;
	}

	bool EnsureAnimBlueprintBackup(UAnimBlueprint* AnimBlueprint)
	{
		if (LoadObject<UAnimBlueprint>(nullptr, BackupAnimBlueprintObjectPath))
		{
			return true;
		}

		UAnimationGraph* AnimGraph = FindAnimationGraph(AnimBlueprint);
		int32 LegacyTwoBoneNodeCount = 0;
		int32 QuadrupedNodeCount = 0;
		if (AnimGraph)
		{
			for (UEdGraphNode* GraphNode : AnimGraph->Nodes)
			{
				LegacyTwoBoneNodeCount += Cast<UAnimGraphNode_TwoBoneIK>(GraphNode) ? 1 : 0;
				QuadrupedNodeCount += Cast<UAnimGraphNode_QuadrupedRobotIK>(GraphNode) ? 1 : 0;
			}
		}

		if (LegacyTwoBoneNodeCount != 4 || QuadrupedNodeCount != 0)
		{
			UE_LOG(
				LogTemp,
				Error,
				TEXT("Quadruped preset setup cannot create a legacy backup because the source AnimBP is not the expected four-node TwoBoneIK graph."));
			return false;
		}

		FAssetToolsModule& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
		UObject* Backup = AssetTools.Get().DuplicateAsset(
			TEXT("ABP_RobotDog_LegacyTwoBone"),
			TEXT("/Game/Characters/Robot"),
			AnimBlueprint);
		if (!Backup || !SaveAsset(Backup))
		{
			UE_LOG(LogTemp, Error, TEXT("Quadruped preset setup failed to create the legacy AnimBP backup."));
			return false;
		}

		return true;
	}

	bool InstallQuadrupedNode(UAnimBlueprint* AnimBlueprint, UQuadrupedRigProfile* Profile)
	{
		if (!AnimBlueprint || !Profile || AnimBlueprint->FunctionGraphs.Num() == 0)
		{
			return false;
		}

		UAnimationGraph* AnimGraph = FindAnimationGraph(AnimBlueprint);
		if (!AnimGraph)
		{
			UE_LOG(LogTemp, Error, TEXT("Quadruped preset setup could not find ABP_RobotDog's AnimGraph."));
			return false;
		}

		TArray<UAnimGraphNode_TwoBoneIK*> TwoBoneNodes;
		TArray<UAnimGraphNode_QuadrupedRobotIK*> QuadrupedNodes;
		TArray<UAnimGraphNode_LocalToComponentSpace*> LocalToComponentNodes;
		TArray<UAnimGraphNode_ComponentToLocalSpace*> ComponentToLocalNodes;

		for (UEdGraphNode* GraphNode : AnimGraph->Nodes)
		{
			if (UAnimGraphNode_TwoBoneIK* TwoBoneNode = Cast<UAnimGraphNode_TwoBoneIK>(GraphNode))
			{
				TwoBoneNodes.Add(TwoBoneNode);
			}
			else if (UAnimGraphNode_QuadrupedRobotIK* QuadrupedNode = Cast<UAnimGraphNode_QuadrupedRobotIK>(GraphNode))
			{
				QuadrupedNodes.Add(QuadrupedNode);
			}
			else if (UAnimGraphNode_LocalToComponentSpace* LocalToComponentCandidate = Cast<UAnimGraphNode_LocalToComponentSpace>(GraphNode))
			{
				LocalToComponentNodes.Add(LocalToComponentCandidate);
			}
			else if (UAnimGraphNode_ComponentToLocalSpace* ComponentToLocalCandidate = Cast<UAnimGraphNode_ComponentToLocalSpace>(GraphNode))
			{
				ComponentToLocalNodes.Add(ComponentToLocalCandidate);
			}
		}

		bool bModifiedInMemory = false;
		if (QuadrupedNodes.Num() == 1 && TwoBoneNodes.Num() == 0)
		{
			if (QuadrupedNodes[0]->Node.RigProfile == Profile &&
				AnimBlueprint->Status != BS_Error &&
				AnimBlueprint->GeneratedClass)
			{
				return true;
			}

			QuadrupedNodes[0]->Modify();
			QuadrupedNodes[0]->Node.RigProfile = Profile;
			bModifiedInMemory = true;
		}
		else
		{
			if (QuadrupedNodes.Num() != 0 ||
				TwoBoneNodes.Num() != 4 ||
				LocalToComponentNodes.Num() != 1 ||
				ComponentToLocalNodes.Num() != 1)
			{
				UE_LOG(
					LogTemp,
					Error,
					TEXT("Quadruped preset setup expected four TwoBoneIK nodes, one LocalToComponent node, and one ComponentToLocal node."));
				return false;
			}

			UAnimGraphNode_LocalToComponentSpace* LocalToComponentNode = LocalToComponentNodes[0];
			UAnimGraphNode_ComponentToLocalSpace* ComponentToLocalNode = ComponentToLocalNodes[0];
			UEdGraphPin* SourcePosePin = FindPosePin(LocalToComponentNode, EGPD_Output);
			UEdGraphPin* DestinationPosePin = FindPosePin(ComponentToLocalNode, EGPD_Input);
			if (!IsExpectedLegacyIKChain(SourcePosePin, DestinationPosePin, TwoBoneNodes))
			{
				UE_LOG(LogTemp, Error, TEXT("Quadruped preset setup found unexpected links in the legacy IK chain; no graph changes were made."));
				return false;
			}

			AnimGraph->Modify();
			FGraphNodeCreator<UAnimGraphNode_QuadrupedRobotIK> NodeCreator(*AnimGraph);
			UAnimGraphNode_QuadrupedRobotIK* NewNode = NodeCreator.CreateNode();
			NewNode->Node.RigProfile = Profile;
			NewNode->NodePosX = (LocalToComponentNode->NodePosX + ComponentToLocalNode->NodePosX) / 2;
			NewNode->NodePosY = (LocalToComponentNode->NodePosY + ComponentToLocalNode->NodePosY) / 2;
			NodeCreator.Finalize();
			bModifiedInMemory = true;

			UEdGraphPin* NewInputPin = FindPosePin(NewNode, EGPD_Input);
			UEdGraphPin* NewOutputPin = FindPosePin(NewNode, EGPD_Output);
			if (!NewInputPin || !NewOutputPin)
			{
				UE_LOG(LogTemp, Error, TEXT("Quadruped preset setup could not create the custom node pose pins."));
				RestoreAnimBlueprintFromDisk(AnimBlueprint);
				return false;
			}

			SourcePosePin->BreakAllPinLinks();
			DestinationPosePin->BreakAllPinLinks();
			for (UAnimGraphNode_TwoBoneIK* TwoBoneNode : TwoBoneNodes)
			{
				AnimGraph->RemoveNode(TwoBoneNode);
			}

			const UEdGraphSchema* Schema = AnimGraph->GetSchema();
			if (!Schema ||
				!Schema->TryCreateConnection(SourcePosePin, NewInputPin) ||
				!Schema->TryCreateConnection(NewOutputPin, DestinationPosePin))
			{
				UE_LOG(LogTemp, Error, TEXT("Quadruped preset setup failed to connect the custom AnimGraph node."));
				RestoreAnimBlueprintFromDisk(AnimBlueprint);
				return false;
			}
		}

		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(AnimBlueprint);
		FKismetEditorUtilities::CompileBlueprint(AnimBlueprint);
		if (AnimBlueprint->Status == BS_Error || !AnimBlueprint->GeneratedClass)
		{
			UE_LOG(LogTemp, Error, TEXT("Quadruped preset setup produced an AnimBP compile error; the source asset was not saved."));
			if (bModifiedInMemory)
			{
				RestoreAnimBlueprintFromDisk(AnimBlueprint);
			}
			return false;
		}

		AnimBlueprint->MarkPackageDirty();
		if (!SaveAsset(AnimBlueprint))
		{
			UE_LOG(LogTemp, Error, TEXT("Quadruped preset setup failed to save ABP_RobotDog."));
			RestoreAnimBlueprintFromDisk(AnimBlueprint);
			return false;
		}

		return true;
	}

	bool EnsureQuadrupedEnemyBlueprint()
	{
		UBlueprint* EnemyBlueprint = TunaSweeperEditorSetup::EnsureBlueprint(
			QuadrupedEnemyAssetPath,
			QuadrupedEnemyAssetName,
			ATunaSweeperQuadrupedEnemyCharacter::StaticClass());
		if (!EnemyBlueprint || !EnemyBlueprint->GeneratedClass)
		{
			UE_LOG(LogTemp, Error, TEXT("Quadruped enemy setup could not create BP_QuadrupedGunEnemy."));
			return false;
		}

		ATunaSweeperQuadrupedEnemyCharacter* EnemyDefaults =
			Cast<ATunaSweeperQuadrupedEnemyCharacter>(EnemyBlueprint->GeneratedClass->GetDefaultObject());
		if (!EnemyDefaults || !EnemyDefaults->GetMesh() || !EnemyDefaults->GetQuadrupedComponent())
		{
			UE_LOG(LogTemp, Error, TEXT("BP_QuadrupedGunEnemy has incomplete native defaults."));
			return false;
		}

		EnemyDefaults->Modify();
		USkeletalMeshComponent* EnemyMesh = EnemyDefaults->GetMesh();
		UQuadrupedComponent* EnemyQuadruped = EnemyDefaults->GetQuadrupedComponent();
		// BP_QuadrupedDog stores legacy shared effector placeholders (0, 0, -50) in every
		// leg slot. Those are not usable as world-space footfall offsets for this component.
		EnemyQuadruped->InitializeDefaultLegs(60.0f, 30.0f);

		UBlueprint* SourceBlueprint = LoadObject<UBlueprint>(nullptr, SourceQuadrupedBlueprintObjectPath);
		const AQuadrupedCharacter* SourceDefaults = SourceBlueprint && SourceBlueprint->GeneratedClass
			? Cast<AQuadrupedCharacter>(SourceBlueprint->GeneratedClass->GetDefaultObject())
			: nullptr;
		if (SourceDefaults && SourceDefaults->GetMesh())
		{
			USkeletalMeshComponent* SourceMesh = SourceDefaults->GetMesh();
			EnemyMesh->SetSkeletalMeshAsset(SourceMesh->GetSkeletalMeshAsset());
			EnemyMesh->SetAnimationMode(EAnimationMode::AnimationBlueprint);
			EnemyMesh->SetAnimInstanceClass(SourceMesh->GetAnimClass());
			EnemyMesh->SetRelativeTransform(SourceMesh->GetRelativeTransform());

			if (const UCapsuleComponent* SourceCapsule = SourceDefaults->GetCapsuleComponent())
			{
				EnemyDefaults->GetCapsuleComponent()->InitCapsuleSize(
					SourceCapsule->GetUnscaledCapsuleRadius(),
					SourceCapsule->GetUnscaledCapsuleHalfHeight());
			}

			if (const UQuadrupedComponent* SourceQuadruped = SourceDefaults->QuadrupedComponent)
			{
				EnemyQuadruped->GroundCheckDistance = SourceQuadruped->GroundCheckDistance;
				EnemyQuadruped->GroundCheckStartOffset = SourceQuadruped->GroundCheckStartOffset;
			}
		}

		EnemyMesh->SetVisibility(true);
		EnemyMesh->SetHiddenInGame(false);
		EnemyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		EnemyQuadruped->LookAheadSeconds = 0.10f;
		EnemyQuadruped->StepThreshold = 30.0f;
		EnemyQuadruped->StepHeight = 26.0f;
		EnemyQuadruped->StepDuration = 0.12f;
		EnemyQuadruped->MaxStepDistance = 80.0f;
		EnemyQuadruped->GroupStepThresholdScale = 0.35f;
		EnemyQuadruped->MaxLegReach = 150.0f;
		EnemyQuadruped->GroundProbeRadius = 6.0f;
		EnemyQuadruped->MinGroundNormalZ = 0.65f;
		EnemyQuadruped->bMoveGaitGroupTogether = true;
		EnemyQuadruped->bDrawDebug = false;
		EnemyQuadruped->bLogDiagnostics = true;

		EnemyDefaults->MarkPackageDirty();
		EnemyBlueprint->MarkPackageDirty();
		if (!SaveAsset(EnemyBlueprint))
		{
			UE_LOG(LogTemp, Error, TEXT("Quadruped enemy setup failed to save BP_QuadrupedGunEnemy."));
			return false;
		}

		UE_LOG(LogTemp, Display, TEXT("Created /Game/Blueprints/BP_QuadrupedGunEnemy."));
		return true;
	}
}

namespace TunaSweeperQuadrupedPresetSetup
{
	bool Run()
	{
		UE_LOG(LogTemp, Display, TEXT("Starting RobotDog quadruped IK preset setup."));

		UQuadrupedRigProfile* Profile = EnsureRigProfile();
		UAnimBlueprint* AnimBlueprint = LoadObject<UAnimBlueprint>(nullptr, AnimBlueprintObjectPath);
		if (!Profile || !AnimBlueprint || !EnsureAnimBlueprintBackup(AnimBlueprint))
		{
			return false;
		}

		const bool bSucceeded =
			InstallQuadrupedNode(AnimBlueprint, Profile) &&
			EnsureQuadrupedEnemyBlueprint();
		if (bSucceeded)
		{
			UE_LOG(LogTemp, Display, TEXT("RobotDog quadruped IK preset setup completed."));
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("RobotDog quadruped IK preset setup failed."));
		}
		return bSucceeded;
	}
}
