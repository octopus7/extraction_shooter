#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "Inventory/TunaSweeperInventoryTypes.h"
#include "TunaSweeperHudItemInfoPanelWidget.generated.h"

class UTextBlock;
class UTileView;
class UWidget;
class UDragDropOperation;

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
	TObjectPtr<UTextBlock> SelectedItemDescriptionText;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UWidget> ModdingPanel;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ModdingText;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UTileView> AttachmentSlotTileView;

private:
	bool TryResolveAttachmentDropSlotFromCursor(
		const FVector2D& ScreenSpacePosition,
		FTunaSweeperItemSlotReference& OutSlotReference) const;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UObject>> AttachmentTileObjects;
};
