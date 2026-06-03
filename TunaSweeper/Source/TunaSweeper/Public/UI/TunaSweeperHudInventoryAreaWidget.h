#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "Game/TunaSweeperGameInstance.h"
#include "TunaSweeperHudInventoryAreaWidget.generated.h"

class UWidget;
class UCanvasPanel;
class UOverlay;
class UTileView;
class UDragDropOperation;
class UButton;
class UProgressBar;
class USizeBox;
class UTextBlock;
class UTunaSweeperCurrencyDisplayWidget;
struct FTunaSweeperItemSlotReference;

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API UTunaSweeperHudInventoryAreaWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|HUD")
	void SetInventoryVisible(bool bVisible);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|HUD")
	void SetAuxiliaryBagVisible(bool bVisible);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|HUD")
	void RefreshInventoryItems();

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|HUD")
	void SetHudState(const FTunaSweeperPlayerHudState& InHudState);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual bool NativeOnDrop(
		const FGeometry& InGeometry,
		const FDragDropEvent& InDragDropEvent,
		UDragDropOperation* InOperation) override;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UWidget> InventoryPanel;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UWidget> AuxiliaryBagPanel;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UTileView> EquipmentReserveTileView;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional))
	TObjectPtr<USizeBox> EquipmentReserveSizeBox;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UTileView> AuxiliaryBagTileView;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UTileView> InventoryTileView;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UButton> SortInventoryButton;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SortInventoryButtonText;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UTunaSweeperCurrencyDisplayWidget> CurrencyDisplayWidget;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UWidget> InventoryWeightPanel;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> InventoryWeightLabelText;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> InventoryWeightText;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> InventoryWeightGauge;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional))
	TObjectPtr<USizeBox> InventoryWeightGaugeBox;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UOverlay> InventoryWeightGaugeOverlay;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UCanvasPanel> InventoryWeightMarkerCanvas;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> InventoryWeightOverweightMarker;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UWidget> InventoryWeightWarningIcon;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> InventoryWeightWarningText;

private:
	void EnsureCurrencyDisplayWidget();
	void AttachCurrencyDisplayAboveInventoryPanel();
	void EnsureSortInventoryButtonPlacement();
	void EnsureWeightThresholdMarkerWidgets();
	void RefreshWeightThresholdMarker();
	void ApplyHudState();

	bool TryResolveDropSlotFromCursor(
		const FVector2D& ScreenSpacePosition,
		FTunaSweeperItemSlotReference& OutSlotReference);

	UFUNCTION()
	void HandleSortInventoryClicked();

	UPROPERTY(Transient)
	TArray<TObjectPtr<UObject>> TileObjects;

	UPROPERTY(EditAnywhere, Category = "TunaSweeper|HUD")
	FTunaSweeperPlayerHudState PreviewHudState;
};
