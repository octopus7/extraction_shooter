#include "UI/TunaSweeperItemThumbnailSlotWidget.h"

#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "Game/TunaSweeperGameInstance.h"
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

void UTunaSweeperItemThumbnailSlotWidget::NativeOnDragEnter(
	const FGeometry& InGeometry,
	const FDragDropEvent& InDragDropEvent,
	UDragDropOperation* InOperation)
{
	Super::NativeOnDragEnter(InGeometry, InDragDropEvent, InOperation);
	ApplyDropHighlight(CanAcceptDragOperation(InOperation));
}

void UTunaSweeperItemThumbnailSlotWidget::NativeOnDragLeave(
	const FDragDropEvent& InDragDropEvent,
	UDragDropOperation* InOperation)
{
	ApplyDropHighlight(false);
	Super::NativeOnDragLeave(InDragDropEvent, InOperation);
}

bool UTunaSweeperItemThumbnailSlotWidget::NativeOnDragOver(
	const FGeometry& InGeometry,
	const FDragDropEvent& InDragDropEvent,
	UDragDropOperation* InOperation)
{
	const bool bCanAcceptDrop = CanAcceptDragOperation(InOperation);
	ApplyDropHighlight(bCanAcceptDrop);
	return bCanAcceptDrop || Super::NativeOnDragOver(InGeometry, InDragDropEvent, InOperation);
}

bool UTunaSweeperItemThumbnailSlotWidget::NativeOnDrop(
	const FGeometry& InGeometry,
	const FDragDropEvent& InDragDropEvent,
	UDragDropOperation* InOperation)
{
	ApplyDropHighlight(false);

	const UTunaSweeperItemDragDropOperation* ItemDragOperation = Cast<UTunaSweeperItemDragDropOperation>(InOperation);
	UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	if (!ItemDragOperation || !TunaGameInstance)
	{
		return Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);
	}

	FTunaSweeperItemSlotReference SourceSlot = ItemDragOperation->TileData.SlotReference;
	if (!SourceSlot.IsValid())
	{
		SourceSlot.Source = ItemDragOperation->TileData.Source;
		SourceSlot.SlotIndex = ItemDragOperation->TileData.SourceIndex;
	}
	const FTunaSweeperItemSlotReference TargetSlot = GetCachedSlotReference();
	return TunaGameInstance->MoveItemBetweenSlots(SourceSlot, TargetSlot) ||
		Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);
}

void UTunaSweeperItemThumbnailSlotWidget::ApplyTileData()
{
	if (SlotBackground)
	{
		ApplyDropHighlight(false);
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

void UTunaSweeperItemThumbnailSlotWidget::ApplyDropHighlight(bool bCanAcceptDrop)
{
	if (!SlotBackground)
	{
		return;
	}

	SlotBackground->SetRenderOpacity(bCanAcceptDrop ? 1.0f : (CachedTileData.bIsEmpty ? 0.45f : 1.0f));
	SlotBackground->SetBrushColor(bCanAcceptDrop
		? FLinearColor(0.32f, 0.82f, 0.52f, 1.0f)
		: FLinearColor::White);
}

bool UTunaSweeperItemThumbnailSlotWidget::CanAcceptDragOperation(UDragDropOperation* InOperation) const
{
	const UTunaSweeperItemDragDropOperation* ItemDragOperation = Cast<UTunaSweeperItemDragDropOperation>(InOperation);
	UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	if (!ItemDragOperation || !TunaGameInstance || ItemDragOperation->TileData.bIsEmpty)
	{
		return false;
	}

	FTunaSweeperItemSlotReference SourceSlot = ItemDragOperation->TileData.SlotReference;
	if (!SourceSlot.IsValid())
	{
		SourceSlot.Source = ItemDragOperation->TileData.Source;
		SourceSlot.SlotIndex = ItemDragOperation->TileData.SourceIndex;
	}
	return TunaGameInstance->CanMoveItemBetweenSlots(SourceSlot, GetCachedSlotReference());
}

FTunaSweeperItemSlotReference UTunaSweeperItemThumbnailSlotWidget::GetCachedSlotReference() const
{
	if (CachedTileData.SlotReference.IsValid())
	{
		return CachedTileData.SlotReference;
	}

	FTunaSweeperItemSlotReference SlotReference;
	SlotReference.Source = CachedTileData.Source;
	SlotReference.SlotIndex = CachedTileData.SourceIndex;
	return SlotReference;
}
