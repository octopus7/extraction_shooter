#include "Interaction/TunaSweeperWorkbenchActor.h"

#include "Components/StaticMeshComponent.h"

ATunaSweeperWorkbenchActor::ATunaSweeperWorkbenchActor()
{
	const TSoftClassPtr<UTunaSweeperInteractionMarkerWidget> MarkerWidgetClass(
		FSoftObjectPath(TEXT("/Game/UI/WBP_InteractionMarker.WBP_InteractionMarker_C")));

	ConfigureInteractionDefaults(
		ETunaSweeperInteractionType::WorkbenchCraft,
		FText::FromString(TEXT("\uC81C\uC870")),
		MarkerWidgetClass,
		FName(TEXT("ui.interaction.workbench_craft")));

	if (InteractableComponent)
	{
		InteractableComponent->SetRelativeLocation(FVector(-82.0f, 0.0f, 120.0f));
	}

	DismantleInteractableComponent = CreateDefaultSubobject<UTunaSweeperInteractableComponent>(TEXT("DismantleInteractable"));
	DismantleInteractableComponent->SetupAttachment(RootComponent);
	DismantleInteractableComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 120.0f));
	DismantleInteractableComponent->ConfigureInteractionDefaults(
		ETunaSweeperInteractionType::WorkbenchDismantle,
		FText::FromString(TEXT("\uBD84\uD574")),
		MarkerWidgetClass,
		FName(TEXT("ui.interaction.workbench_dismantle")));

	BlueprintRegisterInteractableComponent = CreateDefaultSubobject<UTunaSweeperInteractableComponent>(TEXT("BlueprintRegisterInteractable"));
	BlueprintRegisterInteractableComponent->SetupAttachment(RootComponent);
	BlueprintRegisterInteractableComponent->SetRelativeLocation(FVector(82.0f, 0.0f, 120.0f));
	BlueprintRegisterInteractableComponent->ConfigureInteractionDefaults(
		ETunaSweeperInteractionType::WorkbenchBlueprintRegister,
		FText::FromString(TEXT("\uC124\uACC4\uB3C4 \uB4F1\uB85D")),
		MarkerWidgetClass,
		FName(TEXT("ui.interaction.workbench_blueprint_register")));

	if (VisualMesh)
	{
		VisualMesh->SetRelativeScale3D(FVector(1.35f, 0.7f, 0.55f));
		VisualMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 45.0f));
	}
}

void ATunaSweeperWorkbenchActor::SetWorkbenchId(int32 InWorkbenchId)
{
	WorkbenchId = FMath::Max(1, InWorkbenchId);
}

void ATunaSweeperWorkbenchActor::ConfigureWorkbenchDefaults(int32 InWorkbenchId)
{
	SetWorkbenchId(InWorkbenchId);
}
