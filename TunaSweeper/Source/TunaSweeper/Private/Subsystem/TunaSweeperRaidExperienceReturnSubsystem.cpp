#include "Subsystem/TunaSweeperRaidExperienceReturnSubsystem.h"

#include "Engine/AssetManager.h"
#include "Game/TunaSweeperGameInstance.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "MediaSource.h"
#include "Misc/PackageName.h"
#include "Player/TunaSweeperPlayerController.h"
#include "Subsystem/TunaSweeperLevelTransitionSubsystem.h"
#include "UI/TunaSweeperLevelTransitionWidget.h"
#include "UI/TunaSweeperRaidExperienceWidget.h"

void UTunaSweeperRaidExperienceReturnSubsystem::Deinitialize()
{
	FinishAndClearWidget();
	TargetLevelPreloadHandle.Reset();
	Super::Deinitialize();
}

bool UTunaSweeperRaidExperienceReturnSubsystem::StartReturnPresentation(
	UObject* WorldContextObject,
	FName InTargetLevelName,
	TSoftObjectPtr<UMediaSource> InTransitionMediaSource,
	TSoftClassPtr<UTunaSweeperLevelTransitionWidget> InTransitionWidgetClass,
	float InFadeToBlackDuration,
	float InFadeFromBlackDuration,
	const FText& InTransitionMessage)
{
	if (bReturnPresentationActive || InTargetLevelName.IsNone())
	{
		return false;
	}

	UTunaSweeperGameInstance* TunaGameInstance = Cast<UTunaSweeperGameInstance>(GetGameInstance());
	if (!TunaGameInstance)
	{
		return false;
	}

	FTunaSweeperExperienceAnimationState AnimationState;
	if (!TunaGameInstance->ConsumePendingRaidExperienceAnimationState(AnimationState))
	{
		return false;
	}

	ActiveWidget = CreateWidget<UTunaSweeperRaidExperienceWidget>(
		TunaGameInstance,
		UTunaSweeperRaidExperienceWidget::StaticClass());
	if (!ActiveWidget)
	{
		return false;
	}

	TargetLevelPreloadHandle.Reset();
	TransitionMediaSource = InTransitionMediaSource;
	TransitionWidgetClass = InTransitionWidgetClass;
	TransitionMessage = InTransitionMessage;
	TargetLevelName = InTargetLevelName;
	FadeToBlackDuration = FMath::Max(0.01f, InFadeToBlackDuration);
	FadeFromBlackDuration = FMath::Max(0.01f, InFadeFromBlackDuration);
	LastWorldContextObject = WorldContextObject ? WorldContextObject : Cast<UObject>(TunaGameInstance);
	bExperienceAnimationFinished = false;
	bTargetLevelPreloadFinished = false;
	bReturnPresentationActive = true;

	ActiveWidget->AddToViewport(900);
	ActiveWidget->OnAnimationFinished.AddUObject(
		this,
		&UTunaSweeperRaidExperienceReturnSubsystem::HandleExperienceAnimationFinished);
	ActiveWidget->OnContinueRequested.AddUObject(
		this,
		&UTunaSweeperRaidExperienceReturnSubsystem::HandleContinueRequested);
	ActiveWidget->StartExperiencePresentation(AnimationState);

	if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(LastWorldContextObject, 0))
	{
		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(ActiveWidget->TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PlayerController->SetInputMode(InputMode);
		PlayerController->bShowMouseCursor = true;
		PlayerController->SetIgnoreMoveInput(true);
		PlayerController->SetIgnoreLookInput(true);
	}

	const FSoftObjectPath TargetLevelPath = BuildTargetLevelSoftObjectPath(TargetLevelName);
	if (TargetLevelPath.IsValid())
	{
		TargetLevelPreloadHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(
			TargetLevelPath,
			FStreamableDelegate::CreateUObject(
				this,
				&UTunaSweeperRaidExperienceReturnSubsystem::HandleTargetLevelPreloadFinished));
		if (!TargetLevelPreloadHandle.IsValid() || TargetLevelPreloadHandle->HasLoadCompleted())
		{
			HandleTargetLevelPreloadFinished();
		}
	}
	else
	{
		HandleTargetLevelPreloadFinished();
	}

	return true;
}

void UTunaSweeperRaidExperienceReturnSubsystem::HandleExperienceAnimationFinished()
{
	bExperienceAnimationFinished = true;
	EvaluateContinueReady();
}

void UTunaSweeperRaidExperienceReturnSubsystem::HandleTargetLevelPreloadFinished()
{
	bTargetLevelPreloadFinished = true;
	EvaluateContinueReady();
}

void UTunaSweeperRaidExperienceReturnSubsystem::HandleContinueRequested()
{
	if (!bReturnPresentationActive || !bExperienceAnimationFinished || !bTargetLevelPreloadFinished)
	{
		return;
	}

	UObject* WorldContextObject = LastWorldContextObject ? LastWorldContextObject.Get() : Cast<UObject>(GetGameInstance());
	const FName LocalTargetLevelName = TargetLevelName;
	const TSoftObjectPtr<UMediaSource> LocalTransitionMediaSource = TransitionMediaSource;
	const TSoftClassPtr<UTunaSweeperLevelTransitionWidget> LocalTransitionWidgetClass = TransitionWidgetClass;
	const FText LocalTransitionMessage = TransitionMessage;
	const float LocalFadeToBlackDuration = FadeToBlackDuration;
	const float LocalFadeFromBlackDuration = FadeFromBlackDuration;
	FinishAndClearWidget();

	if (!LocalTransitionMediaSource.IsNull() && !LocalTransitionWidgetClass.IsNull())
	{
		if (UGameInstance* GameInstance = GetGameInstance())
		{
			if (UTunaSweeperLevelTransitionSubsystem* TransitionSubsystem = GameInstance->GetSubsystem<UTunaSweeperLevelTransitionSubsystem>())
			{
				if (TransitionSubsystem->StartTransition(
					WorldContextObject,
					LocalTargetLevelName,
					LocalTransitionMediaSource,
					LocalTransitionWidgetClass,
					LocalFadeToBlackDuration,
					LocalFadeFromBlackDuration,
					LocalTransitionMessage))
				{
					return;
				}
			}
		}
	}

	UGameplayStatics::OpenLevel(WorldContextObject, LocalTargetLevelName);
}

void UTunaSweeperRaidExperienceReturnSubsystem::EvaluateContinueReady()
{
	if (ActiveWidget)
	{
		ActiveWidget->SetContinueReady(bExperienceAnimationFinished && bTargetLevelPreloadFinished);
	}
}

void UTunaSweeperRaidExperienceReturnSubsystem::FinishAndClearWidget()
{
	if (ActiveWidget)
	{
		ActiveWidget->OnAnimationFinished.RemoveAll(this);
		ActiveWidget->OnContinueRequested.RemoveAll(this);
		ActiveWidget->RemoveFromParent();
	}

	if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetGameInstance(), 0))
	{
		if (ATunaSweeperPlayerController* TunaPlayerController = Cast<ATunaSweeperPlayerController>(PlayerController))
		{
			TunaPlayerController->ApplyDefaultGameInputMode();
		}
		else
		{
			FInputModeGameAndUI InputMode;
			InputMode.SetHideCursorDuringCapture(false);
			InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			PlayerController->SetInputMode(InputMode);
			PlayerController->bShowMouseCursor = true;
			PlayerController->SetIgnoreMoveInput(false);
			PlayerController->SetIgnoreLookInput(false);
		}
	}

	ActiveWidget = nullptr;
	LastWorldContextObject = nullptr;
	TransitionMediaSource.Reset();
	TransitionWidgetClass.Reset();
	TransitionMessage = FText::GetEmpty();
	TargetLevelName = NAME_None;
	FadeToBlackDuration = 0.2f;
	FadeFromBlackDuration = 0.2f;
	bExperienceAnimationFinished = false;
	bTargetLevelPreloadFinished = false;
	bReturnPresentationActive = false;
}

FSoftObjectPath UTunaSweeperRaidExperienceReturnSubsystem::BuildTargetLevelSoftObjectPath(
	FName InTargetLevelName) const
{
	if (InTargetLevelName.IsNone())
	{
		return FSoftObjectPath();
	}

	FString TargetLevelString = InTargetLevelName.ToString();
	FString PackageName = FPackageName::ObjectPathToPackageName(TargetLevelString);
	if (PackageName.IsEmpty() || !PackageName.StartsWith(TEXT("/")))
	{
		PackageName = FString::Printf(TEXT("/Game/%s"), *TargetLevelString);
	}

	const FString AssetName = FPackageName::GetShortName(PackageName);
	if (AssetName.IsEmpty())
	{
		return FSoftObjectPath();
	}

	return FSoftObjectPath(FString::Printf(TEXT("%s.%s"), *PackageName, *AssetName));
}
