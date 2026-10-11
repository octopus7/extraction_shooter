#include "LoopRailArrivalWidget.h"

#include "Brushes/SlateRoundedBoxBrush.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"

namespace LoopRailArrivalPaint
{
	struct FGlyph
	{
		uint8 Rows[7];
		int32 Width = 5;
	};

	const FGlyph& Glyph(TCHAR Character)
	{
		// Five-column bitmap typeface; these masks are glyph shapes, not UI text.
		static const FGlyph Digits[] = {
			{{14, 17, 19, 21, 25, 17, 14}},
			{{4, 12, 4, 4, 4, 4, 14}},
			{{14, 17, 1, 2, 4, 8, 31}},
			{{30, 1, 1, 14, 1, 1, 30}},
			{{2, 6, 10, 18, 31, 2, 2}},
			{{31, 16, 16, 30, 1, 1, 30}},
			{{14, 16, 16, 30, 17, 17, 14}},
			{{31, 1, 2, 4, 8, 8, 8}},
			{{14, 17, 17, 14, 17, 17, 14}},
			{{14, 17, 17, 15, 1, 1, 14}}
		};
		static const FGlyph Colon = {{0, 0, 2, 0, 2, 0, 0}, 3};
		static const FGlyph Dash = {{0, 0, 0, 31, 0, 0, 0}};
		static const FGlyph Blank = {{0, 0, 0, 0, 0, 0, 0}, 3};
		if (Character >= TEXT('0') && Character <= TEXT('9')) return Digits[Character - TEXT('0')];
		if (Character == TEXT(':')) return Colon;
		if (Character == TEXT('-')) return Dash;
		return Blank;
	}

	void AddDisc(const FVector2D& Center, float Radius, const FLinearColor& Color,
		const FSlateRenderTransform& Transform, TArray<FSlateVertex>& Vertices, TArray<SlateIndex>& Indices)
	{
		constexpr int32 Segments = 16;
		const SlateIndex First = static_cast<SlateIndex>(Vertices.Num());
		const FColor VertexColor = Color.ToFColor(true);
		Vertices.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(
			Transform, FVector2f(Center), FVector2f::ZeroVector, VertexColor));
		for (int32 Index = 0; Index < Segments; ++Index)
		{
			const float Angle = 2.0f * PI * Index / Segments;
			const FVector2D Point = Center + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Radius;
			Vertices.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(
				Transform, FVector2f(Point), FVector2f::ZeroVector, VertexColor));
			Indices.Add(First);
			Indices.Add(static_cast<SlateIndex>(First + 1 + Index));
			Indices.Add(static_cast<SlateIndex>(First + 1 + (Index + 1) % Segments));
		}
	}
}

void ULoopRailArrivalWidget::SetArrivalDisplayText(const FText& InText)
{
	if (ArrivalDisplayText.IdenticalTo(InText)) return;
	ArrivalDisplayText = InText;
	InvalidateLayoutAndVolatility();
}

int32 ULoopRailArrivalWidget::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
	int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const int32 BaseLayer = Super::NativePaint(Args, AllottedGeometry, MyCullingRect,
		OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
	const FVector2D Size = AllottedGeometry.GetLocalSize();
	if (Size.X <= 0 || Size.Y <= 0 || !FSlateApplication::IsInitialized()) return BaseLayer;

	static const FSlateRoundedBoxBrush Panel(FLinearColor::Black, 9.0f,
		FLinearColor(0.035f, 0.045f, 0.06f, 1.0f), 1.5f);
	FSlateDrawElement::MakeBox(OutDrawElements, BaseLayer + 1, AllottedGeometry.ToPaintGeometry(),
		&Panel, ESlateDrawEffect::None, Panel.GetTint(InWidgetStyle) * InWidgetStyle.GetColorAndOpacityTint());

	const FString Text = ArrivalDisplayText.ToString();
	if (Text.IsEmpty() || !FSlateApplication::Get().GetRenderer()) return BaseLayer + 1;
	int32 Columns = -2;
	for (TCHAR Character : Text) Columns += LoopRailArrivalPaint::Glyph(Character).Width + 2;
	if (Columns <= 0) return BaseLayer + 1;

	// The lower half stays empty. A long minute count shrinks uniformly to fit.
	const float Scale = FMath::Min(Size.X / 460.0f, Size.Y / 220.0f);
	const float Pitch = FMath::Min(12.0f * Scale, (Size.X - 48.0f * Scale) / Columns);
	const float Left = (Size.X - Columns * Pitch) * 0.5f;
	const float Top = 20.0f * Scale;
	const FLinearColor Color = DotColor * InWidgetStyle.GetColorAndOpacityTint();
	FLinearColor HaloColor = Color;
	HaloColor.A *= 0.12f;
	const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
	const FSlateResourceHandle Resource = FSlateApplication::Get().GetRenderer()->GetResourceHandle(*WhiteBrush);
	const FSlateRenderTransform& Transform = AllottedGeometry.GetAccumulatedRenderTransform();
	TArray<FSlateVertex> Vertices;
	TArray<SlateIndex> Indices;
	Vertices.Reserve(5000);
	Indices.Reserve(14000);
	auto Flush = [&]()
	{
		if (Vertices.IsEmpty()) return;
		FSlateDrawElement::MakeCustomVerts(OutDrawElements, BaseLayer + 2, Resource, Vertices, Indices, nullptr, 0, 0);
		Vertices.Reset();
		Indices.Reset();
	};
	int32 ColumnOffset = 0;
	for (TCHAR Character : Text)
	{
		const LoopRailArrivalPaint::FGlyph& Glyph = LoopRailArrivalPaint::Glyph(Character);
		for (int32 Row = 0; Row < 7; ++Row)
		{
			for (int32 Column = 0; Column < Glyph.Width; ++Column)
			{
				if (!(Glyph.Rows[Row] & (1 << (Glyph.Width - 1 - Column)))) continue;
				if (Vertices.Num() > 60000) Flush();
				const FVector2D Center(Left + (ColumnOffset + Column + 0.5f) * Pitch, Top + (Row + 0.5f) * Pitch);
				LoopRailArrivalPaint::AddDisc(Center, Pitch * 0.47f, HaloColor, Transform, Vertices, Indices);
				LoopRailArrivalPaint::AddDisc(Center, Pitch * 0.32f, Color, Transform, Vertices, Indices);
			}
		}
		ColumnOffset += Glyph.Width + 2;
	}
	Flush();
	return BaseLayer + 2;
}
