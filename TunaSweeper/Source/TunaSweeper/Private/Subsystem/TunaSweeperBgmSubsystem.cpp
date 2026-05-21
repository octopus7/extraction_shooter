#include "Subsystem/TunaSweeperBgmSubsystem.h"

#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

UTunaSweeperBgmSubsystem::UTunaSweeperBgmSubsystem()
{
	TitleBgmSound = TSoftObjectPtr<USoundBase>(
		FSoftObjectPath(TEXT("/Game/Audio/BGM/Where_the_Birds_Still_Sing.Where_the_Birds_Still_Sing")));
}

void UTunaSweeperBgmSubsystem::Deinitialize()
{
	ClearActiveBgmComponent(true);
	Super::Deinitialize();
}

bool UTunaSweeperBgmSubsystem::PlayBgm(
	TSoftObjectPtr<USoundBase> BgmSound,
	float FadeInDuration,
	float VolumeMultiplier,
	bool bLoop)
{
	if (BgmSound.IsNull())
	{
		return false;
	}

	USoundBase* LoadedSound = BgmSound.LoadSynchronous();
	UGameInstance* GameInstance = GetGameInstance();
	if (!LoadedSound || !GameInstance)
	{
		return false;
	}

	const float ClampedVolume = FMath::Max(0.0f, VolumeMultiplier);
	const float ClampedFadeInDuration = FMath::Max(0.0f, FadeInDuration);

	if (ActiveBgmComponent && ActiveBgmSound == LoadedSound)
	{
		bFadeOutStopRequested = false;
		bLoopActiveBgm = bLoop;
		ActiveBgmVolume = ClampedVolume;
		if (ClampedFadeInDuration > 0.0f)
		{
			ActiveBgmComponent->FadeIn(ClampedFadeInDuration, ActiveBgmVolume);
		}
		else
		{
			ActiveBgmComponent->SetVolumeMultiplier(ActiveBgmVolume);
			if (!ActiveBgmComponent->IsPlaying())
			{
				ActiveBgmComponent->Play(0.0f);
			}
		}
		return true;
	}

	ClearActiveBgmComponent(true);

	ActiveBgmComponent = UGameplayStatics::CreateSound2D(
		GameInstance,
		LoadedSound,
		ClampedFadeInDuration > 0.0f ? 0.0f : ClampedVolume,
		1.0f,
		0.0f,
		nullptr,
		true,
		false);
	if (!ActiveBgmComponent)
	{
		return false;
	}

	ActiveBgmSound = LoadedSound;
	ActiveBgmVolume = ClampedVolume;
	bLoopActiveBgm = bLoop;
	bFadeOutStopRequested = false;
	ActiveBgmComponent->OnAudioFinished.AddDynamic(this, &UTunaSweeperBgmSubsystem::HandleActiveBgmFinished);

	if (ClampedFadeInDuration > 0.0f)
	{
		ActiveBgmComponent->FadeIn(ClampedFadeInDuration, ActiveBgmVolume);
	}
	else
	{
		ActiveBgmComponent->Play(0.0f);
	}

	return true;
}

bool UTunaSweeperBgmSubsystem::PlayTitleBgm(float FadeInDuration)
{
	return PlayBgm(TitleBgmSound, FadeInDuration, TitleBgmVolume, bTitleBgmLooping);
}

void UTunaSweeperBgmSubsystem::FadeOutAndStop(float FadeOutDuration)
{
	if (!ActiveBgmComponent)
	{
		return;
	}

	const float ClampedFadeOutDuration = FMath::Max(0.0f, FadeOutDuration);
	if (ClampedFadeOutDuration <= 0.0f)
	{
		StopBgm();
		return;
	}

	bFadeOutStopRequested = true;
	bLoopActiveBgm = false;
	ActiveBgmComponent->FadeOut(ClampedFadeOutDuration, 0.0f);
}

void UTunaSweeperBgmSubsystem::StopBgm()
{
	ClearActiveBgmComponent(true);
}

bool UTunaSweeperBgmSubsystem::IsBgmPlaying() const
{
	return ActiveBgmComponent && ActiveBgmComponent->IsPlaying();
}

void UTunaSweeperBgmSubsystem::HandleActiveBgmFinished()
{
	if (!ActiveBgmComponent)
	{
		return;
	}

	if (bFadeOutStopRequested || !bLoopActiveBgm)
	{
		ClearActiveBgmComponent(false);
		return;
	}

	ActiveBgmComponent->SetVolumeMultiplier(ActiveBgmVolume);
	ActiveBgmComponent->Play(0.0f);
}

void UTunaSweeperBgmSubsystem::ClearActiveBgmComponent(bool bStopComponent)
{
	UAudioComponent* ComponentToClear = ActiveBgmComponent;
	ActiveBgmComponent = nullptr;
	ActiveBgmSound = nullptr;
	ActiveBgmVolume = 1.0f;
	bLoopActiveBgm = false;
	bFadeOutStopRequested = false;

	if (!ComponentToClear)
	{
		return;
	}

	ComponentToClear->OnAudioFinished.RemoveDynamic(this, &UTunaSweeperBgmSubsystem::HandleActiveBgmFinished);
	if (bStopComponent)
	{
		ComponentToClear->Stop();
	}
	ComponentToClear->DestroyComponent();
}
