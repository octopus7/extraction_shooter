#include "UI/TunaSweeperItemThumbnailSlotWidget.h"

#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "GameFramework/PlayerController.h"
#include "Game/TunaSweeperGameInstance.h"
#include "InputCoreTypes.h"
#include "Input/Reply.h"
#include "UI/TunaSweeperItemDragDropOperation.h"
#include "UI/TunaSweeperItemHoverPromptWidget.h"
#include "UI/TunaSweeperItemStackTileItemObject.h"
#include "Blueprint/WidgetLayoutLibrary.h"

void UTunaSweeperItemThumbnailSlotWidget::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	IUserObjectListEntry::NativeOnListItemObjectSet(ListItemObject);

	const UTunaSweeperItemStackTileItemObject* TileItemObject = Cast<UTunaSweeperItemStackTileItemObject>(ListItemObject);
	CachedTileData = TileItemObject ? TileItemObject->GetTileData() : FTunaSweeperItemStackTileData();
	ApplyTileData();

	if (ActiveHoverPrompt)
	{
		if (CanShowHoverPrompt())
		{
			ActiveHoverPrompt->SetItemTileData(CachedTileData);
			SetHoveredItemSlot();
		}
		else
		{
			HideHoverPrompt();
			ClearHoveredItemSlot();
		}
	}
}

void UTunaSweeperItemThumbnailSlotWidget::NativeDestruct()
{
	HideHoverPrompt();
	ClearHoveredItemSlot();
	Super::NativeDestruct();
}

void UTunaSweeperItemThumbnailSlotWidget::NativeOnMouseEnter(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);

	if (CanShowHoverPrompt())
	{
		SetHoveredItemSlot();
		ShowHoverPrompt(InMouseEvent);
	}
}

void UTunaSweeperItemThumbnailSlotWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	HideHoverPrompt();
	ClearHoveredItemSlot();
	Super::NativeOnMouseLeave(InMouseEvent);
}

FReply UTunaSweeperItemThumbnailSlotWidget::NativeOnMouseMove(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent)
{
	if (CanShowHoverPrompt())
	{
		SetHoveredItemSlot();
		ShowHoverPrompt(InMouseEvent);
	}
	else
	{
		HideHoverPrompt();
		ClearHoveredItemSlot();
	}

	return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

FReply UTunaSweeperItemThumbnailSlotWidget::NativeOnMouseButtonDown(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent)
{
	if (!CachedTileData.bIsEmpty)
	{
		if (UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
			TunaGameInstance && CachedTileData.Source != ETunaSweeperItemSlotSource::SelectedWeaponAttachment)
		{
			TunaGameInstance->SelectItemSlot(GetCachedSlotReference());
		}

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

	HideHoverPrompt();
	ClearHoveredItemSlot();

	UTunaSweeperItemDragDropOperation* DragOperation = NewObject<UTunaSweeperItemDragDropOperation>(this);
	if (!DragOperation)
	{
		OutOperation = nullptr;
		return;
	}

	DragOperation->TileData = CachedTileData;
	DragOperation->HoveredSlotReference = FTunaSweeperItemSlotReference();
	DragOperation->bHasHoveredSlotReference = false;
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
	const bool bCanAcceptDrop = CanAcceptDragOperation(InOperation);
	UpdateHoveredDropSlot(InOperation, bCanAcceptDrop);
	ApplyDropHighlight(bCanAcceptDrop);
}

void UTunaSweeperItemThumbnailSlotWidget::NativeOnDragLeave(
	const FDragDropEvent& InDragDropEvent,
	UDragDropOperation* InOperation)
{
	UpdateHoveredDropSlot(InOperation, false);
	ApplyDropHighlight(false);
	Super::NativeOnDragLeave(InDragDropEvent, InOperation);
}

bool UTunaSweeperItemThumbnailSlotWidget::NativeOnDragOver(
	const FGeometry& InGeometry,
	const FDragDropEvent& InDragDropEvent,
	UDragDropOperation* InOperation)
{
	const bool bCanAcceptDrop = CanAcceptDragOperation(InOperation);
	UpdateHoveredDropSlot(InOperation, bCanAcceptDrop);
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
	const bool bMoved = TunaGameInstance->MoveItemBetweenSlots(SourceSlot, TargetSlot);
	if (UTunaSweeperItemDragDropOperation* MutableItemDragOperation = Cast<UTunaSweeperItemDragDropOperation>(InOperation))
	{
		MutableItemDragOperation->bHasHoveredSlotReference = false;
	}

	return bMoved || Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);
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
		ItemNameText->SetText(
			(!CachedTileData.bIsEmpty || CachedTileData.bShowEmptySlotLabel)
				? CachedTileData.DisplayName
				: FText::GetEmpty());
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

void UTunaSweeperItemThumbnailSlotWidget::UpdateHoveredDropSlot(
	UDragDropOperation* InOperation,
	bool bCanAcceptDrop) const
{
	UTunaSweeperItemDragDropOperation* ItemDragOperation = Cast<UTunaSweeperItemDragDropOperation>(InOperation);
	if (!ItemDragOperation)
	{
		return;
	}

	const FTunaSweeperItemSlotReference SlotReference = GetCachedSlotReference();
	const bool bIsCurrentHoveredSlot =
		ItemDragOperation->bHasHoveredSlotReference &&
		ItemDragOperation->HoveredSlotReference.Source == SlotReference.Source &&
		ItemDragOperation->HoveredSlotReference.SlotIndex == SlotReference.SlotIndex;

	if (bCanAcceptDrop)
	{
		ItemDragOperation->HoveredSlotReference = SlotReference;
		ItemDragOperation->bHasHoveredSlotReference = true;
	}
	else if (bIsCurrentHoveredSlot)
	{
		ItemDragOperation->HoveredSlotReference = FTunaSweeperItemSlotReference();
		ItemDragOperation->bHasHoveredSlotReference = false;
	}
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

bool UTunaSweeperItemThumbnailSlotWidget::CanShowHoverPrompt() const
{
	return !CachedTileData.bIsEmpty && GetCachedSlotReference().IsValid();
}

void UTunaSweeperItemThumbnailSlotWidget::ShowHoverPrompt(const FPointerEvent& InMouseEvent)
{
	if (!CanShowHoverPrompt())
	{
		HideHoverPrompt();
		return;
	}

	if (!ActiveHoverPrompt)
	{
		APlayerController* OwningPlayer = GetOwningPlayer();
		ActiveHoverPrompt = OwningPlayer
			? CreateWidget<UTunaSweeperItemHoverPromptWidget>(OwningPlayer, UTunaSweeperItemHoverPromptWidget::StaticClass())
			: CreateWidget<UTunaSweeperItemHoverPromptWidget>(GetWorld(), UTunaSweeperItemHoverPromptWidget::StaticClass());
		if (ActiveHoverPrompt)
		{
			ActiveHoverPrompt->SetVisibility(ESlateVisibility::HitTestInvisible);
			ActiveHoverPrompt->AddToViewport(85);
		}
	}

	if (!ActiveHoverPrompt)
	{
		return;
	}

	ActiveHoverPrompt->SetItemTileData(CachedTileData);
	UpdateHoverPromptPosition(InMouseEvent);
}

void UTunaSweeperItemThumbnailSlotWidget::HideHoverPrompt()
{
	if (ActiveHoverPrompt)
	{
		ActiveHoverPrompt->RemoveFromParent();
		ActiveHoverPrompt = nullptr;
	}
}

void UTunaSweeperItemThumbnailSlotWidget::UpdateHoverPromptPosition(const FPointerEvent& InMouseEvent) const
{
	if (!ActiveHoverPrompt)
	{
		return;
	}

	FVector2D ViewportPosition = InMouseEvent.GetScreenSpacePosition();
	if (UWorld* World = GetWorld())
	{
		ViewportPosition = UWidgetLayoutLibrary::GetMousePositionOnViewport(World);
	}

	ActiveHoverPrompt->SetPromptViewportPosition(ViewportPosition);
}

void UTunaSweeperItemThumbnailSlotWidget::SetHoveredItemSlot() const
{
	if (UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>())
	{
		TunaGameInstance->SetHoveredItemSlot(GetCachedSlotReference());
	}
}

void UTunaSweeperItemThumbnailSlotWidget::ClearHoveredItemSlot() const
{
	if (UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>())
	{
		TunaGameInstance->ClearHoveredItemSlot(GetCachedSlotReference());
	}
}
