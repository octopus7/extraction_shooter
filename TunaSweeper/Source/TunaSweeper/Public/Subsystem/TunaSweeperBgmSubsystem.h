#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "TunaSweeperBgmSubsystem.generated.h"

class UAudioComponent;
class USoundBase;

UCLASS()
class TUNASWEEPER_API UTunaSweeperBgmSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UTunaSweeperBgmSubsystem();

	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|BGM")
	bool PlayBgm(TSoftObjectPtr<USoundBase> BgmSound, float FadeInDuration = 0.75f, float VolumeMultiplier = 0.65f, bool bLoop = true);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|BGM")
	bool PlayTitleBgm(float FadeInDuration = 0.75f);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|BGM")
	void FadeOutAndStop(float FadeOutDuration = 0.5f);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|BGM")
	void StopBgm();

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|BGM")
	bool IsBgmPlaying() const;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TunaSweeper|BGM")
	TSoftObjectPtr<USoundBase> TitleBgmSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TunaSweeper|BGM", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "1.0"))
	float TitleBgmVolume = 0.65f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TunaSweeper|BGM")
	bool bTitleBgmLooping = true;

private:
	UFUNCTION()
	void HandleActiveBgmFinished();

	void ClearActiveBgmComponent(bool bStopComponent);

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> ActiveBgmComponent;

	UPROPERTY(Transient)
	TObjectPtr<USoundBase> ActiveBgmSound;

	float ActiveBgmVolume = 1.0f;
	bool bLoopActiveBgm = false;
	bool bFadeOutStopRequested = false;
};
