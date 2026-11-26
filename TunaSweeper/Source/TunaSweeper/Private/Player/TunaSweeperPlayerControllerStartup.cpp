#include "Player/TunaSweeperPlayerController.h"
#include "Game/TunaSweeperGameInstance.h"
#include "UI/Sol62/StartupLanguageGPT62Sol.h"
#include "UI/TunaSweeperScreenFadeWidget.h"
#include "UI/TunaSweeperStartupLogoWidget.h"

bool ATunaSweeperPlayerController::TryStartDeveloperLogo()
{
	bDeveloperLogoSkippedByDailyLimit = false;
	if (!IsLocalController()) return false;
	UTunaSweeperGameInstance* Instance = Cast<UTunaSweeperGameInstance>(GetGameInstance());
	if (!Instance || !Instance->TryBeginDeveloperLogo(&bDeveloperLogoSkippedByDailyLimit)) return false;
	StartupLogoWidget = CreateWidget<UTunaSweeperStartupLogoWidget>(this);
	if (!StartupLogoWidget) return false;
	StartupLogoWidget->OnFinished.BindUObject(this, &ThisClass::HandleDeveloperLogoFinished);
	StartupLogoWidget->AddToViewport(2000);
	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(StartupLogoWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
	StartupLogoWidget->SetKeyboardFocus();
	bShowMouseCursor = false;
	StartupLogoWidget->Start();
	return true;
}

void ATunaSweeperPlayerController::HandleDeveloperLogoFinished(bool bVideoCompleted)
{
	if (bVideoCompleted)
	{
		if (UTunaSweeperGameInstance* Instance = Cast<UTunaSweeperGameInstance>(GetGameInstance()))
		{
			Instance->RecordDeveloperLogoCompleted();
		}
	}
	// Keep the opaque logo alive until the next startup screen and its fade exist.
	UTunaSweeperStartupLogoWidget* FinishedWidget = StartupLogoWidget;
	StartupLogoWidget = nullptr;
	EnsureIntroMenuWidget();
	if (FinishedWidget) FinishedWidget->RemoveFromParent();
	UE_LOG(LogTemp, Display, TEXT("StartupLogo: startup continued (video completed=%d)"), bVideoCompleted);
}

bool ATunaSweeperPlayerController::TryShowStartupLanguage()
{
	const auto* Game = Cast<UTunaSweeperGameInstance>(GetGameInstance());
	if (!Game || Game->HasConfirmedTextLanguage())
	{
		return false;
	}

	UClass* WidgetClass = LoadClass<UStartupLanguageGPT62Sol>(nullptr,
		TEXT("/Game/UI/Sol62/WBP_StartupLanguage_GPT62Sol.WBP_StartupLanguage_GPT62Sol_C"));
	if (!WidgetClass)
	{
		UE_LOG(LogTemp, Error, TEXT("StartupLanguage: widget could not be loaded"));
		return false;
	}
	StartupLanguageWidget = CreateWidget<UStartupLanguageGPT62Sol>(this, WidgetClass);
	if (!StartupLanguageWidget)
	{
		return false;
	}
	StartupLanguageWidget->OnLanguageSelected.BindUObject(this, &ThisClass::HandleStartupLanguageSelected);
	StartupLanguageWidget->AddToViewport(50);
	StartupLanguageWidget->ForceLayoutPrepass();

	FInputModeUIOnly InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
	bShowMouseCursor = true;
	ScreenFadeWidget = CreateWidget<UTunaSweeperScreenFadeWidget>(this, UTunaSweeperScreenFadeWidget::StaticClass());
	if (ScreenFadeWidget)
	{
		StartupLanguageWidget->SetIsEnabled(false);
		ScreenFadeWidget->AddToViewport(1000);
		ScreenFadeWidget->StartFadeFromBlack(0.45f,
			FSimpleDelegate::CreateUObject(StartupLanguageWidget.Get(), &UStartupLanguageGPT62Sol::EnableSelection));
	}
	else
	{
		StartupLanguageWidget->EnableSelection();
	}
	UE_LOG(LogTemp, Display, TEXT("StartupLanguage: first-launch selection shown"));
	return true;
}

void ATunaSweeperPlayerController::HandleStartupLanguageSelected(ETunaSweeperItemTextLanguage Language)
{
	auto* Game = Cast<UTunaSweeperGameInstance>(GetGameInstance());
	if (!StartupLanguageWidget)
	{
		return;
	}
	if (!Game || !Game->ConfirmInitialTextLanguage(Language))
	{
		UE_LOG(LogTemp, Warning, TEXT("StartupLanguage: preference could not be persisted"));
		StartupLanguageWidget->EnableSelection();
		return;
	}
	if (ScreenFadeWidget)
	{
		ScreenFadeWidget->AddToViewport(1000);
		ScreenFadeWidget->StartFadeToBlack(0.45f,
			FSimpleDelegate::CreateUObject(this, &ThisClass::CompleteStartupLanguageTransition));
		return;
	}
	CompleteStartupLanguageTransition();
}

void ATunaSweeperPlayerController::CompleteStartupLanguageTransition()
{
	if (ScreenFadeWidget)
	{
		ScreenFadeWidget->RemoveFromParent();
		ScreenFadeWidget = nullptr;
	}
	StartupLanguageWidget->OnLanguageSelected.Unbind();
	StartupLanguageWidget->RemoveFromParent();
	StartupLanguageWidget = nullptr;
	EnsureIntroMenuWidget();
}
