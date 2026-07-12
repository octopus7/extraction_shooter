#include "UI/TunaSweeperGameHudWidget.h"
#include "TunaSweeperGameHudWidgetShared.h"

#include "AI/TunaSweeperEnemyAIController.h"
#include "AI/TunaSweeperEnemyCharacter.h"
#include "EngineUtils.h"

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
	DrawEnemyCombatDebugOverlay(AllottedGeometry, OutDrawElements, CurrentLayerId);

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

void UTunaSweeperGameHudWidget::DrawEnemyCombatDebugOverlay(
	const FGeometry& AllottedGeometry,
	FSlateWindowElementList& OutDrawElements,
	int32& InOutLayerId) const
{
	if (!bShowEnemyCombatDebug || !GetWorld())
	{
		return;
	}

	APlayerController* PlayerController = GetOwningPlayer();
	APawn* PlayerPawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	const FVector2D LocalSize = AllottedGeometry.GetLocalSize();
	if (!PlayerController || !PlayerPawn || LocalSize.X <= 1.0f || LocalSize.Y <= 1.0f)
	{
		return;
	}

	FVector2D PlayerScreenPosition = LocalSize * 0.5f;
	UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(
		PlayerController,
		PlayerPawn->GetActorLocation(),
		PlayerScreenPosition,
		true);
	PlayerScreenPosition.X = FMath::Clamp(PlayerScreenPosition.X, 0.0f, LocalSize.X);
	PlayerScreenPosition.Y = FMath::Clamp(PlayerScreenPosition.Y, 0.0f, LocalSize.Y);

	const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
	if (!WhiteBrush)
	{
		return;
	}

	TArray<FVector2D> PlacedWidgetCenters;
	TArray<float> PlacedWidgetCollisionRadii;
	float MaxPlacedWidgetCollisionRadius = 0.0f;
	const FVector PlayerLocation = PlayerPawn->GetActorLocation();
	for (TActorIterator<ATunaSweeperEnemyCharacter> EnemyIt(GetWorld()); EnemyIt; ++EnemyIt)
	{
		ATunaSweeperEnemyCharacter* Enemy = *EnemyIt;
		ATunaSweeperEnemyAIController* EnemyController = Enemy
			? Cast<ATunaSweeperEnemyAIController>(Enemy->GetController())
			: nullptr;
		FTunaSweeperEnemyCombatDebugSnapshot Snapshot;
		if (!Enemy || !EnemyController || !EnemyController->GetCombatDebugSnapshot(Snapshot))
		{
			continue;
		}

		FVector2D EnemyScreenPosition = FVector2D::ZeroVector;
		const bool bProjected = UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(
			PlayerController, Enemy->GetActorLocation(), EnemyScreenPosition, true);
		const bool bOnScreen = bProjected &&
			EnemyScreenPosition.X >= 0.0f && EnemyScreenPosition.X <= LocalSize.X &&
			EnemyScreenPosition.Y >= 0.0f && EnemyScreenPosition.Y <= LocalSize.Y;
		if (!bOnScreen && !Snapshot.bIsCombatEngaged)
		{
			continue;
		}

		FVector ToEnemy = Enemy->GetActorLocation() - PlayerLocation;
		ToEnemy.Z = 0.0f;
		if (ToEnemy.IsNearlyZero())
		{
			continue;
		}
		const float DistanceMeters = ToEnemy.Size() / 100.0f;
		ToEnemy.Normalize();

		FVector2D DirectionProbe = FVector2D::ZeroVector;
		UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(
			PlayerController, PlayerLocation + ToEnemy * 1000.0f, DirectionProbe, true);
		FVector2D OrbitDirection = (DirectionProbe - PlayerScreenPosition).GetSafeNormal();
		if (OrbitDirection.IsNearlyZero() && bProjected)
		{
			OrbitDirection = (EnemyScreenPosition - PlayerScreenPosition).GetSafeNormal();
		}
		if (OrbitDirection.IsNearlyZero())
		{
			OrbitDirection = FVector2D(1.0f, 0.0f);
		}

		const float Scale = bOnScreen ? 1.0f : 0.5f;
		const float Diameter = 110.0f * Scale;
		const float Radius = Diameter * 0.5f;
		const int32 DistanceFontSize = FMath::Max(22, FMath::RoundToInt(28.0f * Scale));
		const int32 StateFontSize = FMath::Max(10, FMath::RoundToInt(11.0f * Scale));
		const FString DistanceText = FString::Printf(TEXT("%.1fm"), DistanceMeters);
		const FString StatePrefix = bOnScreen ? TEXT("AI: ") : FString();
		const float DistanceTextWidth = DistanceText.Len() * DistanceFontSize * 0.56f;
		const float StateTextWidth = (StatePrefix.Len() + Snapshot.StateLabel.Len()) * StateFontSize * 0.52f;
		const float CollisionRadius = FMath::Max3(
			Radius + 16.0f,
			DistanceTextWidth * 0.5f + 12.0f,
			StateTextWidth * 0.5f + 12.0f);
		const float BaseBearingRadians = FMath::Atan2(OrbitDirection.Y, OrbitDirection.X);
		const FVector2D ActorBearingDirection = OrbitDirection;
		const FVector2D BaseAnchor = PlayerScreenPosition + ActorBearingDirection * EnemyCombatDebugOrbitRadius;
		FVector2D WidgetCenter = BaseAnchor;
		const float RingSpacing = FMath::Max(
			Diameter + 46.0f,
			2.0f * FMath::Max(CollisionRadius, MaxPlacedWidgetCollisionRadius) + 24.0f);
		constexpr int32 MaxLayoutRings = 12;
		constexpr int32 MaxInnerRingAngleSteps = 2;
		constexpr int32 MaxOuterRingAngleSteps = 6;
		constexpr float LayoutAngleStepRadians = 8.0f * PI / 180.0f;
		bool bFoundLayout = false;
		for (int32 RingIndex = 0; RingIndex < MaxLayoutRings && !bFoundLayout; ++RingIndex)
		{
			const int32 MaxAngleSteps = RingIndex == 0 ? MaxInnerRingAngleSteps : MaxOuterRingAngleSteps;
			const float OrbitRadius = EnemyCombatDebugOrbitRadius + RingSpacing * RingIndex;
			for (int32 AttemptIndex = 0; AttemptIndex <= MaxAngleSteps * 2; ++AttemptIndex)
			{
				float AngleOffsetRadians = 0.0f;
				if (AttemptIndex > 0)
				{
					const int32 OffsetStep = (AttemptIndex + 1) / 2;
					const float OffsetSign = (AttemptIndex % 2) == 1 ? 1.0f : -1.0f;
					AngleOffsetRadians = OffsetSign * OffsetStep * LayoutAngleStepRadians;
				}

				const float LayoutBearingRadians = BaseBearingRadians + AngleOffsetRadians;
				const FVector2D CandidateDirection(FMath::Cos(LayoutBearingRadians), FMath::Sin(LayoutBearingRadians));
				const FVector2D CandidateCenter = PlayerScreenPosition + CandidateDirection * OrbitRadius;
				bool bOverlapsPlacedWidget = false;
				for (int32 PlacedIndex = 0; PlacedIndex < PlacedWidgetCenters.Num(); ++PlacedIndex)
				{
					const float RequiredSpacing = CollisionRadius + PlacedWidgetCollisionRadii[PlacedIndex];
					if (FVector2D::DistSquared(CandidateCenter, PlacedWidgetCenters[PlacedIndex]) < FMath::Square(RequiredSpacing))
					{
						bOverlapsPlacedWidget = true;
						break;
					}
				}

				if (!bOverlapsPlacedWidget)
				{
					WidgetCenter = CandidateCenter;
					bFoundLayout = true;
					break;
				}
			}
		}
		if (!bFoundLayout)
		{
			// Do not draw a marker in an overlapping fallback position. The next frame retries the layout.
			continue;
		}
		PlacedWidgetCenters.Add(WidgetCenter);
		PlacedWidgetCollisionRadii.Add(CollisionRadius);
		MaxPlacedWidgetCollisionRadius = FMath::Max(MaxPlacedWidgetCollisionRadius, CollisionRadius);
		const FVector2D WidgetTopLeft = WidgetCenter - FVector2D(Radius, Radius);
		if (!WidgetCenter.Equals(BaseAnchor, 1.0f))
		{
			FSlateDrawElement::MakeLines(
				OutDrawElements,
				InOutLayerId + 1,
				AllottedGeometry.ToPaintGeometry(),
				TArray<FVector2D>{ BaseAnchor, WidgetCenter },
				ESlateDrawEffect::None,
				FLinearColor(1.0f, 0.46f, 0.10f, 0.55f),
				true,
				1.0f);
		}

		FSlateRoundedBoxBrush CircleBrush(FLinearColor(0.035f, 0.018f, 0.012f, 0.90f), Radius);
		FSlateDrawElement::MakeBox(
			OutDrawElements,
			InOutLayerId + 1,
			MakeHudLocalBoxGeometry(AllottedGeometry, WidgetTopLeft, FVector2D(Diameter, Diameter)),
			&CircleBrush,
			ESlateDrawEffect::None,
			FLinearColor::White);

		TArray<FVector2D> CirclePoints;
		constexpr int32 CircleSegments = 28;
		CirclePoints.Reserve(CircleSegments + 1);
		for (int32 SegmentIndex = 0; SegmentIndex <= CircleSegments; ++SegmentIndex)
		{
			const float Angle = 2.0f * PI * static_cast<float>(SegmentIndex) / CircleSegments;
			CirclePoints.Add(WidgetCenter + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * (Radius - Scale));
		}
		FSlateDrawElement::MakeLines(
			OutDrawElements, InOutLayerId + 2, AllottedGeometry.ToPaintGeometry(), CirclePoints,
			ESlateDrawEffect::None, FLinearColor(1.0f, 0.39f, 0.08f, 0.92f), true, 1.5f * Scale);

		FVector2D FacingProbe = FVector2D::ZeroVector;
		FVector2D EnemyBaseScreen = EnemyScreenPosition;
		const bool bFacingProjected = UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(
			PlayerController, Enemy->GetActorLocation() + Snapshot.FacingDirection * 180.0f, FacingProbe, true);
		FVector2D FacingDirection = bFacingProjected ? (FacingProbe - EnemyBaseScreen).GetSafeNormal() : FVector2D::ZeroVector;
		if (FacingDirection.IsNearlyZero())
		{
			FacingDirection = ActorBearingDirection;
		}
		const FVector2D FacingPerpendicular(-FacingDirection.Y, FacingDirection.X);
		const FVector2D TriangleTip = WidgetCenter + FacingDirection * (Radius + 8.0f * Scale);
		TArray<FVector2D> TrianglePoints;
		TrianglePoints.Add(TriangleTip);
		TrianglePoints.Add(TriangleTip - FacingDirection * (13.0f * Scale) + FacingPerpendicular * (6.0f * Scale));
		TrianglePoints.Add(TriangleTip - FacingDirection * (13.0f * Scale) - FacingPerpendicular * (6.0f * Scale));
		TrianglePoints.Add(TriangleTip);
		FSlateDrawElement::MakeLines(
			OutDrawElements, InOutLayerId + 3, AllottedGeometry.ToPaintGeometry(), TrianglePoints,
			ESlateDrawEffect::None, FLinearColor(1.0f, 0.76f, 0.28f, 1.0f), true, 1.5f * Scale);

		auto DrawTextAt = [&](const FString& Text, const FVector2D& Position, int32 FontSize, int32 MinimumFontSize, bool bBold, const FLinearColor& Color)
		{
			const int32 ResolvedFontSize = FMath::Max(MinimumFontSize, FMath::RoundToInt(FontSize * Scale));
			const FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle(bBold ? "Bold" : "Regular", ResolvedFontSize);
			const FVector2D TextSize(Diameter, FMath::Max(18.0f * Scale, static_cast<float>(ResolvedFontSize + 4)));
			FSlateDrawElement::MakeText(
				OutDrawElements, InOutLayerId + 4,
				MakeHudLocalBoxGeometry(AllottedGeometry, Position + FVector2D(1.0f, 1.0f), TextSize),
				Text, Font, ESlateDrawEffect::None, FLinearColor(0.0f, 0.0f, 0.0f, 0.92f));
			FSlateDrawElement::MakeText(
				OutDrawElements, InOutLayerId + 4,
				MakeHudLocalBoxGeometry(AllottedGeometry, Position, TextSize),
				Text, Font, ESlateDrawEffect::None, Color);
		};

		auto DrawCenteredText = [&](const FString& Text, float OffsetY, int32 FontSize, int32 MinimumFontSize, bool bBold, const FLinearColor& Color)
		{
			const int32 ResolvedFontSize = FMath::Max(MinimumFontSize, FMath::RoundToInt(FontSize * Scale));
			const float ApproximateWidth = Text.Len() * ResolvedFontSize * 0.52f;
			DrawTextAt(Text, FVector2D(WidgetCenter.X - ApproximateWidth * 0.5f, WidgetCenter.Y + OffsetY * Scale), FontSize, MinimumFontSize, bBold, Color);
		};

		const float DistanceTopY = -Radius + 4.0f * Scale;
		DrawCenteredText(
			DistanceText,
			DistanceTopY / Scale,
			28,
			22,
			true,
			FLinearColor(1.0f, 0.96f, 0.87f, 1.0f));
		const float PrefixWidth = StatePrefix.Len() * StateFontSize * 0.52f;
		const float StateWidth = Snapshot.StateLabel.Len() * StateFontSize * 0.52f;
		const float StateTopY = DistanceTopY + DistanceFontSize + 4.0f;
		const FVector2D StateStart(WidgetCenter.X - (PrefixWidth + StateWidth) * 0.5f, WidgetCenter.Y + StateTopY);
		if (!StatePrefix.IsEmpty())
		{
			DrawTextAt(StatePrefix, StateStart, 11, 10, false, FLinearColor(0.78f, 0.80f, 0.82f, 1.0f));
		}
		DrawTextAt(Snapshot.StateLabel, StateStart + FVector2D(PrefixWidth, 0.0f), 11, 10, true, FLinearColor(0.64f, 0.96f, 1.0f, 1.0f));
		if (Snapshot.MaxStateSeconds > 0.0f)
		{
			const float TimeTopY = StateTopY + StateFontSize + 4.0f;
			DrawCenteredText(
				FString::Printf(TEXT("%.1f / %.1fs"), Snapshot.RemainingStateSeconds, Snapshot.MaxStateSeconds),
				TimeTopY / Scale,
				10,
				9,
				false,
				FLinearColor(1.0f, 0.93f, 0.76f, 1.0f));
		}

		InOutLayerId += 4;
	}
}

