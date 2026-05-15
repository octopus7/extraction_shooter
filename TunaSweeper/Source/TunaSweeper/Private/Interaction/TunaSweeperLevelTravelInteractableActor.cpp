#include "Interaction/TunaSweeperLevelTravelInteractableActor.h"

#include "Interaction/TunaSweeperInteractableComponent.h"
#include "Kismet/GameplayStatics.h"

ATunaSweeperLevelTravelInteractableActor::ATunaSweeperLevelTravelInteractableActor()
{
	TargetLevelName = NAME_None;

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
	TSoftClassPtr<UTunaSweeperInteractionMarkerWidget> InMarkerWidgetClass)
{
	Modify();
	TargetLevelName = InTargetLevelName;
	ConfigureInteractionDefaults(ETunaSweeperInteractionType::LevelTravel, InInteractionDisplayName, InMarkerWidgetClass);
}

bool ATunaSweeperLevelTravelInteractableActor::TravelToTargetLevel(APawn* InstigatorPawn)
{
	if (TargetLevelName.IsNone())
	{
		return false;
	}

	UObject* WorldContextObject = InstigatorPawn ? Cast<UObject>(InstigatorPawn) : Cast<UObject>(this);
	UGameplayStatics::OpenLevel(WorldContextObject, TargetLevelName);
	return true;
}
