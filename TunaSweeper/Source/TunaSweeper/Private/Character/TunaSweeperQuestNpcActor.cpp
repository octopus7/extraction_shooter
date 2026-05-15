#include "Character/TunaSweeperQuestNpcActor.h"

#include "Components/StaticMeshComponent.h"
#include "Interaction/TunaSweeperInteractableComponent.h"
#include "Subsystem/TunaSweeperQuestSubsystem.h"
#include "UObject/ConstructorHelpers.h"

ATunaSweeperQuestNpcActor::ATunaSweeperQuestNpcActor()
{
	QuestId = UTunaSweeperQuestSubsystem::GetFirstOutingQuestId();
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

void ATunaSweeperQuestNpcActor::ConfigureQuestNpcDefaults(
	FName InQuestId,
	const FText& InNpcDisplayName,
	TSoftClassPtr<UTunaSweeperInteractionMarkerWidget> InMarkerWidgetClass)
{
	Modify();
	QuestId = InQuestId;
	NpcDisplayName = InNpcDisplayName;
	ConfigureInteractionDefaults(
		ETunaSweeperInteractionType::Quest,
		FText::FromString(TEXT("\uD018\uC2A4\uD2B8")),
		InMarkerWidgetClass);
}
