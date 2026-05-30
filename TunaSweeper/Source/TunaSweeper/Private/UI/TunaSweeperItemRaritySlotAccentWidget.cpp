#include "UI/TunaSweeperItemRaritySlotAccentWidget.h"

#include "Framework/Application/SlateApplication.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"

namespace TunaSweeperItemRaritySlotAccent
{
	struct FItemGradeStyle
	{
		FLinearColor TopColor;
		FLinearColor BottomColor;
		FLinearColor AccentColor;
		bool bShowAccent = false;
	};

	FItemGradeStyle ResolveItemGradeStyle(ETunaSweeperItemGrade ItemGrade)
	{
		switch (ItemGrade)
		{
		case ETunaSweeperItemGrade::Uncommon:
			return {
				FLinearColor(0.16f, 1.00f, 0.42f, 0.24f),
				FLinearColor(0.05f, 0.62f, 0.34f, 0.00f),
				FLinearColor::Transparent,
				false
			};
		case ETunaSweeperItemGrade::Rare:
			return {
				FLinearColor(0.08f, 0.78f, 1.00f, 0.42f),
				FLinearColor(0.06f, 0.26f, 0.95f, 0.00f),
				FLinearColor(0.10f, 0.78f, 1.00f, 0.94f),
				true
			};
		case ETunaSweeperItemGrade::Epic:
			return {
				FLinearColor(0.68f, 0.28f, 1.00f, 0.48f),
				FLinearColor(0.90f, 0.18f, 1.00f, 0.00f),
				FLinearColor(0.76f, 0.30f, 1.00f, 0.96f),
				true
			};
		case ETunaSweeperItemGrade::Legendary:
			return {
				FLinearColor(1.00f, 0.68f, 0.12f, 0.54f),
				FLinearColor(1.00f, 0.30f, 0.04f, 0.00f),
				FLinearColor(1.00f, 0.72f, 0.16f, 1.00f),
				true
			};
		case ETunaSweeperItemGrade::Common:
		default:
			return {
				FLinearColor(0.70f, 0.75f, 0.78f, 0.12f),
				FLinearColor(0.40f, 0.46f, 0.50f, 0.00f),
				FLinearColor::Transparent,
				false
			};
		}
	}

	void AddGradientQuad(
		const FSlateRenderTransform& RenderTransform,
		const FVector2D& Position,
		const FVector2D& Size,
		const FLinearColor& TopColor,
		const FLinearColor& BottomColor,
		TArray<FSlateVertex>& OutVertices,
		TArray<SlateIndex>& OutIndices)
	{
		const SlateIndex BaseIndex = static_cast<SlateIndex>(OutVertices.Num());
		const FColor TopVertexColor = TopColor.ToFColor(true);
		const FColor BottomVertexColor = BottomColor.ToFColor(true);
		const FVector2D TopLeft = Position;
		const FVector2D TopRight = Position + FVector2D(Size.X, 0.0f);
		const FVector2D BottomLeft = Position + FVector2D(0.0f, Size.Y);
		const FVector2D BottomRight = Position + Size;

		OutVertices.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(
			RenderTransform,
			FVector2f(TopLeft),
			FVector2f::ZeroVector,
			TopVertexColor));
		OutVertices.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(
			RenderTransform,
			FVector2f(TopRight),
			FVector2f::ZeroVector,
			TopVertexColor));
		OutVertices.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(
			RenderTransform,
			FVector2f(BottomLeft),
			FVector2f::ZeroVector,
			BottomVertexColor));
		OutVertices.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(
			RenderTransform,
			FVector2f(BottomRight),
			FVector2f::ZeroVector,
			BottomVertexColor));

		OutIndices.Add(BaseIndex);
		OutIndices.Add(BaseIndex + 1);
		OutIndices.Add(BaseIndex + 2);
		OutIndices.Add(BaseIndex + 1);
		OutIndices.Add(BaseIndex + 3);
		OutIndices.Add(BaseIndex + 2);
	}
}

void UTunaSweeperItemRaritySlotAccentWidget::SetItemGrade(
	ETunaSweeperItemGrade InItemGrade,
	bool bInVisible)
{
	ItemGrade = InItemGrade;
	bRarityVisible = bInVisible;
	SetVisibility(bRarityVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	InvalidateLayoutAndVolatility();
}

int32 UTunaSweeperItemRaritySlotAccentWidget::NativePaint(
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

	if (!bRarityVisible || !FSlateApplication::IsInitialized())
	{
		return PaintedLayerId;
	}

	const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
	if (!WhiteBrush || !FSlateApplication::Get().GetRenderer())
	{
		return PaintedLayerId;
	}

	const FVector2D LocalSize = AllottedGeometry.GetLocalSize();
	if (LocalSize.X <= 2.0f || LocalSize.Y <= 2.0f)
	{
		return PaintedLayerId;
	}

	const TunaSweeperItemRaritySlotAccent::FItemGradeStyle GradeStyle =
		TunaSweeperItemRaritySlotAccent::ResolveItemGradeStyle(ItemGrade);
	const FSlateRenderTransform& AccumulatedRenderTransform = AllottedGeometry.GetAccumulatedRenderTransform();
	TArray<FSlateVertex> Vertices;
	TArray<SlateIndex> Indices;
	Vertices.Reserve(12);
	Indices.Reserve(18);

	constexpr float SlotInset = 0.0f;
	const FVector2D FillPosition(SlotInset, SlotInset);
	const FVector2D FillSize(
		FMath::Max(1.0f, LocalSize.X - SlotInset * 2.0f),
		FMath::Max(1.0f, LocalSize.Y - SlotInset * 2.0f));
	TunaSweeperItemRaritySlotAccent::AddGradientQuad(
		AccumulatedRenderTransform,
		FillPosition,
		FillSize,
		GradeStyle.TopColor,
		GradeStyle.BottomColor,
		Vertices,
		Indices);

	if (GradeStyle.bShowAccent)
	{
		constexpr float AccentHeight = 3.0f;
		constexpr float AccentGlowHeight = 9.0f;
		const FLinearColor AccentGlowColor = GradeStyle.AccentColor.CopyWithNewOpacity(GradeStyle.AccentColor.A * 0.24f);
		TunaSweeperItemRaritySlotAccent::AddGradientQuad(
			AccumulatedRenderTransform,
			FillPosition,
			FVector2D(FillSize.X, AccentGlowHeight),
			AccentGlowColor,
			AccentGlowColor.CopyWithNewOpacity(0.0f),
			Vertices,
			Indices);
		TunaSweeperItemRaritySlotAccent::AddGradientQuad(
			AccumulatedRenderTransform,
			FillPosition,
			FVector2D(FillSize.X, AccentHeight),
			GradeStyle.AccentColor,
			GradeStyle.AccentColor,
			Vertices,
			Indices);
	}

	FSlateDrawElement::MakeCustomVerts(
		OutDrawElements,
		PaintedLayerId,
		FSlateApplication::Get().GetRenderer()->GetResourceHandle(*WhiteBrush),
		Vertices,
		Indices,
		nullptr,
		0,
		0);

	return PaintedLayerId;
}
