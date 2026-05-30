#include "UI/ItemContainerWidget.h"

#include "Blueprint/DragDropOperation.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/TileView.h"
#include "Components/VerticalBoxSlot.h"
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

namespace ItemContainerUi
{
	constexpr int32 ContainerTileColumnCount = 5;
	constexpr float ContainerTileWidth = 96.0f;
	constexpr float ContainerTileHeight = 96.0f;
	constexpr float ShopTileHeight = 118.0f;
	constexpr float ContainerPanelPadding = 14.0f;
	constexpr float ContainerTileViewScrollbarReserveWidth = 22.0f;
	constexpr float ContainerPanelHeaderHeight = 74.0f;
	constexpr float StorageFilterIconSize = 21.0f;
	constexpr float StorageFilterHeaderTopPadding = 5.0f;
	constexpr float StorageFilterHeaderBottomPadding = 5.0f;
	constexpr float StorageFilterTileTopPadding = 4.0f;
	constexpr int32 ShopVisibleRowCount = 6;
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

	float ResolveHeaderHeight(ETunaSweeperItemSlotSource)
	{
		return ContainerPanelHeaderHeight;
	}

	bool IsWeaponCategory(FName CategoryTag)
	{
		return CategoryTag == FName(TEXT("item.category.weapon.gun")) ||
			CategoryTag == FName(TEXT("item.category.weapon.melee"));
	}

	bool IsGearCategory(FName CategoryTag)
	{
		return CategoryTag == FName(TEXT("item.category.head")) ||
			CategoryTag == FName(TEXT("item.category.body")) ||
			CategoryTag == FName(TEXT("item.category.face")) ||
			CategoryTag == FName(TEXT("item.category.ear")) ||
			CategoryTag == FName(TEXT("item.category.bag"));
	}

	bool IsKnownStorageFilterCategory(FName CategoryTag)
	{
		return IsWeaponCategory(CategoryTag) ||
			CategoryTag == FName(TEXT("item.category.ammo")) ||
			CategoryTag == FName(TEXT("item.category.attachment")) ||
			CategoryTag == FName(TEXT("item.category.consumable")) ||
			CategoryTag == FName(TEXT("item.category.throwable")) ||
			IsGearCategory(CategoryTag) ||
			CategoryTag == FName(TEXT("item.category.material")) ||
			CategoryTag == FName(TEXT("item.category.blueprint"));
	}

	bool DoesItemMatchStorageFilter(
		const FTunaSweeperItemDefinition& ItemDefinition,
		bool bHasItemDefinition,
		ETunaSweeperStorageFilter Filter)
	{
		if (Filter == ETunaSweeperStorageFilter::All)
		{
			return true;
		}

		const FName CategoryTag = bHasItemDefinition ? ItemDefinition.CategoryTag : NAME_None;
		switch (Filter)
		{
		case ETunaSweeperStorageFilter::Weapon:
			return IsWeaponCategory(CategoryTag);
		case ETunaSweeperStorageFilter::Ammo:
			return CategoryTag == FName(TEXT("item.category.ammo"));
		case ETunaSweeperStorageFilter::Attachment:
			return CategoryTag == FName(TEXT("item.category.attachment"));
		case ETunaSweeperStorageFilter::Consumable:
			return CategoryTag == FName(TEXT("item.category.consumable")) ||
				CategoryTag == FName(TEXT("item.category.throwable"));
		case ETunaSweeperStorageFilter::Gear:
			return IsGearCategory(CategoryTag);
		case ETunaSweeperStorageFilter::Material:
			return CategoryTag == FName(TEXT("item.category.material"));
		case ETunaSweeperStorageFilter::Blueprint:
			return CategoryTag == FName(TEXT("item.category.blueprint"));
		case ETunaSweeperStorageFilter::Other:
			return CategoryTag.IsNone() || !IsKnownStorageFilterCategory(CategoryTag);
		default:
			return false;
		}
	}

	const TCHAR* ResolveStorageFilterIconObjectPath(ETunaSweeperStorageFilter Filter)
	{
		switch (Filter)
		{
		case ETunaSweeperStorageFilter::All:
			return TEXT("/Game/UI/StorageFilters/T_UIStorageFilter_All.T_UIStorageFilter_All");
		case ETunaSweeperStorageFilter::Weapon:
			return TEXT("/Game/UI/StorageFilters/T_UIStorageFilter_Weapon.T_UIStorageFilter_Weapon");
		case ETunaSweeperStorageFilter::Ammo:
			return TEXT("/Game/UI/StorageFilters/T_UIStorageFilter_Ammo.T_UIStorageFilter_Ammo");
		case ETunaSweeperStorageFilter::Attachment:
			return TEXT("/Game/UI/StorageFilters/T_UIStorageFilter_Attachment.T_UIStorageFilter_Attachment");
		case ETunaSweeperStorageFilter::Consumable:
			return TEXT("/Game/UI/StorageFilters/T_UIStorageFilter_Consumable.T_UIStorageFilter_Consumable");
		case ETunaSweeperStorageFilter::Gear:
			return TEXT("/Game/UI/StorageFilters/T_UIStorageFilter_Gear.T_UIStorageFilter_Gear");
		case ETunaSweeperStorageFilter::Material:
			return TEXT("/Game/UI/StorageFilters/T_UIStorageFilter_Material.T_UIStorageFilter_Material");
		case ETunaSweeperStorageFilter::Blueprint:
			return TEXT("/Game/UI/StorageFilters/T_UIStorageFilter_Blueprint.T_UIStorageFilter_Blueprint");
		case ETunaSweeperStorageFilter::Other:
			return TEXT("/Game/UI/StorageFilters/T_UIStorageFilter_Other.T_UIStorageFilter_Other");
		default:
			return TEXT("");
		}
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

void UItemContainerWidget::NativeConstruct()
{
	Super::NativeConstruct();
	TunaSweeperUIFont::ApplyFontToWidgetTree(this);

	if (UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>())
	{
		TunaGameInstance->OnInventoryStateChanged.RemoveAll(this);
		TunaGameInstance->OnInventoryStateChanged.AddUObject(this, &UItemContainerWidget::PopulateContainerItems);
		TunaGameInstance->OnLanguageChanged.RemoveAll(this);
		TunaGameInstance->OnLanguageChanged.AddUObject(this, &UItemContainerWidget::PopulateContainerItems);
	}

	PopulateContainerItems();
}

void UItemContainerWidget::NativeDestruct()
{
	if (UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>())
	{
		TunaGameInstance->OnInventoryStateChanged.RemoveAll(this);
		TunaGameInstance->OnLanguageChanged.RemoveAll(this);
	}

	Super::NativeDestruct();
}

void UItemContainerWidget::RefreshHeaderControls()
{
}

void UItemContainerWidget::RebuildCompactedSlotView(
	const TArray<FTunaSweeperInventorySlot>* Slots,
	UTunaSweeperGameInstance* TunaGameInstance,
	UTunaSweeperItemDataSubsystem* ItemDataSubsystem)
{
}

bool UItemContainerWidget::UsesCompactedSlotView() const
{
	return false;
}

int32 UItemContainerWidget::GetCompactedSlotViewCount() const
{
	return 0;
}

int32 UItemContainerWidget::ResolveSourceSlotIndexFromDisplayIndex(int32 DisplaySlotIndex) const
{
	return DisplaySlotIndex;
}

int32 UItemContainerWidget::ResolveCompactedOccupiedSlotCount(int32 DefaultOccupiedSlotCount) const
{
	return DefaultOccupiedSlotCount;
}

int32 UItemContainerWidget::ResolveCompactedOccupancyDenominator(
	int32 Capacity,
	int32 TotalOccupiedSlotCount) const
{
	return Capacity;
}

bool UItemContainerWidget::TryResolveCompactedDropSlotFromCursor(
	const FVector2D& ScreenSpacePosition,
	FTunaSweeperItemSlotReference& OutSlotReference)
{
	return false;
}

void UItemContainerWidget::ApplyContainerPanelSize(int32 RowCount, float EntryHeight)
{
	if (!RootSizeBox)
	{
		return;
	}

	RootSizeBox->SetWidthOverride(ItemContainerUi::ContainerPanelWidth);
	RootSizeBox->ClearMinDesiredHeight();
	RootSizeBox->SetHeightOverride(
		ItemContainerUi::ResolveHeaderHeight(SlotSource) + RowCount * EntryHeight);
}

void UStorageContainerWidget::RefreshHeaderControls()
{
	Super::RefreshHeaderControls();
	EnsureStorageSortButton();
	EnsureStorageFilterControls();
	RefreshStorageSortButton();
	RefreshStorageFilterControls();
}

void UStorageContainerWidget::RebuildCompactedSlotView(
	const TArray<FTunaSweeperInventorySlot>* Slots,
	UTunaSweeperGameInstance* TunaGameInstance,
	UTunaSweeperItemDataSubsystem* ItemDataSubsystem)
{
	VisibleStorageSlotIndices.Reset();
	if (SlotSource != ETunaSweeperItemSlotSource::Storage ||
		ActiveStorageFilter == ETunaSweeperStorageFilter::All ||
		!Slots ||
		!TunaGameInstance)
	{
		return;
	}

	for (int32 SlotIndex = 0; SlotIndex < Slots->Num(); ++SlotIndex)
	{
		const FTunaSweeperInventorySlot& StorageSlot = (*Slots)[SlotIndex];
		if (!StorageSlot.ItemUid.IsValid())
		{
			continue;
		}

		FTunaSweeperItemInstance ItemInstance;
		if (!TunaGameInstance->TryGetItemInstance(StorageSlot.ItemUid, ItemInstance))
		{
			continue;
		}

		FTunaSweeperItemDefinition ItemDefinition;
		const bool bHasItemDefinition = ItemDataSubsystem &&
			ItemDataSubsystem->TryGetItemDefinition(ItemInstance.ItemId, ItemDefinition);
		if (ItemContainerUi::DoesItemMatchStorageFilter(
			ItemDefinition,
			bHasItemDefinition,
			ActiveStorageFilter))
		{
			VisibleStorageSlotIndices.Add(SlotIndex);
		}
	}
}

bool UStorageContainerWidget::UsesCompactedSlotView() const
{
	return SlotSource == ETunaSweeperItemSlotSource::Storage &&
		ActiveStorageFilter != ETunaSweeperStorageFilter::All;
}

int32 UStorageContainerWidget::GetCompactedSlotViewCount() const
{
	return VisibleStorageSlotIndices.Num();
}

int32 UStorageContainerWidget::ResolveSourceSlotIndexFromDisplayIndex(int32 DisplaySlotIndex) const
{
	return VisibleStorageSlotIndices.IsValidIndex(DisplaySlotIndex)
		? VisibleStorageSlotIndices[DisplaySlotIndex]
		: INDEX_NONE;
}

int32 UStorageContainerWidget::ResolveCompactedOccupiedSlotCount(int32 DefaultOccupiedSlotCount) const
{
	return VisibleStorageSlotIndices.Num();
}

int32 UStorageContainerWidget::ResolveCompactedOccupancyDenominator(
	int32 Capacity,
	int32 TotalOccupiedSlotCount) const
{
	return TotalOccupiedSlotCount;
}

bool UStorageContainerWidget::TryResolveCompactedDropSlotFromCursor(
	const FVector2D& ScreenSpacePosition,
	FTunaSweeperItemSlotReference& OutSlotReference)
{
	FTunaSweeperItemSlotReference VisibleSlotReference;
	if (!ItemContainerUi::TryResolveSlotFromTileView(
		ContainerTileView,
		SlotSource,
		VisibleStorageSlotIndices.Num(),
		ScreenSpacePosition,
		VisibleSlotReference) ||
		!VisibleStorageSlotIndices.IsValidIndex(VisibleSlotReference.SlotIndex))
	{
		return false;
	}

	OutSlotReference.Source = ETunaSweeperItemSlotSource::Storage;
	OutSlotReference.SlotIndex = VisibleStorageSlotIndices[VisibleSlotReference.SlotIndex];
	return true;
}

void UStorageContainerWidget::ApplyContainerPanelSize(int32 RowCount, float EntryHeight)
{
	if (!RootSizeBox)
	{
		return;
	}

	RootSizeBox->SetWidthOverride(ItemContainerUi::ContainerPanelWidth);
	const float CachedHeight = RootSizeBox->GetCachedGeometry().GetLocalSize().Y;
	int32 ViewportSizeX = 0;
	int32 ViewportSizeY = 0;
	if (APlayerController* OwningPlayer = GetOwningPlayer())
	{
		OwningPlayer->GetViewportSize(ViewportSizeX, ViewportSizeY);
	}
	const bool bCachedHeightLooksLikeViewportStretch =
		CachedHeight > 1.0f &&
		(ViewportSizeY <= 0 || CachedHeight <= static_cast<float>(ViewportSizeY) * 1.05f);
	if (bCachedHeightLooksLikeViewportStretch)
	{
		StorageStretchedPanelMinHeight = FMath::Max(StorageStretchedPanelMinHeight, CachedHeight);
	}

	RootSizeBox->ClearHeightOverride();
	if (StorageStretchedPanelMinHeight > 1.0f)
	{
		RootSizeBox->SetMinDesiredHeight(StorageStretchedPanelMinHeight);
	}
	else
	{
		RootSizeBox->ClearMinDesiredHeight();
	}
}

void UStorageContainerWidget::EnsureStorageSortButton()
{
	if (!WidgetTree)
	{
		return;
	}

	if (!StorageSortButton)
	{
		StorageSortButton = Cast<UButton>(WidgetTree->FindWidget(FName(TEXT("StorageSortButton"))));
	}
	if (!StorageSortButton)
	{
		StorageSortButton = WidgetTree->ConstructWidget<UButton>(
			UButton::StaticClass(),
			TEXT("StorageSortButton"));
	}

	if (!StorageSortButtonText)
	{
		StorageSortButtonText = Cast<UTextBlock>(WidgetTree->FindWidget(FName(TEXT("StorageSortButtonText"))));
	}
	if (!StorageSortButtonText)
	{
		StorageSortButtonText = WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(),
			TEXT("StorageSortButtonText"));
	}

	if (!StorageSortButton || !StorageSortButtonText)
	{
		return;
	}

	StorageSortButton->SetContent(StorageSortButtonText);
	StorageSortButton->SetClickMethod(EButtonClickMethod::DownAndUp);
	StorageSortButton->SetBackgroundColor(FLinearColor(0.46f, 0.72f, 0.86f, 0.96f));
	FButtonStyle ButtonStyle = StorageSortButton->GetStyle();
	ButtonStyle.NormalPadding = FMargin(8.0f, 3.0f);
	ButtonStyle.PressedPadding = FMargin(8.0f, 4.0f, 8.0f, 2.0f);
	StorageSortButton->SetStyle(ButtonStyle);

	StorageSortButtonText->SetColorAndOpacity(FSlateColor(FLinearColor(0.02f, 0.03f, 0.035f, 1.0f)));
	TunaSweeperUIFont::ApplyFont(StorageSortButtonText, 14, ETunaSweeperUIFontWeight::Bold);

	StorageSortButton->OnClicked.RemoveDynamic(this, &UStorageContainerWidget::HandleStorageSortButtonClicked);
	StorageSortButton->OnClicked.AddDynamic(this, &UStorageContainerWidget::HandleStorageSortButtonClicked);

	UHorizontalBox* ContainerHeaderRow = Cast<UHorizontalBox>(WidgetTree->FindWidget(FName(TEXT("ContainerHeaderRow"))));
	if (!ContainerHeaderRow)
	{
		RefreshStorageSortButton();
		return;
	}

	auto FindChildIndex = [](const UPanelWidget* Parent, const UWidget* Child) -> int32
	{
		if (!Parent || !Child)
		{
			return INDEX_NONE;
		}

		for (int32 ChildIndex = 0; ChildIndex < Parent->GetChildrenCount(); ++ChildIndex)
		{
			if (Parent->GetChildAt(ChildIndex) == Child)
			{
				return ChildIndex;
			}
		}

		return INDEX_NONE;
	};

	const int32 ButtonIndex = FindChildIndex(ContainerHeaderRow, StorageSortButton);
	const int32 OccupancyIndex = FindChildIndex(ContainerHeaderRow, ContainerOccupancyText);
	const bool bButtonShouldMove =
		ButtonIndex == INDEX_NONE ||
		(OccupancyIndex != INDEX_NONE && ButtonIndex != OccupancyIndex - 1) ||
		(OccupancyIndex == INDEX_NONE && ButtonIndex != ContainerHeaderRow->GetChildrenCount() - 1);

	if (bButtonShouldMove)
	{
		if (UPanelWidget* CurrentParent = StorageSortButton->GetParent())
		{
			CurrentParent->RemoveChild(StorageSortButton);
		}

		const int32 UpdatedOccupancyIndex = FindChildIndex(ContainerHeaderRow, ContainerOccupancyText);
		const int32 InsertIndex = UpdatedOccupancyIndex != INDEX_NONE
			? UpdatedOccupancyIndex
			: ContainerHeaderRow->GetChildrenCount();
		ContainerHeaderRow->InsertChildAt(
			FMath::Clamp(InsertIndex, 0, ContainerHeaderRow->GetChildrenCount()),
			StorageSortButton);
	}

	if (UHorizontalBoxSlot* ButtonSlot = Cast<UHorizontalBoxSlot>(StorageSortButton->Slot))
	{
		ButtonSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		ButtonSlot->SetHorizontalAlignment(HAlign_Right);
		ButtonSlot->SetVerticalAlignment(VAlign_Center);
		ButtonSlot->SetPadding(FMargin(0.0f, 0.0f, 8.0f, 0.0f));
	}

	RefreshStorageSortButton();
}

void UStorageContainerWidget::RefreshStorageSortButton()
{
	if (!StorageSortButton)
	{
		return;
	}

	const bool bShowStorageSortButton = SlotSource == ETunaSweeperItemSlotSource::Storage;
	const UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	const FText SortText = ItemContainerUi::ResolveUiText(
		TunaGameInstance,
		TEXT("ui.inventory.sort"),
		TEXT("\uC815\uB9AC"));

	StorageSortButton->SetVisibility(
		bShowStorageSortButton ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	StorageSortButton->SetIsEnabled(bShowStorageSortButton);
	StorageSortButton->SetToolTipText(SortText);

	if (StorageSortButtonText)
	{
		StorageSortButtonText->SetText(SortText);
	}
}

void UStorageContainerWidget::EnsureStorageFilterControls()
{
	if (!WidgetTree)
	{
		return;
	}

	if (!StorageFilterTabsWidget)
	{
		StorageFilterTabsWidget = Cast<UHorizontalBox>(WidgetTree->FindWidget(FName(TEXT("StorageFilterTabsRow"))));
	}

	if (!StorageFilterTabsWidget)
	{
		StorageFilterTabsWidget = WidgetTree->ConstructWidget<UHorizontalBox>(
			UHorizontalBox::StaticClass(),
			TEXT("StorageFilterTabsRow"));
	}

	if (!StorageFilterTabsWidget)
	{
		return;
	}

	auto FindChildIndex = [](const UPanelWidget* Parent, const UWidget* Child) -> int32
	{
		if (!Parent || !Child)
		{
			return INDEX_NONE;
		}

		for (int32 ChildIndex = 0; ChildIndex < Parent->GetChildrenCount(); ++ChildIndex)
		{
			if (Parent->GetChildAt(ChildIndex) == Child)
			{
				return ChildIndex;
			}
		}

		return INDEX_NONE;
	};

	auto ConfigureFilterHeaderSlot = [](UPanelSlot* PanelSlot)
	{
		if (UVerticalBoxSlot* VerticalTabsSlot = Cast<UVerticalBoxSlot>(PanelSlot))
		{
			VerticalTabsSlot->SetHorizontalAlignment(HAlign_Fill);
			VerticalTabsSlot->SetVerticalAlignment(VAlign_Center);
			VerticalTabsSlot->SetPadding(FMargin(
				0.0f,
				ItemContainerUi::StorageFilterHeaderTopPadding,
				0.0f,
				ItemContainerUi::StorageFilterHeaderBottomPadding));
		}
	};

	UPanelWidget* TargetParent = nullptr;
	int32 TargetIndex = INDEX_NONE;

	UHorizontalBox* ContainerHeaderRow = Cast<UHorizontalBox>(WidgetTree->FindWidget(FName(TEXT("ContainerHeaderRow"))));
	if (ContainerHeaderRow)
	{
		if (UPanelWidget* HeaderParent = ContainerHeaderRow->GetParent())
		{
			const int32 HeaderIndex = FindChildIndex(HeaderParent, ContainerHeaderRow);
			if (HeaderIndex != INDEX_NONE)
			{
				TargetParent = HeaderParent;
				TargetIndex = HeaderIndex + 1;
			}
		}
	}

	if (!TargetParent && ContainerTileView)
	{
		if (UPanelWidget* TileParent = ContainerTileView->GetParent())
		{
			const int32 TileIndex = FindChildIndex(TileParent, ContainerTileView);
			if (TileIndex != INDEX_NONE)
			{
				TargetParent = TileParent;
				TargetIndex = TileIndex;
			}
		}
	}

	if (TargetParent)
	{
		UPanelWidget* CurrentParent = StorageFilterTabsWidget->GetParent();
		const int32 CurrentIndex = FindChildIndex(CurrentParent, StorageFilterTabsWidget);
		if (CurrentParent != TargetParent || CurrentIndex != TargetIndex)
		{
			if (CurrentParent)
			{
				CurrentParent->RemoveChild(StorageFilterTabsWidget);
			}

			if (ContainerHeaderRow && ContainerHeaderRow->GetParent() == TargetParent)
			{
				TargetIndex = FindChildIndex(TargetParent, ContainerHeaderRow) + 1;
			}
			else if (ContainerTileView && ContainerTileView->GetParent() == TargetParent)
			{
				TargetIndex = FindChildIndex(TargetParent, ContainerTileView);
			}

			TargetIndex = FMath::Clamp(TargetIndex, 0, TargetParent->GetChildrenCount());
			ConfigureFilterHeaderSlot(TargetParent->InsertChildAt(TargetIndex, StorageFilterTabsWidget));
		}
		else
		{
			ConfigureFilterHeaderSlot(StorageFilterTabsWidget->Slot);
		}
	}
	else if (!StorageFilterTabsWidget->GetParent() && ContainerHeaderRow)
	{
		UHorizontalBoxSlot* TabsSlot = ContainerHeaderRow->AddChildToHorizontalBox(StorageFilterTabsWidget);
		if (TabsSlot)
		{
			TabsSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			TabsSlot->SetHorizontalAlignment(HAlign_Right);
			TabsSlot->SetVerticalAlignment(VAlign_Center);
			TabsSlot->SetPadding(FMargin(12.0f, 0.0f, 0.0f, 0.0f));
		}
	}

	if (ContainerTileView)
	{
		if (UVerticalBoxSlot* TileViewSlot = Cast<UVerticalBoxSlot>(ContainerTileView->Slot))
		{
			TileViewSlot->SetPadding(FMargin(0.0f, ItemContainerUi::StorageFilterTileTopPadding, 0.0f, 0.0f));
		}
	}

	if (StorageFilterButtons.Num() == 0)
	{
		AddStorageFilterButton(ETunaSweeperStorageFilter::All);
		AddStorageFilterButton(ETunaSweeperStorageFilter::Weapon);
		AddStorageFilterButton(ETunaSweeperStorageFilter::Ammo);
		AddStorageFilterButton(ETunaSweeperStorageFilter::Attachment);
		AddStorageFilterButton(ETunaSweeperStorageFilter::Consumable);
		AddStorageFilterButton(ETunaSweeperStorageFilter::Gear);
		AddStorageFilterButton(ETunaSweeperStorageFilter::Material);
		AddStorageFilterButton(ETunaSweeperStorageFilter::Blueprint);
		AddStorageFilterButton(ETunaSweeperStorageFilter::Other);
	}

	StorageFilterTabsWidget->SetVisibility(
		SlotSource == ETunaSweeperItemSlotSource::Storage ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}

void UStorageContainerWidget::RefreshStorageFilterControls()
{
	if (!StorageFilterTabsWidget)
	{
		return;
	}

	const bool bShowStorageFilters = SlotSource == ETunaSweeperItemSlotSource::Storage;
	StorageFilterTabsWidget->SetVisibility(bShowStorageFilters ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	if (!bShowStorageFilters)
	{
		return;
	}

	for (int32 ButtonIndex = 0; ButtonIndex < StorageFilterButtons.Num(); ++ButtonIndex)
	{
		UButton* Button = StorageFilterButtons[ButtonIndex];
		UImage* ButtonImage = StorageFilterButtonImages.IsValidIndex(ButtonIndex)
			? StorageFilterButtonImages[ButtonIndex]
			: nullptr;
		const ETunaSweeperStorageFilter ButtonFilter = StorageFilterButtonValues.IsValidIndex(ButtonIndex)
			? StorageFilterButtonValues[ButtonIndex]
			: ETunaSweeperStorageFilter::All;
		const bool bActive = ButtonFilter == ActiveStorageFilter;

		if (Button)
		{
			Button->SetIsEnabled(true);
			Button->SetRenderOpacity(bActive ? 1.0f : 0.72f);
			Button->SetBackgroundColor(bActive
				? FLinearColor(0.78f, 0.58f, 0.18f, 0.88f)
				: FLinearColor(0.18f, 0.20f, 0.22f, 0.58f));
			Button->SetToolTipText(ResolveStorageFilterText(ButtonFilter));
		}
		if (ButtonImage)
		{
			ButtonImage->SetColorAndOpacity(bActive
				? FLinearColor(1.0f, 0.92f, 0.54f, 1.0f)
				: FLinearColor(0.86f, 0.91f, 0.94f, 1.0f));
		}
	}
}

void UStorageContainerWidget::SetStorageFilter(ETunaSweeperStorageFilter NewFilter)
{
	if (ActiveStorageFilter == NewFilter)
	{
		return;
	}

	ActiveStorageFilter = NewFilter;
	if (ContainerTileView)
	{
		ContainerTileView->SetScrollOffset(0.0f);
	}
	PopulateContainerItems();
}

void UStorageContainerWidget::AddStorageFilterButton(ETunaSweeperStorageFilter Filter)
{
	if (!WidgetTree || !StorageFilterTabsWidget)
	{
		return;
	}

	const TCHAR* FilterName = TEXT("Unknown");
	switch (Filter)
	{
	case ETunaSweeperStorageFilter::All:
		FilterName = TEXT("All");
		break;
	case ETunaSweeperStorageFilter::Weapon:
		FilterName = TEXT("Weapon");
		break;
	case ETunaSweeperStorageFilter::Ammo:
		FilterName = TEXT("Ammo");
		break;
	case ETunaSweeperStorageFilter::Attachment:
		FilterName = TEXT("Attachment");
		break;
	case ETunaSweeperStorageFilter::Consumable:
		FilterName = TEXT("Consumable");
		break;
	case ETunaSweeperStorageFilter::Gear:
		FilterName = TEXT("Gear");
		break;
	case ETunaSweeperStorageFilter::Material:
		FilterName = TEXT("Material");
		break;
	case ETunaSweeperStorageFilter::Blueprint:
		FilterName = TEXT("Blueprint");
		break;
	case ETunaSweeperStorageFilter::Other:
		FilterName = TEXT("Other");
		break;
	default:
		break;
	}
	UButton* Button = WidgetTree->ConstructWidget<UButton>(
		UButton::StaticClass(),
		*FString::Printf(TEXT("StorageFilterButton_%s"), FilterName));
	USizeBox* IconBox = WidgetTree->ConstructWidget<USizeBox>(
		USizeBox::StaticClass(),
		*FString::Printf(TEXT("StorageFilterIconBox_%s"), FilterName));
	UImage* ButtonImage = WidgetTree->ConstructWidget<UImage>(
		UImage::StaticClass(),
		*FString::Printf(TEXT("StorageFilterButtonImage_%s"), FilterName));
	if (!Button || !IconBox || !ButtonImage)
	{
		return;
	}

	IconBox->SetWidthOverride(ItemContainerUi::StorageFilterIconSize);
	IconBox->SetHeightOverride(ItemContainerUi::StorageFilterIconSize);
	if (UTexture2D* IconTexture = LoadObject<UTexture2D>(
		nullptr,
		ItemContainerUi::ResolveStorageFilterIconObjectPath(Filter)))
	{
		ButtonImage->SetBrushFromTexture(IconTexture, true);
	}
	IconBox->SetContent(ButtonImage);
	Button->SetContent(IconBox);
	Button->SetToolTipText(ResolveStorageFilterText(Filter));
	FButtonStyle ButtonStyle = Button->GetStyle();
	ButtonStyle.NormalPadding = FMargin(3.0f);
	ButtonStyle.PressedPadding = FMargin(3.0f);
	Button->SetStyle(ButtonStyle);

	switch (Filter)
	{
	case ETunaSweeperStorageFilter::All:
		Button->OnClicked.AddDynamic(this, &UStorageContainerWidget::HandleStorageFilterAllClicked);
		break;
	case ETunaSweeperStorageFilter::Weapon:
		Button->OnClicked.AddDynamic(this, &UStorageContainerWidget::HandleStorageFilterWeaponClicked);
		break;
	case ETunaSweeperStorageFilter::Ammo:
		Button->OnClicked.AddDynamic(this, &UStorageContainerWidget::HandleStorageFilterAmmoClicked);
		break;
	case ETunaSweeperStorageFilter::Attachment:
		Button->OnClicked.AddDynamic(this, &UStorageContainerWidget::HandleStorageFilterAttachmentClicked);
		break;
	case ETunaSweeperStorageFilter::Consumable:
		Button->OnClicked.AddDynamic(this, &UStorageContainerWidget::HandleStorageFilterConsumableClicked);
		break;
	case ETunaSweeperStorageFilter::Gear:
		Button->OnClicked.AddDynamic(this, &UStorageContainerWidget::HandleStorageFilterGearClicked);
		break;
	case ETunaSweeperStorageFilter::Material:
		Button->OnClicked.AddDynamic(this, &UStorageContainerWidget::HandleStorageFilterMaterialClicked);
		break;
	case ETunaSweeperStorageFilter::Blueprint:
		Button->OnClicked.AddDynamic(this, &UStorageContainerWidget::HandleStorageFilterBlueprintClicked);
		break;
	case ETunaSweeperStorageFilter::Other:
		Button->OnClicked.AddDynamic(this, &UStorageContainerWidget::HandleStorageFilterOtherClicked);
		break;
	default:
		break;
	}

	UHorizontalBoxSlot* ButtonSlot = StorageFilterTabsWidget->AddChildToHorizontalBox(Button);
	if (ButtonSlot)
	{
		ButtonSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		ButtonSlot->SetVerticalAlignment(VAlign_Center);
		ButtonSlot->SetPadding(FMargin(0.0f, 0.0f, 4.0f, 0.0f));
	}

	StorageFilterButtons.Add(Button);
	StorageFilterButtonImages.Add(ButtonImage);
	StorageFilterButtonValues.Add(Filter);
}

FText UStorageContainerWidget::ResolveStorageFilterText(ETunaSweeperStorageFilter Filter) const
{
	const UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	switch (Filter)
	{
	case ETunaSweeperStorageFilter::All:
		return ItemContainerUi::ResolveUiText(TunaGameInstance, TEXT("ui.storage.filter.all"), TEXT("\uC804\uCCB4"));
	case ETunaSweeperStorageFilter::Weapon:
		return ItemContainerUi::ResolveUiText(TunaGameInstance, TEXT("ui.storage.filter.weapon"), TEXT("\uBB34\uAE30"));
	case ETunaSweeperStorageFilter::Ammo:
		return ItemContainerUi::ResolveUiText(TunaGameInstance, TEXT("ui.storage.filter.ammo"), TEXT("\uD0C4\uC57D"));
	case ETunaSweeperStorageFilter::Attachment:
		return ItemContainerUi::ResolveUiText(TunaGameInstance, TEXT("ui.storage.filter.attachment"), TEXT("\uBD80\uCC29"));
	case ETunaSweeperStorageFilter::Consumable:
		return ItemContainerUi::ResolveUiText(TunaGameInstance, TEXT("ui.storage.filter.consumable"), TEXT("\uC18C\uBAA8"));
	case ETunaSweeperStorageFilter::Gear:
		return ItemContainerUi::ResolveUiText(TunaGameInstance, TEXT("ui.storage.filter.gear"), TEXT("\uC7A5\uBE44"));
	case ETunaSweeperStorageFilter::Material:
		return ItemContainerUi::ResolveUiText(TunaGameInstance, TEXT("ui.storage.filter.material"), TEXT("\uC7AC\uB8CC"));
	case ETunaSweeperStorageFilter::Blueprint:
		return ItemContainerUi::ResolveUiText(TunaGameInstance, TEXT("ui.storage.filter.blueprint"), TEXT("\uC124\uACC4\uB3C4"));
	case ETunaSweeperStorageFilter::Other:
		return ItemContainerUi::ResolveUiText(TunaGameInstance, TEXT("ui.storage.filter.other"), TEXT("\uAE30\uD0C0"));
	default:
		return FText::GetEmpty();
	}
}

void UStorageContainerWidget::HandleStorageFilterAllClicked()
{
	SetStorageFilter(ETunaSweeperStorageFilter::All);
}

void UStorageContainerWidget::HandleStorageFilterWeaponClicked()
{
	SetStorageFilter(ETunaSweeperStorageFilter::Weapon);
}

void UStorageContainerWidget::HandleStorageFilterAmmoClicked()
{
	SetStorageFilter(ETunaSweeperStorageFilter::Ammo);
}

void UStorageContainerWidget::HandleStorageFilterAttachmentClicked()
{
	SetStorageFilter(ETunaSweeperStorageFilter::Attachment);
}

void UStorageContainerWidget::HandleStorageFilterConsumableClicked()
{
	SetStorageFilter(ETunaSweeperStorageFilter::Consumable);
}

void UStorageContainerWidget::HandleStorageFilterGearClicked()
{
	SetStorageFilter(ETunaSweeperStorageFilter::Gear);
}

void UStorageContainerWidget::HandleStorageFilterMaterialClicked()
{
	SetStorageFilter(ETunaSweeperStorageFilter::Material);
}

void UStorageContainerWidget::HandleStorageFilterBlueprintClicked()
{
	SetStorageFilter(ETunaSweeperStorageFilter::Blueprint);
}

void UStorageContainerWidget::HandleStorageFilterOtherClicked()
{
	SetStorageFilter(ETunaSweeperStorageFilter::Other);
}

void UStorageContainerWidget::HandleStorageSortButtonClicked()
{
	if (UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>())
	{
		TunaGameInstance->CompactStorageSlots();
	}
}

void UShopContainerWidget::EnsureShopCurrencyDisplayWidget()
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

bool UItemContainerWidget::TryResolveDropSlotFromCursor(
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

	if (UsesCompactedSlotView())
	{
		return TryResolveCompactedDropSlotFromCursor(ScreenSpacePosition, OutSlotReference);
	}

	return ItemContainerUi::TryResolveSlotFromTileView(
		ContainerTileView,
		SlotSource,
		Capacity,
		ScreenSpacePosition,
		OutSlotReference);
}

void UItemContainerWidget::SetContainerInstanceInternal(
	const FTunaSweeperLootContainerInstance& InContainerInstance)
{
	SlotSource = ETunaSweeperItemSlotSource::LootContainer;
	ActiveShopId = INDEX_NONE;
	ActiveWorkbenchId = INDEX_NONE;
	ActiveWorkbenchMode = ETunaSweeperWorkbenchMode::Craft;
	ContainerInstance = InContainerInstance;
	PopulateContainerItems();
}

void UItemContainerWidget::SetStorageViewInternal()
{
	SlotSource = ETunaSweeperItemSlotSource::Storage;
	ActiveShopId = INDEX_NONE;
	ActiveWorkbenchId = INDEX_NONE;
	ActiveWorkbenchMode = ETunaSweeperWorkbenchMode::Craft;
	ContainerInstance = FTunaSweeperLootContainerInstance();
	PopulateContainerItems();
}

void UItemContainerWidget::SetShopViewInternal(int32 ShopId)
{
	SlotSource = ETunaSweeperItemSlotSource::Shop;
	ActiveShopId = ShopId;
	ActiveWorkbenchId = INDEX_NONE;
	ActiveWorkbenchMode = ETunaSweeperWorkbenchMode::Craft;
	ContainerInstance = FTunaSweeperLootContainerInstance();
	PopulateContainerItems();
}

void UItemContainerWidget::SetWorkbenchViewInternal(
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

bool UItemContainerWidget::NativeOnDrop(
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
			ItemContainerUi::TryOpenStackSplitPopupForDrop(
				GetOwningPlayer(),
				TunaGameInstance,
				ItemDragOperation,
				CursorSlotReference,
				InDragDropEvent.GetScreenSpacePosition())) ||
			ItemContainerUi::TryMoveFromDropSlot(TunaGameInstance, ItemDragOperation, CursorSlotReference)))
	{
		return true;
	}

	if (InDragDropEvent.GetModifierKeys().IsControlDown() &&
		ItemDragOperation->bHasHoveredSlotReference &&
		ItemContainerUi::TryOpenStackSplitPopupForDrop(
			GetOwningPlayer(),
			TunaGameInstance,
			ItemDragOperation,
			ItemDragOperation->HoveredSlotReference,
			InDragDropEvent.GetScreenSpacePosition()))
	{
		return true;
	}

	if (ItemContainerUi::TryMoveFromHoveredDropSlot(TunaGameInstance, ItemDragOperation))
	{
		return true;
	}

	return Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);
}

void UItemContainerWidget::PopulateContainerItems()
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
	UTunaSweeperItemDataSubsystem* ItemDataSubsystem = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UTunaSweeperItemDataSubsystem>()
		: nullptr;

	RebuildCompactedSlotView(Slots, TunaGameInstance, ItemDataSubsystem);
	const bool bCompactedSlotView = UsesCompactedSlotView();
	int32 Capacity = Slots ? Slots->Num() : FMath::Max(0, ContainerInstance.Capacity);
	int32 OccupiedSlotCount = Slots
		? ItemContainerUi::CountOccupiedSlots(*Slots)
		: ItemContainerUi::CountOccupiedStacks(ContainerInstance.Items);
	const int32 TotalOccupiedSlotCountBeforeCompaction = Slots
		? OccupiedSlotCount
		: 0;
	if (bCompactedSlotView)
	{
		Capacity = GetCompactedSlotViewCount();
		OccupiedSlotCount = ResolveCompactedOccupiedSlotCount(OccupiedSlotCount);
	}
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
	const int32 UiSlotCount = ItemContainerUi::RoundUpToUiSlotCount(Capacity);
	const int32 DisplaySlotCount = bCompactedSlotView ? GetCompactedSlotViewCount() : UiSlotCount;
	const int32 RowCount = FMath::Max(
		1,
		FMath::DivideAndRoundUp(UiSlotCount, ItemContainerUi::ContainerTileColumnCount));
	const float EntryHeight = ItemContainerUi::ResolveEntryHeight(SlotSource);
	ApplyContainerPanelSize(RowCount, EntryHeight);

	if (ContainerTitleText)
	{
		FText DisplayName;
		if (SlotSource == ETunaSweeperItemSlotSource::Shop)
		{
			UTunaSweeperItemDataSubsystem* TitleItemDataSubsystem = GetGameInstance()
				? GetGameInstance()->GetSubsystem<UTunaSweeperItemDataSubsystem>()
				: nullptr;
			DisplayName = ItemContainerUi::GetShopDisplayName(
				TunaGameInstance,
				TitleItemDataSubsystem,
				ActiveShopId);
		}
		else if (SlotSource == ETunaSweeperItemSlotSource::WorkbenchRecipe ||
			SlotSource == ETunaSweeperItemSlotSource::WorkbenchDismantleItem ||
			SlotSource == ETunaSweeperItemSlotSource::WorkbenchBlueprintItem)
		{
			DisplayName = ItemContainerUi::GetWorkbenchDisplayName(TunaGameInstance, ActiveWorkbenchMode);
		}
		else if (SlotSource == ETunaSweeperItemSlotSource::Storage)
		{
			DisplayName = ItemContainerUi::GetStorageDisplayName(TunaGameInstance);
		}
		else
		{
			DisplayName = TunaGameInstance && TunaGameInstance->HasActiveLootContainer()
				? TunaGameInstance->GetActiveLootContainerDisplayName()
				: ContainerInstance.DisplayName;
		}
		ContainerTitleText->SetText(DisplayName.IsEmpty()
			? ItemContainerUi::ResolveUiText(
				TunaGameInstance,
				TEXT("ui.common.container_fallback"),
				TEXT("Container"))
			: DisplayName);
	}
	if (ContainerOccupancyText)
	{
		ContainerOccupancyText->SetText(FText::FromString(FString::Printf(
			TEXT("(%d/%d)"),
			OccupiedSlotCount,
			bCompactedSlotView
				? ResolveCompactedOccupancyDenominator(Capacity, TotalOccupiedSlotCountBeforeCompaction)
				: Capacity)));
	}

	const ETunaSweeperItemTextLanguage Language = TunaGameInstance
		? TunaGameInstance->GetCurrentTextLanguage()
		: ETunaSweeperItemTextLanguage::English;

	TileObjects.Reset();
	ContainerTileView->ClearListItems();
	ContainerTileView->SetEntryWidth(ItemContainerUi::ContainerTileWidth);
	ContainerTileView->SetEntryHeight(EntryHeight);

	for (int32 SlotIndex = 0; SlotIndex < DisplaySlotCount; ++SlotIndex)
	{
		const int32 SourceSlotIndex = bCompactedSlotView
			? ResolveSourceSlotIndexFromDisplayIndex(SlotIndex)
			: SlotIndex;
		FTunaSweeperItemInstance ItemInstance;
		if (Slots && Slots->IsValidIndex(SourceSlotIndex) && (*Slots)[SourceSlotIndex].ItemUid.IsValid() && TunaGameInstance)
		{
			TunaGameInstance->TryGetItemInstance((*Slots)[SourceSlotIndex].ItemUid, ItemInstance);
		}

		UTunaSweeperItemStackTileItemObject* TileObject = NewObject<UTunaSweeperItemStackTileItemObject>(this);
		if (!TileObject)
		{
			continue;
		}

		FTunaSweeperItemStackTileData TileData;
		if (SlotSource == ETunaSweeperItemSlotSource::Shop && ShopItems.IsValidIndex(SlotIndex))
		{
			TileData = ItemContainerUi::BuildShopTileData(
				TunaGameInstance,
				ItemDataSubsystem,
				ShopItems[SlotIndex],
				Language);
		}
		else if (SlotSource == ETunaSweeperItemSlotSource::WorkbenchRecipe && WorkbenchRecipes.IsValidIndex(SlotIndex))
		{
			TileData = ItemContainerUi::BuildWorkbenchTileData(
				TunaGameInstance,
				ItemDataSubsystem,
				WorkbenchRecipes[SlotIndex],
				Language);
		}
		else if (SlotSource == ETunaSweeperItemSlotSource::WorkbenchDismantleItem && DismantleCandidates.IsValidIndex(SlotIndex))
		{
			TileData = ItemContainerUi::BuildWorkbenchDismantleTileData(
				TunaGameInstance,
				ItemDataSubsystem,
				DismantleCandidates[SlotIndex],
				Language);
		}
		else if (SlotSource == ETunaSweeperItemSlotSource::WorkbenchBlueprintItem && BlueprintItems.IsValidIndex(SlotIndex))
		{
			TileData = ItemContainerUi::BuildWorkbenchBlueprintTileData(
				TunaGameInstance,
				ItemDataSubsystem,
				BlueprintItems[SlotIndex],
				Language);
		}
		else
		{
			TileData = ItemContainerUi::BuildTileData(
				TunaGameInstance,
				ItemDataSubsystem,
				ItemInstance,
				SlotSource,
				SourceSlotIndex,
				Language);
		}

		TileObject->Initialize(TileData);
		TileObjects.Add(TileObject);
		ContainerTileView->AddItem(TileObject);
	}
}

void ULootContainerWidget::SetContainerInstance(const FTunaSweeperLootContainerInstance& InContainerInstance)
{
	SetContainerInstanceInternal(InContainerInstance);
}

void ULootContainerWidget::SetStorageView()
{
	SetStorageViewInternal();
}

void ULootContainerWidget::SetShopView(int32 ShopId)
{
	SetShopViewInternal(ShopId);
}

void ULootContainerWidget::SetWorkbenchView(int32 WorkbenchId, ETunaSweeperWorkbenchMode WorkbenchMode)
{
	SetWorkbenchViewInternal(WorkbenchId, WorkbenchMode);
}

void UStorageContainerWidget::SetStorageView()
{
	ActiveStorageFilter = ETunaSweeperStorageFilter::All;
	VisibleStorageSlotIndices.Reset();
	SetStorageViewInternal();
}

void UShopContainerWidget::SetShopView(int32 ShopId)
{
	SetShopViewInternal(ShopId);
}

void UShopContainerWidget::NativeConstruct()
{
	Super::NativeConstruct();
	EnsureShopCurrencyDisplayWidget();

	if (ShopRefreshStockButton)
	{
		ShopRefreshStockButton->OnClicked.RemoveDynamic(
			this,
			&UShopContainerWidget::HandleShopRefreshStockButtonClicked);
		ShopRefreshStockButton->OnClicked.AddDynamic(
			this,
			&UShopContainerWidget::HandleShopRefreshStockButtonClicked);
	}
}

void UShopContainerWidget::NativeDestruct()
{
	if (ShopRefreshStockButton)
	{
		ShopRefreshStockButton->OnClicked.RemoveDynamic(
			this,
			&UShopContainerWidget::HandleShopRefreshStockButtonClicked);
	}

	Super::NativeDestruct();
}

void UShopContainerWidget::RefreshHeaderControls()
{
	Super::RefreshHeaderControls();
	EnsureShopCurrencyDisplayWidget();

	if (ShopRefreshStockButton)
	{
		ShopRefreshStockButton->SetVisibility(ESlateVisibility::Visible);
		ShopRefreshStockButton->SetIsEnabled(true);
	}
	if (ShopRefreshStockButtonText)
	{
		ShopRefreshStockButtonText->SetText(ItemContainerUi::ResolveUiText(
			GetGameInstance<UTunaSweeperGameInstance>(),
			TEXT("ui.shop.debug_refresh_stock"),
			TEXT("\uAC31\uC2E0")));
	}
	if (ShopCurrencyDisplayWidget)
	{
		ShopCurrencyDisplayWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UShopContainerWidget::ApplyContainerPanelSize(int32 RowCount, float EntryHeight)
{
	if (!RootSizeBox)
	{
		return;
	}

	const float TargetHeight =
		ItemContainerUi::ResolveHeaderHeight(SlotSource) +
		ItemContainerUi::ShopVisibleRowCount * EntryHeight;

	RootSizeBox->SetWidthOverride(ItemContainerUi::ContainerPanelWidth);
	RootSizeBox->ClearMinDesiredHeight();
	RootSizeBox->SetHeightOverride(FMath::Max(1.0f, TargetHeight));
}

void UShopContainerWidget::HandleShopRefreshStockButtonClicked()
{
	UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	if (TunaGameInstance && TunaGameInstance->DebugRestockActiveShop(true))
	{
		PopulateContainerItems();
	}
}
