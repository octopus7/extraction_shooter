#include "TunaSweeperGameHudWidgetShared.h"

void UTunaSweeperGameHudWidget::AddHeadphoneNoiseRipple(const FVector& DirectionFromListener, float Strength)
{
	QueueHeadphoneNoiseRipple(DirectionFromListener, Strength, FVector::ZeroVector, nullptr, false);
}

void UTunaSweeperGameHudWidget::AddHeadphoneNoiseRippleFromSource(
	const FVector& SourceLocation,
	AActor* SourceActor,
	const FVector& DirectionFromListener,
	float Strength)
{
	QueueHeadphoneNoiseRipple(DirectionFromListener, Strength, SourceLocation, SourceActor, true);
}

void UTunaSweeperGameHudWidget::QueueHeadphoneNoiseRipple(
	const FVector& DirectionFromListener,
	float Strength,
	const FVector& SourceLocation,
	AActor* SourceActor,
	bool bTrackSourceLocation)
{
	FHeadphoneNoiseRipple NewRipple;
	NewRipple.DirectionFromListener = DirectionFromListener.GetSafeNormal2D();
	if (NewRipple.DirectionFromListener.IsNearlyZero())
	{
		NewRipple.DirectionFromListener = FVector::ForwardVector;
	}
	NewRipple.SourceLocation = SourceActor ? SourceActor->GetActorLocation() : SourceLocation;
	NewRipple.SourceActor = SourceActor;
	NewRipple.bTrackSourceLocation = bTrackSourceLocation;
	NewRipple.Strength = FMath::Clamp(Strength, 0.0f, 1.0f);
	NewRipple.ElapsedSeconds = 0.0f;
	NewRipple.Seed = NextHeadphoneNoiseRippleSeed++;
	if (NextHeadphoneNoiseRippleSeed <= 0)
	{
		NextHeadphoneNoiseRippleSeed = 1;
	}

	HeadphoneNoiseRipples.Add(NewRipple);
	constexpr int32 MaxActiveHeadphoneNoiseRipples = 12;
	if (HeadphoneNoiseRipples.Num() > MaxActiveHeadphoneNoiseRipples)
	{
		HeadphoneNoiseRipples.RemoveAt(0, HeadphoneNoiseRipples.Num() - MaxActiveHeadphoneNoiseRipples, EAllowShrinking::No);
	}

	UE_LOG(
		LogTunaSweeperGameHud,
		Log,
		TEXT("Headphone HUD ripple queued: strength=%.3f direction=(%.2f, %.2f, %.2f) source=(%.1f, %.1f, %.1f) sourceActor=%s trackSource=%s activeRipples=%d"),
		NewRipple.Strength,
		NewRipple.DirectionFromListener.X,
		NewRipple.DirectionFromListener.Y,
		NewRipple.DirectionFromListener.Z,
		NewRipple.SourceLocation.X,
		NewRipple.SourceLocation.Y,
		NewRipple.SourceLocation.Z,
		*GetNameSafe(NewRipple.SourceActor.Get()),
		NewRipple.bTrackSourceLocation ? TEXT("true") : TEXT("false"),
		HeadphoneNoiseRipples.Num());

	Invalidate(EInvalidateWidgetReason::Paint);
}

void UTunaSweeperGameHudWidget::TickHeadphoneNoiseRipples(float InDeltaTime)
{
	const float DeltaSeconds = FMath::Max(0.0f, InDeltaTime);
	for (FHeadphoneNoiseRipple& Ripple : HeadphoneNoiseRipples)
	{
		Ripple.ElapsedSeconds += DeltaSeconds;
	}

	const float Lifetime = FMath::Max(
		FMath::Max(1.05f, HeadphoneNoiseRippleLifetimeSeconds),
		bShowHeadphoneDebugNoiseDirectionSolidCircle
			? FMath::Max(0.05f, HeadphoneDebugNoiseDirectionSolidCircleFadeSeconds)
			: 0.0f);
	HeadphoneNoiseRipples.RemoveAllSwap([Lifetime](const FHeadphoneNoiseRipple& Ripple)
	{
		return Ripple.ElapsedSeconds >= Lifetime;
	});
}

void UTunaSweeperGameHudWidget::DrawHeadphoneNoiseRipples(
	const FGeometry& AllottedGeometry,
	FSlateWindowElementList& OutDrawElements,
	int32& InOutLayerId) const
{
	if (HeadphoneNoiseRipples.IsEmpty())
	{
		return;
	}

	const FVector2D LocalSize = AllottedGeometry.GetLocalSize();
	if (LocalSize.X <= 1.0f || LocalSize.Y <= 1.0f)
	{
		return;
	}

	FVector2D RingCenter = LocalSize * 0.5f;
	APawn* PlayerPawn = nullptr;
	APlayerController* PlayerController = GetOwningPlayer();
	if (PlayerController)
	{
		PlayerPawn = PlayerController->GetPawn();
		if (PlayerPawn)
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

	const float ResolutionScale = FMath::Clamp(static_cast<float>(LocalSize.Y / 1080.0), 0.72f, 1.6f);
	const float Radius = FMath::Max(16.0f, FMath::Max(244.0f, HeadphoneNoiseRingRadius) * ResolutionScale);
	const float RingThickness = FMath::Max(1.0f, 18.0f * ResolutionScale);
	const float ParticleSize = FMath::Max(4.0f, HeadphoneNoiseParticleSize);
	const float Lifetime = FMath::Max(1.05f, HeadphoneNoiseRippleLifetimeSeconds);
	const float BurstDurationSeconds = FMath::Min(0.24f, Lifetime * 0.42f);
	constexpr float ParticleFadeOutDurationSeconds = 0.4f;
	constexpr float ParticleSustainAlpha = 0.6f;
	constexpr float ParticleBurstAlpha = 1.0f;
	const int32 DrawLayerId = InOutLayerId + 1;
	const bool bDrawDebugNoiseDirectionSolidCircle = bShowHeadphoneDebugNoiseDirectionSolidCircle &&
		HeadphoneDebugNoiseDirectionSolidCircleColor.A > 0.0f &&
		HeadphoneDebugNoiseDirectionSolidCircleDiameter > 0.0f;
	const float DebugNoiseDirectionSolidCircleDiameter =
		FMath::Max(1.0f, HeadphoneDebugNoiseDirectionSolidCircleDiameter);
	const FVector2D DebugNoiseDirectionSolidCircleSize(
		DebugNoiseDirectionSolidCircleDiameter,
		DebugNoiseDirectionSolidCircleDiameter);
	const float DebugNoiseDirectionSolidCircleOrbitRadius =
		FMath::Max(16.0f, FMath::Max(244.0f, HeadphoneDebugIdleRingRadius) * ResolutionScale);
	const float DebugNoiseDirectionSolidCircleLifetime =
		FMath::Max(0.05f, HeadphoneDebugNoiseDirectionSolidCircleFadeSeconds);
	FSlateBrush DebugNoiseDirectionSolidCircleBrush = MakeHudRoundedBoxBrush(
		DebugNoiseDirectionSolidCircleSize,
		FLinearColor::White,
		DebugNoiseDirectionSolidCircleDiameter * 0.5f,
		FLinearColor::Transparent,
		0.0f);
	FSlateBrush HeadphoneNoiseCircleParticleBrush = MakeHudRoundedBoxBrush(
		FVector2D(ParticleSize * 1.35f, ParticleSize * 1.35f),
		FLinearColor::White,
		ParticleSize,
		FLinearColor::Transparent,
		0.0f);

	auto DrawCircleParticle = [&](
		const FVector2D& ParticleCenter,
		float Size,
		const FLinearColor& Color,
		int32 LayerId)
	{
		if (Color.A <= 0.0f || Size <= 0.0f)
		{
			return;
		}

		const FVector2D DrawSize(Size, Size);
		FSlateDrawElement::MakeBox(
			OutDrawElements,
			LayerId,
			MakeHudLocalBoxGeometry(
				AllottedGeometry,
				FVector2D(
					FMath::RoundToDouble(ParticleCenter.X - DrawSize.X * 0.5),
					FMath::RoundToDouble(ParticleCenter.Y - DrawSize.Y * 0.5)),
				DrawSize),
			&HeadphoneNoiseCircleParticleBrush,
			ESlateDrawEffect::None,
			Color);
	};

	for (const FHeadphoneNoiseRipple& Ripple : HeadphoneNoiseRipples)
	{
		const float Alpha = FMath::Clamp(Ripple.ElapsedSeconds / Lifetime, 0.0f, 1.0f);
		const float StrengthAlpha = FMath::Clamp(Ripple.Strength, 0.0f, 1.0f);
		const float BurstPeakSeconds = FMath::Min(0.085f, BurstDurationSeconds * 0.45f);
		const float BurstRiseAlpha = FMath::Clamp(Ripple.ElapsedSeconds / FMath::Max(0.01f, BurstPeakSeconds), 0.0f, 1.0f);
		const float BurstFallAlpha = FMath::Clamp(
			(Ripple.ElapsedSeconds - BurstPeakSeconds) / FMath::Max(0.01f, BurstDurationSeconds - BurstPeakSeconds),
			0.0f,
			1.0f);
		const float BurstRise = SmoothTransitionAlpha(BurstRiseAlpha);
		const float BurstDecay = 1.0f - SmoothTransitionAlpha(BurstFallAlpha);
		const float ElasticKick = FMath::Sin(BurstRiseAlpha * PI) * 0.12f * BurstDecay;
		const float BurstAmount = FMath::Clamp(BurstRise * BurstDecay * (1.0f + ElasticKick), 0.0f, 1.15f);
		const float RemainingSeconds = Lifetime - Ripple.ElapsedSeconds;
		const float BaseBurstParticleAlpha = FMath::Lerp(
			ParticleSustainAlpha,
			ParticleBurstAlpha,
			SmoothTransitionAlpha(FMath::Clamp(BurstAmount, 0.0f, 1.0f)));
		const float BaseFadeOutParticleAlpha = ParticleSustainAlpha * SmoothTransitionAlpha(FMath::Clamp(
			RemainingSeconds / ParticleFadeOutDurationSeconds,
			0.0f,
			1.0f));
		const float BaseParticleAlpha = RemainingSeconds <= ParticleFadeOutDurationSeconds
			? BaseFadeOutParticleAlpha
			: BaseBurstParticleAlpha;
		const float ParticleMaxAlpha = BaseParticleAlpha * StrengthAlpha;
		const float CurrentParticleSize = ParticleSize;
		const float CurrentRingThickness =
			RingThickness *
			FMath::Lerp(0.34f, 1.0f, BurstAmount) *
			FMath::Clamp(StrengthAlpha, 0.05f, 1.0f);

		FVector DirectionFromListener = Ripple.DirectionFromListener.GetSafeNormal2D();
		if (DirectionFromListener.IsNearlyZero())
		{
			DirectionFromListener = FVector::ForwardVector;
		}
		if (Ripple.bTrackSourceLocation && PlayerPawn)
		{
			const FVector CurrentSourceLocation = Ripple.SourceActor.IsValid()
				? Ripple.SourceActor->GetActorLocation()
				: Ripple.SourceLocation;
			FVector TrackedDirection = CurrentSourceLocation - PlayerPawn->GetActorLocation();
			TrackedDirection.Z = 0.0f;
			if (!TrackedDirection.IsNearlyZero())
			{
				DirectionFromListener = TrackedDirection.GetSafeNormal2D();
			}
		}

		FVector2D ScreenDirection = FVector2D(1.0f, 0.0f);
		if (PlayerController && PlayerPawn)
		{
			FVector2D DirectionTarget = FVector2D::ZeroVector;
			const FVector TargetLocation = PlayerPawn->GetActorLocation() + DirectionFromListener * 240.0f;
			if (UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(
				PlayerController,
				TargetLocation,
				DirectionTarget,
				true))
			{
				const FVector2D ProjectedDirection = DirectionTarget - RingCenter;
				if (!ProjectedDirection.IsNearlyZero())
				{
					ScreenDirection = ProjectedDirection.GetSafeNormal();
				}
			}
		}
		else
		{
			const FVector2D FallbackDirection(DirectionFromListener.X, -DirectionFromListener.Y);
			if (!FallbackDirection.IsNearlyZero())
			{
				ScreenDirection = FallbackDirection.GetSafeNormal();
			}
		}

		const FVector2D ScreenRight(-ScreenDirection.Y, ScreenDirection.X);
		const float DirectionAngle = FMath::Atan2(ScreenDirection.Y, ScreenDirection.X);

		const int32 EffectiveMinSectorParticleCount = FMath::Max(128, HeadphoneNoiseMinSectorParticleCount);
		const int32 EffectiveMaxSectorParticleCount = FMath::Max(
			EffectiveMinSectorParticleCount,
			FMath::Max(480, HeadphoneNoiseMaxSectorParticleCount));
		const int32 SectorCount = FMath::RoundToInt(FMath::Lerp(
			static_cast<float>(EffectiveMinSectorParticleCount),
			static_cast<float>(EffectiveMaxSectorParticleCount),
			SmoothTransitionAlpha(StrengthAlpha)));
		const float SectorHalfAngle = FMath::DegreesToRadians(FMath::Clamp(HeadphoneNoiseSectorHalfAngleDegrees, 1.0f, 90.0f));
		for (int32 Index = 0; Index < SectorCount; ++Index)
		{
			const float SignedAngleUnit = HeadphoneNoiseCenterWeightedSignedUnit(
				HeadphoneNoiseHash01(Ripple.Seed, Index, 0.13f),
				1.35f);
			const float NormalizedAngle = FMath::Abs(SignedAngleUnit);
			const float Influence = HeadphoneNoiseAngularInfluence(NormalizedAngle);
			if (Influence <= 0.01f)
			{
				continue;
			}

			const float Angle = DirectionAngle + SignedAngleUnit * SectorHalfAngle;
			const FVector2D ArcDirection(FMath::Cos(Angle), FMath::Sin(Angle));
			const float RadiusNoise = HeadphoneNoiseHash01(Ripple.Seed, Index, 1.73f) * 2.0f - 1.0f;
			const float PhaseNoise = HeadphoneNoiseHash01(Ripple.Seed, Index, 2.41f) * 2.0f * PI;
			const float Wobble =
				FMath::Sin(Alpha * 2.0f * PI * 2.15f + PhaseNoise) *
				CurrentRingThickness *
				FMath::Lerp(0.12f, 0.72f, Influence);
			const float RadiusOffset = RadiusNoise * CurrentRingThickness * FMath::Lerp(0.22f, 1.1f, Influence) + Wobble;
			const float TangentScatter =
				(HeadphoneNoiseHash01(Ripple.Seed, Index, 3.29f) * 2.0f - 1.0f) *
				CurrentRingThickness *
				FMath::Lerp(0.10f, 0.82f, Influence) *
				FMath::Lerp(0.08f, 1.0f, BurstAmount);

			FLinearColor ParticleColor = FLinearColor::White;
			ParticleColor.A = ParticleMaxAlpha * FMath::Lerp(0.12f, 1.0f, Influence);
			const float SizeNoise = HeadphoneNoiseHash01(Ripple.Seed, Index, 5.13f);
			const float SandParticleSize = CurrentParticleSize * FMath::Lerp(0.86f, 1.20f, SizeNoise);
			const FVector2D SandParticleCenter =
				RingCenter + ArcDirection * (Radius + RadiusOffset) + ScreenRight * TangentScatter;
			DrawCircleParticle(
				SandParticleCenter,
				SandParticleSize,
				ParticleColor,
				DrawLayerId + 1);

			if (BurstAmount > 0.12f && Influence > 0.48f && HeadphoneNoiseHash01(Ripple.Seed, Index, 6.29f) < 0.32f)
			{
				FLinearColor FragmentColor = ParticleColor;
				const FVector2D FragmentOffset =
					ArcDirection * CurrentRingThickness * (HeadphoneNoiseHash01(Ripple.Seed, Index, 7.01f) * 0.7f - 0.35f) +
					ScreenRight * CurrentRingThickness * (HeadphoneNoiseHash01(Ripple.Seed, Index, 7.79f) * 1.1f - 0.55f);
				DrawCircleParticle(
					SandParticleCenter + FragmentOffset,
					FMath::Max(1.0f, SandParticleSize * 0.85f),
					FragmentColor,
					DrawLayerId + 1);
			}
		}

		if (bDrawDebugNoiseDirectionSolidCircle)
		{
			const float DebugFadeAlpha = FMath::Clamp(Ripple.ElapsedSeconds / DebugNoiseDirectionSolidCircleLifetime, 0.0f, 1.0f);
			const float DebugFade = 1.0f - SmoothTransitionAlpha(DebugFadeAlpha);
			if (DebugFade > KINDA_SMALL_NUMBER)
			{
				FLinearColor DebugCircleColor = HeadphoneDebugNoiseDirectionSolidCircleColor;
				DebugCircleColor.A *= DebugFade;
				const FVector2D DebugCircleCenter =
					RingCenter + ScreenDirection * DebugNoiseDirectionSolidCircleOrbitRadius;
				FSlateDrawElement::MakeBox(
					OutDrawElements,
					DrawLayerId + 2,
					MakeHudLocalBoxGeometry(
						AllottedGeometry,
						DebugCircleCenter - DebugNoiseDirectionSolidCircleSize * 0.5f,
						DebugNoiseDirectionSolidCircleSize),
					&DebugNoiseDirectionSolidCircleBrush,
					ESlateDrawEffect::None,
					DebugCircleColor);
			}
		}
	}

	InOutLayerId += 3;
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

