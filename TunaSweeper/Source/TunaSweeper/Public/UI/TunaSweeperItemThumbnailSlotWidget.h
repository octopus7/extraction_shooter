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

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API UTunaSweeperItemThumbnailSlotWidget : public UUserWidget, public IUserObjectListEntry
{
	GENERATED_BODY()

protected:
	virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;
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

	UPROPERTY(Transient)
	FTunaSweeperItemStackTileData CachedTileData;
};
