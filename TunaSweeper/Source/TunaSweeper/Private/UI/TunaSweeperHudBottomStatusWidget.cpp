#include "UI/TunaSweeperHudBottomStatusWidget.h"

#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"

namespace TunaSweeperHudStatus
{
	FText MakeRoundedFloatText(float Value)
	{
		FNumberFormattingOptions NumberFormat;
		NumberFormat.MinimumFractionalDigits = 0;
		NumberFormat.MaximumFractionalDigits = 1;
		return FText::AsNumber(Value, &NumberFormat);
	}
}

void UTunaSweeperHudBottomStatusWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (PreviewHudState.MaxCarryWeight <= 0.0f)
	{
		PreviewHudState = FTunaSweeperPlayerHudState();
	}

	ApplyHudState();
}

void UTunaSweeperHudBottomStatusWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

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

	if (WeightText)
	{
		WeightText->SetText(FText::Format(
			FText::FromString(TEXT("{0}/{1} kg")),
			TunaSweeperHudStatus::MakeRoundedFloatText(PreviewHudState.CurrentCarryWeight),
			TunaSweeperHudStatus::MakeRoundedFloatText(PreviewHudState.MaxCarryWeight)));
	}

	if (HealthText)
	{
		HealthText->SetText(FText::Format(FText::FromString(TEXT("HP {0}")), FText::AsNumber(FMath::RoundToInt(PreviewHudState.Health))));
	}

	if (HungerText)
	{
		HungerText->SetText(FText::Format(FText::FromString(TEXT("Food {0}")), FText::AsNumber(FMath::RoundToInt(PreviewHudState.Hunger))));
	}

	if (HydrationText)
	{
		HydrationText->SetText(FText::Format(FText::FromString(TEXT("Water {0}")), FText::AsNumber(FMath::RoundToInt(PreviewHudState.Hydration))));
	}

	if (CarryWeightGauge)
	{
		const float GaugePercent = PreviewHudState.MovementBlockedWeight > 0.0f
			? PreviewHudState.CurrentCarryWeight / PreviewHudState.MovementBlockedWeight
			: 0.0f;
		CarryWeightGauge->SetPercent(FMath::Clamp(GaugePercent, 0.0f, 1.0f));
	}

	if (WeightWarningIcon)
	{
		WeightWarningIcon->SetVisibility(
			PreviewHudState.IsCarryWeightOverLimit()
				? ESlateVisibility::HitTestInvisible
				: ESlateVisibility::Hidden);
	}
}

