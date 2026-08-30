#pragma once

#include "CoreMinimal.h"

class UAnimBlueprint;
class UPhysicsAsset;
class USkeletalMesh;

namespace ChainPhysicsSetup
{
	enum class ESetupState : uint8
	{
		Unset,
		Partial,
		Complete,
		Conflict
	};

	enum class EPreset : uint8
	{
		Hair,
		Accessory,
		Cloth
	};

	enum class ESetupMode : uint8
	{
		Repair,
		Regenerate
	};

	struct FChainCandidate
	{
		FName RootBone;
		FName EndBone;
		TArray<FName> BoneNames;
		FString DetectionReason;
		float TotalLength = 0.0f;
		int32 Confidence = 0;
		bool bSelected = true;
		ESetupState SetupState = ESetupState::Unset;
		TWeakObjectPtr<UPhysicsAsset> DetectedPhysicsAsset;
	};

	struct FAnimBlueprintCandidate
	{
		TWeakObjectPtr<UAnimBlueprint> AnimBlueprint;
		TWeakObjectPtr<UPhysicsAsset> DetectedPhysicsAsset;
		bool bSelected = false;
		bool bHasConnectedRigidBody = false;
	};

	struct FAnalysisResult
	{
		TWeakObjectPtr<USkeletalMesh> SkeletalMesh;
		TArray<FChainCandidate> Chains;
		TArray<FAnimBlueprintCandidate> AnimBlueprints;
		FString Summary;
	};

	struct FSetupOptions
	{
		EPreset Preset = EPreset::Hair;
		ESetupMode Mode = ESetupMode::Repair;
		float RadiusScale = 1.0f;
		FString OutputPhysicsAssetObjectPath;
	};

	struct FSetupResult
	{
		bool bSucceeded = false;
		bool bModified = false;
		TWeakObjectPtr<UPhysicsAsset> PhysicsAsset;
		FString Message;
	};

	bool AnalyzeSkeletalMesh(USkeletalMesh* SkeletalMesh, FAnalysisResult& OutResult);
	bool AddManualChain(FAnalysisResult& Analysis, FName RootBone, FString& OutError);
	FSetupResult SetupSelectedChains(FAnalysisResult& Analysis, const FSetupOptions& Options);
	FText GetSetupStateText(ESetupState State);
	FLinearColor GetSetupStateColor(ESetupState State);
}
