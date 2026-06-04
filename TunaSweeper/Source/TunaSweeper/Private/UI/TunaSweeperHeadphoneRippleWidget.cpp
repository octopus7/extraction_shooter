#include "UI/TunaSweeperHeadphoneRippleWidget.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "GameFramework/PlayerController.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"

namespace TunaSweeperHeadphoneRipple
{
	constexpr int32 MaxActiveRipples = 6;

	float Hash01(int32 Seed, int32 Index, float Salt)
	{
		const float RawValue = FMath::Sin(
			(static_cast<float>(Seed) + 1.0f) * 12.9898f +
			(static_cast<float>(Index) + 1.0f) * (37.719f + Salt) +
			Salt * 78.233f) * 43758.5453f;
		return RawValue - FMath::FloorToFloat(RawValue);
	}

	FPaintGeometry MakeLocalBoxGeometry(
		const FGeometry& AllottedGeometry,
		const FVector2D& Position,
		const FVector2D& Size)
	{
		return AllottedGeometry.ToPaintGeometry(
			FVector2f(static_cast<float>(Size.X), static_cast<float>(Size.Y)),
			FSlateLayoutTransform(FVector2f(static_cast<float>(Position.X), static_cast<float>(Position.Y))));
	}

	float CenterWeightedSignedUnit(float Value01, float Exponent)
	{
		const float SignedValue = Value01 * 2.0f - 1.0f;
		const float Magnitude = FMath::Pow(FMath::Abs(SignedValue), FMath::Max(1.0f, Exponent));
		return FMath::Sign(SignedValue) * Magnitude;
	}

	float AngularInfluence(float NormalizedAngle)
	{
		const float ClampedAngle = FMath::Clamp(NormalizedAngle, 0.0f, 1.0f);
		return FMath::Pow(1.0f - FMath::SmoothStep(0.0f, 1.0f, ClampedAngle), 1.35f);
	}
}

TSharedRef<SWidget> UTunaSweeperHeadphoneRippleWidget::RebuildWidget()
{
	EnsureNativeRoot();
	return Super::RebuildWidget();
}

void UTunaSweeperHeadphoneRippleWidget::NativeConstruct()
{
	Super::NativeConstruct();

	EnsureNativeRoot();
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UTunaSweeperHeadphoneRippleWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	const float DeltaSeconds = FMath::Max(0.0f, InDeltaTime);
	for (FScreenRipple& Ripple : ActiveRipples)
	{
		Ripple.ElapsedSeconds += DeltaSeconds;
	}

	ActiveRipples.RemoveAllSwap([this](const FScreenRipple& Ripple)
	{
		return Ripple.ElapsedSeconds >= FMath::Max(0.05f, RippleLifetimeSeconds);
	});

	if (ActiveRipples.Num() > 0)
	{
		Invalidate(EInvalidateWidgetReason::Paint);
	}
}

void UTunaSweeperHeadphoneRippleWidget::SetListenerActor(AActor* InListenerActor)
{
	ListenerActor = InListenerActor;
}

void UTunaSweeperHeadphoneRippleWidget::AddNoiseRipple(
	AActor* InListenerActor,
	const FVector& InDirectionFromListener,
	float InStrength)
{
	if (InListenerActor)
	{
		ListenerActor = InListenerActor;
	}

	FScreenRipple NewRipple;
	NewRipple.DirectionFromListener = InDirectionFromListener.GetSafeNormal2D();
	if (NewRipple.DirectionFromListener.IsNearlyZero())
	{
		NewRipple.DirectionFromListener = FVector::ForwardVector;
	}
	NewRipple.Strength = FMath::Clamp(InStrength, 0.0f, 1.0f);
	NewRipple.ElapsedSeconds = 0.0f;
	NewRipple.Seed = NextSeed++;
	if (NextSeed <= 0)
	{
		NextSeed = 1;
	}

	ActiveRipples.Add(NewRipple);
	if (ActiveRipples.Num() > TunaSweeperHeadphoneRipple::MaxActiveRipples)
	{
		ActiveRipples.RemoveAt(0, ActiveRipples.Num() - TunaSweeperHeadphoneRipple::MaxActiveRipples, EAllowShrinking::No);
	}

	Invalidate(EInvalidateWidgetReason::Paint);
}

int32 UTunaSweeperHeadphoneRippleWidget::NativePaint(
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

	const FVector2D LocalSize = AllottedGeometry.GetLocalSize();
	if (ActiveRipples.Num() == 0 || LocalSize.X <= 1.0f || LocalSize.Y <= 1.0f)
	{
		return PaintedLayerId;
	}

	FVector2D ListenerCenter = FVector2D::ZeroVector;
	if (!TryGetListenerScreenCenter(AllottedGeometry, ListenerCenter))
	{
		ListenerCenter = LocalSize * 0.5f;
	}

	const float ResolutionScale = FMath::Clamp(static_cast<float>(LocalSize.Y / 1080.0), 0.72f, 1.6f);
	const float Radius = FMath::Max(16.0f, RingRadiusAt1080p * ResolutionScale);
	const float RingThickness = FMath::Max(0.5f, RingThicknessPixels * ResolutionScale);
	const float ParticleSize = GetLocalPixelSize();
	const float Lifetime = FMath::Max(0.05f, RippleLifetimeSeconds);
	const int32 DrawLayerId = PaintedLayerId + 1;

	for (const FScreenRipple& Ripple : ActiveRipples)
	{
		const float Alpha = FMath::Clamp(Ripple.ElapsedSeconds / Lifetime, 0.0f, 1.0f);
		const float Fade = FMath::Sin(Alpha * PI);
		if (Fade <= KINDA_SMALL_NUMBER)
		{
			continue;
		}

		const FVector2D ScreenDirection = ResolveScreenDirection(AllottedGeometry, Ripple, ListenerCenter);
		const FVector2D ScreenRight(-ScreenDirection.Y, ScreenDirection.X);
		const float DirectionAngle = FMath::Atan2(ScreenDirection.Y, ScreenDirection.X);
		const float StrengthAlpha = FMath::Clamp(Ripple.Strength, 0.0f, 1.0f);

		const int32 RingCount = FMath::Clamp(BaseRingParticleCount, 0, 512);
		const FLinearColor BaseRingColor(
			RippleColor.R,
			RippleColor.G,
			RippleColor.B,
			0.08f * Fade * FMath::Lerp(0.55f, 1.0f, StrengthAlpha));
		for (int32 Index = 0; Index < RingCount; ++Index)
		{
			const float Angle = (static_cast<float>(Index) / static_cast<float>(RingCount)) * 2.0f * PI;
			const float Wave = FMath::Sin(Alpha * 2.0f * PI + Angle * 3.0f + Ripple.Seed * 0.17f);
			const float RadiusJitter = Wave * RingThickness * 0.22f;
			const FVector2D UnitDirection(FMath::Cos(Angle), FMath::Sin(Angle));
			const FVector2D ParticlePosition = ListenerCenter + UnitDirection * (Radius + RadiusJitter);
			DrawParticle(AllottedGeometry, OutDrawElements, DrawLayerId, ParticlePosition, ParticleSize, BaseRingColor);
		}

		const int32 SectorCount = FMath::RoundToInt(FMath::Lerp(
			static_cast<float>(FMath::Max(0, MinSectorParticleCount)),
			static_cast<float>(FMath::Max(MinSectorParticleCount, MaxSectorParticleCount)),
			StrengthAlpha));
		const float SectorHalfAngle = FMath::DegreesToRadians(FMath::Clamp(SectorHalfAngleDegrees, 1.0f, 90.0f));
		for (int32 Index = 0; Index < SectorCount; ++Index)
		{
			const float SignedAngleUnit = TunaSweeperHeadphoneRipple::CenterWeightedSignedUnit(
				TunaSweeperHeadphoneRipple::Hash01(Ripple.Seed, Index, 0.13f),
				1.85f);
			const float AngleOffset = SignedAngleUnit * SectorHalfAngle;
			const float NormalizedAngle = FMath::Abs(SignedAngleUnit);
			const float Influence = TunaSweeperHeadphoneRipple::AngularInfluence(NormalizedAngle);
			if (Influence <= 0.01f)
			{
				continue;
			}

			const float Angle = DirectionAngle + AngleOffset;
			const FVector2D ArcDirection(FMath::Cos(Angle), FMath::Sin(Angle));

			const float RadiusNoise = TunaSweeperHeadphoneRipple::Hash01(Ripple.Seed, Index, 1.73f) * 2.0f - 1.0f;
			const float PhaseNoise = TunaSweeperHeadphoneRipple::Hash01(Ripple.Seed, Index, 2.41f) * 2.0f * PI;
			const float Wobble =
				FMath::Sin(Alpha * 2.0f * PI * 1.65f + PhaseNoise) *
				RingThickness *
				FMath::Lerp(0.08f, 0.42f, Influence);
			const float RadiusOffset = RadiusNoise * RingThickness * FMath::Lerp(0.2f, 1.0f, Influence) + Wobble;
			const float TangentScatter =
				(TunaSweeperHeadphoneRipple::Hash01(Ripple.Seed, Index, 3.29f) * 2.0f - 1.0f) *
				RingThickness *
				FMath::Lerp(0.04f, 0.45f, Influence) *
				FMath::Pow(Alpha, 1.25f);
			const FVector2D ParticlePosition =
				ListenerCenter +
				ArcDirection * (Radius + RadiusOffset) +
				ScreenRight * TangentScatter;

			FLinearColor ParticleColor = RippleColor;
			ParticleColor.A =
				Fade *
				FMath::Lerp(0.34f, 0.82f, StrengthAlpha) *
				FMath::Lerp(0.18f, 1.0f, Influence);
			DrawParticle(AllottedGeometry, OutDrawElements, DrawLayerId + 1, ParticlePosition, ParticleSize, ParticleColor);
		}
	}

	return DrawLayerId + 1;
}

void UTunaSweeperHeadphoneRippleWidget::EnsureNativeRoot()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"), RF_Transient);
	}

	if (WidgetTree->RootWidget)
	{
		return;
	}

	NativeRootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(
		UCanvasPanel::StaticClass(),
		TEXT("HeadphoneRippleRoot"));
	if (!NativeRootCanvas)
	{
		return;
	}

	NativeRootCanvas->SetVisibility(ESlateVisibility::HitTestInvisible);
	WidgetTree->RootWidget = NativeRootCanvas;
}

bool UTunaSweeperHeadphoneRippleWidget::TryGetListenerScreenCenter(
	const FGeometry& AllottedGeometry,
	FVector2D& OutCenter) const
{
	const AActor* Listener = ListenerActor.Get();
	if (!Listener)
	{
		return false;
	}

	APlayerController* PlayerController = GetOwningPlayer();
	if (!PlayerController)
	{
		return false;
	}

	if (!UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(
		PlayerController,
		Listener->GetActorLocation(),
		OutCenter,
		true))
	{
		return false;
	}

	const FVector2D LocalSize = AllottedGeometry.GetLocalSize();
	OutCenter.X = FMath::Clamp(OutCenter.X, 0.0, LocalSize.X);
	OutCenter.Y = FMath::Clamp(OutCenter.Y, 0.0, LocalSize.Y);
	return true;
}

FVector2D UTunaSweeperHeadphoneRippleWidget::ResolveScreenDirection(
	const FGeometry& AllottedGeometry,
	const FScreenRipple& Ripple,
	const FVector2D& Center) const
{
	const AActor* Listener = ListenerActor.Get();
	APlayerController* PlayerController = GetOwningPlayer();
	if (Listener && PlayerController)
	{
		FVector2D DirectionTarget = FVector2D::ZeroVector;
		const FVector TargetLocation = Listener->GetActorLocation() + Ripple.DirectionFromListener.GetSafeNormal2D() * 240.0f;
		if (UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(
			PlayerController,
			TargetLocation,
			DirectionTarget,
			true))
		{
			const FVector2D ProjectedDirection = DirectionTarget - Center;
			if (!ProjectedDirection.IsNearlyZero())
			{
				return ProjectedDirection.GetSafeNormal();
			}
		}
	}

	const FVector Direction = Ripple.DirectionFromListener.GetSafeNormal2D();
	const FVector2D FallbackDirection(Direction.X, -Direction.Y);
	return FallbackDirection.IsNearlyZero() ? FVector2D(1.0f, 0.0f) : FallbackDirection.GetSafeNormal();
}

float UTunaSweeperHeadphoneRippleWidget::GetLocalPixelSize() const
{
	const float ViewportScale = FMath::Max(0.25f, UWidgetLayoutLibrary::GetViewportScale(this));
	return FMath::Max(0.25f, ParticlePixelSize / ViewportScale);
}

void UTunaSweeperHeadphoneRippleWidget::DrawParticle(
	const FGeometry& AllottedGeometry,
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId,
	const FVector2D& Center,
	float Size,
	const FLinearColor& Color) const
{
	if (Color.A <= 0.0f || Size <= 0.0f)
	{
		return;
	}

	const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
	if (!WhiteBrush)
	{
		return;
	}

	const FVector2D RoundedPosition(
		FMath::RoundToDouble(Center.X - Size * 0.5),
		FMath::RoundToDouble(Center.Y - Size * 0.5));
	const FVector2D ParticleSize(Size, Size);
	FSlateDrawElement::MakeBox(
		OutDrawElements,
		LayerId,
		TunaSweeperHeadphoneRipple::MakeLocalBoxGeometry(AllottedGeometry, RoundedPosition, ParticleSize),
		WhiteBrush,
		ESlateDrawEffect::None,
		Color);
}
