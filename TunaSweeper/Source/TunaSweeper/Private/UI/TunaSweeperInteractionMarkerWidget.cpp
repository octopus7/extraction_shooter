#include "UI/TunaSweeperInteractionMarkerWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"

void UTunaSweeperInteractionMarkerWidget::NativeConstruct()
{
	Super::NativeConstruct();

	CacheNamedWidgets();
	ApplyState();
}

void UTunaSweeperInteractionMarkerWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	CacheNamedWidgets();

	if (IsDesignTime())
	{
		CachedAlpha = 1.0f;
		CachedRingScale = 1.0f;
		CachedLabelAlpha = 1.0f;
	}

	ApplyState();
}

void UTunaSweeperInteractionMarkerWidget::SetMarkerText(const FText& InText)
{
	CachedDisplayText = InText;
	ApplyState();
}

void UTunaSweeperInteractionMarkerWidget::SetMarkerPresentation(float InAlpha, float InRingScale, float InLabelAlpha)
{
	CachedAlpha = FMath::Clamp(InAlpha, 0.0f, 1.0f);
	CachedRingScale = FMath::Max(InRingScale, 0.01f);
	CachedLabelAlpha = FMath::Clamp(InLabelAlpha, 0.0f, 1.0f);
	ApplyState();
}

void UTunaSweeperInteractionMarkerWidget::CacheNamedWidgets()
{
	if (!WidgetTree)
	{
		return;
	}

	if (!MarkerRoot)
	{
		MarkerRoot = WidgetTree->FindWidget(TEXT("MarkerRoot"));
	}

	if (!RingText)
	{
		RingText = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("RingText")));
	}

	if (!FilledText)
	{
		FilledText = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("FilledText")));
	}

	if (!LabelBackground)
	{
		LabelBackground = Cast<UBorder>(WidgetTree->FindWidget(TEXT("LabelBackground")));
	}

	if (!DisplayNameText)
	{
		DisplayNameText = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("DisplayNameText")));
	}
}

void UTunaSweeperInteractionMarkerWidget::ApplyState()
{
	SetRenderOpacity(CachedAlpha);

	if (MarkerRoot)
	{
		MarkerRoot->SetRenderOpacity(CachedAlpha);
	}

	if (RingText)
	{
		RingText->SetRenderScale(FVector2D(CachedRingScale));
		RingText->SetRenderOpacity(CachedAlpha);
	}

	if (FilledText)
	{
		FilledText->SetRenderOpacity(CachedAlpha);
	}

	if (DisplayNameText)
	{
		DisplayNameText->SetText(CachedDisplayText);
		DisplayNameText->SetRenderOpacity(CachedLabelAlpha);
	}

	if (LabelBackground)
	{
		LabelBackground->SetRenderOpacity(CachedLabelAlpha);
	}
}
