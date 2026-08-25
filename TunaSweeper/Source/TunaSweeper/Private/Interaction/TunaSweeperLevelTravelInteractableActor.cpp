#include "Interaction/TunaSweeperLevelTravelInteractableActor.h"

#include "Game/TunaSweeperGameInstance.h"
#include "Interaction/TunaSweeperInteractableComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Subsystem/TunaSweeperLevelTransitionSubsystem.h"
#include "Subsystem/TunaSweeperQuestSubsystem.h"
#include "Subsystem/TunaSweeperRaidExperienceReturnSubsystem.h"

ATunaSweeperLevelTravelInteractableActor::ATunaSweeperLevelTravelInteractableActor()
{
	if (InteractableComponent)
	{
		InteractableComponent->SetInteractionTypeDisplayNameAndStringKey(
			ETunaSweeperInteractionType::LevelTravel,
			FText::FromString(TEXT("Travel")),
			FName(TEXT("ui.interaction.travel")));
	}
}

bool ATunaSweeperLevelTravelInteractableActor::TravelToTargetLevel(APawn* InstigatorPawn)
{
	UObject* WorldContextObject = InstigatorPawn ? Cast<UObject>(InstigatorPawn) : Cast<UObject>(this);
	const FName SourceLevelName = GetWorld() ? FName(*GetWorld()->GetMapName()) : NAME_None;
	UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	FName TargetLevelName = NAME_None;
	FTunaSweeperLevelTravelPresentationDefinition Presentation;
	if (!TunaGameInstance || !TunaGameInstance->TryResolveLevelTravel(Destination, TargetLevelName, Presentation))
	{
		return false;
	}

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		TunaGameInstance->HandleLevelTravelPersistence(SourceLevelName, TargetLevelName);

		if (UTunaSweeperQuestSubsystem* QuestSubsystem = GameInstance->GetSubsystem<UTunaSweeperQuestSubsystem>())
		{
			QuestSubsystem->NotifyLevelTravelRequested(SourceLevelName, TargetLevelName);
		}
	}

	const FText ResolvedTransitionMessage = !Presentation.TransitionMessageStringKey.IsNone()
		? TunaGameInstance->ResolveLocalizedText(Presentation.TransitionMessageStringKey, Presentation.TransitionMessage)
		: Presentation.TransitionMessage;
	if (TunaGameInstance->HasPendingRaidExperienceAnimationState())
	{
		if (UGameInstance* GameInstance = GetGameInstance())
		{
			if (UTunaSweeperRaidExperienceReturnSubsystem* ExperienceReturnSubsystem =
				GameInstance->GetSubsystem<UTunaSweeperRaidExperienceReturnSubsystem>())
			{
				if (ExperienceReturnSubsystem->StartReturnPresentation(
					WorldContextObject,
					TargetLevelName,
					Presentation.TransitionMediaSource,
					Presentation.TransitionWidgetClass,
					Presentation.FadeToBlackDuration,
					Presentation.FadeFromBlackDuration,
					ResolvedTransitionMessage))
				{
					return true;
				}
			}
		}
	}

	if (!Presentation.TransitionMediaSource.IsNull() && !Presentation.TransitionWidgetClass.IsNull())
	{
		if (UGameInstance* GameInstance = GetGameInstance())
		{
			if (UTunaSweeperLevelTransitionSubsystem* TransitionSubsystem =
				GameInstance->GetSubsystem<UTunaSweeperLevelTransitionSubsystem>())
			{
				if (TransitionSubsystem->StartTransition(
					WorldContextObject,
					TargetLevelName,
					Presentation.TransitionMediaSource,
					Presentation.TransitionWidgetClass,
					Presentation.FadeToBlackDuration,
					Presentation.FadeFromBlackDuration,
					ResolvedTransitionMessage))
				{
					return true;
				}
			}
		}
	}

	UGameplayStatics::OpenLevel(WorldContextObject, TargetLevelName);
	return true;
}
