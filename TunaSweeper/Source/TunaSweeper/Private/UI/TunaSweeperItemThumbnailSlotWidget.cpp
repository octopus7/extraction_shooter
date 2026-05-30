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
#include "Rendering/DrawElements.h"
#include "UI/TunaSweeperCurrencyDisplayWidget.h"
#include "UI/TunaSweeperItemDragDropOperation.h"
#include "UI/TunaSweeperItemHoverBorderEffectWidget.h"
#include "UI/TunaSweeperItemHoverPromptWidget.h"
#include "UI/TunaSweeperItemRaritySlotAccentWidget.h"
#include "UI/TunaSweeperItemStackSplitPopupWidget.h"
#include "UI/TunaSweeperItemStackTileItemObject.h"
#include "UI/TunaSweeperUIFont.h"
#include "Blueprint/WidgetLayoutLibrary.h"

namespace TunaSweeperItemThumbnailSlotHover
{
	enum class EBorderFlowRoute : uint8
	{
		LowerLeft,
		UpperRight
	};

	FVector2D GetBorderRoutePoint(
		const FVector2D& Min,
		const FVector2D& Max,
		EBorderFlowRoute Route,
		float Alpha)
	{
		const float ClampedAlpha = FMath::Clamp(Alpha, 0.0f, 1.0f);
		const float Width = FMath::Max(1.0f, Max.X - Min.X);
		const float Height = FMath::Max(1.0f, Max.Y - Min.Y);

		if (Route == EBorderFlowRoute::LowerLeft)
		{
			const float Split = Width / (Width + Height);
			if (ClampedAlpha <= Split)
			{
				const float EdgeAlpha = Split > KINDA_SMALL_NUMBER ? ClampedAlpha / Split : 0.0f;
				return FMath::Lerp(FVector2D(Max.X, Max.Y), FVector2D(Min.X, Max.Y), EdgeAlpha);
			}

			const float EdgeAlpha = (ClampedAlpha - Split) / FMath::Max(KINDA_SMALL_NUMBER, 1.0f - Split);
			return FMath::Lerp(FVector2D(Min.X, Max.Y), FVector2D(Min.X, Min.Y), EdgeAlpha);
		}

		const float Split = Height / (Width + Height);
		if (ClampedAlpha <= Split)
		{
			const float EdgeAlpha = Split > KINDA_SMALL_NUMBER ? ClampedAlpha / Split : 0.0f;
			return FMath::Lerp(FVector2D(Max.X, Max.Y), FVector2D(Max.X, Min.Y), EdgeAlpha);
		}

		const float EdgeAlpha = (ClampedAlpha - Split) / FMath::Max(KINDA_SMALL_NUMBER, 1.0f - Split);
		return FMath::Lerp(FVector2D(Max.X, Min.Y), FVector2D(Min.X, Min.Y), EdgeAlpha);
	}

	void BuildBorderRouteSegment(
		const FVector2D& Min,
		const FVector2D& Max,
		EBorderFlowRoute Route,
		float StartAlpha,
		float EndAlpha,
		TArray<FVector2D>& OutPoints)
	{
		const float ClampedStart = FMath::Clamp(StartAlpha, 0.0f, 1.0f);
		const float ClampedEnd = FMath::Clamp(EndAlpha, 0.0f, 1.0f);
		OutPoints.Reset();

		if (ClampedEnd <= ClampedStart)
		{
			return;
		}

		const int32 SegmentCount = FMath::Clamp(FMath::CeilToInt((ClampedEnd - ClampedStart) * 18.0f), 2, 8);
		OutPoints.Reserve(SegmentCount + 1);
		for (int32 SegmentIndex = 0; SegmentIndex <= SegmentCount; ++SegmentIndex)
		{
			const float Alpha = static_cast<float>(SegmentIndex) / static_cast<float>(SegmentCount);
			OutPoints.Add(GetBorderRoutePoint(Min, Max, Route, FMath::Lerp(ClampedStart, ClampedEnd, Alpha)));
		}
	}

	void DrawLineStrip(
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FGeometry& AllottedGeometry,
		const TArray<FVector2D>& Points,
		const FLinearColor& Color,
		float Thickness)
	{
		if (Points.Num() < 2 || Color.A <= 0.0f || Thickness <= 0.0f)
		{
			return;
		}

		FSlateDrawElement::MakeLines(
			OutDrawElements,
			LayerId,
			AllottedGeometry.ToPaintGeometry(),
			Points,
			ESlateDrawEffect::None,
			Color,
			true,
			FMath::Max(1.0f, Thickness));
	}

	float GetPacketEnvelope(float HeadAlpha)
	{
		const float StartFade = FMath::Clamp(HeadAlpha / 0.10f, 0.0f, 1.0f);
		const float EndFade = 1.0f - FMath::Clamp((HeadAlpha - 0.94f) / 0.06f, 0.0f, 1.0f) * 0.28f;
		return FMath::Clamp(StartFade * EndFade, 0.0f, 1.0f);
	}

	float GetCornerPulse(float HeadAlpha)
	{
		const float PulseAlpha = FMath::Clamp((HeadAlpha - 0.78f) / 0.22f, 0.0f, 1.0f);
		return FMath::Sin(PulseAlpha * PI);
	}
}

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
	if (!CanShowHoverBorderEffect())
	{
		SetHoverBorderEffectActive(false);
	}

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
	SetHoverBorderEffectActive(false);
	HideHoverPrompt();
	ClearHoveredItemSlot();
	Super::NativeDestruct();
}

void UTunaSweeperItemThumbnailSlotWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!bHoverBorderEffectActive && HoverBorderEffectOpacity <= 0.0f)
	{
		return;
	}

	HoverBorderAnimationSeconds += FMath::Max(0.0f, InDeltaTime);
	const float TargetOpacity = bHoverBorderEffectActive ? 1.0f : 0.0f;
	const float InterpSpeed = bHoverBorderEffectActive ? 16.0f : 12.0f;
	HoverBorderEffectOpacity = FMath::FInterpTo(
		HoverBorderEffectOpacity,
		TargetOpacity,
		FMath::Max(0.0f, InDeltaTime),
		InterpSpeed);
	if (!bHoverBorderEffectActive && HoverBorderEffectOpacity < 0.01f)
	{
		HoverBorderEffectOpacity = 0.0f;
	}

	Invalidate(EInvalidateWidgetReason::Paint);
}

int32 UTunaSweeperItemThumbnailSlotWidget::NativePaint(
	const FPaintArgs& Args,
	const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId,
	const FWidgetStyle& InWidgetStyle,
	bool bParentEnabled) const
{
	const int32 PaintedLayerId = Super::NativePaint(
		Args,
		AllottedGeometry,
		MyCullingRect,
		OutDrawElements,
		LayerId,
		InWidgetStyle,
		bParentEnabled);

	if (HoverBorderEffectOpacity <= 0.0f)
	{
		return PaintedLayerId;
	}

	DrawHoverBorderEffect(AllottedGeometry, OutDrawElements, PaintedLayerId + 1);
	return PaintedLayerId + 3;
}

void UTunaSweeperItemThumbnailSlotWidget::NativeOnMouseEnter(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);

	SetHoverBorderEffectActive(CanShowHoverBorderEffect());
	if (CanShowHoverPrompt())
	{
		SetHoveredItemSlot();
		ShowHoverPrompt(InMouseEvent);
	}
}

void UTunaSweeperItemThumbnailSlotWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	SetHoverBorderEffectActive(false);
	HideHoverPrompt();
	ClearHoveredItemSlot();
	Super::NativeOnMouseLeave(InMouseEvent);
}

FReply UTunaSweeperItemThumbnailSlotWidget::NativeOnMouseMove(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent)
{
	SetHoverBorderEffectActive(CanShowHoverBorderEffect());
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
	SetHoverBorderEffectActive(false);

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
	EnsureHoverBorderEffectWidget();
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
	if (SlotBackground)
	{
		SlotBackground->SetPadding(FMargin(1.0f));
	}

	if (!WidgetTree)
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

	if (!RaritySlotAccentWidget)
	{
		RaritySlotAccentWidget = Cast<UTunaSweeperItemRaritySlotAccentWidget>(
			WidgetTree->FindWidget(FName(TEXT("RaritySlotAccentWidget"))));
	}
	if (!RaritySlotAccentWidget)
	{
		RaritySlotAccentWidget = WidgetTree->ConstructWidget<UTunaSweeperItemRaritySlotAccentWidget>(
			UTunaSweeperItemRaritySlotAccentWidget::StaticClass(),
			FName(TEXT("RaritySlotAccentWidget")));
	}
	if (!RaritySlotAccentWidget)
	{
		return;
	}

	if (RaritySlotAccentWidget->GetParent() == SlotOverlay &&
		SlotOverlay->GetChildIndex(RaritySlotAccentWidget) != 0)
	{
		SlotOverlay->RemoveChild(RaritySlotAccentWidget);
	}
	else if (RaritySlotAccentWidget->GetParent() && RaritySlotAccentWidget->GetParent() != SlotOverlay)
	{
		RaritySlotAccentWidget->RemoveFromParent();
	}

	UOverlaySlot* AccentSlot = RaritySlotAccentWidget->GetParent() == SlotOverlay
		? Cast<UOverlaySlot>(RaritySlotAccentWidget->Slot)
		: Cast<UOverlaySlot>(SlotOverlay->InsertChildAt(0, RaritySlotAccentWidget));
	if (!AccentSlot)
	{
		AccentSlot = Cast<UOverlaySlot>(SlotOverlay->InsertChildAt(0, RaritySlotAccentWidget));
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

void UTunaSweeperItemThumbnailSlotWidget::EnsureHoverBorderEffectWidget()
{
	if (HoverBorderEffectWidget || !WidgetTree)
	{
		return;
	}

	HoverBorderEffectWidget = Cast<UTunaSweeperItemHoverBorderEffectWidget>(
		WidgetTree->FindWidget(FName(TEXT("HoverBorderEffectWidget"))));
	if (HoverBorderEffectWidget)
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

	HoverBorderEffectWidget = WidgetTree->ConstructWidget<UTunaSweeperItemHoverBorderEffectWidget>(
		UTunaSweeperItemHoverBorderEffectWidget::StaticClass(),
		FName(TEXT("HoverBorderEffectWidget")));
	if (!HoverBorderEffectWidget)
	{
		return;
	}

	UOverlaySlot* EffectSlot = SlotOverlay->AddChildToOverlay(HoverBorderEffectWidget);
	if (EffectSlot)
	{
		EffectSlot->SetHorizontalAlignment(HAlign_Fill);
		EffectSlot->SetVerticalAlignment(VAlign_Fill);
		EffectSlot->SetPadding(FMargin(0.0f));
	}
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

bool UTunaSweeperItemThumbnailSlotWidget::CanShowHoverBorderEffect() const
{
	return GetCachedSlotReference().IsValid();
}

void UTunaSweeperItemThumbnailSlotWidget::SetHoverBorderEffectActive(bool bInActive)
{
	if (bInActive && !CanShowHoverBorderEffect())
	{
		bInActive = false;
	}

	EnsureHoverBorderEffectWidget();
	if (HoverBorderEffectWidget)
	{
		HoverBorderEffectWidget->SetHoverBorderEffectActive(bInActive);
	}

	bHoverBorderEffectActive = false;
	HoverBorderEffectOpacity = 0.0f;
	Invalidate(EInvalidateWidgetReason::Paint);
}

void UTunaSweeperItemThumbnailSlotWidget::DrawHoverBorderEffect(
	const FGeometry& AllottedGeometry,
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId) const
{
	using namespace TunaSweeperItemThumbnailSlotHover;

	const FGeometry* PaintGeometry = SlotBackground ? &SlotBackground->GetCachedGeometry() : &AllottedGeometry;
	const FVector2D LocalSize = PaintGeometry->GetLocalSize();
	if (LocalSize.X <= 12.0f || LocalSize.Y <= 12.0f)
	{
		return;
	}

	const float Opacity = FMath::Clamp(HoverBorderEffectOpacity, 0.0f, 1.0f);
	const float Inset = 4.5f;
	const FVector2D AlignmentOffset(3.0f, 3.0f);
	const FVector2D Min = AlignmentOffset + FVector2D(Inset, Inset);
	const FVector2D Max = AlignmentOffset + FVector2D(
		FMath::Max(Inset + 1.0f, LocalSize.X - Inset),
		FMath::Max(Inset + 1.0f, LocalSize.Y - Inset));

	TArray<FVector2D> Points;
	Points.Reserve(12);
	Points.Add(Min);
	Points.Add(FVector2D(Max.X, Min.Y));
	Points.Add(Max);
	Points.Add(FVector2D(Min.X, Max.Y));
	Points.Add(Min);
	DrawLineStrip(
		OutDrawElements,
		LayerId,
		*PaintGeometry,
		Points,
		FLinearColor(0.10f, 0.70f, 0.82f, 0.15f * Opacity),
		1.0f);

	const float CycleAlpha = FMath::Frac(HoverBorderAnimationSeconds / 2.32f);
	constexpr float PacketLength = 0.18f;
	constexpr float LeadLength = 0.055f;
	float CornerPulse = 0.0f;

	auto DrawPacket = [&](
		float HeadAlpha,
		float AlphaScale)
	{
		const float Envelope = GetPacketEnvelope(HeadAlpha) * AlphaScale * Opacity;
		if (Envelope <= 0.0f)
		{
			return;
		}

		CornerPulse = FMath::Max(CornerPulse, GetCornerPulse(HeadAlpha) * Envelope);
		const float TailAlpha = FMath::Max(0.0f, HeadAlpha - PacketLength);
		const float LeadAlpha = FMath::Max(TailAlpha, HeadAlpha - LeadLength);

		for (const EBorderFlowRoute Route : { EBorderFlowRoute::LowerLeft, EBorderFlowRoute::UpperRight })
		{
			BuildBorderRouteSegment(Min, Max, Route, TailAlpha, HeadAlpha, Points);
			DrawLineStrip(
				OutDrawElements,
				LayerId + 1,
				*PaintGeometry,
				Points,
				FLinearColor(0.04f, 0.90f, 1.00f, 0.10f * Envelope),
				4.4f);
			DrawLineStrip(
				OutDrawElements,
				LayerId + 2,
				*PaintGeometry,
				Points,
				FLinearColor(0.12f, 0.88f, 1.00f, 0.42f * Envelope),
				1.45f);

			BuildBorderRouteSegment(Min, Max, Route, LeadAlpha, HeadAlpha, Points);
			DrawLineStrip(
				OutDrawElements,
				LayerId + 2,
				*PaintGeometry,
				Points,
				FLinearColor(0.78f, 0.98f, 1.00f, 0.78f * Envelope),
				2.0f);
		}
	};

	DrawPacket(CycleAlpha, 1.00f);
	DrawPacket(FMath::Frac(CycleAlpha + 0.36f), 0.62f);
	DrawPacket(FMath::Frac(CycleAlpha + 0.68f), 0.40f);

	const float CornerAlpha = (0.24f + 0.50f * CornerPulse) * Opacity;
	if (CornerAlpha > 0.0f)
	{
		const float CornerLength = FMath::Min(18.0f, FMath::Min(Max.X - Min.X, Max.Y - Min.Y) * 0.34f);
		Points.Reset();
		Points.Add(FVector2D(Min.X, Min.Y + CornerLength));
		Points.Add(Min);
		Points.Add(FVector2D(Min.X + CornerLength, Min.Y));
		DrawLineStrip(
			OutDrawElements,
			LayerId + 2,
			*PaintGeometry,
			Points,
			FLinearColor(0.55f, 0.96f, 1.00f, CornerAlpha),
			2.2f);
		DrawLineStrip(
			OutDrawElements,
			LayerId + 1,
			*PaintGeometry,
			Points,
			FLinearColor(0.04f, 0.82f, 1.00f, CornerAlpha * 0.22f),
			5.2f);
	}
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
