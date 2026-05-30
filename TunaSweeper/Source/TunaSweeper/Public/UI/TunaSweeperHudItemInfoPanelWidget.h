#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "Inventory/TunaSweeperInventoryTypes.h"
#include "TunaSweeperHudItemInfoPanelWidget.generated.h"

class UTextBlock;
class UTileView;
class UWidget;
class UDragDropOperation;
class UBorder;
class UButton;
class UHorizontalBox;
class UImage;
class USizeBox;
class UTexture2D;
class UVerticalBox;

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API UTunaSweeperHudItemInfoPanelWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|HUD")
	void RefreshSelectedItemInfo();

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|HUD")
	void SetSelectedItemInfo(const FText& ItemName, const FText& ItemDescription, bool bShowModdingPanel);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|HUD")
	void ClearSelectedItemInfo();

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|HUD")
	void SetModdingPanelVisible(bool bVisible);

	void SetPanelLayoutLimits(float InPanelWidth, float InMaxPanelHeight);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual bool NativeOnDrop(
		const FGeometry& InGeometry,
		const FDragDropEvent& InDragDropEvent,
		UDragDropOperation* InOperation) override;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SelectedItemNameText;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional))
	TObjectPtr<USizeBox> RootSizeBox;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UBorder> PanelBackground;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UHorizontalBox> HeaderRow;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> PanelStack;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UHorizontalBox> SelectedItemDetailRow;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UWidget> SelectedItemIconContainer;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UImage> SelectedItemIconImage;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SelectedItemDescriptionText;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SelectedItemFormulaText;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> SelectedItemSpecStack;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SelectedItemSpecTitleText;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UHorizontalBox> SelectedItemSpecValueRow;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SelectedItemSpecLabelText;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SelectedItemSpecValueText;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SelectedItemSpecSecondaryText;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UButton> CloseButton;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UWidget> ModdingPanel;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ModdingText;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UTileView> AttachmentSlotTileView;

private:
	UFUNCTION()
	void HandleCloseButtonClicked();

	void CacheNamedWidgets();
	void EnsureThumbnailWidgets();
	void SetSelectedItemThumbnail(UTexture2D* IconTexture);
	void ClearSelectedItemThumbnail();
	void SetSelectedItemSpecInfo(const FText& TitleText, const FText& LabelText, const FText& ValueText, const FText& SecondaryText, const FLinearColor& ValueColor, bool bVisible);
	bool TryResolveAttachmentDropSlotFromCursor(
		const FVector2D& ScreenSpacePosition,
		FTunaSweeperItemSlotReference& OutSlotReference) const;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UObject>> AttachmentTileObjects;
};
