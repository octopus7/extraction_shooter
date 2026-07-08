#include "TunaSweeperGameHudWidgetShared.h"

void UTunaSweeperGameHudWidget::CacheHudTransitionBaseline(UWidget* Widget)
{
	if (!Widget)
	{
		return;
	}

	const TWeakObjectPtr<UWidget> WidgetKey(Widget);
	if (!HudTransitionBaseTransforms.Contains(WidgetKey))
	{
		HudTransitionBaseTransforms.Add(WidgetKey, Widget->GetRenderTransform());
		HudTransitionBaseOpacities.Add(WidgetKey, Widget->GetRenderOpacity());
	}
}

bool UTunaSweeperGameHudWidget::HasActiveHudTransition(const UWidget* Widget) const
{
	if (!Widget)
	{
		return false;
	}

	for (const FHudWidgetTransition& Transition : ActiveHudTransitions)
	{
		if (Transition.Widget.Get() == Widget)
		{
			return true;
		}
	}

	return false;
}

ETunaSweeperHudTransitionEdge UTunaSweeperGameHudWidget::ResolveHudTransitionEdge(
	const UWidget* Widget,
	ETunaSweeperHudTransitionEdge DirectionOverride) const
{
	if (!Widget || DirectionOverride != ETunaSweeperHudTransitionEdge::Auto)
	{
		return DirectionOverride;
	}

	const UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Widget->Slot);
	if (!CanvasSlot)
	{
		return ETunaSweeperHudTransitionEdge::FadeOnly;
	}

	const FAnchors Anchors = CanvasSlot->GetAnchors();
	const FVector2D Alignment = CanvasSlot->GetAlignment();
	const FVector2D Position = CanvasSlot->GetPosition();
	const bool bInsideCenterContentPanel = CenterContentPanel && Widget->GetParent() == CenterContentPanel;

	ETunaSweeperHudTransitionEdge BestEdge = ETunaSweeperHudTransitionEdge::FadeOnly;
	float BestScore = TNumericLimits<float>::Max();
	auto ConsiderEdge = [&BestEdge, &BestScore](ETunaSweeperHudTransitionEdge Edge, bool bCandidate, float Score)
	{
		if (bCandidate && Score < BestScore)
		{
			BestEdge = Edge;
			BestScore = Score;
		}
	};

	const bool bPinnedLeft = Anchors.Minimum.X <= 0.05f && Anchors.Maximum.X <= 0.05f && Alignment.X <= 0.5f;
	const bool bPinnedRight = Anchors.Minimum.X >= 0.95f && Anchors.Maximum.X >= 0.95f && Alignment.X >= 0.5f;
	ConsiderEdge(ETunaSweeperHudTransitionEdge::Left, bPinnedLeft, FMath::Abs(Position.X));
	ConsiderEdge(ETunaSweeperHudTransitionEdge::Right, bPinnedRight, FMath::Abs(Position.X));

	if (!bInsideCenterContentPanel)
	{
		const bool bPinnedTop = Anchors.Minimum.Y <= 0.05f && Anchors.Maximum.Y <= 0.05f && Alignment.Y <= 0.5f;
		const bool bPinnedBottom = Anchors.Minimum.Y >= 0.95f && Anchors.Maximum.Y >= 0.95f && Alignment.Y >= 0.5f;
		ConsiderEdge(ETunaSweeperHudTransitionEdge::Top, bPinnedTop, FMath::Abs(Position.Y));
		ConsiderEdge(ETunaSweeperHudTransitionEdge::Bottom, bPinnedBottom, FMath::Abs(Position.Y));
	}

	return BestEdge;
}

FVector2D UTunaSweeperGameHudWidget::GetHudTransitionHiddenTranslation(
	const UWidget* Widget,
	ETunaSweeperHudTransitionEdge Edge) const
{
	if (Edge == ETunaSweeperHudTransitionEdge::Auto || Edge == ETunaSweeperHudTransitionEdge::FadeOnly || !Widget)
	{
		return FVector2D::ZeroVector;
	}

	const bool bHorizontalEdge = Edge == ETunaSweeperHudTransitionEdge::Left || Edge == ETunaSweeperHudTransitionEdge::Right;
	float Distance = bHorizontalEdge
		? HudWidgetTransitionFallbackHorizontalDistance
		: HudWidgetTransitionFallbackVerticalDistance;

	if (const UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Widget->Slot))
	{
		const FVector2D SlotSize = CanvasSlot->GetSize();
		const float SlotDistance = bHorizontalEdge ? SlotSize.X : SlotSize.Y;
		if (SlotDistance > 1.0f)
		{
			Distance = FMath::Max(Distance, SlotDistance + HudWidgetTransitionDistancePadding);
		}
	}

	switch (Edge)
	{
	case ETunaSweeperHudTransitionEdge::Left:
		return FVector2D(-Distance, 0.0f);
	case ETunaSweeperHudTransitionEdge::Right:
		return FVector2D(Distance, 0.0f);
	case ETunaSweeperHudTransitionEdge::Top:
		return FVector2D(0.0f, -Distance);
	case ETunaSweeperHudTransitionEdge::Bottom:
		return FVector2D(0.0f, Distance);
	default:
		return FVector2D::ZeroVector;
	}
}

void UTunaSweeperGameHudWidget::SetTransitionedWidgetVisibility(
	UWidget* Widget,
	ESlateVisibility TargetVisibility,
	ETunaSweeperHudTransitionEdge DirectionOverride)
{
	if (!Widget)
	{
		return;
	}

	const ETunaSweeperHudTransitionEdge ResolvedEdge = ResolveHudTransitionEdge(Widget, DirectionOverride);
	SetTransitionedWidgetVisibilityFromTranslation(
		Widget,
		TargetVisibility,
		GetHudTransitionHiddenTranslation(Widget, ResolvedEdge));
}

void UTunaSweeperGameHudWidget::SetTransitionedWidgetVisibilityFromTranslation(
	UWidget* Widget,
	ESlateVisibility TargetVisibility,
	const FVector2D& HiddenTranslation)
{
	if (!Widget)
	{
		return;
	}

	const bool bTargetShown = IsSlateVisibilityShown(TargetVisibility);
	const bool bCurrentlyShown = IsSlateVisibilityShown(Widget->GetVisibility());
	if (!bTargetShown && !bCurrentlyShown && !HasActiveHudTransition(Widget))
	{
		Widget->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	if (bTargetShown && bCurrentlyShown && !HasActiveHudTransition(Widget))
	{
		Widget->SetVisibility(TargetVisibility);
		return;
	}

	for (const FHudWidgetTransition& Transition : ActiveHudTransitions)
	{
		if (Transition.Widget.Get() == Widget &&
			Transition.bShow == bTargetShown &&
			Transition.FinalVisibility == (bTargetShown ? TargetVisibility : ESlateVisibility::Collapsed))
		{
			return;
		}
	}

	CacheHudTransitionBaseline(Widget);

	const TWeakObjectPtr<UWidget> WidgetKey(Widget);
	const FWidgetTransform BaseTransform = HudTransitionBaseTransforms.FindRef(WidgetKey);
	const float BaseOpacity = HudTransitionBaseOpacities.Contains(WidgetKey)
		? HudTransitionBaseOpacities.FindRef(WidgetKey)
		: 1.0f;
	const FWidgetTransform HiddenTransform = WithAddedTranslation(BaseTransform, HiddenTranslation);

	for (int32 Index = ActiveHudTransitions.Num() - 1; Index >= 0; --Index)
	{
		if (ActiveHudTransitions[Index].Widget.Get() == Widget)
		{
			ActiveHudTransitions.RemoveAtSwap(Index);
		}
	}

	if (bTargetShown)
	{
		if (!bCurrentlyShown)
		{
			Widget->SetRenderTransform(HiddenTransform);
			Widget->SetRenderOpacity(0.0f);
		}
		Widget->SetVisibility(TargetVisibility);
	}

	FHudWidgetTransition Transition;
	Transition.Widget = Widget;
	Transition.StartTransform = Widget->GetRenderTransform();
	Transition.EndTransform = bTargetShown ? BaseTransform : HiddenTransform;
	Transition.StartOpacity = Widget->GetRenderOpacity();
	Transition.EndOpacity = bTargetShown ? BaseOpacity : 0.0f;
	Transition.DurationSeconds = HudWidgetTransitionDurationSeconds;
	Transition.FinalVisibility = bTargetShown ? TargetVisibility : ESlateVisibility::Collapsed;
	Transition.bShow = bTargetShown;
	ActiveHudTransitions.Add(Transition);
}

void UTunaSweeperGameHudWidget::TickHudTransitions(float InDeltaTime)
{
	for (int32 Index = ActiveHudTransitions.Num() - 1; Index >= 0; --Index)
	{
		FHudWidgetTransition& Transition = ActiveHudTransitions[Index];
		UWidget* Widget = Transition.Widget.Get();
		if (!Widget)
		{
			ActiveHudTransitions.RemoveAtSwap(Index);
			continue;
		}

		Transition.ElapsedSeconds += FMath::Max(0.0f, InDeltaTime);
		const float RawAlpha = Transition.DurationSeconds > KINDA_SMALL_NUMBER
			? Transition.ElapsedSeconds / Transition.DurationSeconds
			: 1.0f;
		const float Alpha = SmoothTransitionAlpha(RawAlpha);

		FWidgetTransform CurrentTransform = Transition.StartTransform;
		CurrentTransform.Translation = FMath::Lerp(Transition.StartTransform.Translation, Transition.EndTransform.Translation, Alpha);
		CurrentTransform.Scale = FMath::Lerp(Transition.StartTransform.Scale, Transition.EndTransform.Scale, Alpha);
		CurrentTransform.Shear = FMath::Lerp(Transition.StartTransform.Shear, Transition.EndTransform.Shear, Alpha);
		CurrentTransform.Angle = FMath::Lerp(Transition.StartTransform.Angle, Transition.EndTransform.Angle, Alpha);
		Widget->SetRenderTransform(CurrentTransform);
		Widget->SetRenderOpacity(FMath::Lerp(Transition.StartOpacity, Transition.EndOpacity, Alpha));

		if (RawAlpha >= 1.0f)
		{
			const bool bCompletedExternalPanelHide =
				!Transition.bShow &&
				bClearExternalPanelModeAfterHide &&
				ExternalPanelWidget &&
				Widget == ExternalPanelWidget;

			Widget->SetRenderTransform(Transition.EndTransform);
			Widget->SetRenderOpacity(Transition.EndOpacity);
			Widget->SetVisibility(Transition.FinalVisibility);

			if (!Transition.bShow)
			{
				const TWeakObjectPtr<UWidget> WidgetKey(Widget);
				if (const FWidgetTransform* BaseTransform = HudTransitionBaseTransforms.Find(WidgetKey))
				{
					Widget->SetRenderTransform(*BaseTransform);
				}
				if (const float* BaseOpacity = HudTransitionBaseOpacities.Find(WidgetKey))
				{
					Widget->SetRenderOpacity(*BaseOpacity);
				}
			}

			if (bCompletedExternalPanelHide)
			{
				ExternalPanelWidget->SetExternalPanelMode(ETunaSweeperHudExternalPanelMode::None);
				bClearExternalPanelModeAfterHide = false;
			}

			ActiveHudTransitions.RemoveAtSwap(Index);
		}
	}
}

