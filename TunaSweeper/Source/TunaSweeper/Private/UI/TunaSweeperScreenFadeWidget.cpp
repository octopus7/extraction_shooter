#include "UI/TunaSweeperScreenFadeWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"

TSharedRef<SWidget> UTunaSweeperScreenFadeWidget::RebuildWidget()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"), RF_Transient);
	}

	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildFadeWidget();
	}

	return Super::RebuildWidget();
}

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
	const float Progress = FMath::Clamp(FadeElapsedSeconds / FMath::Max(0.01f, FadeDurationSeconds), 0.0f, 1.0f);
	const float Alpha = FadeDirection == EFadeDirection::FromBlack ? 1.0f - Progress : Progress;
	SetFadeOpacity(Alpha);

	if (Progress >= 1.0f)
	{
		bFadeActive = false;
		FSimpleDelegate FinishedDelegate = FadeFinishedDelegate;
		FadeFinishedDelegate.Unbind();
		if (FadeDirection == EFadeDirection::FromBlack)
		{
			RemoveFromParent();
		}
		if (FinishedDelegate.IsBound())
		{
			FinishedDelegate.Execute();
		}
	}
}

void UTunaSweeperScreenFadeWidget::StartFadeFromBlack(float DurationSeconds)
{
	BuildFadeWidget();
	FadeDirection = EFadeDirection::FromBlack;
	FadeFinishedDelegate.Unbind();
	FadeDurationSeconds = FMath::Max(0.01f, DurationSeconds);
	FadeElapsedSeconds = 0.0f;
	bFadeActive = true;
	SetFadeOpacity(1.0f);
}

void UTunaSweeperScreenFadeWidget::StartFadeToBlack(float DurationSeconds, FSimpleDelegate InFadeFinishedDelegate)
{
	BuildFadeWidget();
	FadeDirection = EFadeDirection::ToBlack;
	FadeFinishedDelegate = InFadeFinishedDelegate;
	FadeDurationSeconds = FMath::Max(0.01f, DurationSeconds);
	FadeElapsedSeconds = 0.0f;
	bFadeActive = true;
	SetFadeOpacity(0.0f);
}

void UTunaSweeperScreenFadeWidget::BuildFadeWidget()
{
	if (!WidgetTree || FadeBorder || WidgetTree->RootWidget)
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
		FadeSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
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
