#include "UI/TunaSweeperItemThumbnailSlotWidget.h"

#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "InputCoreTypes.h"
#include "Input/Reply.h"
#include "UI/TunaSweeperItemDragDropOperation.h"
#include "UI/TunaSweeperItemStackTileItemObject.h"

void UTunaSweeperItemThumbnailSlotWidget::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	IUserObjectListEntry::NativeOnListItemObjectSet(ListItemObject);

	const UTunaSweeperItemStackTileItemObject* TileItemObject = Cast<UTunaSweeperItemStackTileItemObject>(ListItemObject);
	CachedTileData = TileItemObject ? TileItemObject->GetTileData() : FTunaSweeperItemStackTileData();
	ApplyTileData();
}

FReply UTunaSweeperItemThumbnailSlotWidget::NativeOnMouseButtonDown(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent)
{
	if (!CachedTileData.bIsEmpty)
	{
		return UWidgetBlueprintLibrary::DetectDragIfPressed(InMouseEvent, this, EKeys::LeftMouseButton).NativeReply;
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void UTunaSweeperItemThumbnailSlotWidget::NativeOnDragDetected(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent,
	UDragDropOperation*& OutOperation)
{
	if (CachedTileData.bIsEmpty)
	{
		OutOperation = nullptr;
		return;
	}

	UTunaSweeperItemDragDropOperation* DragOperation = NewObject<UTunaSweeperItemDragDropOperation>(this);
	if (!DragOperation)
	{
		OutOperation = nullptr;
		return;
	}

	DragOperation->TileData = CachedTileData;
	DragOperation->DefaultDragVisual = this;
	DragOperation->Pivot = EDragPivot::MouseDown;
	OutOperation = DragOperation;
}

void UTunaSweeperItemThumbnailSlotWidget::ApplyTileData()
{
	if (SlotBackground)
	{
		SlotBackground->SetRenderOpacity(CachedTileData.bIsEmpty ? 0.45f : 1.0f);
	}

	if (ItemIconImage)
	{
		UTexture2D* IconTexture = CachedTileData.IconTexture.LoadSynchronous();
		if (IconTexture && !CachedTileData.bIsEmpty)
		{
			ItemIconImage->SetBrushFromTexture(IconTexture, true);
			ItemIconImage->SetOpacity(1.0f);
		}
		else
		{
			ItemIconImage->SetBrushFromTexture(nullptr, false);
			ItemIconImage->SetOpacity(0.0f);
		}
	}

	if (ItemQuantityText)
	{
		ItemQuantityText->SetText(
			CachedTileData.bIsEmpty
				? FText::GetEmpty()
				: FText::Format(FText::FromString(TEXT("x{0}")), FText::AsNumber(CachedTileData.ItemStack.Quantity)));
	}

	if (ItemNameText)
	{
		ItemNameText->SetText(CachedTileData.bIsEmpty ? FText::GetEmpty() : CachedTileData.DisplayName);
	}
}
