#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Inventory/TunaSweeperInventoryTypes.h"
#include "Subsystem/TunaSweeperItemDataSubsystem.h"
#include "TunaSweeperGameInstance.generated.h"

class UTexture2D;

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
};

USTRUCT(BlueprintType)
struct TUNASWEEPER_API FTunaSweeperTempOpenLootItemData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Temp Open Loot")
	FText DisplayName;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Temp Open Loot")
	int32 Quantity = 1;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Temp Open Loot")
	TSoftObjectPtr<UTexture2D> IconTexture;
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|HUD", meta = (ClampMin = "0.0", ClampMax = "100.0", UIMin = "0.0", UIMax = "100.0"))
	float Food = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|HUD", meta = (ClampMin = "0.0", ClampMax = "100.0", UIMin = "0.0", UIMax = "100.0"))
	float Hydration = 100.0f;

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

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|State")
	void ClearRuntimeState();

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Save")
	int32 GetActiveSaveSlotIndex() const { return ActiveSaveSlotIndex; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Save")
	FTunaSweeperSaveSlotSummary GetSaveSlotSummary(int32 SaveSlotIndex) const;

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Save")
	bool ActivateSaveSlot(int32 SaveSlotIndex, bool bStartNewGame);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Save")
	bool DeleteSaveSlot(int32 SaveSlotIndex);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|HUD")
	void SetPlayerHudState(const FTunaSweeperPlayerHudState& InHudState);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|HUD")
	void SetCarryWeight(float CurrentCarryWeight, float MaxCarryWeight, float MovementBlockedWeight);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|HUD")
	float GetCarryWeightMovementSpeedMultiplier() const;

	const TArray<FTunaSweeperTempOpenLootItemData>& GetOrCreateTempOpenLootItems();

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Temp Open Loot")
	void GetTempOpenLootItems(TArray<FTunaSweeperTempOpenLootItemData>& OutItems);

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
	const TArray<FTunaSweeperInventorySlot>& GetActiveLootContainerSlots();
	const TArray<FTunaSweeperInventorySlot>& GetSelectedWeaponAttachmentSlots();
	const TArray<FName>& GetSelectedWeaponAttachmentSlotTags() const { return SelectedWeaponAttachmentSlotTags; }
	bool HasActiveLootContainer() const { return bHasActiveLootContainer; }
	FText GetActiveLootContainerDisplayName() const { return ActiveLootContainerDisplayName; }
	int32 GetActiveLootContainerCapacity() const { return ActiveLootContainerCapacity; }
	bool HasSelectedInventoryItem() const { return SelectedItemSlotReference.IsValid(); }
	FTunaSweeperItemSlotReference GetSelectedItemSlotReference() const { return SelectedItemSlotReference; }

	bool TryGetItemInstance(const FGuid& ItemUid, FTunaSweeperItemInstance& OutItemInstance) const;
	bool TryGetSlotItemInstance(const FTunaSweeperItemSlotReference& SlotReference, FTunaSweeperItemInstance& OutItemInstance);
	bool TryGetSelectedItemInstance(FTunaSweeperItemInstance& OutItemInstance);
	bool TryGetSelectedItemDefinition(FTunaSweeperItemDefinition& OutItemDefinition);
	bool CanSlotAcceptItem(const FTunaSweeperItemSlotReference& SlotReference, const FGuid& ItemUid);
	bool CanMoveItemBetweenSlots(
		const FTunaSweeperItemSlotReference& SourceSlot,
		const FTunaSweeperItemSlotReference& TargetSlot,
		FString* OutFailureReason = nullptr);
	bool MoveItemBetweenSlots(
		const FTunaSweeperItemSlotReference& SourceSlot,
		const FTunaSweeperItemSlotReference& TargetSlot);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Inventory")
	bool AddItemToFirstAvailableInventorySlot(int32 ItemId, int32 Quantity = 1);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Inventory")
	void CompactInventorySlots();

	void SelectItemSlot(const FTunaSweeperItemSlotReference& SlotReference);
	void ClearSelectedItemSelection();
	void SetActiveLootContainerInstance(const FTunaSweeperLootContainerInstance& InContainerInstance);
	void SaveGameState();
	void ClearInventoryAndSave();
	void HandleLevelTravelPersistence(FName SourceLevelName, FName TargetLevelName);

	FSimpleMulticastDelegate OnInventoryStateChanged;
	FSimpleMulticastDelegate OnSelectedInventoryItemChanged;

private:
	void GenerateTempOpenLootItems();
	void GeneratePlayerInventoryItems();
	void EnsureInventoryStateInitialized();
	bool LoadGameState();
	bool SaveGameStateInternal() const;
	void ResetRuntimeStateForSaveSlotSelection();
	void GenerateDefaultInventoryState();
	void ResetPlayerSlotArrays();
	void RefreshLegacyPlayerInventoryItems();
	void BroadcastInventoryStateChanged();
	FGuid CreateItemInstance(int32 ItemId, int32 Quantity);
	bool AddItemUidToFirstEmptySlot(const FGuid& ItemUid, TArray<FTunaSweeperInventorySlot>& Slots);
	void RemoveInvalidSlotReferences(TArray<FTunaSweeperInventorySlot>& Slots) const;
	void EnsureSlotArraySize(TArray<FTunaSweeperInventorySlot>& Slots, int32 DesiredSize) const;
	TArray<FTunaSweeperInventorySlot>* GetMutableSlotsForSource(ETunaSweeperItemSlotSource Source);
	const TArray<FTunaSweeperInventorySlot>* GetSlotsForSource(ETunaSweeperItemSlotSource Source) const;
	int32 CalculateInventoryCapacityForEquipmentSlots(const TArray<FTunaSweeperInventorySlot>& InEquipmentSlots);
	int32 GetInventoryCapacityForItemUid(const FGuid& ItemUid);
	bool IsItemCompatibleWithEquipmentSlot(int32 SlotIndex, const FGuid& ItemUid);
	bool DoesItemDefinitionMatchEquipmentSlot(int32 SlotIndex, const FTunaSweeperItemDefinition& ItemDefinition) const;
	bool IsBackpackItemUid(const FGuid& ItemUid);
	bool IsBackpackItemDefinition(const FTunaSweeperItemDefinition& ItemDefinition) const;
	void MigrateLegacyEquipmentSlots();
	void RefreshSelectedWeaponAttachmentSlots();
	bool CommitSelectedWeaponAttachmentSlotsToSelectedItem();
	bool DoesSelectedWeaponAcceptAttachmentSlot(FName AttachmentSlotTag) const;
	bool IsItemCompatibleWithSelectedWeaponAttachmentSlot(int32 SlotIndex, const FGuid& ItemUid);
	void ClearSelectedItemIfInvalid();
	bool HasOccupiedInventorySlotsBeyondCapacity(
		const TArray<FTunaSweeperInventorySlot>& InInventorySlots,
		int32 Capacity) const;
	void CollectPlayerOwnedItemUids(TSet<FGuid>& OutItemUids) const;
	bool BackupExistingSaveGame(const FString& ExistingSlotName) const;
	void TrimSaveGameBackups() const;
	int32 SanitizeSaveSlotIndex(int32 SaveSlotIndex) const;
	FString GetSaveGameSlotName(int32 SaveSlotIndex) const;
	FString GetExistingSaveGameSlotName(int32 SaveSlotIndex) const;
	FString GetSaveGameFilePath(const FString& SaveSlotName) const;
	FString GetSaveGameBackupDirectory() const;
	FString CreateSaveGameBackupFilePath(int32 SaveSlotIndex, FDateTime BackupTime) const;
	float GetCurrentActiveSlotTotalPlaySeconds() const;
	bool IsBunkerToRaidTravel(FName SourceLevelName, FName TargetLevelName) const;
	bool IsRaidToBunkerTravel(FName SourceLevelName, FName TargetLevelName) const;
	bool IsMapNameMatch(FName MapName, const TCHAR* ExpectedMapName) const;

	UPROPERTY()
	TArray<FTunaSweeperTempOpenLootItemData> TempOpenLootItems;

	UPROPERTY()
	bool bHasGeneratedTempOpenLootItems = false;

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
	TArray<FTunaSweeperInventorySlot> ActiveLootContainerSlots;

	UPROPERTY(Transient)
	TArray<FName> SelectedWeaponAttachmentSlotTags;

	UPROPERTY(Transient)
	TArray<FTunaSweeperInventorySlot> SelectedWeaponAttachmentSlots;

	UPROPERTY(Transient)
	FTunaSweeperItemSlotReference SelectedItemSlotReference;

	UPROPERTY(Transient)
	FText ActiveLootContainerDisplayName;

	UPROPERTY(Transient)
	int32 ActiveLootContainerCapacity = 0;

	UPROPERTY(Transient)
	bool bHasActiveLootContainer = false;

	UPROPERTY(Transient)
	bool bInventoryStateInitialized = false;

	UPROPERTY(Transient)
	int32 ActiveSaveSlotIndex = 1;

	UPROPERTY(Transient)
	float LoadedSlotTotalPlaySeconds = 0.0f;

	UPROPERTY(Transient)
	double ActiveSlotStartTimeSeconds = 0.0;
};
