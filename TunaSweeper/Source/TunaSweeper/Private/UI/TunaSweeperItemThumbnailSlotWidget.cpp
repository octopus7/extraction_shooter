#include "UI/TunaSweeperItemThumbnailSlotWidget.h"

#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/PanelWidget.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Engine/Texture2D.h"
#include "GameFramework/PlayerController.h"
#include "Game/TunaSweeperGameInstance.h"
#include "InputCoreTypes.h"
#include "Input/Reply.h"
#include "UI/TunaSweeperCurrencyDisplayWidget.h"
#include "UI/TunaSweeperItemDragDropOperation.h"
#include "UI/TunaSweeperItemHoverPromptWidget.h"
#include "UI/TunaSweeperItemRaritySlotAccentWidget.h"
#include "UI/TunaSweeperItemStackSplitPopupWidget.h"
#include "UI/TunaSweeperItemStackTileItemObject.h"
#include "UI/TunaSweeperUIFont.h"
#include "Blueprint/WidgetLayoutLibrary.h"

void UTunaSweeperItemThumbnailSlotWidget::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	IUserObjectListEntry::NativeOnListItemObjectSet(ListItemObject);

	const UTunaSweeperItemStackTileItemObject* TileItemObject = Cast<UTunaSweeperItemStackTileItemObject>(ListItemObject);
	SetTileData(TileItemObject ? TileItemObject->GetTileData() : FTunaSweeperItemStackTileData());
}

void UTunaSweeperItemThumbnailSlotWidget::SetTileData(const FTunaSweeperItemStackTileData& InTileData)
{
	CachedTileData = InTileData;
	TunaSweeperUIFont::ApplyFontToWidgetTree(this);
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
		if (CachedTileData.Source == ETunaSweeperItemSlotSource::Shop ||
			CachedTileData.Source == ETunaSweeperItemSlotSource::WorkbenchRecipe)
		{
			return FReply::Handled();
		}

		bSuppressNextMouseButtonUpSelection = false;
		return UWidgetBlueprintLibrary::DetectDragIfPressed(InMouseEvent, this, EKeys::LeftMouseButton).NativeReply;
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UTunaSweeperItemThumbnailSlotWidget::NativeOnMouseButtonUp(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent)
{
	if (!CachedTileData.bIsEmpty && InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		if (CachedTileData.Source == ETunaSweeperItemSlotSource::Shop ||
			CachedTileData.Source == ETunaSweeperItemSlotSource::WorkbenchRecipe ||
			CachedTileData.Source == ETunaSweeperItemSlotSource::WorkbenchDismantleItem ||
			CachedTileData.Source == ETunaSweeperItemSlotSource::WorkbenchBlueprintItem)
		{
			return FReply::Handled();
		}

		if (bSuppressNextMouseButtonUpSelection)
		{
			bSuppressNextMouseButtonUpSelection = false;
			return FReply::Handled();
		}

		if (UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
			TunaGameInstance && CachedTileData.Source != ETunaSweeperItemSlotSource::SelectedWeaponAttachment)
		{
			TunaGameInstance->SelectItemSlot(GetCachedSlotReference());
		}

		return FReply::Handled();
	}

	bSuppressNextMouseButtonUpSelection = false;
	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

void UTunaSweeperItemThumbnailSlotWidget::NativeOnDragDetected(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent,
	UDragDropOperation*& OutOperation)
{
	bSuppressNextMouseButtonUpSelection = true;

	if (CachedTileData.bIsEmpty)
	{
		OutOperation = nullptr;
		return;
	}

	if (CachedTileData.Source == ETunaSweeperItemSlotSource::Shop ||
		CachedTileData.Source == ETunaSweeperItemSlotSource::WorkbenchRecipe)
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

	UTunaSweeperItemDragDropOperation* ItemDragOperation = Cast<UTunaSweeperItemDragDropOperation>(InOperation);
	UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	if (!ItemDragOperation || !TunaGameInstance)
	{
		return Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);
	}
	if (CachedTileData.Source == ETunaSweeperItemSlotSource::WorkbenchDismantleItem ||
		CachedTileData.Source == ETunaSweeperItemSlotSource::WorkbenchBlueprintItem)
	{
		return Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);
	}
	if (ItemDragOperation->TileData.Source == ETunaSweeperItemSlotSource::WorkbenchDismantleItem ||
		ItemDragOperation->TileData.Source == ETunaSweeperItemSlotSource::WorkbenchBlueprintItem)
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
	if (InDragDropEvent.GetModifierKeys().IsControlDown() &&
		UTunaSweeperItemStackSplitPopupWidget::TryOpenStackSplitPopup(
			GetOwningPlayer(),
			TunaGameInstance,
			SourceSlot,
			TargetSlot,
			InDragDropEvent.GetScreenSpacePosition()))
	{
		ItemDragOperation->bHasHoveredSlotReference = false;
		ItemDragOperation->HoveredSlotReference = FTunaSweeperItemSlotReference();
		return true;
	}

	const bool bMoved = TunaGameInstance->MoveItemBetweenSlots(SourceSlot, TargetSlot);
	ItemDragOperation->bHasHoveredSlotReference = false;

	return bMoved || Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);
}

void UTunaSweeperItemThumbnailSlotWidget::ApplyTileData()
{
	if (SlotBackground)
	{
		ApplyDropHighlight(false);
	}

	EnsureRaritySlotAccentWidget();
	ApplyRaritySlotAccent();
	EnsureAttachmentSlotIndicatorWidget();
	EnsureItemPriceCoinWidget();

	const bool bIsEquipmentSlot = CachedTileData.Source == ETunaSweeperItemSlotSource::Equipment;
	if (EquipmentSlotNameText)
	{
		EquipmentSlotNameText->SetText(bIsEquipmentSlot ? CachedTileData.EquipmentSlotDisplayName : FText::GetEmpty());
		EquipmentSlotNameText->SetVisibility(
			bIsEquipmentSlot && !CachedTileData.EquipmentSlotDisplayName.IsEmpty()
				? ESlateVisibility::HitTestInvisible
				: ESlateVisibility::Collapsed);
		EquipmentSlotNameText->SetColorAndOpacity(FSlateColor(FLinearColor(0.72f, 0.80f, 0.86f, 1.0f)));
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

	FText QuantityText = FText::GetEmpty();
	if (!CachedTileData.bIsEmpty)
	{
		if (CachedTileData.Source == ETunaSweeperItemSlotSource::Shop)
		{
			QuantityText = FText::AsNumber(FMath::Max(0, CachedTileData.ShopStockQuantity));
		}
		else if (CachedTileData.Source == ETunaSweeperItemSlotSource::WorkbenchRecipe)
		{
			QuantityText = CachedTileData.bCanCraftWorkbenchRecipe
				? FText::Format(FText::FromString(TEXT("{0}\nOK")), FText::AsNumber(CachedTileData.ItemStack.Quantity))
				: FText::Format(
					FText::FromString(TEXT("{0}\n-{1}")),
					FText::AsNumber(CachedTileData.ItemStack.Quantity),
					FText::AsNumber(FMath::Max(0, CachedTileData.WorkbenchMissingIngredientCount)));
		}
		else if (CachedTileData.Source == ETunaSweeperItemSlotSource::WorkbenchDismantleItem)
		{
			QuantityText = FText::Format(FText::FromString(TEXT("{0}\nDIS")), FText::AsNumber(CachedTileData.ItemStack.Quantity));
		}
		else if (CachedTileData.Source == ETunaSweeperItemSlotSource::WorkbenchBlueprintItem)
		{
			QuantityText = CachedTileData.bCanRegisterWorkbenchBlueprint
				? FText::Format(FText::FromString(TEXT("{0}\nREG")), FText::AsNumber(CachedTileData.ItemStack.Quantity))
				: FText::Format(FText::FromString(TEXT("{0}\nLOCK")), FText::AsNumber(CachedTileData.ItemStack.Quantity));
		}
		else
		{
			QuantityText = FText::AsNumber(CachedTileData.ItemStack.Quantity);
		}
	}

	if (ItemQuantityText)
	{
		ItemQuantityText->SetText(QuantityText);
		ItemQuantityText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		ItemQuantityText->SetJustification(ETextJustify::Right);
		ItemQuantityText->SetLineHeightPercentage(0.72f);
		if (!ItemQuantityPlate)
		{
			if (UOverlaySlot* QuantityOverlaySlot = Cast<UOverlaySlot>(ItemQuantityText->Slot))
			{
				QuantityOverlaySlot->SetHorizontalAlignment(HAlign_Right);
				QuantityOverlaySlot->SetVerticalAlignment(VAlign_Bottom);
				QuantityOverlaySlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 14.0f));
			}
		}
	}
	if (ItemQuantityPlate)
	{
		ItemQuantityPlate->SetBrushColor(FLinearColor(0.36f, 0.38f, 0.40f, 0.50f));
		ItemQuantityPlate->SetVisibility(QuantityText.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}

	if (AttachmentSlotIndicatorText != nullptr)
	{
		const FText AttachmentIndicatorText = BuildAttachmentSlotIndicatorText();
		AttachmentSlotIndicatorText->SetText(AttachmentIndicatorText);
		AttachmentSlotIndicatorText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		AttachmentSlotIndicatorText->SetVisibility(
			AttachmentIndicatorText.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}

	if (ItemNameText)
	{
		const FText NameText = bIsEquipmentSlot
			? (!CachedTileData.bIsEmpty ? CachedTileData.DisplayName : FText::GetEmpty())
			: ((!CachedTileData.bIsEmpty || CachedTileData.bShowEmptySlotLabel)
				? CachedTileData.DisplayName
				: FText::GetEmpty());
		ItemNameText->SetText(NameText);
		ItemNameText->SetJustification(ETextJustify::Right);
		if (ItemNamePlate)
		{
			ItemNamePlate->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.50f));
			ItemNamePlate->SetVisibility(NameText.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
			if (UOverlaySlot* NameOverlaySlot = Cast<UOverlaySlot>(ItemNamePlate->Slot))
			{
				NameOverlaySlot->SetHorizontalAlignment(HAlign_Right);
				NameOverlaySlot->SetVerticalAlignment(VAlign_Bottom);
			}
		}
	}

	const FText PriceText = (!CachedTileData.bIsEmpty && CachedTileData.Source == ETunaSweeperItemSlotSource::Shop)
		? FText::AsNumber(FMath::Max(0, CachedTileData.ShopPrice))
		: FText::GetEmpty();
	if (ItemPriceText)
	{
		ItemPriceText->SetText(PriceText);
		ItemPriceText->SetColorAndOpacity(FSlateColor(FLinearColor(0.92f, 0.96f, 0.92f, 1.0f)));
		ItemPriceText->SetJustification(ETextJustify::Right);
	}
	if (ItemPricePlate)
	{
		ItemPricePlate->SetBrushColor(FLinearColor::Transparent);
		ItemPricePlate->SetVisibility(PriceText.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
	if (ItemPriceCoinImage)
	{
		ItemPriceCoinImage->SetBrushFromTexture(UTunaSweeperCurrencyDisplayWidget::LoadCurrencyCoinIconTexture(), true);
		ItemPriceCoinImage->SetBrushTintColor(FSlateColor(FLinearColor::White));
		ItemPriceCoinImage->SetVisibility(PriceText.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
		ItemPriceCoinImage->SetOpacity(PriceText.IsEmpty() ? 0.0f : 1.0f);
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
	if (RaritySlotAccentWidget)
	{
		RaritySlotAccentWidget->SetRenderOpacity(bCanAcceptDrop ? 0.22f : 1.0f);
	}
}

void UTunaSweeperItemThumbnailSlotWidget::EnsureRaritySlotAccentWidget()
{
	if (RaritySlotAccentWidget || !WidgetTree)
	{
		return;
	}

	RaritySlotAccentWidget = Cast<UTunaSweeperItemRaritySlotAccentWidget>(
		WidgetTree->FindWidget(FName(TEXT("RaritySlotAccentWidget"))));
	if (RaritySlotAccentWidget)
	{
		return;
	}

	UOverlay* SlotOverlay = Cast<UOverlay>(WidgetTree->FindWidget(FName(TEXT("SlotOverlay"))));
	if (!SlotOverlay && ItemIconImage)
	{
		SlotOverlay = Cast<UOverlay>(ItemIconImage->GetParent());
	}
	if (!SlotOverlay)
	{
		return;
	}

	RaritySlotAccentWidget = WidgetTree->ConstructWidget<UTunaSweeperItemRaritySlotAccentWidget>(
		UTunaSweeperItemRaritySlotAccentWidget::StaticClass(),
		FName(TEXT("RaritySlotAccentWidget")));
	if (!RaritySlotAccentWidget)
	{
		return;
	}

	int32 InsertIndex = 0;
	if (SlotBackground && SlotBackground->GetParent() == SlotOverlay)
	{
		InsertIndex = SlotOverlay->GetChildIndex(SlotBackground) + 1;
	}
	else if (ItemIconImage && ItemIconImage->GetParent() == SlotOverlay)
	{
		InsertIndex = SlotOverlay->GetChildIndex(ItemIconImage);
	}

	UOverlaySlot* AccentSlot = Cast<UOverlaySlot>(SlotOverlay->InsertChildAt(InsertIndex, RaritySlotAccentWidget));
	if (!AccentSlot)
	{
		AccentSlot = SlotOverlay->AddChildToOverlay(RaritySlotAccentWidget);
	}
	if (AccentSlot)
	{
		AccentSlot->SetHorizontalAlignment(HAlign_Fill);
		AccentSlot->SetVerticalAlignment(VAlign_Fill);
		AccentSlot->SetPadding(FMargin(0.0f));
	}
}

void UTunaSweeperItemThumbnailSlotWidget::ApplyRaritySlotAccent()
{
	if (!RaritySlotAccentWidget)
	{
		return;
	}

	const ETunaSweeperItemGrade ItemGrade = CachedTileData.bHasItemDefinition
		? CachedTileData.ItemDefinition.ItemGrade
		: ETunaSweeperItemGrade::Common;
	RaritySlotAccentWidget->SetItemGrade(
		ItemGrade,
		!CachedTileData.bIsEmpty && CachedTileData.bHasItemDefinition);
}

void UTunaSweeperItemThumbnailSlotWidget::EnsureAttachmentSlotIndicatorWidget()
{
	if (AttachmentSlotIndicatorText == nullptr && WidgetTree)
	{
		AttachmentSlotIndicatorText = Cast<UTextBlock>(WidgetTree->FindWidget(FName(TEXT("AttachmentSlotIndicatorText"))));
	}

	if (AttachmentSlotIndicatorText == nullptr && WidgetTree)
	{
		UOverlay* SlotOverlay = Cast<UOverlay>(WidgetTree->FindWidget(FName(TEXT("SlotOverlay"))));
		if (!SlotOverlay && ItemIconImage)
		{
			SlotOverlay = Cast<UOverlay>(ItemIconImage->GetParent());
		}

		if (SlotOverlay)
		{
			AttachmentSlotIndicatorText = WidgetTree->ConstructWidget<UTextBlock>(
				UTextBlock::StaticClass(),
				TEXT("AttachmentSlotIndicatorText"));
			if (AttachmentSlotIndicatorText)
			{
				SlotOverlay->AddChildToOverlay(AttachmentSlotIndicatorText);
			}
		}
	}

	if (!AttachmentSlotIndicatorText)
	{
		return;
	}

	AttachmentSlotIndicatorText->SetAutoWrapText(false);
	AttachmentSlotIndicatorText->SetJustification(ETextJustify::Left);
	TunaSweeperUIFont::ApplyFont(AttachmentSlotIndicatorText, 13.0f);

	if (UOverlaySlot* IndicatorSlot = Cast<UOverlaySlot>(AttachmentSlotIndicatorText->Slot))
	{
		IndicatorSlot->SetHorizontalAlignment(HAlign_Left);
		IndicatorSlot->SetVerticalAlignment(VAlign_Top);
		IndicatorSlot->SetPadding(FMargin(0.0f));
	}
}

void UTunaSweeperItemThumbnailSlotWidget::EnsureItemPriceCoinWidget()
{
	if (ItemPriceCoinImage == nullptr && WidgetTree)
	{
		ItemPriceCoinImage = Cast<UImage>(WidgetTree->FindWidget(FName(TEXT("ItemPriceCoinImage"))));
	}

	if ((ItemPriceCoinImage || !WidgetTree || !ItemPricePlate || !ItemPriceText))
	{
		return;
	}

	UHorizontalBox* PriceRow = WidgetTree->ConstructWidget<UHorizontalBox>(
		UHorizontalBox::StaticClass(),
		TEXT("ItemPriceRow"));
	USizeBox* CoinSizeBox = WidgetTree->ConstructWidget<USizeBox>(
		USizeBox::StaticClass(),
		TEXT("ItemPriceCoinSizeBox"));
	ItemPriceCoinImage = WidgetTree->ConstructWidget<UImage>(
		UImage::StaticClass(),
		TEXT("ItemPriceCoinImage"));
	if (!PriceRow || !CoinSizeBox || !ItemPriceCoinImage)
	{
		return;
	}

	ItemPriceText->RemoveFromParent();
	ItemPricePlate->SetContent(PriceRow);

	CoinSizeBox->SetWidthOverride(13.0f);
	CoinSizeBox->SetHeightOverride(13.0f);
	CoinSizeBox->SetContent(ItemPriceCoinImage);
	UHorizontalBoxSlot* CoinSlot = PriceRow->AddChildToHorizontalBox(CoinSizeBox);
	if (CoinSlot)
	{
		CoinSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		CoinSlot->SetVerticalAlignment(VAlign_Center);
	}

	UHorizontalBoxSlot* TextSlot = PriceRow->AddChildToHorizontalBox(ItemPriceText);
	if (TextSlot)
	{
		TextSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		TextSlot->SetVerticalAlignment(VAlign_Center);
		TextSlot->SetPadding(FMargin(3.0f, 0.0f, 0.0f, 0.0f));
	}
}

FText UTunaSweeperItemThumbnailSlotWidget::BuildAttachmentSlotIndicatorText() const
{
	if (CachedTileData.bIsEmpty ||
		!CachedTileData.bHasItemDefinition ||
		CachedTileData.ItemDefinition.AttachmentSlotTags.Num() <= 0)
	{
		return FText::GetEmpty();
	}

	FString IndicatorText;
	for (const FName& AttachmentSlotTag : CachedTileData.ItemDefinition.AttachmentSlotTags)
	{
		if (AttachmentSlotTag.IsNone())
		{
			continue;
		}

		const FGuid* AttachmentUid = CachedTileData.ItemInstance.AttachmentSlots.Find(AttachmentSlotTag);
		IndicatorText += AttachmentUid && AttachmentUid->IsValid()
			? TEXT("\u25CF")
			: TEXT("\u25CB");
	}

	return IndicatorText.IsEmpty()
		? FText::GetEmpty()
		: FText::FromString(IndicatorText);
}

bool UTunaSweeperItemThumbnailSlotWidget::CanAcceptDragOperation(UDragDropOperation* InOperation) const
{
	const UTunaSweeperItemDragDropOperation* ItemDragOperation = Cast<UTunaSweeperItemDragDropOperation>(InOperation);
	UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	if (!ItemDragOperation || !TunaGameInstance || ItemDragOperation->TileData.bIsEmpty)
	{
		return false;
	}
	if (CachedTileData.Source == ETunaSweeperItemSlotSource::WorkbenchDismantleItem ||
		CachedTileData.Source == ETunaSweeperItemSlotSource::WorkbenchBlueprintItem)
	{
		return false;
	}
	if (ItemDragOperation->TileData.Source == ETunaSweeperItemSlotSource::WorkbenchDismantleItem ||
		ItemDragOperation->TileData.Source == ETunaSweeperItemSlotSource::WorkbenchBlueprintItem)
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
