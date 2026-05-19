#include "Character/TunaSweeperQuestNpcActor.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Interaction/TunaSweeperInteractableComponent.h"
#include "Subsystem/TunaSweeperQuestSubsystem.h"
#include "UObject/ConstructorHelpers.h"

ATunaSweeperQuestNpcActor::ATunaSweeperQuestNpcActor()
{
	QuestId = UTunaSweeperQuestSubsystem::GetFirstOutingQuestId();
	ProviderId = UTunaSweeperQuestSubsystem::GetInstructorProviderId();
	NpcDisplayName = FText::FromString(TEXT("\uAD50\uAD00"));

	if (VisualMesh)
	{
		VisualMesh->SetRelativeScale3D(FVector(0.7f, 0.7f, 1.8f));

		static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
		if (CylinderMesh.Succeeded())
		{
			VisualMesh->SetStaticMesh(CylinderMesh.Object);
		}
	}

	if (InteractableComponent)
	{
		InteractableComponent->SetInteractionTypeAndDisplayName(
			ETunaSweeperInteractionType::Quest,
			FText::FromString(TEXT("\uD018\uC2A4\uD2B8")));
	}
}

FName ATunaSweeperQuestNpcActor::ResolveQuestId() const
{
	const FName EffectiveProviderId = ProviderId.IsNone()
		? UTunaSweeperQuestSubsystem::GetInstructorProviderId()
		: ProviderId;

	if (UWorld* World = GetWorld())
	{
		if (UGameInstance* GameInstance = World->GetGameInstance())
		{
			if (const UTunaSweeperQuestSubsystem* QuestSubsystem = GameInstance->GetSubsystem<UTunaSweeperQuestSubsystem>())
			{
				FName ResolvedQuestId = NAME_None;
				if (QuestSubsystem->TryResolveQuestForProvider(EffectiveProviderId, QuestId, ResolvedQuestId))
				{
					return ResolvedQuestId;
				}

				return NAME_None;
			}
		}
	}

	return QuestId;
}

void ATunaSweeperQuestNpcActor::ConfigureQuestNpcDefaults(
	FName InQuestId,
	const FText& InNpcDisplayName,
	TSoftClassPtr<UTunaSweeperInteractionMarkerWidget> InMarkerWidgetClass,
	FName InProviderId)
{
	Modify();
	QuestId = InQuestId;
	ProviderId = InProviderId.IsNone() ? UTunaSweeperQuestSubsystem::GetInstructorProviderId() : InProviderId;
	NpcDisplayName = InNpcDisplayName;
	ConfigureInteractionDefaults(
		ETunaSweeperInteractionType::Quest,
		FText::FromString(TEXT("\uD018\uC2A4\uD2B8")),
		InMarkerWidgetClass);
}
