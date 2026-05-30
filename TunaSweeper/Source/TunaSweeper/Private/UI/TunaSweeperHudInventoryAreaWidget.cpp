#include "UI/TunaSweeperHudInventoryAreaWidget.h"

#include "Blueprint/DragDropOperation.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/TileView.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/Widget.h"
#include "Engine/Texture2D.h"
#include "Game/TunaSweeperGameInstance.h"
#include "GameFramework/PlayerController.h"
#include "Subsystem/TunaSweeperItemDataSubsystem.h"
#include "UI/TunaSweeperItemDragDropOperation.h"
#include "UI/TunaSweeperCurrencyDisplayWidget.h"
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
	constexpr float EquipmentReserveEntryWidth = 112.0f;
	constexpr float EquipmentReserveEntryHeight = 124.0f;
	constexpr float EquipmentReserveWidth = EquipmentReserveColumnCount * EquipmentReserveEntryWidth;
	constexpr float EquipmentReserveHeight = 2.0f * EquipmentReserveEntryHeight;
	constexpr float AuxiliaryBagTileWidth = 96.0f;
	constexpr float AuxiliaryBagTileHeight = 96.0f;
	constexpr float CurrencyExteriorOffset = 26.0f;
	constexpr float CurrencyExteriorRightInset = 8.0f;
	constexpr float WeightThresholdMarkerFontSize = 13.0f;

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
		ETunaSweeperItemTextLanguage Language,
		bool bSortLocked)
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
		TileData.bSortLocked = bSortLocked;

		if (Source == ETunaSweeperItemSlotSource::Equipment)
		{
			TileData.EquipmentSlotDisplayName = GetEquipmentSlotDisplayName(SourceIndex, TunaGameInstance);
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
				BuildTileData(TunaGameInstance, ItemDataSubsystem, ItemInstance, Source, Index, Language, Slots[Index].bSortLocked));
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
	EnsureCurrencyDisplayWidget();
	EnsureWeightThresholdMarkerWidgets();
	if (InventoryWeightPanel)
	{
		InventoryWeightPanel->SetVisibility(ESlateVisibility::Collapsed);
	}

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

	if (EquipmentReserveSizeBox)
	{
		EquipmentReserveSizeBox->SetWidthOverride(TunaSweeperInventoryArea::EquipmentReserveWidth);
		EquipmentReserveSizeBox->SetHeightOverride(TunaSweeperInventoryArea::EquipmentReserveHeight);
		if (UVerticalBoxSlot* ReserveSlot = Cast<UVerticalBoxSlot>(EquipmentReserveSizeBox->Slot))
		{
			ReserveSlot->SetHorizontalAlignment(HAlign_Left);
		}
	}

	RefreshInventoryItems();
	if (CurrencyDisplayWidget)
	{
		CurrencyDisplayWidget->RefreshCurrencyBalance();
	}
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
		InventoryWeightPanel->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (CurrencyDisplayWidget)
	{
		CurrencyDisplayWidget->SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void UTunaSweeperHudInventoryAreaWidget::SetHudState(const FTunaSweeperPlayerHudState& InHudState)
{
	PreviewHudState = InHudState;
	PreviewHudState.NormalizeWeightLimits();
	ApplyHudState();
}

void UTunaSweeperHudInventoryAreaWidget::EnsureCurrencyDisplayWidget()
{
	if (!WidgetTree)
	{
		return;
	}

	if (!CurrencyDisplayWidget)
	{
		CurrencyDisplayWidget = Cast<UTunaSweeperCurrencyDisplayWidget>(
			WidgetTree->FindWidget(FName(TEXT("CurrencyDisplayWidget"))));
	}

	if (!CurrencyDisplayWidget)
	{
		CurrencyDisplayWidget = WidgetTree->ConstructWidget<UTunaSweeperCurrencyDisplayWidget>(
			UTunaSweeperCurrencyDisplayWidget::StaticClass(),
			TEXT("CurrencyDisplayWidgetRuntime"));
	}

	if (CurrencyDisplayWidget)
	{
		CurrencyDisplayWidget->EnsureCurrencyContent();
		CurrencyDisplayWidget->RefreshCurrencyBalance();
	}

	AttachCurrencyDisplayAboveInventoryPanel();
}

void UTunaSweeperHudInventoryAreaWidget::AttachCurrencyDisplayAboveInventoryPanel()
{
	if (!CurrencyDisplayWidget || !InventoryPanel || !WidgetTree)
	{
		return;
	}

	USizeBox* MainInventorySizeBox = Cast<USizeBox>(WidgetTree->FindWidget(FName(TEXT("MainInventorySizeBox"))));
	if (!MainInventorySizeBox)
	{
		return;
	}

	UOverlay* InventoryPanelOverlay = Cast<UOverlay>(WidgetTree->FindWidget(FName(TEXT("InventoryPanelOverlay"))));
	if (!InventoryPanelOverlay)
	{
		InventoryPanelOverlay = WidgetTree->ConstructWidget<UOverlay>(
			UOverlay::StaticClass(),
			TEXT("InventoryPanelOverlay"));
	}

	UHorizontalBox* InventoryCurrencyExteriorRow = Cast<UHorizontalBox>(
		WidgetTree->FindWidget(FName(TEXT("InventoryCurrencyExteriorRow"))));
	if (!InventoryCurrencyExteriorRow)
	{
		InventoryCurrencyExteriorRow = WidgetTree->ConstructWidget<UHorizontalBox>(
			UHorizontalBox::StaticClass(),
			TEXT("InventoryCurrencyExteriorRow"));
	}

	if (!InventoryPanelOverlay || !InventoryCurrencyExteriorRow)
	{
		return;
	}

	if (MainInventorySizeBox->GetContent() != InventoryPanelOverlay)
	{
		InventoryPanel->RemoveFromParent();
		MainInventorySizeBox->SetContent(InventoryPanelOverlay);
	}

	InventoryPanelOverlay->ClearChildren();
	InventoryPanel->RemoveFromParent();
	InventoryCurrencyExteriorRow->RemoveFromParent();
	CurrencyDisplayWidget->RemoveFromParent();

	UOverlaySlot* PanelSlot = InventoryPanelOverlay->AddChildToOverlay(InventoryPanel);
	if (PanelSlot)
	{
		PanelSlot->SetHorizontalAlignment(HAlign_Fill);
		PanelSlot->SetVerticalAlignment(VAlign_Top);
	}

	UOverlaySlot* CurrencyRowSlot = InventoryPanelOverlay->AddChildToOverlay(InventoryCurrencyExteriorRow);
	if (CurrencyRowSlot)
	{
		CurrencyRowSlot->SetHorizontalAlignment(HAlign_Right);
		CurrencyRowSlot->SetVerticalAlignment(VAlign_Top);
		CurrencyRowSlot->SetPadding(FMargin(0.0f, 0.0f, TunaSweeperInventoryArea::CurrencyExteriorRightInset, 0.0f));
	}
	InventoryCurrencyExteriorRow->SetRenderTranslation(
		FVector2D(0.0f, -TunaSweeperInventoryArea::CurrencyExteriorOffset));

	InventoryCurrencyExteriorRow->ClearChildren();
	UHorizontalBoxSlot* CurrencySlot = InventoryCurrencyExteriorRow->AddChildToHorizontalBox(CurrencyDisplayWidget);
	if (CurrencySlot)
	{
		CurrencySlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		CurrencySlot->SetHorizontalAlignment(HAlign_Right);
		CurrencySlot->SetVerticalAlignment(VAlign_Center);
	}

	CurrencyDisplayWidget->SetRenderTranslation(FVector2D::ZeroVector);
	CurrencyDisplayWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
	CurrencyDisplayWidget->RefreshCurrencyBalance();
}

void UTunaSweeperHudInventoryAreaWidget::EnsureWeightThresholdMarkerWidgets()
{
	if (!WidgetTree)
	{
		return;
	}

	if (!InventoryWeightGaugeBox)
	{
		InventoryWeightGaugeBox = Cast<USizeBox>(WidgetTree->FindWidget(FName(TEXT("InventoryWeightGaugeBox"))));
	}
	if (!InventoryWeightGaugeOverlay)
	{
		InventoryWeightGaugeOverlay = Cast<UOverlay>(WidgetTree->FindWidget(FName(TEXT("InventoryWeightGaugeOverlay"))));
	}
	if (!InventoryWeightMarkerCanvas)
	{
		InventoryWeightMarkerCanvas = Cast<UCanvasPanel>(WidgetTree->FindWidget(FName(TEXT("InventoryWeightMarkerCanvas"))));
	}
	if (!InventoryWeightOverweightMarker)
	{
		InventoryWeightOverweightMarker = Cast<UTextBlock>(WidgetTree->FindWidget(FName(TEXT("InventoryWeightOverweightMarker"))));
	}

	if (!InventoryWeightGauge || !InventoryWeightGaugeBox)
	{
		return;
	}

	if (!InventoryWeightGaugeOverlay)
	{
		InventoryWeightGaugeOverlay = WidgetTree->ConstructWidget<UOverlay>(
			UOverlay::StaticClass(),
			TEXT("InventoryWeightGaugeOverlayRuntime"));
	}
	if (!InventoryWeightMarkerCanvas)
	{
		InventoryWeightMarkerCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(
			UCanvasPanel::StaticClass(),
			TEXT("InventoryWeightMarkerCanvasRuntime"));
	}
	if (!InventoryWeightOverweightMarker)
	{
		InventoryWeightOverweightMarker = WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(),
			TEXT("InventoryWeightOverweightMarkerRuntime"));
	}
	if (!InventoryWeightGaugeOverlay || !InventoryWeightMarkerCanvas || !InventoryWeightOverweightMarker)
	{
		return;
	}

	if (InventoryWeightGaugeBox->GetContent() != InventoryWeightGaugeOverlay)
	{
		InventoryWeightGaugeOverlay->RemoveFromParent();
		InventoryWeightGauge->RemoveFromParent();
		InventoryWeightGaugeBox->SetContent(InventoryWeightGaugeOverlay);
	}

	if (InventoryWeightGauge->GetParent() != InventoryWeightGaugeOverlay)
	{
		InventoryWeightGauge->RemoveFromParent();
		if (UOverlaySlot* GaugeSlot = InventoryWeightGaugeOverlay->AddChildToOverlay(InventoryWeightGauge))
		{
			GaugeSlot->SetHorizontalAlignment(HAlign_Fill);
			GaugeSlot->SetVerticalAlignment(VAlign_Fill);
		}
	}

	if (InventoryWeightMarkerCanvas->GetParent() != InventoryWeightGaugeOverlay)
	{
		InventoryWeightMarkerCanvas->RemoveFromParent();
		if (UOverlaySlot* MarkerCanvasSlot = InventoryWeightGaugeOverlay->AddChildToOverlay(InventoryWeightMarkerCanvas))
		{
			MarkerCanvasSlot->SetHorizontalAlignment(HAlign_Fill);
			MarkerCanvasSlot->SetVerticalAlignment(VAlign_Fill);
		}
	}

	if (InventoryWeightOverweightMarker->GetParent() != InventoryWeightMarkerCanvas)
	{
		InventoryWeightOverweightMarker->RemoveFromParent();
		InventoryWeightMarkerCanvas->AddChild(InventoryWeightOverweightMarker);
	}

	InventoryWeightOverweightMarker->SetText(FText::FromString(TEXT("\u25B2")));
	InventoryWeightOverweightMarker->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.90f, 0.30f, 1.0f)));
	InventoryWeightOverweightMarker->SetJustification(ETextJustify::Center);
	InventoryWeightOverweightMarker->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.85f));
	InventoryWeightOverweightMarker->SetShadowOffset(FVector2D(1.0f, 1.0f));
	TunaSweeperUIFont::ApplyFont(
		InventoryWeightOverweightMarker,
		TunaSweeperInventoryArea::WeightThresholdMarkerFontSize,
		ETunaSweeperUIFontWeight::Bold);

	RefreshWeightThresholdMarker();
}

void UTunaSweeperHudInventoryAreaWidget::RefreshWeightThresholdMarker()
{
	if (!InventoryWeightOverweightMarker)
	{
		return;
	}

	const bool bHasThreshold =
		PreviewHudState.MovementBlockedWeight > 0.0f &&
		PreviewHudState.OverweightCarryWeight > 0.0f;
	if (!bHasThreshold)
	{
		InventoryWeightOverweightMarker->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	const float MarkerPercent = FMath::Clamp(
		PreviewHudState.OverweightCarryWeight / PreviewHudState.MovementBlockedWeight,
		0.0f,
		1.0f);
	InventoryWeightOverweightMarker->SetVisibility(ESlateVisibility::HitTestInvisible);
	if (UCanvasPanelSlot* MarkerSlot = Cast<UCanvasPanelSlot>(InventoryWeightOverweightMarker->Slot))
	{
		MarkerSlot->SetAnchors(FAnchors(MarkerPercent, 1.0f, MarkerPercent, 1.0f));
		MarkerSlot->SetAlignment(FVector2D(0.5f, 1.0f));
		MarkerSlot->SetPosition(FVector2D::ZeroVector);
		MarkerSlot->SetAutoSize(true);
	}
}

void UTunaSweeperHudInventoryAreaWidget::ApplyHudState()
{
	PreviewHudState.NormalizeWeightLimits();
	EnsureWeightThresholdMarkerWidgets();
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
	RefreshWeightThresholdMarker();

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
		TunaSweeperInventoryArea::EquipmentReserveEntryHeight,
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
