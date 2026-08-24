#include "TunaSweeperIntroMenuWidgetShared.h"
#include "Settings/TunaSweeperBuildFlavor.h"

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
		if (TunaSweeperBuildFlavor::IsDemo())
		{
			const FTunaSweeperSaveSlotSummary DemoSummary = TunaGameInstance->GetSaveSlotSummary(1);
			if (!DemoSummary.bHasData || !DemoSummary.bDifficultySelected || bAlwaysNewStart)
			{
				ShowDifficultySelection();
				return;
			}
			if (!TunaGameInstance->ActivateSaveSlot(1, false))
			{
				if (UTunaSweeperToastSubsystem* ToastSubsystem = TunaGameInstance->GetSubsystem<UTunaSweeperToastSubsystem>())
				{
					ToastSubsystem->ShowToast(ResolveUiText(
						FName(TEXT("ui.toast.load_failed")),
						FText::FromString(TEXT("저장 데이터를 불러오지 못했습니다."))));
				}
				return;
			}
			BeginTravelToLevel(TunaSweeperBuildFlavor::GetBunkerLevelName());
			return;
		}

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
			if (!TunaGameInstance->ActivateSaveSlot(ActiveSaveSlotIndex, !Summary.bHasData))
			{
				return;
			}
		}
		if (!TunaGameInstance->IsActiveSaveSlotDifficultySelected())
		{
			ShowDifficultySelection();
			return;
		}
		TargetLevelName = TunaGameInstance->ResolveInitialGameplayLevelName();
	}

	BeginTravelToLevel(TargetLevelName);
}

void UTunaSweeperIntroMenuWidget::BeginTravelToLevel(FName TargetLevelName)
{
	if (bStartTravelPending || TargetLevelName.IsNone())
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
		DifficultyFarmingButton.Get(),
		DifficultyNormalButton.Get(),
		DifficultyHardButton.Get(),
		DifficultyStartButton.Get(),
		DifficultyBackButton.Get(),
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
