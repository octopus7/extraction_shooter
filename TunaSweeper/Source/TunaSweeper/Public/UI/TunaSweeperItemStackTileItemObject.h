#pragma once

#include "CoreMinimal.h"
#include "Inventory/TunaSweeperInventoryTypes.h"
#include "Subsystem/TunaSweeperItemDataSubsystem.h"
#include "UObject/Object.h"
#include "TunaSweeperItemStackTileItemObject.generated.h"

class UTexture2D;

USTRUCT(BlueprintType)
struct TUNASWEEPER_API FTunaSweeperItemStackTileData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Item Tile")
	FTunaSweeperItemStack ItemStack;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Item Tile")
	FTunaSweeperItemInstance ItemInstance;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Item Tile")
	FText DisplayName;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Item Tile")
	TSoftObjectPtr<UTexture2D> IconTexture;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Item Tile")
	ETunaSweeperItemSlotSource Source = ETunaSweeperItemSlotSource::Inventory;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Item Tile")
	int32 SourceIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Item Tile")
	FTunaSweeperItemSlotReference SlotReference;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Item Tile")
	bool bIsEmpty = true;
};

UCLASS(BlueprintType)
class TUNASWEEPER_API UTunaSweeperItemStackTileItemObject : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(const FTunaSweeperItemStackTileData& InTileData);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Item Tile")
	const FTunaSweeperItemStackTileData& GetTileData() const { return TileData; }

private:
	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Item Tile", meta = (AllowPrivateAccess = "true"))
	FTunaSweeperItemStackTileData TileData;
};
