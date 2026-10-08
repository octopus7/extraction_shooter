#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/TunaSweeperStartupLogoFlow.h"
#include "TunaSweeperStartupLogoWidget.generated.h"

class UImage;
class UMediaPlayer;
class UMediaTexture;
class UMediaSource;
class UMediaSoundComponent;

DECLARE_DELEGATE_OneParam(FTunaSweeperStartupLogoFinished, bool);

UCLASS(Config=Game)
class TUNASWEEPER_API UTunaSweeperStartupLogoWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	FTunaSweeperStartupLogoFinished OnFinished;
	void Start();
	void Stop();

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& Geometry, float DeltaTime) override;
	virtual void NativeDestruct() override;

	UPROPERTY(Config) float FadeSeconds = 0.7f;
	UPROPERTY(Config) float StillSeconds = 1.5f;
	UPROPERTY(Config) float BlackSeconds = 0.2f;

private:
	UFUNCTION() void HandleOpened(FString Url);
	UFUNCTION() void HandleFailed(FString Url);
	UFUNCTION() void HandleEnded();
	UPROPERTY(Transient) TObjectPtr<UImage> VideoImage;
	UPROPERTY(Transient) TObjectPtr<UImage> StillImage;
	UPROPERTY(Transient) TObjectPtr<UImage> BlackImage;
	UPROPERTY(Transient) TObjectPtr<UMediaPlayer> MediaPlayer;
	UPROPERTY(Transient) TObjectPtr<UMediaTexture> MediaTexture;
	UPROPERTY(Transient) TObjectPtr<UMediaSource> MediaSource;
	UPROPERTY(Transient) TObjectPtr<UMediaSoundComponent> MediaSound;
	TunaSweeperStartupLogo::FFlow Flow;
	bool bStarted = false;
	bool bFinished = false;
};
