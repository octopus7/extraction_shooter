#include "Interaction/TunaSweeperResearchStationActor.h"

#include "Components/StaticMeshComponent.h"
#include "Interaction/TunaSweeperInteractableComponent.h"

ATunaSweeperResearchStationActor::ATunaSweeperResearchStationActor()
{
	PrimaryActorTick.bCanEverTick = false;

	ConfigureResearchStationDefaults(
		FText::FromString(TEXT("능력치 연구")),
		TSoftClassPtr<UTunaSweeperInteractionMarkerWidget>(
			FSoftObjectPath(TEXT("/Game/UI/WBP_InteractionMarker.WBP_InteractionMarker_C"))));

	if (VisualMesh)
	{
		// A small editor-visible placement puck; the sink mesh supplies the in-game visual.
		VisualMesh->SetRelativeScale3D(FVector(0.32f, 0.32f, 0.08f));
		VisualMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 4.0f));
		VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		VisualMesh->SetHiddenInGame(true);
	}

	if (InteractableComponent)
	{
		InteractableComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 45.0f));
	}
}

void ATunaSweeperResearchStationActor::ConfigureResearchStationDefaults(
	const FText& InInteractionDisplayName,
	TSoftClassPtr<UTunaSweeperInteractionMarkerWidget> InMarkerWidgetClass)
{
	ConfigureInteractionDefaults(
		ETunaSweeperInteractionType::Research,
		InInteractionDisplayName.IsEmpty() ? FText::FromString(TEXT("능력치 연구")) : InInteractionDisplayName,
		InMarkerWidgetClass,
		FName(TEXT("ui.interaction.research")));
}
