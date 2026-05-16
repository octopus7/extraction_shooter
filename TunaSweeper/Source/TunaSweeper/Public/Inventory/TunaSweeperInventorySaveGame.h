#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Inventory/TunaSweeperInventoryTypes.h"
#include "TunaSweeperInventorySaveGame.generated.h"

UCLASS()
class TUNASWEEPER_API UTunaSweeperInventorySaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Inventory")
	int32 SaveVersion = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Inventory")
	TArray<FTunaSweeperItemInstance> ItemInstances;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Inventory")
	TArray<FTunaSweeperInventorySlot> InventorySlots;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Inventory")
	TArray<FTunaSweeperInventorySlot> EquipmentSlots;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Inventory")
	TArray<FTunaSweeperInventorySlot> AuxiliaryBagSlots;
};
