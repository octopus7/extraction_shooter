#include "UI/TunaSweeperTitleWindParticleWidget.h"

#include "Rendering/DrawElements.h"

namespace TunaSweeperTitleWindParticles
{
	constexpr int32 ParticleCount = 48;

	float Hash01(int32 Seed, float Salt)
	{
		return FMath::Frac(FMath::Sin((static_cast<float>(Seed) + 1.0f) * (12.9898f + Salt) + Salt * 78.233f) * 43758.5453f);
	}

	FLinearColor GetParticleColor(float Blend, float Alpha)
	{
		const FLinearColor Cyan(0.02f, 0.94f, 0.86f, Alpha);
		const FLinearColor Green(0.22f, 0.92f, 0.36f, Alpha);
		return FMath::Lerp(Cyan, Green, Blend);
	}
}

void UTunaSweeperTitleWindParticleWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	AnimationSeconds += FMath::Max(0.0f, InDeltaTime);
	Invalidate(EInvalidateWidgetReason::Paint);
}

int32 UTunaSweeperTitleWindParticleWidget::NativePaint(
	const FPaintArgs& Args,
	const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId,
	const FWidgetStyle& InWidgetStyle,
	bool bParentEnabled) const
{
	const int32 BaseLayerId = Super::NativePaint(
		Args,
		AllottedGeometry,
		MyCullingRect,
		OutDrawElements,
		LayerId,
		InWidgetStyle,
		bParentEnabled);

	const FVector2D LocalSize = AllottedGeometry.GetLocalSize();
	if (LocalSize.X <= 1.0f || LocalSize.Y <= 1.0f)
	{
		return BaseLayerId;
	}

	const FVector2f WindDirection(1.0f, -0.16f);
	for (int32 Index = 0; Index < TunaSweeperTitleWindParticles::ParticleCount; ++Index)
	{
		const float BaseX = TunaSweeperTitleWindParticles::Hash01(Index, 0.13f);
		const float BaseY = TunaSweeperTitleWindParticles::Hash01(Index, 1.71f);
		const float Speed = FMath::Lerp(0.018f, 0.048f, TunaSweeperTitleWindParticles::Hash01(Index, 2.31f));
		const float Depth = TunaSweeperTitleWindParticles::Hash01(Index, 3.37f);
		const float Travel = FMath::Frac(BaseX + AnimationSeconds * Speed);
		const float SwayPhase = AnimationSeconds * FMath::Lerp(0.7f, 1.65f, Depth) + BaseX * 2.0f * PI;
		const float Sway = FMath::Sin(SwayPhase) * FMath::Lerp(16.0f, 52.0f, Depth);

		const FVector2f Head(
			static_cast<float>(FMath::Lerp(-140.0, LocalSize.X + 140.0, Travel)),
			static_cast<float>(BaseY * LocalSize.Y + Sway - Travel * 110.0f));
		const float Length = FMath::Lerp(2.0f, 7.0f, Depth);
		const FVector2f Tail = Head - WindDirection * Length;

		TArray<FVector2f> Points;
		Points.Reserve(2);
		Points.Add(Tail);
		Points.Add(Head);

		const float Pulse = 0.55f + 0.45f * FMath::Sin(SwayPhase * 1.7f);
		const float Alpha = FMath::Lerp(0.07f, 0.22f, Depth) * Pulse;
		const float Thickness = FMath::Lerp(1.6f, 3.0f, Depth);
		const FLinearColor ParticleColor = TunaSweeperTitleWindParticles::GetParticleColor(
			TunaSweeperTitleWindParticles::Hash01(Index, 4.03f),
			Alpha);

		FSlateDrawElement::MakeLines(
			OutDrawElements,
			BaseLayerId + 1,
			AllottedGeometry.ToPaintGeometry(),
			Points,
			ESlateDrawEffect::None,
			ParticleColor,
			true,
			Thickness);
	}

	return BaseLayerId + 1;
}
