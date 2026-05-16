#include "Interaction/TunaSweeperLevelTravelInteractableActor.h"

#include "Game/TunaSweeperGameInstance.h"
#include "Interaction/TunaSweeperInteractableComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Subsystem/TunaSweeperLevelTransitionSubsystem.h"
#include "Subsystem/TunaSweeperQuestSubsystem.h"

ATunaSweeperLevelTravelInteractableActor::ATunaSweeperLevelTravelInteractableActor()
{
	TargetLevelName = NAME_None;
	FadeToBlackDuration = 0.2f;
	FadeFromBlackDuration = 0.2f;
	TransitionMessage = FText::GetEmpty();
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
	TSoftClassPtr<UTunaSweeperLevelTransitionWidget> InTransitionWidgetClass,
	const FText& InTransitionMessage)
{
	Modify();
	TargetLevelName = InTargetLevelName;
	TransitionMediaSource = InTransitionMediaSource;
	TransitionMessage = InTransitionMessage;
	FadeToBlackDuration = 0.2f;
	FadeFromBlackDuration = 0.2f;
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
	const FName SourceLevelName = GetWorld() ? FName(*GetWorld()->GetMapName()) : NAME_None;
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UTunaSweeperGameInstance* TunaGameInstance = Cast<UTunaSweeperGameInstance>(GameInstance))
		{
			TunaGameInstance->HandleLevelTravelPersistence(SourceLevelName, TargetLevelName);
		}

		if (UTunaSweeperQuestSubsystem* QuestSubsystem = GameInstance->GetSubsystem<UTunaSweeperQuestSubsystem>())
		{
			QuestSubsystem->NotifyLevelTravelRequested(SourceLevelName, TargetLevelName);
		}
	}

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
					FadeFromBlackDuration,
					TransitionMessage))
				{
					return true;
				}
			}
		}
	}

	UGameplayStatics::OpenLevel(WorldContextObject, TargetLevelName);
	return true;
}
