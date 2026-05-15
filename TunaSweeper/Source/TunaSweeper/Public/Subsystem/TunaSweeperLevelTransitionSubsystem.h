#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "TunaSweeperLevelTransitionSubsystem.generated.h"

class UMediaPlayer;
class UMediaSource;
class UMediaTexture;
class UTunaSweeperLevelTransitionWidget;

UCLASS()
class TUNASWEEPER_API UTunaSweeperLevelTransitionSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickable() const override;

	bool StartTransition(
		UObject* WorldContextObject,
		FName InTargetLevelName,
		TSoftObjectPtr<UMediaSource> InMediaSource,
		TSoftClassPtr<UTunaSweeperLevelTransitionWidget> InWidgetClass,
		float InFadeToBlackDuration,
		float InFadeFromBlackDuration);

private:
	enum class ETransitionPhase : uint8
	{
		Idle,
		PlayingVideo,
		FadingToBlackBeforeLoad,
		WaitingForPostLoad,
		FadingFromBlackAfterLoad
	};

	UFUNCTION()
	void HandleMediaOpened(FString OpenedUrl);

	UFUNCTION()
	void HandleMediaOpenFailed(FString FailedUrl);

	UFUNCTION()
	void HandleMediaEndReached();

	void HandlePostLoadMapWithWorld(UWorld* LoadedWorld);
	bool EnsureTransitionWidget(UObject* WorldContextObject);
	void BeginFadeToBlack();
	void OpenTargetLevel();
	void FinishTransition();
	void SetBlackOpacity(float InOpacity);

	UPROPERTY(Transient)
	TObjectPtr<UTunaSweeperLevelTransitionWidget> ActiveWidget;

	UPROPERTY(Transient)
	TObjectPtr<UMediaPlayer> MediaPlayer;

	UPROPERTY(Transient)
	TObjectPtr<UMediaTexture> MediaTexture;

	UPROPERTY(Transient)
	TObjectPtr<UObject> LastWorldContextObject;

	TSoftClassPtr<UTunaSweeperLevelTransitionWidget> WidgetClass;
	FName TargetLevelName;
	ETransitionPhase Phase = ETransitionPhase::Idle;
	float FadeElapsedSeconds = 0.0f;
	float FadeToBlackDuration = 0.45f;
	float FadeFromBlackDuration = 0.55f;
	bool bOpenLevelRequested = false;
	FDelegateHandle PostLoadMapHandle;
};
