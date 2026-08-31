#include "UI/TunaSweeperSpeechBubbleLayerWidget.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "UI/TunaSweeperScreenSpaceSpeechBubbleWidget.h"

TSharedRef<SWidget> UTunaSweeperSpeechBubbleLayerWidget::RebuildWidget()
{
	EnsureWidgetTree();
	return Super::RebuildWidget();
}

void UTunaSweeperSpeechBubbleLayerWidget::EnsureWidgetTree()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"), RF_Transient);
	}
	if (!WidgetTree->RootWidget || !BubbleCanvas)
	{
		BubbleCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("SpeechBubbleCanvas"));
		WidgetTree->RootWidget = BubbleCanvas;
		SetVisibility(ESlateVisibility::HitTestInvisible);
		BubbleCanvas->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

bool UTunaSweeperSpeechBubbleLayerWidget::AddBubble(UTunaSweeperScreenSpaceSpeechBubbleWidget* Bubble)
{
	EnsureWidgetTree();
	if (!BubbleCanvas || !IsValid(Bubble))
	{
		return false;
	}
	if (Bubble->GetParent() == BubbleCanvas)
	{
		return true;
	}

	UCanvasPanelSlot* BubbleSlot = BubbleCanvas->AddChildToCanvas(Bubble);
	if (!BubbleSlot)
	{
		return false;
	}
	BubbleSlot->SetAutoSize(true);
	BubbleSlot->SetAlignment(FVector2D::ZeroVector);
	return true;
}

void UTunaSweeperSpeechBubbleLayerWidget::RemoveBubble(UTunaSweeperScreenSpaceSpeechBubbleWidget* Bubble)
{
	if (BubbleCanvas && IsValid(Bubble))
	{
		BubbleCanvas->RemoveChild(Bubble);
	}
}

void UTunaSweeperSpeechBubbleLayerWidget::RemoveAllBubbles()
{
	if (BubbleCanvas)
	{
		BubbleCanvas->ClearChildren();
	}
}

void UTunaSweeperSpeechBubbleLayerWidget::SetBubbleAnchor(
	UTunaSweeperScreenSpaceSpeechBubbleWidget* Bubble,
	const FVector2D& LogicalAnchor)
{
	if (!IsValid(Bubble))
	{
		return;
	}

	ForceLayoutPrepass();
	if (UCanvasPanelSlot* BubbleSlot = Cast<UCanvasPanelSlot>(Bubble->Slot))
	{
		BubbleSlot->SetPosition(LogicalAnchor - Bubble->GetLocalAnchorPoint());
	}
}

FVector2D UTunaSweeperSpeechBubbleLayerWidget::GetLogicalLayerSize() const
{
	const FVector2D CachedSize = GetCachedGeometry().GetLocalSize();
	if (CachedSize.X > 0.0f && CachedSize.Y > 0.0f)
	{
		return CachedSize;
	}

	const float ViewportScale = FMath::Max(UWidgetLayoutLibrary::GetViewportScale(this), KINDA_SMALL_NUMBER);
	return UWidgetLayoutLibrary::GetViewportSize(this) / ViewportScale;
}
