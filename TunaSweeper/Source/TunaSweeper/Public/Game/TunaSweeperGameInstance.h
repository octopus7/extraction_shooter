#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Effect/TunaSweeperProjectileHitEffectDataAsset.h"
#include "Inventory/TunaSweeperInventoryTypes.h"
#include "Inventory/TunaSweeperSaveGame.h"
#include "Subsystem/TunaSweeperItemDataSubsystem.h"
#include "Weapon/TunaSweeperWeaponSpreadRecoilDataAsset.h"
#include "TunaSweeperGameInstance.generated.h"

class APawn;

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
struct TUNASWEEPER_API FTunaSweeperPlayerHudState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|HUD", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float CurrentCarryWeight = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|HUD", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float MaxCarryWeight = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|HUD", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float MovementBlockedWeight = 100.0f;

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

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Save")
	int32 GetActiveSaveSlotIndex() const { return ActiveSaveSlotIndex; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Save")
	FTunaSweeperSaveSlotSummary GetSaveSlotSummary(int32 SaveSlotIndex) const;

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Save")
	bool SetActiveSaveSlotIndex(int32 SaveSlotIndex);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Save")
	bool ActivateSaveSlot(int32 SaveSlotIndex, bool bStartNewGame);

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
	int32 GetExperienceLevelForTotal(int64 ExperiencePoints) const;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Experience")
	int64 GetExperienceForLevel(int32 Level) const;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Experience")
	int64 GetExperienceForNextLevel(int32 Level) const;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Experience")
	float GetExperienceProgressForTotal(int64 ExperiencePoints) const;

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

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|HUD")
	float GetCarryWeightMovementSpeedMultiplier() const;

	const TArray<FTunaSweeperItemStack>& GetOrCreatePlayerInventoryItems();

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Inventory")
	void GetPlayerInventoryItems(TArray<FTunaSweeperItemStack>& OutItems);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Inventory")
	int32 GetCurrentInventorySlotCapacity();

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Inventory")
	int32 GetEquippedBackpackSlotBonus();

	const TArray<FTunaSweeperInventorySlot>& GetInventorySlots();
	const TArray<FTunaSweeperInventorySlot>& GetEquipmentSlots();
	const TArray<FTunaSweeperInventorySlot>& GetAuxiliaryBagSlots();
	const TArray<FTunaSweeperInventorySlot>& GetUsableQuickSlots();
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
	bool TryGetSlotItemInstance(const FTunaSweeperItemSlotReference& SlotReference, FTunaSweeperItemInstance& OutItemInstance);
	bool TryGetSelectedItemInstance(FTunaSweeperItemInstance& OutItemInstance);
	bool TryGetSelectedItemDefinition(FTunaSweeperItemDefinition& OutItemDefinition);
	bool IsEquipmentWeaponSlotOccupied(int32 WeaponSlotNumber);
	bool TryGetEquipmentWeaponSlotItem(
		int32 WeaponSlotNumber,
		FTunaSweeperItemInstance& OutItemInstance,
		FTunaSweeperItemDefinition& OutItemDefinition);
	bool IsEquipmentMeleeSlotOccupied();
	bool TryGetEquipmentMeleeSlotItem(
		FTunaSweeperItemInstance& OutItemInstance,
		FTunaSweeperItemDefinition& OutItemDefinition);
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
	bool TryUseItemInSlot(const FTunaSweeperItemSlotReference& SlotReference, APawn* InstigatorPawn);
	bool TryUseHoveredItem(APawn* InstigatorPawn);
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
	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Inventory")
	void NotifyActiveLootContainerUiClosed();
	void SaveGameState();
	void ClearInventoryAndSave();
	void HandleLevelTravelPersistence(FName SourceLevelName, FName TargetLevelName);

	FSimpleMulticastDelegate OnInventoryStateChanged;
	FSimpleMulticastDelegate OnSelectedInventoryItemChanged;
	FSimpleMulticastDelegate OnActiveLootContainerUiClosed;
	FSimpleMulticastDelegate OnMemoStateChanged;
	FSimpleMulticastDelegate OnMapMarkersChanged;
	FSimpleMulticastDelegate OnLanguageChanged;

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
	void ResetRuntimeStateForSaveSlotSelection();
	void GenerateDefaultInventoryState();
	void ResetPlayerSlotArrays();
	void RefreshLegacyPlayerInventoryItems();
	int32 ResolveItemExperienceValue(int32 ItemId);
	FTunaSweeperExperienceAnimationState BuildExperienceAnimationState(
		int64 StartExperiencePoints,
		int64 TargetExperiencePoints,
		int64 GainedExperiencePoints) const;
	void BroadcastInventoryStateChanged();
	FGuid CreateItemInstance(int32 ItemId, int32 Quantity);
	bool AddItemUidToFirstEmptySlot(const FGuid& ItemUid, TArray<FTunaSweeperInventorySlot>& Slots);
	bool AddItemUidToFirstEmptyCompatibleEquipmentSlot(const FGuid& ItemUid);
	void RemoveInvalidSlotReferences(TArray<FTunaSweeperInventorySlot>& Slots) const;
	void EnsureSlotArraySize(TArray<FTunaSweeperInventorySlot>& Slots, int32 DesiredSize) const;
	TArray<FTunaSweeperInventorySlot>* GetMutableSlotsForSource(ETunaSweeperItemSlotSource Source);
	const TArray<FTunaSweeperInventorySlot>* GetSlotsForSource(ETunaSweeperItemSlotSource Source) const;
	int32 CalculateInventoryCapacityForEquipmentSlots(const TArray<FTunaSweeperInventorySlot>& InEquipmentSlots);
	int32 GetInventoryCapacityForItemUid(const FGuid& ItemUid);
	bool IsItemCompatibleWithEquipmentSlot(int32 SlotIndex, const FGuid& ItemUid);
	bool DoesItemDefinitionMatchEquipmentSlot(int32 SlotIndex, const FTunaSweeperItemDefinition& ItemDefinition) const;
	bool IsItemCompatibleWithUsableQuickSlot(const FGuid& ItemUid);
	bool IsUsableQuickSlotItemDefinition(const FTunaSweeperItemDefinition& ItemDefinition) const;
	bool DoesItemDefinitionHaveUseEffect(const FTunaSweeperItemDefinition& ItemDefinition) const;
	bool IsBackpackItemUid(const FGuid& ItemUid);
	bool IsBackpackItemDefinition(const FTunaSweeperItemDefinition& ItemDefinition) const;
	bool IsEquipmentWeaponSlotNumberValid(int32 WeaponSlotNumber) const;
	int32 GetEquipmentSlotIndexForWeaponSlotNumber(int32 WeaponSlotNumber) const;
	bool IsGunItemDefinition(const FTunaSweeperItemDefinition& ItemDefinition) const;
	bool IsMeleeItemDefinition(const FTunaSweeperItemDefinition& ItemDefinition) const;
	bool IsAmmoItemDefinition(const FTunaSweeperItemDefinition& ItemDefinition) const;
	bool IsAmmoDefinitionCompatibleWithWeapon(
		const FTunaSweeperItemDefinition& WeaponDefinition,
		const FTunaSweeperItemDefinition& AmmoDefinition) const;
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
	TSet<FName> CompletedScenarioFlags;

	UPROPERTY(Transient)
	TMap<FName, FTunaSweeperWorldProgressSaveData> WorldProgressStatesById;

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

	UPROPERTY(Transient)
	FName PendingScenarioCompletionFlag;

	UPROPERTY(Transient)
	int32 ActiveSaveSlotIndex = 1;

	UPROPERTY(Transient)
	ETunaSweeperItemTextLanguage CurrentTextLanguage = ETunaSweeperItemTextLanguage::English;

	UPROPERTY(Transient)
	float LoadedSlotTotalPlaySeconds = 0.0f;

	UPROPERTY(Transient)
	double ActiveSlotStartTimeSeconds = 0.0;
};
