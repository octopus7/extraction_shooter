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

// A fixed soft white frame above the video and still, masking both rectangular edges.
UCLASS()
class TUNASWEEPER_API UTunaSweeperStartupVideoEdge : public UWidget
{
	GENERATED_BODY()
protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
};

DECLARE_DELEGATE_OneParam(FTunaSweeperStartupLogoFinished, bool);

UCLASS(Config=Game)
class TUNASWEEPER_API UTunaSweeperStartupLogoWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	UTunaSweeperStartupLogoWidget(const FObjectInitializer& ObjectInitializer);
	FTunaSweeperStartupLogoFinished OnFinished;
	void Start();
	void Stop();

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& Geometry, float DeltaTime) override;
	virtual void NativeDestruct() override;
	virtual FReply NativeOnKeyDown(const FGeometry&, const FKeyEvent&) override;
	virtual FReply NativeOnKeyUp(const FGeometry&, const FKeyEvent&) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry&, const FPointerEvent&) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry&, const FPointerEvent&) override;
	virtual FReply NativeOnMouseMove(const FGeometry&, const FPointerEvent&) override;
	virtual FReply NativeOnMouseWheel(const FGeometry&, const FPointerEvent&) override;
	virtual FReply NativeOnAnalogValueChanged(const FGeometry&, const FAnalogInputEvent&) override;
	virtual FReply NativeOnTouchStarted(const FGeometry&, const FPointerEvent&) override;

	UPROPERTY(Config) float FadeSeconds = 0.25f;
	UPROPERTY(Config) float StillSeconds = 1.0f;
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
