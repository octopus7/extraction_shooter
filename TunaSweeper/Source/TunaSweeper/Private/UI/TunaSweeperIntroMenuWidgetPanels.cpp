#include "TunaSweeperIntroMenuWidgetShared.h"

#include "EngineUtils.h"
#include "Title/TunaSweeperTitlePresentationActor.h"

namespace TunaSweeperDistribution
{
	const TCHAR* SectionName = TEXT("TunaSweeper.Distribution");
	const TCHAR* ChannelKey = TEXT("DistributionChannel");
	const TCHAR* ProjectSettingsSectionName = TEXT("/Script/EngineSettings.GeneralProjectSettings");
	const TCHAR* ProjectVersionKey = TEXT("ProjectVersion");
	const TCHAR* SteamFullGameAppIdKey = TEXT("SteamFullGameAppId");
}

FString UTunaSweeperIntroMenuWidget::GetDistributionChannel() const
{
#if WITH_EDITOR
	if (const UTunaSweeperDistributionPreviewSettings* PreviewSettings = GetDefault<UTunaSweeperDistributionPreviewSettings>(); PreviewSettings && PreviewSettings->DistributionChannel != ETunaSweeperPreviewDistributionChannel::Editor)
	{
		return PreviewSettings->DistributionChannel == ETunaSweeperPreviewDistributionChannel::Steam ? TEXT("Steam") : TEXT("Stove");
	}
#endif
	FString DistributionChannel(TEXT("Steam"));
	GConfig->GetString(TunaSweeperDistribution::SectionName, TunaSweeperDistribution::ChannelKey, DistributionChannel, GGameIni);
	DistributionChannel.TrimStartAndEndInline();
	return DistributionChannel.IsEmpty() ? TEXT("Steam") : DistributionChannel;
}

bool UTunaSweeperIntroMenuWidget::IsSteamDemoDistribution() const
{
#if WITH_EDITOR
	const UTunaSweeperDistributionPreviewSettings* PreviewSettings = GetDefault<UTunaSweeperDistributionPreviewSettings>();
	return PreviewSettings && PreviewSettings->BuildType == ETunaSweeperPreviewBuildType::Demo && GetDistributionChannel().Equals(TEXT("Steam"), ESearchCase::IgnoreCase);
#elif TUNASWEEPER_DEMO
	return GetDistributionChannel().Equals(TEXT("Steam"), ESearchCase::IgnoreCase);
#else
	return false;
#endif
}

void UTunaSweeperIntroMenuWidget::RefreshDistributionPresentation()
{
	if (!WidgetTree) return;
	FString ProjectVersion(TEXT("0.0.0"));
	GConfig->GetString(TunaSweeperDistribution::ProjectSettingsSectionName, TunaSweeperDistribution::ProjectVersionKey, ProjectVersion, GGameIni);
	ProjectVersion.TrimStartAndEndInline();
	if (ProjectVersion.IsEmpty()) ProjectVersion = TEXT("0.0.0");
	if (UTextBlock* VersionText = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("VersionText"))))
	{
		VersionText->SetText(FText::FromString(FString::Printf(TEXT("v%s.%s"), *ProjectVersion, *GetDistributionChannel().ToLower())));
	}
	if (!IsSteamDemoDistribution() && SteamDemoWishlistButton)
	{
		SteamDemoWishlistButton->RemoveFromParent();
		SteamDemoWishlistButton = nullptr;
	}
	EnsureSteamDemoWishlistButton();
}

void UTunaSweeperIntroMenuWidget::EnsureSteamDemoWishlistButton()
{
	if (SteamDemoWishlistButton || !IsSteamDemoDistribution() || !WidgetTree) return;
	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
	if (!RootCanvas) return;
	UButton* WishlistButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("SteamDemoWishlistButton"));
	UTextBlock* WishlistButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SteamDemoWishlistButtonText"));
	if (!WishlistButton || !WishlistButtonText) return;
	WishlistButtonText->SetText(ResolveUiText(FName(TEXT("ui.title.wishlist")), FText::FromString(TEXT("위시리스트에 추가"))));
	WishlistButtonText->SetJustification(ETextJustify::Center);
	WishlistButtonText->SetColorAndOpacity(FSlateColor(FLinearColor(0.90f, 0.96f, 0.96f, 1.0f)));
	TunaSweeperUIFont::ApplyFont(WishlistButtonText, 18, ETunaSweeperUIFontWeight::Bold);
	WishlistButton->SetContent(WishlistButtonText);
	const FVector2D ButtonSize(300.0f, 48.0f);
	FButtonStyle ButtonStyle;
	ButtonStyle.SetNormal(TunaSweeperSettingsUi::MakeRoundedBoxBrush(ButtonSize, FLinearColor(0.03f, 0.08f, 0.09f, 0.85f), FLinearColor(0.32f, 0.90f, 0.96f, 0.90f), 1.5f, 7.0f));
	ButtonStyle.SetHovered(TunaSweeperSettingsUi::MakeRoundedBoxBrush(ButtonSize, FLinearColor(0.06f, 0.16f, 0.18f, 0.95f), FLinearColor(0.58f, 0.96f, 1.0f, 1.0f), 2.0f, 7.0f));
	ButtonStyle.SetPressed(TunaSweeperSettingsUi::MakeRoundedBoxBrush(ButtonSize, FLinearColor(0.02f, 0.05f, 0.06f, 0.95f), FLinearColor(0.22f, 0.70f, 0.76f, 0.90f), 1.0f, 7.0f));
	WishlistButton->SetStyle(ButtonStyle);
	WishlistButton->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleSteamDemoWishlistClicked);
	if (UCanvasPanelSlot* WishlistSlot = RootCanvas->AddChildToCanvas(WishlistButton))
	{
		WishlistSlot->SetAnchors(FAnchors(0.5f, 1.0f));
		WishlistSlot->SetAlignment(FVector2D(0.5f, 1.0f));
		WishlistSlot->SetPosition(FVector2D(0.0f, -42.0f));
		WishlistSlot->SetSize(ButtonSize);
		WishlistSlot->SetZOrder(5);
		SteamDemoWishlistButton = WishlistButton;
	}
}

void UTunaSweeperIntroMenuWidget::HandleSteamDemoWishlistClicked()
{
	if (!IsSteamDemoDistribution())
	{
		return;
	}

	FString FullGameAppId;
	GConfig->GetString(TunaSweeperDistribution::SectionName, TunaSweeperDistribution::SteamFullGameAppIdKey, FullGameAppId, GGameIni);
	if (!FullGameAppId.IsNumeric() || FullGameAppId.IsEmpty())
	{
		return;
	}

	IOnlineSubsystem* SteamSubsystem = IOnlineSubsystem::Get(STEAM_SUBSYSTEM);
	if (!SteamSubsystem)
	{
		return;
	}

	const IOnlineExternalUIPtr ExternalUI = SteamSubsystem->GetExternalUIInterface();
	if (!ExternalUI.IsValid())
	{
		return;
	}

	FShowStoreParams StoreParams;
	StoreParams.ProductId = FullGameAppId;
	StoreParams.bAddToCart = false;
	ExternalUI->ShowStoreUI(0, StoreParams);
}

void UTunaSweeperIntroMenuWidget::ShowMainMenu()
{
	SetTitlePresentationMainMenuActive(true);
	bDifficultyAdjustmentMode = false;
	HideOverlayPanels();
	HideDeleteConfirmDialog();
	ResetDeleteHoldProgress();

	if (MainMenuPanel)
	{
		MainMenuPanel->SetVisibility(ESlateVisibility::Visible);
	}
	SetTitleLogoVisible(true);
	if (SaveSlotPanel)
	{
		SaveSlotPanel->SetVisibility(ESlateVisibility::Collapsed);
	}

	SelectedSaveSlotIndex = INDEX_NONE;
	SetAlwaysNewStartButtonVisible(true);
	RefreshMainMenu();
	RefreshSaveSlotMenu();
}

void UTunaSweeperIntroMenuWidget::ShowDifficultySelection()
{
	SetTitlePresentationMainMenuActive(false);
	EnsureDifficultySelectionPanel();
	HideDeleteConfirmDialog();
	ResetDeleteHoldProgress();
	HideOverlayPanels();

	if (MainMenuPanel)
	{
		MainMenuPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (SaveSlotPanel)
	{
		SaveSlotPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	SetTitleLogoVisible(false);
	SetAlwaysNewStartButtonVisible(false);

	SelectedDifficultyStage = INDEX_NONE;
	if (const UTunaSweeperGameInstance* TunaGameInstance = Cast<UTunaSweeperGameInstance>(GetGameInstance()))
	{
		if (TunaGameInstance->IsActiveSaveSlotDifficultySelected())
		{
			SelectedDifficultyStage = FMath::Clamp(TunaGameInstance->GetActiveSaveSlotDifficultyStage(), 1, 3);
		}
	}

	if (DifficultySelectPanel)
	{
		DifficultySelectPanel->SetVisibility(ESlateVisibility::Visible);
	}

	RefreshDifficultySelectionPanel();
}

void UTunaSweeperIntroMenuWidget::OpenForDifficultyAdjustment()
{
	bDifficultyAdjustmentMode = true;
	bClosingDifficultyAdjustment = false;
	ShowDifficultySelection();

	if (SelectedDifficultyStage == INDEX_NONE)
	{
		SelectedDifficultyStage = 1;
		if (const UTunaSweeperGameInstance* TunaGameInstance = Cast<UTunaSweeperGameInstance>(GetGameInstance()))
		{
			SelectedDifficultyStage = FMath::Clamp(TunaGameInstance->GetActiveSaveSlotDifficultyStage(), 1, 3);
		}
		RefreshDifficultySelectionPanel();
	}

	if (DifficultyStartButton)
	{
		DifficultyStartButton->SetUserFocus(GetOwningPlayer());
	}
}

void UTunaSweeperIntroMenuWidget::CloseDifficultyAdjustment()
{
	if (!bDifficultyAdjustmentMode || bClosingDifficultyAdjustment)
	{
		return;
	}

	bClosingDifficultyAdjustment = true;
	bDifficultyAdjustmentMode = false;
	RemoveFromParent();
	OnDifficultyAdjustmentClosed.Broadcast();
	bClosingDifficultyAdjustment = false;
}

void UTunaSweeperIntroMenuWidget::ShowSaveSlotSelection()
{
	SetTitlePresentationMainMenuActive(false);
	HideDeleteConfirmDialog();
	ResetDeleteHoldProgress();
	HideOverlayPanels();

	if (MainMenuPanel)
	{
		MainMenuPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	SetTitleLogoVisible(true);
	if (SaveSlotPanel)
	{
		SaveSlotPanel->SetVisibility(ESlateVisibility::Visible);
	}

	SelectedSaveSlotIndex = INDEX_NONE;
	SaveSlotSelectionRingAngle = 0.0f;
	SetAlwaysNewStartButtonVisible(false);
	RefreshSaveSlotMenu();
}

void UTunaSweeperIntroMenuWidget::ShowSettingsPanel()
{
	SetTitlePresentationMainMenuActive(false);
	HideDeleteConfirmDialog();
	ResetDeleteHoldProgress();

	if (SaveSlotPanel)
	{
		SaveSlotPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (MainMenuPanel)
	{
		MainMenuPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (DifficultySelectPanel)
	{
		DifficultySelectPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	SetTitleLogoVisible(false);
	if (CreditsPanel)
	{
		CreditsPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (SettingsPanel)
	{
		SettingsPanel->SetVisibility(ESlateVisibility::Visible);
	}

	SetAlwaysNewStartButtonVisible(false);
	if (const UTunaSweeperGameInstance* TunaGameInstance = Cast<UTunaSweeperGameInstance>(GetGameInstance()))
	{
		PendingInterfaceLanguage = TunaGameInstance->GetCurrentTextLanguage();
	}
	ShowGraphicsSettingsTab();
}

void UTunaSweeperIntroMenuWidget::ShowGraphicsSettingsTab()
{
	bShowingInterfaceSettingsTab = false;
	bShowingDevelopmentSettingsTab = false;

	if (GraphicsSettingsPanel)
	{
		GraphicsSettingsPanel->SetVisibility(ESlateVisibility::Visible);
	}
	if (InterfaceSettingsPanel)
	{
		InterfaceSettingsPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (DevelopmentSettingsPanel)
	{
		DevelopmentSettingsPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (SettingsGraphicsTabButton)
	{
		SettingsGraphicsTabButton->SetIsEnabled(false);
	}
	if (SettingsInterfaceTabButton)
	{
		SettingsInterfaceTabButton->SetIsEnabled(true);
	}
	if (SettingsDevelopmentTabButton)
	{
		SettingsDevelopmentTabButton->SetIsEnabled(true);
	}

	RefreshSettingsPanel();
}

void UTunaSweeperIntroMenuWidget::ShowInterfaceSettingsTab()
{
	bShowingInterfaceSettingsTab = true;
	bShowingDevelopmentSettingsTab = false;

	if (const UTunaSweeperGameInstance* TunaGameInstance = Cast<UTunaSweeperGameInstance>(GetGameInstance()))
	{
		PendingInterfaceLanguage = TunaGameInstance->GetCurrentTextLanguage();
	}
	if (GraphicsSettingsPanel)
	{
		GraphicsSettingsPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (InterfaceSettingsPanel)
	{
		InterfaceSettingsPanel->SetVisibility(ESlateVisibility::Visible);
	}
	if (DevelopmentSettingsPanel)
	{
		DevelopmentSettingsPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (SettingsGraphicsTabButton)
	{
		SettingsGraphicsTabButton->SetIsEnabled(true);
	}
	if (SettingsInterfaceTabButton)
	{
		SettingsInterfaceTabButton->SetIsEnabled(false);
	}
	if (SettingsDevelopmentTabButton)
	{
		SettingsDevelopmentTabButton->SetIsEnabled(true);
	}

	RefreshInterfaceSettingsPanel();
}

void UTunaSweeperIntroMenuWidget::ShowDevelopmentSettingsTab()
{
	bShowingInterfaceSettingsTab = false;
	bShowingDevelopmentSettingsTab = true;

	if (GraphicsSettingsPanel)
	{
		GraphicsSettingsPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (InterfaceSettingsPanel)
	{
		InterfaceSettingsPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (DevelopmentSettingsPanel)
	{
		DevelopmentSettingsPanel->SetVisibility(ESlateVisibility::Visible);
	}
	if (SettingsGraphicsTabButton)
	{
		SettingsGraphicsTabButton->SetIsEnabled(true);
	}
	if (SettingsInterfaceTabButton)
	{
		SettingsInterfaceTabButton->SetIsEnabled(true);
	}
	if (SettingsDevelopmentTabButton)
	{
		SettingsDevelopmentTabButton->SetIsEnabled(false);
	}

	RefreshDevelopmentSettingsPanel();
}

void UTunaSweeperIntroMenuWidget::ShowCreditsPanel()
{
	SetTitlePresentationMainMenuActive(false);
	HideDeleteConfirmDialog();
	ResetDeleteHoldProgress();

	if (MainMenuPanel)
	{
		MainMenuPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (SaveSlotPanel)
	{
		SaveSlotPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (DifficultySelectPanel)
	{
		DifficultySelectPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (SettingsPanel)
	{
		SettingsPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	SetTitleLogoVisible(true);
	if (CreditsText)
	{
		CreditsText->SetText(FText::FromString(BuildCreditsColumnText(0)));
	}
	if (CreditsText2)
	{
		CreditsText2->SetText(FText::FromString(BuildCreditsColumnText(1)));
	}
	if (CreditsText3)
	{
		CreditsText3->SetText(FText::FromString(BuildCreditsColumnText(2)));
	}
	if (CreditsPanel)
	{
		CreditsPanel->SetVisibility(ESlateVisibility::Visible);
	}
	SetAlwaysNewStartButtonVisible(false);
	if (CreditsScrollBox)
	{
		CreditsScrollOffset = 0.0f;
		CreditsScrollBox->SetScrollOffset(0.0f);
	}
	if (CreditsScrollBox2)
	{
		CreditsScrollBox2->SetScrollOffset(0.0f);
	}
	if (CreditsScrollBox3)
	{
		CreditsScrollBox3->SetScrollOffset(0.0f);
	}
}

void UTunaSweeperIntroMenuWidget::SetTitlePresentationMainMenuActive(bool bActive)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	for (TActorIterator<ATunaSweeperTitlePresentationActor> ActorIt(World); ActorIt; ++ActorIt)
	{
		ActorIt->SetMainMenuPresentationActive(bActive);
		break;
	}
}

void UTunaSweeperIntroMenuWidget::HideOverlayPanels()
{
	if (DifficultySelectPanel)
	{
		DifficultySelectPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (SettingsPanel)
	{
		SettingsPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (CreditsPanel)
	{
		CreditsPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	SetTitleLogoVisible(true);

	CreditsScrollOffset = 0.0f;
}

void UTunaSweeperIntroMenuWidget::SetTitleLogoVisible(bool bVisible)
{
	if (!WidgetTree)
	{
		return;
	}

	if (UWidget* LogoWidget = WidgetTree->FindWidget(FName(TEXT("LogoImage"))))
	{
		LogoWidget->SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void UTunaSweeperIntroMenuWidget::SelectSaveSlot(int32 SaveSlotIndex)
{
	if (SelectedSaveSlotIndex == SaveSlotIndex)
	{
		return;
	}

	HideDeleteConfirmDialog();
	ResetDeleteHoldProgress();
	SelectedSaveSlotIndex = FMath::Clamp(SaveSlotIndex, 1, 3);
	SaveSlotSelectionRingAngle = 0.0f;
	RefreshSaveSlotMenu();
}
