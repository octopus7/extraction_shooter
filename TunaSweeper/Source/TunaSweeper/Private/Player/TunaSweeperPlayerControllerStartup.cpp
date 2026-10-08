#include "Player/TunaSweeperPlayerController.h"
#include "Game/TunaSweeperGameInstance.h"
#include "UI/TunaSweeperStartupLogoWidget.h"

bool ATunaSweeperPlayerController::TryStartDeveloperLogo()
{
	if (!IsLocalController()) return false;
	UTunaSweeperGameInstance* Instance = Cast<UTunaSweeperGameInstance>(GetGameInstance());
	if (!Instance || !Instance->TryBeginDeveloperLogo()) return false;
	StartupLogoWidget = CreateWidget<UTunaSweeperStartupLogoWidget>(this);
	if (!StartupLogoWidget) return false;
	StartupLogoWidget->OnFinished.BindUObject(this, &ThisClass::HandleDeveloperLogoFinished);
	StartupLogoWidget->AddToViewport(2000);
	FInputModeUIOnly InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
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
	// Keep the opaque widget alive until the title and its own fade overlay exist.
	UTunaSweeperStartupLogoWidget* FinishedWidget = StartupLogoWidget;
	StartupLogoWidget = nullptr;
	EnsureIntroMenuWidget();
	if (FinishedWidget) FinishedWidget->RemoveFromParent();
	UE_LOG(LogTemp, Display, TEXT("StartupLogo: title released (video completed=%d)"), bVideoCompleted);
}
