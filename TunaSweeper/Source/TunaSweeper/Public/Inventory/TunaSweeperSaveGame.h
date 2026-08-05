#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Housing/TunaSweeperHousingTypes.h"
#include "Inventory/TunaSweeperInventoryTypes.h"
#include "Quest/TunaSweeperQuestTypes.h"
#include "TunaSweeperSaveGame.generated.h"

UENUM(BlueprintType)
enum class ETunaSweeperWorldProgressState : uint8
{
	InProgress = 0 UMETA(DisplayName = "In Progress"),
	Completed = 1 UMETA(DisplayName = "Completed")
};

USTRUCT(BlueprintType)
struct TUNASWEEPER_API FTunaSweeperWorldProgressSaveData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|World Progress")
	FName ObjectId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|World Progress")
	FName InfoId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|World Progress")
	ETunaSweeperWorldProgressState State = ETunaSweeperWorldProgressState::InProgress;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|World Progress")
	int32 ProgressQuantity = 0;
};

USTRUCT(BlueprintType)
struct TUNASWEEPER_API FTunaSweeperMapMarkerSaveData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Map")
	int32 MarkerId = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Map")
	FVector2D MapPosition = FVector2D(0.5f, 0.5f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Map", meta = (ClampMin = "0", UIMin = "0"))
	int32 MarkerIconIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Map", meta = (ClampMin = "0", UIMin = "0"))
	int32 MarkerColorIndex = 0;
};

USTRUCT(BlueprintType)
struct TUNASWEEPER_API FTunaSweeperShopStockSaveData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Shop")
	int32 ShopId = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Shop")
	int32 SlotIndex = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Shop")
	int32 ItemId = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Shop", meta = (ClampMin = "0", UIMin = "0"))
	int32 StockQuantity = 0;
};

USTRUCT(BlueprintType)
struct TUNASWEEPER_API FTunaSweeperPiggyBankSaveData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Piggy Bank")
	FName PiggyBankId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Piggy Bank", meta = (ClampMin = "0", UIMin = "0"))
	int32 StoredAncientCoinValue = 0;
};

UCLASS()
class TUNASWEEPER_API UTunaSweeperSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Save")
	int32 SaveVersion = 19;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Save")
	int32 SaveSlotIndex = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Save")
	FName DatasetId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Save")
	FName SaveCompatibilityId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Save")
	FString DatasetRevision;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Save")
	float TotalPlaySeconds = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Save", meta = (ClampMin = "1", ClampMax = "3", UIMin = "1", UIMax = "3"))
	int32 DifficultyStage = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Save")
	bool bDifficultySelected = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Save")
	int64 LastSavedAtTicks = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Experience", meta = (ClampMin = "0", UIMin = "0"))
	int64 TotalExperiencePoints = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Scenario")
	TArray<FName> CompletedScenarioFlags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Memo")
	TArray<int32> AcquiredMemoIds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Inventory")
	TArray<int32> EverAcquiredItemIds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Map")
	TArray<FTunaSweeperMapMarkerSaveData> MapMarkers;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Inventory")
	TArray<FTunaSweeperItemInstance> ItemInstances;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Inventory")
	TArray<FTunaSweeperInventorySlot> InventorySlots;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Inventory")
	TArray<FTunaSweeperInventorySlot> EquipmentSlots;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Inventory")
	TArray<FTunaSweeperInventorySlot> AuxiliaryBagSlots;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Inventory")
	TArray<FTunaSweeperInventorySlot> UsableQuickSlots;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Storage", meta = (ClampMin = "0", UIMin = "0"))
	int32 StorageSlotCapacity = 100;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Storage")
	TArray<FTunaSweeperInventorySlot> StorageSlots;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Shop")
	TArray<FTunaSweeperShopStockSaveData> ShopStockStates;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|World Progress")
	TArray<FTunaSweeperWorldProgressSaveData> WorldProgressStates;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Piggy Bank")
	TArray<FTunaSweeperPiggyBankSaveData> PiggyBankStates;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Housing")
	TArray<FTunaSweeperHousingPlacedFacilitySaveData> HousingFacilities;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Housing")
	TArray<FName> UnlockedHousingFacilityIds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Workbench")
	TArray<FName> UnlockedWorkbenchRecipeIds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Quest")
	TArray<FTunaSweeperQuestProgressSaveData> QuestProgressStates;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Quest")
	FName TrackedQuestId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Quest", meta = (ClampMin = "0", UIMin = "0"))
	int32 QuestCoinBalance = 0;
};

UCLASS()
class TUNASWEEPER_API UTunaSweeperSaveSettings : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Save")
	int32 LastSelectedSaveSlotIndex = 1;
};
