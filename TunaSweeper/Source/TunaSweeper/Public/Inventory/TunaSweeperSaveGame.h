#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Inventory/TunaSweeperInventoryTypes.h"
#include "TunaSweeperSaveGame.generated.h"

UCLASS()
class TUNASWEEPER_API UTunaSweeperSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Save")
	int32 SaveVersion = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Save")
	int32 SaveSlotIndex = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Save")
	float TotalPlaySeconds = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Save")
	int64 LastSavedAtTicks = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Inventory")
	TArray<FTunaSweeperItemInstance> ItemInstances;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Inventory")
	TArray<FTunaSweeperInventorySlot> InventorySlots;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Inventory")
	TArray<FTunaSweeperInventorySlot> EquipmentSlots;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Inventory")
	TArray<FTunaSweeperInventorySlot> AuxiliaryBagSlots;
};

UCLASS()
class TUNASWEEPER_API UTunaSweeperSaveSettings : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Save")
	int32 LastSelectedSaveSlotIndex = 1;
};
