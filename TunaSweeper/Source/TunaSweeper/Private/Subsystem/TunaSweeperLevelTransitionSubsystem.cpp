#include "Subsystem/TunaSweeperLevelTransitionSubsystem.h"

#include "Blueprint/UserWidget.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"
#include "HAL/PlatformTime.h"
#include "Kismet/GameplayStatics.h"
#include "MediaPlayer.h"
#include "MediaSource.h"
#include "MediaTexture.h"
#include "Player/TunaSweeperPlayerController.h"
#include "Stats/Stats.h"
#include "UI/TunaSweeperLevelTransitionWidget.h"
#include "UObject/UObjectGlobals.h"

void UTunaSweeperLevelTransitionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	PostLoadMapHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(
		this,
		&UTunaSweeperLevelTransitionSubsystem::HandlePostLoadMapWithWorld);
}

void UTunaSweeperLevelTransitionSubsystem::Deinitialize()
{
	if (PostLoadMapHandle.IsValid())
	{
		FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapHandle);
		PostLoadMapHandle.Reset();
	}

	FinishTransition();
	Super::Deinitialize();
}

void UTunaSweeperLevelTransitionSubsystem::Tick(float DeltaTime)
{
	switch (Phase)
	{
	case ETransitionPhase::FadingToBlackBeforeVideo:
		FadeElapsedSeconds += DeltaTime;
		SetBlackOpacity(FadeToBlackDuration > 0.0f ? FadeElapsedSeconds / FadeToBlackDuration : 1.0f);
		if (FadeElapsedSeconds >= FadeToBlackDuration)
		{
			SetBlackOpacity(1.0f);
			BeginVideoReveal();
		}
		break;

	case ETransitionPhase::FadingFromBlackToVideo:
		FadeElapsedSeconds += DeltaTime;
		SetBlackOpacity(1.0f - (FadeFromBlackDuration > 0.0f ? FadeElapsedSeconds / FadeFromBlackDuration : 1.0f));
		if (FadeElapsedSeconds >= FadeFromBlackDuration)
		{
			SetBlackOpacity(0.0f);
			OpenTargetLevel();
		}
		break;

	case ETransitionPhase::WaitingForMinimumVideoTime:
		if (GetVideoVisibleElapsedSeconds() >= MinimumVideoDisplaySeconds)
		{
			FinishTransition();
		}
		break;

	default:
		break;
	}
}

TStatId UTunaSweeperLevelTransitionSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UTunaSweeperLevelTransitionSubsystem, STATGROUP_Tickables);
}

bool UTunaSweeperLevelTransitionSubsystem::IsTickable() const
{
	return Phase != ETransitionPhase::Idle;
}

bool UTunaSweeperLevelTransitionSubsystem::StartTransition(
	UObject* WorldContextObject,
	FName InTargetLevelName,
	TSoftObjectPtr<UMediaSource> InMediaSource,
	TSoftClassPtr<UTunaSweeperLevelTransitionWidget> InWidgetClass,
	float InFadeToBlackDuration,
	float InFadeFromBlackDuration,
	const FText& InTransitionMessage)
{
	if (Phase != ETransitionPhase::Idle || InTargetLevelName.IsNone() || InMediaSource.IsNull() || InWidgetClass.IsNull())
	{
		return false;
	}

	TargetLevelName = InTargetLevelName;
	WidgetClass = InWidgetClass;
	TransitionMessage = InTransitionMessage;
	FadeToBlackDuration = FMath::Max(0.01f, InFadeToBlackDuration);
	FadeFromBlackDuration = FMath::Max(0.01f, InFadeFromBlackDuration);
	FadeElapsedSeconds = 0.0f;
	VideoVisibleStartSeconds = 0.0;
	bOpenLevelRequested = false;
	LastWorldContextObject = WorldContextObject;

	if (!EnsureTransitionWidget(WorldContextObject))
	{
		FinishTransition();
		return false;
	}

	UMediaSource* MediaSource = InMediaSource.LoadSynchronous();
	if (!MediaSource)
	{
		FinishTransition();
		return false;
	}

	MediaPlayer = NewObject<UMediaPlayer>(this, TEXT("LevelTransitionMediaPlayer"));
	MediaTexture = NewObject<UMediaTexture>(this, TEXT("LevelTransitionMediaTexture"));
	if (!MediaPlayer || !MediaTexture)
	{
		FinishTransition();
		return false;
	}

	MediaPlayer->OnMediaOpened.AddDynamic(this, &UTunaSweeperLevelTransitionSubsystem::HandleMediaOpened);
	MediaPlayer->OnMediaOpenFailed.AddDynamic(this, &UTunaSweeperLevelTransitionSubsystem::HandleMediaOpenFailed);
	MediaPlayer->OnEndReached.AddDynamic(this, &UTunaSweeperLevelTransitionSubsystem::HandleMediaEndReached);
	MediaPlayer->SetLooping(true);

	MediaTexture->SetMediaPlayer(MediaPlayer);
	MediaTexture->UpdateResource();

	ActiveWidget->SetVideoTexture(MediaTexture);
	ActiveWidget->SetTransitionMessage(TransitionMessage);
	ActiveWidget->SetVideoVisible(false);
	ActiveWidget->SetBlackOpacity(0.0f);

	Phase = ETransitionPhase::FadingToBlackBeforeVideo;
	if (!MediaPlayer->OpenSource(MediaSource))
	{
		FinishTransition();
		return false;
	}

	return true;
}

void UTunaSweeperLevelTransitionSubsystem::HandleMediaOpened(FString)
{
	if (MediaPlayer && Phase != ETransitionPhase::Idle)
	{
		MediaPlayer->Play();
	}
}

void UTunaSweeperLevelTransitionSubsystem::HandleMediaOpenFailed(FString)
{
	OpenTargetLevel();
}

void UTunaSweeperLevelTransitionSubsystem::HandleMediaEndReached()
{
	if (MediaPlayer)
	{
		MediaPlayer->Rewind();
		MediaPlayer->Play();
	}
}

void UTunaSweeperLevelTransitionSubsystem::HandlePostLoadMapWithWorld(UWorld* LoadedWorld)
{
	if (Phase != ETransitionPhase::LoadingLevel)
	{
		return;
	}

	EnsureTransitionWidget(LoadedWorld);
	if (ActiveWidget)
	{
		if (MediaTexture)
		{
			ActiveWidget->SetVideoTexture(MediaTexture);
		}
		ActiveWidget->SetTransitionMessage(TransitionMessage);
		ActiveWidget->SetVideoVisible(true);
		ActiveWidget->SetBlackOpacity(0.0f);
	}

	if (GetVideoVisibleElapsedSeconds() >= MinimumVideoDisplaySeconds)
	{
		FinishTransition();
		return;
	}

	Phase = ETransitionPhase::WaitingForMinimumVideoTime;
}

bool UTunaSweeperLevelTransitionSubsystem::EnsureTransitionWidget(UObject* WorldContextObject)
{
	if (ActiveWidget && ActiveWidget->IsInViewport())
	{
		return true;
	}

	TSubclassOf<UTunaSweeperLevelTransitionWidget> LoadedWidgetClass = WidgetClass.LoadSynchronous();
	UGameInstance* GameInstance = GetGameInstance();
	if (!LoadedWidgetClass || !GameInstance)
	{
		return false;
	}

	ActiveWidget = CreateWidget<UTunaSweeperLevelTransitionWidget>(GameInstance, LoadedWidgetClass);
	if (!ActiveWidget)
	{
		return false;
	}

	ActiveWidget->AddToViewport(1000);
	ActiveWidget->SetVideoVisible(false);
	ActiveWidget->SetTransitionMessage(TransitionMessage);
	ActiveWidget->SetBlackOpacity(0.0f);

	if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(WorldContextObject ? WorldContextObject : GameInstance, 0))
	{
		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(ActiveWidget->TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PlayerController->SetInputMode(InputMode);
		PlayerController->bShowMouseCursor = false;
	}

	return true;
}

void UTunaSweeperLevelTransitionSubsystem::BeginVideoReveal()
{
	if (Phase != ETransitionPhase::FadingToBlackBeforeVideo)
	{
		return;
	}

	if (ActiveWidget)
	{
		if (MediaTexture)
		{
			ActiveWidget->SetVideoTexture(MediaTexture);
		}
		ActiveWidget->SetTransitionMessage(TransitionMessage);
		ActiveWidget->SetVideoVisible(true);
		ActiveWidget->SetBlackOpacity(1.0f);
	}

	VideoVisibleStartSeconds = FPlatformTime::Seconds();
	FadeElapsedSeconds = 0.0f;
	Phase = ETransitionPhase::FadingFromBlackToVideo;
}

void UTunaSweeperLevelTransitionSubsystem::OpenTargetLevel()
{
	if (bOpenLevelRequested || TargetLevelName.IsNone())
	{
		return;
	}

	bOpenLevelRequested = true;
	Phase = ETransitionPhase::LoadingLevel;
	if (VideoVisibleStartSeconds <= 0.0)
	{
		VideoVisibleStartSeconds = FPlatformTime::Seconds();
	}
	UObject* WorldContextObject = LastWorldContextObject ? LastWorldContextObject.Get() : GetGameInstance();
	UGameplayStatics::OpenLevel(WorldContextObject, TargetLevelName);
}

void UTunaSweeperLevelTransitionSubsystem::FinishTransition()
{
	if (MediaPlayer)
	{
		MediaPlayer->Close();
	}

	if (ActiveWidget)
	{
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
	MediaPlayer = nullptr;
	MediaTexture = nullptr;
	LastWorldContextObject = nullptr;
	TargetLevelName = NAME_None;
	TransitionMessage = FText::GetEmpty();
	Phase = ETransitionPhase::Idle;
	FadeElapsedSeconds = 0.0f;
	VideoVisibleStartSeconds = 0.0;
	bOpenLevelRequested = false;
}

void UTunaSweeperLevelTransitionSubsystem::SetBlackOpacity(float InOpacity)
{
	if (ActiveWidget)
	{
		ActiveWidget->SetBlackOpacity(InOpacity);
	}
}

float UTunaSweeperLevelTransitionSubsystem::GetVideoVisibleElapsedSeconds() const
{
	return VideoVisibleStartSeconds > 0.0
		? static_cast<float>(FPlatformTime::Seconds() - VideoVisibleStartSeconds)
		: 0.0f;
}
