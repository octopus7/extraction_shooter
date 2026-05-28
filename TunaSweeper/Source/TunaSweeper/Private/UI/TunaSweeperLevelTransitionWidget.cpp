#include "UI/TunaSweeperLevelTransitionWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Framework/Application/SlateApplication.h"
#include "MediaTexture.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "UI/TunaSweeperUIFont.h"

namespace
{
	constexpr float LetterboxPanelRatio = 0.10f;
	constexpr int32 VideoZOrder = 0;
	constexpr int32 LetterboxZOrder = 10;
	constexpr int32 MessageZOrder = 20;
	constexpr int32 BlackFadeZOrder = 30;
	constexpr int32 CircularRevealSegmentCount = 192;

	void ConfigureLetterboxSlot(UCanvasPanelSlot* Slot, bool bTop)
	{
		if (!Slot)
		{
			return;
		}

		Slot->SetAnchors(bTop
			? FAnchors(0.0f, 0.0f, 1.0f, LetterboxPanelRatio)
			: FAnchors(0.0f, 1.0f - LetterboxPanelRatio, 1.0f, 1.0f));
		Slot->SetOffsets(FMargin(0.0f));
		Slot->SetAlignment(FVector2D::ZeroVector);
		Slot->SetZOrder(LetterboxZOrder);
	}

	void SetCanvasZOrder(UWidget* Widget, int32 ZOrder)
	{
		if (UCanvasPanelSlot* CanvasSlot = Widget ? Cast<UCanvasPanelSlot>(Widget->Slot) : nullptr)
		{
			CanvasSlot->SetZOrder(ZOrder);
		}
	}
}

void UTunaSweeperLevelTransitionWidget::NativeConstruct()
{
	Super::NativeConstruct();
	TunaSweeperUIFont::ApplyFontToWidgetTree(this);

	EnsureLetterboxPanels();
	SetCanvasZOrder(VideoImage, VideoZOrder);
	SetCanvasZOrder(MessageBackground, MessageZOrder);
	SetCanvasZOrder(BlackFadePanel, BlackFadeZOrder);

	SetVideoVisible(false);
	SetBlackOpacity(0.0f);
	SetCircularRevealMask(0.0f, false);
	SetLetterboxEnabled(false);
	SetTransitionMessage(FText::GetEmpty());
}

void UTunaSweeperLevelTransitionWidget::SetVideoTexture(UMediaTexture* InMediaTexture)
{
	if (!VideoImage)
	{
		return;
	}

	FSlateBrush VideoBrush;
	VideoBrush.DrawAs = ESlateBrushDrawType::Image;
	VideoBrush.SetResourceObject(InMediaTexture);
	VideoBrush.SetImageSize(FVector2D(1920.0f, 1080.0f));
	VideoImage->SetBrush(VideoBrush);
}

void UTunaSweeperLevelTransitionWidget::SetVideoVisible(bool bVisible)
{
	bVideoVisible = bVisible;
	if (VideoImage)
	{
		VideoImage->SetVisibility(bVisible ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}
	UpdateLetterboxVisibility();
}

void UTunaSweeperLevelTransitionWidget::SetBlackOpacity(float InOpacity)
{
	if (!BlackFadePanel)
	{
		return;
	}

	BlackFadePanel->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	BlackFadePanel->SetRenderOpacity(FMath::Clamp(InOpacity, 0.0f, 1.0f));
}

void UTunaSweeperLevelTransitionWidget::SetCircularRevealMask(float HoleRadiusPixels, bool bVisible)
{
	CircularRevealHoleRadiusPixels = FMath::Max(0.0f, HoleRadiusPixels);
	bCircularRevealMaskVisible = bVisible;
	InvalidateLayoutAndVolatility();
}

void UTunaSweeperLevelTransitionWidget::SetLetterboxEnabled(bool bEnabled)
{
	bLetterboxEnabled = bEnabled;
	EnsureLetterboxPanels();
	UpdateLetterboxVisibility();
}

void UTunaSweeperLevelTransitionWidget::SetTransitionMessage(const FText& InMessage)
{
	if (!TransitionMessageText)
	{
		return;
	}

	TransitionMessageText->SetText(InMessage);
	const ESlateVisibility MessageVisibility = InMessage.IsEmpty()
		? ESlateVisibility::Collapsed
		: ESlateVisibility::SelfHitTestInvisible;

	TransitionMessageText->SetVisibility(MessageVisibility);
	if (MessageBackground)
	{
		MessageBackground->SetVisibility(MessageVisibility);
	}
}

int32 UTunaSweeperLevelTransitionWidget::NativePaint(
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

	if (!bCircularRevealMaskVisible)
	{
		return PaintedLayerId;
	}

	DrawCircularRevealMask(AllottedGeometry, OutDrawElements, PaintedLayerId + 1);
	return PaintedLayerId + 1;
}

void UTunaSweeperLevelTransitionWidget::EnsureLetterboxPanels()
{
	if (LetterboxTopPanel && LetterboxBottomPanel)
	{
		return;
	}

	if (!WidgetTree)
	{
		return;
	}

	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
	if (!RootCanvas)
	{
		return;
	}

	FSlateBrush LetterboxBrush;
	LetterboxBrush.DrawAs = ESlateBrushDrawType::Box;
	LetterboxBrush.TintColor = FSlateColor(FLinearColor::Black);
	LetterboxBrush.SetImageSize(FVector2D(1920.0f, 108.0f));

	if (!LetterboxTopPanel)
	{
		LetterboxTopPanel = WidgetTree->ConstructWidget<UBorder>(
			UBorder::StaticClass(),
			TEXT("LetterboxTopPanel"));
		if (LetterboxTopPanel)
		{
			LetterboxTopPanel->SetBrush(LetterboxBrush);
			LetterboxTopPanel->SetVisibility(ESlateVisibility::Collapsed);
			ConfigureLetterboxSlot(RootCanvas->AddChildToCanvas(LetterboxTopPanel), true);
		}
	}

	if (!LetterboxBottomPanel)
	{
		LetterboxBottomPanel = WidgetTree->ConstructWidget<UBorder>(
			UBorder::StaticClass(),
			TEXT("LetterboxBottomPanel"));
		if (LetterboxBottomPanel)
		{
			LetterboxBottomPanel->SetBrush(LetterboxBrush);
			LetterboxBottomPanel->SetVisibility(ESlateVisibility::Collapsed);
			ConfigureLetterboxSlot(RootCanvas->AddChildToCanvas(LetterboxBottomPanel), false);
		}
	}
}

void UTunaSweeperLevelTransitionWidget::DrawCircularRevealMask(
	const FGeometry& AllottedGeometry,
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId) const
{
	if (!FSlateApplication::IsInitialized())
	{
		return;
	}

	const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
	if (!WhiteBrush || !FSlateApplication::Get().GetRenderer())
	{
		return;
	}

	const FVector2D LocalSize = AllottedGeometry.GetLocalSize();
	if (LocalSize.X <= 0.0f || LocalSize.Y <= 0.0f)
	{
		return;
	}

	const FColor MaskColor = FLinearColor::Black.ToFColor(true);
	if (CircularRevealHoleRadiusPixels <= KINDA_SMALL_NUMBER)
	{
		FSlateDrawElement::MakeBox(
			OutDrawElements,
			LayerId,
			AllottedGeometry.ToPaintGeometry(),
			WhiteBrush,
			ESlateDrawEffect::None,
			FLinearColor::Black);
		return;
	}

	const FVector2D Center(LocalSize.X * 0.5f, LocalSize.Y * 0.5f);
	const float OuterRadius = FMath::Sqrt(FMath::Square(LocalSize.X) + FMath::Square(LocalSize.Y)) * 0.5f + 96.0f;
	const float InnerRadius = FMath::Min(CircularRevealHoleRadiusPixels, OuterRadius);
	if (InnerRadius >= OuterRadius - KINDA_SMALL_NUMBER)
	{
		return;
	}

	CircularRevealVertices.Reset();
	CircularRevealIndices.Reset();
	CircularRevealVertices.Reserve((CircularRevealSegmentCount + 1) * 2);
	CircularRevealIndices.Reserve(CircularRevealSegmentCount * 6);

	const FSlateRenderTransform& AccumulatedRenderTransform = AllottedGeometry.GetAccumulatedRenderTransform();
	for (int32 SegmentIndex = 0; SegmentIndex <= CircularRevealSegmentCount; ++SegmentIndex)
	{
		const float Alpha = static_cast<float>(SegmentIndex) / static_cast<float>(CircularRevealSegmentCount);
		const float Angle = Alpha * 2.0f * PI;
		const FVector2D Direction(FMath::Cos(Angle), FMath::Sin(Angle));
		const FVector2D OuterPoint = Center + Direction * OuterRadius;
		const FVector2D InnerPoint = Center + Direction * InnerRadius;

		CircularRevealVertices.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(
			AccumulatedRenderTransform,
			FVector2f(OuterPoint),
			FVector2f::ZeroVector,
			MaskColor));
		CircularRevealVertices.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(
			AccumulatedRenderTransform,
			FVector2f(InnerPoint),
			FVector2f::ZeroVector,
			MaskColor));
	}

	for (int32 SegmentIndex = 0; SegmentIndex < CircularRevealSegmentCount; ++SegmentIndex)
	{
		const SlateIndex A = static_cast<SlateIndex>(SegmentIndex * 2);
		const SlateIndex B = static_cast<SlateIndex>(A + 1);
		const SlateIndex C = static_cast<SlateIndex>(A + 2);
		const SlateIndex D = static_cast<SlateIndex>(A + 3);

		CircularRevealIndices.Add(A);
		CircularRevealIndices.Add(C);
		CircularRevealIndices.Add(B);
		CircularRevealIndices.Add(C);
		CircularRevealIndices.Add(D);
		CircularRevealIndices.Add(B);
	}

	FSlateDrawElement::MakeCustomVerts(
		OutDrawElements,
		LayerId,
		FSlateApplication::Get().GetRenderer()->GetResourceHandle(*WhiteBrush),
		CircularRevealVertices,
		CircularRevealIndices,
		nullptr,
		0,
		0);
}

void UTunaSweeperLevelTransitionWidget::UpdateLetterboxVisibility()
{
	const ESlateVisibility LetterboxVisibility = bVideoVisible && bLetterboxEnabled
		? ESlateVisibility::SelfHitTestInvisible
		: ESlateVisibility::Collapsed;

	if (LetterboxTopPanel)
	{
		LetterboxTopPanel->SetVisibility(LetterboxVisibility);
	}
	if (LetterboxBottomPanel)
	{
		LetterboxBottomPanel->SetVisibility(LetterboxVisibility);
	}
}
