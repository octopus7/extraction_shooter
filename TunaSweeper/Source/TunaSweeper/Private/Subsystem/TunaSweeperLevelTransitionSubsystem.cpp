#include "Subsystem/TunaSweeperLevelTransitionSubsystem.h"

#include "Blueprint/UserWidget.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"
#include "HAL/PlatformTime.h"
#include "Kismet/GameplayStatics.h"
#include "MediaPlayer.h"
#include "MediaSource.h"
#include "MediaTexture.h"
#include "Player/TunaSweeperPlayerController.h"
#include "Subsystem/TunaSweeperBgmSubsystem.h"
#include "Subsystem/TunaSweeperEnemySpawnSubsystem.h"
#include "Stats/Stats.h"
#include "UI/TunaSweeperLevelTransitionWidget.h"
#include "UObject/UObjectGlobals.h"

namespace
{
	constexpr float CircularRevealInitialRadiusPixels = 72.0f;
	constexpr float CircularRevealInitialDurationSeconds = 0.2f;
	constexpr float CircularRevealHoldDurationSeconds = 0.1f;
	constexpr float CircularRevealFinalDurationSeconds = 0.3f;

	bool ShouldUseLetterboxForMediaSource(UMediaSource* MediaSource, const TSoftObjectPtr<UMediaSource>& MediaSourceReference)
	{
		const FString MediaSourceName = MediaSource ? MediaSource->GetName() : FString();
		const FString MediaSourcePath = MediaSourceReference.ToSoftObjectPath().ToString();
		return MediaSourceName.Contains(TEXT("BunkerToRaid")) || MediaSourcePath.Contains(TEXT("BunkerToRaid"));
	}

	float EaseOutElastic(float Alpha)
	{
		const float ClampedAlpha = FMath::Clamp(Alpha, 0.0f, 1.0f);
		if (ClampedAlpha <= 0.0f || ClampedAlpha >= 1.0f)
		{
			return ClampedAlpha;
		}

		constexpr float Period = (2.0f * PI) / 3.0f;
		return FMath::Pow(2.0f, -10.0f * ClampedAlpha) *
			FMath::Sin((ClampedAlpha * 10.0f - 0.75f) * Period) + 1.0f;
	}

	float SmoothStep(float Alpha)
	{
		const float ClampedAlpha = FMath::Clamp(Alpha, 0.0f, 1.0f);
		return ClampedAlpha * ClampedAlpha * (3.0f - 2.0f * ClampedAlpha);
	}
}

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

	case ETransitionPhase::CircularRevealInitialElastic:
	case ETransitionPhase::CircularRevealHold:
	case ETransitionPhase::CircularRevealFinalExpand:
		UpdateCircularReveal(DeltaTime);
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
	bUseLetterbox = false;
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
	bUseLetterbox = ShouldUseLetterboxForMediaSource(MediaSource, InMediaSource);

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
	ActiveWidget->SetLetterboxEnabled(bUseLetterbox);
	ActiveWidget->SetVideoVisible(false);
	ActiveWidget->SetBlackOpacity(0.0f);
	ActiveWidget->SetCircularRevealMask(0.0f, false);

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UTunaSweeperBgmSubsystem* BgmSubsystem = GameInstance->GetSubsystem<UTunaSweeperBgmSubsystem>())
		{
			BgmSubsystem->FadeOutAndStop(FadeToBlackDuration);
		}
	}

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

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UTunaSweeperEnemySpawnSubsystem* EnemySpawnSubsystem = GameInstance->GetSubsystem<UTunaSweeperEnemySpawnSubsystem>())
		{
			EnemySpawnSubsystem->EnsureRaidRuntimeActorsSpawnedForWorld(LoadedWorld);
		}
	}

	EnsureTransitionWidget(LoadedWorld);
	if (ActiveWidget)
	{
		if (MediaTexture)
		{
			ActiveWidget->SetVideoTexture(MediaTexture);
		}
		ActiveWidget->SetTransitionMessage(FText::GetEmpty());
		ActiveWidget->SetLetterboxEnabled(bUseLetterbox);
		ActiveWidget->SetVideoVisible(false);
		ActiveWidget->SetBlackOpacity(0.0f);
	}

	BeginCircularReveal();
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
	ActiveWidget->SetLetterboxEnabled(false);
	ActiveWidget->SetTransitionMessage(TransitionMessage);
	ActiveWidget->SetBlackOpacity(0.0f);
	ActiveWidget->SetCircularRevealMask(0.0f, false);

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
		ActiveWidget->SetLetterboxEnabled(bUseLetterbox);
		ActiveWidget->SetVideoVisible(true);
		ActiveWidget->SetBlackOpacity(1.0f);
	}

	VideoVisibleStartSeconds = FPlatformTime::Seconds();
	FadeElapsedSeconds = 0.0f;
	Phase = ETransitionPhase::FadingFromBlackToVideo;
}

void UTunaSweeperLevelTransitionSubsystem::BeginCircularReveal()
{
	if (!ActiveWidget)
	{
		FinishTransition();
		return;
	}

	CircularRevealFinalRadius = GetFullscreenRevealRadius();
	FadeElapsedSeconds = 0.0f;
	SetBlackOpacity(0.0f);
	SetCircularRevealMask(0.0f, true);
	Phase = ETransitionPhase::CircularRevealInitialElastic;
}

void UTunaSweeperLevelTransitionSubsystem::UpdateCircularReveal(float DeltaTime)
{
	FadeElapsedSeconds += FMath::Max(0.0f, DeltaTime);

	switch (Phase)
	{
	case ETransitionPhase::CircularRevealInitialElastic:
	{
		const float Alpha = CircularRevealInitialDurationSeconds > 0.0f
			? FadeElapsedSeconds / CircularRevealInitialDurationSeconds
			: 1.0f;
		SetCircularRevealMask(CircularRevealInitialRadiusPixels * FMath::Max(0.0f, EaseOutElastic(Alpha)), true);
		if (FadeElapsedSeconds >= CircularRevealInitialDurationSeconds)
		{
			FadeElapsedSeconds = 0.0f;
			SetCircularRevealMask(CircularRevealInitialRadiusPixels, true);
			Phase = ETransitionPhase::CircularRevealHold;
		}
		break;
	}

	case ETransitionPhase::CircularRevealHold:
		SetCircularRevealMask(CircularRevealInitialRadiusPixels, true);
		if (FadeElapsedSeconds >= CircularRevealHoldDurationSeconds)
		{
			FadeElapsedSeconds = 0.0f;
			Phase = ETransitionPhase::CircularRevealFinalExpand;
		}
		break;

	case ETransitionPhase::CircularRevealFinalExpand:
	{
		const float Alpha = CircularRevealFinalDurationSeconds > 0.0f
			? FadeElapsedSeconds / CircularRevealFinalDurationSeconds
			: 1.0f;
		const float Radius = FMath::Lerp(
			CircularRevealInitialRadiusPixels,
			FMath::Max(CircularRevealInitialRadiusPixels, CircularRevealFinalRadius),
			SmoothStep(Alpha));
		SetCircularRevealMask(Radius, true);
		if (FadeElapsedSeconds >= CircularRevealFinalDurationSeconds)
		{
			SetCircularRevealMask(0.0f, false);
			FinishTransition();
		}
		break;
	}

	default:
		break;
	}
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
		ActiveWidget->SetCircularRevealMask(0.0f, false);
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
	CircularRevealFinalRadius = 0.0f;
	VideoVisibleStartSeconds = 0.0;
	bOpenLevelRequested = false;
	bUseLetterbox = false;
}

void UTunaSweeperLevelTransitionSubsystem::SetBlackOpacity(float InOpacity)
{
	if (ActiveWidget)
	{
		ActiveWidget->SetBlackOpacity(InOpacity);
	}
}

void UTunaSweeperLevelTransitionSubsystem::SetCircularRevealMask(float HoleRadiusPixels, bool bVisible)
{
	if (ActiveWidget)
	{
		ActiveWidget->SetCircularRevealMask(HoleRadiusPixels, bVisible);
	}
}

float UTunaSweeperLevelTransitionSubsystem::GetVideoVisibleElapsedSeconds() const
{
	return VideoVisibleStartSeconds > 0.0
		? static_cast<float>(FPlatformTime::Seconds() - VideoVisibleStartSeconds)
		: 0.0f;
}

float UTunaSweeperLevelTransitionSubsystem::GetFullscreenRevealRadius() const
{
	FVector2D ViewportSize(1920.0f, 1080.0f);
	if (GEngine && GEngine->GameViewport && GEngine->GameViewport->Viewport)
	{
		const FIntPoint ViewportIntSize = GEngine->GameViewport->Viewport->GetSizeXY();
		if (ViewportIntSize.X > 0 && ViewportIntSize.Y > 0)
		{
			ViewportSize = FVector2D(
				static_cast<float>(ViewportIntSize.X),
				static_cast<float>(ViewportIntSize.Y));
		}
	}

	return FMath::Sqrt(FMath::Square(ViewportSize.X) + FMath::Square(ViewportSize.Y)) * 0.5f + 96.0f;
}
