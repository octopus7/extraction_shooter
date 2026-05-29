#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "Inventory/TunaSweeperInventoryTypes.h"
#include "Subsystem/TunaSweeperItemDataSubsystem.h"
#include "TunaSweeperLootContainerWidget.generated.h"

class USizeBox;
class UTextBlock;
class UTileView;
class UButton;
class UDragDropOperation;
struct FTunaSweeperItemSlotReference;

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API UTunaSweeperLootContainerWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Loot Container")
	void SetContainerInstance(const FTunaSweeperLootContainerInstance& InContainerInstance);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Storage")
	void SetStorageView();

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Shop")
	void SetShopView(int32 ShopId);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Workbench")
	void SetWorkbenchView(
		int32 WorkbenchId,
		ETunaSweeperWorkbenchMode WorkbenchMode = ETunaSweeperWorkbenchMode::Craft);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual bool NativeOnDrop(
		const FGeometry& InGeometry,
		const FDragDropEvent& InDragDropEvent,
		UDragDropOperation* InOperation) override;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Loot Container", meta = (BindWidgetOptional))
	TObjectPtr<USizeBox> RootSizeBox;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Loot Container", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ContainerTitleText;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Loot Container", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ContainerOccupancyText;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Shop", meta = (BindWidgetOptional))
	TObjectPtr<UButton> ShopRefreshStockButton;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Shop", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ShopRefreshStockButtonText;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Loot Container", meta = (BindWidgetOptional))
	TObjectPtr<UTileView> ContainerTileView;

private:
	UFUNCTION()
	void HandleShopRefreshStockButtonClicked();

	void PopulateContainerItems();
	void RefreshShopRefreshStockButton();
	bool TryResolveDropSlotFromCursor(
		const FVector2D& ScreenSpacePosition,
		FTunaSweeperItemSlotReference& OutSlotReference);

	UPROPERTY(Transient)
	FTunaSweeperLootContainerInstance ContainerInstance;

	UPROPERTY(Transient)
	ETunaSweeperItemSlotSource SlotSource = ETunaSweeperItemSlotSource::LootContainer;

	UPROPERTY(Transient)
	int32 ActiveShopId = INDEX_NONE;

	UPROPERTY(Transient)
	int32 ActiveWorkbenchId = INDEX_NONE;

	UPROPERTY(Transient)
	ETunaSweeperWorkbenchMode ActiveWorkbenchMode = ETunaSweeperWorkbenchMode::Craft;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UObject>> TileObjects;
};
