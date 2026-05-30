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
class UTunaSweeperCurrencyDisplayWidget;
struct FTunaSweeperItemSlotReference;

UCLASS(Abstract, BlueprintType, Blueprintable)
class TUNASWEEPER_API UTunaSweeperItemContainerPanelWidget : public UUserWidget
{
	GENERATED_BODY()

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

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Shop", meta = (BindWidgetOptional))
	TObjectPtr<UTunaSweeperCurrencyDisplayWidget> ShopCurrencyDisplayWidget;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Loot Container", meta = (BindWidgetOptional))
	TObjectPtr<UTileView> ContainerTileView;

	void SetContainerInstanceInternal(const FTunaSweeperLootContainerInstance& InContainerInstance);
	void SetStorageViewInternal();
	void SetShopViewInternal(int32 ShopId);
	void SetWorkbenchViewInternal(
		int32 WorkbenchId,
		ETunaSweeperWorkbenchMode WorkbenchMode = ETunaSweeperWorkbenchMode::Craft);

	virtual void PopulateContainerItems();
	virtual void RefreshHeaderControls();

	void EnsureShopCurrencyDisplayWidget();

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

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API UTunaSweeperLootContainerWidget : public UTunaSweeperItemContainerPanelWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Loot Container")
	void SetContainerInstance(const FTunaSweeperLootContainerInstance& InContainerInstance);

	// Compatibility fallback for older HUD blueprints that still route every external container through this widget.
	void SetStorageView();
	void SetShopView(int32 ShopId);
	void SetWorkbenchView(
		int32 WorkbenchId,
		ETunaSweeperWorkbenchMode WorkbenchMode = ETunaSweeperWorkbenchMode::Craft);
};

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API UTunaSweeperStorageContainerWidget : public UTunaSweeperItemContainerPanelWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Storage")
	void SetStorageView();
};

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API UTunaSweeperShopContainerWidget : public UTunaSweeperItemContainerPanelWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Shop")
	void SetShopView(int32 ShopId);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void RefreshHeaderControls() override;

private:
	UFUNCTION()
	void HandleShopRefreshStockButtonClicked();
};
