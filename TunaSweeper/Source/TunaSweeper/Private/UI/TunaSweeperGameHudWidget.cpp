#include "UI/TunaSweeperGameHudWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Character/TunaSweeperTopDownCharacter.h"
#include "Component/TunaSweeperDebuffComponent.h"
#include "Component/TunaSweeperVitalsComponent.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/Widget.h"
#include "Engine/Texture2D.h"
#include "Game/TunaSweeperGameInstance.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Framework/Application/SlateApplication.h"
#include "InputCoreTypes.h"
#include "Player/TunaSweeperPlayerController.h"
#include "Rendering/DrawElements.h"
#include "Subsystem/TunaSweeperHousingSubsystem.h"
#include "Subsystem/TunaSweeperQuestSubsystem.h"
#include "UI/TunaSweeperExtractionProgressWidget.h"
#include "UI/TunaSweeperHudBottomStatusWidget.h"
#include "UI/TunaSweeperHudDebuffBarWidget.h"
#include "UI/TunaSweeperHudExternalPanelWidget.h"
#include "UI/TunaSweeperHousingPanelWidget.h"
#include "UI/TunaSweeperHudInventoryAreaWidget.h"
#include "UI/TunaSweeperHudItemInfoPanelWidget.h"
#include "UI/TunaSweeperHudQuickSlotBarWidget.h"
#include "UI/TunaSweeperHudTopReserveWidget.h"
#include "UI/TunaSweeperItemStackTileItemObject.h"
#include "UI/TunaSweeperItemThumbnailSlotWidget.h"
#include "UI/TunaSweeperMapWidget.h"
#include "UI/TunaSweeperMemoWidget.h"
#include "UI/TunaSweeperQuestWidget.h"
#include "UI/TunaSweeperReloadRingWidget.h"
#include "UI/TunaSweeperShopSellPanelWidget.h"
#include "UI/TunaSweeperUIFont.h"
#include "UI/TunaSweeperUiText.h"
#include "Styling/SlateBrush.h"
#include "UObject/UObjectGlobals.h"

namespace
{
	constexpr int32 InventoryQuickSlotFirstNumber = 3;
	constexpr int32 InventoryQuickSlotLastNumber = 8;
	constexpr float InventoryQuickSlotPanelWidth = 760.0f;
	constexpr float InventoryQuickSlotPanelHeight = 168.0f;
	constexpr float InventoryQuickSlotTileSize = 112.0f;
	constexpr float InventoryQuickSlotTileScale = 1.12f;
	constexpr float CursorDistanceRightOffset = 34.0f;
	constexpr float CursorDistanceBottomOffset = 40.0f;
	constexpr float CursorDistanceMinTextWidth = 42.0f;
	constexpr float DebuffBarLeftOffset = 24.0f;
	constexpr float DebuffBarBottomOffset = CursorDistanceBottomOffset;
	constexpr float HudWidgetTransitionDurationSeconds = 0.18f;
	constexpr float HudWidgetTransitionDistancePadding = 36.0f;
	constexpr float HudWidgetTransitionFallbackHorizontalDistance = 420.0f;
	constexpr float HudWidgetTransitionFallbackVerticalDistance = 220.0f;
	constexpr float ShopSellPanelTransitionOffsetY = -24.0f;
	constexpr float ShopSellPanelWidth = 330.0f;
	constexpr float ShopSellPanelHeight = 240.0f;
	constexpr float UtilityPanelLeftInset = 34.0f;
	constexpr float UtilityPanelRightInset = 34.0f;
	constexpr float UtilityPanelTopOffset = 96.0f;
	constexpr float UtilityPanelHeight = 620.0f;
	constexpr float QuestMenuHorizontalMargin = 250.0f;
	constexpr float QuestMenuTopMargin = 112.0f;
	constexpr float QuestMenuBottomMargin = 56.0f;
	constexpr float QuestInteractionHorizontalMargin = 200.0f;
	constexpr float QuestInteractionTopMargin = 76.0f;
	constexpr float QuestInteractionBottomMargin = 56.0f;
	constexpr int32 MaxActiveDamageNumberPopups = 64;
	constexpr float DamageNumberGrowDurationAlpha = 0.14f / 3.0f;
	constexpr float DamageNumberSettleDurationAlpha = 0.28f / 2.0f;
	const FName PistolWeaponTypeTag(TEXT("weapon.type.pistol"));
	const FName RifleWeaponTypeTag(TEXT("weapon.type.rifle"));
	const FName ShotgunWeaponTypeTag(TEXT("weapon.type.shotgun"));

	using TunaSweeperUiText::ResolveUiText;

	FSlateBrush MakeHudRoundedBoxBrush(
		const FVector2D& ImageSize,
		const FLinearColor& FillColor,
		float Radius,
		const FLinearColor& OutlineColor,
		float OutlineWidth)
	{
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
		Brush.TintColor = FSlateColor(FillColor);
		Brush.SetImageSize(ImageSize);
		Brush.OutlineSettings = FSlateBrushOutlineSettings(Radius, FSlateColor(OutlineColor), OutlineWidth);
		Brush.OutlineSettings.bUseBrushTransparency = false;
		return Brush;
	}

	bool IsSlateVisibilityShown(ESlateVisibility Visibility)
	{
		return Visibility != ESlateVisibility::Collapsed && Visibility != ESlateVisibility::Hidden;
	}

	FWidgetTransform WithAddedTranslation(const FWidgetTransform& BaseTransform, const FVector2D& AddedTranslation)
	{
		FWidgetTransform Result = BaseTransform;
		Result.Translation = BaseTransform.Translation + AddedTranslation;
		return Result;
	}

	float SmoothTransitionAlpha(float Alpha)
	{
		const float ClampedAlpha = FMath::Clamp(Alpha, 0.0f, 1.0f);
		return ClampedAlpha * ClampedAlpha * (3.0f - 2.0f * ClampedAlpha);
	}

	float EaseOutCubic(float Alpha)
	{
		const float ClampedAlpha = FMath::Clamp(Alpha, 0.0f, 1.0f);
		const float InverseAlpha = 1.0f - ClampedAlpha;
		return 1.0f - InverseAlpha * InverseAlpha * InverseAlpha;
	}

	FLinearColor GetDamageNumberColor(ETunaSweeperDamageNumberType DamageNumberType)
	{
		switch (DamageNumberType)
		{
		case ETunaSweeperDamageNumberType::Critical:
			return FLinearColor(1.0f, 0.78f, 0.08f, 1.0f);
		case ETunaSweeperDamageNumberType::Headshot:
			return FLinearColor(1.0f, 0.18f, 0.06f, 1.0f);
		default:
			return FLinearColor(0.95f, 0.98f, 1.0f, 1.0f);
		}
	}

	float GetDamageNumberFontSize(ETunaSweeperDamageNumberType DamageNumberType)
	{
		switch (DamageNumberType)
		{
		case ETunaSweeperDamageNumberType::Critical:
			return 60.0f;
		case ETunaSweeperDamageNumberType::Headshot:
			return 72.0f;
		default:
			return 46.0f;
		}
	}

	int32 GetDamageNumberOutlineSize(ETunaSweeperDamageNumberType DamageNumberType)
	{
		switch (DamageNumberType)
		{
		case ETunaSweeperDamageNumberType::Critical:
		case ETunaSweeperDamageNumberType::Headshot:
			return 4;
		default:
			return 3;
		}
	}

	FString FormatDamageNumber(float DamageAmount)
	{
		const float RoundedDamage = FMath::RoundToFloat(DamageAmount);
		if (FMath::IsNearlyEqual(DamageAmount, RoundedDamage, 0.05f))
		{
			return FString::Printf(TEXT("%d"), FMath::RoundToInt(DamageAmount));
		}

		return FString::Printf(TEXT("%.1f"), DamageAmount);
	}

	bool IsPrecisionCrosshairWeaponType(FName WeaponTypeTag)
	{
		return WeaponTypeTag == PistolWeaponTypeTag || WeaponTypeTag == RifleWeaponTypeTag;
	}

	FTunaSweeperItemStackTileData BuildQuickSlotTileData(
		UTunaSweeperGameInstance* TunaGameInstance,
		UTunaSweeperItemDataSubsystem* ItemDataSubsystem,
		const FTunaSweeperInventorySlot& Slot,
		int32 SlotIndex)
	{
		FTunaSweeperItemStackTileData TileData;
		TileData.Source = ETunaSweeperItemSlotSource::UsableQuickSlot;
		TileData.SourceIndex = SlotIndex;
		TileData.SlotReference.Source = ETunaSweeperItemSlotSource::UsableQuickSlot;
		TileData.SlotReference.SlotIndex = SlotIndex;
		TileData.bIsEmpty = true;

		FTunaSweeperItemInstance ItemInstance;
		if (!TunaGameInstance || !Slot.ItemUid.IsValid() || !TunaGameInstance->TryGetItemInstance(Slot.ItemUid, ItemInstance))
		{
			return TileData;
		}

		TileData.ItemInstance = ItemInstance;
		TileData.ItemStack.ItemId = ItemInstance.ItemId;
		TileData.ItemStack.Quantity = FMath::Max(1, ItemInstance.Quantity);
		TileData.bIsEmpty = false;
		const ETunaSweeperItemTextLanguage Language = TunaGameInstance
			? TunaGameInstance->GetCurrentTextLanguage()
			: ETunaSweeperItemTextLanguage::English;

		if (!ItemDataSubsystem)
		{
			TileData.DisplayName = FText::Format(
				ResolveUiText(TunaGameInstance, TEXT("ui.common.item_fallback"), TEXT("Item {0}")),
				FText::AsNumber(ItemInstance.ItemId));
			return TileData;
		}

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

		return TileData;
	}
}

void UTunaSweeperGameHudWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>())
	{
		TunaGameInstance->OnSelectedInventoryItemChanged.RemoveAll(this);
		TunaGameInstance->OnSelectedInventoryItemChanged.AddUObject(this, &UTunaSweeperGameHudWidget::HandleSelectedInventoryItemChanged);
		TunaGameInstance->OnLanguageChanged.RemoveAll(this);
		TunaGameInstance->OnLanguageChanged.AddUObject(this, &UTunaSweeperGameHudWidget::HandleLanguageChanged);
	}
	if (UTunaSweeperQuestSubsystem* QuestSubsystem = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UTunaSweeperQuestSubsystem>()
		: nullptr)
	{
		QuestSubsystem->OnQuestProgressChanged.RemoveAll(this);
		QuestSubsystem->OnQuestProgressChanged.AddUObject(this, &UTunaSweeperGameHudWidget::HandleQuestProgressChanged);
	}
	if (UTunaSweeperHousingSubsystem* HousingSubsystem = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UTunaSweeperHousingSubsystem>()
		: nullptr)
	{
		HousingSubsystem->OnHousingStateChanged.RemoveAll(this);
		HousingSubsystem->OnHousingStateChanged.AddUObject(this, &UTunaSweeperGameHudWidget::HandleHousingStateChanged);
	}
	if (TopStatusReserveWidget)
	{
		TopStatusReserveWidget->OnHudModeSelected.RemoveDynamic(this, &UTunaSweeperGameHudWidget::HandleHudModeTabSelected);
		TopStatusReserveWidget->OnHudModeSelected.AddDynamic(this, &UTunaSweeperGameHudWidget::HandleHudModeTabSelected);
	}

	EnsureExtractionProgressWidget();
	EnsureCursorDistanceWidget();
	EnsureDebuffBarWidget();
	EnsureInventoryQuickSlotPanelWidget();
	EnsureHousingPanelWidget();
	EnsureMapPanelWidget();
	EnsureMemoPanelWidget();
	EnsureQuestPanelWidgets();
	TunaSweeperUIFont::ApplyFontToWidgetTree(this);
	NormalizeCenterContentPanelLayout();
	CacheAmmoCancelableActionWidgets();
	RefreshLocalizedTexts();
	SetHudMode(ETunaSweeperHudMode::None);
	SetItemInfoPanelVisible(false);
	RefreshBottomStatusFromGameInstance();
	RefreshDebuffBarFromPlayer();
	RefreshQuickSlotsFromGameState();
	RefreshInventoryQuickSlotPanel();
	RefreshCancelableActionWidgets();
	RefreshDialogueHudVisibility();
	RefreshCursorDistanceWidget();
}

void UTunaSweeperGameHudWidget::NativeDestruct()
{
	if (UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>())
	{
		TunaGameInstance->OnSelectedInventoryItemChanged.RemoveAll(this);
		TunaGameInstance->OnLanguageChanged.RemoveAll(this);
	}
	if (UTunaSweeperQuestSubsystem* QuestSubsystem = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UTunaSweeperQuestSubsystem>()
		: nullptr)
	{
		QuestSubsystem->OnQuestProgressChanged.RemoveAll(this);
	}
	if (UTunaSweeperHousingSubsystem* HousingSubsystem = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UTunaSweeperHousingSubsystem>()
		: nullptr)
	{
		HousingSubsystem->OnHousingStateChanged.RemoveAll(this);
	}
	if (TopStatusReserveWidget)
	{
		TopStatusReserveWidget->OnHudModeSelected.RemoveDynamic(this, &UTunaSweeperGameHudWidget::HandleHudModeTabSelected);
	}

	UpdateMouseCursorForReloadGauge(false);

	Super::NativeDestruct();
}

void UTunaSweeperGameHudWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	RefreshBottomStatusFromGameInstance();
	RefreshDebuffBarFromPlayer();
	RefreshQuickSlotsFromGameState();
	RefreshInventoryQuickSlotPanel();
	RefreshCancelableActionWidgets(&MyGeometry);
	RefreshDialogueHudVisibility();
	RefreshExtractionProgressWidget();
	RefreshCursorDistanceWidget();
	TickHudTransitions(InDeltaTime);
	UpdateCrosshairState(InDeltaTime);
	TickDamageNumberPopups(InDeltaTime);
	Invalidate(EInvalidateWidgetReason::Paint);
}

FReply UTunaSweeperGameHudWidget::NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Tab && IsHousingModeActive())
	{
		if (UTunaSweeperHousingSubsystem* HousingSubsystem = GetGameInstance()
			? GetGameInstance()->GetSubsystem<UTunaSweeperHousingSubsystem>()
			: nullptr)
		{
			HousingSubsystem->CloseHousingMode();
		}
		return FReply::Handled();
	}

	if (InKeyEvent.GetKey() == EKeys::Tab && ActiveHudMode != ETunaSweeperHudMode::None)
	{
		ToggleInventoryOnlyPanel();
		if (ATunaSweeperPlayerController* TunaPlayerController = Cast<ATunaSweeperPlayerController>(GetOwningPlayer()))
		{
			TunaPlayerController->ApplyDefaultGameInputMode();
		}
		return FReply::Handled();
	}

	return Super::NativeOnPreviewKeyDown(InGeometry, InKeyEvent);
}

int32 UTunaSweeperGameHudWidget::NativePaint(
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

	if (IsWeaponCrosshairSuppressed() || !FSlateApplication::IsInitialized())
	{
		return PaintedLayerId;
	}

	const FName WeaponTypeTag = GetSelectedWeaponTypeTag();
	const bool bShotgunCrosshair = WeaponTypeTag == ShotgunWeaponTypeTag;
	const bool bPrecisionCrosshair = IsPrecisionCrosshairWeaponType(WeaponTypeTag);
	if (!bShotgunCrosshair && !bPrecisionCrosshair)
	{
		return PaintedLayerId;
	}

	FVector2D CrosshairLocalPosition = FVector2D::ZeroVector;
	if (!TryGetWeaponCrosshairLocalPosition(AllottedGeometry, CrosshairLocalPosition))
	{
		return PaintedLayerId;
	}

	auto DrawLineStrip = [&](
		const TArray<FVector2D>& Points,
		const FLinearColor& Color,
		float Thickness,
		int32 DrawLayerId)
	{
		if (Points.Num() < 2 || Color.A <= 0.0f)
		{
			return;
		}

		FSlateDrawElement::MakeLines(
			OutDrawElements,
			DrawLayerId,
			AllottedGeometry.ToPaintGeometry(),
			Points,
			ESlateDrawEffect::None,
			Color,
			true,
			FMath::Max(1.0f, Thickness));
	};

	auto BuildArcPoints = [](
		const FVector2D& Center,
		float Radius,
		float StartAngleDegrees,
		float EndAngleDegrees,
		int32 SegmentCount,
		TArray<FVector2D>& OutPoints)
	{
		OutPoints.Reset();
		OutPoints.Reserve(SegmentCount + 1);
		for (int32 SegmentIndex = 0; SegmentIndex <= SegmentCount; ++SegmentIndex)
		{
			const float Alpha = static_cast<float>(SegmentIndex) / static_cast<float>(SegmentCount);
			const float AngleDegrees = FMath::Lerp(StartAngleDegrees, EndAngleDegrees, Alpha);
			const float AngleRadians = FMath::DegreesToRadians(AngleDegrees);
			OutPoints.Add(Center + FVector2D(FMath::Cos(AngleRadians), FMath::Sin(AngleRadians)) * Radius);
		}
	};

	if (bShotgunCrosshair)
	{
		const int32 SegmentCount = FMath::Clamp(ShotgunCrosshairSegmentCount, 8, 128);
		TArray<FVector2D> CirclePoints;
		BuildArcPoints(CrosshairLocalPosition, FMath::Max(1.0f, ShotgunCrosshairRadius), 0.0f, 360.0f, SegmentCount, CirclePoints);
		DrawLineStrip(CirclePoints, ShotgunCrosshairColor, ShotgunCrosshairThickness, PaintedLayerId + 1);
		return PaintedLayerId + 1;
	}

	const float AimAlpha = SmoothTransitionAlpha(PrecisionCrosshairAimAlpha);
	const float ParenthesisAlpha = 1.0f - AimAlpha;
	if (ParenthesisAlpha > 0.01f)
	{
		FLinearColor ParenthesisColor = PrecisionCrosshairColor;
		ParenthesisColor.A *= ParenthesisAlpha;

		const int32 ParenthesisSegments = 18;
		const float ParenthesisOffset = FMath::Max(1.0f, PrecisionCrosshairParenthesisOffset);
		const float ParenthesisRadius = FMath::Max(1.0f, PrecisionCrosshairParenthesisRadius);
		TArray<FVector2D> ParenthesisPoints;
		BuildArcPoints(
			CrosshairLocalPosition + FVector2D(-ParenthesisOffset, 0.0f),
			ParenthesisRadius,
			110.0f,
			250.0f,
			ParenthesisSegments,
			ParenthesisPoints);
		DrawLineStrip(ParenthesisPoints, ParenthesisColor, PrecisionCrosshairThickness, PaintedLayerId + 1);

		BuildArcPoints(
			CrosshairLocalPosition + FVector2D(ParenthesisOffset, 0.0f),
			ParenthesisRadius,
			-70.0f,
			70.0f,
			ParenthesisSegments,
			ParenthesisPoints);
		DrawLineStrip(ParenthesisPoints, ParenthesisColor, PrecisionCrosshairThickness, PaintedLayerId + 1);
	}

	if (AimAlpha > 0.01f)
	{
		FLinearColor BarColor = PrecisionCrosshairColor;
		BarColor.A *= AimAlpha;

		const float BarDistance = FMath::Lerp(
			FMath::Max(1.0f, PrecisionCrosshairAimBarStartDistance),
			FMath::Max(1.0f, PrecisionCrosshairAimBarEndDistance),
			AimAlpha);
		const float BarLength = FMath::Max(1.0f, PrecisionCrosshairAimBarLength);

		TArray<FVector2D> SegmentPoints;
		SegmentPoints.SetNum(2);
		SegmentPoints[0] = CrosshairLocalPosition + FVector2D(-BarDistance - BarLength, 0.0f);
		SegmentPoints[1] = CrosshairLocalPosition + FVector2D(-BarDistance, 0.0f);
		DrawLineStrip(SegmentPoints, BarColor, PrecisionCrosshairThickness, PaintedLayerId + 2);

		SegmentPoints[0] = CrosshairLocalPosition + FVector2D(BarDistance, 0.0f);
		SegmentPoints[1] = CrosshairLocalPosition + FVector2D(BarDistance + BarLength, 0.0f);
		DrawLineStrip(SegmentPoints, BarColor, PrecisionCrosshairThickness, PaintedLayerId + 2);

		SegmentPoints[0] = CrosshairLocalPosition + FVector2D(0.0f, -BarDistance - BarLength);
		SegmentPoints[1] = CrosshairLocalPosition + FVector2D(0.0f, -BarDistance);
		DrawLineStrip(SegmentPoints, BarColor, PrecisionCrosshairThickness, PaintedLayerId + 2);

		SegmentPoints[0] = CrosshairLocalPosition + FVector2D(0.0f, BarDistance);
		SegmentPoints[1] = CrosshairLocalPosition + FVector2D(0.0f, BarDistance + BarLength);
		DrawLineStrip(SegmentPoints, BarColor, PrecisionCrosshairThickness, PaintedLayerId + 2);
	}

	return PaintedLayerId + 2;
}

void UTunaSweeperGameHudWidget::SetCenterPanelsVisible(bool bVisible)
{
	if (!bVisible)
	{
		CloseLootContainerPanelIfOpen();
	}

	ActiveHudMode = bVisible && ActiveHudMode == ETunaSweeperHudMode::None
		? ETunaSweeperHudMode::Inventory
		: (bVisible ? ActiveHudMode : ETunaSweeperHudMode::None);
	ApplyHudModeVisibility();
}

void UTunaSweeperGameHudWidget::SetInventoryAreaVisible(bool bVisible)
{
	if (bVisible)
	{
		ActiveHudMode = ETunaSweeperHudMode::Inventory;
		ApplyHudModeVisibility();
	}

	if (InventoryAreaWidget)
	{
		SetTransitionedWidgetVisibility(
			InventoryAreaWidget,
			bVisible ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed,
			InventoryAreaTransitionEdge);
		InventoryAreaWidget->SetInventoryVisible(bVisible);
	}
}

void UTunaSweeperGameHudWidget::SetItemInfoPanelVisible(bool bVisible)
{
	if (bVisible)
	{
		ActiveHudMode = ETunaSweeperHudMode::Inventory;
		ApplyHudModeVisibility();
	}

	if (ItemInfoPanelWidget)
	{
		SetTransitionedWidgetVisibility(
			ItemInfoPanelWidget,
			bVisible && ActiveHudMode == ETunaSweeperHudMode::Inventory
				? ESlateVisibility::SelfHitTestInvisible
				: ESlateVisibility::Collapsed,
			ItemInfoPanelTransitionEdge);
	}
}

void UTunaSweeperGameHudWidget::ShowExternalPanel(ETunaSweeperHudExternalPanelMode PanelMode)
{
	if (PanelMode != ETunaSweeperHudExternalPanelMode::LootingBox &&
		PanelMode != ETunaSweeperHudExternalPanelMode::Storage &&
		PanelMode != ETunaSweeperHudExternalPanelMode::Shop)
	{
		if (PanelMode != ETunaSweeperHudExternalPanelMode::Workbench)
		{
			CloseLootContainerPanelIfOpen();
		}
	}

	if (PanelMode != ETunaSweeperHudExternalPanelMode::None)
	{
		ActiveHudMode = ETunaSweeperHudMode::Inventory;
	}

	if (ExternalPanelWidget)
	{
		if (PanelMode != ETunaSweeperHudExternalPanelMode::None)
		{
			bClearExternalPanelModeAfterHide = false;
			ExternalPanelWidget->SetExternalPanelMode(PanelMode);
		}
		else if (!bClearExternalPanelModeAfterHide ||
			(!IsSlateVisibilityShown(ExternalPanelWidget->GetVisibility()) && !HasActiveHudTransition(ExternalPanelWidget)))
		{
			bClearExternalPanelModeAfterHide = false;
			ExternalPanelWidget->SetExternalPanelMode(ETunaSweeperHudExternalPanelMode::None);
		}
	}

	ApplyHudModeVisibility();
}

void UTunaSweeperGameHudWidget::ShowInventoryOnlyPanel()
{
	if (IsBunkerMap())
	{
		ShowStoragePanel();
		return;
	}

	SetHudMode(ETunaSweeperHudMode::Inventory);
	ShowExternalPanel(ETunaSweeperHudExternalPanelMode::None);
	HandleSelectedInventoryItemChanged();
}

void UTunaSweeperGameHudWidget::ToggleInventoryOnlyPanel()
{
	if (ActiveHudMode == ETunaSweeperHudMode::None)
	{
		ShowInventoryOnlyPanel();
	}
	else
	{
		if (UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>())
		{
			TunaGameInstance->ClearSelectedItemSelection();
		}

		CloseLootContainerPanelIfOpen();
		SetHudMode(ETunaSweeperHudMode::None);
	}
}

void UTunaSweeperGameHudWidget::ShowLootContainerPanel(const FTunaSweeperLootContainerInstance& ContainerInstance)
{
	ActiveHudMode = ETunaSweeperHudMode::Inventory;

	if (ExternalPanelWidget)
	{
		bClearExternalPanelModeAfterHide = false;
		ExternalPanelWidget->SetLootContainerInstance(ContainerInstance);
	}

	ApplyHudModeVisibility();
	HandleSelectedInventoryItemChanged();
}

void UTunaSweeperGameHudWidget::ShowStoragePanel()
{
	if (!IsBunkerMap())
	{
		return;
	}

	ShowExternalPanel(ETunaSweeperHudExternalPanelMode::Storage);
	if (ExternalPanelWidget)
	{
		ExternalPanelWidget->SetStorageContainer();
	}

	HandleSelectedInventoryItemChanged();
}

void UTunaSweeperGameHudWidget::ShowShopPanel(int32 ShopId)
{
	if (!IsBunkerMap())
	{
		return;
	}

	CloseLootContainerPanelIfOpen();

	if (UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>())
	{
		TunaGameInstance->SetActiveShop(ShopId);
	}

	ShowExternalPanel(ETunaSweeperHudExternalPanelMode::Shop);
	if (ExternalPanelWidget)
	{
		ExternalPanelWidget->SetShopContainer(ShopId);
	}

	HandleSelectedInventoryItemChanged();
}

void UTunaSweeperGameHudWidget::ShowWorkbenchPanel(int32 WorkbenchId, ETunaSweeperWorkbenchMode WorkbenchMode)
{
	if (!IsBunkerMap())
	{
		return;
	}

	CloseLootContainerPanelIfOpen();

	if (UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>())
	{
		TunaGameInstance->SetActiveWorkbench(WorkbenchId, WorkbenchMode);
		if (WorkbenchMode == ETunaSweeperWorkbenchMode::Craft)
		{
			TunaGameInstance->ClearSelectedItemSelection();
		}
	}

	ShowExternalPanel(ETunaSweeperHudExternalPanelMode::Workbench);
	if (ExternalPanelWidget)
	{
		ExternalPanelWidget->SetWorkbenchContainer(WorkbenchId, WorkbenchMode);
	}

	HandleSelectedInventoryItemChanged();
}

void UTunaSweeperGameHudWidget::ShowMemoPanel(int32 MemoId)
{
	SetHudMode(ETunaSweeperHudMode::Memo);
	EnsureMemoPanelWidget();
	if (MemoPanelWidget)
	{
		MemoPanelWidget->OpenMemo(MemoId);
	}
}

void UTunaSweeperGameHudWidget::ShowQuestPanel(FName QuestId)
{
	bQuestPanelOpenedFromInteraction = true;
	SetHudMode(ETunaSweeperHudMode::Quest);
	EnsureQuestPanelWidgets();
	if (InteractionQuestPanelWidget)
	{
		InteractionQuestPanelWidget->InitializeQuest(QuestId);
	}
}

void UTunaSweeperGameHudWidget::ShowMenuQuestPanel(FName QuestId)
{
	bQuestPanelOpenedFromInteraction = false;
	SetHudMode(ETunaSweeperHudMode::Quest);
	EnsureQuestPanelWidgets();
	if (MenuQuestPanelWidget)
	{
		MenuQuestPanelWidget->InitializeQuest(QuestId);
	}
}

void UTunaSweeperGameHudWidget::SetHudMode(ETunaSweeperHudMode InHudMode)
{
	if (InHudMode != ETunaSweeperHudMode::None && IsHousingModeActive())
	{
		return;
	}

	if (ActiveHudMode == ETunaSweeperHudMode::Quest && InHudMode != ETunaSweeperHudMode::Quest)
	{
		if (MenuQuestPanelWidget)
		{
			MenuQuestPanelWidget->ResetQuestSelection();
		}
		if (InteractionQuestPanelWidget)
		{
			InteractionQuestPanelWidget->ResetQuestSelection();
		}
	}

	if (InHudMode != ETunaSweeperHudMode::Quest)
	{
		bQuestPanelOpenedFromInteraction = false;
	}

	if (InHudMode != ETunaSweeperHudMode::Inventory)
	{
		CloseLootContainerPanelIfOpen();
	}

	if (InHudMode != ETunaSweeperHudMode::None)
	{
		if (UTunaSweeperHousingSubsystem* HousingSubsystem = GetGameInstance()
			? GetGameInstance()->GetSubsystem<UTunaSweeperHousingSubsystem>()
			: nullptr)
		{
			HousingSubsystem->CloseHousingMode();
		}
	}

	ActiveHudMode = InHudMode;
	if (ActiveHudMode == ETunaSweeperHudMode::Inventory &&
		IsBunkerMap() &&
		ExternalPanelWidget &&
		ExternalPanelWidget->GetExternalPanelMode() == ETunaSweeperHudExternalPanelMode::None)
	{
		bClearExternalPanelModeAfterHide = false;
		ExternalPanelWidget->SetStorageContainer();
	}
	ApplyHudModeVisibility();
	HandleSelectedInventoryItemChanged();
}

void UTunaSweeperGameHudWidget::SetExtractionProgress(
	float CurrentSeconds,
	float RequiredSeconds,
	bool bActive)
{
	ExtractionProgressCurrentSeconds = FMath::Max(0.0f, CurrentSeconds);
	ExtractionProgressRequiredSeconds = FMath::Max(0.1f, RequiredSeconds);
	bExtractionProgressActive = bActive && ExtractionProgressCurrentSeconds > 0.0f;
	RefreshExtractionProgressWidget();
}

bool UTunaSweeperGameHudWidget::IsInventoryUiOpen() const
{
	if (ActiveHudMode != ETunaSweeperHudMode::None)
	{
		return true;
	}

	auto IsWidgetVisible = [](const UWidget* Widget)
	{
		if (!Widget)
		{
			return false;
		}

		const ESlateVisibility Visibility = Widget->GetVisibility();
		return Visibility != ESlateVisibility::Collapsed && Visibility != ESlateVisibility::Hidden;
	};

	return
		IsWidgetVisible(InventoryAreaWidget) ||
		IsWidgetVisible(ItemInfoPanelWidget) ||
		IsWidgetVisible(ExternalPanelWidget) ||
		IsWidgetVisible(InventoryQuickSlotPanel) ||
		IsWidgetVisible(ShopSellPanelWidget);
}

bool UTunaSweeperGameHudWidget::IsShopPanelOpen() const
{
	return ActiveHudMode == ETunaSweeperHudMode::Inventory &&
		ExternalPanelWidget &&
		ExternalPanelWidget->GetExternalPanelMode() == ETunaSweeperHudExternalPanelMode::Shop;
}

bool UTunaSweeperGameHudWidget::IsWorkbenchPanelOpen() const
{
	return ActiveHudMode == ETunaSweeperHudMode::Inventory &&
		ExternalPanelWidget &&
		ExternalPanelWidget->GetExternalPanelMode() == ETunaSweeperHudExternalPanelMode::Workbench;
}

ETunaSweeperWorkbenchMode UTunaSweeperGameHudWidget::GetWorkbenchPanelMode() const
{
	const UTunaSweeperGameInstance* WorkbenchGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	return WorkbenchGameInstance && IsWorkbenchPanelOpen()
		? WorkbenchGameInstance->GetActiveWorkbenchMode()
		: ETunaSweeperWorkbenchMode::Craft;
}

bool UTunaSweeperGameHudWidget::TryAssignWorkbenchDismantleCandidateToTarget(
	const FTunaSweeperItemSlotReference& SlotReference)
{
	return IsWorkbenchPanelOpen() &&
		GetWorkbenchPanelMode() == ETunaSweeperWorkbenchMode::Dismantle &&
		ExternalPanelWidget &&
		ExternalPanelWidget->AssignWorkbenchDismantleCandidateToTarget(SlotReference);
}

bool UTunaSweeperGameHudWidget::TryAssignFocusedWorkbenchDismantleCandidateToTarget()
{
	return IsWorkbenchPanelOpen() &&
		GetWorkbenchPanelMode() == ETunaSweeperWorkbenchMode::Dismantle &&
		ExternalPanelWidget &&
		ExternalPanelWidget->AssignFocusedWorkbenchDismantleCandidateToTarget();
}

bool UTunaSweeperGameHudWidget::TryAssignWorkbenchBlueprintItemToTarget(
	const FTunaSweeperItemSlotReference& SlotReference)
{
	return IsWorkbenchPanelOpen() &&
		GetWorkbenchPanelMode() == ETunaSweeperWorkbenchMode::BlueprintRegister &&
		ExternalPanelWidget &&
		ExternalPanelWidget->AssignWorkbenchBlueprintItemToTarget(SlotReference);
}

bool UTunaSweeperGameHudWidget::TryAssignFocusedWorkbenchBlueprintItemToTarget()
{
	return IsWorkbenchPanelOpen() &&
		GetWorkbenchPanelMode() == ETunaSweeperWorkbenchMode::BlueprintRegister &&
		ExternalPanelWidget &&
		ExternalPanelWidget->AssignFocusedWorkbenchBlueprintItemToTarget();
}

bool UTunaSweeperGameHudWidget::TrySellSelectedShopItem()
{
	if (!IsShopPanelOpen())
	{
		return false;
	}

	EnsureShopSellPanelWidget();
	return ShopSellPanelWidget && ShopSellPanelWidget->TrySellSelectedHoveredItem();
}

void UTunaSweeperGameHudWidget::ShowDamageNumber(
	float DamageAmount,
	FVector WorldLocation,
	ETunaSweeperDamageNumberType DamageNumberType)
{
	if (DamageAmount <= 0.0f || !WidgetTree)
	{
		return;
	}

	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
	if (!RootCanvas)
	{
		return;
	}

	while (DamageNumberPopups.Num() >= MaxActiveDamageNumberPopups)
	{
		RemoveDamageNumberPopupAt(0);
	}

	UTextBlock* DamageText = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(),
		MakeUniqueObjectName(WidgetTree, UTextBlock::StaticClass(), TEXT("DamageNumberText")));
	if (!DamageText)
	{
		return;
	}

	DamageText->SetText(FText::FromString(FormatDamageNumber(DamageAmount)));
	DamageText->SetColorAndOpacity(FSlateColor(GetDamageNumberColor(DamageNumberType)));
	DamageText->SetJustification(ETextJustify::Center);
	DamageText->SetVisibility(ESlateVisibility::HitTestInvisible);
	DamageText->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	DamageText->SetShadowOffset(FVector2D::ZeroVector);
	DamageText->SetShadowColorAndOpacity(FLinearColor::Transparent);
	FSlateFontInfo DamageFont = TunaSweeperUIFont::MakeFont(
		DamageText,
		GetDamageNumberFontSize(DamageNumberType),
		ETunaSweeperUIFontWeight::Bold);
	DamageFont.OutlineSettings = FFontOutlineSettings(
		GetDamageNumberOutlineSize(DamageNumberType),
		FLinearColor(0.0f, 0.0f, 0.0f, 0.92f));
	DamageText->SetFont(DamageFont);

	UCanvasPanelSlot* CanvasSlot = RootCanvas->AddChildToCanvas(DamageText);
	if (!CanvasSlot)
	{
		return;
	}

	CanvasSlot->SetAutoSize(true);
	CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
	CanvasSlot->SetZOrder(950);

	FDamageNumberPopup Popup;
	Popup.TextWidget = DamageText;
	Popup.WorldLocation = WorldLocation;
	Popup.DamageNumberType = DamageNumberType;
	switch (DamageNumberType)
	{
	case ETunaSweeperDamageNumberType::Critical:
		Popup.DurationSeconds = 0.92f;
		Popup.RiseDistance = 76.0f;
		Popup.PeakScale = 3.0f;
		Popup.SettleScale = 1.5f;
		Popup.FadeStartAlpha = 0.48f;
		Popup.ScreenDrift = FVector2D(FMath::FRandRange(-24.0f, 24.0f), FMath::FRandRange(-8.0f, 2.0f));
		break;
	case ETunaSweeperDamageNumberType::Headshot:
		Popup.DurationSeconds = 1.08f;
		Popup.RiseDistance = 104.0f;
		Popup.PeakScale = 3.0f;
		Popup.SettleScale = 1.5f;
		Popup.FadeStartAlpha = 0.56f;
		Popup.ScreenDrift = FVector2D(FMath::FRandRange(-34.0f, 34.0f), FMath::FRandRange(-14.0f, 0.0f));
		break;
	default:
		Popup.DurationSeconds = 0.72f;
		Popup.RiseDistance = 46.0f;
		Popup.PeakScale = 1.28f;
		Popup.SettleScale = 1.0f;
		Popup.FadeStartAlpha = 0.42f;
		Popup.ScreenDrift = FVector2D(FMath::FRandRange(-14.0f, 14.0f), FMath::FRandRange(-4.0f, 4.0f));
		break;
	}

	DamageNumberPopups.Add(MoveTemp(Popup));
	TickDamageNumberPopups(0.0f);
}

void UTunaSweeperGameHudWidget::CacheHudTransitionBaseline(UWidget* Widget)
{
	if (!Widget)
	{
		return;
	}

	const TWeakObjectPtr<UWidget> WidgetKey(Widget);
	if (!HudTransitionBaseTransforms.Contains(WidgetKey))
	{
		HudTransitionBaseTransforms.Add(WidgetKey, Widget->GetRenderTransform());
		HudTransitionBaseOpacities.Add(WidgetKey, Widget->GetRenderOpacity());
	}
}

bool UTunaSweeperGameHudWidget::HasActiveHudTransition(const UWidget* Widget) const
{
	if (!Widget)
	{
		return false;
	}

	for (const FHudWidgetTransition& Transition : ActiveHudTransitions)
	{
		if (Transition.Widget.Get() == Widget)
		{
			return true;
		}
	}

	return false;
}

ETunaSweeperHudTransitionEdge UTunaSweeperGameHudWidget::ResolveHudTransitionEdge(
	const UWidget* Widget,
	ETunaSweeperHudTransitionEdge DirectionOverride) const
{
	if (!Widget || DirectionOverride != ETunaSweeperHudTransitionEdge::Auto)
	{
		return DirectionOverride;
	}

	const UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Widget->Slot);
	if (!CanvasSlot)
	{
		return ETunaSweeperHudTransitionEdge::FadeOnly;
	}

	const FAnchors Anchors = CanvasSlot->GetAnchors();
	const FVector2D Alignment = CanvasSlot->GetAlignment();
	const FVector2D Position = CanvasSlot->GetPosition();
	const bool bInsideCenterContentPanel = CenterContentPanel && Widget->GetParent() == CenterContentPanel;

	ETunaSweeperHudTransitionEdge BestEdge = ETunaSweeperHudTransitionEdge::FadeOnly;
	float BestScore = TNumericLimits<float>::Max();
	auto ConsiderEdge = [&BestEdge, &BestScore](ETunaSweeperHudTransitionEdge Edge, bool bCandidate, float Score)
	{
		if (bCandidate && Score < BestScore)
		{
			BestEdge = Edge;
			BestScore = Score;
		}
	};

	const bool bPinnedLeft = Anchors.Minimum.X <= 0.05f && Anchors.Maximum.X <= 0.05f && Alignment.X <= 0.5f;
	const bool bPinnedRight = Anchors.Minimum.X >= 0.95f && Anchors.Maximum.X >= 0.95f && Alignment.X >= 0.5f;
	ConsiderEdge(ETunaSweeperHudTransitionEdge::Left, bPinnedLeft, FMath::Abs(Position.X));
	ConsiderEdge(ETunaSweeperHudTransitionEdge::Right, bPinnedRight, FMath::Abs(Position.X));

	if (!bInsideCenterContentPanel)
	{
		const bool bPinnedTop = Anchors.Minimum.Y <= 0.05f && Anchors.Maximum.Y <= 0.05f && Alignment.Y <= 0.5f;
		const bool bPinnedBottom = Anchors.Minimum.Y >= 0.95f && Anchors.Maximum.Y >= 0.95f && Alignment.Y >= 0.5f;
		ConsiderEdge(ETunaSweeperHudTransitionEdge::Top, bPinnedTop, FMath::Abs(Position.Y));
		ConsiderEdge(ETunaSweeperHudTransitionEdge::Bottom, bPinnedBottom, FMath::Abs(Position.Y));
	}

	return BestEdge;
}

FVector2D UTunaSweeperGameHudWidget::GetHudTransitionHiddenTranslation(
	const UWidget* Widget,
	ETunaSweeperHudTransitionEdge Edge) const
{
	if (Edge == ETunaSweeperHudTransitionEdge::Auto || Edge == ETunaSweeperHudTransitionEdge::FadeOnly || !Widget)
	{
		return FVector2D::ZeroVector;
	}

	const bool bHorizontalEdge = Edge == ETunaSweeperHudTransitionEdge::Left || Edge == ETunaSweeperHudTransitionEdge::Right;
	float Distance = bHorizontalEdge
		? HudWidgetTransitionFallbackHorizontalDistance
		: HudWidgetTransitionFallbackVerticalDistance;

	if (const UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Widget->Slot))
	{
		const FVector2D SlotSize = CanvasSlot->GetSize();
		const float SlotDistance = bHorizontalEdge ? SlotSize.X : SlotSize.Y;
		if (SlotDistance > 1.0f)
		{
			Distance = FMath::Max(Distance, SlotDistance + HudWidgetTransitionDistancePadding);
		}
	}

	switch (Edge)
	{
	case ETunaSweeperHudTransitionEdge::Left:
		return FVector2D(-Distance, 0.0f);
	case ETunaSweeperHudTransitionEdge::Right:
		return FVector2D(Distance, 0.0f);
	case ETunaSweeperHudTransitionEdge::Top:
		return FVector2D(0.0f, -Distance);
	case ETunaSweeperHudTransitionEdge::Bottom:
		return FVector2D(0.0f, Distance);
	default:
		return FVector2D::ZeroVector;
	}
}

void UTunaSweeperGameHudWidget::SetTransitionedWidgetVisibility(
	UWidget* Widget,
	ESlateVisibility TargetVisibility,
	ETunaSweeperHudTransitionEdge DirectionOverride)
{
	if (!Widget)
	{
		return;
	}

	const ETunaSweeperHudTransitionEdge ResolvedEdge = ResolveHudTransitionEdge(Widget, DirectionOverride);
	SetTransitionedWidgetVisibilityFromTranslation(
		Widget,
		TargetVisibility,
		GetHudTransitionHiddenTranslation(Widget, ResolvedEdge));
}

void UTunaSweeperGameHudWidget::SetTransitionedWidgetVisibilityFromTranslation(
	UWidget* Widget,
	ESlateVisibility TargetVisibility,
	const FVector2D& HiddenTranslation)
{
	if (!Widget)
	{
		return;
	}

	const bool bTargetShown = IsSlateVisibilityShown(TargetVisibility);
	const bool bCurrentlyShown = IsSlateVisibilityShown(Widget->GetVisibility());
	if (!bTargetShown && !bCurrentlyShown && !HasActiveHudTransition(Widget))
	{
		Widget->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	if (bTargetShown && bCurrentlyShown && !HasActiveHudTransition(Widget))
	{
		Widget->SetVisibility(TargetVisibility);
		return;
	}

	for (const FHudWidgetTransition& Transition : ActiveHudTransitions)
	{
		if (Transition.Widget.Get() == Widget &&
			Transition.bShow == bTargetShown &&
			Transition.FinalVisibility == (bTargetShown ? TargetVisibility : ESlateVisibility::Collapsed))
		{
			return;
		}
	}

	CacheHudTransitionBaseline(Widget);

	const TWeakObjectPtr<UWidget> WidgetKey(Widget);
	const FWidgetTransform BaseTransform = HudTransitionBaseTransforms.FindRef(WidgetKey);
	const float BaseOpacity = HudTransitionBaseOpacities.Contains(WidgetKey)
		? HudTransitionBaseOpacities.FindRef(WidgetKey)
		: 1.0f;
	const FWidgetTransform HiddenTransform = WithAddedTranslation(BaseTransform, HiddenTranslation);

	for (int32 Index = ActiveHudTransitions.Num() - 1; Index >= 0; --Index)
	{
		if (ActiveHudTransitions[Index].Widget.Get() == Widget)
		{
			ActiveHudTransitions.RemoveAtSwap(Index);
		}
	}

	if (bTargetShown)
	{
		if (!bCurrentlyShown)
		{
			Widget->SetRenderTransform(HiddenTransform);
			Widget->SetRenderOpacity(0.0f);
		}
		Widget->SetVisibility(TargetVisibility);
	}

	FHudWidgetTransition Transition;
	Transition.Widget = Widget;
	Transition.StartTransform = Widget->GetRenderTransform();
	Transition.EndTransform = bTargetShown ? BaseTransform : HiddenTransform;
	Transition.StartOpacity = Widget->GetRenderOpacity();
	Transition.EndOpacity = bTargetShown ? BaseOpacity : 0.0f;
	Transition.DurationSeconds = HudWidgetTransitionDurationSeconds;
	Transition.FinalVisibility = bTargetShown ? TargetVisibility : ESlateVisibility::Collapsed;
	Transition.bShow = bTargetShown;
	ActiveHudTransitions.Add(Transition);
}

void UTunaSweeperGameHudWidget::TickHudTransitions(float InDeltaTime)
{
	for (int32 Index = ActiveHudTransitions.Num() - 1; Index >= 0; --Index)
	{
		FHudWidgetTransition& Transition = ActiveHudTransitions[Index];
		UWidget* Widget = Transition.Widget.Get();
		if (!Widget)
		{
			ActiveHudTransitions.RemoveAtSwap(Index);
			continue;
		}

		Transition.ElapsedSeconds += FMath::Max(0.0f, InDeltaTime);
		const float RawAlpha = Transition.DurationSeconds > KINDA_SMALL_NUMBER
			? Transition.ElapsedSeconds / Transition.DurationSeconds
			: 1.0f;
		const float Alpha = SmoothTransitionAlpha(RawAlpha);

		FWidgetTransform CurrentTransform = Transition.StartTransform;
		CurrentTransform.Translation = FMath::Lerp(Transition.StartTransform.Translation, Transition.EndTransform.Translation, Alpha);
		CurrentTransform.Scale = FMath::Lerp(Transition.StartTransform.Scale, Transition.EndTransform.Scale, Alpha);
		CurrentTransform.Shear = FMath::Lerp(Transition.StartTransform.Shear, Transition.EndTransform.Shear, Alpha);
		CurrentTransform.Angle = FMath::Lerp(Transition.StartTransform.Angle, Transition.EndTransform.Angle, Alpha);
		Widget->SetRenderTransform(CurrentTransform);
		Widget->SetRenderOpacity(FMath::Lerp(Transition.StartOpacity, Transition.EndOpacity, Alpha));

		if (RawAlpha >= 1.0f)
		{
			const bool bCompletedExternalPanelHide =
				!Transition.bShow &&
				bClearExternalPanelModeAfterHide &&
				ExternalPanelWidget &&
				Widget == ExternalPanelWidget;

			Widget->SetRenderTransform(Transition.EndTransform);
			Widget->SetRenderOpacity(Transition.EndOpacity);
			Widget->SetVisibility(Transition.FinalVisibility);

			if (!Transition.bShow)
			{
				const TWeakObjectPtr<UWidget> WidgetKey(Widget);
				if (const FWidgetTransform* BaseTransform = HudTransitionBaseTransforms.Find(WidgetKey))
				{
					Widget->SetRenderTransform(*BaseTransform);
				}
				if (const float* BaseOpacity = HudTransitionBaseOpacities.Find(WidgetKey))
				{
					Widget->SetRenderOpacity(*BaseOpacity);
				}
			}

			if (bCompletedExternalPanelHide)
			{
				ExternalPanelWidget->SetExternalPanelMode(ETunaSweeperHudExternalPanelMode::None);
				bClearExternalPanelModeAfterHide = false;
			}

			ActiveHudTransitions.RemoveAtSwap(Index);
		}
	}
}

void UTunaSweeperGameHudWidget::TickDamageNumberPopups(float InDeltaTime)
{
	APlayerController* PlayerController = GetOwningPlayer();
	if (!PlayerController)
	{
		return;
	}

	for (int32 Index = DamageNumberPopups.Num() - 1; Index >= 0; --Index)
	{
		FDamageNumberPopup& Popup = DamageNumberPopups[Index];
		UTextBlock* TextWidget = Popup.TextWidget.Get();
		if (!TextWidget)
		{
			DamageNumberPopups.RemoveAt(Index);
			continue;
		}

		Popup.ElapsedSeconds += FMath::Max(0.0f, InDeltaTime);
		const float DurationSeconds = FMath::Max(0.01f, Popup.DurationSeconds);
		const float Alpha = FMath::Clamp(Popup.ElapsedSeconds / DurationSeconds, 0.0f, 1.0f);
		if (Alpha >= 1.0f)
		{
			RemoveDamageNumberPopupAt(Index);
			continue;
		}

		FVector2D ScreenPosition = FVector2D::ZeroVector;
		if (!UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(
			PlayerController,
			Popup.WorldLocation,
			ScreenPosition,
			false))
		{
			TextWidget->SetRenderOpacity(0.0f);
			continue;
		}

		if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(TextWidget->Slot))
		{
			const float Rise = EaseOutCubic(Alpha) * Popup.RiseDistance;
			CanvasSlot->SetPosition(ScreenPosition + Popup.ScreenDrift * Alpha + FVector2D(0.0f, -Rise));
		}

		float Scale = Popup.SettleScale;
		if (Alpha <= DamageNumberGrowDurationAlpha)
		{
			Scale = FMath::Lerp(0.72f, Popup.PeakScale, EaseOutCubic(Alpha / DamageNumberGrowDurationAlpha));
		}
		else
		{
			const float SettleAlpha = SmoothTransitionAlpha(
				(Alpha - DamageNumberGrowDurationAlpha) / DamageNumberSettleDurationAlpha);
			Scale = FMath::Lerp(Popup.PeakScale, Popup.SettleScale, SettleAlpha);
		}

		FWidgetTransform Transform;
		Transform.Scale = FVector2D(Scale, Scale);
		if (Popup.DamageNumberType != ETunaSweeperDamageNumberType::Normal)
		{
			const float ShakeStrength = Popup.DamageNumberType == ETunaSweeperDamageNumberType::Headshot ? 3.2f : 1.6f;
			Transform.Angle = FMath::Sin(Popup.ElapsedSeconds * 42.0f) * ShakeStrength * (1.0f - Alpha);
		}
		TextWidget->SetRenderTransform(Transform);

		const float FadeStartAlpha = FMath::Clamp(Popup.FadeStartAlpha, 0.0f, 0.95f);
		const float Opacity = Alpha <= FadeStartAlpha
			? 1.0f
			: 1.0f - FMath::Clamp((Alpha - FadeStartAlpha) / (1.0f - FadeStartAlpha), 0.0f, 1.0f);
		TextWidget->SetRenderOpacity(Opacity);
	}
}

void UTunaSweeperGameHudWidget::RemoveDamageNumberPopupAt(int32 PopupIndex)
{
	if (!DamageNumberPopups.IsValidIndex(PopupIndex))
	{
		return;
	}

	if (UTextBlock* TextWidget = DamageNumberPopups[PopupIndex].TextWidget.Get())
	{
		TextWidget->RemoveFromParent();
	}
	DamageNumberPopups.RemoveAt(PopupIndex);
}

void UTunaSweeperGameHudWidget::ApplyHudModeVisibility()
{
	const bool bUtilityModeOpen = ActiveHudMode != ETunaSweeperHudMode::None;
	const bool bInventoryMode = ActiveHudMode == ETunaSweeperHudMode::Inventory;
	const bool bMapMode = ActiveHudMode == ETunaSweeperHudMode::Map;
	const bool bMemoMode = ActiveHudMode == ETunaSweeperHudMode::Memo;
	const bool bQuestMode = ActiveHudMode == ETunaSweeperHudMode::Quest;
	const UTunaSweeperGameInstance* WorkbenchGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	const bool bWorkbenchPanelOpen =
		bInventoryMode &&
		ExternalPanelWidget &&
		ExternalPanelWidget->GetExternalPanelMode() == ETunaSweeperHudExternalPanelMode::Workbench;
	const bool bCraftWorkbenchPanelOpen =
		bWorkbenchPanelOpen &&
		(!WorkbenchGameInstance || WorkbenchGameInstance->GetActiveWorkbenchMode() == ETunaSweeperWorkbenchMode::Craft);

	if (TopStatusReserveWidget)
	{
		const bool bShowTopStatusReserve = bUtilityModeOpen && !(bQuestMode && bQuestPanelOpenedFromInteraction);
		SetTransitionedWidgetVisibility(
			TopStatusReserveWidget,
			bShowTopStatusReserve ? ESlateVisibility::Visible : ESlateVisibility::Collapsed,
			TopStatusReserveTransitionEdge);
		TopStatusReserveWidget->SetActiveMode(ActiveHudMode);
	}

	if (CenterContentPanel)
	{
		CenterContentPanel->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}

	if (InventoryAreaWidget)
	{
		const bool bShowInventoryArea = bUtilityModeOpen && bInventoryMode && !bCraftWorkbenchPanelOpen;
		SetTransitionedWidgetVisibility(
			InventoryAreaWidget,
			bShowInventoryArea ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed,
			InventoryAreaTransitionEdge);
		InventoryAreaWidget->SetInventoryVisible(bShowInventoryArea);
	}

	EnsureInventoryQuickSlotPanelWidget();
	if (InventoryQuickSlotPanel)
	{
		const bool bExternalPanelOpen =
			ExternalPanelWidget &&
			ExternalPanelWidget->GetExternalPanelMode() != ETunaSweeperHudExternalPanelMode::None;
		const bool bShowInventoryQuickSlotPanel =
			bUtilityModeOpen &&
			bInventoryMode &&
			!bExternalPanelOpen;
		SetTransitionedWidgetVisibility(
			InventoryQuickSlotPanel,
			bShowInventoryQuickSlotPanel
				? ESlateVisibility::SelfHitTestInvisible
				: ESlateVisibility::Collapsed,
			InventoryQuickSlotPanelTransitionEdge);
		if (bShowInventoryQuickSlotPanel)
		{
			RefreshInventoryQuickSlotPanel();
		}
	}

	if (ItemInfoPanelWidget && !bInventoryMode)
	{
		SetTransitionedWidgetVisibility(ItemInfoPanelWidget, ESlateVisibility::Collapsed, ItemInfoPanelTransitionEdge);
	}
	else if (ItemInfoPanelWidget && bCraftWorkbenchPanelOpen)
	{
		SetTransitionedWidgetVisibility(ItemInfoPanelWidget, ESlateVisibility::Collapsed, ItemInfoPanelTransitionEdge);
	}

	if (ShopSellPanelWidget && !bInventoryMode)
	{
		SetShopSellPanelVisible(false);
	}

	EnsureHousingPanelWidget();
	if (HousingPanelWidget)
	{
		const UTunaSweeperHousingSubsystem* HousingSubsystem = GetGameInstance()
			? GetGameInstance()->GetSubsystem<UTunaSweeperHousingSubsystem>()
			: nullptr;
		const bool bShowHousingPanel =
			!bUtilityModeOpen &&
			HousingSubsystem &&
			HousingSubsystem->IsHousingModeOpen() &&
			!IsDialogueSequenceActive();
		SetTransitionedWidgetVisibility(
			HousingPanelWidget,
			bShowHousingPanel ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed,
			ETunaSweeperHudTransitionEdge::Right);
		if (bShowHousingPanel)
		{
			HousingPanelWidget->RefreshHousingPanel();
		}
	}

	EnsureMapPanelWidget();
	if (MapPanelWidget)
	{
		SetTransitionedWidgetVisibility(
			MapPanelWidget,
			bUtilityModeOpen && bMapMode ? ESlateVisibility::Visible : ESlateVisibility::Collapsed,
			MapPanelTransitionEdge);
		if (bUtilityModeOpen && bMapMode)
		{
			MapPanelWidget->RefreshMapView();
			MapPanelWidget->SetUserFocus(GetOwningPlayer());
		}
	}

	EnsureMemoPanelWidget();
	if (MemoPanelWidget)
	{
		SetTransitionedWidgetVisibility(
			MemoPanelWidget,
			bUtilityModeOpen && bMemoMode ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed,
			MemoPanelTransitionEdge);
		if (bUtilityModeOpen && bMemoMode)
		{
			MemoPanelWidget->RefreshMemoView();
		}
	}

	EnsureQuestPanelWidgets();
	if (MenuQuestPanelWidget)
	{
		SetTransitionedWidgetVisibility(
			MenuQuestPanelWidget,
			bUtilityModeOpen && bQuestMode && !bQuestPanelOpenedFromInteraction
				? ESlateVisibility::SelfHitTestInvisible
				: ESlateVisibility::Collapsed,
			QuestPanelTransitionEdge);
		if (bUtilityModeOpen && bQuestMode && !bQuestPanelOpenedFromInteraction)
		{
			MenuQuestPanelWidget->RefreshQuestView();
		}
	}
	if (InteractionQuestPanelWidget)
	{
		SetTransitionedWidgetVisibility(
			InteractionQuestPanelWidget,
			bUtilityModeOpen && bQuestMode && bQuestPanelOpenedFromInteraction
				? ESlateVisibility::SelfHitTestInvisible
				: ESlateVisibility::Collapsed,
			QuestPanelTransitionEdge);
		if (bUtilityModeOpen && bQuestMode && bQuestPanelOpenedFromInteraction)
		{
			InteractionQuestPanelWidget->RefreshQuestView();
		}
	}

	if (ExternalPanelWidget)
	{
		const bool bShowExternalPanel =
			bUtilityModeOpen &&
			bInventoryMode &&
			!bClearExternalPanelModeAfterHide &&
			ExternalPanelWidget->GetExternalPanelMode() != ETunaSweeperHudExternalPanelMode::None;
		SetTransitionedWidgetVisibility(
			ExternalPanelWidget,
			bShowExternalPanel ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed,
			ExternalPanelTransitionEdge);
		if (bClearExternalPanelModeAfterHide &&
			!IsSlateVisibilityShown(ExternalPanelWidget->GetVisibility()) &&
			!HasActiveHudTransition(ExternalPanelWidget))
		{
			ExternalPanelWidget->SetExternalPanelMode(ETunaSweeperHudExternalPanelMode::None);
			bClearExternalPanelModeAfterHide = false;
		}
	}

	if (UnsupportedModePanel)
	{
		SetTransitionedWidgetVisibility(
			UnsupportedModePanel,
			bUtilityModeOpen && !bInventoryMode && !bMapMode && !bMemoMode && !bQuestMode
				? ESlateVisibility::HitTestInvisible
				: ESlateVisibility::Collapsed,
			UnsupportedModePanelTransitionEdge);
	}

	if (UnsupportedModeText)
	{
		UnsupportedModeText->SetText(ResolveUiText(
			GetGameInstance<UTunaSweeperGameInstance>(),
			TEXT("ui.common.unimplemented"),
			TEXT("\uBBF8\uAD6C\uD604")));
	}

	if (ModeTitleText)
	{
		const bool bShowModeTitle = bUtilityModeOpen && (bQuestMode || bMemoMode);
		const UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
		SetTransitionedWidgetVisibility(
			ModeTitleText,
			bShowModeTitle ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed,
			ModeTitleTransitionEdge);
		ModeTitleText->SetText(
			bQuestMode
				? ResolveUiText(TunaGameInstance, TEXT("ui.hud.mode.quest"), TEXT("\uD018\uC2A4\uD2B8"))
				: bMemoMode
					? ResolveUiText(TunaGameInstance, TEXT("ui.hud.mode.memo"), TEXT("\uBA54\uBAA8"))
					: FText::GetEmpty());
	}

	RefreshExtractionProgressWidget();
	RefreshCursorDistanceWidget();
}

void UTunaSweeperGameHudWidget::NormalizeCenterContentPanelLayout()
{
	if (UCanvasPanelSlot* CenterSlot = CenterContentPanel
		? Cast<UCanvasPanelSlot>(CenterContentPanel->Slot)
		: nullptr)
	{
		CenterSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 0.0f));
		CenterSlot->SetAlignment(FVector2D(0.0f, 0.0f));
		CenterSlot->SetOffsets(FMargin(
			UtilityPanelLeftInset,
			UtilityPanelTopOffset,
			UtilityPanelRightInset,
			UtilityPanelHeight));
	}
}

void UTunaSweeperGameHudWidget::CloseLootContainerPanelIfOpen()
{
	if (!ExternalPanelWidget ||
		ExternalPanelWidget->GetExternalPanelMode() == ETunaSweeperHudExternalPanelMode::None)
	{
		return;
	}

	if (ExternalPanelWidget->GetExternalPanelMode() == ETunaSweeperHudExternalPanelMode::LootingBox)
	{
		if (UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>())
		{
			TunaGameInstance->NotifyActiveLootContainerUiClosed();
		}
	}
	else if (ExternalPanelWidget->GetExternalPanelMode() == ETunaSweeperHudExternalPanelMode::Shop)
	{
		if (UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>())
		{
			TunaGameInstance->ClearActiveShop();
		}
		SetShopSellPanelVisible(false);
	}
	else if (ExternalPanelWidget->GetExternalPanelMode() == ETunaSweeperHudExternalPanelMode::Workbench)
	{
		if (UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>())
		{
			TunaGameInstance->ClearActiveWorkbench();
		}
	}

	bClearExternalPanelModeAfterHide = true;
}

void UTunaSweeperGameHudWidget::EnsureExtractionProgressWidget()
{
	if (ExtractionProgressWidget || !WidgetTree)
	{
		return;
	}

	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
	if (!RootCanvas)
	{
		return;
	}

	ExtractionProgressWidget = WidgetTree->ConstructWidget<UTunaSweeperExtractionProgressWidget>(
		UTunaSweeperExtractionProgressWidget::StaticClass(),
		TEXT("ExtractionProgressWidget_Runtime"));
	if (!ExtractionProgressWidget)
	{
		return;
	}

	ExtractionProgressWidget->SetVisibility(ESlateVisibility::Collapsed);

	UCanvasPanelSlot* CanvasSlot = RootCanvas->AddChildToCanvas(ExtractionProgressWidget);
	if (CanvasSlot)
	{
		CanvasSlot->SetAnchors(FAnchors(0.5f, 0.0f, 0.5f, 0.0f));
		CanvasSlot->SetAlignment(FVector2D(0.5f, 0.0f));
		CanvasSlot->SetPosition(FVector2D(0.0f, FMath::Max(0.0f, ExtractionProgressTopOffset)));
		CanvasSlot->SetSize(FVector2D(
			FMath::Max(1.0, ExtractionProgressWidgetSize.X),
			FMath::Max(1.0, ExtractionProgressWidgetSize.Y)));
		CanvasSlot->SetZOrder(45);
	}
}

void UTunaSweeperGameHudWidget::EnsureCursorDistanceWidget()
{
	if (CursorDistancePanel || !WidgetTree)
	{
		return;
	}

	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
	if (!RootCanvas)
	{
		return;
	}

	CursorDistancePanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("CursorDistancePanel"));
	CursorDistanceText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CursorDistanceText"));
	if (!CursorDistancePanel || !CursorDistanceText)
	{
		return;
	}

	CursorDistancePanel->SetVisibility(ESlateVisibility::Collapsed);
	CursorDistancePanel->SetPadding(FMargin(12.0f, 5.0f, 12.0f, 5.0f));
	CursorDistancePanel->SetBrush(MakeHudRoundedBoxBrush(
		FVector2D(64.0f, 30.0f),
		FLinearColor(0.0f, 0.0f, 0.0f, 0.70f),
		8.0f,
		FLinearColor(0.0f, 0.0f, 0.0f, 0.0f),
		0.0f));
	CursorDistancePanel->SetContent(CursorDistanceText);

	CursorDistanceText->SetText(FText::FromString(TEXT("0M")));
	CursorDistanceText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	CursorDistanceText->SetJustification(ETextJustify::Right);
	CursorDistanceText->SetMinDesiredWidth(CursorDistanceMinTextWidth);
	TunaSweeperUIFont::ApplyFont(CursorDistanceText, 18.0f, ETunaSweeperUIFontWeight::Bold);

	UCanvasPanelSlot* CanvasSlot = RootCanvas->AddChildToCanvas(CursorDistancePanel);
	if (CanvasSlot)
	{
		CanvasSlot->SetAnchors(FAnchors(1.0f, 1.0f, 1.0f, 1.0f));
		CanvasSlot->SetAlignment(FVector2D(1.0f, 1.0f));
		CanvasSlot->SetPosition(FVector2D(-CursorDistanceRightOffset, -CursorDistanceBottomOffset));
		CanvasSlot->SetAutoSize(true);
		CanvasSlot->SetZOrder(8);
	}
}

void UTunaSweeperGameHudWidget::EnsureDebuffBarWidget()
{
	if (!WidgetTree)
	{
		return;
	}

	auto ConfigureDebuffBarCanvasSlot = [](UCanvasPanelSlot* CanvasSlot)
	{
		if (!CanvasSlot)
		{
			return;
		}

		CanvasSlot->SetAnchors(FAnchors(0.0f, 1.0f, 0.0f, 1.0f));
		CanvasSlot->SetAlignment(FVector2D(0.0f, 1.0f));
		CanvasSlot->SetPosition(FVector2D(DebuffBarLeftOffset, -DebuffBarBottomOffset));
		CanvasSlot->SetAutoSize(true);
		CanvasSlot->SetZOrder(36);
	};

	if (DebuffBarWidget)
	{
		ConfigureDebuffBarCanvasSlot(Cast<UCanvasPanelSlot>(DebuffBarWidget->Slot));
		return;
	}

	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
	if (!RootCanvas)
	{
		return;
	}

	TSubclassOf<UTunaSweeperHudDebuffBarWidget> DebuffBarWidgetClass =
		LoadClass<UTunaSweeperHudDebuffBarWidget>(
			nullptr,
			TEXT("/Game/UI/WBP_HudDebuffBar.WBP_HudDebuffBar_C"));
	if (!DebuffBarWidgetClass)
	{
		DebuffBarWidgetClass = UTunaSweeperHudDebuffBarWidget::StaticClass();
	}

	DebuffBarWidget = CreateWidget<UTunaSweeperHudDebuffBarWidget>(
		GetOwningPlayer(),
		DebuffBarWidgetClass);
	if (!DebuffBarWidget)
	{
		return;
	}

	DebuffBarWidget->SetVisibility(ESlateVisibility::Collapsed);

	UCanvasPanelSlot* CanvasSlot = RootCanvas->AddChildToCanvas(DebuffBarWidget);
	ConfigureDebuffBarCanvasSlot(CanvasSlot);
}

void UTunaSweeperGameHudWidget::EnsureInventoryQuickSlotPanelWidget()
{
	if (InventoryQuickSlotPanel || !WidgetTree)
	{
		return;
	}

	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
	if (!RootCanvas)
	{
		return;
	}

	TSubclassOf<UTunaSweeperItemThumbnailSlotWidget> EntryWidgetClass =
		LoadClass<UTunaSweeperItemThumbnailSlotWidget>(
			nullptr,
			TEXT("/Game/UI/WBP_ItemThumbnailSlot.WBP_ItemThumbnailSlot_C"));
	if (!EntryWidgetClass)
	{
		return;
	}

	InventoryQuickSlotPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("InventoryQuickSlotPanel"));
	UVerticalBox* PanelStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("InventoryQuickSlotStack"));
	UTextBlock* GuideText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("InventoryQuickSlotGuideText"));
	InventoryQuickSlotRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("InventoryQuickSlotRow"));
	if (!InventoryQuickSlotPanel || !PanelStack || !GuideText || !InventoryQuickSlotRow)
	{
		return;
	}

	InventoryQuickSlotPanel->SetVisibility(ESlateVisibility::Collapsed);
	InventoryQuickSlotPanel->SetPadding(FMargin(18.0f, 12.0f, 18.0f, 12.0f));
	InventoryQuickSlotPanel->SetBrush(MakeHudRoundedBoxBrush(
		FVector2D(InventoryQuickSlotPanelWidth, InventoryQuickSlotPanelHeight),
		FLinearColor(0.015f, 0.018f, 0.018f, 0.68f),
		8.0f,
		FLinearColor(0.48f, 0.54f, 0.52f, 0.48f),
		1.0f));
	InventoryQuickSlotPanel->SetContent(PanelStack);

	InventoryQuickSlotGuideText = GuideText;
	GuideText->SetText(ResolveUiText(
		GetGameInstance<UTunaSweeperGameInstance>(),
		TEXT("ui.hud.quick_slot_guide"),
		TEXT("\uC544\uC774\uD15C\uC744 \uC2AC\uB86F\uC73C\uB85C \uB4DC\uB798\uADF8\uD558\uC5EC \uD035\uC2AC\uB86F\uC744 \uC124\uC815\uD558\uC138\uC694")));
	GuideText->SetColorAndOpacity(FSlateColor(FLinearColor(0.94f, 0.94f, 0.90f, 1.0f)));
	GuideText->SetJustification(ETextJustify::Center);
	TunaSweeperUIFont::ApplyFont(GuideText, 24, ETunaSweeperUIFontWeight::Bold);
	UVerticalBoxSlot* GuideSlot = PanelStack->AddChildToVerticalBox(GuideText);
	if (GuideSlot)
	{
		GuideSlot->SetHorizontalAlignment(HAlign_Fill);
		GuideSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
	}

	UVerticalBoxSlot* RowStackSlot = PanelStack->AddChildToVerticalBox(InventoryQuickSlotRow);
	if (RowStackSlot)
	{
		RowStackSlot->SetHorizontalAlignment(HAlign_Center);
	}

	InventoryQuickSlotWidgets.Reset();
	for (int32 SlotNumber = InventoryQuickSlotFirstNumber; SlotNumber <= InventoryQuickSlotLastNumber; ++SlotNumber)
	{
		UVerticalBox* SlotStack = WidgetTree->ConstructWidget<UVerticalBox>(
			UVerticalBox::StaticClass(),
			FName(*FString::Printf(TEXT("InventoryQuickSlot%dStack"), SlotNumber)));
		USizeBox* SlotSizeBox = WidgetTree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass(),
			FName(*FString::Printf(TEXT("InventoryQuickSlot%dSizeBox"), SlotNumber)));
		UTunaSweeperItemThumbnailSlotWidget* SlotWidget = WidgetTree->ConstructWidget<UTunaSweeperItemThumbnailSlotWidget>(
			EntryWidgetClass,
			FName(*FString::Printf(TEXT("InventoryQuickSlot%dWidget"), SlotNumber)));
		USizeBox* KeySizeBox = WidgetTree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass(),
			FName(*FString::Printf(TEXT("InventoryQuickSlot%dKeySizeBox"), SlotNumber)));
		UBorder* KeyBackground = WidgetTree->ConstructWidget<UBorder>(
			UBorder::StaticClass(),
			FName(*FString::Printf(TEXT("InventoryQuickSlot%dKeyBackground"), SlotNumber)));
		UTextBlock* KeyText = WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(),
			FName(*FString::Printf(TEXT("InventoryQuickSlot%dKeyText"), SlotNumber)));
		if (!SlotStack || !SlotSizeBox || !SlotWidget || !KeySizeBox || !KeyBackground || !KeyText)
		{
			continue;
		}

		SlotSizeBox->SetWidthOverride(InventoryQuickSlotTileSize);
		SlotSizeBox->SetHeightOverride(InventoryQuickSlotTileSize);
		SlotWidget->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		SlotWidget->SetRenderScale(FVector2D(InventoryQuickSlotTileScale, InventoryQuickSlotTileScale));
		SlotSizeBox->SetContent(SlotWidget);
		UVerticalBoxSlot* SlotWidgetStackSlot = SlotStack->AddChildToVerticalBox(SlotSizeBox);
		if (SlotWidgetStackSlot)
		{
			SlotWidgetStackSlot->SetHorizontalAlignment(HAlign_Center);
		}

		KeySizeBox->SetWidthOverride(34.0f);
		KeySizeBox->SetHeightOverride(28.0f);
		KeyBackground->SetPadding(FMargin(8.0f, 1.0f));
		KeyBackground->SetBrush(MakeHudRoundedBoxBrush(
			FVector2D(34.0f, 28.0f),
			FLinearColor(0.96f, 0.96f, 0.96f, 0.98f),
			5.0f,
			FLinearColor(1.0f, 1.0f, 1.0f, 0.98f),
			0.0f));
		KeyText->SetText(FText::AsNumber(SlotNumber));
		KeyText->SetColorAndOpacity(FSlateColor(FLinearColor(0.02f, 0.024f, 0.028f, 1.0f)));
		KeyText->SetJustification(ETextJustify::Center);
		TunaSweeperUIFont::ApplyFont(KeyText, 17, ETunaSweeperUIFontWeight::Bold);
		KeyBackground->SetContent(KeyText);
		KeySizeBox->SetContent(KeyBackground);
		UVerticalBoxSlot* KeyStackSlot = SlotStack->AddChildToVerticalBox(KeySizeBox);
		if (KeyStackSlot)
		{
			KeyStackSlot->SetHorizontalAlignment(HAlign_Center);
			KeyStackSlot->SetPadding(FMargin(0.0f, 3.0f, 0.0f, 0.0f));
		}

		UHorizontalBoxSlot* RowSlot = InventoryQuickSlotRow->AddChildToHorizontalBox(SlotStack);
		if (RowSlot)
		{
			RowSlot->SetPadding(FMargin(SlotNumber == InventoryQuickSlotFirstNumber ? 0.0f : 12.0f, 0.0f, 0.0f, 0.0f));
			RowSlot->SetVerticalAlignment(VAlign_Bottom);
		}

		InventoryQuickSlotWidgets.Add(SlotWidget);
	}

	UCanvasPanelSlot* CanvasSlot = RootCanvas->AddChildToCanvas(InventoryQuickSlotPanel);
	if (CanvasSlot)
	{
		CanvasSlot->SetAnchors(FAnchors(0.5f, 1.0f, 0.5f, 1.0f));
		CanvasSlot->SetAlignment(FVector2D(0.5f, 1.0f));
		CanvasSlot->SetPosition(FVector2D(0.0f, -34.0f));
		CanvasSlot->SetSize(FVector2D(InventoryQuickSlotPanelWidth, InventoryQuickSlotPanelHeight));
		CanvasSlot->SetZOrder(35);
	}
}

void UTunaSweeperGameHudWidget::EnsureHousingPanelWidget()
{
	if (HousingPanelWidget || !WidgetTree)
	{
		return;
	}

	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
	if (!RootCanvas)
	{
		return;
	}

	HousingPanelWidget = CreateWidget<UTunaSweeperHousingPanelWidget>(
		GetOwningPlayer(),
		UTunaSweeperHousingPanelWidget::StaticClass());
	if (!HousingPanelWidget)
	{
		return;
	}

	HousingPanelWidget->SetVisibility(ESlateVisibility::Collapsed);
	UCanvasPanelSlot* CanvasSlot = RootCanvas->AddChildToCanvas(HousingPanelWidget);
	if (CanvasSlot)
	{
		constexpr float PanelWidth = 360.0f;
		constexpr float EdgeMargin = 24.0f;
		CanvasSlot->SetAnchors(FAnchors(1.0f, 0.0f, 1.0f, 1.0f));
		CanvasSlot->SetAlignment(FVector2D(0.0f, 0.0f));
		CanvasSlot->SetOffsets(FMargin(-(PanelWidth + EdgeMargin), EdgeMargin, PanelWidth, EdgeMargin));
		CanvasSlot->SetZOrder(42);
	}
}

void UTunaSweeperGameHudWidget::EnsureMapPanelWidget()
{
	if (MapPanelWidget || !WidgetTree)
	{
		return;
	}

	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
	if (!RootCanvas)
	{
		return;
	}

	MapPanelWidget = CreateWidget<UTunaSweeperMapWidget>(
		GetOwningPlayer(),
		UTunaSweeperMapWidget::StaticClass());
	if (!MapPanelWidget)
	{
		return;
	}

	MapPanelWidget->SetVisibility(ESlateVisibility::Collapsed);
	UCanvasPanelSlot* CanvasSlot = RootCanvas->AddChildToCanvas(MapPanelWidget);
	if (CanvasSlot)
	{
		CanvasSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		CanvasSlot->SetOffsets(FMargin(0.0f));
		CanvasSlot->SetAlignment(FVector2D(0.0f, 0.0f));
		CanvasSlot->SetZOrder(-5);
	}
}

void UTunaSweeperGameHudWidget::EnsureMemoPanelWidget()
{
	if (MemoPanelWidget || !WidgetTree)
	{
		return;
	}

	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
	if (!RootCanvas)
	{
		return;
	}

	MemoPanelWidget = CreateWidget<UTunaSweeperMemoWidget>(
		GetOwningPlayer(),
		UTunaSweeperMemoWidget::StaticClass());
	if (!MemoPanelWidget)
	{
		return;
	}

	MemoPanelWidget->SetVisibility(ESlateVisibility::Collapsed);
	UCanvasPanelSlot* CanvasSlot = RootCanvas->AddChildToCanvas(MemoPanelWidget);
	if (CanvasSlot)
	{
		CanvasSlot->SetAnchors(FAnchors(0.5f, 0.5f));
		CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		CanvasSlot->SetPosition(FVector2D(0.0f, 34.0f));
		CanvasSlot->SetSize(FVector2D(1220.0f, 672.0f));
		CanvasSlot->SetZOrder(20);
	}
}

void UTunaSweeperGameHudWidget::EnsureQuestPanelWidgets()
{
	if ((MenuQuestPanelWidget && InteractionQuestPanelWidget) || !WidgetTree)
	{
		return;
	}

	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
	if (!RootCanvas)
	{
		return;
	}

	auto ResolveQuestWidgetClass = [](const TCHAR* WidgetClassPath, UClass* FallbackClass)
	{
		TSoftClassPtr<UTunaSweeperQuestWidget> SoftWidgetClass{ FSoftObjectPath(WidgetClassPath) };
		TSubclassOf<UTunaSweeperQuestWidget> LoadedClass = SoftWidgetClass.LoadSynchronous();
		return LoadedClass ? LoadedClass : TSubclassOf<UTunaSweeperQuestWidget>(FallbackClass);
	};

	auto AddQuestWidgetToCanvas = [this, RootCanvas](
		TObjectPtr<UTunaSweeperQuestWidget>& OutWidget,
		TSubclassOf<UTunaSweeperQuestWidget> WidgetClass,
		const FMargin& Margins)
	{
		if (OutWidget || !WidgetClass)
		{
			return;
		}

		OutWidget = CreateWidget<UTunaSweeperQuestWidget>(GetOwningPlayer(), WidgetClass);
		if (!OutWidget)
		{
			return;
		}

		OutWidget->SetVisibility(ESlateVisibility::Collapsed);
		UCanvasPanelSlot* CanvasSlot = RootCanvas->AddChildToCanvas(OutWidget);
		if (CanvasSlot)
		{
			CanvasSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
			CanvasSlot->SetAlignment(FVector2D::ZeroVector);
			CanvasSlot->SetOffsets(Margins);
			CanvasSlot->SetZOrder(20);
		}
	};

	AddQuestWidgetToCanvas(
		MenuQuestPanelWidget,
		ResolveQuestWidgetClass(
			TEXT("/Game/UI/WBP_QuestMenu.WBP_QuestMenu_C"),
			UTunaSweeperMenuQuestWidget::StaticClass()),
		FMargin(
			QuestMenuHorizontalMargin,
			QuestMenuTopMargin,
			QuestMenuHorizontalMargin,
			QuestMenuBottomMargin));
	AddQuestWidgetToCanvas(
		InteractionQuestPanelWidget,
		ResolveQuestWidgetClass(
			TEXT("/Game/UI/WBP_QuestInteraction.WBP_QuestInteraction_C"),
			UTunaSweeperInteractionQuestWidget::StaticClass()),
		FMargin(
			QuestInteractionHorizontalMargin,
			QuestInteractionTopMargin,
			QuestInteractionHorizontalMargin,
			QuestInteractionBottomMargin));
}

void UTunaSweeperGameHudWidget::EnsureShopSellPanelWidget()
{
	if (ShopSellPanelWidget || !WidgetTree)
	{
		return;
	}

	UCanvasPanel* ParentCanvas = ItemInfoPanelWidget
		? Cast<UCanvasPanel>(ItemInfoPanelWidget->GetParent())
		: nullptr;
	if (!ParentCanvas)
	{
		ParentCanvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
	}
	if (!ParentCanvas)
	{
		return;
	}

	ShopSellPanelWidget = CreateWidget<UTunaSweeperShopSellPanelWidget>(
		GetOwningPlayer(),
		UTunaSweeperShopSellPanelWidget::StaticClass());
	if (!ShopSellPanelWidget)
	{
		return;
	}

	ShopSellPanelWidget->SetVisibility(ESlateVisibility::Collapsed);
	UCanvasPanelSlot* CanvasSlot = ParentCanvas->AddChildToCanvas(ShopSellPanelWidget);
	if (!CanvasSlot)
	{
		return;
	}

	if (const UCanvasPanelSlot* ItemInfoCanvasSlot = ItemInfoPanelWidget
		? Cast<UCanvasPanelSlot>(ItemInfoPanelWidget->Slot)
		: nullptr)
	{
		CanvasSlot->SetAnchors(ItemInfoCanvasSlot->GetAnchors());
		CanvasSlot->SetAlignment(ItemInfoCanvasSlot->GetAlignment());
		CanvasSlot->SetPosition(ItemInfoCanvasSlot->GetPosition());
		CanvasSlot->SetSize(FVector2D(ShopSellPanelWidth, ShopSellPanelHeight));
		CanvasSlot->SetZOrder(ItemInfoCanvasSlot->GetZOrder() + 1);
		return;
	}

	CanvasSlot->SetAnchors(FAnchors(0.5f, 0.5f));
	CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
	CanvasSlot->SetPosition(FVector2D(0.0f, -24.0f));
	CanvasSlot->SetSize(FVector2D(ShopSellPanelWidth, ShopSellPanelHeight));
	CanvasSlot->SetZOrder(31);
}

void UTunaSweeperGameHudWidget::SetShopSellPanelVisible(bool bVisible)
{
	EnsureShopSellPanelWidget();

	if (!ShopSellPanelWidget)
	{
		return;
	}

	if (bVisible)
	{
		ActiveHudMode = ETunaSweeperHudMode::Inventory;
		ShopSellPanelWidget->RefreshSelectedItem();
	}

	SetTransitionedWidgetVisibilityFromTranslation(
		ShopSellPanelWidget,
		bVisible && ActiveHudMode == ETunaSweeperHudMode::Inventory
			? ESlateVisibility::SelfHitTestInvisible
			: ESlateVisibility::Collapsed,
		FVector2D(0.0f, ShopSellPanelTransitionOffsetY));
}

void UTunaSweeperGameHudWidget::RefreshBottomStatusFromGameInstance()
{
	if (!BottomStatusWidget && !InventoryAreaWidget)
	{
		return;
	}

	const UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	FTunaSweeperPlayerHudState HudState = TunaGameInstance ? TunaGameInstance->PlayerHudState : FTunaSweeperPlayerHudState();

	if (const APlayerController* PlayerController = GetOwningPlayer())
	{
		const APawn* Pawn = PlayerController->GetPawn();
		const UTunaSweeperVitalsComponent* VitalsComponent = nullptr;
		if (const ATunaSweeperTopDownCharacter* TunaCharacter = Cast<ATunaSweeperTopDownCharacter>(Pawn))
		{
			VitalsComponent = TunaCharacter->GetVitalsComponent();
		}
		else if (Pawn)
		{
			VitalsComponent = Pawn->FindComponentByClass<UTunaSweeperVitalsComponent>();
		}

		if (VitalsComponent)
		{
			const FTunaSweeperVitalsState& VitalsState = VitalsComponent->GetVitalsState();
			HudState.Health = VitalsState.Health;
			HudState.MaxHealth = VitalsState.MaxHealth;
			HudState.Food = VitalsState.Food;
			HudState.MaxFood = VitalsState.MaxFood;
			HudState.Hydration = VitalsState.Hydration;
			HudState.MaxHydration = VitalsState.MaxHydration;
		}
	}

	if (BottomStatusWidget)
	{
		BottomStatusWidget->SetHudState(HudState);
	}

	if (InventoryAreaWidget)
	{
		InventoryAreaWidget->SetHudState(HudState);
	}
}

void UTunaSweeperGameHudWidget::RefreshDebuffBarFromPlayer()
{
	EnsureDebuffBarWidget();
	bDebuffBarHasActiveDebuffs = false;
	if (!DebuffBarWidget)
	{
		return;
	}

	TArray<FTunaSweeperActiveDebuffState> ActiveDebuffs;
	const APlayerController* PlayerController = GetOwningPlayer();
	const APawn* PlayerPawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	const UTunaSweeperDebuffComponent* DebuffComponent = PlayerPawn
		? PlayerPawn->FindComponentByClass<UTunaSweeperDebuffComponent>()
		: nullptr;
	if (DebuffComponent)
	{
		ActiveDebuffs = DebuffComponent->GetActiveDebuffs();
		bDebuffBarHasActiveDebuffs = ActiveDebuffs.Num() > 0;
	}

	DebuffBarWidget->SetActiveDebuffs(ActiveDebuffs);
}

void UTunaSweeperGameHudWidget::RefreshQuickSlotsFromGameState()
{
	if (!QuickSlotBarWidget)
	{
		return;
	}

	UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	UTunaSweeperItemDataSubsystem* ItemDataSubsystem = TunaGameInstance
		? TunaGameInstance->GetSubsystem<UTunaSweeperItemDataSubsystem>()
		: nullptr;
	const ETunaSweeperItemTextLanguage Language = TunaGameInstance
		? TunaGameInstance->GetCurrentTextLanguage()
		: ETunaSweeperItemTextLanguage::English;

	int32 SelectedSlotNumber = 0;
	bool bMeleeQuickSlotSelected = false;
	if (const APlayerController* PlayerController = GetOwningPlayer())
	{
		if (const ATunaSweeperTopDownCharacter* TunaCharacter = Cast<ATunaSweeperTopDownCharacter>(PlayerController->GetPawn()))
		{
			SelectedSlotNumber = TunaCharacter->GetSelectedWeaponSlotNumber();
			bMeleeQuickSlotSelected = TunaCharacter->IsMeleeWeaponSelected();
		}
	}
	if (bMeleeQuickSlotSelected)
	{
		QuickSlotBarWidget->SetSelectedMeleeQuickSlot();
	}
	else
	{
		QuickSlotBarWidget->SetSelectedQuickSlot(SelectedSlotNumber);
	}

	for (int32 SlotNumber = 1; SlotNumber <= 2; ++SlotNumber)
	{
		FTunaSweeperItemInstance WeaponInstance;
		FTunaSweeperItemDefinition WeaponDefinition;
		if (!TunaGameInstance ||
			!TunaGameInstance->TryGetEquipmentWeaponSlotItem(SlotNumber, WeaponInstance, WeaponDefinition))
		{
			QuickSlotBarWidget->ClearQuickSlotIcon(SlotNumber);
			QuickSlotBarWidget->SetWeaponAmmoTypeText(SlotNumber, FText::GetEmpty(), false);
			QuickSlotBarWidget->SetWeaponAmmoText(SlotNumber, 0, 0, false);
			continue;
		}

		UTexture2D* IconTexture = nullptr;
		if (ItemDataSubsystem)
		{
			const FString IconObjectPath = ItemDataSubsystem->BuildItemIconObjectPath(WeaponDefinition);
			if (!IconObjectPath.IsEmpty())
			{
				IconTexture = LoadObject<UTexture2D>(nullptr, *IconObjectPath);
			}
		}
		QuickSlotBarWidget->SetQuickSlotIcon(SlotNumber, IconTexture);

		FText AmmoTypeText = ResolveUiText(TunaGameInstance, TEXT("ui.hud.ammo_unset"), TEXT("\uD0C4\uC57D \uBBF8\uC9C0\uC815"));
		const int32 AmmoItemId = TunaGameInstance->GetWeaponSelectedAmmoItemId(SlotNumber);
		if (AmmoItemId != INDEX_NONE)
		{
			AmmoTypeText = FText::Format(
				ResolveUiText(TunaGameInstance, TEXT("ui.common.item_fallback"), TEXT("Item {0}")),
				FText::AsNumber(AmmoItemId));
			if (ItemDataSubsystem)
			{
				ItemDataSubsystem->TryGetItemNameText(AmmoItemId, Language, AmmoTypeText);
			}
		}
		QuickSlotBarWidget->SetWeaponAmmoTypeText(SlotNumber, AmmoTypeText, true);
		QuickSlotBarWidget->SetWeaponAmmoText(
			SlotNumber,
			TunaGameInstance->GetWeaponLoadedAmmoCount(SlotNumber),
			TunaGameInstance->GetWeaponInventoryAmmoCount(SlotNumber),
			true);
	}

	FTunaSweeperItemInstance MeleeInstance;
	FTunaSweeperItemDefinition MeleeDefinition;
	if (!TunaGameInstance ||
		!TunaGameInstance->TryGetEquipmentMeleeSlotItem(MeleeInstance, MeleeDefinition) ||
		!ItemDataSubsystem)
	{
		QuickSlotBarWidget->ClearMeleeQuickSlotIcon();
	}
	else
	{
		UTexture2D* IconTexture = nullptr;
		const FString IconObjectPath = ItemDataSubsystem->BuildItemIconObjectPath(MeleeDefinition);
		if (!IconObjectPath.IsEmpty())
		{
			IconTexture = LoadObject<UTexture2D>(nullptr, *IconObjectPath);
		}
		QuickSlotBarWidget->SetMeleeQuickSlotIcon(IconTexture);
	}

	static const TArray<FTunaSweeperInventorySlot> EmptyQuickSlots;
	const TArray<FTunaSweeperInventorySlot>& UsableQuickSlots = TunaGameInstance
		? TunaGameInstance->GetUsableQuickSlots()
		: EmptyQuickSlots;
	for (int32 SlotNumber = InventoryQuickSlotFirstNumber; SlotNumber <= InventoryQuickSlotLastNumber; ++SlotNumber)
	{
		const int32 SlotIndex = SlotNumber - InventoryQuickSlotFirstNumber;
		FTunaSweeperItemInstance ItemInstance;
		FTunaSweeperItemDefinition ItemDefinition;
		if (!UsableQuickSlots.IsValidIndex(SlotIndex) ||
			!TunaGameInstance ||
			!TunaGameInstance->TryGetItemInstance(UsableQuickSlots[SlotIndex].ItemUid, ItemInstance) ||
			!ItemDataSubsystem ||
			!ItemDataSubsystem->TryGetItemDefinition(ItemInstance.ItemId, ItemDefinition))
		{
			QuickSlotBarWidget->ClearQuickSlotIcon(SlotNumber);
			QuickSlotBarWidget->SetWeaponAmmoTypeText(SlotNumber, FText::GetEmpty(), false);
			QuickSlotBarWidget->SetWeaponAmmoText(SlotNumber, 0, 0, false);
			continue;
		}

		UTexture2D* IconTexture = nullptr;
		const FString IconObjectPath = ItemDataSubsystem->BuildItemIconObjectPath(ItemDefinition);
		if (!IconObjectPath.IsEmpty())
		{
			IconTexture = LoadObject<UTexture2D>(nullptr, *IconObjectPath);
		}

		QuickSlotBarWidget->SetQuickSlotIcon(SlotNumber, IconTexture);
		QuickSlotBarWidget->SetWeaponAmmoTypeText(SlotNumber, FText::GetEmpty(), false);
		QuickSlotBarWidget->SetWeaponAmmoText(SlotNumber, 0, 0, false);
	}
}

void UTunaSweeperGameHudWidget::RefreshInventoryQuickSlotPanel()
{
	if (InventoryQuickSlotWidgets.Num() <= 0)
	{
		return;
	}

	UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	UTunaSweeperItemDataSubsystem* ItemDataSubsystem = TunaGameInstance
		? TunaGameInstance->GetSubsystem<UTunaSweeperItemDataSubsystem>()
		: nullptr;
	static const TArray<FTunaSweeperInventorySlot> EmptyQuickSlots;
	const TArray<FTunaSweeperInventorySlot>& UsableQuickSlots = TunaGameInstance
		? TunaGameInstance->GetUsableQuickSlots()
		: EmptyQuickSlots;

	for (int32 SlotIndex = 0; SlotIndex < InventoryQuickSlotWidgets.Num(); ++SlotIndex)
	{
		if (!InventoryQuickSlotWidgets[SlotIndex])
		{
			continue;
		}

		const FTunaSweeperInventorySlot& QuickSlot = UsableQuickSlots.IsValidIndex(SlotIndex)
			? UsableQuickSlots[SlotIndex]
			: FTunaSweeperInventorySlot();
		InventoryQuickSlotWidgets[SlotIndex]->SetTileData(BuildQuickSlotTileData(
			TunaGameInstance,
			ItemDataSubsystem,
			QuickSlot,
			SlotIndex));
	}
}

void UTunaSweeperGameHudWidget::RefreshLocalizedTexts()
{
	UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	if (InventoryQuickSlotGuideText)
	{
		InventoryQuickSlotGuideText->SetText(ResolveUiText(
			TunaGameInstance,
			TEXT("ui.hud.quick_slot_guide"),
			TEXT("\uC544\uC774\uD15C\uC744 \uC2AC\uB86F\uC73C\uB85C \uB4DC\uB798\uADF8\uD558\uC5EC \uD035\uC2AC\uB86F\uC744 \uC124\uC815\uD558\uC138\uC694")));
	}

	if (UnsupportedModeText)
	{
		UnsupportedModeText->SetText(ResolveUiText(
			TunaGameInstance,
			TEXT("ui.common.unimplemented"),
			TEXT("\uBBF8\uAD6C\uD604")));
	}
}

void UTunaSweeperGameHudWidget::RefreshCancelableActionWidgets(const FGeometry* GeometryForPlacement)
{
	CacheAmmoCancelableActionWidgets();

	const bool bDialogueActive = IsDialogueSequenceActive();
	const bool bHousingModeActive = IsHousingModeActive();
	ATunaSweeperTopDownCharacter* TunaCharacter = nullptr;
	if (const APlayerController* PlayerController = GetOwningPlayer())
	{
		TunaCharacter = Cast<ATunaSweeperTopDownCharacter>(PlayerController->GetPawn());
	}

	const bool bShowCancelableAction = !bDialogueActive && !bHousingModeActive && TunaCharacter && TunaCharacter->IsCancelableActionActive();
	const float CancelableActionProgress = bShowCancelableAction ? TunaCharacter->GetCancelableActionProgress() : 0.0f;
	const bool bUseCrosshairReloadGauge = bShowCancelableAction && IsReloadGaugeReplacingCrosshair(TunaCharacter);
	if (GeometryForPlacement)
	{
		UpdateCenterCancelableActionGaugePlacement(*GeometryForPlacement, bUseCrosshairReloadGauge);
	}
	UpdateMouseCursorForReloadGauge(bUseCrosshairReloadGauge);

	bool bShowReloadPrompt = false;
	if (!bDialogueActive && !bHousingModeActive && !bShowCancelableAction && TunaCharacter && !TunaCharacter->IsAmmoSelectionOpen() && !IsInventoryUiOpen())
	{
		if (UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>())
		{
			const int32 SelectedWeaponSlotNumber = TunaCharacter->GetSelectedWeaponSlotNumber();
			bShowReloadPrompt =
				SelectedWeaponSlotNumber > 0 &&
				TunaGameInstance->IsEquipmentWeaponSlotOccupied(SelectedWeaponSlotNumber) &&
				TunaGameInstance->GetWeaponMagazineCapacity(SelectedWeaponSlotNumber) > 0 &&
				TunaGameInstance->GetWeaponLoadedAmmoCount(SelectedWeaponSlotNumber) <= 0 &&
				TunaGameInstance->GetWeaponSelectedAmmoItemId(SelectedWeaponSlotNumber) != INDEX_NONE &&
				TunaGameInstance->GetWeaponInventoryAmmoCount(SelectedWeaponSlotNumber) > 0;
		}
	}

	if (QuickSlotBarWidget)
	{
		QuickSlotBarWidget->SetCancelableActionProgress(CancelableActionProgress, bShowCancelableAction);
	}

	if (CenterCancelableActionGaugeRoot)
	{
		CenterCancelableActionGaugeRoot->SetVisibility(bShowCancelableAction ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	if (CenterReloadPromptRoot)
	{
		CenterReloadPromptRoot->SetVisibility(bShowReloadPrompt ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	if (CenterCancelableActionPercentText)
	{
		CenterCancelableActionPercentText->SetText(
			bShowCancelableAction
				? FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(CancelableActionProgress * 100.0f)))
				: FText::GetEmpty());
	}

	if (CenterCancelableActionRingWidget)
	{
		CenterCancelableActionRingWidget->SetCancelableActionProgress(CancelableActionProgress, bShowCancelableAction);
	}

	const int32 FilledSegmentCount = FMath::CeilToInt(CancelableActionProgress * CenterCancelableActionSegments.Num());
	for (int32 SegmentIndex = 0; SegmentIndex < CenterCancelableActionSegments.Num(); ++SegmentIndex)
	{
		if (CenterCancelableActionSegments[SegmentIndex])
		{
			CenterCancelableActionSegments[SegmentIndex]->SetRenderOpacity(
				bShowCancelableAction && SegmentIndex < FilledSegmentCount ? 1.0f : 0.18f);
		}
	}

	TArray<FText> AmmoOptionTexts;
	int32 FocusedOptionIndex = INDEX_NONE;
	BuildAmmoSelectorOptionTexts(AmmoOptionTexts, FocusedOptionIndex);
	if (QuickSlotBarWidget)
	{
		const int32 SelectedWeaponSlotNumber = TunaCharacter ? TunaCharacter->GetSelectedWeaponSlotNumber() : 0;
		const bool bAmmoSelectionOpen = TunaCharacter && TunaCharacter->IsAmmoSelectionOpen();
		if (bAmmoSelectionOpen)
		{
			QuickSlotBarWidget->SetAmmoSelectorOptions(
				AmmoOptionTexts,
				FocusedOptionIndex,
				SelectedWeaponSlotNumber,
				true);
		}
		else
		{
			QuickSlotBarWidget->SetAmmoSelectorPrompt(SelectedWeaponSlotNumber, FText::GetEmpty(), false);
		}
	}
}

void UTunaSweeperGameHudWidget::CacheAmmoCancelableActionWidgets()
{
	if (!WidgetTree)
	{
		return;
	}

	CenterCancelableActionGaugeRoot = WidgetTree->FindWidget(FName(TEXT("CenterCancelableActionGaugeRoot")));
	if (!CenterCancelableActionGaugeRoot)
	{
		CenterCancelableActionGaugeRoot = WidgetTree->FindWidget(FName(TEXT("CenterReloadGaugeRoot")));
	}
	CenterCancelableActionRingWidget = Cast<UTunaSweeperReloadRingWidget>(WidgetTree->FindWidget(FName(TEXT("CenterCancelableActionRingWidget"))));
	if (!CenterCancelableActionRingWidget)
	{
		CenterCancelableActionRingWidget = Cast<UTunaSweeperReloadRingWidget>(WidgetTree->FindWidget(FName(TEXT("CenterReloadRingWidget"))));
	}
	CenterReloadPromptRoot = WidgetTree->FindWidget(FName(TEXT("CenterReloadPromptRoot")));
	CenterCancelableActionPercentText = Cast<UTextBlock>(WidgetTree->FindWidget(FName(TEXT("CenterCancelableActionPercentText"))));
	if (!CenterCancelableActionPercentText)
	{
		CenterCancelableActionPercentText = Cast<UTextBlock>(WidgetTree->FindWidget(FName(TEXT("CenterReloadPercentText"))));
	}
	CenterCancelableActionSegments.SetNum(12);
	for (int32 SegmentNumber = 1; SegmentNumber <= CenterCancelableActionSegments.Num(); ++SegmentNumber)
	{
		UBorder* Segment = Cast<UBorder>(WidgetTree->FindWidget(
			FName(*FString::Printf(TEXT("CenterCancelableActionSegment%02d"), SegmentNumber))));
		if (!Segment)
		{
			Segment = Cast<UBorder>(WidgetTree->FindWidget(
				FName(*FString::Printf(TEXT("CenterReloadSegment%02d"), SegmentNumber))));
		}
		CenterCancelableActionSegments[SegmentNumber - 1] = Segment;
		if (!Segment)
		{
			continue;
		}

		const float AngleDegrees = (SegmentNumber - 1) * 30.0f - 90.0f;
		const float AngleRadians = FMath::DegreesToRadians(AngleDegrees);
		const FVector2D SegmentPosition(
			FMath::Cos(AngleRadians) * 36.0f,
			FMath::Sin(AngleRadians) * 36.0f);
		if (UCanvasPanelSlot* SegmentSlot = Cast<UCanvasPanelSlot>(Segment->Slot))
		{
			SegmentSlot->SetPosition(SegmentPosition);
			SegmentSlot->SetSize(FVector2D(12.0f, 5.0f));
		}
		Segment->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		Segment->SetRenderTransformAngle(AngleDegrees + 90.0f);
	}
}

void UTunaSweeperGameHudWidget::RefreshDialogueHudVisibility()
{
	if (IsHousingModeActive())
	{
		ForceCollapseHudWidget(BottomStatusWidget);
		ForceCollapseHudWidget(QuickSlotBarWidget);
		ForceCollapseHudWidget(DebuffBarWidget);
		return;
	}

	const bool bSuppressBottomHud = IsGameplayBottomHudSuppressed();
	const ESlateVisibility BottomStatusVisibility = bSuppressBottomHud
		? ESlateVisibility::Collapsed
		: ESlateVisibility::HitTestInvisible;
	const ESlateVisibility QuickSlotVisibility = bSuppressBottomHud
		? ESlateVisibility::Collapsed
		: ESlateVisibility::HitTestInvisible;
	const ESlateVisibility DebuffBarVisibility = (bSuppressBottomHud || !bDebuffBarHasActiveDebuffs)
		? ESlateVisibility::Collapsed
		: ESlateVisibility::HitTestInvisible;

	if (BottomStatusWidget)
	{
		SetTransitionedWidgetVisibility(BottomStatusWidget, BottomStatusVisibility, BottomStatusTransitionEdge);
	}

	if (QuickSlotBarWidget)
	{
		SetTransitionedWidgetVisibility(QuickSlotBarWidget, QuickSlotVisibility, QuickSlotBarTransitionEdge);
	}

	if (DebuffBarWidget)
	{
		SetTransitionedWidgetVisibility(DebuffBarWidget, DebuffBarVisibility, DebuffBarTransitionEdge);
	}
}

void UTunaSweeperGameHudWidget::RefreshExtractionProgressWidget()
{
	EnsureExtractionProgressWidget();
	if (!ExtractionProgressWidget)
	{
		return;
	}

	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(ExtractionProgressWidget->Slot))
	{
		CanvasSlot->SetPosition(FVector2D(0.0f, FMath::Max(0.0f, ExtractionProgressTopOffset)));
		CanvasSlot->SetSize(FVector2D(
			FMath::Max(1.0, ExtractionProgressWidgetSize.X),
			FMath::Max(1.0, ExtractionProgressWidgetSize.Y)));
	}

	const bool bShouldShowProgress =
		bExtractionProgressActive &&
		ExtractionProgressCurrentSeconds > 0.0f &&
		!IsGameplayBottomHudSuppressed();
	ExtractionProgressWidget->SetExtractionProgress(
		ExtractionProgressCurrentSeconds,
		ExtractionProgressRequiredSeconds,
		bShouldShowProgress);
}

void UTunaSweeperGameHudWidget::RefreshCursorDistanceWidget()
{
	EnsureCursorDistanceWidget();
	if (!CursorDistancePanel)
	{
		return;
	}

	auto HideDistancePanel = [this]()
	{
		LastCursorDistanceMeters = INDEX_NONE;
		SetTransitionedWidgetVisibility(
			CursorDistancePanel,
			ESlateVisibility::Collapsed,
			CursorDistanceTransitionEdge);
	};

	if (ActiveHudMode != ETunaSweeperHudMode::None || IsGameplayBottomHudSuppressed())
	{
		HideDistancePanel();
		return;
	}

	ATunaSweeperPlayerController* TunaPlayerController = Cast<ATunaSweeperPlayerController>(GetOwningPlayer());
	ATunaSweeperTopDownCharacter* TunaCharacter = TunaPlayerController
		? Cast<ATunaSweeperTopDownCharacter>(TunaPlayerController->GetPawn())
		: nullptr;
	if (!TunaPlayerController || !TunaCharacter || TunaCharacter->IsDead())
	{
		HideDistancePanel();
		return;
	}

	FVector CursorWorldPoint = FVector::ZeroVector;
	if (!TunaPlayerController->TryGetCursorWorldPointOnPlane(TunaCharacter->GetActorLocation().Z, CursorWorldPoint))
	{
		HideDistancePanel();
		return;
	}

	const float DistanceMeters = FVector::Dist2D(TunaCharacter->GetActorLocation(), CursorWorldPoint) / 100.0f;
	const int32 RoundedDistanceMeters = FMath::Max(0, FMath::RoundToInt(DistanceMeters));
	if (CursorDistanceText && LastCursorDistanceMeters != RoundedDistanceMeters)
	{
		CursorDistanceText->SetText(FText::FromString(FString::Printf(TEXT("%dM"), RoundedDistanceMeters)));
		LastCursorDistanceMeters = RoundedDistanceMeters;
	}

	SetTransitionedWidgetVisibility(
		CursorDistancePanel,
		ESlateVisibility::HitTestInvisible,
		CursorDistanceTransitionEdge);
}

void UTunaSweeperGameHudWidget::ForceCollapseHudWidget(UWidget* Widget)
{
	if (!Widget)
	{
		return;
	}

	CacheHudTransitionBaseline(Widget);
	ActiveHudTransitions.RemoveAll([Widget](const FHudWidgetTransition& Transition)
	{
		return Transition.Widget.Get() == Widget;
	});

	const TWeakObjectPtr<UWidget> WidgetKey(Widget);
	if (const FWidgetTransform* BaseTransform = HudTransitionBaseTransforms.Find(WidgetKey))
	{
		Widget->SetRenderTransform(*BaseTransform);
	}
	if (const float* BaseOpacity = HudTransitionBaseOpacities.Find(WidgetKey))
	{
		Widget->SetRenderOpacity(*BaseOpacity);
	}
	Widget->SetVisibility(ESlateVisibility::Collapsed);
}

void UTunaSweeperGameHudWidget::BuildAmmoSelectorOptionTexts(TArray<FText>& OutOptionTexts, int32& OutFocusedIndex) const
{
	OutOptionTexts.Reset();
	OutFocusedIndex = INDEX_NONE;

	const APlayerController* PlayerController = GetOwningPlayer();
	const ATunaSweeperTopDownCharacter* TunaCharacter = PlayerController
		? Cast<ATunaSweeperTopDownCharacter>(PlayerController->GetPawn())
		: nullptr;
	if (!TunaCharacter || !TunaCharacter->IsAmmoSelectionOpen())
	{
		return;
	}

	TArray<int32> AmmoItemIds;
	TunaCharacter->GetAmmoSelectionItemIds(AmmoItemIds);
	if (AmmoItemIds.Num() <= 0)
	{
		return;
	}

	UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	UTunaSweeperItemDataSubsystem* ItemDataSubsystem = TunaGameInstance
		? TunaGameInstance->GetSubsystem<UTunaSweeperItemDataSubsystem>()
		: nullptr;
	const ETunaSweeperItemTextLanguage Language = TunaGameInstance
		? TunaGameInstance->GetCurrentTextLanguage()
		: ETunaSweeperItemTextLanguage::English;

	for (int32 AmmoItemId : AmmoItemIds)
	{
		FText AmmoName = FText::Format(
			ResolveUiText(TunaGameInstance, TEXT("ui.common.item_fallback"), TEXT("Item {0}")),
			FText::AsNumber(AmmoItemId));
		if (ItemDataSubsystem)
		{
			ItemDataSubsystem->TryGetItemNameText(AmmoItemId, Language, AmmoName);
		}
		OutOptionTexts.Add(AmmoName);
	}

	OutFocusedIndex = TunaCharacter->GetAmmoSelectionFocusIndex();
}

bool UTunaSweeperGameHudWidget::IsDialogueSequenceActive() const
{
	const ATunaSweeperPlayerController* TunaPlayerController = Cast<ATunaSweeperPlayerController>(GetOwningPlayer());
	return TunaPlayerController && TunaPlayerController->IsDialogueSequenceActive();
}

bool UTunaSweeperGameHudWidget::IsHousingModeActive() const
{
	const UTunaSweeperHousingSubsystem* HousingSubsystem = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UTunaSweeperHousingSubsystem>()
		: nullptr;
	return HousingSubsystem && HousingSubsystem->IsHousingModeOpen();
}

bool UTunaSweeperGameHudWidget::IsGameplayBottomHudSuppressed() const
{
	return IsDialogueSequenceActive() || IsInventoryUiOpen() || IsHousingModeActive();
}

bool UTunaSweeperGameHudWidget::IsBunkerMap() const
{
	const UWorld* World = GetWorld();
	return World && World->GetMapName().EndsWith(TEXT("BunkerMap"));
}

FName UTunaSweeperGameHudWidget::GetSelectedWeaponTypeTag() const
{
	const APlayerController* PlayerController = GetOwningPlayer();
	const ATunaSweeperTopDownCharacter* TunaCharacter = PlayerController
		? Cast<ATunaSweeperTopDownCharacter>(PlayerController->GetPawn())
		: nullptr;
	if (!TunaCharacter || TunaCharacter->IsDead())
	{
		return NAME_None;
	}

	const int32 SelectedWeaponSlotNumber = TunaCharacter->GetSelectedWeaponSlotNumber();
	if (SelectedWeaponSlotNumber <= 0)
	{
		return NAME_None;
	}

	UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	if (!TunaGameInstance)
	{
		return NAME_None;
	}

	FTunaSweeperItemInstance WeaponInstance;
	FTunaSweeperItemDefinition WeaponDefinition;
	if (!TunaGameInstance->TryGetEquipmentWeaponSlotItem(SelectedWeaponSlotNumber, WeaponInstance, WeaponDefinition))
	{
		return NAME_None;
	}

	return WeaponDefinition.WeaponTypeTag;
}

bool UTunaSweeperGameHudWidget::IsWeaponCrosshairSuppressed() const
{
	if (IsInventoryUiOpen() || IsDialogueSequenceActive() || IsHousingModeActive())
	{
		return true;
	}

	return IsReloadGaugeReplacingCrosshair();
}

bool UTunaSweeperGameHudWidget::IsReloadGaugeReplacingCrosshair(const ATunaSweeperTopDownCharacter* TunaCharacter) const
{
	if (IsInventoryUiOpen() || IsDialogueSequenceActive() || IsHousingModeActive())
	{
		return false;
	}

	const ATunaSweeperTopDownCharacter* ResolvedCharacter = TunaCharacter;
	if (!ResolvedCharacter)
	{
		const APlayerController* PlayerController = GetOwningPlayer();
		ResolvedCharacter = PlayerController
			? Cast<ATunaSweeperTopDownCharacter>(PlayerController->GetPawn())
			: nullptr;
	}

	return ResolvedCharacter && ResolvedCharacter->IsWeaponReloading();
}

bool UTunaSweeperGameHudWidget::TryGetWeaponCrosshairLocalPosition(const FGeometry& AllottedGeometry, FVector2D& OutLocalPosition) const
{
	if (!FSlateApplication::IsInitialized())
	{
		return false;
	}

	const FVector2D CursorAbsolutePosition = FSlateApplication::Get().GetCursorPos();
	const FVector2D CursorLocalPosition = AllottedGeometry.AbsoluteToLocal(CursorAbsolutePosition);
	const FVector2D LocalSize = AllottedGeometry.GetLocalSize();
	if (CursorLocalPosition.X < 0.0f ||
		CursorLocalPosition.Y < 0.0f ||
		CursorLocalPosition.X > LocalSize.X ||
		CursorLocalPosition.Y > LocalSize.Y)
	{
		return false;
	}

	OutLocalPosition = CursorLocalPosition;
	if (const APlayerController* PlayerController = GetOwningPlayer())
	{
		if (const ATunaSweeperTopDownCharacter* TunaCharacter = Cast<ATunaSweeperTopDownCharacter>(PlayerController->GetPawn()))
		{
			OutLocalPosition += TunaCharacter->GetWeaponRecoilCrosshairScreenOffset();
		}
	}

	return true;
}

void UTunaSweeperGameHudWidget::UpdateCenterCancelableActionGaugePlacement(
	const FGeometry& AllottedGeometry,
	bool bUseCrosshairPosition)
{
	if (!CenterCancelableActionGaugeRoot)
	{
		return;
	}

	UCanvasPanelSlot* GaugeSlot = Cast<UCanvasPanelSlot>(CenterCancelableActionGaugeRoot->Slot);
	if (!GaugeSlot)
	{
		return;
	}

	if (!bCenterCancelableActionGaugeSlotLayoutCached)
	{
		DefaultCenterCancelableActionGaugeAnchors = GaugeSlot->GetAnchors();
		DefaultCenterCancelableActionGaugeAlignment = GaugeSlot->GetAlignment();
		DefaultCenterCancelableActionGaugePosition = GaugeSlot->GetPosition();
		DefaultCenterCancelableActionGaugeSize = GaugeSlot->GetSize();
		bCenterCancelableActionGaugeSlotLayoutCached = true;
	}

	GaugeSlot->SetAlignment(FVector2D(0.5f, 0.5f));
	GaugeSlot->SetSize(DefaultCenterCancelableActionGaugeSize);

	FVector2D CrosshairLocalPosition = FVector2D::ZeroVector;
	if (bUseCrosshairPosition && TryGetWeaponCrosshairLocalPosition(AllottedGeometry, CrosshairLocalPosition))
	{
		GaugeSlot->SetAnchors(FAnchors(0.0f, 0.0f, 0.0f, 0.0f));
		GaugeSlot->SetPosition(CrosshairLocalPosition);
		return;
	}

	GaugeSlot->SetAnchors(DefaultCenterCancelableActionGaugeAnchors);
	GaugeSlot->SetAlignment(DefaultCenterCancelableActionGaugeAlignment);
	GaugeSlot->SetPosition(DefaultCenterCancelableActionGaugePosition);
	GaugeSlot->SetSize(DefaultCenterCancelableActionGaugeSize);
}

void UTunaSweeperGameHudWidget::UpdateMouseCursorForReloadGauge(bool bShouldHideCursor)
{
	APlayerController* PlayerController = GetOwningPlayer();
	if (!PlayerController)
	{
		bReloadGaugeHidMouseCursor = false;
		return;
	}

	if (bShouldHideCursor)
	{
		PlayerController->CurrentMouseCursor = EMouseCursor::None;
		bReloadGaugeHidMouseCursor = true;
		return;
	}

	if (bReloadGaugeHidMouseCursor)
	{
		PlayerController->CurrentMouseCursor = PlayerController->DefaultMouseCursor;
		bReloadGaugeHidMouseCursor = false;
	}
}

void UTunaSweeperGameHudWidget::UpdateCrosshairState(float InDeltaTime)
{
	float TargetAimAlpha = 0.0f;
	if (!IsWeaponCrosshairSuppressed() && IsPrecisionCrosshairWeaponType(GetSelectedWeaponTypeTag()))
	{
		const APlayerController* PlayerController = GetOwningPlayer();
		const ATunaSweeperTopDownCharacter* TunaCharacter = PlayerController
			? Cast<ATunaSweeperTopDownCharacter>(PlayerController->GetPawn())
			: nullptr;
		TargetAimAlpha = TunaCharacter && TunaCharacter->IsAiming() ? 1.0f : 0.0f;
	}

	PrecisionCrosshairAimAlpha = FMath::FInterpTo(
		PrecisionCrosshairAimAlpha,
		TargetAimAlpha,
		FMath::Max(0.0f, InDeltaTime),
		FMath::Max(0.0f, PrecisionCrosshairAimInterpSpeed));
	if (FMath::IsNearlyEqual(PrecisionCrosshairAimAlpha, TargetAimAlpha, 0.001f))
	{
		PrecisionCrosshairAimAlpha = TargetAimAlpha;
	}
}

void UTunaSweeperGameHudWidget::HandleSelectedInventoryItemChanged()
{
	UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	const bool bCenterVisible =
		ActiveHudMode == ETunaSweeperHudMode::Inventory &&
		CenterContentPanel &&
		CenterContentPanel->GetVisibility() != ESlateVisibility::Collapsed;
	const bool bHasSelection = bCenterVisible &&
		TunaGameInstance &&
		TunaGameInstance->HasSelectedInventoryItem();

	bool bShowShopSellPanel = false;
	if (bHasSelection && IsShopPanelOpen() && TunaGameInstance)
	{
		int32 SalePrice = 0;
		bShowShopSellPanel = TunaGameInstance->TryGetSlotSellPrice(
			TunaGameInstance->GetSelectedItemSlotReference(),
			SalePrice);
	}

	SetShopSellPanelVisible(bShowShopSellPanel);
	SetItemInfoPanelVisible(bHasSelection && !bShowShopSellPanel);
	if (bShowShopSellPanel && ShopSellPanelWidget)
	{
		ShopSellPanelWidget->RefreshSelectedItem();
	}
	else if (bHasSelection && ItemInfoPanelWidget)
	{
		ItemInfoPanelWidget->RefreshSelectedItemInfo();
	}
}

void UTunaSweeperGameHudWidget::HandleQuestProgressChanged()
{
	if (MenuQuestPanelWidget)
	{
		MenuQuestPanelWidget->RefreshQuestView();
	}
	if (InteractionQuestPanelWidget)
	{
		InteractionQuestPanelWidget->RefreshQuestView();
	}
}

void UTunaSweeperGameHudWidget::HandleHousingStateChanged()
{
	ApplyHudModeVisibility();
	RefreshDialogueHudVisibility();
	RefreshCancelableActionWidgets();
	Invalidate(EInvalidateWidgetReason::LayoutAndVolatility);
}

void UTunaSweeperGameHudWidget::HandleLanguageChanged()
{
	RefreshLocalizedTexts();
	ApplyHudModeVisibility();
	RefreshBottomStatusFromGameInstance();
	RefreshQuickSlotsFromGameState();
	RefreshInventoryQuickSlotPanel();
	if (ItemInfoPanelWidget)
	{
		ItemInfoPanelWidget->RefreshSelectedItemInfo();
	}
	if (ShopSellPanelWidget)
	{
		ShopSellPanelWidget->RefreshSelectedItem();
	}
	if (MemoPanelWidget)
	{
		MemoPanelWidget->RefreshMemoView();
	}
	if (MenuQuestPanelWidget)
	{
		MenuQuestPanelWidget->RefreshQuestView();
	}
	if (InteractionQuestPanelWidget)
	{
		InteractionQuestPanelWidget->RefreshQuestView();
	}
}

void UTunaSweeperGameHudWidget::HandleHudModeTabSelected(ETunaSweeperHudMode SelectedMode)
{
	if (SelectedMode == ETunaSweeperHudMode::Quest)
	{
		ShowMenuQuestPanel();
		return;
	}

	SetHudMode(SelectedMode);
}
