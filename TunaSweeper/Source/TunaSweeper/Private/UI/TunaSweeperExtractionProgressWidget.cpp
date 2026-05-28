#include "UI/TunaSweeperExtractionProgressWidget.h"

#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"

namespace
{
	FPaintGeometry MakeLocalBoxGeometry(
		const FGeometry& AllottedGeometry,
		const FVector2D& Position,
		const FVector2D& Size)
	{
		return AllottedGeometry.ToPaintGeometry(
			FVector2f(static_cast<float>(Size.X), static_cast<float>(Size.Y)),
			FSlateLayoutTransform(FVector2f(static_cast<float>(Position.X), static_cast<float>(Position.Y))));
	}
}

void UTunaSweeperExtractionProgressWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UTunaSweeperExtractionProgressWidget::SetExtractionProgress(
	float InCurrentSeconds,
	float InRequiredSeconds,
	bool bInVisible)
{
	CurrentSeconds = FMath::Max(0.0f, InCurrentSeconds);
	RequiredSeconds = FMath::Max(0.1f, InRequiredSeconds);
	bVisibleGauge = bInVisible;
	SetVisibility(bVisibleGauge ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	InvalidateLayoutAndVolatility();
}

int32 UTunaSweeperExtractionProgressWidget::NativePaint(
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

	if (!bVisibleGauge || !FSlateApplication::IsInitialized())
	{
		return PaintedLayerId;
	}

	const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
	if (!WhiteBrush || !FSlateApplication::Get().GetRenderer())
	{
		return PaintedLayerId;
	}

	const FVector2D LocalSize = AllottedGeometry.GetLocalSize();
	const float ClampedBorderThickness = FMath::Clamp(BorderThickness, 0.0f, FMath::Min(LocalSize.X, LocalSize.Y) * 0.35f);
	const FVector2D InnerPosition(ClampedBorderThickness, ClampedBorderThickness);
	const FVector2D InnerSize(
		FMath::Max(0.0f, LocalSize.X - ClampedBorderThickness * 2.0f),
		FMath::Max(0.0f, LocalSize.Y - ClampedBorderThickness * 2.0f));
	const float Progress = RequiredSeconds > 0.0f
		? FMath::Clamp(CurrentSeconds / RequiredSeconds, 0.0f, 1.0f)
		: 0.0f;
	const FVector2D FillSize(InnerSize.X * Progress, InnerSize.Y);

	FSlateDrawElement::MakeBox(
		OutDrawElements,
		PaintedLayerId + 1,
		AllottedGeometry.ToPaintGeometry(),
		WhiteBrush,
		ESlateDrawEffect::None,
		BorderColor);

	FSlateDrawElement::MakeBox(
		OutDrawElements,
		PaintedLayerId + 2,
		MakeLocalBoxGeometry(AllottedGeometry, InnerPosition, InnerSize),
		WhiteBrush,
		ESlateDrawEffect::None,
		EmptyColor);

	if (FillSize.X > 0.0f)
	{
		FSlateDrawElement::MakeBox(
			OutDrawElements,
			PaintedLayerId + 3,
			MakeLocalBoxGeometry(AllottedGeometry, InnerPosition, FillSize),
			WhiteBrush,
			ESlateDrawEffect::None,
			FillColor);
	}

	const FString TimeText = FString::Printf(TEXT("%.1fs"), CurrentSeconds);
	const FSlateFontInfo FontInfo = FCoreStyle::GetDefaultFontStyle(FName(TEXT("Bold")), FontSize);
	const TSharedRef<FSlateFontMeasure> FontMeasure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
	const FVector2D TextSize = FontMeasure->Measure(TimeText, FontInfo);
	const FVector2D TextPosition(
		FMath::Max(0.0f, (LocalSize.X - TextSize.X) * 0.5f),
		FMath::Max(0.0f, (LocalSize.Y - TextSize.Y) * 0.5f));
	const FVector2D ShadowOffset(1.0f, 1.0f);

	FSlateDrawElement::MakeText(
		OutDrawElements,
		PaintedLayerId + 4,
		MakeLocalBoxGeometry(AllottedGeometry, TextPosition + ShadowOffset, TextSize),
		TimeText,
		FontInfo,
		ESlateDrawEffect::None,
		TextShadowColor);

	FSlateDrawElement::MakeText(
		OutDrawElements,
		PaintedLayerId + 5,
		MakeLocalBoxGeometry(AllottedGeometry, TextPosition, TextSize),
		TimeText,
		FontInfo,
		ESlateDrawEffect::None,
		TextColor);

	return PaintedLayerId + 5;
}
