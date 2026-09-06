#include "TunaSweeperIntroMenuWidgetShared.h"
#include "UI/TunaSweeperGraphicsSettingsWidget.h"
#include "UObject/UnrealType.h"

UWidget* UTunaSweeperIntroMenuWidget::FindIntroWidget(FName Name) const
{
	if (!WidgetTree) return nullptr;
	UWidget* Result = WidgetTree->FindWidget(Name);
	if (Result) return Result;
	TFunction<void(UWidgetTree*)> Visit = [&](UWidgetTree* Tree)
	{
		if (!Tree || Result) return;
		Tree->ForEachWidget([&](UWidget* Widget)
		{
			if (!Result && Widget->GetFName() == Name) Result = Widget;
			// Include the child widget itself, then traverse its layout. Graphics owns its controls.
			if (UUserWidget* Child = Cast<UUserWidget>(Widget))
				if (!Child->IsA<UTunaSweeperGraphicsSettingsWidget>()) Visit(Child->WidgetTree);
		});
	};
	Visit(WidgetTree);
	return Result;
}

void UTunaSweeperIntroMenuWidget::BindScreenWidgets()
{
	if (WidgetTree) WidgetTree->ForEachWidgetAndDescendants([](UWidget* Widget) {
		if (UTextBlock* Text = Cast<UTextBlock>(Widget)) TunaSweeperUIFont::ApplyFont(Text, Text->GetFont().Size);
	});
	// Keep gameplay handlers in the controller; resolve authored child-WBP widgets by name.
	for (TFieldIterator<FObjectPropertyBase> It(StaticClass()); It; ++It)
	{
		if (!It->PropertyClass->IsChildOf(UWidget::StaticClass())) continue;
		if (UWidget* Widget = FindIntroWidget(It->GetFName()))
			if (Widget->IsA(It->PropertyClass)) It->SetObjectPropertyValue_InContainer(this, Widget);
	}
	if (UWidget* View = FindIntroWidget(TEXT("MainMenuPanelView"))) MainMenuPanel = View;
	if (UWidget* View = FindIntroWidget(TEXT("SaveSlotPanelView"))) SaveSlotPanel = View;
	if (UWidget* View = FindIntroWidget(TEXT("SettingsPanelView"))) SettingsPanel = View;
	if (UWidget* View = FindIntroWidget(TEXT("CreditsPanelView"))) CreditsPanel = View;
	if (UWidget* View = FindIntroWidget(TEXT("DemoNoticePanelView"))) DemoNoticePanel = View;
	TitleGraphicsSettingsWidget = Cast<UTunaSweeperGraphicsSettingsWidget>(FindIntroWidget(TEXT("TitleGraphicsSettingsWidget")));
}

void UTunaSweeperIntroMenuWidget::BeginSettingsEntry()
{
	SettingsTransitionTime = 0.0f;
	bSettingsExiting = false;
	TabTransitionTime = -1.0f;
	FadingScreen = nullptr;
	if (UWidget* Page = FindIntroWidget(TEXT("SettingsPageStack"))) Page->SetIsEnabled(true);
	if (MainMenuPanel) { MainMenuPanel->SetVisibility(ESlateVisibility::HitTestInvisible); MainMenuPanel->SetRenderOpacity(1.0f); }
	SetTitleLogoVisible(true);
	TickMenuTransitions(0.0f);
}

void UTunaSweeperIntroMenuWidget::BeginSettingsExit()
{
	if (bSettingsExiting) return;
	SettingsTransitionTime = 0.0f;
	bSettingsExiting = true;
	TabTransitionTime = -1.0f;
	if (MainMenuPanel) MainMenuPanel->SetVisibility(ESlateVisibility::HitTestInvisible);
	SetTitleLogoVisible(true);
}

void UTunaSweeperIntroMenuWidget::RequestSettingsTab(int32 TabIndex)
{
	if (bSettingsExiting || SettingsTransitionTime >= 0.0f) return;
	const int32 Current = bShowingDevelopmentSettingsTab ? 2 : (bShowingInterfaceSettingsTab ? 1 : 0);
	if (TabTransitionTime < 0.0f && TabIndex == Current) return;
	PendingSettingsTab = FMath::Clamp(TabIndex, 0, 2);
	if (TabTransitionTime < 0.0f || bTabContentSwitched) { TabTransitionTime = 0.0f; bTabContentSwitched = false; }
}

void UTunaSweeperIntroMenuWidget::FadeInScreen(UWidget* Screen)
{
	if (FadingScreen) FadingScreen->SetRenderOpacity(1.0f);
	FadingScreen = Screen;
	ScreenFadeTime = 0.0f;
	if (Screen) Screen->SetRenderOpacity(0.0f);
}

void UTunaSweeperIntroMenuWidget::TickMenuTransitions(float DeltaSeconds)
{
	auto Smooth = [](float T) { T = FMath::Clamp(T, 0.0f, 1.0f); return T * T * (3.0f - 2.0f * T); };
	auto Opacity = [this](const TCHAR* Name, float Alpha) { if (UWidget* Widget = FindIntroWidget(Name)) Widget->SetRenderOpacity(Alpha); };
	if (FadingScreen) {
		ScreenFadeTime += DeltaSeconds;
		FadingScreen->SetRenderOpacity(Smooth(ScreenFadeTime / 0.25f));
		if (ScreenFadeTime >= 0.25f) FadingScreen = nullptr;
	}
	if (SettingsTransitionTime >= 0.0f)
	{
		SettingsTransitionTime += DeltaSeconds;
		const float Duration = FMath::Max(SettingsFadeDuration, 0.1f);
		const float Alpha = Smooth(SettingsTransitionTime / Duration);
		const float SettingsAlpha = bSettingsExiting ? 1.0f - Alpha : Alpha;
		Opacity(TEXT("SettingsDimImage"), SettingsAlpha);
		Opacity(TEXT("SettingsBotanicalImage"), SettingsAlpha * 0.48f);
		Opacity(TEXT("SettingsTitleText"), SettingsAlpha);
		Opacity(TEXT("BackFromSettingsButtonBox"), SettingsAlpha);
		if (MainMenuPanel) MainMenuPanel->SetRenderOpacity(1.0f - SettingsAlpha);
		Opacity(TEXT("LogoImage"), 1.0f - SettingsAlpha);
		const TCHAR* Tabs[] = { TEXT("GraphicsTabButtonBox"), TEXT("InterfaceTabButtonBox"), TEXT("DevelopmentTabButtonBox") };
		for (int32 Index = 0; Index < 3; ++Index)
			Opacity(Tabs[Index], bSettingsExiting ? SettingsAlpha : Smooth((SettingsTransitionTime - 0.10f - Index * SettingsTabStagger) / 0.24f));
		Opacity(TEXT("SettingsPageStack"), bSettingsExiting ? SettingsAlpha : Smooth((SettingsTransitionTime - 0.22f) / Duration));
		const float End = bSettingsExiting ? Duration : FMath::Max(Duration + 0.22f, 0.34f + 2.0f * SettingsTabStagger);
		if (SettingsTransitionTime >= End) {
			SettingsTransitionTime = -1.0f;
			if (bSettingsExiting) {
				bSettingsExiting = false; bFinishingSettingsExit = true;
				ShowMainMenu(); bFinishingSettingsExit = false;
			} else {
				if (MainMenuPanel) MainMenuPanel->SetVisibility(ESlateVisibility::Collapsed);
				SetTitleLogoVisible(false);
			}
		}
		return;
	}
	if (TabTransitionTime >= 0.0f)
	{
		TabTransitionTime += DeltaSeconds;
		const float Duration = FMath::Max(SettingsTabFadeDuration, 0.05f);
		if (!bTabContentSwitched && TabTransitionTime >= Duration) {
			bTabContentSwitched = true;
			if (PendingSettingsTab == 2) ShowDevelopmentSettingsTab();
			else if (PendingSettingsTab == 1) ShowInterfaceSettingsTab();
			else ShowGraphicsSettingsTab();
		}
		Opacity(TEXT("SettingsPageStack"), bTabContentSwitched ? Smooth((TabTransitionTime - Duration) / Duration) : 1.0f - Smooth(TabTransitionTime / Duration));
		if (UWidget* Page = FindIntroWidget(TEXT("SettingsPageStack"))) Page->SetIsEnabled(TabTransitionTime >= Duration * 2.0f);
		if (TabTransitionTime >= Duration * 2.0f) TabTransitionTime = -1.0f;
	}
}
