#include "UI/TunaSweeperPauseMenuWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Components/BackgroundBlur.h"
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
	Label->SetColorAndOpacity(FSlateColor(FLinearColor(0.93f, 0.99f, 1.0f)));
	Label->SetShadowOffset(Size >= 28 ? FVector2D(3.0f, 3.0f) : FVector2D(2.0f, 2.0f));
	Label->SetShadowColorAndOpacity(FLinearColor(0.005f, 0.025f, 0.04f, Size >= 28 ? 0.80f : 0.65f));
	Label->SetJustification(ETextJustify::Center);
	Label->SetAutoWrapText(true);
	Parent->AddChildToVerticalBox(Label)->SetPadding(FMargin(0.0f, 8.0f, 0.0f, Size >= 28 ? 26.0f : 18.0f));
	return Label;
}

UButton* UTunaSweeperPauseMenuWidget::AddButton(UVerticalBox* Parent, FName Name, UTextBlock*& Label)
{
	UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
	FButtonStyle Style = Button->GetStyle();
	Style.SetNormal(FSlateRoundedBoxBrush(FLinearColor(0.012f, 0.35f, 0.43f), 15.0f));
	Style.SetHovered(FSlateRoundedBoxBrush(FLinearColor(0.025f, 0.49f, 0.58f), 15.0f));
	Style.SetPressed(FSlateRoundedBoxBrush(FLinearColor(0.008f, 0.23f, 0.29f), 15.0f));
	Style.SetDisabled(FSlateRoundedBoxBrush(FLinearColor(0.08f, 0.18f, 0.20f), 15.0f));
	Style.SetNormalPadding(FMargin(22.0f, 12.0f));
	Style.SetPressedPadding(FMargin(22.0f, 13.0f, 22.0f, 11.0f));
	Button->SetStyle(Style);
	Button->SetBackgroundColor(FLinearColor::White);
	Button->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	MenuButtons.Add(Button);
	Label = WidgetTree->ConstructWidget<UTextBlock>();
	TunaSweeperUIFont::ApplyFont(Label, 23, ETunaSweeperUIFontWeight::Bold);
	Label->SetColorAndOpacity(FSlateColor(FLinearColor(0.97f, 1.0f, 1.0f)));
	Label->SetShadowOffset(FVector2D(1.0f, 1.0f));
	Label->SetShadowColorAndOpacity(FLinearColor(0.005f, 0.035f, 0.05f, 0.55f));
	Label->SetJustification(ETextJustify::Center);
	Button->SetContent(Label);
	USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>();
	Size->SetHeightOverride(64.0f);
	Size->SetContent(Button);
	Parent->AddChildToVerticalBox(Size)->SetPadding(FMargin(0.0f, 8.0f));
	return Button;
}

void UTunaSweeperPauseMenuWidget::BuildWidgetTree()
{
	UOverlay* Layers = WidgetTree->ConstructWidget<UOverlay>();
	WidgetTree->RootWidget = Layers;
	UBackgroundBlur* Blur = WidgetTree->ConstructWidget<UBackgroundBlur>(UBackgroundBlur::StaticClass(), TEXT("PauseBackgroundBlur"));
	Blur->SetBlurStrength(8.0f);
	Blur->SetApplyAlphaToBlur(true);
	UOverlaySlot* BlurSlot = Layers->AddChildToOverlay(Blur);
	BlurSlot->SetHorizontalAlignment(HAlign_Fill);
	BlurSlot->SetVerticalAlignment(VAlign_Fill);
	UBorder* Dim = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("PauseBackgroundTint"));
	Dim->SetBrushColor(FLinearColor(0.008f, 0.018f, 0.05f, 0.25f));
	UOverlaySlot* DimSlot = Layers->AddChildToOverlay(Dim);
	DimSlot->SetHorizontalAlignment(HAlign_Fill);
	DimSlot->SetVerticalAlignment(VAlign_Fill);
	auto MakePanel = [&](FName Name, float Width, TObjectPtr<UBorder>& Panel)
	{
		USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>();
		Size->SetWidthOverride(Width);
		UOverlaySlot* Slot = Layers->AddChildToOverlay(Size);
		Slot->SetHorizontalAlignment(HAlign_Center);
		Slot->SetVerticalAlignment(VAlign_Center);
		Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), Name);
		Panel->SetBrushColor(FLinearColor::Transparent);
		Panel->SetPadding(FMargin(24.0f));
		Size->SetContent(Panel);
		UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>();
		Panel->SetContent(Column);
		return Column;
	};
	UVerticalBox* Menu = MakePanel(TEXT("PauseMenuPanel"), 440.0f, MenuPanel);
	HeadingText = AddText(Menu, TEXT("PauseHeading"), 38);
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

void UTunaSweeperPauseMenuWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	for (UButton* Button : MenuButtons)
	{
		if (!Button) continue;
		const bool bHighlighted = Button->IsHovered() || Button->HasKeyboardFocus();
		const float TargetScale = Button->IsPressed() ? 0.985f : (bHighlighted ? 1.025f : 1.0f);
		const float Scale = FMath::FInterpTo(Button->GetRenderTransform().Scale.X, TargetScale, InDeltaTime, 18.0f);
		Button->SetRenderScale(FVector2D(Scale));
	}
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
