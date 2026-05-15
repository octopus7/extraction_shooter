#include "Subsystem/TunaSweeperLevelTransitionSubsystem.h"

#include "Blueprint/UserWidget.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "MediaPlayer.h"
#include "MediaSource.h"
#include "MediaTexture.h"
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
	case ETransitionPhase::FadingToBlackBeforeLoad:
		FadeElapsedSeconds += DeltaTime;
		SetBlackOpacity(FadeToBlackDuration > 0.0f ? FadeElapsedSeconds / FadeToBlackDuration : 1.0f);
		if (FadeElapsedSeconds >= FadeToBlackDuration)
		{
			SetBlackOpacity(1.0f);
			OpenTargetLevel();
		}
		break;

	case ETransitionPhase::FadingFromBlackAfterLoad:
		FadeElapsedSeconds += DeltaTime;
		SetBlackOpacity(1.0f - (FadeFromBlackDuration > 0.0f ? FadeElapsedSeconds / FadeFromBlackDuration : 1.0f));
		if (FadeElapsedSeconds >= FadeFromBlackDuration)
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
	float InFadeFromBlackDuration)
{
	if (Phase != ETransitionPhase::Idle || InTargetLevelName.IsNone() || InMediaSource.IsNull() || InWidgetClass.IsNull())
	{
		return false;
	}

	TargetLevelName = InTargetLevelName;
	WidgetClass = InWidgetClass;
	FadeToBlackDuration = FMath::Max(0.01f, InFadeToBlackDuration);
	FadeFromBlackDuration = FMath::Max(0.01f, InFadeFromBlackDuration);
	FadeElapsedSeconds = 0.0f;
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
	MediaPlayer->SetLooping(false);

	MediaTexture->SetMediaPlayer(MediaPlayer);
	MediaTexture->UpdateResource();

	ActiveWidget->SetVideoTexture(MediaTexture);
	ActiveWidget->SetVideoVisible(true);
	ActiveWidget->SetBlackOpacity(0.0f);

	Phase = ETransitionPhase::PlayingVideo;
	if (!MediaPlayer->OpenSource(MediaSource))
	{
		FinishTransition();
		return false;
	}

	return true;
}

void UTunaSweeperLevelTransitionSubsystem::HandleMediaOpened(FString)
{
	if (MediaPlayer && Phase == ETransitionPhase::PlayingVideo)
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
	BeginFadeToBlack();
}

void UTunaSweeperLevelTransitionSubsystem::HandlePostLoadMapWithWorld(UWorld* LoadedWorld)
{
	if (Phase != ETransitionPhase::WaitingForPostLoad)
	{
		return;
	}

	EnsureTransitionWidget(LoadedWorld);
	if (ActiveWidget)
	{
		ActiveWidget->SetVideoVisible(false);
		ActiveWidget->SetBlackOpacity(1.0f);
	}

	FadeElapsedSeconds = 0.0f;
	Phase = ETransitionPhase::FadingFromBlackAfterLoad;
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
	ActiveWidget->SetBlackOpacity(1.0f);

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

void UTunaSweeperLevelTransitionSubsystem::BeginFadeToBlack()
{
	if (Phase != ETransitionPhase::PlayingVideo)
	{
		return;
	}

	FadeElapsedSeconds = 0.0f;
	Phase = ETransitionPhase::FadingToBlackBeforeLoad;
}

void UTunaSweeperLevelTransitionSubsystem::OpenTargetLevel()
{
	if (bOpenLevelRequested || TargetLevelName.IsNone())
	{
		return;
	}

	bOpenLevelRequested = true;
	Phase = ETransitionPhase::WaitingForPostLoad;
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

	ActiveWidget = nullptr;
	MediaPlayer = nullptr;
	MediaTexture = nullptr;
	LastWorldContextObject = nullptr;
	TargetLevelName = NAME_None;
	Phase = ETransitionPhase::Idle;
	FadeElapsedSeconds = 0.0f;
	bOpenLevelRequested = false;
}

void UTunaSweeperLevelTransitionSubsystem::SetBlackOpacity(float InOpacity)
{
	if (ActiveWidget)
	{
		ActiveWidget->SetBlackOpacity(InOpacity);
	}
}
