#include "UI/TunaSweeperScreenFadeWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"

void UTunaSweeperScreenFadeWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BuildFadeWidget();
}

void UTunaSweeperScreenFadeWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!bFadeActive)
	{
		return;
	}

	FadeElapsedSeconds += InDeltaTime;
	const float Alpha = FMath::Clamp(1.0f - FadeElapsedSeconds / FMath::Max(0.01f, FadeDurationSeconds), 0.0f, 1.0f);
	SetFadeOpacity(Alpha);

	if (Alpha <= 0.0f)
	{
		bFadeActive = false;
		RemoveFromParent();
	}
}

void UTunaSweeperScreenFadeWidget::StartFadeFromBlack(float DurationSeconds)
{
	BuildFadeWidget();
	FadeDurationSeconds = FMath::Max(0.01f, DurationSeconds);
	FadeElapsedSeconds = 0.0f;
	bFadeActive = true;
	SetFadeOpacity(1.0f);
}

void UTunaSweeperScreenFadeWidget::BuildFadeWidget()
{
	if (!WidgetTree || FadeBorder)
	{
		return;
	}

	UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(
		UCanvasPanel::StaticClass(),
		TEXT("ScreenFadeRoot"));
	FadeBorder = WidgetTree->ConstructWidget<UBorder>(
		UBorder::StaticClass(),
		TEXT("ScreenFadeBorder"));

	if (!RootCanvas || !FadeBorder)
	{
		return;
	}

	WidgetTree->RootWidget = RootCanvas;
	FadeBorder->SetBrushColor(FLinearColor::Black);
	FadeBorder->SetRenderOpacity(1.0f);
	FadeBorder->SetVisibility(ESlateVisibility::HitTestInvisible);

	UCanvasPanelSlot* FadeSlot = RootCanvas->AddChildToCanvas(FadeBorder);
	if (FadeSlot)
	{
		FadeSlot->SetAnchors(FAnchors(0.45f, 0.45f, 0.55f, 0.55f));
		FadeSlot->SetOffsets(FMargin(0.0f));
		FadeSlot->SetAlignment(FVector2D::ZeroVector);
		FadeSlot->SetZOrder(0);
	}
}

void UTunaSweeperScreenFadeWidget::SetFadeOpacity(float Opacity)
{
	if (FadeBorder)
	{
		const float ClampedOpacity = FMath::Clamp(Opacity, 0.0f, 1.0f);
		FadeBorder->SetBrushColor(FLinearColor::Black);
		FadeBorder->SetRenderOpacity(ClampedOpacity);
		FadeBorder->SetVisibility(ClampedOpacity > 0.01f
			? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Collapsed);
	}
}
