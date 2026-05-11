#include "UI/TunaSweeperLootContainerWidget.h"

#include "Blueprint/DragDropOperation.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/TileView.h"
#include "Engine/Engine.h"
#include "Engine/Texture2D.h"
#include "Subsystem/TunaSweeperItemDataSubsystem.h"
#include "UI/TunaSweeperItemDragDropOperation.h"
#include "UI/TunaSweeperItemStackTileItemObject.h"

namespace TunaSweeperLootContainerUi
{
	FTunaSweeperItemStackTileData BuildTileData(
		UTunaSweeperItemDataSubsystem* ItemDataSubsystem,
		const FTunaSweeperItemStack& ItemStack,
		int32 SourceIndex)
	{
		FTunaSweeperItemStackTileData TileData;
		TileData.ItemStack = ItemStack;
		TileData.Source = ETunaSweeperItemSlotSource::LootContainer;
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
}

void UTunaSweeperLootContainerWidget::NativeConstruct()
{
	Super::NativeConstruct();
	PopulateContainerItems();
}

void UTunaSweeperLootContainerWidget::SetContainerInstance(const FTunaSweeperLootContainerInstance& InContainerInstance)
{
	ContainerInstance = InContainerInstance;
	PopulateContainerItems();
}

bool UTunaSweeperLootContainerWidget::NativeOnDrop(
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
			FString::Printf(TEXT("[Container] Drop received: item=%d qty=%d"),
				ItemDragOperation->TileData.ItemStack.ItemId,
				ItemDragOperation->TileData.ItemStack.Quantity));
	}

	return true;
}

void UTunaSweeperLootContainerWidget::PopulateContainerItems()
{
	if (!ContainerTileView)
	{
		return;
	}

	const int32 Capacity = FMath::Clamp(ContainerInstance.Capacity, 5, 15);
	const int32 RowCount = FMath::Max(1, FMath::DivideAndRoundUp(Capacity, 5));
	if (RootSizeBox)
	{
		RootSizeBox->SetHeightOverride(74.0f + RowCount * 116.0f);
	}

	if (ContainerTitleText)
	{
		ContainerTitleText->SetText(ContainerInstance.DisplayName.IsEmpty()
			? FText::FromString(TEXT("Container"))
			: ContainerInstance.DisplayName);
	}

	UTunaSweeperItemDataSubsystem* ItemDataSubsystem = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UTunaSweeperItemDataSubsystem>()
		: nullptr;

	TileObjects.Reset();
	ContainerTileView->ClearListItems();
	ContainerTileView->SetEntryWidth(96.0f);
	ContainerTileView->SetEntryHeight(116.0f);

	for (int32 SlotIndex = 0; SlotIndex < Capacity; ++SlotIndex)
	{
		FTunaSweeperItemStack ItemStack;
		if (ContainerInstance.Items.IsValidIndex(SlotIndex))
		{
			ItemStack = ContainerInstance.Items[SlotIndex];
		}
		else
		{
			ItemStack.ItemId = INDEX_NONE;
		}

		UTunaSweeperItemStackTileItemObject* TileObject = NewObject<UTunaSweeperItemStackTileItemObject>(this);
		if (!TileObject)
		{
			continue;
		}

		TileObject->Initialize(TunaSweeperLootContainerUi::BuildTileData(ItemDataSubsystem, ItemStack, SlotIndex));
		TileObjects.Add(TileObject);
		ContainerTileView->AddItem(TileObject);
	}
}

