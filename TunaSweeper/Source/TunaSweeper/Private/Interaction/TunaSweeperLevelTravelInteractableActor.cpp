#include "Interaction/TunaSweeperLevelTravelInteractableActor.h"

#include "Interaction/TunaSweeperInteractableComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Subsystem/TunaSweeperLevelTransitionSubsystem.h"

ATunaSweeperLevelTravelInteractableActor::ATunaSweeperLevelTravelInteractableActor()
{
	TargetLevelName = NAME_None;
	TransitionWidgetClass = TSoftClassPtr<UTunaSweeperLevelTransitionWidget>(
		FSoftObjectPath(TEXT("/Game/UI/WBP_LevelTransitionVideo.WBP_LevelTransitionVideo_C")));

	if (InteractableComponent)
	{
		InteractableComponent->SetInteractionTypeAndDisplayName(
			ETunaSweeperInteractionType::LevelTravel,
			FText::FromString(TEXT("Travel")));
	}
}

void ATunaSweeperLevelTravelInteractableActor::ConfigureLevelTravelDefaults(
	FName InTargetLevelName,
	const FText& InInteractionDisplayName,
	TSoftClassPtr<UTunaSweeperInteractionMarkerWidget> InMarkerWidgetClass,
	TSoftObjectPtr<UMediaSource> InTransitionMediaSource,
	TSoftClassPtr<UTunaSweeperLevelTransitionWidget> InTransitionWidgetClass)
{
	Modify();
	TargetLevelName = InTargetLevelName;
	TransitionMediaSource = InTransitionMediaSource;
	if (!InTransitionWidgetClass.IsNull())
	{
		TransitionWidgetClass = InTransitionWidgetClass;
	}
	ConfigureInteractionDefaults(ETunaSweeperInteractionType::LevelTravel, InInteractionDisplayName, InMarkerWidgetClass);
}

bool ATunaSweeperLevelTravelInteractableActor::TravelToTargetLevel(APawn* InstigatorPawn)
{
	if (TargetLevelName.IsNone())
	{
		return false;
	}

	UObject* WorldContextObject = InstigatorPawn ? Cast<UObject>(InstigatorPawn) : Cast<UObject>(this);
	if (!TransitionMediaSource.IsNull() && !TransitionWidgetClass.IsNull())
	{
		if (UGameInstance* GameInstance = GetGameInstance())
		{
			if (UTunaSweeperLevelTransitionSubsystem* TransitionSubsystem = GameInstance->GetSubsystem<UTunaSweeperLevelTransitionSubsystem>())
			{
				if (TransitionSubsystem->StartTransition(
					WorldContextObject,
					TargetLevelName,
					TransitionMediaSource,
					TransitionWidgetClass,
					FadeToBlackDuration,
					FadeFromBlackDuration))
				{
					return true;
				}
			}
		}
	}

	UGameplayStatics::OpenLevel(WorldContextObject, TargetLevelName);
	return true;
}
