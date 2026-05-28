#pragma once

#include "CoreMinimal.h"
#include "Engine/StreamableManager.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "TunaSweeperRaidExperienceReturnSubsystem.generated.h"

class UMediaSource;
class UTunaSweeperLevelTransitionWidget;
class UTunaSweeperRaidExperienceWidget;

UCLASS()
class TUNASWEEPER_API UTunaSweeperRaidExperienceReturnSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Deinitialize() override;

	bool StartReturnPresentation(
		UObject* WorldContextObject,
		FName InTargetLevelName,
		TSoftObjectPtr<UMediaSource> InTransitionMediaSource,
		TSoftClassPtr<UTunaSweeperLevelTransitionWidget> InTransitionWidgetClass,
		float InFadeToBlackDuration,
		float InFadeFromBlackDuration,
		const FText& InTransitionMessage);

private:
	void HandleExperienceAnimationFinished();
	void HandleTargetLevelPreloadFinished();
	void HandleContinueRequested();
	void EvaluateContinueReady();
	void FinishAndClearWidget();
	FSoftObjectPath BuildTargetLevelSoftObjectPath(FName InTargetLevelName) const;

	UPROPERTY(Transient)
	TObjectPtr<UTunaSweeperRaidExperienceWidget> ActiveWidget;

	UPROPERTY(Transient)
	TObjectPtr<UObject> LastWorldContextObject;

	TSharedPtr<FStreamableHandle> TargetLevelPreloadHandle;
	TSoftObjectPtr<UMediaSource> TransitionMediaSource;
	TSoftClassPtr<UTunaSweeperLevelTransitionWidget> TransitionWidgetClass;
	FText TransitionMessage;
	FName TargetLevelName;
	float FadeToBlackDuration = 0.2f;
	float FadeFromBlackDuration = 0.2f;
	bool bExperienceAnimationFinished = false;
	bool bTargetLevelPreloadFinished = false;
	bool bReturnPresentationActive = false;
};
