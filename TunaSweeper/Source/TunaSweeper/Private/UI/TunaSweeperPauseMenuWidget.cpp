#include "UI/TunaSweeperPauseMenuWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Player/TunaSweeperPlayerController.h"
#include "UI/TunaSweeperIntroMenuWidget.h"
#include "UI/TunaSweeperUIFont.h"

void UTunaSweeperPauseMenuWidget::InitializePauseMenu(bool bRaid)
{
	bRaidContext = bRaid;
	SetIsFocusable(true);
}

TSharedRef<SWidget> UTunaSweeperPauseMenuWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget) BuildWidgetTree();
	return Super::RebuildWidget();
}

FText UTunaSweeperPauseMenuWidget::Text(FName Key) const
{
	const UTunaSweeperGameInstance* Instance = Cast<UTunaSweeperGameInstance>(GetGameInstance());
	return Instance ? Instance->ResolveLocalizedText(Key, FText::GetEmpty()) : FText::GetEmpty();
}

UTextBlock* UTunaSweeperPauseMenuWidget::AddText(UVerticalBox* Parent, FName Name, int32 Size)
{
	UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
	TunaSweeperUIFont::ApplyFont(Label, Size);
	Label->SetColorAndOpacity(FSlateColor(FLinearColor(0.94f, 0.91f, 0.82f)));
	Label->SetJustification(ETextJustify::Center);
	Label->SetAutoWrapText(true);
	Parent->AddChildToVerticalBox(Label)->SetPadding(FMargin(0.0f, 8.0f));
	return Label;
}

UButton* UTunaSweeperPauseMenuWidget::AddButton(UVerticalBox* Parent, FName Name, UTextBlock*& Label)
{
	UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
	Button->SetBackgroundColor(FLinearColor(0.19f, 0.23f, 0.20f));
	Label = WidgetTree->ConstructWidget<UTextBlock>();
	TunaSweeperUIFont::ApplyFont(Label, 22);
	Label->SetColorAndOpacity(FSlateColor(FLinearColor(0.94f, 0.91f, 0.82f)));
	Label->SetJustification(ETextJustify::Center);
	Button->SetContent(Label);
	USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>();
	Size->SetHeightOverride(56.0f);
	Size->SetContent(Button);
	Parent->AddChildToVerticalBox(Size)->SetPadding(FMargin(0.0f, 7.0f));
	return Button;
}

void UTunaSweeperPauseMenuWidget::BuildWidgetTree()
{
	UBorder* Background = WidgetTree->ConstructWidget<UBorder>();
	Background->SetBrushColor(FLinearColor(0.015f, 0.025f, 0.02f, 0.83f));
	Background->SetPadding(FMargin(24.0f));
	WidgetTree->RootWidget = Background;
	UOverlay* Layers = WidgetTree->ConstructWidget<UOverlay>();
	Background->SetContent(Layers);
	auto MakePanel = [&](FName Name, float Width, TObjectPtr<UBorder>& Panel)
	{
		USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>();
		Size->SetWidthOverride(Width);
		UOverlaySlot* Slot = Layers->AddChildToOverlay(Size);
		Slot->SetHorizontalAlignment(HAlign_Center);
		Slot->SetVerticalAlignment(VAlign_Center);
		Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), Name);
		Panel->SetBrushColor(FLinearColor(0.045f, 0.065f, 0.055f, 0.98f));
		Panel->SetPadding(FMargin(36.0f, 24.0f));
		Size->SetContent(Panel);
		UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>();
		Panel->SetContent(Column);
		return Column;
	};
	UVerticalBox* Menu = MakePanel(TEXT("PauseMenuPanel"), 440.0f, MenuPanel);
	HeadingText = AddText(Menu, TEXT("PauseHeading"), 32);
	UTextBlock* Label = nullptr;
	ResumeButton = AddButton(Menu, TEXT("ResumeButton"), Label); ResumeText = Label;
	ResumeButton->OnClicked.AddDynamic(this, &ThisClass::HandleResume);
	UButton* SettingsButton = AddButton(Menu, TEXT("SettingsButton"), Label); SettingsText = Label;
	SettingsButton->OnClicked.AddDynamic(this, &ThisClass::HandleSettings);
	UButton* TitleButton = AddButton(Menu, TEXT("ReturnToTitleButton"), Label); TitleText = Label;
	TitleButton->OnClicked.AddDynamic(this, &ThisClass::HandleTitle);
	UButton* QuitButton = AddButton(Menu, TEXT("QuitButton"), Label); QuitText = Label;
	QuitButton->OnClicked.AddDynamic(this, &ThisClass::HandleQuit);
	UVerticalBox* Confirmation = MakePanel(TEXT("PauseConfirmationPanel"), 680.0f, ConfirmationPanel);
	ConfirmationHeading = AddText(Confirmation, TEXT("ConfirmationHeading"), 28);
	WarningText = AddText(Confirmation, TEXT("LossWarning"), 20);
	UButton* ConfirmButton = AddButton(Confirmation, TEXT("ConfirmExitButton"), Label); ConfirmText = Label;
	ConfirmButton->OnClicked.AddDynamic(this, &ThisClass::HandleConfirm);
	CancelButton = AddButton(Confirmation, TEXT("CancelExitButton"), Label); CancelText = Label;
	CancelButton->OnClicked.AddDynamic(this, &ThisClass::HandleCancel);
	ConfirmationPanel->SetVisibility(ESlateVisibility::Collapsed);
}

void UTunaSweeperPauseMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (UTunaSweeperGameInstance* Instance = Cast<UTunaSweeperGameInstance>(GetGameInstance()))
		Instance->OnLanguageChanged.AddUObject(this, &ThisClass::RefreshTexts);
	RefreshTexts();
}

void UTunaSweeperPauseMenuWidget::NativeDestruct()
{
	if (UTunaSweeperGameInstance* Instance = Cast<UTunaSweeperGameInstance>(GetGameInstance()))
		Instance->OnLanguageChanged.RemoveAll(this);
	if (SettingsWidget)
	{
		SettingsWidget->OnPauseSettingsClosed.RemoveAll(this);
		SettingsWidget->ClosePauseSettings();
		SettingsWidget = nullptr;
	}
	Super::NativeDestruct();
}

void UTunaSweeperPauseMenuWidget::RefreshTexts()
{
	if (!HeadingText) return;
	HeadingText->SetText(Text(TEXT("ui.pause.title")));
	ResumeText->SetText(Text(TEXT("ui.pause.resume")));
	SettingsText->SetText(Text(TEXT("ui.title.settings")));
	TitleText->SetText(Text(TEXT("ui.pause.return_to_title")));
	QuitText->SetText(Text(TEXT("ui.title.quit")));
	ConfirmationHeading->SetText(Text(bQuitRequested ? TEXT("ui.pause.confirm_quit") : TEXT("ui.pause.confirm_title")));
	WarningText->SetText(Text(bExitFailed ? TEXT("ui.pause.save_failed") :
		(bRaidContext ? TEXT("ui.pause.raid_warning") : TEXT("ui.pause.bunker_warning"))));
	ConfirmText->SetText(Text(bQuitRequested ? TEXT("ui.title.quit") : TEXT("ui.pause.return_to_title")));
	CancelText->SetText(Text(TEXT("ui.common.cancel")));
}

void UTunaSweeperPauseMenuWidget::HandleResume()
{
	if (ATunaSweeperPlayerController* Controller = Cast<ATunaSweeperPlayerController>(GetOwningPlayer()))
		Controller->ResumeFromPauseMenu();
}

void UTunaSweeperPauseMenuWidget::HandleSettings()
{
	if (SettingsWidget) return;
	UClass* SettingsClass = LoadClass<UTunaSweeperIntroMenuWidget>(nullptr, TEXT("/Game/UI/WBP_IntroMenu.WBP_IntroMenu_C"));
	if (!SettingsClass) return;
	SettingsWidget = CreateWidget<UTunaSweeperIntroMenuWidget>(GetOwningPlayer(), SettingsClass);
	if (!SettingsWidget) return;
	SettingsWidget->PrepareForPauseSettings();
	SettingsWidget->OnPauseSettingsClosed.AddUObject(this, &ThisClass::HandleSettingsClosed);
	MenuPanel->SetVisibility(ESlateVisibility::Collapsed);
	SettingsWidget->AddToViewport(2100);
	SettingsWidget->SetUserFocus(GetOwningPlayer());
}

void UTunaSweeperPauseMenuWidget::HandleSettingsClosed()
{
	SettingsWidget = nullptr;
	MenuPanel->SetVisibility(ESlateVisibility::Visible);
	RefreshTexts();
	ResumeButton->SetUserFocus(GetOwningPlayer());
}

void UTunaSweeperPauseMenuWidget::HandleTitle() { ShowConfirmation(false); }
void UTunaSweeperPauseMenuWidget::HandleQuit() { ShowConfirmation(true); }

void UTunaSweeperPauseMenuWidget::ShowConfirmation(bool bQuit)
{
	bQuitRequested = bQuit;
	bConfirmationOpen = true;
	bExitFailed = false;
	RefreshTexts();
	MenuPanel->SetVisibility(ESlateVisibility::Collapsed);
	ConfirmationPanel->SetVisibility(ESlateVisibility::Visible);
	CancelButton->SetUserFocus(GetOwningPlayer());
}

void UTunaSweeperPauseMenuWidget::ShowExitFailure()
{
	bExitFailed = true;
	RefreshTexts();
	CancelButton->SetUserFocus(GetOwningPlayer());
}

void UTunaSweeperPauseMenuWidget::HandleConfirm()
{
	if (bConfirmationOpen)
		if (ATunaSweeperPlayerController* Controller = Cast<ATunaSweeperPlayerController>(GetOwningPlayer()))
			Controller->ExitFromPauseMenu(bQuitRequested);
}

void UTunaSweeperPauseMenuWidget::HandleCancel()
{
	bConfirmationOpen = false;
	bExitFailed = false;
	ConfirmationPanel->SetVisibility(ESlateVisibility::Collapsed);
	MenuPanel->SetVisibility(ESlateVisibility::Visible);
	ResumeButton->SetUserFocus(GetOwningPlayer());
}

FReply UTunaSweeperPauseMenuWidget::NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& Event)
{
	if (ATunaSweeperPlayerController::IsPauseMenuKey(Event.GetKey(), GetWorld()))
	{
		if (!Event.IsRepeat())
		{
			if (SettingsWidget) SettingsWidget->ClosePauseSettings();
			else if (bConfirmationOpen) HandleCancel();
			else HandleResume();
		}
		return FReply::Handled();
	}
	return Super::NativeOnPreviewKeyDown(Geometry, Event);
}
