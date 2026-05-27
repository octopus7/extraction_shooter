#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
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

UCLASS()
class TUNASWEEPER_API UTunaSweeperSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Save")
	int32 SaveVersion = 6;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Save")
	int32 SaveSlotIndex = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Save")
	float TotalPlaySeconds = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Save")
	int64 LastSavedAtTicks = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Scenario")
	TArray<FName> CompletedScenarioFlags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Memo")
	TArray<int32> AcquiredMemoIds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Inventory")
	TArray<FTunaSweeperItemInstance> ItemInstances;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Inventory")
	TArray<FTunaSweeperInventorySlot> InventorySlots;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Inventory")
	TArray<FTunaSweeperInventorySlot> EquipmentSlots;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Inventory")
	TArray<FTunaSweeperInventorySlot> AuxiliaryBagSlots;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|World Progress")
	TArray<FTunaSweeperWorldProgressSaveData> WorldProgressStates;

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
