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
	FText DescriptionText;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Item Tile")
	FTunaSweeperItemDefinition ItemDefinition;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Item Tile")
	TSoftObjectPtr<UTexture2D> IconTexture;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Item Tile")
	ETunaSweeperItemSlotSource Source = ETunaSweeperItemSlotSource::Inventory;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Item Tile")
	int32 SourceIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Item Tile")
	FTunaSweeperItemSlotReference SlotReference;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Item Tile")
	int32 ShopId = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Item Tile")
	int32 ShopStockQuantity = 0;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Item Tile")
	int32 ShopTotalStockQuantity = 0;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Item Tile")
	int32 ShopPrice = 0;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Item Tile")
	int32 WorkbenchId = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Item Tile")
	FName WorkbenchRecipeId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Item Tile")
	FText WorkbenchIngredientText;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Item Tile")
	int32 WorkbenchMissingIngredientCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Item Tile")
	bool bCanCraftWorkbenchRecipe = false;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Item Tile")
	FText WorkbenchDismantleResultText;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Item Tile")
	bool bCanDismantleWorkbenchItem = false;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Item Tile")
	FName WorkbenchBlueprintRecipeId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Item Tile")
	bool bCanRegisterWorkbenchBlueprint = false;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Item Tile")
	bool bWorkbenchBlueprintAlreadyUnlocked = false;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Item Tile")
	bool bIsEmpty = true;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Item Tile")
	bool bShowEmptySlotLabel = false;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Item Tile")
	bool bHasItemDefinition = false;
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
