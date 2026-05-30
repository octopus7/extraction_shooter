#include "UI/TunaSweeperItemHoverBorderEffectWidget.h"

#include "Rendering/DrawElements.h"

namespace TunaSweeperItemHoverBorderEffect
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

void UTunaSweeperItemHoverBorderEffectWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UTunaSweeperItemHoverBorderEffectWidget::SetHoverBorderEffectActive(bool bInActive)
{
	if (bEffectActive == bInActive)
	{
		return;
	}

	bEffectActive = bInActive;
	if (bEffectActive && EffectOpacity <= 0.0f)
	{
		AnimationSeconds = 0.0f;
	}
	Invalidate(EInvalidateWidgetReason::Paint);
}

void UTunaSweeperItemHoverBorderEffectWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!bEffectActive && EffectOpacity <= 0.0f)
	{
		return;
	}

	AnimationSeconds += FMath::Max(0.0f, InDeltaTime);
	const float TargetOpacity = bEffectActive ? 1.0f : 0.0f;
	const float InterpSpeed = bEffectActive ? 16.0f : 12.0f;
	EffectOpacity = FMath::FInterpTo(
		EffectOpacity,
		TargetOpacity,
		FMath::Max(0.0f, InDeltaTime),
		InterpSpeed);
	if (!bEffectActive && EffectOpacity < 0.01f)
	{
		EffectOpacity = 0.0f;
	}

	Invalidate(EInvalidateWidgetReason::Paint);
}

int32 UTunaSweeperItemHoverBorderEffectWidget::NativePaint(
	const FPaintArgs& Args,
	const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId,
	const FWidgetStyle& InWidgetStyle,
	bool bParentEnabled) const
{
	using namespace TunaSweeperItemHoverBorderEffect;

	const int32 PaintedLayerId = Super::NativePaint(
		Args,
		AllottedGeometry,
		MyCullingRect,
		OutDrawElements,
		LayerId,
		InWidgetStyle,
		bParentEnabled);

	if (EffectOpacity <= 0.0f)
	{
		return PaintedLayerId;
	}

	const FVector2D LocalSize = AllottedGeometry.GetLocalSize();
	if (LocalSize.X <= 12.0f || LocalSize.Y <= 12.0f)
	{
		return PaintedLayerId;
	}

	const float Opacity = FMath::Clamp(EffectOpacity, 0.0f, 1.0f);
	constexpr float Inset = 2.0f;
	const FVector2D Min(Inset, Inset);
	const FVector2D Max(
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
		PaintedLayerId + 1,
		AllottedGeometry,
		Points,
		FLinearColor(0.10f, 0.70f, 0.82f, 0.15f * Opacity),
		1.0f);

	const float CycleAlpha = FMath::Frac(AnimationSeconds / 2.32f);
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
				PaintedLayerId + 2,
				AllottedGeometry,
				Points,
				FLinearColor(0.04f, 0.90f, 1.00f, 0.10f * Envelope),
				4.4f);
			DrawLineStrip(
				OutDrawElements,
				PaintedLayerId + 3,
				AllottedGeometry,
				Points,
				FLinearColor(0.12f, 0.88f, 1.00f, 0.42f * Envelope),
				1.45f);

			BuildBorderRouteSegment(Min, Max, Route, LeadAlpha, HeadAlpha, Points);
			DrawLineStrip(
				OutDrawElements,
				PaintedLayerId + 3,
				AllottedGeometry,
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
			PaintedLayerId + 3,
			AllottedGeometry,
			Points,
			FLinearColor(0.55f, 0.96f, 1.00f, CornerAlpha),
			2.2f);
		DrawLineStrip(
			OutDrawElements,
			PaintedLayerId + 2,
			AllottedGeometry,
			Points,
			FLinearColor(0.04f, 0.82f, 1.00f, CornerAlpha * 0.22f),
			5.2f);
	}

	return PaintedLayerId + 3;
}
