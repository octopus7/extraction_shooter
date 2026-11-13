#include "TunaSweeperIntroMenuWidgetShared.h"
#include "Settings/TunaSweeperBuildFlavor.h"
#include "UI/DifficultySelectionWidget.h"
#include "Subsystem/TunaSweeperDifficultySubsystem.h"

void UTunaSweeperIntroMenuWidget::EnsureDifficultySelectionPanel()
{
    ApplyDemoNoticeVisualStyle();
    // The complete difficulty tree is authored in WBP_DifficultySelection.
    // BindScreenWidgets traverses the saved child blueprint, including its buttons.
    if (!DifficultySelectPanel) BindScreenWidgets();
}

void UTunaSweeperIntroMenuWidget::SelectDifficultyStage(int32 DifficultyStage)
{
	if (bStartTravelPending || bClosingDifficultyAdjustment || !IsDifficultySelectionVisible()) return;
	const int32 ClickedStage = FMath::Clamp(DifficultyStage, 1, 3);
	if (SelectedDifficultyStage == ClickedStage)
	{
		HandleDifficultyStartClicked();
		return;
	}
	SelectedDifficultyStage = ClickedStage;
	RefreshDifficultySelectionPanel();
}

void UTunaSweeperIntroMenuWidget::RefreshDifficultySelectionPanel()
{
	LoadDifficultyDefinitions();
	const bool bDemoNotice = TunaSweeperBuildFlavor::IsDemo() && !bDifficultyAdjustmentMode;
	if (bDemoNotice)
	{
		SelectedDifficultyStage = 2;
	}

	if (bDemoNotice && DemoNoticePanel)
	{
		if (DemoNoticeTitleText)
		{
			DemoNoticeTitleText->SetText(ResolveUiText(
				FName(TEXT("ui.title.demo_notice_title")),
				FText::GetEmpty()));
		}
		if (DemoNoticeMessageText)
		{
			DemoNoticeMessageText->SetText(ResolveUiText(
				FName(TEXT("ui.title.demo_notice_message")),
				FText::GetEmpty()));
		}
		if (DemoNoticeBackButtonText)
		{
			DemoNoticeBackButtonText->SetText(ResolveUiText(
				FName(TEXT("ui.common.back")),
				FText::GetEmpty()));
		}
		if (DemoNoticeConfirmButtonText)
		{
			DemoNoticeConfirmButtonText->SetText(ResolveUiText(
				FName(TEXT("ui.common.confirm")),
				FText::GetEmpty()));
		}
		if (DemoNoticeConfirmButton)
		{
			DemoNoticeConfirmButton->SetIsEnabled(!bStartTravelPending);
		}
		return;
	}

    if (auto* View = Cast<UDifficultySelectionWidget>(DifficultySelectPanel))
    {
        View->Present(SelectedDifficultyStage, bStartTravelPending, bDifficultyAdjustmentMode);
        return;
    }

}

void UTunaSweeperIntroMenuWidget::ApplyDemoNoticeVisualStyle()
{
	if (!WidgetTree || !DemoNoticePanel)
	{
		return;
	}

	// Keep the watercolor background recognizable, but push it behind the notice content.
	if (UImage* BackgroundImage = Cast<UImage>(FindIntroWidget(TEXT("DemoNoticeBackgroundImage"))))
	{
		BackgroundImage->SetColorAndOpacity(FLinearColor(0.90f, 0.94f, 0.93f, 1.0f));
	}
	if (UImage* ReadabilityWash = Cast<UImage>(FindIntroWidget(TEXT("DemoNoticeReadabilityWash"))))
	{
		FSlateBrush WashBrush = ReadabilityWash->GetBrush();
		WashBrush.DrawAs = ESlateBrushDrawType::Box;
		WashBrush.SetResourceObject(nullptr);
		WashBrush.TintColor = FSlateColor(FLinearColor(0.055f, 0.17f, 0.19f, 0.10f));
		ReadabilityWash->SetBrush(WashBrush);
		ReadabilityWash->SetColorAndOpacity(FLinearColor::White);
		ReadabilityWash->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	// The illustration is a JPG with a white paper background. A restrained cream-teal tint
	// blends that rectangle into the dimmed watercolor instead of making it look pasted on.
	if (DemoNoticeArtworkImage)
	{
		DemoNoticeArtworkImage->SetColorAndOpacity(FLinearColor(0.90f, 0.93f, 0.89f, 1.0f));
		DemoNoticeArtworkImage->SetRenderOpacity(0.96f);
	}

	const FLinearColor HeadingColor(0.10f, 0.25f, 0.28f, 1.0f);
	const FLinearColor BodyColor(0.18f, 0.34f, 0.36f, 1.0f);
	if (DemoNoticeTitleText)
	{
		DemoNoticeTitleText->SetColorAndOpacity(FSlateColor(HeadingColor));
	}
	if (DemoNoticeMessageText)
	{
		DemoNoticeMessageText->SetColorAndOpacity(FSlateColor(BodyColor));
	}

	// The old child images contain coral/cyan double outlines. The button styles now provide
	// the full background and interaction states, so those decorative images stay hidden.
	if (UImage* BackBackground = Cast<UImage>(FindIntroWidget(TEXT("DemoNoticeBackButtonBackground"))))
	{
		BackBackground->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (UImage* ConfirmBackground = Cast<UImage>(FindIntroWidget(TEXT("DemoNoticeConfirmButtonBackground"))))
	{
		ConfirmBackground->SetVisibility(ESlateVisibility::Collapsed);
	}

	TunaSweeperUIStyle::ApplyButton(
		DemoNoticeBackButton,
		TunaSweeperUIStyle::EButtonRole::Secondary);
	TunaSweeperUIStyle::ApplyButton(
		DemoNoticeConfirmButton,
		TunaSweeperUIStyle::EButtonRole::Primary);
	TunaSweeperUIStyle::ApplyLabel(DemoNoticeBackButtonText);
	TunaSweeperUIStyle::ApplyLabel(DemoNoticeConfirmButtonText);
}


void UTunaSweeperIntroMenuWidget::LoadDifficultyDefinitions()
{
	if (bDifficultyDefinitionsLoaded)
	{
		return;
	}

	DifficultyOptionTexts.Reset();
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UTunaSweeperDifficultySubsystem* DifficultySubsystem =
			GameInstance->GetSubsystem<UTunaSweeperDifficultySubsystem>())
		{
			for (int32 DifficultyStage = 1; DifficultyStage <= 3; ++DifficultyStage)
			{
				FTunaSweeperDifficultyDefinition Definition;
				if (DifficultySubsystem->TryGetDifficultyDefinition(DifficultyStage, Definition))
				{
					DifficultyOptionTexts.Add({DifficultyStage, Definition.TitleStringKey, Definition.DescriptionStringKey});
				}
			}
		}
	}

	bDifficultyDefinitionsLoaded = true;
}

FText UTunaSweeperIntroMenuWidget::BuildDifficultyTitleText(int32 DifficultyStage) const
{
	if (const FDifficultyOptionText* OptionText = DifficultyOptionTexts.FindByPredicate(
		[DifficultyStage](const FDifficultyOptionText& Candidate)
		{
			return Candidate.DifficultyStage == DifficultyStage;
		}))
	{
		return ResolveUiText(OptionText->TitleStringKey, FText::GetEmpty());
	}

	return FText::GetEmpty();
}

FText UTunaSweeperIntroMenuWidget::BuildDifficultyDescriptionText(int32 DifficultyStage) const
{
	if (const FDifficultyOptionText* OptionText = DifficultyOptionTexts.FindByPredicate(
		[DifficultyStage](const FDifficultyOptionText& Candidate)
		{
			return Candidate.DifficultyStage == DifficultyStage;
		}))
	{
		return ResolveUiText(OptionText->DescriptionStringKey, FText::GetEmpty());
	}

	return FText::GetEmpty();
}
