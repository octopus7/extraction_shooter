#include "UI/TunaSweeperLootContainerWidget.h"

#include "Blueprint/DragDropOperation.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/TileView.h"
#include "Engine/Texture2D.h"
#include "Game/TunaSweeperGameInstance.h"
#include "GameFramework/PlayerController.h"
#include "Subsystem/TunaSweeperItemDataSubsystem.h"
#include "UI/TunaSweeperCurrencyDisplayWidget.h"
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
	constexpr float ShopTileHeight = 118.0f;
	constexpr float ContainerPanelPadding = 14.0f;
	constexpr float ContainerTileViewScrollbarReserveWidth = 22.0f;
	constexpr float ContainerPanelHeaderHeight = 74.0f;
	constexpr float ContainerPanelWidth =
		ContainerPanelPadding * 2.0f + ContainerTileColumnCount * ContainerTileWidth + ContainerTileViewScrollbarReserveWidth;

	using TunaSweeperUiText::ResolveUiText;

	int32 RoundUpToUiSlotCount(int32 SlotCount)
	{
		return SlotCount > 0
			? FMath::DivideAndRoundUp(SlotCount, ContainerTileColumnCount) * ContainerTileColumnCount
			: 0;
	}

	float ResolveEntryHeight(ETunaSweeperItemSlotSource Source)
	{
		return Source == ETunaSweeperItemSlotSource::Shop
			? ShopTileHeight
			: ContainerTileHeight;
	}

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

	FText BuildWorkbenchIngredientText(
		const UTunaSweeperGameInstance* TunaGameInstance,
		UTunaSweeperItemDataSubsystem* ItemDataSubsystem,
		const FTunaSweeperWorkbenchRecipeView& RecipeView,
		ETunaSweeperItemTextLanguage Language)
	{
		TArray<FString> IngredientLines;
		for (const FTunaSweeperWorkbenchIngredientView& IngredientView : RecipeView.Ingredients)
		{
			FText IngredientName;
			if (!ItemDataSubsystem ||
				!ItemDataSubsystem->TryGetItemNameText(IngredientView.ItemId, Language, IngredientName))
			{
				IngredientName = FText::Format(
					ResolveUiText(TunaGameInstance, TEXT("ui.common.item_fallback"), TEXT("Item {0}")),
					FText::AsNumber(IngredientView.ItemId));
			}

			IngredientLines.Add(FString::Printf(
				TEXT("%s %d/%d"),
				*IngredientName.ToString(),
				FMath::Max(0, IngredientView.AvailableQuantity),
				FMath::Max(1, IngredientView.RequiredQuantity)));
		}

		return FText::FromString(FString::Join(IngredientLines, TEXT("\n")));
	}

	FTunaSweeperItemStackTileData BuildWorkbenchTileData(
		const UTunaSweeperGameInstance* TunaGameInstance,
		UTunaSweeperItemDataSubsystem* ItemDataSubsystem,
		const FTunaSweeperWorkbenchRecipeView& RecipeView,
		ETunaSweeperItemTextLanguage Language)
	{
		FTunaSweeperItemStackTileData TileData;
		TileData.ItemStack.ItemId = RecipeView.OutputItemId;
		TileData.ItemStack.Quantity = FMath::Max(1, RecipeView.OutputQuantity);
		TileData.ItemInstance.ItemId = RecipeView.OutputItemId;
		TileData.ItemInstance.Quantity = FMath::Max(1, RecipeView.OutputQuantity);
		TileData.Source = ETunaSweeperItemSlotSource::WorkbenchRecipe;
		TileData.SourceIndex = RecipeView.SlotIndex;
		TileData.SlotReference.Source = ETunaSweeperItemSlotSource::WorkbenchRecipe;
		TileData.SlotReference.SlotIndex = RecipeView.SlotIndex;
		TileData.WorkbenchId = RecipeView.WorkbenchId;
		TileData.WorkbenchRecipeId = RecipeView.RecipeId;
		TileData.WorkbenchIngredientText = BuildWorkbenchIngredientText(
			TunaGameInstance,
			ItemDataSubsystem,
			RecipeView,
			Language);
		TileData.WorkbenchMissingIngredientCount = RecipeView.MissingIngredientCount;
		TileData.bCanCraftWorkbenchRecipe = RecipeView.bCanCraft;
		TileData.bIsEmpty = RecipeView.OutputItemId == INDEX_NONE;

		if (!TileData.bIsEmpty && ItemDataSubsystem)
		{
			FTunaSweeperItemDefinition ItemDefinition;
			if (ItemDataSubsystem->TryGetItemDefinition(RecipeView.OutputItemId, ItemDefinition))
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
				FText::AsNumber(RecipeView.OutputItemId));
		}

		return TileData;
	}

	FText BuildItemStackLinesText(
		const UTunaSweeperGameInstance* TunaGameInstance,
		UTunaSweeperItemDataSubsystem* ItemDataSubsystem,
		const TArray<FTunaSweeperItemStack>& ItemStacks,
		ETunaSweeperItemTextLanguage Language)
	{
		TArray<FString> ResultLines;
		for (const FTunaSweeperItemStack& ItemStack : ItemStacks)
		{
			FText ItemName;
			if (!ItemDataSubsystem ||
				!ItemDataSubsystem->TryGetItemNameText(ItemStack.ItemId, Language, ItemName))
			{
				ItemName = FText::Format(
					ResolveUiText(TunaGameInstance, TEXT("ui.common.item_fallback"), TEXT("Item {0}")),
					FText::AsNumber(ItemStack.ItemId));
			}

			ResultLines.Add(FString::Printf(
				TEXT("%s x%d"),
				*ItemName.ToString(),
				FMath::Max(1, ItemStack.Quantity)));
		}

		return FText::FromString(FString::Join(ResultLines, TEXT("\n")));
	}

	FTunaSweeperItemStackTileData BuildWorkbenchDismantleTileData(
		const UTunaSweeperGameInstance* TunaGameInstance,
		UTunaSweeperItemDataSubsystem* ItemDataSubsystem,
		const FTunaSweeperWorkbenchDismantleCandidateView& CandidateView,
		ETunaSweeperItemTextLanguage Language)
	{
		FTunaSweeperItemStackTileData TileData = BuildTileData(
			TunaGameInstance,
			ItemDataSubsystem,
			FTunaSweeperItemInstance(),
			ETunaSweeperItemSlotSource::WorkbenchDismantleItem,
			CandidateView.ListIndex,
			Language);
		TileData.ItemStack.ItemId = CandidateView.ItemId;
		TileData.ItemStack.Quantity = FMath::Max(1, CandidateView.Quantity);
		TileData.ItemInstance.ItemId = CandidateView.ItemId;
		TileData.ItemInstance.Quantity = FMath::Max(1, CandidateView.Quantity);
		TileData.Source = ETunaSweeperItemSlotSource::WorkbenchDismantleItem;
		TileData.SourceIndex = CandidateView.ListIndex;
		TileData.SlotReference = CandidateView.SlotReference;
		TileData.WorkbenchDismantleResultText = BuildItemStackLinesText(
			TunaGameInstance,
			ItemDataSubsystem,
			CandidateView.Results,
			Language);
		TileData.bCanDismantleWorkbenchItem = CandidateView.bCanDismantle;
		TileData.bIsEmpty = CandidateView.ItemId == INDEX_NONE;

		if (!TileData.bIsEmpty && ItemDataSubsystem)
		{
			FTunaSweeperItemDefinition ItemDefinition;
			if (ItemDataSubsystem->TryGetItemDefinition(CandidateView.ItemId, ItemDefinition))
			{
				TileData.ItemDefinition = ItemDefinition;
				TileData.bHasItemDefinition = true;
				ItemDataSubsystem->TryGetItemNameTextByKey(ItemDefinition.NameStringKey, Language, TileData.DisplayName);
				ItemDataSubsystem->TryGetItemTextByKey(ItemDefinition.DescriptionStringKey, Language, TileData.DescriptionText);
				const FString IconObjectPath = ItemDataSubsystem->BuildItemIconObjectPath(ItemDefinition);
				if (!IconObjectPath.IsEmpty())
				{
					TileData.IconTexture = TSoftObjectPtr<UTexture2D>(FSoftObjectPath(IconObjectPath));
				}
			}
		}

		if (!TileData.bIsEmpty && TileData.DisplayName.IsEmpty())
		{
			TileData.DisplayName = FText::Format(
				ResolveUiText(TunaGameInstance, TEXT("ui.common.item_fallback"), TEXT("Item {0}")),
				FText::AsNumber(CandidateView.ItemId));
		}
		return TileData;
	}

	FTunaSweeperItemStackTileData BuildWorkbenchBlueprintTileData(
		const UTunaSweeperGameInstance* TunaGameInstance,
		UTunaSweeperItemDataSubsystem* ItemDataSubsystem,
		const FTunaSweeperWorkbenchBlueprintItemView& BlueprintItemView,
		ETunaSweeperItemTextLanguage Language)
	{
		FTunaSweeperItemStackTileData TileData = BuildTileData(
			TunaGameInstance,
			ItemDataSubsystem,
			FTunaSweeperItemInstance(),
			ETunaSweeperItemSlotSource::WorkbenchBlueprintItem,
			BlueprintItemView.ListIndex,
			Language);
		TileData.ItemStack.ItemId = BlueprintItemView.ItemId;
		TileData.ItemStack.Quantity = FMath::Max(1, BlueprintItemView.Quantity);
		TileData.ItemInstance.ItemId = BlueprintItemView.ItemId;
		TileData.ItemInstance.Quantity = FMath::Max(1, BlueprintItemView.Quantity);
		TileData.Source = ETunaSweeperItemSlotSource::WorkbenchBlueprintItem;
		TileData.SourceIndex = BlueprintItemView.ListIndex;
		TileData.SlotReference = BlueprintItemView.SlotReference;
		TileData.WorkbenchBlueprintRecipeId = BlueprintItemView.RecipeId;
		TileData.bCanRegisterWorkbenchBlueprint = BlueprintItemView.bCanRegister;
		TileData.bWorkbenchBlueprintAlreadyUnlocked = BlueprintItemView.bAlreadyUnlocked;
		TileData.bIsEmpty = BlueprintItemView.ItemId == INDEX_NONE;

		if (!TileData.bIsEmpty && ItemDataSubsystem)
		{
			FTunaSweeperItemDefinition ItemDefinition;
			if (ItemDataSubsystem->TryGetItemDefinition(BlueprintItemView.ItemId, ItemDefinition))
			{
				TileData.ItemDefinition = ItemDefinition;
				TileData.bHasItemDefinition = true;
				ItemDataSubsystem->TryGetItemNameTextByKey(ItemDefinition.NameStringKey, Language, TileData.DisplayName);
				ItemDataSubsystem->TryGetItemTextByKey(ItemDefinition.DescriptionStringKey, Language, TileData.DescriptionText);
				const FString IconObjectPath = ItemDataSubsystem->BuildItemIconObjectPath(ItemDefinition);
				if (!IconObjectPath.IsEmpty())
				{
					TileData.IconTexture = TSoftObjectPtr<UTexture2D>(FSoftObjectPath(IconObjectPath));
				}
			}
		}

		if (!TileData.bIsEmpty && TileData.DisplayName.IsEmpty())
		{
			TileData.DisplayName = FText::Format(
				ResolveUiText(TunaGameInstance, TEXT("ui.common.item_fallback"), TEXT("Item {0}")),
				FText::AsNumber(BlueprintItemView.ItemId));
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

	FText GetWorkbenchDisplayName(const UTunaSweeperGameInstance* TunaGameInstance, ETunaSweeperWorkbenchMode WorkbenchMode)
	{
		switch (WorkbenchMode)
		{
		case ETunaSweeperWorkbenchMode::Dismantle:
			return ResolveUiText(TunaGameInstance, TEXT("ui.workbench.dismantle"), TEXT("\uBD84\uD574"));
		case ETunaSweeperWorkbenchMode::BlueprintRegister:
			return ResolveUiText(TunaGameInstance, TEXT("ui.workbench.blueprint_register"), TEXT("\uC124\uACC4\uB3C4 \uB4F1\uB85D"));
		default:
			return ResolveUiText(TunaGameInstance, TEXT("ui.workbench.craft"), TEXT("\uC81C\uC870"));
		}
	}
}

void UTunaSweeperItemContainerPanelWidget::NativeConstruct()
{
	Super::NativeConstruct();
	TunaSweeperUIFont::ApplyFontToWidgetTree(this);
	EnsureShopCurrencyDisplayWidget();

	if (UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>())
	{
		TunaGameInstance->OnInventoryStateChanged.RemoveAll(this);
		TunaGameInstance->OnInventoryStateChanged.AddUObject(this, &UTunaSweeperItemContainerPanelWidget::PopulateContainerItems);
		TunaGameInstance->OnLanguageChanged.RemoveAll(this);
		TunaGameInstance->OnLanguageChanged.AddUObject(this, &UTunaSweeperItemContainerPanelWidget::PopulateContainerItems);
	}

	PopulateContainerItems();
}

void UTunaSweeperItemContainerPanelWidget::NativeDestruct()
{
	if (UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>())
	{
		TunaGameInstance->OnInventoryStateChanged.RemoveAll(this);
		TunaGameInstance->OnLanguageChanged.RemoveAll(this);
	}

	Super::NativeDestruct();
}

void UTunaSweeperItemContainerPanelWidget::RefreshHeaderControls()
{
	EnsureShopCurrencyDisplayWidget();

	if (ShopRefreshStockButton)
	{
		ShopRefreshStockButton->SetVisibility(ESlateVisibility::Collapsed);
		ShopRefreshStockButton->SetIsEnabled(false);
	}
	if (ShopRefreshStockButtonText)
	{
		ShopRefreshStockButtonText->SetText(TunaSweeperLootContainerUi::ResolveUiText(
			GetGameInstance<UTunaSweeperGameInstance>(),
			TEXT("ui.shop.debug_refresh_stock"),
			TEXT("\uAC31\uC2E0")));
	}
	if (ShopCurrencyDisplayWidget)
	{
		ShopCurrencyDisplayWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UTunaSweeperItemContainerPanelWidget::EnsureShopCurrencyDisplayWidget()
{
	if (ShopCurrencyDisplayWidget || !WidgetTree)
	{
		return;
	}

	UHorizontalBox* ContainerHeaderRow = Cast<UHorizontalBox>(WidgetTree->FindWidget(FName(TEXT("ContainerHeaderRow"))));
	if (!ContainerHeaderRow)
	{
		return;
	}

	ShopCurrencyDisplayWidget = WidgetTree->ConstructWidget<UTunaSweeperCurrencyDisplayWidget>(
		UTunaSweeperCurrencyDisplayWidget::StaticClass(),
		TEXT("ShopCurrencyDisplayWidget"));
	if (!ShopCurrencyDisplayWidget)
	{
		return;
	}

	UHorizontalBoxSlot* CurrencySlot = ContainerHeaderRow->AddChildToHorizontalBox(ShopCurrencyDisplayWidget);
	if (CurrencySlot)
	{
		CurrencySlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		CurrencySlot->SetHorizontalAlignment(HAlign_Right);
		CurrencySlot->SetVerticalAlignment(VAlign_Center);
		CurrencySlot->SetPadding(FMargin(12.0f, 0.0f, 0.0f, 0.0f));
	}

	ShopCurrencyDisplayWidget->SetVisibility(ESlateVisibility::Collapsed);
}

bool UTunaSweeperItemContainerPanelWidget::TryResolveDropSlotFromCursor(
	const FVector2D& ScreenSpacePosition,
	FTunaSweeperItemSlotReference& OutSlotReference)
{
	UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	if (SlotSource == ETunaSweeperItemSlotSource::Shop ||
		SlotSource == ETunaSweeperItemSlotSource::WorkbenchRecipe ||
		SlotSource == ETunaSweeperItemSlotSource::WorkbenchDismantleItem ||
		SlotSource == ETunaSweeperItemSlotSource::WorkbenchBlueprintItem)
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

void UTunaSweeperItemContainerPanelWidget::SetContainerInstanceInternal(
	const FTunaSweeperLootContainerInstance& InContainerInstance)
{
	SlotSource = ETunaSweeperItemSlotSource::LootContainer;
	ActiveShopId = INDEX_NONE;
	ActiveWorkbenchId = INDEX_NONE;
	ActiveWorkbenchMode = ETunaSweeperWorkbenchMode::Craft;
	ContainerInstance = InContainerInstance;
	PopulateContainerItems();
}

void UTunaSweeperItemContainerPanelWidget::SetStorageViewInternal()
{
	SlotSource = ETunaSweeperItemSlotSource::Storage;
	ActiveShopId = INDEX_NONE;
	ActiveWorkbenchId = INDEX_NONE;
	ActiveWorkbenchMode = ETunaSweeperWorkbenchMode::Craft;
	ContainerInstance = FTunaSweeperLootContainerInstance();
	PopulateContainerItems();
}

void UTunaSweeperItemContainerPanelWidget::SetShopViewInternal(int32 ShopId)
{
	SlotSource = ETunaSweeperItemSlotSource::Shop;
	ActiveShopId = ShopId;
	ActiveWorkbenchId = INDEX_NONE;
	ActiveWorkbenchMode = ETunaSweeperWorkbenchMode::Craft;
	ContainerInstance = FTunaSweeperLootContainerInstance();
	PopulateContainerItems();
}

void UTunaSweeperItemContainerPanelWidget::SetWorkbenchViewInternal(
	int32 WorkbenchId,
	ETunaSweeperWorkbenchMode WorkbenchMode)
{
	SlotSource = WorkbenchMode == ETunaSweeperWorkbenchMode::Dismantle
		? ETunaSweeperItemSlotSource::WorkbenchDismantleItem
		: (WorkbenchMode == ETunaSweeperWorkbenchMode::BlueprintRegister
			? ETunaSweeperItemSlotSource::WorkbenchBlueprintItem
			: ETunaSweeperItemSlotSource::WorkbenchRecipe);
	ActiveShopId = INDEX_NONE;
	ActiveWorkbenchId = WorkbenchId;
	ActiveWorkbenchMode = WorkbenchMode;
	ContainerInstance = FTunaSweeperLootContainerInstance();
	PopulateContainerItems();
}

bool UTunaSweeperItemContainerPanelWidget::NativeOnDrop(
	const FGeometry& InGeometry,
	const FDragDropEvent& InDragDropEvent,
	UDragDropOperation* InOperation)
{
	UTunaSweeperItemDragDropOperation* ItemDragOperation = Cast<UTunaSweeperItemDragDropOperation>(InOperation);
	if (SlotSource == ETunaSweeperItemSlotSource::Shop ||
		SlotSource == ETunaSweeperItemSlotSource::WorkbenchRecipe ||
		SlotSource == ETunaSweeperItemSlotSource::WorkbenchDismantleItem ||
		SlotSource == ETunaSweeperItemSlotSource::WorkbenchBlueprintItem ||
		!ItemDragOperation ||
		ItemDragOperation->TileData.bIsEmpty)
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

void UTunaSweeperItemContainerPanelWidget::PopulateContainerItems()
{
	RefreshHeaderControls();

	if (!ContainerTileView)
	{
		return;
	}

	UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	TArray<FTunaSweeperShopItemView> ShopItems;
	TArray<FTunaSweeperWorkbenchRecipeView> WorkbenchRecipes;
	TArray<FTunaSweeperWorkbenchDismantleCandidateView> DismantleCandidates;
	TArray<FTunaSweeperWorkbenchBlueprintItemView> BlueprintItems;
	const TArray<FTunaSweeperInventorySlot>* Slots = nullptr;
	if (TunaGameInstance)
	{
		if (SlotSource == ETunaSweeperItemSlotSource::Shop)
		{
			TunaGameInstance->GetActiveShopItems(ShopItems);
		}
		else if (SlotSource == ETunaSweeperItemSlotSource::WorkbenchRecipe)
		{
			TunaGameInstance->GetActiveWorkbenchRecipes(WorkbenchRecipes);
		}
		else if (SlotSource == ETunaSweeperItemSlotSource::WorkbenchDismantleItem)
		{
			TunaGameInstance->GetActiveWorkbenchDismantleCandidates(DismantleCandidates);
		}
		else if (SlotSource == ETunaSweeperItemSlotSource::WorkbenchBlueprintItem)
		{
			TunaGameInstance->GetActiveWorkbenchBlueprintItems(BlueprintItems);
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
	int32 Capacity = Slots ? Slots->Num() : FMath::Max(0, ContainerInstance.Capacity);
	int32 OccupiedSlotCount = Slots
		? TunaSweeperLootContainerUi::CountOccupiedSlots(*Slots)
		: TunaSweeperLootContainerUi::CountOccupiedStacks(ContainerInstance.Items);
	if (SlotSource == ETunaSweeperItemSlotSource::Shop)
	{
		Capacity = ShopItems.Num();
		OccupiedSlotCount = ShopItems.Num();
	}
	else if (SlotSource == ETunaSweeperItemSlotSource::WorkbenchRecipe)
	{
		Capacity = WorkbenchRecipes.Num();
		OccupiedSlotCount = WorkbenchRecipes.Num();
	}
	else if (SlotSource == ETunaSweeperItemSlotSource::WorkbenchDismantleItem)
	{
		Capacity = DismantleCandidates.Num();
		OccupiedSlotCount = DismantleCandidates.Num();
	}
	else if (SlotSource == ETunaSweeperItemSlotSource::WorkbenchBlueprintItem)
	{
		Capacity = BlueprintItems.Num();
		OccupiedSlotCount = BlueprintItems.Num();
	}
	const int32 UiSlotCount = TunaSweeperLootContainerUi::RoundUpToUiSlotCount(Capacity);
	const int32 RowCount = FMath::Max(
		1,
		FMath::DivideAndRoundUp(UiSlotCount, TunaSweeperLootContainerUi::ContainerTileColumnCount));
	const float EntryHeight = TunaSweeperLootContainerUi::ResolveEntryHeight(SlotSource);
	if (RootSizeBox)
	{
		RootSizeBox->SetWidthOverride(TunaSweeperLootContainerUi::ContainerPanelWidth);
		RootSizeBox->SetHeightOverride(
			TunaSweeperLootContainerUi::ContainerPanelHeaderHeight + RowCount * EntryHeight);
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
		else if (SlotSource == ETunaSweeperItemSlotSource::WorkbenchRecipe ||
			SlotSource == ETunaSweeperItemSlotSource::WorkbenchDismantleItem ||
			SlotSource == ETunaSweeperItemSlotSource::WorkbenchBlueprintItem)
		{
			DisplayName = TunaSweeperLootContainerUi::GetWorkbenchDisplayName(TunaGameInstance, ActiveWorkbenchMode);
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
	ContainerTileView->SetEntryHeight(EntryHeight);

	for (int32 SlotIndex = 0; SlotIndex < UiSlotCount; ++SlotIndex)
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

		FTunaSweeperItemStackTileData TileData;
		if (SlotSource == ETunaSweeperItemSlotSource::Shop && ShopItems.IsValidIndex(SlotIndex))
		{
			TileData = TunaSweeperLootContainerUi::BuildShopTileData(
				TunaGameInstance,
				ItemDataSubsystem,
				ShopItems[SlotIndex],
				Language);
		}
		else if (SlotSource == ETunaSweeperItemSlotSource::WorkbenchRecipe && WorkbenchRecipes.IsValidIndex(SlotIndex))
		{
			TileData = TunaSweeperLootContainerUi::BuildWorkbenchTileData(
				TunaGameInstance,
				ItemDataSubsystem,
				WorkbenchRecipes[SlotIndex],
				Language);
		}
		else if (SlotSource == ETunaSweeperItemSlotSource::WorkbenchDismantleItem && DismantleCandidates.IsValidIndex(SlotIndex))
		{
			TileData = TunaSweeperLootContainerUi::BuildWorkbenchDismantleTileData(
				TunaGameInstance,
				ItemDataSubsystem,
				DismantleCandidates[SlotIndex],
				Language);
		}
		else if (SlotSource == ETunaSweeperItemSlotSource::WorkbenchBlueprintItem && BlueprintItems.IsValidIndex(SlotIndex))
		{
			TileData = TunaSweeperLootContainerUi::BuildWorkbenchBlueprintTileData(
				TunaGameInstance,
				ItemDataSubsystem,
				BlueprintItems[SlotIndex],
				Language);
		}
		else
		{
			TileData = TunaSweeperLootContainerUi::BuildTileData(
				TunaGameInstance,
				ItemDataSubsystem,
				ItemInstance,
				SlotSource,
				SlotIndex,
				Language);
		}

		TileObject->Initialize(TileData);
		TileObjects.Add(TileObject);
		ContainerTileView->AddItem(TileObject);
	}
}

void UTunaSweeperLootContainerWidget::SetContainerInstance(const FTunaSweeperLootContainerInstance& InContainerInstance)
{
	SetContainerInstanceInternal(InContainerInstance);
}

void UTunaSweeperLootContainerWidget::SetStorageView()
{
	SetStorageViewInternal();
}

void UTunaSweeperLootContainerWidget::SetShopView(int32 ShopId)
{
	SetShopViewInternal(ShopId);
}

void UTunaSweeperLootContainerWidget::SetWorkbenchView(int32 WorkbenchId, ETunaSweeperWorkbenchMode WorkbenchMode)
{
	SetWorkbenchViewInternal(WorkbenchId, WorkbenchMode);
}

void UTunaSweeperStorageContainerWidget::SetStorageView()
{
	SetStorageViewInternal();
}

void UTunaSweeperShopContainerWidget::SetShopView(int32 ShopId)
{
	SetShopViewInternal(ShopId);
}

void UTunaSweeperShopContainerWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (ShopRefreshStockButton)
	{
		ShopRefreshStockButton->OnClicked.RemoveDynamic(
			this,
			&UTunaSweeperShopContainerWidget::HandleShopRefreshStockButtonClicked);
		ShopRefreshStockButton->OnClicked.AddDynamic(
			this,
			&UTunaSweeperShopContainerWidget::HandleShopRefreshStockButtonClicked);
	}
}

void UTunaSweeperShopContainerWidget::NativeDestruct()
{
	if (ShopRefreshStockButton)
	{
		ShopRefreshStockButton->OnClicked.RemoveDynamic(
			this,
			&UTunaSweeperShopContainerWidget::HandleShopRefreshStockButtonClicked);
	}

	Super::NativeDestruct();
}

void UTunaSweeperShopContainerWidget::RefreshHeaderControls()
{
	if (ShopRefreshStockButton)
	{
		ShopRefreshStockButton->SetVisibility(ESlateVisibility::Visible);
		ShopRefreshStockButton->SetIsEnabled(true);
	}
	if (ShopRefreshStockButtonText)
	{
		ShopRefreshStockButtonText->SetText(TunaSweeperLootContainerUi::ResolveUiText(
			GetGameInstance<UTunaSweeperGameInstance>(),
			TEXT("ui.shop.debug_refresh_stock"),
			TEXT("\uAC31\uC2E0")));
	}
	if (ShopCurrencyDisplayWidget)
	{
		ShopCurrencyDisplayWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UTunaSweeperShopContainerWidget::HandleShopRefreshStockButtonClicked()
{
	UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	if (TunaGameInstance && TunaGameInstance->DebugRestockActiveShop(true))
	{
		PopulateContainerItems();
	}
}
