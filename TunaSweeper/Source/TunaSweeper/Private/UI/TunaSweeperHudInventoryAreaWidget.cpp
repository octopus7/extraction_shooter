#include "UI/TunaSweeperHudInventoryAreaWidget.h"

#include "Blueprint/DragDropOperation.h"
#include "Components/TileView.h"
#include "Components/Widget.h"
#include "Engine/Engine.h"
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
		const FTunaSweeperItemStack& ItemStack,
		ETunaSweeperItemSlotSource Source,
		int32 SourceIndex)
	{
		FTunaSweeperItemStackTileData TileData;
		TileData.ItemStack = ItemStack;
		TileData.Source = Source;
		TileData.SourceIndex = SourceIndex;
		TileData.bIsEmpty = ItemStack.ItemId == INDEX_NONE;

		if (!TileData.bIsEmpty && ItemDataSubsystem)
		{
			FTunaSweeperItemDefinition ItemDefinition;
			if (ItemDataSubsystem->TryGetItemDefinition(ItemStack.ItemId, ItemDefinition))
			{
				FText DisplayName;
				if (ItemDataSubsystem->TryGetItemNameTextByKey(ItemDefinition.NameStringKey, ETunaSweeperItemTextLanguage::Korean, DisplayName))
				{
					TileData.DisplayName = DisplayName;
				}
				else
				{
					TileData.DisplayName = FText::FromString(FString::Printf(TEXT("Item %d"), ItemStack.ItemId));
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
}

void UTunaSweeperHudInventoryAreaWidget::NativeConstruct()
{
	Super::NativeConstruct();

	PopulateReservedTiles();
	RefreshInventoryItems();
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
	if (!InventoryTileView)
	{
		return;
	}

	UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	UTunaSweeperItemDataSubsystem* ItemDataSubsystem = TunaGameInstance
		? TunaGameInstance->GetSubsystem<UTunaSweeperItemDataSubsystem>()
		: nullptr;

	TileObjects.Reset();
	InventoryTileView->ClearListItems();
	InventoryTileView->SetEntryWidth(TunaSweeperInventoryArea::InventoryTileWidth);
	InventoryTileView->SetEntryHeight(TunaSweeperInventoryArea::InventoryTileHeight);

	if (!TunaGameInstance)
	{
		return;
	}

	const TArray<FTunaSweeperItemStack>& InventoryItems = TunaGameInstance->GetOrCreatePlayerInventoryItems();
	for (int32 Index = 0; Index < InventoryItems.Num(); ++Index)
	{
		UTunaSweeperItemStackTileItemObject* TileObject = TunaSweeperInventoryArea::CreateTileObject(
			this,
			TunaSweeperInventoryArea::BuildTileData(ItemDataSubsystem, InventoryItems[Index], ETunaSweeperItemSlotSource::Inventory, Index));
		if (TileObject)
		{
			TileObjects.Add(TileObject);
			InventoryTileView->AddItem(TileObject);
		}
	}
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

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			1.5f,
			FColor::Yellow,
			FString::Printf(TEXT("[Inventory] Drop received: item=%d qty=%d"),
				ItemDragOperation->TileData.ItemStack.ItemId,
				ItemDragOperation->TileData.ItemStack.Quantity));
	}

	return true;
}

void UTunaSweeperHudInventoryAreaWidget::PopulateReservedTiles()
{
	UTunaSweeperItemDataSubsystem* ItemDataSubsystem = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UTunaSweeperItemDataSubsystem>()
		: nullptr;

	if (EquipmentReserveTileView)
	{
		EquipmentReserveTileView->ClearListItems();
		EquipmentReserveTileView->SetEntryWidth(TunaSweeperInventoryArea::EquipmentReserveEntryWidth);
		EquipmentReserveTileView->SetEntryHeight(TunaSweeperInventoryArea::InventoryTileHeight);
		for (int32 Index = 0; Index < 8; ++Index)
		{
			FTunaSweeperItemStack EmptyStack;
			EmptyStack.ItemId = INDEX_NONE;
			UTunaSweeperItemStackTileItemObject* TileObject = TunaSweeperInventoryArea::CreateTileObject(
				this,
				TunaSweeperInventoryArea::BuildTileData(ItemDataSubsystem, EmptyStack, ETunaSweeperItemSlotSource::Equipment, Index));
			if (TileObject)
			{
				TileObjects.Add(TileObject);
				EquipmentReserveTileView->AddItem(TileObject);
			}
		}
	}

	if (AuxiliaryBagTileView)
	{
		AuxiliaryBagTileView->ClearListItems();
		AuxiliaryBagTileView->SetEntryWidth(TunaSweeperInventoryArea::AuxiliaryBagTileWidth);
		AuxiliaryBagTileView->SetEntryHeight(TunaSweeperInventoryArea::AuxiliaryBagTileHeight);
		for (int32 Index = 0; Index < 2; ++Index)
		{
			FTunaSweeperItemStack EmptyStack;
			EmptyStack.ItemId = INDEX_NONE;
			UTunaSweeperItemStackTileItemObject* TileObject = TunaSweeperInventoryArea::CreateTileObject(
				this,
				TunaSweeperInventoryArea::BuildTileData(ItemDataSubsystem, EmptyStack, ETunaSweeperItemSlotSource::AuxiliaryBag, Index));
			if (TileObject)
			{
				TileObjects.Add(TileObject);
				AuxiliaryBagTileView->AddItem(TileObject);
			}
		}
	}
}
void UTunaSweeperHudInventoryAreaWidget::SetAuxiliaryBagVisible(bool bVisible)
{
	if (AuxiliaryBagPanel)
	{
		AuxiliaryBagPanel->SetVisibility(bVisible ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}
}
