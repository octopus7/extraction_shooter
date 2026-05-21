#include "UI/TunaSweeperHudBottomStatusWidget.h"

#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "UI/TunaSweeperUIFont.h"

namespace TunaSweeperHudStatus
{
	FText MakeVitalsText(float Value)
	{
		FNumberFormattingOptions NumberFormat;
		NumberFormat.MinimumFractionalDigits = 0;
		NumberFormat.MaximumFractionalDigits = 1;
		return FText::AsNumber(Value, &NumberFormat);
	}

	float MakeVitalsPercent(float Value, float MaxValue)
	{
		return MaxValue > 0.0f
			? FMath::Clamp(Value / MaxValue, 0.0f, 1.0f)
			: 0.0f;
	}
}

void UTunaSweeperHudBottomStatusWidget::NativeConstruct()
{
	Super::NativeConstruct();
	TunaSweeperUIFont::ApplyFontToWidgetTree(this);

	if (PreviewHudState.MaxCarryWeight <= 0.0f)
	{
		PreviewHudState = FTunaSweeperPlayerHudState();
	}

	ApplyHudState();
}

void UTunaSweeperHudBottomStatusWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	TunaSweeperUIFont::ApplyFontToWidgetTree(this);

	if (PreviewHudState.MaxCarryWeight <= 0.0f)
	{
		PreviewHudState = FTunaSweeperPlayerHudState();
	}

	ApplyHudState();
}

void UTunaSweeperHudBottomStatusWidget::SetHudState(const FTunaSweeperPlayerHudState& InHudState)
{
	PreviewHudState = InHudState;
	PreviewHudState.NormalizeWeightLimits();
	ApplyHudState();
}

void UTunaSweeperHudBottomStatusWidget::ApplyHudState()
{
	PreviewHudState.NormalizeWeightLimits();

	auto CollapseLegacyWeightWidget = [](UWidget* Widget)
	{
		if (Widget)
		{
			Widget->SetVisibility(ESlateVisibility::Collapsed);
		}
	};

	CollapseLegacyWeightWidget(WeightText.Get());
	CollapseLegacyWeightWidget(WeightRow.Get());
	CollapseLegacyWeightWidget(GaugeOverlay.Get());
	CollapseLegacyWeightWidget(CarryWeightGauge.Get());
	CollapseLegacyWeightWidget(WeightWarningIcon.Get());

	if (WeightText)
	{
		WeightText->SetText(FText::GetEmpty());
	}

	if (HealthText)
	{
		HealthText->SetText(FText::Format(
			FText::FromString(TEXT("HP {0} / {1}")),
			TunaSweeperHudStatus::MakeVitalsText(PreviewHudState.Health),
			TunaSweeperHudStatus::MakeVitalsText(PreviewHudState.MaxHealth)));
	}

	if (HungerText)
	{
		HungerText->SetText(FText::Format(
			FText::FromString(TEXT("배부름 {0}")),
			TunaSweeperHudStatus::MakeVitalsText(PreviewHudState.Food)));
	}

	if (HydrationText)
	{
		HydrationText->SetText(FText::Format(
			FText::FromString(TEXT("수분 {0}")),
			TunaSweeperHudStatus::MakeVitalsText(PreviewHudState.Hydration)));
	}

	if (HealthGauge)
	{
		HealthGauge->SetPercent(TunaSweeperHudStatus::MakeVitalsPercent(PreviewHudState.Health, PreviewHudState.MaxHealth));
	}

	if (HungerGauge)
	{
		HungerGauge->SetPercent(TunaSweeperHudStatus::MakeVitalsPercent(PreviewHudState.Food, PreviewHudState.MaxFood));
	}

	if (HydrationGauge)
	{
		HydrationGauge->SetPercent(TunaSweeperHudStatus::MakeVitalsPercent(PreviewHudState.Hydration, PreviewHudState.MaxHydration));
	}

	if (CarryWeightGauge)
	{
		CarryWeightGauge->SetPercent(0.0f);
	}

	if (WeightWarningIcon)
	{
		WeightWarningIcon->SetVisibility(ESlateVisibility::Collapsed);
	}
}
