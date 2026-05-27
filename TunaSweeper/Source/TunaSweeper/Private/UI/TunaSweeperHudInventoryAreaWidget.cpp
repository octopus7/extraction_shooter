#include "UI/TunaSweeperHudInventoryAreaWidget.h"

#include "Blueprint/DragDropOperation.h"
#include "Components/Button.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/TileView.h"
#include "Components/Widget.h"
#include "Engine/Texture2D.h"
#include "Game/TunaSweeperGameInstance.h"
#include "GameFramework/PlayerController.h"
#include "Subsystem/TunaSweeperItemDataSubsystem.h"
#include "UI/TunaSweeperItemDragDropOperation.h"
#include "UI/TunaSweeperItemStackSplitPopupWidget.h"
#include "UI/TunaSweeperItemStackTileItemObject.h"
#include "UI/TunaSweeperUIFont.h"
#include "UI/TunaSweeperUiText.h"

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

	FText MakeRoundedFloatText(float Value)
	{
		FNumberFormattingOptions NumberFormat;
		NumberFormat.MinimumFractionalDigits = 0;
		NumberFormat.MaximumFractionalDigits = 1;
		return FText::AsNumber(Value, &NumberFormat);
	}

	using TunaSweeperUiText::ResolveUiText;

	FText GetEquipmentSlotDisplayName(int32 SlotIndex, const UTunaSweeperGameInstance* TunaGameInstance)
	{
		struct FEquipmentSlotText
		{
			const TCHAR* StringKey;
			const TCHAR* Fallback;
		};

		static const FEquipmentSlotText SlotNames[] = {
			{ TEXT("ui.inventory.slot.gun1"), TEXT("\uCD1D\uAE30 1") },
			{ TEXT("ui.inventory.slot.gun2"), TEXT("\uCD1D\uAE30 2") },
			{ TEXT("ui.inventory.slot.melee"), TEXT("\uADFC\uC811") },
			{ TEXT("ui.inventory.slot.head"), TEXT("\uBA38\uB9AC") },
			{ TEXT("ui.inventory.slot.body"), TEXT("\uC2E0\uCCB4") },
			{ TEXT("ui.inventory.slot.face"), TEXT("\uC5BC\uAD74") },
			{ TEXT("ui.inventory.slot.ear"), TEXT("\uC774\uC5B4\uD3F0") },
			{ TEXT("ui.inventory.slot.bag"), TEXT("\uAC00\uBC29") }
		};

		if (SlotIndex >= 0 && SlotIndex < UE_ARRAY_COUNT(SlotNames))
		{
			return ResolveUiText(TunaGameInstance, SlotNames[SlotIndex].StringKey, SlotNames[SlotIndex].Fallback);
		}

		return FText::Format(
			ResolveUiText(TunaGameInstance, TEXT("ui.inventory.slot.generic"), TEXT("Slot {0}")),
			FText::AsNumber(SlotIndex + 1));
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
		TileData.bShowEmptySlotLabel = Source == ETunaSweeperItemSlotSource::Equipment;

		if (TileData.bIsEmpty && TileData.bShowEmptySlotLabel)
		{
			TileData.DisplayName = GetEquipmentSlotDisplayName(SourceIndex, TunaGameInstance);
		}

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
					TileData.DisplayName = FText::FromString(FString::Printf(TEXT("Item %d"), ItemInstance.ItemId));
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

		const ETunaSweeperItemTextLanguage Language = TunaGameInstance
			? TunaGameInstance->GetCurrentTextLanguage()
			: ETunaSweeperItemTextLanguage::English;

		for (int32 Index = 0; Index < Slots.Num(); ++Index)
		{
			FTunaSweeperItemInstance ItemInstance;
			if (TunaGameInstance && Slots[Index].ItemUid.IsValid())
			{
				TunaGameInstance->TryGetItemInstance(Slots[Index].ItemUid, ItemInstance);
			}

			UTunaSweeperItemStackTileItemObject* TileObject = CreateTileObject(
				Outer,
				BuildTileData(TunaGameInstance, ItemDataSubsystem, ItemInstance, Source, Index, Language));
			if (TileObject)
			{
				TileObjects.Add(TileObject);
				TileView->AddItem(TileObject);
			}
		}
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

	bool TryResolveSlotFromTileView(
		const UTileView* TileView,
		ETunaSweeperItemSlotSource Source,
		int32 ColumnCount,
		int32 SlotCount,
		const FVector2D& ScreenSpacePosition,
		FTunaSweeperItemSlotReference& OutSlotReference)
	{
		if (!TileView || ColumnCount <= 0 || SlotCount <= 0)
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
		if (ColumnIndex < 0 || ColumnIndex >= ColumnCount || RowIndex < 0)
		{
			return false;
		}

		const int32 FirstVisibleItemIndex = FMath::Max(0, FMath::FloorToInt(TileView->GetScrollOffset()));
		const int32 SlotIndex = FirstVisibleItemIndex + RowIndex * ColumnCount + ColumnIndex;
		if (SlotIndex < 0 || SlotIndex >= SlotCount)
		{
			return false;
		}

		OutSlotReference.Source = Source;
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
}

bool UTunaSweeperHudInventoryAreaWidget::TryResolveDropSlotFromCursor(
	const FVector2D& ScreenSpacePosition,
	FTunaSweeperItemSlotReference& OutSlotReference)
{
	UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	if (!TunaGameInstance)
	{
		return false;
	}

	if (TunaSweeperInventoryArea::TryResolveSlotFromTileView(
		EquipmentReserveTileView,
		ETunaSweeperItemSlotSource::Equipment,
		TunaSweeperInventoryArea::EquipmentReserveColumnCount,
		TunaGameInstance->GetEquipmentSlots().Num(),
		ScreenSpacePosition,
		OutSlotReference))
	{
		return true;
	}

	if (TunaSweeperInventoryArea::TryResolveSlotFromTileView(
		AuxiliaryBagTileView,
		ETunaSweeperItemSlotSource::AuxiliaryBag,
		1,
		TunaGameInstance->GetAuxiliaryBagSlots().Num(),
		ScreenSpacePosition,
		OutSlotReference))
	{
		return true;
	}

	return TunaSweeperInventoryArea::TryResolveSlotFromTileView(
		InventoryTileView,
		ETunaSweeperItemSlotSource::Inventory,
		TunaSweeperInventoryArea::InventoryTileColumnCount,
		TunaGameInstance->GetInventorySlots().Num(),
		ScreenSpacePosition,
		OutSlotReference);
}

void UTunaSweeperHudInventoryAreaWidget::NativeConstruct()
{
	Super::NativeConstruct();
	TunaSweeperUIFont::ApplyFontToWidgetTree(this);

	if (SortInventoryButton)
	{
		SortInventoryButton->OnClicked.RemoveDynamic(this, &UTunaSweeperHudInventoryAreaWidget::HandleSortInventoryClicked);
		SortInventoryButton->OnClicked.AddDynamic(this, &UTunaSweeperHudInventoryAreaWidget::HandleSortInventoryClicked);
	}

	if (UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>())
	{
		TunaGameInstance->OnInventoryStateChanged.RemoveAll(this);
		TunaGameInstance->OnInventoryStateChanged.AddUObject(this, &UTunaSweeperHudInventoryAreaWidget::RefreshInventoryItems);
		TunaGameInstance->OnLanguageChanged.RemoveAll(this);
		TunaGameInstance->OnLanguageChanged.AddUObject(this, &UTunaSweeperHudInventoryAreaWidget::RefreshInventoryItems);
		TunaGameInstance->OnLanguageChanged.AddUObject(this, &UTunaSweeperHudInventoryAreaWidget::ApplyHudState);
	}

	RefreshInventoryItems();
	ApplyHudState();
}

void UTunaSweeperHudInventoryAreaWidget::NativeDestruct()
{
	if (UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>())
	{
		TunaGameInstance->OnInventoryStateChanged.RemoveAll(this);
		TunaGameInstance->OnLanguageChanged.RemoveAll(this);
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

	if (InventoryWeightPanel)
	{
		InventoryWeightPanel->SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void UTunaSweeperHudInventoryAreaWidget::SetHudState(const FTunaSweeperPlayerHudState& InHudState)
{
	PreviewHudState = InHudState;
	PreviewHudState.NormalizeWeightLimits();
	ApplyHudState();
}

void UTunaSweeperHudInventoryAreaWidget::ApplyHudState()
{
	PreviewHudState.NormalizeWeightLimits();
	const UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();

	if (SortInventoryButtonText)
	{
		SortInventoryButtonText->SetText(TunaSweeperInventoryArea::ResolveUiText(
			TunaGameInstance,
			TEXT("ui.inventory.sort"),
			TEXT("\uC815\uB9AC")));
	}

	if (InventoryWeightLabelText)
	{
		InventoryWeightLabelText->SetText(TunaSweeperInventoryArea::ResolveUiText(
			TunaGameInstance,
			TEXT("ui.inventory.weight_label"),
			TEXT("\uC18C\uC9C0 \uC911\uB7C9")));
	}

	if (InventoryWeightWarningText)
	{
		InventoryWeightWarningText->SetText(TunaSweeperInventoryArea::ResolveUiText(
			TunaGameInstance,
			TEXT("ui.inventory.overweight"),
			TEXT("\uACFC\uC911\uB7C9")));
	}

	if (InventoryWeightText)
	{
		InventoryWeightText->SetText(FText::Format(
			TunaSweeperInventoryArea::ResolveUiText(
				TunaGameInstance,
				TEXT("ui.inventory.weight_pattern"),
				TEXT("{0}/{1}kg")),
			TunaSweeperInventoryArea::MakeRoundedFloatText(PreviewHudState.CurrentCarryWeight),
			TunaSweeperInventoryArea::MakeRoundedFloatText(PreviewHudState.MaxCarryWeight)));
	}

	if (InventoryWeightGauge)
	{
		const float GaugePercent = PreviewHudState.MovementBlockedWeight > 0.0f
			? PreviewHudState.CurrentCarryWeight / PreviewHudState.MovementBlockedWeight
			: 0.0f;
		InventoryWeightGauge->SetPercent(FMath::Clamp(GaugePercent, 0.0f, 1.0f));
		InventoryWeightGauge->SetFillColorAndOpacity(
			PreviewHudState.IsCarryWeightMovementBlocked()
				? FLinearColor(0.92f, 0.16f, 0.10f, 1.0f)
				: PreviewHudState.IsCarryWeightOverLimit()
					? FLinearColor(0.96f, 0.74f, 0.18f, 1.0f)
					: FLinearColor(0.60f, 0.84f, 0.36f, 1.0f));
	}

	if (InventoryWeightWarningIcon)
	{
		InventoryWeightWarningIcon->SetVisibility(
			PreviewHudState.IsCarryWeightOverLimit()
				? ESlateVisibility::HitTestInvisible
				: ESlateVisibility::Hidden);
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
	UTunaSweeperItemDragDropOperation* ItemDragOperation = Cast<UTunaSweeperItemDragDropOperation>(InOperation);
	if (!ItemDragOperation || ItemDragOperation->TileData.bIsEmpty)
	{
		return Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);
	}

	UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	FTunaSweeperItemSlotReference CursorSlotReference;
	if (TryResolveDropSlotFromCursor(InDragDropEvent.GetScreenSpacePosition(), CursorSlotReference) &&
		((InDragDropEvent.GetModifierKeys().IsControlDown() &&
			TunaSweeperInventoryArea::TryOpenStackSplitPopupForDrop(
				GetOwningPlayer(),
				TunaGameInstance,
				ItemDragOperation,
				CursorSlotReference,
				InDragDropEvent.GetScreenSpacePosition())) ||
			TunaSweeperInventoryArea::TryMoveFromDropSlot(TunaGameInstance, ItemDragOperation, CursorSlotReference)))
	{
		return true;
	}

	if (InDragDropEvent.GetModifierKeys().IsControlDown() &&
		ItemDragOperation->bHasHoveredSlotReference &&
		TunaSweeperInventoryArea::TryOpenStackSplitPopupForDrop(
			GetOwningPlayer(),
			TunaGameInstance,
			ItemDragOperation,
			ItemDragOperation->HoveredSlotReference,
			InDragDropEvent.GetScreenSpacePosition()))
	{
		return true;
	}

	if (TunaSweeperInventoryArea::TryMoveFromHoveredDropSlot(TunaGameInstance, ItemDragOperation))
	{
		return true;
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
