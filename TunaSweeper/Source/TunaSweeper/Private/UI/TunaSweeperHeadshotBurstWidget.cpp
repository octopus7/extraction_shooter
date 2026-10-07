#include "UI/TunaSweeperHeadshotBurstWidget.h"

#include "Framework/Application/SlateApplication.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"

int32 UTunaSweeperHeadshotBurstWidget::NativePaint(const FPaintArgs& Args, const FGeometry& Geometry,
	const FSlateRect& CullingRect, FSlateWindowElementList& Elements, int32 LayerId,
	const FWidgetStyle& Style, bool bParentEnabled) const
{
	const int32 BaseLayer = Super::NativePaint(Args, Geometry, CullingRect, Elements, LayerId, Style, bParentEnabled);
	if (!FSlateApplication::IsInitialized() || !FSlateApplication::Get().GetRenderer()) return BaseLayer;
	const FVector2f Size(Geometry.GetLocalSize());
	if (Size.X <= 0 || Size.Y <= 0) return BaseLayer;
	const FSlateRenderTransform& Transform = Geometry.GetAccumulatedRenderTransform();
	const FLinearColor Tint = Style.GetColorAndOpacityTint();
	TArray<FSlateVertex> Vertices;
	TArray<SlateIndex> Indices;
	Vertices.Reserve(100);
	Indices.Reserve(260);
	const FVector2f Points[] = {
		{-.48f,-.08f}, {-.30f,-.19f}, {-.38f,-.42f}, {-.13f,-.30f}, {-.07f,-.49f},
		{.08f,-.30f}, {.29f,-.43f}, {.27f,-.19f}, {.48f,-.14f}, {.35f,.03f},
		{.47f,.25f}, {.22f,.21f}, {.18f,.47f}, {0,.30f}, {-.20f,.42f}, {-.22f,.22f},
		{-.47f,.28f}, {-.35f,.08f}
	};
	auto AddFan = [&](float Scale, FVector2f Offset, FLinearColor CenterColor, FLinearColor EdgeColor)
	{
		const SlateIndex Base = static_cast<SlateIndex>(Vertices.Num());
		const FVector2f Center = Size * .5f + Offset;
		Vertices.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(Transform, Center,
			FVector2f::ZeroVector, (CenterColor * Tint).ToFColor(true)));
		for (const FVector2f& Point : Points)
			Vertices.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(Transform,
				Center + Point * Size * Scale, FVector2f::ZeroVector, (EdgeColor * Tint).ToFColor(true)));
		for (int32 Index = 0; Index < UE_ARRAY_COUNT(Points); ++Index)
		{
			Indices.Add(Base);
			Indices.Add(Base + 1 + Index);
			Indices.Add(Base + 1 + (Index + 1) % UE_ARRAY_COUNT(Points));
		}
	};
	// A dark lower rim, deep red silhouette, then a vivid red inset.
	AddFan(1.0f, FVector2f(2, 4), FLinearColor(.045f,.003f,.008f), FLinearColor(.045f,.003f,.008f));
	AddFan(.96f, FVector2f::ZeroVector, FLinearColor(.48f,.008f,.018f), FLinearColor(.28f,.003f,.009f));
	AddFan(.87f, FVector2f(-1,-2), FLinearColor(1.0f,.075f,.04f), FLinearColor(.70f,.015f,.025f));
	FSlateDrawElement::MakeCustomVerts(Elements, BaseLayer + 1,
		FSlateApplication::Get().GetRenderer()->GetResourceHandle(*FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"))),
		Vertices, Indices, nullptr, 0, 0);
	return BaseLayer + 1;
}
