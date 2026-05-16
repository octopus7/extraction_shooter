#pragma once

#include "Blueprint/DragDropOperation.h"
#include "CoreMinimal.h"
#include "UI/TunaSweeperItemStackTileItemObject.h"
#include "TunaSweeperItemDragDropOperation.generated.h"

UCLASS(BlueprintType)
class TUNASWEEPER_API UTunaSweeperItemDragDropOperation : public UDragDropOperation
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, Category = "TunaSweeper|Item Drag")
	FTunaSweeperItemStackTileData TileData;

	UPROPERTY(BlueprintReadWrite, Category = "TunaSweeper|Item Drag")
	FTunaSweeperItemSlotReference HoveredSlotReference;

	UPROPERTY(BlueprintReadWrite, Category = "TunaSweeper|Item Drag")
	bool bHasHoveredSlotReference = false;
};
