#include "Interaction/TunaSweeperQuestLightController.h"

#include "Components/LightComponentBase.h"
#include "Components/SceneComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Subsystem/TunaSweeperQuestSubsystem.h"

ATunaSweeperQuestLightController::ATunaSweeperQuestLightController()
{
	PrimaryActorTick.bCanEverTick = false;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot")));
}

void ATunaSweeperQuestLightController::BeginPlay()
{
	Super::BeginPlay();
	if (auto* Game = GetGameInstance())
		if (auto* Quests = Game->GetSubsystem<UTunaSweeperQuestSubsystem>())
			Quests->OnQuestProgressChanged.AddUObject(this, &ThisClass::RefreshQuestLights);
	RefreshQuestLights();
}

void ATunaSweeperQuestLightController::EndPlay(const EEndPlayReason::Type Reason)
{
	if (auto* Game = GetGameInstance())
		if (auto* Quests = Game->GetSubsystem<UTunaSweeperQuestSubsystem>())
			Quests->OnQuestProgressChanged.RemoveAll(this);
	Super::EndPlay(Reason);
}

void ATunaSweeperQuestLightController::RefreshQuestLights()
{
	// Never change other actors in editor construction or mutate baked lighting.
	if (!GetWorld() || !GetWorld()->IsGameWorld()) return;
	const auto* Game = GetGameInstance();
	const auto* Quests = Game ? Game->GetSubsystem<UTunaSweeperQuestSubsystem>() : nullptr;
	for (const auto& Group : LightGroups)
	{
		const bool bComplete = Quests && Group.CompletedQuestId > 0 &&
			Quests->FindQuestDefinition(Group.CompletedQuestId) &&
			Quests->GetQuestState(Group.CompletedQuestId) == ETunaSweeperQuestState::RewardCompleted;
		const bool bVisible = bComplete ? Group.bVisibleAfterCompletion : Group.bVisibleBeforeCompletion;
		for (AActor* Actor : Group.LightActors)
		{
			if (!IsValid(Actor) || Actor->GetWorld() != GetWorld()) continue;
			TInlineComponentArray<ULightComponentBase*> Lights(Actor);
			for (auto* Light : Lights)
			{
				if (Light->Mobility == EComponentMobility::Static) continue;
				Light->SetVisibility(bVisible);
			}
		}
	}
}
