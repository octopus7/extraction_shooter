#include "UI/TunaSweeperHudItemInfoPanelWidget.h"

#include "Blueprint/DragDropOperation.h"
#include "Components/TextBlock.h"
#include "Components/TileView.h"
#include "Components/Widget.h"
#include "Engine/Texture2D.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Subsystem/TunaSweeperItemDataSubsystem.h"
#include "UI/TunaSweeperItemDragDropOperation.h"
#include "UI/TunaSweeperItemStackTileItemObject.h"

namespace TunaSweeperItemInfoPanel
{
	constexpr int32 AttachmentSlotColumnCount = 2;
	constexpr float AttachmentSlotTileWidth = 96.0f;
	constexpr float AttachmentSlotTileHeight = 96.0f;

	FText GetAttachmentSlotDisplayName(FName AttachmentSlotTag)
	{
		if (AttachmentSlotTag == TEXT("attachment.slot.magazine"))
		{
			return FText::FromString(TEXT("\uD0C4\uCC3D"));
		}
		if (AttachmentSlotTag == TEXT("attachment.slot.optic"))
		{
			return FText::FromString(TEXT("\uAD11\uD559"));
		}

		return FText::FromName(AttachmentSlotTag);
	}

	bool TryResolveSlotFromTileView(
		const UTileView* TileView,
		int32 SlotCount,
		const FVector2D& ScreenSpacePosition,
		FTunaSweeperItemSlotReference& OutSlotReference)
	{
		if (!TileView || SlotCount <= 0)
		{
			return false;
		}

		const FGeometry& TileViewGeometry = TileView->GetCachedGeometry();
		const FVector2D LocalPosition = TileViewGeometry.AbsoluteToLocal(ScreenSpacePosition);
		const FVector2D LocalSize = TileViewGeometry.GetLocalSize();
		if (LocalPosition.X < 0.0f || LocalPosition.Y < 0.0f ||
			LocalPosition.X >= LocalSize.X || LocalPosition.Y >= LocalSize.Y)
		{
			return false;
		}

		const float EntryWidth = FMath::Max(1.0f, TileView->GetEntryWidth());
		const float EntryHeight = FMath::Max(1.0f, TileView->GetEntryHeight());
		const int32 ColumnIndex = FMath::FloorToInt(LocalPosition.X / EntryWidth);
		const int32 RowIndex = FMath::FloorToInt(LocalPosition.Y / EntryHeight);
		if (ColumnIndex < 0 || ColumnIndex >= AttachmentSlotColumnCount || RowIndex < 0)
		{
			return false;
		}

		const int32 FirstVisibleItemIndex = FMath::Max(0, FMath::FloorToInt(TileView->GetScrollOffset()));
		const int32 SlotIndex = FirstVisibleItemIndex + RowIndex * AttachmentSlotColumnCount + ColumnIndex;
		if (SlotIndex < 0 || SlotIndex >= SlotCount)
		{
			return false;
		}

		OutSlotReference.Source = ETunaSweeperItemSlotSource::SelectedWeaponAttachment;
		OutSlotReference.SlotIndex = SlotIndex;
		return true;
	}

	bool TryMoveFromDropSlot(
		UTunaSweeperGameInstance* TunaGameInstance,
		UTunaSweeperItemDragDropOperation* ItemDragOperation,
		const FTunaSweeperItemSlotReference& TargetSlot)
	{
		if (!TunaGameInstance || !ItemDragOperation || ItemDragOperation->TileData.bIsEmpty || !TargetSlot.IsValid())
		{
			return false;
		}

		FTunaSweeperItemSlotReference SourceSlot = ItemDragOperation->TileData.SlotReference;
		if (!SourceSlot.IsValid())
		{
			SourceSlot.Source = ItemDragOperation->TileData.Source;
			SourceSlot.SlotIndex = ItemDragOperation->TileData.SourceIndex;
		}

		const bool bMoved = TunaGameInstance->MoveItemBetweenSlots(SourceSlot, TargetSlot);
		ItemDragOperation->bHasHoveredSlotReference = false;
		ItemDragOperation->HoveredSlotReference = FTunaSweeperItemSlotReference();
		return bMoved;
	}

	bool TryMoveFromHoveredDropSlot(
		UTunaSweeperGameInstance* TunaGameInstance,
		UTunaSweeperItemDragDropOperation* ItemDragOperation)
	{
		if (!TunaGameInstance || !ItemDragOperation || ItemDragOperation->TileData.bIsEmpty ||
			!ItemDragOperation->bHasHoveredSlotReference || !ItemDragOperation->HoveredSlotReference.IsValid())
		{
			return false;
		}

		const bool bMoved = TryMoveFromDropSlot(TunaGameInstance, ItemDragOperation, ItemDragOperation->HoveredSlotReference);
		ItemDragOperation->bHasHoveredSlotReference = false;
		ItemDragOperation->HoveredSlotReference = FTunaSweeperItemSlotReference();
		return bMoved;
	}
}

void UTunaSweeperHudItemInfoPanelWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>())
	{
		TunaGameInstance->OnSelectedInventoryItemChanged.RemoveAll(this);
		TunaGameInstance->OnSelectedInventoryItemChanged.AddUObject(this, &UTunaSweeperHudItemInfoPanelWidget::RefreshSelectedItemInfo);
		TunaGameInstance->OnInventoryStateChanged.RemoveAll(this);
		TunaGameInstance->OnInventoryStateChanged.AddUObject(this, &UTunaSweeperHudItemInfoPanelWidget::RefreshSelectedItemInfo);
	}

	RefreshSelectedItemInfo();
}

void UTunaSweeperHudItemInfoPanelWidget::NativeDestruct()
{
	if (UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>())
	{
		TunaGameInstance->OnSelectedInventoryItemChanged.RemoveAll(this);
		TunaGameInstance->OnInventoryStateChanged.RemoveAll(this);
	}

	Super::NativeDestruct();
}

bool UTunaSweeperHudItemInfoPanelWidget::NativeOnDrop(
	const FGeometry& InGeometry,
	const FDragDropEvent& InDragDropEvent,
	UDragDropOperation* InOperation)
{
	UTunaSweeperItemDragDropOperation* ItemDragOperation = Cast<UTunaSweeperItemDragDropOperation>(InOperation);
	if (!ItemDragOperation || ItemDragOperation->TileData.bIsEmpty)
	{
		return Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);
	}

	UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	FTunaSweeperItemSlotReference CursorSlotReference;
	if (TryResolveAttachmentDropSlotFromCursor(InDragDropEvent.GetScreenSpacePosition(), CursorSlotReference) &&
		TunaSweeperItemInfoPanel::TryMoveFromDropSlot(TunaGameInstance, ItemDragOperation, CursorSlotReference))
	{
		return true;
	}

	if (TunaSweeperItemInfoPanel::TryMoveFromHoveredDropSlot(TunaGameInstance, ItemDragOperation))
	{
		return true;
	}

	return Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);
}

void UTunaSweeperHudItemInfoPanelWidget::RefreshSelectedItemInfo()
{
	UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	UTunaSweeperItemDataSubsystem* ItemDataSubsystem = TunaGameInstance
		? TunaGameInstance->GetSubsystem<UTunaSweeperItemDataSubsystem>()
		: nullptr;

	FTunaSweeperItemInstance SelectedItemInstance;
	FTunaSweeperItemDefinition SelectedItemDefinition;
	if (!TunaGameInstance || !ItemDataSubsystem ||
		!TunaGameInstance->TryGetSelectedItemInstance(SelectedItemInstance) ||
		!ItemDataSubsystem->TryGetItemDefinition(SelectedItemInstance.ItemId, SelectedItemDefinition))
	{
		ClearSelectedItemInfo();
		return;
	}

	FText DisplayName;
	if (!ItemDataSubsystem->TryGetItemNameTextByKey(SelectedItemDefinition.NameStringKey, ETunaSweeperItemTextLanguage::Korean, DisplayName))
	{
		DisplayName = FText::FromString(FString::Printf(TEXT("Item %d"), SelectedItemInstance.ItemId));
	}

	FText Description;
	ItemDataSubsystem->TryGetItemDescriptionText(SelectedItemInstance.ItemId, ETunaSweeperItemTextLanguage::Korean, Description);

	const TArray<FTunaSweeperInventorySlot>& AttachmentSlots = TunaGameInstance->GetSelectedWeaponAttachmentSlots();
	const TArray<FName>& AttachmentSlotTags = TunaGameInstance->GetSelectedWeaponAttachmentSlotTags();
	SetSelectedItemInfo(DisplayName, Description, AttachmentSlots.Num() > 0);

	AttachmentTileObjects.Reset();
	if (AttachmentSlotTileView)
	{
		AttachmentSlotTileView->ClearListItems();
		AttachmentSlotTileView->SetEntryWidth(TunaSweeperItemInfoPanel::AttachmentSlotTileWidth);
		AttachmentSlotTileView->SetEntryHeight(TunaSweeperItemInfoPanel::AttachmentSlotTileHeight);

		for (int32 SlotIndex = 0; SlotIndex < AttachmentSlots.Num(); ++SlotIndex)
		{
			FTunaSweeperItemStackTileData TileData;
			TileData.Source = ETunaSweeperItemSlotSource::SelectedWeaponAttachment;
			TileData.SourceIndex = SlotIndex;
			TileData.SlotReference.Source = ETunaSweeperItemSlotSource::SelectedWeaponAttachment;
			TileData.SlotReference.SlotIndex = SlotIndex;
			TileData.bIsEmpty = true;
			TileData.bShowEmptySlotLabel = true;
			TileData.DisplayName = AttachmentSlotTags.IsValidIndex(SlotIndex)
				? TunaSweeperItemInfoPanel::GetAttachmentSlotDisplayName(AttachmentSlotTags[SlotIndex])
				: FText::FromString(TEXT("Mod"));

			if (AttachmentSlots.IsValidIndex(SlotIndex) && AttachmentSlots[SlotIndex].ItemUid.IsValid() &&
				TunaGameInstance->TryGetItemInstance(AttachmentSlots[SlotIndex].ItemUid, TileData.ItemInstance))
			{
				TileData.bIsEmpty = false;
				TileData.ItemStack.ItemId = TileData.ItemInstance.ItemId;
				TileData.ItemStack.Quantity = FMath::Max(1, TileData.ItemInstance.Quantity);

				FTunaSweeperItemDefinition AttachmentDefinition;
				if (ItemDataSubsystem->TryGetItemDefinition(TileData.ItemInstance.ItemId, AttachmentDefinition))
				{
					ItemDataSubsystem->TryGetItemNameTextByKey(AttachmentDefinition.NameStringKey, ETunaSweeperItemTextLanguage::Korean, TileData.DisplayName);
					const FString IconObjectPath = ItemDataSubsystem->BuildItemIconObjectPath(AttachmentDefinition);
					if (!IconObjectPath.IsEmpty())
					{
						TileData.IconTexture = TSoftObjectPtr<UTexture2D>(FSoftObjectPath(IconObjectPath));
					}
				}
			}

			UTunaSweeperItemStackTileItemObject* TileObject = NewObject<UTunaSweeperItemStackTileItemObject>(this);
			if (TileObject)
			{
				TileObject->Initialize(TileData);
				AttachmentTileObjects.Add(TileObject);
				AttachmentSlotTileView->AddItem(TileObject);
			}
		}
	}

	if (ModdingText)
	{
		const bool bHasRifleModding = SelectedItemDefinition.WeaponTypeTag == TEXT("weapon.type.rifle") && AttachmentSlots.Num() > 0;
		ModdingText->SetText(bHasRifleModding
			? FText::FromString(TEXT("\uC18C\uCD1D \uBAA8\uB529: \uB300\uC6A9\uB7C9 \uD0C4\uCC3D / \uAD11\uD559 \uC7A5\uBE44"))
			: FText::FromString(TEXT("\uBD80\uCC29\uBB3C")));
	}
}

void UTunaSweeperHudItemInfoPanelWidget::SetSelectedItemInfo(const FText& ItemName, const FText& ItemDescription, bool bShowModdingPanel)
{
	if (SelectedItemNameText)
	{
		SelectedItemNameText->SetText(ItemName);
	}

	if (SelectedItemDescriptionText)
	{
		SelectedItemDescriptionText->SetText(ItemDescription);
	}

	SetModdingPanelVisible(bShowModdingPanel);
}

void UTunaSweeperHudItemInfoPanelWidget::ClearSelectedItemInfo()
{
	AttachmentTileObjects.Reset();
	if (AttachmentSlotTileView)
	{
		AttachmentSlotTileView->ClearListItems();
	}

	if (SelectedItemNameText)
	{
		SelectedItemNameText->SetText(FText::FromString(TEXT("No Item")));
	}

	if (SelectedItemDescriptionText)
	{
		SelectedItemDescriptionText->SetText(FText::GetEmpty());
	}

	SetModdingPanelVisible(false);
}

void UTunaSweeperHudItemInfoPanelWidget::SetModdingPanelVisible(bool bVisible)
{
	if (ModdingPanel)
	{
		ModdingPanel->SetVisibility(bVisible ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}
}

bool UTunaSweeperHudItemInfoPanelWidget::TryResolveAttachmentDropSlotFromCursor(
	const FVector2D& ScreenSpacePosition,
	FTunaSweeperItemSlotReference& OutSlotReference) const
{
	const UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	const int32 SlotCount = TunaGameInstance ? TunaGameInstance->GetSelectedWeaponAttachmentSlotTags().Num() : 0;
	return TunaSweeperItemInfoPanel::TryResolveSlotFromTileView(
		AttachmentSlotTileView,
		SlotCount,
		ScreenSpacePosition,
		OutSlotReference);
}
