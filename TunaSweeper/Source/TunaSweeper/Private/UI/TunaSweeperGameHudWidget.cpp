#include "UI/TunaSweeperGameHudWidget.h"
#include "TunaSweeperGameHudWidgetShared.h"

DEFINE_LOG_CATEGORY(LogTunaSweeperGameHud);

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
	EnsureInventoryWeightPanelWidget();
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
	TickHeadphoneNoiseRipples(InDeltaTime);
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
	int32 CurrentLayerId = PaintedLayerId;

	if (!FSlateApplication::IsInitialized())
	{
		return CurrentLayerId;
	}

	if (bShowHeadphoneDebugIdleRing)
	{
		if (const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
		{
			const FVector2D LocalSize = AllottedGeometry.GetLocalSize();
			if (LocalSize.X > 1.0f && LocalSize.Y > 1.0f)
			{
				FVector2D RingCenter = LocalSize * 0.5f;
				if (APlayerController* PlayerController = GetOwningPlayer())
				{
					if (APawn* PlayerPawn = PlayerController->GetPawn())
					{
						FVector2D ProjectedPlayerPosition = FVector2D::ZeroVector;
						if (UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(
							PlayerController,
							PlayerPawn->GetActorLocation(),
							ProjectedPlayerPosition,
							true))
						{
							RingCenter.X = FMath::Clamp(ProjectedPlayerPosition.X, 0.0, LocalSize.X);
							RingCenter.Y = FMath::Clamp(ProjectedPlayerPosition.Y, 0.0, LocalSize.Y);
						}
					}
				}

				const int32 ParticleCount = FMath::Clamp(HeadphoneDebugIdleRingParticleCount, 8, 256);
				const float Radius = FMath::Max(16.0f, FMath::Max(244.0f, HeadphoneDebugIdleRingRadius));
				const float ParticleSize = FMath::Max(0.5f, HeadphoneDebugIdleRingParticleSize);
				const FVector2D ParticleDrawSize(ParticleSize, ParticleSize);
				const float AnimationSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
				for (int32 ParticleIndex = 0; ParticleIndex < ParticleCount; ++ParticleIndex)
				{
					const float Angle = (static_cast<float>(ParticleIndex) / static_cast<float>(ParticleCount)) * 2.0f * PI;
					const float RadiusWave = FMath::Sin(AnimationSeconds * 2.8f + Angle * 5.0f) * 2.5f;
					const FVector2D UnitDirection(FMath::Cos(Angle), FMath::Sin(Angle));
					const FVector2D ParticleCenter = RingCenter + UnitDirection * (Radius + RadiusWave);
					FSlateDrawElement::MakeBox(
						OutDrawElements,
						CurrentLayerId + 1,
						MakeHudLocalBoxGeometry(
							AllottedGeometry,
							ParticleCenter - ParticleDrawSize * 0.5f,
							ParticleDrawSize),
						WhiteBrush,
						ESlateDrawEffect::None,
						HeadphoneDebugIdleRingColor);
				}
				CurrentLayerId += 1;
			}
		}
	}

	DrawHeadphoneNoiseRipples(AllottedGeometry, OutDrawElements, CurrentLayerId);

	if (IsWeaponCrosshairSuppressed())
	{
		return CurrentLayerId;
	}

	const FName WeaponTypeTag = GetSelectedWeaponTypeTag();
	const bool bShotgunCrosshair = WeaponTypeTag == ShotgunWeaponTypeTag;
	const bool bPrecisionCrosshair = IsPrecisionCrosshairWeaponType(WeaponTypeTag);
	if (!bShotgunCrosshair && !bPrecisionCrosshair)
	{
		return CurrentLayerId;
	}

	FVector2D CrosshairLocalPosition = FVector2D::ZeroVector;
	if (!TryGetWeaponCrosshairLocalPosition(AllottedGeometry, CrosshairLocalPosition))
	{
		return CurrentLayerId;
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
		DrawLineStrip(CirclePoints, ShotgunCrosshairColor, ShotgunCrosshairThickness, CurrentLayerId + 1);
		return CurrentLayerId + 1;
	}

	const float AimAlpha = SmoothTransitionAlpha(PrecisionCrosshairAimAlpha);
	FLinearColor BarColor = PrecisionCrosshairColor;

	const float BarDistance = FMath::Lerp(
		FMath::Max(1.0f, PrecisionCrosshairAimBarStartDistance),
		FMath::Max(1.0f, PrecisionCrosshairAimBarEndDistance),
		AimAlpha);
	const float BarLength = FMath::Max(1.0f, PrecisionCrosshairAimBarLength);

	TArray<FVector2D> SegmentPoints;
	SegmentPoints.SetNum(2);
	SegmentPoints[0] = CrosshairLocalPosition + FVector2D(-BarDistance - BarLength, 0.0f);
	SegmentPoints[1] = CrosshairLocalPosition + FVector2D(-BarDistance, 0.0f);
	DrawLineStrip(SegmentPoints, BarColor, PrecisionCrosshairThickness, CurrentLayerId + 1);

	SegmentPoints[0] = CrosshairLocalPosition + FVector2D(BarDistance, 0.0f);
	SegmentPoints[1] = CrosshairLocalPosition + FVector2D(BarDistance + BarLength, 0.0f);
	DrawLineStrip(SegmentPoints, BarColor, PrecisionCrosshairThickness, CurrentLayerId + 1);

	SegmentPoints[0] = CrosshairLocalPosition + FVector2D(0.0f, -BarDistance - BarLength);
	SegmentPoints[1] = CrosshairLocalPosition + FVector2D(0.0f, -BarDistance);
	DrawLineStrip(SegmentPoints, BarColor, PrecisionCrosshairThickness, CurrentLayerId + 1);

	SegmentPoints[0] = CrosshairLocalPosition + FVector2D(0.0f, BarDistance);
	SegmentPoints[1] = CrosshairLocalPosition + FVector2D(0.0f, BarDistance + BarLength);
	DrawLineStrip(SegmentPoints, BarColor, PrecisionCrosshairThickness, CurrentLayerId + 1);

	if (AimAlpha > 0.01f)
	{
		if (const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
		{
			FLinearColor DotColor = PrecisionCrosshairCenterDotColor;
			DotColor.A *= AimAlpha;
			const float DotDiameter = FMath::Max(1.0f, PrecisionCrosshairCenterDotDiameter);
			const FVector2D DotSize(DotDiameter, DotDiameter);
			FSlateDrawElement::MakeBox(
				OutDrawElements,
				CurrentLayerId + 2,
				MakeHudLocalBoxGeometry(AllottedGeometry, CrosshairLocalPosition - DotSize * 0.5f, DotSize),
				WhiteBrush,
				ESlateDrawEffect::None,
				DotColor);
		}
	}

	return CurrentLayerId + 2;
}

