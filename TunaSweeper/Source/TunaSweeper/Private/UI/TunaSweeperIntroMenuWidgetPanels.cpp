#include "TunaSweeperIntroMenuWidgetShared.h"

#include "EngineUtils.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/PanelWidget.h"
#include "Settings/TunaSweeperBuildFlavor.h"
#include "Title/TunaSweeperTitlePresentationActor.h"
#include "UI/TunaSweeperGraphicsSettingsWidget.h"

namespace TunaSweeperDistribution
{
	const TCHAR* SectionName = TEXT("TunaSweeper.Distribution");
	const TCHAR* ChannelKey = TEXT("DistributionChannel");
	const TCHAR* ProjectSettingsSectionName = TEXT("/Script/EngineSettings.GeneralProjectSettings");
	const TCHAR* ProjectVersionKey = TEXT("ProjectVersion");
}

FString UTunaSweeperIntroMenuWidget::GetDistributionChannel() const
{
#if WITH_EDITOR
	if (const UTunaSweeperBuildTargetSettings* BuildTargetSettings = GetDefault<UTunaSweeperBuildTargetSettings>())
	{
		return BuildTargetSettings->GetDistributionChannel();
	}
#endif
	FString DistributionChannel(TEXT("Steam"));
	GConfig->GetString(TunaSweeperDistribution::SectionName, TunaSweeperDistribution::ChannelKey, DistributionChannel, GGameIni);
	DistributionChannel.TrimStartAndEndInline();
	return DistributionChannel.IsEmpty() ? TEXT("Steam") : DistributionChannel;
}

void UTunaSweeperIntroMenuWidget::RefreshDistributionPresentation()
{
	if (!WidgetTree) return;
	FString ProjectVersion(TEXT("0.0.0"));
	GConfig->GetString(TunaSweeperDistribution::ProjectSettingsSectionName, TunaSweeperDistribution::ProjectVersionKey, ProjectVersion, GGameIni);
	ProjectVersion.TrimStartAndEndInline();
	if (ProjectVersion.IsEmpty()) ProjectVersion = TEXT("0.0.0");
	if (UTextBlock* VersionText = Cast<UTextBlock>(FindIntroWidget(TEXT("VersionText"))))
	{
		VersionText->SetText(FText::Format(
			ResolveUiText(FName(TEXT("ui.title.version_pattern")), FText::GetEmpty()),
			FText::FromString(ProjectVersion),
			FText::FromString(GetDistributionChannel().ToLower())));
		VersionText->SetJustification(ETextJustify::Right);
		VersionText->SetAutoWrapText(false);
		if (UCanvasPanelSlot* VersionSlot = Cast<UCanvasPanelSlot>(VersionText->Slot))
		{
			VersionSlot->SetAnchors(FAnchors(1.0f, 1.0f));
			VersionSlot->SetAlignment(FVector2D(1.0f, 1.0f));
			VersionSlot->SetPosition(FVector2D(-40.0f, -24.0f));
			VersionSlot->SetAutoSize(true);
		}
	}
}

void UTunaSweeperIntroMenuWidget::ShowMainMenu()
{
	if (bPauseSettingsMode)
	{
		ClosePauseSettings();
		return;
	}
	if (!bFinishingSettingsExit && SettingsPanel && SettingsPanel->GetVisibility() == ESlateVisibility::Visible)
	{
		BeginSettingsExit();
		return;
	}
	if (MainMenuPanel) MainMenuPanel->SetRenderOpacity(1.0f);
	if (UWidget* Logo = FindIntroWidget(TEXT("LogoImage"))) Logo->SetRenderOpacity(1.0f);
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
	RefreshMainMenu();
	RefreshSaveSlotMenu();
}

void UTunaSweeperIntroMenuWidget::ShowDifficultySelection()
{
	const bool bDemoNotice = TunaSweeperBuildFlavor::IsDemo() && !bDifficultyAdjustmentMode;
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

	SelectedDifficultyStage = INDEX_NONE;
	if (const UTunaSweeperGameInstance* TunaGameInstance = Cast<UTunaSweeperGameInstance>(GetGameInstance()))
	{
		if (TunaGameInstance->IsActiveSaveSlotDifficultySelected())
		{
			SelectedDifficultyStage = FMath::Clamp(TunaGameInstance->GetActiveSaveSlotDifficultyStage(), 1, 3);
		}
	}

	if (bDemoNotice && DemoNoticePanel)
	{
		DemoNoticePanel->SetVisibility(ESlateVisibility::Visible);
		FadeInScreen(DemoNoticePanel);
	}
	else if (DifficultySelectPanel)
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
	if (TunaSweeperBuildFlavor::IsDemo())
	{
		ShowMainMenu();
		return;
	}

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
		FadeInScreen(SaveSlotPanel);
	}

	SelectedSaveSlotIndex = INDEX_NONE;
	SaveSlotSelectionRingAngle = 0.0f;
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
	if (DemoNoticePanel)
	{
		DemoNoticePanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	SetTitleLogoVisible(false);
	if (SettingsPanel)
	{
		SettingsPanel->SetVisibility(ESlateVisibility::Visible);
	}

	if (const UTunaSweeperGameInstance* TunaGameInstance = Cast<UTunaSweeperGameInstance>(GetGameInstance()))
	{
		PendingInterfaceLanguage = TunaGameInstance->GetCurrentTextLanguage();
	}
	ShowGraphicsSettingsTab();
	BeginSettingsEntry();
}

void UTunaSweeperIntroMenuWidget::ShowGraphicsSettingsTab()
{
	bShowingInterfaceSettingsTab = false;
	bShowingDevelopmentSettingsTab = false;
	EnsureGraphicsSettingsWidget();
	if (SettingsStatusText)
	{
		SettingsStatusText->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (GraphicsSettingsPanel)
	{
		GraphicsSettingsPanel->SetVisibility(ESlateVisibility::Visible);
	}
	if (TitleGraphicsSettingsWidget)
	{
		TitleGraphicsSettingsWidget->RefreshFromSettings();
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

void UTunaSweeperIntroMenuWidget::EnsureGraphicsSettingsWidget()
{
	if (!TitleGraphicsSettingsWidget)
	{
		TitleGraphicsSettingsWidget = Cast<UTunaSweeperGraphicsSettingsWidget>(FindIntroWidget(TEXT("TitleGraphicsSettingsWidget")));
	}
	if (bPauseSettingsMode && TitleGraphicsSettingsWidget)
		TitleGraphicsSettingsWidget->UseLocalizedStringKeysOnly();
}

void UTunaSweeperIntroMenuWidget::ShowInterfaceSettingsTab()
{
	if (TitleGraphicsSettingsWidget)
	{
		TitleGraphicsSettingsWidget->DiscardPendingChanges();
	}
	bShowingInterfaceSettingsTab = true;
	bShowingDevelopmentSettingsTab = false;
	if (SettingsStatusText)
	{
		SettingsStatusText->SetVisibility(ESlateVisibility::Visible);
	}

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
#if UE_BUILD_SHIPPING
	ShowGraphicsSettingsTab();
	return;
#else
	if (TitleGraphicsSettingsWidget)
	{
		TitleGraphicsSettingsWidget->DiscardPendingChanges();
	}
	bShowingInterfaceSettingsTab = false;
	bShowingDevelopmentSettingsTab = true;
	if (SettingsStatusText)
	{
		SettingsStatusText->SetVisibility(ESlateVisibility::Visible);
	}

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
#endif
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
	if (LaboratoryPanel)
	{
		LaboratoryPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (DifficultySelectPanel)
	{
		DifficultySelectPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (DemoNoticePanel)
	{
		DemoNoticePanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (SettingsPanel)
	{
		SettingsPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	SetTitleLogoVisible(true);
}

void UTunaSweeperIntroMenuWidget::SetTitleLogoVisible(bool bVisible)
{
	if (!WidgetTree)
	{
		return;
	}

	if (UWidget* LogoWidget = FindIntroWidget(FName(TEXT("LogoImage"))))
	{
		LogoWidget->SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (DemoBuildImage)
	{
		DemoBuildImage->SetVisibility(
			bVisible && TunaSweeperBuildFlavor::IsDemo()
				? ESlateVisibility::HitTestInvisible
				: ESlateVisibility::Collapsed);
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
