#include "TunaSweeperIntroMenuWidgetShared.h"
#include "BossLab/TunaSweeperBossLabSubsystem.h"

void UTunaSweeperIntroMenuWidget::EnsureLaboratoryMenu()
{
	if (bPauseSettingsMode || bDifficultyAdjustmentMode || !WidgetTree) return;
	UVerticalBox* MainStack = Cast<UVerticalBox>(FindIntroWidget(TEXT("MainMenuPanel")));
	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
	if (!MainStack || !RootCanvas) return;
	if (!LaboratoryButton)
	{
		UWidgetTree* MainTree = MainStack->GetTypedOuter<UWidgetTree>();
		if (!MainTree) return;
		USizeBox* Box = MainTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("LaboratoryButtonBox"));
		LaboratoryButton = MainTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("LaboratoryButton"));
		UTextBlock* Label = MainTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("LaboratoryButtonText"));
		Box->SetWidthOverride(418.0f);
		Box->SetHeightOverride(98.0f);
		Box->SetContent(LaboratoryButton);
		LaboratoryButton->SetContent(Label);
		LaboratoryButton->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleLaboratoryClicked);

		// Re-add the existing slots to update the live Slate tree as well as the UMG tree.
		struct FChildLayout
		{
			UWidget* Child;
			FMargin Padding;
			FSlateChildSize Size;
			EHorizontalAlignment Horizontal;
			EVerticalAlignment Vertical;
		};
		TArray<FChildLayout> Children;
		bool bInserted = false;
		for (UWidget* Child : MainStack->GetAllChildren())
		{
			if (Child->GetFName() == TEXT("SettingsButtonBox"))
			{
				Children.Add({Box, FMargin(12, 0, 0, -4), FSlateChildSize(ESlateSizeRule::Automatic), HAlign_Left, VAlign_Center});
				bInserted = true;
			}
			const UVerticalBoxSlot* ExistingSlot = CastChecked<UVerticalBoxSlot>(Child->Slot);
			Children.Add({Child, ExistingSlot->GetPadding(), ExistingSlot->GetSize(), ExistingSlot->GetHorizontalAlignment(), ExistingSlot->GetVerticalAlignment()});
		}
		if (!bInserted) Children.Add({Box, FMargin(12, 0, 0, -4), FSlateChildSize(ESlateSizeRule::Automatic), HAlign_Left, VAlign_Center});
		MainStack->ClearChildren();
		for (const FChildLayout& Child : Children)
		{
			UVerticalBoxSlot* ChildSlot = MainStack->AddChildToVerticalBox(Child.Child);
			ChildSlot->SetPadding(Child.Padding);
			ChildSlot->SetSize(Child.Size);
			ChildSlot->SetHorizontalAlignment(Child.Horizontal);
			ChildSlot->SetVerticalAlignment(Child.Vertical);
		}
	}
	if (LaboratoryPanel) return;

	UOverlay* Panel = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("LaboratoryPanel"));
	LaboratoryPanel = Panel;
	Panel->SetVisibility(ESlateVisibility::Collapsed);
	UCanvasPanelSlot* PanelSlot = RootCanvas->AddChildToCanvas(Panel);
	PanelSlot->SetAnchors(FAnchors(0, 0, 1, 1));
	PanelSlot->SetOffsets(FMargin(0));
	PanelSlot->SetZOrder(15);
	UBorder* Background = WidgetTree->ConstructWidget<UBorder>();
	Background->SetBrushColor(FLinearColor(0.006f, 0.025f, 0.035f, 0.93f));
	UOverlaySlot* BackgroundSlot = Panel->AddChildToOverlay(Background);
	BackgroundSlot->SetHorizontalAlignment(HAlign_Fill);
	BackgroundSlot->SetVerticalAlignment(VAlign_Fill);
	USizeBox* ContentBox = WidgetTree->ConstructWidget<USizeBox>();
	ContentBox->SetWidthOverride(620);
	UOverlaySlot* ContentSlot = Panel->AddChildToOverlay(ContentBox);
	ContentSlot->SetHorizontalAlignment(HAlign_Center);
	ContentSlot->SetVerticalAlignment(VAlign_Center);
	UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>();
	ContentBox->SetContent(Content);

	auto AddText = [this, Content](const TCHAR* Name, int32 Size, FLinearColor Color, float Bottom)
	{
		UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		TunaSweeperUIFont::ApplyFont(Text, Size);
		Text->SetColorAndOpacity(Color);
		Text->SetJustification(ETextJustify::Center);
		Text->SetAutoWrapText(true);
		Content->AddChildToVerticalBox(Text)->SetPadding(FMargin(0, 0, 0, Bottom));
	};
	AddText(TEXT("LaboratoryTitleText"), 42, FLinearColor(0.92f, 0.98f, 0.97f), 12);
	AddText(TEXT("LaboratoryDescriptionText"), 18, FLinearColor(0.60f, 0.75f, 0.77f), 34);
	auto AddAction = [this, Content](const TCHAR* Name, const TCHAR* LabelName, float Height,
		TunaSweeperUIStyle::EButtonRole Role, float Bottom)
	{
		USizeBox* Box = WidgetTree->ConstructWidget<USizeBox>();
		Box->SetHeightOverride(Height);
		UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
		UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), LabelName);
		TunaSweeperUIFont::ApplyFont(Text, 24);
		Text->SetJustification(ETextJustify::Center);
		Button->SetContent(Text);
		TunaSweeperUIStyle::ApplyButton(Button, Role);
		Box->SetContent(Button);
		Content->AddChildToVerticalBox(Box)->SetPadding(FMargin(0, 0, 0, Bottom));
		return Button;
	};
	BossDevelopmentButton = AddAction(TEXT("BossDevelopmentButton"), TEXT("BossDevelopmentButtonText"), 84,
		TunaSweeperUIStyle::EButtonRole::Primary, 16);
	BossSinglePlayerButton = AddAction(TEXT("BossSinglePlayerButton"), TEXT("BossSinglePlayerButtonText"), 84,
		TunaSweeperUIStyle::EButtonRole::Secondary, 38);
	BossMultiplayerButton = AddAction(TEXT("BossMultiplayerButton"), TEXT("BossMultiplayerButtonText"), 68,
		TunaSweeperUIStyle::EButtonRole::Secondary, 12);
	AddText(TEXT("LaboratoryMultiplayerStatusText"), 16, FLinearColor(0.52f, 0.63f, 0.65f), 30);
	LaboratoryBackButton = AddAction(TEXT("LaboratoryBackButton"), TEXT("LaboratoryBackButtonText"), 48,
		TunaSweeperUIStyle::EButtonRole::Icon, 0);
	BossDevelopmentButton->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleBossDevelopmentClicked);
	BossSinglePlayerButton->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleBossSinglePlayerClicked);
	BossMultiplayerButton->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleBossMultiplayerClicked);
	LaboratoryBackButton->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleLaboratoryBackClicked);
	RefreshLaboratoryTexts();
}

void UTunaSweeperIntroMenuWidget::RefreshLaboratoryTexts()
{
	const TPair<const TCHAR*, const TCHAR*> Entries[] = {
		{TEXT("LaboratoryButtonText"), TEXT("ui.lab.title")},
		{TEXT("LaboratoryTitleText"), TEXT("ui.lab.title")},
		{TEXT("LaboratoryDescriptionText"), TEXT("ui.lab.description")},
		{TEXT("BossDevelopmentButtonText"), TEXT("ui.lab.development")},
		{TEXT("BossSinglePlayerButtonText"), TEXT("ui.lab.single")},
		{TEXT("BossMultiplayerButtonText"), TEXT("ui.lab.multiplayer")},
		{TEXT("LaboratoryMultiplayerStatusText"), TEXT("ui.lab.in_development")},
		{TEXT("LaboratoryBackButtonText"), TEXT("ui.common.back")}};
	for (const TPair<const TCHAR*, const TCHAR*>& Entry : Entries)
	{
		SetNamedText(Entry.Key, ResolveUiText(Entry.Value, FText::GetEmpty()));
	}
}

bool UTunaSweeperIntroMenuWidget::IsLaboratoryVisible() const
{
	return LaboratoryPanel && LaboratoryPanel->GetVisibility() == ESlateVisibility::Visible;
}

void UTunaSweeperIntroMenuWidget::HandleLaboratoryClicked()
{
	if (bStartTravelPending || bPauseSettingsMode || bDifficultyAdjustmentMode) return;
	EnsureLaboratoryMenu();
	if (!LaboratoryPanel) return;
	HideOverlayPanels();
	HideDeleteConfirmDialog();
	ResetDeleteHoldProgress();
	SetTitlePresentationMainMenuActive(false);
	SetTitleLogoVisible(false);
	if (MainMenuPanel) MainMenuPanel->SetVisibility(ESlateVisibility::Collapsed);
	if (SaveSlotPanel) SaveSlotPanel->SetVisibility(ESlateVisibility::Collapsed);
	LaboratoryPanel->SetVisibility(ESlateVisibility::Visible);
	RefreshLaboratoryTexts();
	FadeInScreen(LaboratoryPanel);
	if (GetOwningPlayer()) BossDevelopmentButton->SetUserFocus(GetOwningPlayer());
}

void UTunaSweeperIntroMenuWidget::HandleLaboratoryBackClicked()
{
	if (bStartTravelPending) return;
	ShowMainMenu();
	if (GetOwningPlayer() && LaboratoryButton) LaboratoryButton->SetUserFocus(GetOwningPlayer());
}

void UTunaSweeperIntroMenuWidget::EnterBossLaboratory(bool bDevelopment)
{
	if (bStartTravelPending || !IsLaboratoryVisible() || !GetGameInstance()) return;
	if (UTunaSweeperBossLabSubsystem* Lab = GetGameInstance()->GetSubsystem<UTunaSweeperBossLabSubsystem>())
	{
		bStartTravelPending = true;
		SetStartTravelControlsEnabled(false);
		Lab->EnterLab(bDevelopment);
	}
}

void UTunaSweeperIntroMenuWidget::HandleBossDevelopmentClicked() { EnterBossLaboratory(true); }
void UTunaSweeperIntroMenuWidget::HandleBossSinglePlayerClicked() { EnterBossLaboratory(false); }

void UTunaSweeperIntroMenuWidget::HandleBossMultiplayerClicked()
{
	if (bStartTravelPending || !GetGameInstance()) return;
	if (UTunaSweeperToastSubsystem* Toasts = GetGameInstance()->GetSubsystem<UTunaSweeperToastSubsystem>())
	{
		Toasts->ShowLocalizedToast(TEXT("ui.lab.in_development"), FText::GetEmpty());
	}
}
