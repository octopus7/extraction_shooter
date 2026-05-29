#include "UI/TunaSweeperLootContainerWidget.h"

#include "Blueprint/DragDropOperation.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/TileView.h"
#include "Engine/Texture2D.h"
#include "Game/TunaSweeperGameInstance.h"
#include "GameFramework/PlayerController.h"
#include "Subsystem/TunaSweeperItemDataSubsystem.h"
#include "UI/TunaSweeperItemDragDropOperation.h"
#include "UI/TunaSweeperItemStackSplitPopupWidget.h"
#include "UI/TunaSweeperItemStackTileItemObject.h"
#include "UI/TunaSweeperUIFont.h"
#include "UI/TunaSweeperUiText.h"

namespace TunaSweeperLootContainerUi
{
	constexpr int32 ContainerTileColumnCount = 5;
	constexpr float ContainerTileWidth = 96.0f;
	constexpr float ContainerTileHeight = 96.0f;
	constexpr float ContainerPanelPadding = 14.0f;
	constexpr float ContainerTileViewScrollbarReserveWidth = 22.0f;
	constexpr float ContainerPanelHeaderHeight = 74.0f;
	constexpr float ContainerPanelWidth =
		ContainerPanelPadding * 2.0f + ContainerTileColumnCount * ContainerTileWidth + ContainerTileViewScrollbarReserveWidth;

	using TunaSweeperUiText::ResolveUiText;

	FTunaSweeperItemStackTileData BuildTileData(
		const UTunaSweeperGameInstance* TunaGameInstance,
		UTunaSweeperItemDataSubsystem* ItemDataSubsystem,
		const FTunaSweeperItemInstance& ItemInstance,
		ETunaSweeperItemSlotSource Source,
		int32 SourceIndex,
		ETunaSweeperItemTextLanguage Language)
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
				TileData.ItemDefinition = ItemDefinition;
				TileData.bHasItemDefinition = true;

				FText DisplayName;
				if (ItemDataSubsystem->TryGetItemNameTextByKey(ItemDefinition.NameStringKey, Language, DisplayName))
				{
					TileData.DisplayName = DisplayName;
				}
				else
				{
					TileData.DisplayName = FText::Format(
						ResolveUiText(TunaGameInstance, TEXT("ui.common.item_fallback"), TEXT("Item {0}")),
						FText::AsNumber(ItemInstance.ItemId));
				}

				const FString IconObjectPath = ItemDataSubsystem->BuildItemIconObjectPath(ItemDefinition);
				if (!IconObjectPath.IsEmpty())
				{
					TileData.IconTexture = TSoftObjectPtr<UTexture2D>(FSoftObjectPath(IconObjectPath));
				}

				FText DescriptionText;
				if (ItemDataSubsystem->TryGetItemTextByKey(ItemDefinition.DescriptionStringKey, Language, DescriptionText))
				{
					TileData.DescriptionText = DescriptionText;
				}
			}
		}

		if (!TileData.bIsEmpty && TileData.DisplayName.IsEmpty())
		{
			TileData.DisplayName = FText::Format(
				ResolveUiText(TunaGameInstance, TEXT("ui.common.item_fallback"), TEXT("Item {0}")),
				FText::AsNumber(ItemInstance.ItemId));
		}

		return TileData;
	}

	FTunaSweeperItemStackTileData BuildShopTileData(
		const UTunaSweeperGameInstance* TunaGameInstance,
		UTunaSweeperItemDataSubsystem* ItemDataSubsystem,
		const FTunaSweeperShopItemView& ShopItemView,
		ETunaSweeperItemTextLanguage Language)
	{
		FTunaSweeperItemStackTileData TileData;
		TileData.ItemStack.ItemId = ShopItemView.ItemId;
		TileData.ItemStack.Quantity = 1;
		TileData.ItemInstance.ItemId = ShopItemView.ItemId;
		TileData.ItemInstance.Quantity = 1;
		TileData.Source = ETunaSweeperItemSlotSource::Shop;
		TileData.SourceIndex = ShopItemView.SlotIndex;
		TileData.SlotReference.Source = ETunaSweeperItemSlotSource::Shop;
		TileData.SlotReference.SlotIndex = ShopItemView.SlotIndex;
		TileData.ShopId = ShopItemView.ShopId;
		TileData.ShopStockQuantity = ShopItemView.StockQuantity;
		TileData.ShopTotalStockQuantity = ShopItemView.TotalStockQuantity;
		TileData.ShopPrice = ShopItemView.Price;
		TileData.bIsEmpty = ShopItemView.ItemId == INDEX_NONE;

		if (!TileData.bIsEmpty && ItemDataSubsystem)
		{
			FTunaSweeperItemDefinition ItemDefinition;
			if (ItemDataSubsystem->TryGetItemDefinition(ShopItemView.ItemId, ItemDefinition))
			{
				TileData.ItemDefinition = ItemDefinition;
				TileData.bHasItemDefinition = true;

				FText DisplayName;
				if (ItemDataSubsystem->TryGetItemNameTextByKey(ItemDefinition.NameStringKey, Language, DisplayName))
				{
					TileData.DisplayName = DisplayName;
				}

				const FString IconObjectPath = ItemDataSubsystem->BuildItemIconObjectPath(ItemDefinition);
				if (!IconObjectPath.IsEmpty())
				{
					TileData.IconTexture = TSoftObjectPtr<UTexture2D>(FSoftObjectPath(IconObjectPath));
				}

				FText DescriptionText;
				if (ItemDataSubsystem->TryGetItemTextByKey(ItemDefinition.DescriptionStringKey, Language, DescriptionText))
				{
					TileData.DescriptionText = DescriptionText;
				}
			}
		}

		if (!TileData.bIsEmpty && TileData.DisplayName.IsEmpty())
		{
			TileData.DisplayName = FText::Format(
				ResolveUiText(TunaGameInstance, TEXT("ui.common.item_fallback"), TEXT("Item {0}")),
				FText::AsNumber(ShopItemView.ItemId));
		}

		return TileData;
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

		FTunaSweeperItemSlotReference SourceSlot = ItemDragOperation->TileData.SlotReference;
		if (!SourceSlot.IsValid())
		{
			SourceSlot.Source = ItemDragOperation->TileData.Source;
			SourceSlot.SlotIndex = ItemDragOperation->TileData.SourceIndex;
		}

		const bool bMoved = TunaGameInstance->MoveItemBetweenSlots(SourceSlot, ItemDragOperation->HoveredSlotReference);
		ItemDragOperation->bHasHoveredSlotReference = false;
		ItemDragOperation->HoveredSlotReference = FTunaSweeperItemSlotReference();
		return bMoved;
	}

	bool TryResolveSlotFromTileView(
		const UTileView* TileView,
		ETunaSweeperItemSlotSource Source,
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
		if (ColumnIndex < 0 || ColumnIndex >= ContainerTileColumnCount || RowIndex < 0)
		{
			return false;
		}

		const int32 FirstVisibleItemIndex = FMath::Max(0, FMath::FloorToInt(TileView->GetScrollOffset()));
		const int32 SlotIndex = FirstVisibleItemIndex + RowIndex * ContainerTileColumnCount + ColumnIndex;
		if (SlotIndex < 0 || SlotIndex >= SlotCount)
		{
			return false;
		}

		OutSlotReference.Source = Source;
		OutSlotReference.SlotIndex = SlotIndex;
		return true;
	}

	FTunaSweeperItemSlotReference ResolveSourceSlot(const UTunaSweeperItemDragDropOperation* ItemDragOperation)
	{
		FTunaSweeperItemSlotReference SourceSlot;
		if (!ItemDragOperation)
		{
			return SourceSlot;
		}

		SourceSlot = ItemDragOperation->TileData.SlotReference;
		if (!SourceSlot.IsValid())
		{
			SourceSlot.Source = ItemDragOperation->TileData.Source;
			SourceSlot.SlotIndex = ItemDragOperation->TileData.SourceIndex;
		}
		return SourceSlot;
	}

	bool TryOpenStackSplitPopupForDrop(
		APlayerController* OwningPlayer,
		UTunaSweeperGameInstance* TunaGameInstance,
		UTunaSweeperItemDragDropOperation* ItemDragOperation,
		const FTunaSweeperItemSlotReference& TargetSlot,
		const FVector2D& ScreenSpacePosition)
	{
		if (!OwningPlayer || !TunaGameInstance || !ItemDragOperation || ItemDragOperation->TileData.bIsEmpty)
		{
			return false;
		}

		const bool bOpenedPopup = UTunaSweeperItemStackSplitPopupWidget::TryOpenStackSplitPopup(
			OwningPlayer,
			TunaGameInstance,
			ResolveSourceSlot(ItemDragOperation),
			TargetSlot,
			ScreenSpacePosition);
		if (bOpenedPopup)
		{
			ItemDragOperation->bHasHoveredSlotReference = false;
			ItemDragOperation->HoveredSlotReference = FTunaSweeperItemSlotReference();
		}

		return bOpenedPopup;
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

	int32 CountOccupiedSlots(const TArray<FTunaSweeperInventorySlot>& Slots)
	{
		int32 OccupiedSlotCount = 0;
		for (const FTunaSweeperInventorySlot& Slot : Slots)
		{
			if (Slot.ItemUid.IsValid())
			{
				++OccupiedSlotCount;
			}
		}
		return OccupiedSlotCount;
	}

	int32 CountOccupiedStacks(const TArray<FTunaSweeperItemStack>& Items)
	{
		int32 OccupiedStackCount = 0;
		for (const FTunaSweeperItemStack& Item : Items)
		{
			if (Item.ItemId != INDEX_NONE && Item.Quantity > 0)
			{
				++OccupiedStackCount;
			}
		}
		return OccupiedStackCount;
	}

	FText GetStorageDisplayName(const UTunaSweeperGameInstance* TunaGameInstance)
	{
		return ResolveUiText(TunaGameInstance, TEXT("ui.storage.title"), TEXT("\uCC3D\uACE0"));
	}

	FText GetShopDisplayName(
		UTunaSweeperGameInstance* TunaGameInstance,
		UTunaSweeperItemDataSubsystem* ItemDataSubsystem,
		int32 ShopId)
	{
		FTunaSweeperShopDefinition ShopDefinition;
		if (ItemDataSubsystem && ItemDataSubsystem->TryGetShopDefinition(ShopId, ShopDefinition))
		{
			FText DisplayName;
			if (!ShopDefinition.NameStringKey.IsNone() &&
				ItemDataSubsystem->TryGetItemTextByKey(
					ShopDefinition.NameStringKey,
					TunaGameInstance ? TunaGameInstance->GetCurrentTextLanguage() : ETunaSweeperItemTextLanguage::English,
					DisplayName))
			{
				return DisplayName;
			}
		}

		return ResolveUiText(TunaGameInstance, TEXT("ui.shop.title"), TEXT("\uC0C1\uC810"));
	}
}

void UTunaSweeperLootContainerWidget::NativeConstruct()
{
	Super::NativeConstruct();
	TunaSweeperUIFont::ApplyFontToWidgetTree(this);

	if (UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>())
	{
		TunaGameInstance->OnInventoryStateChanged.RemoveAll(this);
		TunaGameInstance->OnInventoryStateChanged.AddUObject(this, &UTunaSweeperLootContainerWidget::PopulateContainerItems);
		TunaGameInstance->OnLanguageChanged.RemoveAll(this);
		TunaGameInstance->OnLanguageChanged.AddUObject(this, &UTunaSweeperLootContainerWidget::PopulateContainerItems);
	}

	PopulateContainerItems();
}

void UTunaSweeperLootContainerWidget::NativeDestruct()
{
	if (UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>())
	{
		TunaGameInstance->OnInventoryStateChanged.RemoveAll(this);
		TunaGameInstance->OnLanguageChanged.RemoveAll(this);
	}

	Super::NativeDestruct();
}

bool UTunaSweeperLootContainerWidget::TryResolveDropSlotFromCursor(
	const FVector2D& ScreenSpacePosition,
	FTunaSweeperItemSlotReference& OutSlotReference)
{
	UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	if (SlotSource == ETunaSweeperItemSlotSource::Shop)
	{
		return false;
	}

	const int32 Capacity = SlotSource == ETunaSweeperItemSlotSource::Storage && TunaGameInstance
		? TunaGameInstance->GetStorageSlots().Num()
		: (TunaGameInstance && TunaGameInstance->HasActiveLootContainer()
			? TunaGameInstance->GetActiveLootContainerSlots().Num()
			: FMath::Max(0, ContainerInstance.Capacity));

	return TunaSweeperLootContainerUi::TryResolveSlotFromTileView(
		ContainerTileView,
		SlotSource,
		Capacity,
		ScreenSpacePosition,
		OutSlotReference);
}

void UTunaSweeperLootContainerWidget::SetContainerInstance(const FTunaSweeperLootContainerInstance& InContainerInstance)
{
	SlotSource = ETunaSweeperItemSlotSource::LootContainer;
	ActiveShopId = INDEX_NONE;
	ContainerInstance = InContainerInstance;
	PopulateContainerItems();
}

void UTunaSweeperLootContainerWidget::SetStorageView()
{
	SlotSource = ETunaSweeperItemSlotSource::Storage;
	ActiveShopId = INDEX_NONE;
	ContainerInstance = FTunaSweeperLootContainerInstance();
	PopulateContainerItems();
}

void UTunaSweeperLootContainerWidget::SetShopView(int32 ShopId)
{
	SlotSource = ETunaSweeperItemSlotSource::Shop;
	ActiveShopId = ShopId;
	ContainerInstance = FTunaSweeperLootContainerInstance();
	PopulateContainerItems();
}

bool UTunaSweeperLootContainerWidget::NativeOnDrop(
	const FGeometry& InGeometry,
	const FDragDropEvent& InDragDropEvent,
	UDragDropOperation* InOperation)
{
	UTunaSweeperItemDragDropOperation* ItemDragOperation = Cast<UTunaSweeperItemDragDropOperation>(InOperation);
	if (SlotSource == ETunaSweeperItemSlotSource::Shop || !ItemDragOperation || ItemDragOperation->TileData.bIsEmpty)
	{
		return Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);
	}

	UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	FTunaSweeperItemSlotReference CursorSlotReference;
	if (TryResolveDropSlotFromCursor(InDragDropEvent.GetScreenSpacePosition(), CursorSlotReference) &&
		((InDragDropEvent.GetModifierKeys().IsControlDown() &&
			TunaSweeperLootContainerUi::TryOpenStackSplitPopupForDrop(
				GetOwningPlayer(),
				TunaGameInstance,
				ItemDragOperation,
				CursorSlotReference,
				InDragDropEvent.GetScreenSpacePosition())) ||
			TunaSweeperLootContainerUi::TryMoveFromDropSlot(TunaGameInstance, ItemDragOperation, CursorSlotReference)))
	{
		return true;
	}

	if (InDragDropEvent.GetModifierKeys().IsControlDown() &&
		ItemDragOperation->bHasHoveredSlotReference &&
		TunaSweeperLootContainerUi::TryOpenStackSplitPopupForDrop(
			GetOwningPlayer(),
			TunaGameInstance,
			ItemDragOperation,
			ItemDragOperation->HoveredSlotReference,
			InDragDropEvent.GetScreenSpacePosition()))
	{
		return true;
	}

	if (TunaSweeperLootContainerUi::TryMoveFromHoveredDropSlot(TunaGameInstance, ItemDragOperation))
	{
		return true;
	}

	return Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);
}

void UTunaSweeperLootContainerWidget::PopulateContainerItems()
{
	if (!ContainerTileView)
	{
		return;
	}

	UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	TArray<FTunaSweeperShopItemView> ShopItems;
	const TArray<FTunaSweeperInventorySlot>* Slots = nullptr;
	if (TunaGameInstance)
	{
		if (SlotSource == ETunaSweeperItemSlotSource::Shop)
		{
			TunaGameInstance->GetActiveShopItems(ShopItems);
		}
		else if (SlotSource == ETunaSweeperItemSlotSource::Storage)
		{
			Slots = &TunaGameInstance->GetStorageSlots();
		}
		else if (TunaGameInstance->HasActiveLootContainer())
		{
			Slots = &TunaGameInstance->GetActiveLootContainerSlots();
		}
	}
	const int32 Capacity = SlotSource == ETunaSweeperItemSlotSource::Shop
		? ShopItems.Num()
		: (Slots ? Slots->Num() : FMath::Max(0, ContainerInstance.Capacity));
	const int32 OccupiedSlotCount = SlotSource == ETunaSweeperItemSlotSource::Shop
		? ShopItems.Num()
		: (Slots
			? TunaSweeperLootContainerUi::CountOccupiedSlots(*Slots)
			: TunaSweeperLootContainerUi::CountOccupiedStacks(ContainerInstance.Items));
	const int32 RowCount = FMath::Max(1, FMath::DivideAndRoundUp(Capacity, TunaSweeperLootContainerUi::ContainerTileColumnCount));
	if (RootSizeBox)
	{
		RootSizeBox->SetWidthOverride(TunaSweeperLootContainerUi::ContainerPanelWidth);
		RootSizeBox->SetHeightOverride(
			TunaSweeperLootContainerUi::ContainerPanelHeaderHeight + RowCount * TunaSweeperLootContainerUi::ContainerTileHeight);
	}

	if (ContainerTitleText)
	{
		FText DisplayName;
		if (SlotSource == ETunaSweeperItemSlotSource::Shop)
		{
			UTunaSweeperItemDataSubsystem* TitleItemDataSubsystem = GetGameInstance()
				? GetGameInstance()->GetSubsystem<UTunaSweeperItemDataSubsystem>()
				: nullptr;
			DisplayName = TunaSweeperLootContainerUi::GetShopDisplayName(
				TunaGameInstance,
				TitleItemDataSubsystem,
				ActiveShopId);
		}
		else if (SlotSource == ETunaSweeperItemSlotSource::Storage)
		{
			DisplayName = TunaSweeperLootContainerUi::GetStorageDisplayName(TunaGameInstance);
		}
		else
		{
			DisplayName = TunaGameInstance && TunaGameInstance->HasActiveLootContainer()
				? TunaGameInstance->GetActiveLootContainerDisplayName()
				: ContainerInstance.DisplayName;
		}
		ContainerTitleText->SetText(DisplayName.IsEmpty()
			? TunaSweeperLootContainerUi::ResolveUiText(
				TunaGameInstance,
				TEXT("ui.common.container_fallback"),
				TEXT("Container"))
			: DisplayName);
	}
	if (ContainerOccupancyText)
	{
		ContainerOccupancyText->SetText(FText::FromString(FString::Printf(TEXT("(%d/%d)"), OccupiedSlotCount, Capacity)));
	}

	UTunaSweeperItemDataSubsystem* ItemDataSubsystem = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UTunaSweeperItemDataSubsystem>()
		: nullptr;
	const ETunaSweeperItemTextLanguage Language = TunaGameInstance
		? TunaGameInstance->GetCurrentTextLanguage()
		: ETunaSweeperItemTextLanguage::English;

	TileObjects.Reset();
	ContainerTileView->ClearListItems();
	ContainerTileView->SetEntryWidth(TunaSweeperLootContainerUi::ContainerTileWidth);
	ContainerTileView->SetEntryHeight(TunaSweeperLootContainerUi::ContainerTileHeight);

	for (int32 SlotIndex = 0; SlotIndex < Capacity; ++SlotIndex)
	{
		FTunaSweeperItemInstance ItemInstance;
		if (Slots && Slots->IsValidIndex(SlotIndex) && (*Slots)[SlotIndex].ItemUid.IsValid() && TunaGameInstance)
		{
			TunaGameInstance->TryGetItemInstance((*Slots)[SlotIndex].ItemUid, ItemInstance);
		}

		UTunaSweeperItemStackTileItemObject* TileObject = NewObject<UTunaSweeperItemStackTileItemObject>(this);
		if (!TileObject)
		{
			continue;
		}

		TileObject->Initialize(SlotSource == ETunaSweeperItemSlotSource::Shop && ShopItems.IsValidIndex(SlotIndex)
			? TunaSweeperLootContainerUi::BuildShopTileData(
				TunaGameInstance,
				ItemDataSubsystem,
				ShopItems[SlotIndex],
				Language)
			: TunaSweeperLootContainerUi::BuildTileData(
				TunaGameInstance,
				ItemDataSubsystem,
				ItemInstance,
				SlotSource,
				SlotIndex,
				Language));
		TileObjects.Add(TileObject);
		ContainerTileView->AddItem(TileObject);
	}
}
