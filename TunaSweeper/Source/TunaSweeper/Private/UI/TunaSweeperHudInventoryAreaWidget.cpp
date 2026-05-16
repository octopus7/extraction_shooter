#include "UI/TunaSweeperHudInventoryAreaWidget.h"

#include "Blueprint/DragDropOperation.h"
#include "Components/Button.h"
#include "Components/TileView.h"
#include "Components/Widget.h"
#include "Engine/Texture2D.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Subsystem/TunaSweeperItemDataSubsystem.h"
#include "UI/TunaSweeperItemDragDropOperation.h"
#include "UI/TunaSweeperItemStackTileItemObject.h"

namespace TunaSweeperInventoryArea
{
	constexpr int32 InventoryTileColumnCount = 5;
	constexpr int32 EquipmentReserveColumnCount = 4;
	constexpr float InventoryTileWidth = 96.0f;
	constexpr float InventoryTileHeight = 96.0f;
	constexpr float InventoryTileViewScrollbarReserveWidth = 22.0f;
	constexpr float InventoryTileViewWidth = InventoryTileColumnCount * InventoryTileWidth + InventoryTileViewScrollbarReserveWidth;
	constexpr float EquipmentReserveEntryWidth = InventoryTileViewWidth / EquipmentReserveColumnCount;
	constexpr float AuxiliaryBagTileWidth = 96.0f;
	constexpr float AuxiliaryBagTileHeight = 96.0f;

	FTunaSweeperItemStackTileData BuildTileData(
		UTunaSweeperItemDataSubsystem* ItemDataSubsystem,
		const FTunaSweeperItemInstance& ItemInstance,
		ETunaSweeperItemSlotSource Source,
		int32 SourceIndex)
	{
		FTunaSweeperItemStackTileData TileData;
		TileData.ItemInstance = ItemInstance;
		TileData.ItemStack.ItemId = ItemInstance.ItemId;
		TileData.ItemStack.Quantity = FMath::Max(1, ItemInstance.Quantity);
		TileData.Source = Source;
		TileData.SourceIndex = SourceIndex;
		TileData.SlotReference.Source = Source;
		TileData.SlotReference.SlotIndex = SourceIndex;
		TileData.bIsEmpty = !ItemInstance.IsValid();

		if (!TileData.bIsEmpty && ItemDataSubsystem)
		{
			FTunaSweeperItemDefinition ItemDefinition;
			if (ItemDataSubsystem->TryGetItemDefinition(ItemInstance.ItemId, ItemDefinition))
			{
				FText DisplayName;
				if (ItemDataSubsystem->TryGetItemNameTextByKey(ItemDefinition.NameStringKey, ETunaSweeperItemTextLanguage::Korean, DisplayName))
				{
					TileData.DisplayName = DisplayName;
				}
				else
				{
					TileData.DisplayName = FText::FromString(FString::Printf(TEXT("Item %d"), ItemInstance.ItemId));
				}

				const FString IconObjectPath = ItemDataSubsystem->BuildItemIconObjectPath(ItemDefinition);
				if (!IconObjectPath.IsEmpty())
				{
					TileData.IconTexture = TSoftObjectPtr<UTexture2D>(FSoftObjectPath(IconObjectPath));
				}
			}
		}

		return TileData;
	}

	UTunaSweeperItemStackTileItemObject* CreateTileObject(
		UObject* Outer,
		const FTunaSweeperItemStackTileData& TileData)
	{
		UTunaSweeperItemStackTileItemObject* TileObject = NewObject<UTunaSweeperItemStackTileItemObject>(Outer);
		if (TileObject)
		{
			TileObject->Initialize(TileData);
		}
		return TileObject;
	}

	void PopulateTileView(
		UObject* Outer,
		UTileView* TileView,
		const TArray<FTunaSweeperInventorySlot>& Slots,
		UTunaSweeperGameInstance* TunaGameInstance,
		UTunaSweeperItemDataSubsystem* ItemDataSubsystem,
		ETunaSweeperItemSlotSource Source,
		float TileWidth,
		float TileHeight,
		TArray<TObjectPtr<UObject>>& TileObjects)
	{
		if (!TileView)
		{
			return;
		}

		TileView->ClearListItems();
		TileView->SetEntryWidth(TileWidth);
		TileView->SetEntryHeight(TileHeight);

		for (int32 Index = 0; Index < Slots.Num(); ++Index)
		{
			FTunaSweeperItemInstance ItemInstance;
			if (TunaGameInstance && Slots[Index].ItemUid.IsValid())
			{
				TunaGameInstance->TryGetItemInstance(Slots[Index].ItemUid, ItemInstance);
			}

			UTunaSweeperItemStackTileItemObject* TileObject = CreateTileObject(
				Outer,
				BuildTileData(ItemDataSubsystem, ItemInstance, Source, Index));
			if (TileObject)
			{
				TileObjects.Add(TileObject);
				TileView->AddItem(TileObject);
			}
		}
	}
}

void UTunaSweeperHudInventoryAreaWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (SortInventoryButton)
	{
		SortInventoryButton->OnClicked.RemoveDynamic(this, &UTunaSweeperHudInventoryAreaWidget::HandleSortInventoryClicked);
		SortInventoryButton->OnClicked.AddDynamic(this, &UTunaSweeperHudInventoryAreaWidget::HandleSortInventoryClicked);
	}

	if (UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>())
	{
		TunaGameInstance->OnInventoryStateChanged.RemoveAll(this);
		TunaGameInstance->OnInventoryStateChanged.AddUObject(this, &UTunaSweeperHudInventoryAreaWidget::RefreshInventoryItems);
	}

	RefreshInventoryItems();
}

void UTunaSweeperHudInventoryAreaWidget::NativeDestruct()
{
	if (UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>())
	{
		TunaGameInstance->OnInventoryStateChanged.RemoveAll(this);
	}

	if (SortInventoryButton)
	{
		SortInventoryButton->OnClicked.RemoveDynamic(this, &UTunaSweeperHudInventoryAreaWidget::HandleSortInventoryClicked);
	}

	Super::NativeDestruct();
}

void UTunaSweeperHudInventoryAreaWidget::SetInventoryVisible(bool bVisible)
{
	if (InventoryPanel)
	{
		InventoryPanel->SetVisibility(bVisible ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void UTunaSweeperHudInventoryAreaWidget::RefreshInventoryItems()
{
	UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	UTunaSweeperItemDataSubsystem* ItemDataSubsystem = TunaGameInstance
		? TunaGameInstance->GetSubsystem<UTunaSweeperItemDataSubsystem>()
		: nullptr;

	TileObjects.Reset();

	static const TArray<FTunaSweeperInventorySlot> EmptySlots;
	TunaSweeperInventoryArea::PopulateTileView(
		this,
		EquipmentReserveTileView,
		TunaGameInstance ? TunaGameInstance->GetEquipmentSlots() : EmptySlots,
		TunaGameInstance,
		ItemDataSubsystem,
		ETunaSweeperItemSlotSource::Equipment,
		TunaSweeperInventoryArea::EquipmentReserveEntryWidth,
		TunaSweeperInventoryArea::InventoryTileHeight,
		TileObjects);

	TunaSweeperInventoryArea::PopulateTileView(
		this,
		AuxiliaryBagTileView,
		TunaGameInstance ? TunaGameInstance->GetAuxiliaryBagSlots() : EmptySlots,
		TunaGameInstance,
		ItemDataSubsystem,
		ETunaSweeperItemSlotSource::AuxiliaryBag,
		TunaSweeperInventoryArea::AuxiliaryBagTileWidth,
		TunaSweeperInventoryArea::AuxiliaryBagTileHeight,
		TileObjects);

	TunaSweeperInventoryArea::PopulateTileView(
		this,
		InventoryTileView,
		TunaGameInstance ? TunaGameInstance->GetInventorySlots() : EmptySlots,
		TunaGameInstance,
		ItemDataSubsystem,
		ETunaSweeperItemSlotSource::Inventory,
		TunaSweeperInventoryArea::InventoryTileWidth,
		TunaSweeperInventoryArea::InventoryTileHeight,
		TileObjects);
}

bool UTunaSweeperHudInventoryAreaWidget::NativeOnDrop(
	const FGeometry& InGeometry,
	const FDragDropEvent& InDragDropEvent,
	UDragDropOperation* InOperation)
{
	const UTunaSweeperItemDragDropOperation* ItemDragOperation = Cast<UTunaSweeperItemDragDropOperation>(InOperation);
	if (!ItemDragOperation || ItemDragOperation->TileData.bIsEmpty)
	{
		return Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);
	}

	return Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);
}

void UTunaSweeperHudInventoryAreaWidget::HandleSortInventoryClicked()
{
	if (UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>())
	{
		TunaGameInstance->CompactInventorySlots();
	}
}

void UTunaSweeperHudInventoryAreaWidget::SetAuxiliaryBagVisible(bool bVisible)
{
	if (AuxiliaryBagPanel)
	{
		AuxiliaryBagPanel->SetVisibility(bVisible ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}
}
