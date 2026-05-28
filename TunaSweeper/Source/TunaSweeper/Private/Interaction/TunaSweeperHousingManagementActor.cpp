#include "Interaction/TunaSweeperHousingManagementActor.h"

#include "Components/StaticMeshComponent.h"
#include "Interaction/TunaSweeperInteractableComponent.h"

ATunaSweeperHousingManagementActor::ATunaSweeperHousingManagementActor()
{
	ConfigureInteractionDefaults(
		ETunaSweeperInteractionType::HousingManagement,
		FText::FromString(TEXT("Facility Management")),
		TSoftClassPtr<UTunaSweeperInteractionMarkerWidget>(FSoftObjectPath(TEXT("/Game/UI/WBP_InteractionMarker.WBP_InteractionMarker_C"))),
		FName(TEXT("ui.interaction.housing_management")));

	if (VisualMesh)
	{
		VisualMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 55.0f));
		VisualMesh->SetRelativeScale3D(FVector(0.8f, 0.45f, 1.1f));
	}

	if (InteractableComponent)
	{
		InteractableComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 155.0f));
	}
}
