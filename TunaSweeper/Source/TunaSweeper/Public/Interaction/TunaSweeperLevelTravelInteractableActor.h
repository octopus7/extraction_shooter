#pragma once

#include "CoreMinimal.h"
#include "Interaction/TunaSweeperInteractableActor.h"
#include "TunaSweeperLevelTravelInteractableActor.generated.h"

class APawn;
class UMediaSource;
class UTunaSweeperLevelTransitionWidget;
class UTunaSweeperInteractionMarkerWidget;

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperLevelTravelInteractableActor : public ATunaSweeperInteractableActor
{
	GENERATED_BODY()

public:
	ATunaSweeperLevelTravelInteractableActor();

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Level Travel")
	FName GetTargetLevelName() const { return TargetLevelName; }

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Level Travel")
	void ConfigureLevelTravelDefaults(
		FName InTargetLevelName,
		const FText& InInteractionDisplayName,
		TSoftClassPtr<UTunaSweeperInteractionMarkerWidget> InMarkerWidgetClass,
		TSoftObjectPtr<UMediaSource> InTransitionMediaSource,
		TSoftClassPtr<UTunaSweeperLevelTransitionWidget> InTransitionWidgetClass);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Level Travel")
	bool TravelToTargetLevel(APawn* InstigatorPawn);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Travel")
	FName TargetLevelName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Travel|Transition Video")
	TSoftObjectPtr<UMediaSource> TransitionMediaSource;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Travel|Transition Video")
	TSoftClassPtr<UTunaSweeperLevelTransitionWidget> TransitionWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Travel|Transition Video", meta = (ClampMin = "0.01", UIMin = "0.01"))
	float FadeToBlackDuration = 0.45f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Travel|Transition Video", meta = (ClampMin = "0.01", UIMin = "0.01"))
	float FadeFromBlackDuration = 0.55f;
};
