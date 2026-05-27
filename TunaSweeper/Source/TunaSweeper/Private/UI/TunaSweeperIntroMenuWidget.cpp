#include "UI/TunaSweeperIntroMenuWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "DLSSLibrary.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Engine/Engine.h"
#include "Game/TunaSweeperGameInstance.h"
#include "GameFramework/GameUserSettings.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Slate/WidgetTransform.h"
#include "Subsystem/TunaSweeperBgmSubsystem.h"
#include "TimerManager.h"
#include "UI/TunaSweeperScreenFadeWidget.h"
#include "UI/TunaSweeperTitleWindParticleWidget.h"
#include "UI/TunaSweeperUIFont.h"

namespace TunaSweeperTitleGraphicsSettings
{
	const TCHAR* SectionName = TEXT("TunaSweeper.GraphicsSettings");
	const TCHAR* DLSSModeKey = TEXT("DLSSMode");

	UDLSSMode ToDLSSMode(ETunaSweeperTitleDLSSMode Mode)
	{
		switch (Mode)
		{
		case ETunaSweeperTitleDLSSMode::Quality:
			return UDLSSMode::Quality;
		case ETunaSweeperTitleDLSSMode::Balanced:
			return UDLSSMode::Balanced;
		case ETunaSweeperTitleDLSSMode::Performance:
			return UDLSSMode::Performance;
		case ETunaSweeperTitleDLSSMode::Off:
		default:
			return UDLSSMode::Off;
		}
	}

	ETunaSweeperTitleDLSSMode ToTitleDLSSMode(int32 ConfigValue)
	{
		switch (ConfigValue)
		{
		case 1:
			return ETunaSweeperTitleDLSSMode::Quality;
		case 2:
			return ETunaSweeperTitleDLSSMode::Balanced;
		case 3:
			return ETunaSweeperTitleDLSSMode::Performance;
		case 0:
		default:
			return ETunaSweeperTitleDLSSMode::Off;
		}
	}

	int32 ToConfigValue(ETunaSweeperTitleDLSSMode Mode)
	{
		switch (Mode)
		{
		case ETunaSweeperTitleDLSSMode::Quality:
			return 1;
		case ETunaSweeperTitleDLSSMode::Balanced:
			return 2;
		case ETunaSweeperTitleDLSSMode::Performance:
			return 3;
		case ETunaSweeperTitleDLSSMode::Off:
		default:
			return 0;
		}
	}
}

void UTunaSweeperIntroMenuWidget::PrepareForInitialViewport()
{
	ResetTitleViewportLayoutState();
	TunaSweeperUIFont::ApplyFontToWidgetTree(this);
	ApplyTitleMenuButtonContentLayout();
	EnsureAlwaysNewStartButton();
	EnsureTitleWindParticleOverlay();
	InvalidateLayoutAndVolatility();
	ForceLayoutPrepass();
}

void UTunaSweeperIntroMenuWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	ResetTitleViewportLayoutState();
}

void UTunaSweeperIntroMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();
	ResetTitleViewportLayoutState();
	SetIsFocusable(true);
	TunaSweeperUIFont::ApplyFontToWidgetTree(this);
	EnsureTitleWindParticleOverlay();

	if (StartButton)
	{
		StartButton->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleStartClicked);
		StartButton->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleStartClicked);
	}

	if (SlotSelectButton)
	{
		SlotSelectButton->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleSlotSelectClicked);
		SlotSelectButton->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleSlotSelectClicked);
	}

	if (SettingsButton)
	{
		SettingsButton->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleSettingsClicked);
		SettingsButton->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleSettingsClicked);
	}

	if (CreditsButton)
	{
		CreditsButton->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleCreditsClicked);
		CreditsButton->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleCreditsClicked);
	}

	if (QuitButton)
	{
		QuitButton->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleQuitClicked);
		QuitButton->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleQuitClicked);
	}

	EnsureAlwaysNewStartButton();
	if (AlwaysNewStartButton)
	{
		AlwaysNewStartButton->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleAlwaysNewStartClicked);
		AlwaysNewStartButton->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleAlwaysNewStartClicked);
	}

	if (SaveSlot1Button)
	{
		SaveSlot1Button->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleSaveSlot1Focused);
		SaveSlot1Button->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleSaveSlot1Focused);
		SaveSlot1Button->OnHovered.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleSaveSlot1Focused);
		SaveSlot1Button->OnHovered.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleSaveSlot1Focused);
	}

	if (SaveSlot2Button)
	{
		SaveSlot2Button->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleSaveSlot2Focused);
		SaveSlot2Button->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleSaveSlot2Focused);
		SaveSlot2Button->OnHovered.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleSaveSlot2Focused);
		SaveSlot2Button->OnHovered.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleSaveSlot2Focused);
	}

	if (SaveSlot3Button)
	{
		SaveSlot3Button->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleSaveSlot3Focused);
		SaveSlot3Button->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleSaveSlot3Focused);
		SaveSlot3Button->OnHovered.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleSaveSlot3Focused);
		SaveSlot3Button->OnHovered.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleSaveSlot3Focused);
	}

	if (PrimarySaveSlotButton)
	{
		PrimarySaveSlotButton->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandlePrimarySaveSlotClicked);
		PrimarySaveSlotButton->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandlePrimarySaveSlotClicked);
	}

	if (DeleteSaveSlotButton)
	{
		DeleteSaveSlotButton->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleDeleteSaveSlotClicked);
		DeleteSaveSlotButton->OnPressed.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleDeleteSaveSlotPressed);
		DeleteSaveSlotButton->OnPressed.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleDeleteSaveSlotPressed);
		DeleteSaveSlotButton->OnReleased.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleDeleteSaveSlotReleased);
		DeleteSaveSlotButton->OnReleased.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleDeleteSaveSlotReleased);
	}

	if (BackToMainMenuButton)
	{
		BackToMainMenuButton->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleBackToMainMenuClicked);
		BackToMainMenuButton->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleBackToMainMenuClicked);
	}

	if (ConfirmDeleteButton)
	{
		ConfirmDeleteButton->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleConfirmDeleteClicked);
		ConfirmDeleteButton->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleConfirmDeleteClicked);
	}

	if (CancelDeleteButton)
	{
		CancelDeleteButton->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleCancelDeleteClicked);
		CancelDeleteButton->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleCancelDeleteClicked);
	}

	if (SettingsGraphicsTabButton)
	{
		SettingsGraphicsTabButton->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleSettingsGraphicsTabClicked);
		SettingsGraphicsTabButton->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleSettingsGraphicsTabClicked);
	}

	if (SettingsInterfaceTabButton)
	{
		SettingsInterfaceTabButton->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleSettingsInterfaceTabClicked);
		SettingsInterfaceTabButton->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleSettingsInterfaceTabClicked);
	}

	if (WindowedModeButton)
	{
		WindowedModeButton->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleWindowedModeClicked);
		WindowedModeButton->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleWindowedModeClicked);
	}

	if (BorderlessWindowModeButton)
	{
		BorderlessWindowModeButton->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleBorderlessWindowModeClicked);
		BorderlessWindowModeButton->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleBorderlessWindowModeClicked);
	}

	if (FullscreenModeButton)
	{
		FullscreenModeButton->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleFullscreenModeClicked);
		FullscreenModeButton->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleFullscreenModeClicked);
	}

	if (Resolution1280Button)
	{
		Resolution1280Button->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleResolution1280Clicked);
		Resolution1280Button->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleResolution1280Clicked);
	}

	if (Resolution1600Button)
	{
		Resolution1600Button->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleResolution1600Clicked);
		Resolution1600Button->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleResolution1600Clicked);
	}

	if (Resolution1920Button)
	{
		Resolution1920Button->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleResolution1920Clicked);
		Resolution1920Button->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleResolution1920Clicked);
	}

	if (Resolution2560Button)
	{
		Resolution2560Button->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleResolution2560Clicked);
		Resolution2560Button->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleResolution2560Clicked);
	}

	if (Resolution3840Button)
	{
		Resolution3840Button->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleResolution3840Clicked);
		Resolution3840Button->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleResolution3840Clicked);
	}

	if (DLSSOffButton)
	{
		DLSSOffButton->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleDLSSOffClicked);
		DLSSOffButton->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleDLSSOffClicked);
	}

	if (DLSSQualityButton)
	{
		DLSSQualityButton->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleDLSSQualityClicked);
		DLSSQualityButton->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleDLSSQualityClicked);
	}

	if (DLSSBalancedButton)
	{
		DLSSBalancedButton->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleDLSSBalancedClicked);
		DLSSBalancedButton->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleDLSSBalancedClicked);
	}

	if (DLSSPerformanceButton)
	{
		DLSSPerformanceButton->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleDLSSPerformanceClicked);
		DLSSPerformanceButton->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleDLSSPerformanceClicked);
	}

	if (BackFromSettingsButton)
	{
		BackFromSettingsButton->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleBackFromSettingsClicked);
		BackFromSettingsButton->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleBackFromSettingsClicked);
	}

	if (LanguageEnglishButton)
	{
		LanguageEnglishButton->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleLanguageEnglishClicked);
		LanguageEnglishButton->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleLanguageEnglishClicked);
	}

	if (LanguageKoreanButton)
	{
		LanguageKoreanButton->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleLanguageKoreanClicked);
		LanguageKoreanButton->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleLanguageKoreanClicked);
	}

	if (LanguageJapaneseButton)
	{
		LanguageJapaneseButton->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleLanguageJapaneseClicked);
		LanguageJapaneseButton->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleLanguageJapaneseClicked);
	}

	if (ConfirmInterfaceSettingsButton)
	{
		ConfirmInterfaceSettingsButton->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleConfirmInterfaceSettingsClicked);
		ConfirmInterfaceSettingsButton->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleConfirmInterfaceSettingsClicked);
	}

	if (CancelInterfaceSettingsButton)
	{
		CancelInterfaceSettingsButton->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleCancelInterfaceSettingsClicked);
		CancelInterfaceSettingsButton->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleCancelInterfaceSettingsClicked);
	}

	if (BackFromCreditsButton)
	{
		BackFromCreditsButton->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleBackFromCreditsClicked);
		BackFromCreditsButton->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleBackFromCreditsClicked);
	}

	if (UTunaSweeperGameInstance* TunaGameInstance = Cast<UTunaSweeperGameInstance>(GetGameInstance()))
	{
		TunaGameInstance->OnLanguageChanged.RemoveAll(this);
		TunaGameInstance->OnLanguageChanged.AddUObject(this, &UTunaSweeperIntroMenuWidget::HandleLanguageChanged);
	}

	ApplyTitleMenuButtonContentLayout();
	RefreshLocalizedTexts();
	LoadTitleGraphicsSettings();
	ApplyDLSSModeToRuntime(PreferredDLSSMode);

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

	SelectedSaveSlotIndex = INDEX_NONE;
	ResetDeleteHoldProgress();
	HideDeleteConfirmDialog();
	HideOverlayPanels();
	ShowMainMenu();
	InvalidateLayoutAndVolatility();
	ForceLayoutPrepass();
}

void UTunaSweeperIntroMenuWidget::NativeDestruct()
{
	if (UTunaSweeperGameInstance* TunaGameInstance = Cast<UTunaSweeperGameInstance>(GetGameInstance()))
	{
		TunaGameInstance->OnLanguageChanged.RemoveAll(this);
	}

	Super::NativeDestruct();
}

FReply UTunaSweeperIntroMenuWidget::NativeOnPreviewKeyDown(
	const FGeometry& InGeometry,
	const FKeyEvent& InKeyEvent)
{
	if (!InKeyEvent.IsRepeat() && InKeyEvent.GetKey() == EKeys::R)
	{
		ReloadIntroLevel();
		return FReply::Handled();
	}

	return Super::NativeOnPreviewKeyDown(InGeometry, InKeyEvent);
}

void UTunaSweeperIntroMenuWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (IsCreditsPanelVisible() && CreditsScrollBox)
	{
		CreditsScrollOffset += InDeltaTime * CreditsScrollSpeed;
		CreditsScrollBox->SetScrollOffset(CreditsScrollOffset);
		if (CreditsScrollBox2)
		{
			CreditsScrollBox2->SetScrollOffset(CreditsScrollOffset);
		}
		if (CreditsScrollBox3)
		{
			CreditsScrollBox3->SetScrollOffset(CreditsScrollOffset);
		}
		if (CreditsScrollOffset > 3600.0f)
		{
			CreditsScrollOffset = 0.0f;
			CreditsScrollBox->SetScrollOffset(0.0f);
			if (CreditsScrollBox2)
			{
				CreditsScrollBox2->SetScrollOffset(0.0f);
			}
			if (CreditsScrollBox3)
			{
				CreditsScrollBox3->SetScrollOffset(0.0f);
			}
		}
	}

	if (!IsSaveSlotSelectionVisible())
	{
		if (bDeleteHoldActive || DeleteHoldElapsedSeconds > 0.0f)
		{
			ResetDeleteHoldProgress();
		}
		return;
	}

	if (SaveSlot1Button && SaveSlot1Button->HasKeyboardFocus())
	{
		SelectSaveSlot(1);
	}
	else if (SaveSlot2Button && SaveSlot2Button->HasKeyboardFocus())
	{
		SelectSaveSlot(2);
	}
	else if (SaveSlot3Button && SaveSlot3Button->HasKeyboardFocus())
	{
		SelectSaveSlot(3);
	}

	if (!bDeleteHoldActive)
	{
		return;
	}

	if (bDeleteConfirmVisible || !CanDeleteSelectedSaveSlot())
	{
		ResetDeleteHoldProgress();
		return;
	}

	DeleteHoldElapsedSeconds += InDeltaTime;
	const float HoldProgress = FMath::Clamp(DeleteHoldElapsedSeconds / DeleteHoldDurationSeconds, 0.0f, 1.0f);
	SetDeleteHoldProgress(HoldProgress);

	if (HoldProgress >= 1.0f)
	{
		bDeleteHoldActive = false;
		ShowDeleteConfirmDialog();
	}
}

void UTunaSweeperIntroMenuWidget::HandleStartClicked()
{
	BeginStartTravel(false);
}

void UTunaSweeperIntroMenuWidget::HandleSlotSelectClicked()
{
	ShowSaveSlotSelection();
}

void UTunaSweeperIntroMenuWidget::HandleSettingsClicked()
{
	ShowSettingsPanel();
}

void UTunaSweeperIntroMenuWidget::HandleCreditsClicked()
{
	ShowCreditsPanel();
}

void UTunaSweeperIntroMenuWidget::HandleQuitClicked()
{
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}

void UTunaSweeperIntroMenuWidget::HandleAlwaysNewStartClicked()
{
	BeginStartTravel(true);
}

void UTunaSweeperIntroMenuWidget::BeginStartTravel(bool bAlwaysNewStart)
{
	if (bStartTravelPending)
	{
		return;
	}

	UTunaSweeperGameInstance* TunaGameInstance = Cast<UTunaSweeperGameInstance>(GetGameInstance());
	FName TargetLevelName = StartTargetLevelName;
	if (TunaGameInstance)
	{
		const int32 ActiveSaveSlotIndex = TunaGameInstance->GetActiveSaveSlotIndex();
		if (bAlwaysNewStart)
		{
			if (!TunaGameInstance->DeleteSaveSlotAndStartNewGame(ActiveSaveSlotIndex))
			{
				return;
			}
		}
		else
		{
			const FTunaSweeperSaveSlotSummary Summary = TunaGameInstance->GetSaveSlotSummary(ActiveSaveSlotIndex);
			TunaGameInstance->ActivateSaveSlot(ActiveSaveSlotIndex, !Summary.bHasData);
		}
		TargetLevelName = TunaGameInstance->ResolveInitialGameplayLevelName();
	}

	if (TargetLevelName.IsNone())
	{
		return;
	}

	bStartTravelPending = true;
	PendingStartTargetLevelName = TargetLevelName;
	SetStartTravelControlsEnabled(false);

	const float FadeDuration = FMath::Max(0.01f, StartTransitionFadeSeconds);
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UTunaSweeperBgmSubsystem* BgmSubsystem = GameInstance->GetSubsystem<UTunaSweeperBgmSubsystem>())
		{
			BgmSubsystem->FadeOutAndStop(FadeDuration);
		}
	}

	StartTravelFadeWidget = CreateWidget<UTunaSweeperScreenFadeWidget>(
		GetOwningPlayer(),
		UTunaSweeperScreenFadeWidget::StaticClass());
	if (StartTravelFadeWidget)
	{
		StartTravelFadeWidget->AddToViewport(1000);
		StartTravelFadeWidget->StartFadeToBlack(
			FadeDuration,
			FSimpleDelegate::CreateUObject(this, &UTunaSweeperIntroMenuWidget::OpenPendingStartTargetLevel));
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			StartTravelTimerHandle,
			this,
			&UTunaSweeperIntroMenuWidget::OpenPendingStartTargetLevel,
			FadeDuration,
			false);
	}
	else
	{
		OpenPendingStartTargetLevel();
	}
}

void UTunaSweeperIntroMenuWidget::ReloadIntroLevel()
{
	if (bStartTravelPending)
	{
		return;
	}

	bStartTravelPending = true;
	PendingStartTargetLevelName = NAME_None;
	SetStartTravelControlsEnabled(false);
	UGameplayStatics::OpenLevel(this, FName(TEXT("IntroMap")));
}

void UTunaSweeperIntroMenuWidget::HandleSaveSlot1Focused()
{
	SelectSaveSlot(1);
}

void UTunaSweeperIntroMenuWidget::HandleSaveSlot2Focused()
{
	SelectSaveSlot(2);
}

void UTunaSweeperIntroMenuWidget::HandleSaveSlot3Focused()
{
	SelectSaveSlot(3);
}

void UTunaSweeperIntroMenuWidget::HandlePrimarySaveSlotClicked()
{
	if (SelectedSaveSlotIndex == INDEX_NONE)
	{
		return;
	}

	UTunaSweeperGameInstance* TunaGameInstance = Cast<UTunaSweeperGameInstance>(GetGameInstance());
	if (!TunaGameInstance)
	{
		return;
	}

	TunaGameInstance->SetActiveSaveSlotIndex(SelectedSaveSlotIndex);
	ShowMainMenu();
}

void UTunaSweeperIntroMenuWidget::HandleDeleteSaveSlotClicked()
{
	HandleDeleteSaveSlotPressed();
}

void UTunaSweeperIntroMenuWidget::HandleDeleteSaveSlotPressed()
{
	if (!CanDeleteSelectedSaveSlot() || bDeleteConfirmVisible)
	{
		ResetDeleteHoldProgress();
		return;
	}

	bDeleteHoldActive = true;
	DeleteHoldElapsedSeconds = 0.0f;
	SetDeleteHoldProgress(0.0f);
}

void UTunaSweeperIntroMenuWidget::HandleDeleteSaveSlotReleased()
{
	if (!bDeleteConfirmVisible)
	{
		ResetDeleteHoldProgress();
	}
	else
	{
		bDeleteHoldActive = false;
	}
}

void UTunaSweeperIntroMenuWidget::HandleBackToMainMenuClicked()
{
	ShowMainMenu();
}

void UTunaSweeperIntroMenuWidget::HandleConfirmDeleteClicked()
{
	if (CanDeleteSelectedSaveSlot())
	{
		if (UTunaSweeperGameInstance* TunaGameInstance = Cast<UTunaSweeperGameInstance>(GetGameInstance()))
		{
			TunaGameInstance->DeleteSaveSlot(SelectedSaveSlotIndex);
		}
	}

	HideDeleteConfirmDialog();
	ResetDeleteHoldProgress();
	RefreshSaveSlotMenu();
	RefreshMainMenu();
}

void UTunaSweeperIntroMenuWidget::HandleCancelDeleteClicked()
{
	HideDeleteConfirmDialog();
	ResetDeleteHoldProgress();
}

void UTunaSweeperIntroMenuWidget::HandleSettingsGraphicsTabClicked()
{
	ShowGraphicsSettingsTab();
}

void UTunaSweeperIntroMenuWidget::HandleSettingsInterfaceTabClicked()
{
	ShowInterfaceSettingsTab();
}

void UTunaSweeperIntroMenuWidget::HandleWindowedModeClicked()
{
	ApplyDisplaySettings(EWindowMode::Windowed);
}

void UTunaSweeperIntroMenuWidget::HandleBorderlessWindowModeClicked()
{
	ApplyDisplaySettings(EWindowMode::WindowedFullscreen);
}

void UTunaSweeperIntroMenuWidget::HandleFullscreenModeClicked()
{
	ApplyDisplaySettings(EWindowMode::Fullscreen);
}

void UTunaSweeperIntroMenuWidget::HandleResolution1280Clicked()
{
	ApplyResolutionSetting(FIntPoint(1280, 720));
}

void UTunaSweeperIntroMenuWidget::HandleResolution1600Clicked()
{
	ApplyResolutionSetting(FIntPoint(1600, 900));
}

void UTunaSweeperIntroMenuWidget::HandleResolution1920Clicked()
{
	ApplyResolutionSetting(FIntPoint(1920, 1080));
}

void UTunaSweeperIntroMenuWidget::HandleResolution2560Clicked()
{
	ApplyResolutionSetting(FIntPoint(2560, 1440));
}

void UTunaSweeperIntroMenuWidget::HandleResolution3840Clicked()
{
	ApplyResolutionSetting(FIntPoint(3840, 2160));
}

void UTunaSweeperIntroMenuWidget::HandleDLSSOffClicked()
{
	ApplyDLSSSetting(ETunaSweeperTitleDLSSMode::Off);
}

void UTunaSweeperIntroMenuWidget::HandleDLSSQualityClicked()
{
	ApplyDLSSSetting(ETunaSweeperTitleDLSSMode::Quality);
}

void UTunaSweeperIntroMenuWidget::HandleDLSSBalancedClicked()
{
	ApplyDLSSSetting(ETunaSweeperTitleDLSSMode::Balanced);
}

void UTunaSweeperIntroMenuWidget::HandleDLSSPerformanceClicked()
{
	ApplyDLSSSetting(ETunaSweeperTitleDLSSMode::Performance);
}

void UTunaSweeperIntroMenuWidget::HandleBackFromSettingsClicked()
{
	ShowMainMenu();
}

void UTunaSweeperIntroMenuWidget::HandleLanguageEnglishClicked()
{
	PendingInterfaceLanguage = ETunaSweeperItemTextLanguage::English;
	RefreshInterfaceSettingsPanel();
}

void UTunaSweeperIntroMenuWidget::HandleLanguageKoreanClicked()
{
	PendingInterfaceLanguage = ETunaSweeperItemTextLanguage::Korean;
	RefreshInterfaceSettingsPanel();
}

void UTunaSweeperIntroMenuWidget::HandleLanguageJapaneseClicked()
{
	PendingInterfaceLanguage = ETunaSweeperItemTextLanguage::Japanese;
	RefreshInterfaceSettingsPanel();
}

void UTunaSweeperIntroMenuWidget::HandleConfirmInterfaceSettingsClicked()
{
	if (UTunaSweeperGameInstance* TunaGameInstance = Cast<UTunaSweeperGameInstance>(GetGameInstance()))
	{
		TunaGameInstance->SetCurrentTextLanguage(PendingInterfaceLanguage, true);
	}

	ShowMainMenu();
}

void UTunaSweeperIntroMenuWidget::HandleCancelInterfaceSettingsClicked()
{
	ShowMainMenu();
}

void UTunaSweeperIntroMenuWidget::HandleBackFromCreditsClicked()
{
	ShowMainMenu();
}

void UTunaSweeperIntroMenuWidget::HandleLanguageChanged()
{
	bTitleMenuButtonContentLayoutApplied = false;
	ApplyTitleMenuButtonContentLayout();
	RefreshLocalizedTexts();
	RefreshMainMenu();
	if (IsSaveSlotSelectionVisible())
	{
		RefreshSaveSlotMenu();
	}
	if (SettingsPanel && SettingsPanel->GetVisibility() == ESlateVisibility::Visible)
	{
		RefreshSettingsPanel();
	}
}

void UTunaSweeperIntroMenuWidget::ShowMainMenu()
{
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

void UTunaSweeperIntroMenuWidget::ShowSaveSlotSelection()
{
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
	SetAlwaysNewStartButtonVisible(false);
	RefreshSaveSlotMenu();
}

void UTunaSweeperIntroMenuWidget::ShowSettingsPanel()
{
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

	if (GraphicsSettingsPanel)
	{
		GraphicsSettingsPanel->SetVisibility(ESlateVisibility::Visible);
	}
	if (InterfaceSettingsPanel)
	{
		InterfaceSettingsPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (SettingsGraphicsTabButton)
	{
		SettingsGraphicsTabButton->SetIsEnabled(false);
	}
	if (SettingsInterfaceTabButton)
	{
		SettingsInterfaceTabButton->SetIsEnabled(true);
	}

	RefreshSettingsPanel();
}

void UTunaSweeperIntroMenuWidget::ShowInterfaceSettingsTab()
{
	bShowingInterfaceSettingsTab = true;

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
	if (SettingsGraphicsTabButton)
	{
		SettingsGraphicsTabButton->SetIsEnabled(true);
	}
	if (SettingsInterfaceTabButton)
	{
		SettingsInterfaceTabButton->SetIsEnabled(false);
	}

	RefreshInterfaceSettingsPanel();
}

void UTunaSweeperIntroMenuWidget::ShowCreditsPanel()
{
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

void UTunaSweeperIntroMenuWidget::HideOverlayPanels()
{
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
	RefreshSaveSlotMenu();
}

void UTunaSweeperIntroMenuWidget::RefreshMainMenu()
{
	FTunaSweeperSaveSlotSummary Summary;
	if (const UTunaSweeperGameInstance* TunaGameInstance = Cast<UTunaSweeperGameInstance>(GetGameInstance()))
	{
		Summary = TunaGameInstance->GetSaveSlotSummary(TunaGameInstance->GetActiveSaveSlotIndex());
	}

	if (CurrentSaveSlotText)
	{
		CurrentSaveSlotText->SetText(BuildCurrentSaveSlotText(Summary.SaveSlotIndex));
	}

	if (StartButtonText)
	{
		StartButtonText->SetText(Summary.bHasData
			? ResolveUiText(FName(TEXT("ui.title.continue")), FText::FromString(TEXT("\uACC4\uC18D\uD558\uAE30")))
			: ResolveUiText(FName(TEXT("ui.title.new_game")), FText::FromString(TEXT("\uC0C8\uAC8C\uC784 \uC2DC\uC791"))));
	}
}

void UTunaSweeperIntroMenuWidget::RefreshSaveSlotMenu()
{
	RefreshSaveSlotButton(1, SaveSlot1Button, SaveSlot1Text);
	RefreshSaveSlotButton(2, SaveSlot2Button, SaveSlot2Text);
	RefreshSaveSlotButton(3, SaveSlot3Button, SaveSlot3Text);

	if (SaveSlotActionRow)
	{
		SaveSlotActionRow->SetVisibility(SelectedSaveSlotIndex == INDEX_NONE
			? ESlateVisibility::Collapsed
			: ESlateVisibility::Visible);
	}

	if (SelectedSaveSlotIndex == INDEX_NONE)
	{
		return;
	}

	FTunaSweeperSaveSlotSummary Summary;
	if (const UTunaSweeperGameInstance* TunaGameInstance = Cast<UTunaSweeperGameInstance>(GetGameInstance()))
	{
		Summary = TunaGameInstance->GetSaveSlotSummary(SelectedSaveSlotIndex);
	}
	else
	{
		Summary.SaveSlotIndex = SelectedSaveSlotIndex;
	}

	if (PrimarySaveSlotButtonText)
	{
		PrimarySaveSlotButtonText->SetText(ResolveUiText(
			FName(TEXT("ui.title.primary_save_slot")),
			FText::FromString(TEXT("\uC138\uC774\uBE0C \uC2AC\uB86F \uC120\uD0DD"))));
	}

	if (DeleteSaveSlotButton)
	{
		DeleteSaveSlotButton->SetIsEnabled(Summary.bHasData);
	}

	if (DeleteSaveSlotButtonText)
	{
		DeleteSaveSlotButtonText->SetText(ResolveUiText(
			FName(TEXT("ui.title.delete_hold")),
			FText::FromString(TEXT("\uAE38\uAC8C \uB20C\uB7EC \uC0AD\uC81C\uD558\uAE30"))));
		DeleteSaveSlotButtonText->SetColorAndOpacity(FSlateColor(Summary.bHasData
			? FLinearColor::White
			: FLinearColor(0.55f, 0.60f, 0.62f, 1.0f)));
	}

	if (!Summary.bHasData)
	{
		ResetDeleteHoldProgress();
	}
}

void UTunaSweeperIntroMenuWidget::RefreshSettingsPanel()
{
	if (bShowingInterfaceSettingsTab)
	{
		RefreshInterfaceSettingsPanel();
		return;
	}

	FIntPoint CurrentResolution(0, 0);
	EWindowMode::Type CurrentWindowMode = EWindowMode::Windowed;
	if (GEngine)
	{
		if (UGameUserSettings* GameUserSettings = GEngine->GetGameUserSettings())
		{
			CurrentResolution = GameUserSettings->GetScreenResolution();
			CurrentWindowMode = GameUserSettings->GetFullscreenMode();
		}
	}

	if (SettingsStatusText)
	{
		const bool bDLSSSupported = UDLSSLibrary::IsDLSSSupported();
		const FText DLSSStatusText = bDLSSSupported
			? BuildDLSSModeText(PreferredDLSSMode)
			: ResolveUiText(
				FName(TEXT("ui.settings.dlss.unavailable")),
				FText::FromString(TEXT("\uC0AC\uC6A9 \uBD88\uAC00")));
		SettingsStatusText->SetText(FText::Format(
			ResolveUiText(
				FName(TEXT("ui.settings.current_graphics")),
				FText::FromString(TEXT("\uD604\uC7AC: {0} / {1}x{2} / DLSS {3}"))),
			BuildWindowModeText(CurrentWindowMode),
			FText::AsNumber(CurrentResolution.X),
			FText::AsNumber(CurrentResolution.Y),
			DLSSStatusText));
	}

	if (DLSSOffButton)
	{
		DLSSOffButton->SetIsEnabled(true);
	}
	if (DLSSQualityButton)
	{
		DLSSQualityButton->SetIsEnabled(IsDLSSModeAvailable(ETunaSweeperTitleDLSSMode::Quality));
	}
	if (DLSSBalancedButton)
	{
		DLSSBalancedButton->SetIsEnabled(IsDLSSModeAvailable(ETunaSweeperTitleDLSSMode::Balanced));
	}
	if (DLSSPerformanceButton)
	{
		DLSSPerformanceButton->SetIsEnabled(IsDLSSModeAvailable(ETunaSweeperTitleDLSSMode::Performance));
	}
}

void UTunaSweeperIntroMenuWidget::RefreshInterfaceSettingsPanel()
{
	const UTunaSweeperGameInstance* TunaGameInstance = Cast<UTunaSweeperGameInstance>(GetGameInstance());
	const ETunaSweeperItemTextLanguage CurrentLanguage = TunaGameInstance
		? TunaGameInstance->GetCurrentTextLanguage()
		: ETunaSweeperItemTextLanguage::English;

	if (SettingsStatusText)
	{
		SettingsStatusText->SetText(FText::Format(
			ResolveUiText(
				FName(TEXT("ui.settings.current_language")),
				FText::FromString(TEXT("\uD604\uC7AC \uC5B8\uC5B4: {0}"))),
			BuildLanguageNameText(CurrentLanguage)));
	}

	if (LanguageEnglishButtonText)
	{
		LanguageEnglishButtonText->SetText(BuildLanguageOptionText(
			ETunaSweeperItemTextLanguage::English,
			PendingInterfaceLanguage == ETunaSweeperItemTextLanguage::English));
	}
	if (LanguageKoreanButtonText)
	{
		LanguageKoreanButtonText->SetText(BuildLanguageOptionText(
			ETunaSweeperItemTextLanguage::Korean,
			PendingInterfaceLanguage == ETunaSweeperItemTextLanguage::Korean));
	}
	if (LanguageJapaneseButtonText)
	{
		LanguageJapaneseButtonText->SetText(BuildLanguageOptionText(
			ETunaSweeperItemTextLanguage::Japanese,
			PendingInterfaceLanguage == ETunaSweeperItemTextLanguage::Japanese));
	}

	if (LanguageEnglishButton)
	{
		LanguageEnglishButton->SetIsEnabled(true);
	}
	if (LanguageKoreanButton)
	{
		LanguageKoreanButton->SetIsEnabled(true);
	}
	if (LanguageJapaneseButton)
	{
		LanguageJapaneseButton->SetIsEnabled(true);
	}
	if (ConfirmInterfaceSettingsButton)
	{
		ConfirmInterfaceSettingsButton->SetIsEnabled(true);
	}
	if (CancelInterfaceSettingsButton)
	{
		CancelInterfaceSettingsButton->SetIsEnabled(true);
	}
}

void UTunaSweeperIntroMenuWidget::RefreshLocalizedTexts()
{
	SetNamedText(
		FName(TEXT("SaveSlotPanelTitleText")),
		ResolveUiText(FName(TEXT("ui.title.slot_select")), FText::FromString(TEXT("\uC2AC\uB86F \uC120\uD0DD"))));
	SetNamedText(
		FName(TEXT("BackToMainMenuButtonText")),
		ResolveUiText(FName(TEXT("ui.common.back")), FText::FromString(TEXT("\uB3CC\uC544\uAC00\uAE30"))));
	SetNamedText(
		FName(TEXT("DeleteConfirmTitleText")),
		ResolveUiText(FName(TEXT("ui.title.delete_confirm_title")), FText::FromString(TEXT("\uC2AC\uB86F \uC0AD\uC81C"))));
	SetNamedText(
		FName(TEXT("DeleteConfirmMessageText")),
		ResolveUiText(FName(TEXT("ui.title.delete_confirm_message")), FText::FromString(TEXT("\uC120\uD0DD\uD55C \uC800\uC7A5 \uB370\uC774\uD130\uB97C \uC0AD\uC81C\uD560\uAE4C\uC694?"))));
	SetNamedText(
		FName(TEXT("ConfirmDeleteButtonText")),
		ResolveUiText(FName(TEXT("ui.common.delete")), FText::FromString(TEXT("\uC0AD\uC81C\uD558\uAE30"))));
	SetNamedText(
		FName(TEXT("CancelDeleteButtonText")),
		ResolveUiText(FName(TEXT("ui.common.cancel")), FText::FromString(TEXT("\uCDE8\uC18C"))));
	SetNamedText(
		FName(TEXT("SettingsTitleText")),
		ResolveUiText(FName(TEXT("ui.title.settings")), FText::FromString(TEXT("\uC124\uC815"))));
	SetNamedText(
		FName(TEXT("SettingsGraphicsTabButtonText")),
		ResolveUiText(FName(TEXT("ui.settings.graphics")), FText::FromString(TEXT("\uADF8\uB798\uD53D"))));
	SetNamedText(
		FName(TEXT("SettingsInterfaceTabButtonText")),
		ResolveUiText(FName(TEXT("ui.settings.interface")), FText::FromString(TEXT("\uC778\uD130\uD398\uC774\uC2A4"))));
	SetNamedText(
		FName(TEXT("WindowModeLabelText")),
		ResolveUiText(FName(TEXT("ui.settings.window_mode")), FText::FromString(TEXT("\uD654\uBA74 \uBAA8\uB4DC"))));
	SetNamedText(
		FName(TEXT("WindowedModeButtonText")),
		BuildWindowModeText(EWindowMode::Windowed));
	SetNamedText(
		FName(TEXT("BorderlessWindowModeButtonText")),
		BuildWindowModeText(EWindowMode::WindowedFullscreen));
	SetNamedText(
		FName(TEXT("FullscreenModeButtonText")),
		BuildWindowModeText(EWindowMode::Fullscreen));
	SetNamedText(
		FName(TEXT("ResolutionLabelText")),
		ResolveUiText(FName(TEXT("ui.settings.resolution")), FText::FromString(TEXT("\uD574\uC0C1\uB3C4"))));
	SetNamedText(
		FName(TEXT("DLSSLabelText")),
		ResolveUiText(FName(TEXT("ui.settings.dlss")), FText::FromString(TEXT("DLSS"))));
	SetNamedText(
		FName(TEXT("DLSSOffButtonText")),
		BuildDLSSModeText(ETunaSweeperTitleDLSSMode::Off));
	SetNamedText(
		FName(TEXT("DLSSQualityButtonText")),
		BuildDLSSModeText(ETunaSweeperTitleDLSSMode::Quality));
	SetNamedText(
		FName(TEXT("DLSSBalancedButtonText")),
		BuildDLSSModeText(ETunaSweeperTitleDLSSMode::Balanced));
	SetNamedText(
		FName(TEXT("DLSSPerformanceButtonText")),
		BuildDLSSModeText(ETunaSweeperTitleDLSSMode::Performance));
	SetNamedText(
		FName(TEXT("LanguageLabelText")),
		ResolveUiText(FName(TEXT("ui.settings.language")), FText::FromString(TEXT("\uC5B8\uC5B4"))));
	SetNamedText(
		FName(TEXT("ConfirmInterfaceSettingsButtonText")),
		ResolveUiText(FName(TEXT("ui.common.confirm")), FText::FromString(TEXT("\uACB0\uC815"))));
	SetNamedText(
		FName(TEXT("CancelInterfaceSettingsButtonText")),
		ResolveUiText(FName(TEXT("ui.common.cancel")), FText::FromString(TEXT("\uCDE8\uC18C"))));
	SetNamedText(
		FName(TEXT("CreditsTitleText")),
		ResolveUiText(FName(TEXT("ui.title.credits")), FText::FromString(TEXT("\uD06C\uB808\uB527"))));
	SetNamedText(
		FName(TEXT("BackFromCreditsButtonText")),
		ResolveUiText(FName(TEXT("ui.common.back")), FText::FromString(TEXT("\uB3CC\uC544\uAC00\uAE30"))));

	if (AlwaysNewStartButtonText)
	{
		AlwaysNewStartButtonText->SetText(ResolveUiText(
			FName(TEXT("ui.title.always_new_start")),
			FText::FromString(TEXT("\uD56D\uC0C1\uC0C8\uB85C\uC2DC\uC791"))));
	}
}

void UTunaSweeperIntroMenuWidget::RefreshSaveSlotButton(int32 SaveSlotIndex, UButton* SlotButton, UTextBlock* SlotText)
{
	if (SlotText)
	{
		SlotText->SetText(BuildSaveSlotButtonText(SaveSlotIndex));
		SlotText->SetColorAndOpacity(FSlateColor(SaveSlotIndex == SelectedSaveSlotIndex
			? FLinearColor::White
			: FLinearColor(0.74f, 0.80f, 0.84f, 1.0f)));
	}

	if (SlotButton)
	{
		SlotButton->SetIsEnabled(true);
	}
}

void UTunaSweeperIntroMenuWidget::LoadTitleGraphicsSettings()
{
	int32 DLSSModeValue = TunaSweeperTitleGraphicsSettings::ToConfigValue(PreferredDLSSMode);
	if (GConfig)
	{
		GConfig->GetInt(
			TunaSweeperTitleGraphicsSettings::SectionName,
			TunaSweeperTitleGraphicsSettings::DLSSModeKey,
			DLSSModeValue,
			GGameUserSettingsIni);
	}

	PreferredDLSSMode = TunaSweeperTitleGraphicsSettings::ToTitleDLSSMode(DLSSModeValue);
}

void UTunaSweeperIntroMenuWidget::SaveTitleGraphicsSettings() const
{
	if (!GConfig)
	{
		return;
	}

	GConfig->SetInt(
		TunaSweeperTitleGraphicsSettings::SectionName,
		TunaSweeperTitleGraphicsSettings::DLSSModeKey,
		TunaSweeperTitleGraphicsSettings::ToConfigValue(PreferredDLSSMode),
		GGameUserSettingsIni);
	GConfig->Flush(false, GGameUserSettingsIni);
}

void UTunaSweeperIntroMenuWidget::ApplyDLSSSetting(ETunaSweeperTitleDLSSMode DLSSMode)
{
	PreferredDLSSMode = DLSSMode;
	ApplyDLSSModeToRuntime(PreferredDLSSMode);
	SaveTitleGraphicsSettings();
	RefreshSettingsPanel();
}

void UTunaSweeperIntroMenuWidget::ApplyDLSSModeToRuntime(ETunaSweeperTitleDLSSMode DLSSMode) const
{
	if (DLSSMode == ETunaSweeperTitleDLSSMode::Off || !IsDLSSModeAvailable(DLSSMode))
	{
		UDLSSLibrary::SetDLSSMode(GetWorld(), UDLSSMode::Off);
		if (IConsoleVariable* ScreenPercentageCVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.ScreenPercentage")))
		{
			ScreenPercentageCVar->Set(100.0f, ECVF_SetByGameSetting);
		}
		return;
	}

	const UDLSSMode RuntimeDLSSMode = TunaSweeperTitleGraphicsSettings::ToDLSSMode(DLSSMode);
	UDLSSLibrary::SetDLSSMode(GetWorld(), RuntimeDLSSMode);
}

bool UTunaSweeperIntroMenuWidget::IsDLSSModeAvailable(ETunaSweeperTitleDLSSMode DLSSMode) const
{
	if (DLSSMode == ETunaSweeperTitleDLSSMode::Off)
	{
		return true;
	}

	return UDLSSLibrary::IsDLSSSupported() &&
		UDLSSLibrary::IsDLSSModeSupported(TunaSweeperTitleGraphicsSettings::ToDLSSMode(DLSSMode));
}

FText UTunaSweeperIntroMenuWidget::BuildWindowModeText(EWindowMode::Type WindowMode) const
{
	switch (WindowMode)
	{
	case EWindowMode::Fullscreen:
		return ResolveUiText(FName(TEXT("ui.settings.fullscreen")), FText::FromString(TEXT("\uC804\uCCB4\uD654\uBA74\uBAA8\uB4DC")));
	case EWindowMode::WindowedFullscreen:
		return ResolveUiText(FName(TEXT("ui.settings.borderless")), FText::FromString(TEXT("\uD14C\uB450\uB9AC \uC5C6\uB294 \uCC3D\uBAA8\uB4DC")));
	case EWindowMode::Windowed:
	default:
		return ResolveUiText(FName(TEXT("ui.settings.windowed")), FText::FromString(TEXT("\uCC3D\uBAA8\uB4DC")));
	}
}

FText UTunaSweeperIntroMenuWidget::BuildDLSSModeText(ETunaSweeperTitleDLSSMode DLSSMode) const
{
	switch (DLSSMode)
	{
	case ETunaSweeperTitleDLSSMode::Quality:
		return ResolveUiText(FName(TEXT("ui.settings.dlss.quality")), FText::FromString(TEXT("\uD488\uC9C8")));
	case ETunaSweeperTitleDLSSMode::Balanced:
		return ResolveUiText(FName(TEXT("ui.settings.dlss.balanced")), FText::FromString(TEXT("\uADE0\uD615")));
	case ETunaSweeperTitleDLSSMode::Performance:
		return ResolveUiText(FName(TEXT("ui.settings.dlss.performance")), FText::FromString(TEXT("\uC131\uB2A5")));
	case ETunaSweeperTitleDLSSMode::Off:
	default:
		return ResolveUiText(FName(TEXT("ui.settings.dlss.off")), FText::FromString(TEXT("\uB044\uAE30")));
	}
}

FText UTunaSweeperIntroMenuWidget::BuildLanguageNameText(ETunaSweeperItemTextLanguage Language) const
{
	switch (Language)
	{
	case ETunaSweeperItemTextLanguage::Korean:
		return FText::FromString(TEXT("\uD55C\uAD6D\uC5B4"));
	case ETunaSweeperItemTextLanguage::Japanese:
		return FText::FromString(TEXT("\u65E5\u672C\u8A9E"));
	case ETunaSweeperItemTextLanguage::English:
	default:
		return FText::FromString(TEXT("English"));
	}
}

FText UTunaSweeperIntroMenuWidget::BuildLanguageOptionText(
	ETunaSweeperItemTextLanguage Language,
	bool bSelected) const
{
	return FText::FromString(FString::Printf(
		TEXT("%s %s"),
		bSelected ? TEXT("[x]") : TEXT("[ ]"),
		*BuildLanguageNameText(Language).ToString()));
}

FText UTunaSweeperIntroMenuWidget::ResolveUiText(FName StringKey, const FText& FallbackText) const
{
	const UTunaSweeperGameInstance* TunaGameInstance = Cast<UTunaSweeperGameInstance>(GetGameInstance());
	return TunaGameInstance
		? TunaGameInstance->ResolveLocalizedText(StringKey, FallbackText)
		: FallbackText;
}

void UTunaSweeperIntroMenuWidget::SetNamedText(FName WidgetName, const FText& Text) const
{
	if (!WidgetTree || WidgetName.IsNone())
	{
		return;
	}

	if (UTextBlock* TextBlock = Cast<UTextBlock>(WidgetTree->FindWidget(WidgetName)))
	{
		TextBlock->SetText(Text);
	}
}

void UTunaSweeperIntroMenuWidget::EnsureAlwaysNewStartButton()
{
	if (AlwaysNewStartButton)
	{
		if (!AlwaysNewStartButtonContainer)
		{
			AlwaysNewStartButtonContainer = AlwaysNewStartButton;
		}
		if (AlwaysNewStartButtonText)
		{
			AlwaysNewStartButtonText->SetText(ResolveUiText(
				FName(TEXT("ui.title.always_new_start")),
				FText::FromString(TEXT("\uD56D\uC0C1\uC0C8\uB85C\uC2DC\uC791"))));
		}
		return;
	}

	if (!WidgetTree)
	{
		return;
	}

	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
	if (!RootCanvas)
	{
		return;
	}

	USizeBox* AlwaysNewStartButtonBox = WidgetTree->ConstructWidget<USizeBox>(
		USizeBox::StaticClass(),
		TEXT("AlwaysNewStartButtonBox"));
	AlwaysNewStartButton = WidgetTree->ConstructWidget<UButton>(
		UButton::StaticClass(),
		TEXT("AlwaysNewStartButton"));
	AlwaysNewStartButtonText = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(),
		TEXT("AlwaysNewStartButtonText"));

	if (!AlwaysNewStartButtonBox || !AlwaysNewStartButton || !AlwaysNewStartButtonText)
	{
		return;
	}

	auto MakeBoxBrush = [](const FLinearColor& Color)
	{
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::Box;
		Brush.TintColor = FSlateColor(Color);
		return Brush;
	};

	FButtonStyle DebugButtonStyle;
	DebugButtonStyle.SetNormal(MakeBoxBrush(FLinearColor(0.34f, 0.03f, 0.02f, 0.76f)));
	DebugButtonStyle.SetHovered(MakeBoxBrush(FLinearColor(0.58f, 0.06f, 0.04f, 0.90f)));
	DebugButtonStyle.SetPressed(MakeBoxBrush(FLinearColor(0.22f, 0.02f, 0.015f, 0.95f)));
	DebugButtonStyle.SetNormalPadding(FMargin(0.0f));
	DebugButtonStyle.SetPressedPadding(FMargin(0.0f, 1.0f, 0.0f, 0.0f));
	AlwaysNewStartButton->SetStyle(DebugButtonStyle);
	AlwaysNewStartButton->SetClickMethod(EButtonClickMethod::DownAndUp);

	AlwaysNewStartButtonText->SetText(ResolveUiText(
		FName(TEXT("ui.title.always_new_start")),
		FText::FromString(TEXT("\uD56D\uC0C1\uC0C8\uB85C\uC2DC\uC791"))));
	AlwaysNewStartButtonText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.92f, 0.88f, 1.0f)));
	AlwaysNewStartButtonText->SetJustification(ETextJustify::Center);
	TunaSweeperUIFont::ApplyFont(AlwaysNewStartButtonText, 16, ETunaSweeperUIFontWeight::Bold);

	AlwaysNewStartButtonBox->SetWidthOverride(180.0f);
	AlwaysNewStartButtonBox->SetHeightOverride(42.0f);
	AlwaysNewStartButton->SetContent(AlwaysNewStartButtonText);
	AlwaysNewStartButtonBox->SetContent(AlwaysNewStartButton);
	AlwaysNewStartButtonContainer = AlwaysNewStartButtonBox;

	UCanvasPanelSlot* DebugButtonSlot = RootCanvas->AddChildToCanvas(AlwaysNewStartButtonBox);
	if (DebugButtonSlot)
	{
		DebugButtonSlot->SetAnchors(FAnchors(1.0f, 0.0f));
		DebugButtonSlot->SetAlignment(FVector2D(1.0f, 0.0f));
		DebugButtonSlot->SetPosition(FVector2D(-36.0f, 36.0f));
		DebugButtonSlot->SetSize(FVector2D(180.0f, 42.0f));
		DebugButtonSlot->SetZOrder(200);
	}
}

void UTunaSweeperIntroMenuWidget::SetAlwaysNewStartButtonVisible(bool bVisible)
{
	EnsureAlwaysNewStartButton();
	if (AlwaysNewStartButtonContainer)
	{
		AlwaysNewStartButtonContainer->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	if (AlwaysNewStartButton)
	{
		AlwaysNewStartButton->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

FText UTunaSweeperIntroMenuWidget::BuildCurrentSaveSlotText(int32 SaveSlotIndex) const
{
	FTunaSweeperSaveSlotSummary Summary;
	if (const UTunaSweeperGameInstance* TunaGameInstance = Cast<UTunaSweeperGameInstance>(GetGameInstance()))
	{
		Summary = TunaGameInstance->GetSaveSlotSummary(SaveSlotIndex);
	}
	else
	{
		Summary.SaveSlotIndex = SaveSlotIndex;
	}

	if (!Summary.bHasData)
	{
		return FText::Format(
			FText::FromString(TEXT("{0} - {1}")),
			FText::Format(
				ResolveUiText(FName(TEXT("ui.title.slot_label")), FText::FromString(TEXT("\uC2AC\uB86F {0}"))),
				FText::AsNumber(SaveSlotIndex)),
			ResolveUiText(FName(TEXT("ui.title.empty_slot")), FText::FromString(TEXT("\uBE48 \uC2AC\uB86F"))));
	}

	return FText::Format(
		FText::FromString(TEXT("{0} - {1}")),
		FText::Format(
			ResolveUiText(FName(TEXT("ui.title.slot_label")), FText::FromString(TEXT("\uC2AC\uB86F {0}"))),
			FText::AsNumber(SaveSlotIndex)),
		FText::Format(
			ResolveUiText(FName(TEXT("ui.title.play_time")), FText::FromString(TEXT("\uD50C\uB808\uC774 \uC2DC\uAC04 {0}"))),
			FText::FromString(FormatPlayTime(Summary.TotalPlaySeconds))));
}

FText UTunaSweeperIntroMenuWidget::BuildSaveSlotButtonText(int32 SaveSlotIndex) const
{
	FTunaSweeperSaveSlotSummary Summary;
	if (const UTunaSweeperGameInstance* TunaGameInstance = Cast<UTunaSweeperGameInstance>(GetGameInstance()))
	{
		Summary = TunaGameInstance->GetSaveSlotSummary(SaveSlotIndex);
	}
	else
	{
		Summary.SaveSlotIndex = SaveSlotIndex;
	}

	if (!Summary.bHasData)
	{
		const TArray<FString> Lines = {
			FText::Format(
				ResolveUiText(FName(TEXT("ui.title.slot_label")), FText::FromString(TEXT("\uC2AC\uB86F {0}"))),
				FText::AsNumber(SaveSlotIndex)).ToString(),
			ResolveUiText(FName(TEXT("ui.title.empty_slot")), FText::FromString(TEXT("\uBE48 \uC2AC\uB86F"))).ToString(),
			ResolveUiText(FName(TEXT("ui.title.start_new_game")), FText::FromString(TEXT("\uC0C8 \uAC8C\uC784 \uC2DC\uC791"))).ToString()
		};
		return FText::FromString(FString::Join(Lines, LINE_TERMINATOR));
	}

	const TArray<FString> Lines = {
		FText::Format(
			ResolveUiText(FName(TEXT("ui.title.slot_label")), FText::FromString(TEXT("\uC2AC\uB86F {0}"))),
			FText::AsNumber(SaveSlotIndex)).ToString(),
		FText::Format(
			ResolveUiText(FName(TEXT("ui.title.play_time")), FText::FromString(TEXT("\uD50C\uB808\uC774 \uC2DC\uAC04 {0}"))),
			FText::FromString(FormatPlayTime(Summary.TotalPlaySeconds))).ToString(),
		ResolveUiText(FName(TEXT("ui.title.has_progress_data")), FText::FromString(TEXT("\uC9C4\uD589 \uB370\uC774\uD130 \uC788\uC74C"))).ToString()
	};
	return FText::FromString(FString::Join(Lines, LINE_TERMINATOR));
}

FString UTunaSweeperIntroMenuWidget::BuildCreditsRollText() const
{
	const FString CreditsFilePath = FPaths::Combine(
		FPaths::ProjectContentDir(),
		TEXT("UI"),
		TEXT("Credits"),
		TEXT("StaffRoll.txt"));

	FString CreditsTextFromFile;
	if (FFileHelper::LoadFileToString(CreditsTextFromFile, *CreditsFilePath) &&
		!CreditsTextFromFile.TrimStartAndEnd().IsEmpty())
	{
		return CreditsTextFromFile;
	}

	return FString(
		TEXT("Tuna Sweeper\n\n")
		TEXT("A Game by BlenG\n\n\n")
		TEXT("Direction\nBlenG\n\n")
		TEXT("Game Design\nBlenG\n\n")
		TEXT("Programming\nBlenG\n\n")
		TEXT("Art Direction\nBlenG\n\n")
		TEXT("UI Design\nBlenG\n\n")
		TEXT("Scenario\nBlenG\n\n")
		TEXT("Level Design\nBlenG\n\n")
		TEXT("Audio Direction\nBlenG\n\n")
		TEXT("QA\nBlenG\n\n\n")
		TEXT("Thank you for playing.\n"));
}

FString UTunaSweeperIntroMenuWidget::BuildCreditsColumnText(int32 ColumnIndex) const
{
	TArray<FString> Lines;
	BuildCreditsRollText().ParseIntoArrayLines(Lines, false);

	if (Lines.IsEmpty())
	{
		return FString();
	}

	const int32 ClampedColumnIndex = FMath::Clamp(ColumnIndex, 0, 2);
	const int32 LinesPerColumn = FMath::Max(1, FMath::DivideAndRoundUp(Lines.Num(), 3));
	const int32 StartIndex = ClampedColumnIndex * LinesPerColumn;
	const int32 EndIndex = FMath::Min(StartIndex + LinesPerColumn, Lines.Num());

	FString ColumnText;
	for (int32 LineIndex = StartIndex; LineIndex < EndIndex; ++LineIndex)
	{
		if (!ColumnText.IsEmpty())
		{
			ColumnText += LINE_TERMINATOR;
		}
		ColumnText += Lines[LineIndex];
	}

	return ColumnText;
}

FString UTunaSweeperIntroMenuWidget::FormatPlayTime(float TotalSeconds) const
{
	const int32 ClampedTotalSeconds = FMath::Max(0, FMath::RoundToInt(TotalSeconds));
	const int32 Hours = ClampedTotalSeconds / 3600;
	const int32 Minutes = (ClampedTotalSeconds / 60) % 60;
	const int32 Seconds = ClampedTotalSeconds % 60;
	return FString::Printf(TEXT("%02d:%02d:%02d"), Hours, Minutes, Seconds);
}

FString UTunaSweeperIntroMenuWidget::FormatSaveTime(int64 LastSavedAtTicks) const
{
	if (LastSavedAtTicks <= 0)
	{
		return FString(TEXT("--"));
	}

	return FDateTime(LastSavedAtTicks).ToString(TEXT("%Y-%m-%d %H:%M"));
}

bool UTunaSweeperIntroMenuWidget::IsSaveSlotSelectionVisible() const
{
	return SaveSlotPanel && SaveSlotPanel->GetVisibility() == ESlateVisibility::Visible;
}

bool UTunaSweeperIntroMenuWidget::IsCreditsPanelVisible() const
{
	return CreditsPanel && CreditsPanel->GetVisibility() == ESlateVisibility::Visible;
}

bool UTunaSweeperIntroMenuWidget::CanDeleteSelectedSaveSlot() const
{
	if (SelectedSaveSlotIndex == INDEX_NONE)
	{
		return false;
	}

	if (const UTunaSweeperGameInstance* TunaGameInstance = Cast<UTunaSweeperGameInstance>(GetGameInstance()))
	{
		return TunaGameInstance->GetSaveSlotSummary(SelectedSaveSlotIndex).bHasData;
	}

	return false;
}

void UTunaSweeperIntroMenuWidget::ApplyDisplaySettings(EWindowMode::Type WindowMode)
{
	if (!GEngine)
	{
		return;
	}

	if (UGameUserSettings* GameUserSettings = GEngine->GetGameUserSettings())
	{
		GameUserSettings->SetFullscreenMode(WindowMode);
		GameUserSettings->ApplySettings(false);
		GameUserSettings->SaveSettings();
	}

	RefreshSettingsPanel();
}

void UTunaSweeperIntroMenuWidget::ApplyResolutionSetting(const FIntPoint& Resolution)
{
	if (!GEngine)
	{
		return;
	}

	if (UGameUserSettings* GameUserSettings = GEngine->GetGameUserSettings())
	{
		GameUserSettings->SetScreenResolution(Resolution);
		GameUserSettings->ApplySettings(false);
		GameUserSettings->SaveSettings();
	}

	RefreshSettingsPanel();
}

void UTunaSweeperIntroMenuWidget::ResetDeleteHoldProgress()
{
	bDeleteHoldActive = false;
	DeleteHoldElapsedSeconds = 0.0f;
	SetDeleteHoldProgress(0.0f);
}

void UTunaSweeperIntroMenuWidget::SetDeleteHoldProgress(float Progress)
{
	if (!DeleteHoldGaugeFill)
	{
		return;
	}

	const float ClampedProgress = FMath::Clamp(Progress, 0.0f, 1.0f);
	DeleteHoldGaugeFill->SetRenderOpacity(ClampedProgress > 0.0f ? 1.0f : 0.0f);
	DeleteHoldGaugeFill->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	DeleteHoldGaugeFill->SetRenderScale(FVector2D(ClampedProgress, ClampedProgress));
}

void UTunaSweeperIntroMenuWidget::ShowDeleteConfirmDialog()
{
	bDeleteConfirmVisible = true;
	SetDeleteHoldProgress(1.0f);

	if (DeleteConfirmPanel)
	{
		DeleteConfirmPanel->SetVisibility(ESlateVisibility::Visible);
	}
}

void UTunaSweeperIntroMenuWidget::HideDeleteConfirmDialog()
{
	bDeleteConfirmVisible = false;

	if (DeleteConfirmPanel)
	{
		DeleteConfirmPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UTunaSweeperIntroMenuWidget::ResetTitleViewportLayoutState()
{
	auto ResetWidgetTransform = [](UWidget* Widget)
	{
		if (!Widget)
		{
			return;
		}

		Widget->SetRenderTransform(FWidgetTransform());
		Widget->SetRenderTransformPivot(FVector2D::ZeroVector);
		Widget->SetRenderScale(FVector2D::UnitVector);
	};

	ResetWidgetTransform(this);
	ResetWidgetTransform(GetRootWidget());
	ResetWidgetTransform(MainMenuPanel.Get());
	ResetWidgetTransform(SaveSlotPanel.Get());
	ResetWidgetTransform(SettingsPanel.Get());
	ResetWidgetTransform(CreditsPanel.Get());

	InvalidateLayoutAndVolatility();
}

void UTunaSweeperIntroMenuWidget::ApplyTitleMenuButtonContentLayout()
{
	if (bTitleMenuButtonContentLayoutApplied || !WidgetTree)
	{
		return;
	}

	auto ApplyContent = [this](
		UButton* Button,
		const FText& Icon,
		UTextBlock* ExistingLabelText,
		const FText& Label,
		bool bPrimary)
	{
		if (!Button)
		{
			return;
		}

		UTextBlock* LabelText = ExistingLabelText;
		if (!LabelText)
		{
			LabelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		}
		if (!LabelText)
		{
			return;
		}

		Button->SetContent(BuildTitleMenuButtonContent(
			Icon,
			LabelText,
			Label,
			bPrimary ? 28 : 20,
			bPrimary ? 28 : 20));
	};

	ApplyContent(
		StartButton,
		FText::FromString(TEXT("\u25B6")),
		StartButtonText,
		ResolveUiText(FName(TEXT("ui.title.continue")), FText::FromString(TEXT("\uACC4\uC18D\uD558\uAE30"))),
		true);
	ApplyContent(
		SlotSelectButton,
		FText::FromString(TEXT("\u25A6")),
		nullptr,
		ResolveUiText(FName(TEXT("ui.title.slot_select")), FText::FromString(TEXT("\uC2AC\uB86F \uC120\uD0DD"))),
		false);
	ApplyContent(
		SettingsButton,
		FText::FromString(TEXT("\u2699")),
		nullptr,
		ResolveUiText(FName(TEXT("ui.title.settings")), FText::FromString(TEXT("\uC124\uC815"))),
		false);
	ApplyContent(
		CreditsButton,
		FText::FromString(TEXT("\u24D8")),
		nullptr,
		ResolveUiText(FName(TEXT("ui.title.credits")), FText::FromString(TEXT("\uD06C\uB808\uB527"))),
		false);
	ApplyContent(
		QuitButton,
		FText::FromString(TEXT("\u00D7")),
		nullptr,
		ResolveUiText(FName(TEXT("ui.title.quit")), FText::FromString(TEXT("\uC885\uB8CC"))),
		false);

	bTitleMenuButtonContentLayoutApplied = true;
}

UWidget* UTunaSweeperIntroMenuWidget::BuildTitleMenuButtonContent(
	const FText& Icon,
	UTextBlock* LabelText,
	const FText& Label,
	int32 LabelFontSize,
	int32 IconFontSize)
{
	if (!WidgetTree || !LabelText)
	{
		return LabelText;
	}

	UHorizontalBox* Content = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	USizeBox* IconBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	UTextBlock* IconText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	USizeBox* BalanceBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	if (!Content || !IconBox || !IconText || !BalanceBox)
	{
		return LabelText;
	}

	const FLinearColor MenuTextColor(0.94f, 0.92f, 0.84f, 1.0f);
	IconText->SetText(Icon);
	TunaSweeperUIFont::ApplyFont(IconText, IconFontSize);
	IconText->SetColorAndOpacity(FSlateColor(MenuTextColor));
	IconText->SetJustification(ETextJustify::Center);

	LabelText->RemoveFromParent();
	LabelText->SetText(Label);
	TunaSweeperUIFont::ApplyFont(LabelText, LabelFontSize);
	LabelText->SetColorAndOpacity(FSlateColor(MenuTextColor));
	LabelText->SetJustification(ETextJustify::Center);

	const float IconLaneWidth = IconFontSize >= 28 ? 58.0f : 46.0f;
	IconBox->SetWidthOverride(IconLaneWidth);
	IconBox->SetHeightOverride(IconFontSize + 8.0f);
	IconBox->SetContent(IconText);
	BalanceBox->SetWidthOverride(IconLaneWidth);
	BalanceBox->SetHeightOverride(IconFontSize + 8.0f);

	if (UHorizontalBoxSlot* IconSlot = Content->AddChildToHorizontalBox(IconBox))
	{
		IconSlot->SetHorizontalAlignment(HAlign_Center);
		IconSlot->SetVerticalAlignment(VAlign_Center);
		IconSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
	}

	if (UHorizontalBoxSlot* LabelSlot = Content->AddChildToHorizontalBox(LabelText))
	{
		LabelSlot->SetHorizontalAlignment(HAlign_Center);
		LabelSlot->SetVerticalAlignment(VAlign_Center);
		LabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
	}

	if (UHorizontalBoxSlot* BalanceSlot = Content->AddChildToHorizontalBox(BalanceBox))
	{
		BalanceSlot->SetHorizontalAlignment(HAlign_Center);
		BalanceSlot->SetVerticalAlignment(VAlign_Center);
		BalanceSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
	}

	return Content;
}

void UTunaSweeperIntroMenuWidget::EnsureTitleWindParticleOverlay()
{
	if (TitleWindParticleOverlay || !WidgetTree)
	{
		return;
	}

	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
	if (!RootCanvas)
	{
		return;
	}

	auto SetCanvasZOrder = [](UWidget* Widget, int32 ZOrder)
	{
		if (UCanvasPanelSlot* CanvasSlot = Widget ? Cast<UCanvasPanelSlot>(Widget->Slot) : nullptr)
		{
			CanvasSlot->SetZOrder(ZOrder);
		}
	};

	SetCanvasZOrder(WidgetTree->FindWidget(TEXT("BackgroundImage")), 0);
	SetCanvasZOrder(WidgetTree->FindWidget(TEXT("LeftScrim")), 2);
	SetCanvasZOrder(WidgetTree->FindWidget(TEXT("LogoImage")), 3);
	SetCanvasZOrder(MainMenuPanel, 4);
	SetCanvasZOrder(WidgetTree->FindWidget(TEXT("VersionText")), 4);
	SetCanvasZOrder(SaveSlotPanel, 10);
	SetCanvasZOrder(SettingsPanel, 10);
	SetCanvasZOrder(CreditsPanel, 10);

	TitleWindParticleOverlay = WidgetTree->ConstructWidget<UTunaSweeperTitleWindParticleWidget>(
		UTunaSweeperTitleWindParticleWidget::StaticClass(),
		TEXT("TitleWindParticleOverlay"));
	if (!TitleWindParticleOverlay)
	{
		return;
	}

	TitleWindParticleOverlay->SetVisibility(ESlateVisibility::HitTestInvisible);

	UCanvasPanelSlot* ParticleSlot = RootCanvas->AddChildToCanvas(TitleWindParticleOverlay);
	if (!ParticleSlot)
	{
		TitleWindParticleOverlay = nullptr;
		return;
	}

	ParticleSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
	ParticleSlot->SetOffsets(FMargin(0.0f));
	ParticleSlot->SetAlignment(FVector2D::ZeroVector);
	ParticleSlot->SetZOrder(1);
}

void UTunaSweeperIntroMenuWidget::OpenPendingStartTargetLevel()
{
	if (!bStartTravelPending || PendingStartTargetLevelName.IsNone())
	{
		return;
	}

	const FName TargetLevelName = PendingStartTargetLevelName;
	bStartTravelPending = false;
	PendingStartTargetLevelName = NAME_None;
	UGameplayStatics::OpenLevel(this, TargetLevelName);
}

void UTunaSweeperIntroMenuWidget::SetStartTravelControlsEnabled(bool bEnabled)
{
	const TArray<UButton*> ButtonsToUpdate = {
		StartButton.Get(),
		SlotSelectButton.Get(),
		SettingsButton.Get(),
		CreditsButton.Get(),
		QuitButton.Get(),
		AlwaysNewStartButton.Get(),
		PrimarySaveSlotButton.Get(),
		DeleteSaveSlotButton.Get(),
		BackToMainMenuButton.Get(),
		ConfirmDeleteButton.Get(),
		CancelDeleteButton.Get(),
		SettingsGraphicsTabButton.Get(),
		SettingsInterfaceTabButton.Get(),
		WindowedModeButton.Get(),
		BorderlessWindowModeButton.Get(),
		FullscreenModeButton.Get(),
		Resolution1280Button.Get(),
		Resolution1600Button.Get(),
		Resolution1920Button.Get(),
		Resolution2560Button.Get(),
		Resolution3840Button.Get(),
		DLSSOffButton.Get(),
		DLSSQualityButton.Get(),
		DLSSBalancedButton.Get(),
		DLSSPerformanceButton.Get(),
		BackFromSettingsButton.Get(),
		LanguageEnglishButton.Get(),
		LanguageKoreanButton.Get(),
		LanguageJapaneseButton.Get(),
		ConfirmInterfaceSettingsButton.Get(),
		CancelInterfaceSettingsButton.Get(),
		BackFromCreditsButton.Get()
	};

	for (UButton* Button : ButtonsToUpdate)
	{
		if (Button)
		{
			Button->SetIsEnabled(bEnabled);
		}
	}
}
