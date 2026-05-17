#pragma once

#include "Blueprint/IUserObjectListEntry.h"
#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "UI/TunaSweeperItemStackTileItemObject.h"
#include "TunaSweeperItemThumbnailSlotWidget.generated.h"

class UBorder;
class UImage;
class UTextBlock;
class UDragDropOperation;
class UTunaSweeperItemHoverPromptWidget;

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API UTunaSweeperItemThumbnailSlotWidget : public UUserWidget, public IUserObjectListEntry
{
	GENERATED_BODY()

protected:
	virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;
	virtual void NativeDestruct() override;
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnDragDetected(
		const FGeometry& InGeometry,
		const FPointerEvent& InMouseEvent,
		UDragDropOperation*& OutOperation) override;
	virtual void NativeOnDragEnter(
		const FGeometry& InGeometry,
		const FDragDropEvent& InDragDropEvent,
		UDragDropOperation* InOperation) override;
	virtual void NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual bool NativeOnDragOver(
		const FGeometry& InGeometry,
		const FDragDropEvent& InDragDropEvent,
		UDragDropOperation* InOperation) override;
	virtual bool NativeOnDrop(
		const FGeometry& InGeometry,
		const FDragDropEvent& InDragDropEvent,
		UDragDropOperation* InOperation) override;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Item Tile", meta = (BindWidgetOptional))
	TObjectPtr<UBorder> SlotBackground;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Item Tile", meta = (BindWidgetOptional))
	TObjectPtr<UImage> ItemIconImage;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Item Tile", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ItemQuantityText;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Item Tile", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ItemNameText;

private:
	void ApplyTileData();
	void ApplyDropHighlight(bool bCanAcceptDrop);
	bool CanAcceptDragOperation(UDragDropOperation* InOperation) const;
	void UpdateHoveredDropSlot(UDragDropOperation* InOperation, bool bCanAcceptDrop) const;
	FTunaSweeperItemSlotReference GetCachedSlotReference() const;
	bool CanShowHoverPrompt() const;
	void ShowHoverPrompt(const FPointerEvent& InMouseEvent);
	void HideHoverPrompt();
	void UpdateHoverPromptPosition(const FPointerEvent& InMouseEvent) const;
	void SetHoveredItemSlot() const;
	void ClearHoveredItemSlot() const;

	UPROPERTY(Transient)
	FTunaSweeperItemStackTileData CachedTileData;

	UPROPERTY(Transient)
	TObjectPtr<UTunaSweeperItemHoverPromptWidget> ActiveHoverPrompt;
};
