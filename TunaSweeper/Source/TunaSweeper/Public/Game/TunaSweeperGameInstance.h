#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Effect/TunaSweeperProjectileHitEffectDataAsset.h"
#include "Game/TunaSweeperDataValueTypes.h"
#include "Inventory/TunaSweeperInventoryTypes.h"
#include "Inventory/TunaSweeperSaveGame.h"
#include "Subsystem/TunaSweeperItemDataSubsystem.h"
#include "Weapon/TunaSweeperWeaponSpreadRecoilDataAsset.h"
#include "TunaSweeperGameInstance.generated.h"

class APawn;
class ATunaSweeperPetCompanionCharacter;
class UTunaSweeperFootstepPresentationDataAsset;
class UTunaSweeperOcclusionRevealSettingsDataAsset;
class UTunaSweeperVitalsComponent;
class UTunaSweeperWeaponPresentationDataAsset;

USTRUCT(BlueprintType)
struct TUNASWEEPER_API FTunaSweeperGameplaySettings
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "TunaSweeper|Gameplay")
	float InteractionTraceDistance = 500.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "TunaSweeper|Gameplay")
	int32 BareInventorySlots = 40;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "TunaSweeper|Gameplay")
	int32 MaxInventorySlots = 100;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "TunaSweeper|Gameplay")
	int32 EquipmentSlotCount = 8;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "TunaSweeper|Gameplay")
	int32 AuxiliaryBagSlotCount = 2;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "TunaSweeper|Gameplay")
	int32 DefaultStorageSlotCount = 100;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "TunaSweeper|Gameplay")
	int32 MaxStorageSlotCount = 1000;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "TunaSweeper|Gameplay")
	bool bEnableDebugGameplay = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "TunaSweeper|Gameplay|Debug")
	bool bEnableVisionDebug = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "TunaSweeper|Gameplay|Debug")
	bool bEnableBunkerVisionDebug = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "TunaSweeper|Gameplay", meta = (ClampMin = "0.1", UIMin = "0.1"))
	float DialogueCharactersPerSecond = 18.0f;
};

USTRUCT(BlueprintType)
struct TUNASWEEPER_API FTunaSweeperSaveSlotSummary
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Save")
	int32 SaveSlotIndex = 1;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Save")
	bool bHasData = false;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Save")
	float TotalPlaySeconds = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Save")
	int32 DifficultyStage = 1;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Save")
	bool bDifficultySelected = false;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Save")
	int64 LastSavedAtTicks = 0;
};

USTRUCT(BlueprintType)
struct TUNASWEEPER_API FTunaSweeperExperienceAnimationState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Experience")
	int64 StartExperiencePoints = 0;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Experience")
	int64 TargetExperiencePoints = 0;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Experience")
	int64 GainedExperiencePoints = 0;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Experience")
	int32 StartLevel = 1;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Experience")
	int32 TargetLevel = 1;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Experience")
	float AnimationDurationSeconds = 3.2f;
};

USTRUCT(BlueprintType)
struct TUNASWEEPER_API FTunaSweeperExperienceLevelStatBonuses
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Experience", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float MaxHealthBonus = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Experience", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float MaxFoodBonus = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Experience", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float MaxHydrationBonus = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Experience", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float MaxStaminaBonus = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Experience", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float CarryStrengthBonus = 0.0f;

	void ClampNonNegative();
};

USTRUCT(BlueprintType)
struct TUNASWEEPER_API FTunaSweeperExperienceLevelReward
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Experience", meta = (ClampMin = "2", UIMin = "2"))
	int32 Level = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Experience", meta = (ClampMin = "0", UIMin = "0"))
	int32 MaxHealthIncrease = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Experience", meta = (ClampMin = "0", UIMin = "0"))
	int32 MaxFoodIncrease = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Experience", meta = (ClampMin = "0", UIMin = "0"))
	int32 MaxHydrationIncrease = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Experience", meta = (ClampMin = "0", UIMin = "0"))
	int32 MaxStaminaIncrease = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Experience", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float CarryStrengthIncrease = 0.0f;

	void Normalize();
};

USTRUCT(BlueprintType)
struct TUNASWEEPER_API FTunaSweeperPlayerHudState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|HUD", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float CurrentCarryWeight = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|HUD", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float MaxCarryWeight = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|HUD", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float OverweightCarryWeight = 35.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|HUD", meta = (ClampMin = "0", UIMin = "0"))
	int32 OverweightThreshold = 7000;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|HUD", meta = (ClampMin = "0", UIMin = "0"))
	int32 OverweightSpeedMultiplier = 5000;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|HUD", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float MovementBlockedWeight = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|HUD", meta = (ClampMin = "0.0", ClampMax = "100.0", UIMin = "0.0", UIMax = "100.0"))
	float Health = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|HUD", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float MaxHealth = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|HUD", meta = (ClampMin = "0.0", ClampMax = "100.0", UIMin = "0.0", UIMax = "100.0"))
	float Food = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|HUD", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float MaxFood = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|HUD", meta = (ClampMin = "0.0", ClampMax = "100.0", UIMin = "0.0", UIMax = "100.0"))
	float Hydration = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|HUD", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float MaxHydration = 100.0f;

	void NormalizeWeightLimits();
	bool IsCarryWeightOverLimit() const;
	bool IsCarryWeightMovementBlocked() const;
	float GetCarryWeightMovementSpeedMultiplier() const;
	float GetOverweightThresholdRatio() const;
};

USTRUCT(BlueprintType)
struct TUNASWEEPER_API FTunaSweeperWorkbenchDismantleCandidateView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Workbench")
	FTunaSweeperItemSlotReference SlotReference;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Workbench")
	int32 ListIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Workbench")
	int32 ItemId = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Workbench", meta = (ClampMin = "1", UIMin = "1"))
	int32 Quantity = 1;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Workbench")
	TArray<FTunaSweeperItemStack> Results;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Workbench")
	bool bCanDismantle = false;
};

USTRUCT(BlueprintType)
struct TUNASWEEPER_API FTunaSweeperWorkbenchBlueprintItemView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Workbench")
	FTunaSweeperItemSlotReference SlotReference;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Workbench")
	int32 ListIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Workbench")
	int32 ItemId = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Workbench", meta = (ClampMin = "1", UIMin = "1"))
	int32 Quantity = 1;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Workbench")
	FName RecipeId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Workbench")
	bool bRecipeKnown = false;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Workbench")
	bool bAlreadyUnlocked = false;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Workbench")
	bool bCanRegister = false;
};

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API UTunaSweeperGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "TunaSweeper|Settings")
	FTunaSweeperGameplaySettings GameplaySettings;

	UPROPERTY(BlueprintReadWrite, Category = "TunaSweeper|State")
	TMap<FName, FString> GameplayInfo;

	UPROPERTY(BlueprintReadWrite, Category = "TunaSweeper|State")
	TMap<FName, float> NumberSettings;

	UPROPERTY(BlueprintReadWrite, Category = "TunaSweeper|State")
	TMap<FName, bool> BoolSettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|HUD")
	FTunaSweeperPlayerHudState PlayerHudState;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TunaSweeper|Projectile Hit Effect")
	TSoftObjectPtr<UTunaSweeperProjectileHitEffectDataAsset> ProjectileHitEffectDataAsset;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TunaSweeper|Weapon Spread Recoil")
	TSoftObjectPtr<UTunaSweeperWeaponSpreadRecoilDataAsset> WeaponSpreadRecoilDataAsset;

	/** Shared presentation used by enemies whose weapon class has no presentation configured. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TunaSweeper|Enemy Weapon Presentation")
	TSoftObjectPtr<UTunaSweeperWeaponPresentationDataAsset> EnemyWeaponFallbackPresentationDataAsset;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TunaSweeper|Footstep Presentation")
	TSoftObjectPtr<UTunaSweeperFootstepPresentationDataAsset> FootstepPresentationDataAsset;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TunaSweeper|Occlusion Reveal")
	TSoftObjectPtr<UTunaSweeperOcclusionRevealSettingsDataAsset> OcclusionRevealSettingsDataAsset;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Occlusion Reveal")
	UTunaSweeperOcclusionRevealSettingsDataAsset* GetOcclusionRevealSettingsDataAsset() const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TunaSweeper|Pet Companion")
	TSubclassOf<ATunaSweeperPetCompanionCharacter> PetCompanionClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TunaSweeper|Pet Companion", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float PetCompanionSpawnDistance = 200.0f;

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Gameplay Info")
	void SetGameplayInfo(FName Key, const FString& Value);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Gameplay Info")
	bool TryGetGameplayInfo(FName Key, FString& OutValue) const;

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Settings")
	void SetNumberSetting(FName Key, float Value);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Settings")
	bool TryGetNumberSetting(FName Key, float& OutValue) const;

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Settings")
	void SetBoolSetting(FName Key, bool bValue);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Settings")
	bool TryGetBoolSetting(FName Key, bool& bOutValue) const;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Dialogue")
	float GetDialogueCharactersPerSecond() const;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Interface")
	ETunaSweeperItemTextLanguage GetCurrentTextLanguage() const { return CurrentTextLanguage; }

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Interface")
	void SetCurrentTextLanguage(ETunaSweeperItemTextLanguage Language, bool bSaveImmediately = true);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Interface")
	FText ResolveLocalizedText(FName StringKey, const FText& FallbackText) const;

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Debug")
	void SetVisionDebugEnabled(bool bEnabled);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Debug")
	bool IsVisionDebugEnabled() const;

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Debug")
	void SetBunkerVisionDebugEnabled(bool bEnabled);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Debug")
	bool IsBunkerVisionDebugEnabled() const;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Projectile Hit Effect")
	bool TryGetProjectileHitEffectDefinition(
		FName EffectId,
		FTunaSweeperProjectileHitEffectDefinition& OutDefinition) const;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Weapon Spread Recoil")
	bool TryGetWeaponSpreadRecoilDefinition(
		FName WeaponTypeTag,
		FTunaSweeperWeaponSpreadRecoilDefinition& OutDefinition) const;

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|State")
	void ClearRuntimeState();

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Pet Companion")
	ATunaSweeperPetCompanionCharacter* SpawnPetCompanionForPlayer(bool bReplaceExisting = true);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Pet Companion")
	ATunaSweeperPetCompanionCharacter* SpawnPetCompanionForPawn(APawn* TargetPawn, bool bReplaceExisting = true);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Pet Companion")
	void DespawnPetCompanion();

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Pet Companion")
	ATunaSweeperPetCompanionCharacter* GetCurrentPetCompanion() const { return CurrentPetCompanion.Get(); }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Save")
	int32 GetActiveSaveSlotIndex() const { return ActiveSaveSlotIndex; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Save")
	FTunaSweeperSaveSlotSummary GetSaveSlotSummary(int32 SaveSlotIndex) const;

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Save")
	bool SetActiveSaveSlotIndex(int32 SaveSlotIndex);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Save")
	bool ActivateSaveSlot(int32 SaveSlotIndex, bool bStartNewGame);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Save")
	int32 GetActiveSaveSlotDifficultyStage() const { return ActiveSaveSlotDifficultyStage; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Save")
	bool IsActiveSaveSlotDifficultySelected() const { return bActiveSaveSlotDifficultySelected; }

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Save")
	bool SetActiveSaveSlotDifficultyStage(int32 DifficultyStage, bool bSaveImmediately = true);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Save")
	bool DeleteSaveSlot(int32 SaveSlotIndex);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Save")
	bool DeleteSaveSlotAndStartNewGame(int32 SaveSlotIndex);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Scenario")
	bool IsScenarioProgressFlagSet(FName ScenarioFlag) const;

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Scenario")
	void MarkScenarioProgressFlag(FName ScenarioFlag, bool bSaveImmediately = true);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Scenario")
	FName ResolveInitialGameplayLevelName();

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Scenario")
	void BeginScenarioBunkerEntry(FName ScenarioFlag);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Scenario")
	bool CompletePendingScenarioBunkerEntryIfNeeded();

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Memo")
	bool IsMemoAcquired(int32 MemoId);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Memo")
	bool MarkMemoAcquired(int32 MemoId, bool bSaveImmediately = false);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Memo")
	void GetAcquiredMemoIds(TArray<int32>& OutMemoIds);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Inventory")
	bool HasEverAcquiredItem(int32 ItemId);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Map")
	void GetMapMarkers(TArray<FTunaSweeperMapMarkerSaveData>& OutMapMarkers);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Map")
	int32 AddMapMarker(
		const FVector2D& MapPosition,
		int32 MarkerIconIndex,
		int32 MarkerColorIndex,
		bool bSaveImmediately = false);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Map")
	bool RemoveMapMarker(int32 MarkerId, bool bSaveImmediately = false);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Experience")
	int64 GetTotalExperiencePoints() const { return TotalExperiencePoints; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Experience")
	int32 GetCurrentExperienceLevel() const;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Experience")
	int32 GetMaxExperienceLevel() const;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Experience")
	int32 GetExperienceLevelForTotal(int64 ExperiencePoints) const;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Experience")
	int64 GetExperienceForLevel(int32 Level) const;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Experience")
	int64 GetExperienceForNextLevel(int32 Level) const;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Experience")
	float GetExperienceProgressForTotal(int64 ExperiencePoints) const;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Experience")
	FTunaSweeperExperienceLevelStatBonuses GetExperienceLevelStatBonuses(int32 Level) const;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Experience")
	FTunaSweeperExperienceLevelStatBonuses GetCurrentExperienceLevelStatBonuses() const;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Experience")
	bool HasPendingRaidExperienceAnimationState() const { return bHasPendingRaidExperienceAnimationState; }

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Experience")
	void BeginRaidExperienceSession();

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Experience")
	int32 AddRaidExperience(int32 ExperienceAmount);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Experience")
	int32 AddRaidExperienceForItem(int32 ItemId, int32 Quantity = 1);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Experience")
	void ClearRaidExperienceGain();

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Experience")
	bool CommitRaidExperienceGain(FTunaSweeperExperienceAnimationState& OutAnimationState);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Experience")
	bool ConsumePendingRaidExperienceAnimationState(FTunaSweeperExperienceAnimationState& OutAnimationState);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|HUD")
	void SetPlayerHudState(const FTunaSweeperPlayerHudState& InHudState);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|HUD")
	void SetCarryWeight(float CurrentCarryWeight, float MaxCarryWeight, float MovementBlockedWeight);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|HUD")
	void RefreshCarryWeightState();

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|HUD")
	float GetCarryWeightMovementSpeedMultiplier() const;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|HUD")
	bool IsCarryWeightOverLimit() const;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|HUD")
	bool IsCarryWeightMovementBlocked() const;

	const TArray<FTunaSweeperItemStack>& GetOrCreatePlayerInventoryItems();

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Inventory")
	void GetPlayerInventoryItems(TArray<FTunaSweeperItemStack>& OutItems);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Inventory")
	int32 GetCurrentInventorySlotCapacity();

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Inventory")
	int32 GetEquippedBackpackSlotBonus();

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Inventory")
	int32 GetEquippedDefenseValue();

	const TArray<FTunaSweeperInventorySlot>& GetInventorySlots();
	const TArray<FTunaSweeperInventorySlot>& GetEquipmentSlots();
	const TArray<FTunaSweeperInventorySlot>& GetAuxiliaryBagSlots();
	const TArray<FTunaSweeperInventorySlot>& GetUsableQuickSlots();
	const TArray<FTunaSweeperInventorySlot>& GetStorageSlots();
	const TArray<FTunaSweeperInventorySlot>& GetActiveLootContainerSlots();
	const TArray<FTunaSweeperInventorySlot>& GetSelectedWeaponAttachmentSlots();
	const TArray<FName>& GetSelectedWeaponAttachmentSlotTags() const { return SelectedWeaponAttachmentSlotTags; }
	bool HasActiveLootContainer() const { return bHasActiveLootContainer; }
	UObject* GetActiveLootContainerOwner() const { return ActiveLootContainerOwner.Get(); }
	FText GetActiveLootContainerDisplayName() const { return ActiveLootContainerDisplayName; }
	int32 GetActiveLootContainerCapacity() const { return ActiveLootContainerCapacity; }
	bool HasSelectedInventoryItem() const { return SelectedItemSlotReference.IsValid(); }
	FTunaSweeperItemSlotReference GetSelectedItemSlotReference() const { return SelectedItemSlotReference; }
	bool HasHoveredItemSlot() const { return HoveredItemSlotReference.IsValid(); }
	FTunaSweeperItemSlotReference GetHoveredItemSlotReference() const { return HoveredItemSlotReference; }

	bool TryGetItemInstance(const FGuid& ItemUid, FTunaSweeperItemInstance& OutItemInstance) const;
	bool TryGetSlotItemUid(const FTunaSweeperItemSlotReference& SlotReference, FGuid& OutItemUid);
	bool TryGetSlotItemInstance(const FTunaSweeperItemSlotReference& SlotReference, FTunaSweeperItemInstance& OutItemInstance);
	bool TryGetSelectedItemInstance(FTunaSweeperItemInstance& OutItemInstance);
	bool TryGetSelectedItemDefinition(FTunaSweeperItemDefinition& OutItemDefinition);
	bool ToggleInventorySlotSortLock(const FTunaSweeperItemSlotReference& SlotReference);
	bool ToggleHoveredInventorySlotSortLock();
	bool IsEquipmentWeaponSlotOccupied(int32 WeaponSlotNumber);
	bool TryGetEquipmentWeaponSlotItem(
		int32 WeaponSlotNumber,
		FTunaSweeperItemInstance& OutItemInstance,
		FTunaSweeperItemDefinition& OutItemDefinition);
	bool IsEquipmentMeleeSlotOccupied();
	bool TryGetEquipmentMeleeSlotItem(
		FTunaSweeperItemInstance& OutItemInstance,
		FTunaSweeperItemDefinition& OutItemDefinition);
	void SetRuntimeSelectedWeaponSlotNumber(int32 WeaponSlotNumber);
	void SetRuntimeSelectedMeleeWeapon();
	bool TryGetRuntimeSelectedWeaponSelection(bool& bOutMeleeWeaponSelected, int32& OutWeaponSlotNumber) const;
	int32 GetWeaponLoadedAmmoCount(int32 WeaponSlotNumber);
	int32 GetWeaponMagazineCapacity(int32 WeaponSlotNumber);
	int32 GetWeaponInventoryAmmoCount(int32 WeaponSlotNumber);
	int32 GetWeaponSelectedAmmoItemId(int32 WeaponSlotNumber);
	float GetWeaponReloadSeconds(int32 WeaponSlotNumber);
	void GetCompatibleAmmoItemIdsForWeaponSlot(
		int32 WeaponSlotNumber,
		TArray<int32>& OutAmmoItemIds,
		bool bRequireInventoryAmmo);
	bool SetSelectedAmmoItemForWeaponSlot(int32 WeaponSlotNumber, int32 AmmoItemId);
	bool TryConsumeLoadedAmmoForWeaponSlot(int32 WeaponSlotNumber);
	bool TryReloadWeaponSlot(int32 WeaponSlotNumber, int32 AmmoItemId, int32& OutLoadedAmmoCount);
	bool CanSlotAcceptItem(const FTunaSweeperItemSlotReference& SlotReference, const FGuid& ItemUid);
	bool CanUseItemInSlot(const FTunaSweeperItemSlotReference& SlotReference, APawn* InstigatorPawn);
	float GetItemUseSecondsInSlot(const FTunaSweeperItemSlotReference& SlotReference);
	bool TryUseItemInSlot(const FTunaSweeperItemSlotReference& SlotReference, APawn* InstigatorPawn);
	bool TryUseHoveredItem(APawn* InstigatorPawn);
	bool CanStackItemBetweenSlots(
		const FTunaSweeperItemSlotReference& SourceSlot,
		const FTunaSweeperItemSlotReference& TargetSlot,
		FString* OutFailureReason = nullptr);
	bool TryFindFirstStackTargetSlot(
		const FTunaSweeperItemSlotReference& SourceSlot,
		ETunaSweeperItemSlotSource TargetSource,
		FTunaSweeperItemSlotReference& OutTargetSlot);
	bool CanMoveItemBetweenSlots(
		const FTunaSweeperItemSlotReference& SourceSlot,
		const FTunaSweeperItemSlotReference& TargetSlot,
		FString* OutFailureReason = nullptr);
	bool MoveItemBetweenSlots(
		const FTunaSweeperItemSlotReference& SourceSlot,
		const FTunaSweeperItemSlotReference& TargetSlot);
	bool CanSplitItemStackBetweenSlots(
		const FTunaSweeperItemSlotReference& SourceSlot,
		const FTunaSweeperItemSlotReference& TargetSlot,
		int32& OutDefaultSplitQuantity,
		int32& OutMaxSplitQuantity,
		FString* OutFailureReason = nullptr);
	bool SplitItemStackBetweenSlots(
		const FTunaSweeperItemSlotReference& SourceSlot,
		const FTunaSweeperItemSlotReference& TargetSlot,
		int32 SplitQuantity);
	bool RemoveItemFromSlot(
		const FTunaSweeperItemSlotReference& SlotReference,
		FTunaSweeperItemInstance& OutRemovedItemInstance);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Inventory")
	bool AddItemToFirstAvailableInventorySlot(int32 ItemId, int32 Quantity = 1);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Inventory")
	bool AddItemToPreferredAvailableSlot(int32 ItemId, int32 Quantity = 1);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Inventory")
	int32 CountInventoryItemById(int32 ItemId);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Inventory")
	int32 ConsumeInventoryItemById(int32 ItemId, int32 RequestedAmount);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Quest")
	bool GrantQuestItemRewards(const TArray<FTunaSweeperItemStack>& ItemRewards);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Inventory")
	void CompactInventorySlots();

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Storage")
	void CompactStorageSlots();

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Storage")
	int32 GetStorageSlotCapacity();

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Storage")
	bool SetStorageSlotCapacity(int32 NewCapacity, bool bSaveImmediately = false);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Shop")
	bool HasActiveShop() const { return bHasActiveShop; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Shop")
	int32 GetActiveShopId() const { return bHasActiveShop ? ActiveShopId : INDEX_NONE; }

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Shop")
	void SetActiveShop(int32 ShopId);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Shop")
	void ClearActiveShop();

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Shop")
	bool GetActiveShopItems(TArray<FTunaSweeperShopItemView>& OutShopItems);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Shop")
	bool TryGetActiveShopItemView(int32 ShopSlotIndex, FTunaSweeperShopItemView& OutShopItem);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Shop")
	bool TryBuyActiveShopSlot(int32 ShopSlotIndex);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Shop|Debug")
	bool DebugRestockActiveShop(bool bSaveImmediately = true);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Shop")
	bool TryGetSlotSellPrice(const FTunaSweeperItemSlotReference& SlotReference, int32& OutSalePrice);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Shop")
	bool TrySellItemInSlot(const FTunaSweeperItemSlotReference& SlotReference, int32& OutSalePrice);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Workbench")
	bool HasActiveWorkbench() const { return bHasActiveWorkbench; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Workbench")
	int32 GetActiveWorkbenchId() const { return bHasActiveWorkbench ? ActiveWorkbenchId : INDEX_NONE; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Workbench")
	ETunaSweeperWorkbenchMode GetActiveWorkbenchMode() const { return ActiveWorkbenchMode; }

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Workbench")
	void SetActiveWorkbench(int32 WorkbenchId, ETunaSweeperWorkbenchMode WorkbenchMode = ETunaSweeperWorkbenchMode::Craft);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Workbench")
	void SetActiveWorkbenchMode(ETunaSweeperWorkbenchMode WorkbenchMode);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Workbench")
	void ClearActiveWorkbench();

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Workbench")
	bool GetActiveWorkbenchRecipes(TArray<FTunaSweeperWorkbenchRecipeView>& OutRecipeViews);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Workbench")
	bool TryGetActiveWorkbenchRecipeView(int32 RecipeSlotIndex, FTunaSweeperWorkbenchRecipeView& OutRecipeView);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Workbench")
	bool CanCraftActiveWorkbenchRecipe(int32 RecipeSlotIndex);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Workbench")
	bool TryCraftActiveWorkbenchRecipe(int32 RecipeSlotIndex, bool bSaveImmediately = true);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Workbench")
	bool GetActiveWorkbenchDismantleCandidates(TArray<FTunaSweeperWorkbenchDismantleCandidateView>& OutCandidateViews);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Workbench")
	bool TryGetWorkbenchDismantleCandidateFromSlot(
		const FTunaSweeperItemSlotReference& SlotReference,
		FTunaSweeperWorkbenchDismantleCandidateView& OutCandidateView);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Workbench")
	bool TryDismantleWorkbenchItemInSlot(
		const FTunaSweeperItemSlotReference& SlotReference,
		TArray<FTunaSweeperItemStack>& OutOverflowItems,
		bool bSaveImmediately = true);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Workbench")
	bool GetActiveWorkbenchBlueprintItems(TArray<FTunaSweeperWorkbenchBlueprintItemView>& OutBlueprintItemViews);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Workbench")
	bool TryRegisterWorkbenchBlueprintFromSlot(
		const FTunaSweeperItemSlotReference& SlotReference,
		bool bSaveImmediately = true);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Workbench")
	bool IsWorkbenchRecipeUnlocked(FName RecipeId) const;

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Workbench")
	bool UnlockWorkbenchRecipe(FName RecipeId, bool bSaveImmediately = false);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Workbench")
	void GetUnlockedWorkbenchRecipeIds(TArray<FName>& OutRecipeIds) const;

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|World Progress")
	FTunaSweeperWorldProgressSaveData GetOrCreateWorldProgressState(
		FName ObjectId,
		FName InfoId,
		int32 InitialProgressQuantity,
		int32 RequiredQuantity);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|World Progress")
	bool TryGetWorldProgressState(FName ObjectId, FTunaSweeperWorldProgressSaveData& OutState) const;

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|World Progress")
	bool UpdateWorldProgressState(
		FName ObjectId,
		FName InfoId,
		ETunaSweeperWorldProgressState State,
		int32 ProgressQuantity,
		int32 RequiredQuantity,
		bool bSaveImmediately = false);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Piggy Bank")
	int32 GetPiggyBankStoredAncientCoinValue(FName PiggyBankId) const;

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Piggy Bank")
	bool AddPiggyBankStoredAncientCoinValue(
		FName PiggyBankId,
		int32 CoinValueDelta,
		bool bSaveImmediately = false);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Housing")
	void GetHousingFacilities(TArray<FTunaSweeperHousingPlacedFacilitySaveData>& OutFacilities);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Housing")
	void SetHousingFacilities(
		const TArray<FTunaSweeperHousingPlacedFacilitySaveData>& InFacilities,
		bool bSaveImmediately = false);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Housing")
	bool IsHousingFacilityUnlocked(FName FacilityId);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Housing")
	bool UnlockHousingFacility(FName FacilityId, bool bSaveImmediately = false);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Housing")
	void GetUnlockedHousingFacilityIds(TArray<FName>& OutFacilityIds);

	void SelectItemSlot(const FTunaSweeperItemSlotReference& SlotReference);
	void ClearSelectedItemSelection();
	void SetHoveredItemSlot(const FTunaSweeperItemSlotReference& SlotReference);
	void ClearHoveredItemSlot(const FTunaSweeperItemSlotReference& SlotReference);
	void ClearHoveredItemSlot();
	void SetActiveLootContainerInstance(const FTunaSweeperLootContainerInstance& InContainerInstance, UObject* InOwner = nullptr);
	void SetActiveLootContainerRuntimeSlots(
		const FTunaSweeperLootContainerInstance& InContainerInstance,
		const TArray<FTunaSweeperInventorySlot>& InRuntimeSlots,
		UObject* InOwner = nullptr);
	FGuid CreateItemInstanceFromTemplate(const FTunaSweeperItemInstance& ItemInstanceTemplate);
	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Inventory")
	void NotifyActiveLootContainerUiClosed();
	void SaveGameState();
	void MarkBunkerItemStateSavePending();
	bool FlushPendingBunkerItemStateSave();
	bool HasPendingBunkerItemStateSave() const { return bPendingBunkerItemStateSave; }
	void ClearInventoryAndSave();
	void HandleLevelTravelPersistence(FName SourceLevelName, FName TargetLevelName);
	void CaptureBunkerEntryVitalsFromPawn(APawn* Pawn);
	bool ConsumePendingBunkerEntryVitals(UTunaSweeperVitalsComponent* VitalsComponent);

	FSimpleMulticastDelegate OnInventoryStateChanged;
	FSimpleMulticastDelegate OnSelectedInventoryItemChanged;
	FSimpleMulticastDelegate OnActiveLootContainerUiClosed;
	FSimpleMulticastDelegate OnMemoStateChanged;
	FSimpleMulticastDelegate OnMapMarkersChanged;
	FSimpleMulticastDelegate OnLanguageChanged;
	FSimpleMulticastDelegate OnExperienceChanged;

private:
	enum class EUsableQuickSlotSaveMode : uint8
	{
		PreserveExisting,
		PersistRuntime,
		Clear
	};

	void GeneratePlayerInventoryItems();
	void EnsureInventoryStateInitialized();
	bool LoadGameState();
	bool SaveGameStateInternal(EUsableQuickSlotSaveMode UsableQuickSlotSaveMode = EUsableQuickSlotSaveMode::PreserveExisting) const;
	void MarkItemStateMutationForSave(bool bSaveImmediatelyOutsideBunker = false);
	void ResetRuntimeStateForSaveSlotSelection();
	void GenerateDefaultInventoryState();
	void ResetPlayerSlotArrays();
	void RefreshLegacyPlayerInventoryItems();
	int32 ResolveItemExperienceValue(int32 ItemId);
	FTunaSweeperExperienceAnimationState BuildExperienceAnimationState(
		int64 StartExperiencePoints,
		int64 TargetExperiencePoints,
		int64 GainedExperiencePoints) const;
	void EnsureExperienceLevelTableLoaded() const;
	bool LoadExperienceLevelTableJson(TArray<int64>& OutExperienceForLevels) const;
	void BuildDefaultExperienceLevelTable(TArray<int64>& OutExperienceForLevels) const;
	FString GetExperienceLevelTableJsonPath() const;
	void EnsureExperienceLevelRewardsLoaded() const;
	bool LoadExperienceLevelRewardsJson(TArray<FTunaSweeperExperienceLevelReward>& OutRewards) const;
	FString GetExperienceLevelRewardsJsonPath() const;
	void BroadcastInventoryStateChanged();
	void MarkItemEverAcquired(int32 ItemId);
	void BackfillEverAcquiredItemIdsFromCurrentItems();
	FGuid CreateItemInstance(int32 ItemId, int32 Quantity);
	bool TryAddItemQuantityToExistingStacks(
		int32 ItemId,
		int32& InOutQuantity,
		TArray<FTunaSweeperInventorySlot>& Slots);
	bool TryAddItemQuantityToFirstEmptySlots(
		int32 ItemId,
		int32& InOutQuantity,
		TArray<FTunaSweeperInventorySlot>& Slots,
		TArray<FGuid>* OutCreatedItemUids = nullptr);
	bool AddItemUidToFirstEmptySlot(const FGuid& ItemUid, TArray<FTunaSweeperInventorySlot>& Slots);
	bool AddItemUidToFirstEmptyCompatibleEquipmentSlot(const FGuid& ItemUid);
	void RemoveInvalidSlotReferences(TArray<FTunaSweeperInventorySlot>& Slots) const;
	void EnsureSlotArraySize(TArray<FTunaSweeperInventorySlot>& Slots, int32 DesiredSize) const;
	int32 GetDefaultStorageSlotCapacity() const;
	int32 GetMaxStorageSlotCapacity() const;
	int32 NormalizeStorageSlotCapacity(int32 RequestedCapacity) const;
	int32 GetShopStockQuantity(
		int32 ShopId,
		int32 SlotIndex,
		const FTunaSweeperShopItemDefinition& ShopItemDefinition) const;
	void SetShopStockQuantity(
		int32 ShopId,
		int32 SlotIndex,
		const FTunaSweeperShopItemDefinition& ShopItemDefinition,
		int32 StockQuantity);
	FTunaSweeperWorkbenchRecipeView BuildWorkbenchRecipeView(
		const FTunaSweeperWorkbenchRecipeDefinition& RecipeDefinition,
		int32 RecipeSlotIndex) const;
	bool IsWorkbenchRecipeDefinitionUnlocked(const FTunaSweeperWorkbenchRecipeDefinition& RecipeDefinition) const;
	bool IsWorkbenchBlueprintItemDefinition(const FTunaSweeperItemDefinition& ItemDefinition) const;
	bool IsWorkbenchItemSlotSourceAllowedForDismantle(ETunaSweeperItemSlotSource Source) const;
	bool IsWorkbenchItemSlotSourceAllowedForBlueprintRegister(ETunaSweeperItemSlotSource Source) const;
	bool TryConsumeSingleItemFromSlot(const FTunaSweeperItemSlotReference& SlotReference);
	bool AddWorkbenchResultToInventoryOrOverflow(
		int32 ItemId,
		int32 Quantity,
		TArray<FTunaSweeperItemStack>& InOutOverflowItems);
	int32 CountWorkbenchIngredientItemById(int32 ItemId) const;
	int32 ConsumeWorkbenchIngredientItemById(int32 ItemId, int32 RequestedAmount);
	bool IsSellableItemSlotSource(ETunaSweeperItemSlotSource Source) const;
	TArray<FTunaSweeperInventorySlot>* GetMutableSlotsForSource(ETunaSweeperItemSlotSource Source);
	const TArray<FTunaSweeperInventorySlot>* GetSlotsForSource(ETunaSweeperItemSlotSource Source) const;
	int32 CalculateInventoryCapacityForEquipmentSlots(const TArray<FTunaSweeperInventorySlot>& InEquipmentSlots);
	int32 GetInventoryCapacityForItemUid(const FGuid& ItemUid);
	bool IsItemCompatibleWithEquipmentSlot(int32 SlotIndex, const FGuid& ItemUid);
	bool DoesItemDefinitionMatchEquipmentSlot(int32 SlotIndex, const FTunaSweeperItemDefinition& ItemDefinition) const;
	bool IsItemCompatibleWithUsableQuickSlot(const FGuid& ItemUid);
	bool IsUsableQuickSlotItemDefinition(const FTunaSweeperItemDefinition& ItemDefinition) const;
	bool DoesItemDefinitionHaveUseEffect(const FTunaSweeperItemDefinition& ItemDefinition) const;
	bool TryResolveItemAttachmentDrop(
		const FTunaSweeperItemSlotReference& SourceSlot,
		const FTunaSweeperItemSlotReference& TargetSlot,
		FName& OutAttachmentSlotTag,
		FGuid& OutExistingAttachmentUid);
	bool ApplyItemAttachmentDrop(
		const FTunaSweeperItemSlotReference& SourceSlot,
		const FTunaSweeperItemSlotReference& TargetSlot,
		FName AttachmentSlotTag,
		const FGuid& ExistingAttachmentUid);
	bool DoesItemDefinitionAcceptAttachment(
		const FTunaSweeperItemDefinition& ItemDefinition,
		const FTunaSweeperItemDefinition& AttachmentDefinition) const;
	bool IsBackpackItemUid(const FGuid& ItemUid);
	bool IsBackpackItemDefinition(const FTunaSweeperItemDefinition& ItemDefinition) const;
	float GetEquippedBackpackCarryStrengthBonus() const;
	float CalculatePlayerCarryWeight() const;
	float CalculateItemInstanceCarryWeight(const FGuid& ItemUid, TSet<FGuid>& VisitedItemUids) const;
	float CalculateMaxCarryWeight() const;
	bool IsEquipmentWeaponSlotNumberValid(int32 WeaponSlotNumber) const;
	int32 GetEquipmentSlotIndexForWeaponSlotNumber(int32 WeaponSlotNumber) const;
	bool IsGunItemDefinition(const FTunaSweeperItemDefinition& ItemDefinition) const;
	bool IsMeleeItemDefinition(const FTunaSweeperItemDefinition& ItemDefinition) const;
	bool IsAmmoItemDefinition(const FTunaSweeperItemDefinition& ItemDefinition) const;
	bool IsAmmoDefinitionCompatibleWithWeapon(
		const FTunaSweeperItemDefinition& WeaponDefinition,
		const FTunaSweeperItemDefinition& AmmoDefinition) const;
	bool IsStackableItemDefinition(const FTunaSweeperItemDefinition& ItemDefinition) const;
	bool IsStackableItemId(int32 ItemId) const;
	bool DoesItemInstanceAllowStacking(const FTunaSweeperItemInstance& ItemInstance) const;
	bool CanStackItemInstances(
		const FTunaSweeperItemInstance& SourceItemInstance,
		const FTunaSweeperItemInstance& TargetItemInstance) const;
	bool TryMergeItemStacksBetweenSlots(
		const FTunaSweeperItemSlotReference& SourceSlot,
		const FTunaSweeperItemSlotReference& TargetSlot,
		int32& OutMergedItemId,
		int32& OutMergedQuantity);
	bool CanGrantQuestItemRewards(const TArray<FTunaSweeperItemStack>& ItemRewards) const;
	int32 CalculateWeaponMagazineCapacity(
		const FTunaSweeperItemInstance& WeaponInstance,
		const FTunaSweeperItemDefinition& WeaponDefinition) const;
	int32 ResolveSelectedAmmoItemIdForWeapon(
		FTunaSweeperItemInstance& WeaponInstance,
		const FTunaSweeperItemDefinition& WeaponDefinition);
	int32 CountInventoryAmmoByItemId(int32 AmmoItemId) const;
	int32 ConsumeInventoryAmmoByItemId(int32 AmmoItemId, int32 RequestedAmount);
	void MigrateLegacyEquipmentSlots();
	void RefreshSelectedWeaponAttachmentSlots();
	bool CommitSelectedWeaponAttachmentSlotsToSelectedItem();
	bool DoesSelectedWeaponAcceptAttachmentSlot(FName AttachmentSlotTag) const;
	bool IsItemCompatibleWithSelectedWeaponAttachmentSlot(int32 SlotIndex, const FGuid& ItemUid);
	void ClearSelectedItemIfInvalid();
	bool HasOccupiedInventorySlotsBeyondCapacity(
		const TArray<FTunaSweeperInventorySlot>& InInventorySlots,
		int32 Capacity) const;
	void CollectItemUidsFromSlots(const TArray<FTunaSweeperInventorySlot>& Slots, TSet<FGuid>& OutItemUids) const;
	void CollectPlayerOwnedItemUids(TSet<FGuid>& OutItemUids, bool bIncludeUsableQuickSlots = true) const;
	bool BackupExistingSaveGame(const FString& ExistingSlotName) const;
	void TrimSaveGameBackups() const;
	bool LoadActiveSaveSlotSelection(int32& OutSaveSlotIndex) const;
	bool SaveActiveSaveSlotSelection() const;
	void InitializeGlobalLanguageSetting();
	bool LoadGlobalLanguageSetting(ETunaSweeperItemTextLanguage& OutLanguage) const;
	void SaveGlobalLanguageSetting() const;
	ETunaSweeperItemTextLanguage DetectDefaultLanguageFromOS() const;
	void ApplyCurrentLanguageCulture() const;
	int32 FindFirstExistingSaveSlotIndex() const;
	int32 SanitizeSaveSlotIndex(int32 SaveSlotIndex) const;
	FString GetSaveGameSlotName(int32 SaveSlotIndex) const;
	FString GetSaveSettingsSlotName() const;
	FString GetExistingSaveGameSlotName(int32 SaveSlotIndex) const;
	FString GetSaveGameFilePath(const FString& SaveSlotName) const;
	FString GetSaveGameBackupDirectory() const;
	FString CreateSaveGameBackupFilePath(int32 SaveSlotIndex, FDateTime BackupTime) const;
	float GetCurrentActiveSlotTotalPlaySeconds() const;
	bool IsCurrentWorldBunkerMap() const;
	bool IsBunkerToRaidTravel(FName SourceLevelName, FName TargetLevelName) const;
	bool IsRaidToBunkerTravel(FName SourceLevelName, FName TargetLevelName) const;
	bool IsMapNameMatch(FName MapName, const TCHAR* ExpectedMapName) const;

	UPROPERTY()
	TArray<FTunaSweeperItemStack> PlayerInventoryItems;

	UPROPERTY()
	bool bHasGeneratedPlayerInventoryItems = false;

	UPROPERTY(Transient)
	TMap<FGuid, FTunaSweeperItemInstance> ItemInstancesByUid;

	UPROPERTY(Transient)
	TArray<FTunaSweeperInventorySlot> PlayerInventorySlots;

	UPROPERTY(Transient)
	TArray<FTunaSweeperInventorySlot> EquipmentSlots;

	UPROPERTY(Transient)
	TArray<FTunaSweeperInventorySlot> AuxiliaryBagSlots;

	UPROPERTY(Transient)
	TArray<FTunaSweeperInventorySlot> UsableQuickSlots;

	UPROPERTY(Transient)
	TArray<FTunaSweeperInventorySlot> StorageSlots;

	UPROPERTY(Transient)
	int32 StorageSlotCapacity = 100;

	UPROPERTY(Transient)
	TMap<FName, FTunaSweeperShopStockSaveData> ShopStockStatesByKey;

	UPROPERTY(Transient)
	int32 ActiveShopId = INDEX_NONE;

	UPROPERTY(Transient)
	bool bHasActiveShop = false;

	UPROPERTY(Transient)
	int32 ActiveWorkbenchId = INDEX_NONE;

	UPROPERTY(Transient)
	ETunaSweeperWorkbenchMode ActiveWorkbenchMode = ETunaSweeperWorkbenchMode::Craft;

	UPROPERTY(Transient)
	bool bHasActiveWorkbench = false;

	UPROPERTY(Transient)
	int32 RuntimeSelectedWeaponSlotNumber = 1;

	UPROPERTY(Transient)
	bool bRuntimeSelectedMeleeWeapon = false;

	UPROPERTY(Transient)
	bool bHasRuntimeSelectedWeaponSelection = false;

	UPROPERTY(Transient)
	TObjectPtr<ATunaSweeperPetCompanionCharacter> CurrentPetCompanion;

	UPROPERTY(Transient)
	TArray<FTunaSweeperInventorySlot> ActiveLootContainerSlots;

	UPROPERTY(Transient)
	TWeakObjectPtr<UObject> ActiveLootContainerOwner;

	UPROPERTY(Transient)
	TArray<FName> SelectedWeaponAttachmentSlotTags;

	UPROPERTY(Transient)
	TArray<FTunaSweeperInventorySlot> SelectedWeaponAttachmentSlots;

	UPROPERTY(Transient)
	FTunaSweeperItemSlotReference SelectedItemSlotReference;

	UPROPERTY(Transient)
	FTunaSweeperItemSlotReference HoveredItemSlotReference;

	UPROPERTY(Transient)
	FText ActiveLootContainerDisplayName;

	UPROPERTY(Transient)
	int32 ActiveLootContainerCapacity = 0;

	UPROPERTY(Transient)
	bool bHasActiveLootContainer = false;

	UPROPERTY(Transient)
	bool bInventoryStateInitialized = false;

	UPROPERTY(Transient)
	bool bPendingBunkerItemStateSave = false;

	UPROPERTY(Transient)
	TSet<FName> CompletedScenarioFlags;

	UPROPERTY(Transient)
	TMap<FName, FTunaSweeperWorldProgressSaveData> WorldProgressStatesById;

	UPROPERTY(Transient)
	TMap<FName, FTunaSweeperPiggyBankSaveData> PiggyBankStatesById;

	UPROPERTY(Transient)
	TArray<FTunaSweeperHousingPlacedFacilitySaveData> HousingFacilities;

	UPROPERTY(Transient)
	TSet<FName> UnlockedHousingFacilityIds;

	UPROPERTY(Transient)
	TSet<FName> UnlockedWorkbenchRecipeIds;

	UPROPERTY(Transient)
	TSet<int32> EverAcquiredItemIds;

	UPROPERTY(Transient)
	TSet<int32> AcquiredMemoIds;

	UPROPERTY(Transient)
	TArray<FTunaSweeperMapMarkerSaveData> MapMarkers;

	UPROPERTY(Transient)
	int32 NextMapMarkerId = 1;

	UPROPERTY(Transient)
	int64 TotalExperiencePoints = 0;

	UPROPERTY(Transient)
	int64 RaidStartExperiencePoints = 0;

	UPROPERTY(Transient)
	int64 PendingRaidExperiencePoints = 0;

	UPROPERTY(Transient)
	FTunaSweeperExperienceAnimationState PendingRaidExperienceAnimationState;

	UPROPERTY(Transient)
	bool bRaidExperienceSessionActive = false;

	UPROPERTY(Transient)
	bool bHasPendingRaidExperienceAnimationState = false;

	mutable bool bExperienceLevelRewardsLoaded = false;
	mutable TArray<FTunaSweeperExperienceLevelReward> CachedExperienceLevelRewards;

	mutable bool bExperienceLevelTableLoaded = false;
	mutable TArray<int64> CachedExperienceForLevels;

	bool bHasPendingBunkerEntryVitals = false;
	float PendingBunkerEntryHealthRatio = 1.0f;
	float PendingBunkerEntryFoodRatio = 1.0f;
	float PendingBunkerEntryHydrationRatio = 1.0f;

	UPROPERTY(Transient)
	FName PendingScenarioCompletionFlag;

	UPROPERTY(Transient)
	int32 ActiveSaveSlotIndex = 1;

	UPROPERTY(Transient)
	int32 ActiveSaveSlotDifficultyStage = 1;

	UPROPERTY(Transient)
	bool bActiveSaveSlotDifficultySelected = false;

	UPROPERTY(Transient)
	ETunaSweeperItemTextLanguage CurrentTextLanguage = ETunaSweeperItemTextLanguage::English;

	UPROPERTY(Transient)
	float LoadedSlotTotalPlaySeconds = 0.0f;

	UPROPERTY(Transient)
	double ActiveSlotStartTimeSeconds = 0.0;
};
