#include "Interaction/TunaSweeperDifficultyAdjustmentActor.h"

#include "Components/StaticMeshComponent.h"

ATunaSweeperDifficultyAdjustmentActor::ATunaSweeperDifficultyAdjustmentActor()
{
	ConfigureDifficultyAdjustmentDefaults(
		FText::FromString(TEXT("\uB09C\uC774\uB3C4 \uC870\uC815")),
		TSoftClassPtr<UTunaSweeperInteractionMarkerWidget>(
			FSoftObjectPath(TEXT("/Game/UI/WBP_InteractionMarker.WBP_InteractionMarker_C"))),
		FName(TEXT("ui.interaction.difficulty_adjustment")));

	if (VisualMesh)
	{
		VisualMesh->SetRelativeScale3D(FVector(0.72f, 0.36f, 0.92f));
		VisualMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 46.0f));
	}

	if (InteractableComponent)
	{
		InteractableComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 130.0f));
	}
}

void ATunaSweeperDifficultyAdjustmentActor::ConfigureDifficultyAdjustmentDefaults(
	const FText& InInteractionDisplayName,
	TSoftClassPtr<UTunaSweeperInteractionMarkerWidget> InMarkerWidgetClass,
	FName InInteractionDisplayNameStringKey)
{
	ConfigureInteractionDefaults(
		ETunaSweeperInteractionType::DifficultyAdjustment,
		InInteractionDisplayName,
		InMarkerWidgetClass,
		InInteractionDisplayNameStringKey.IsNone()
			? FName(TEXT("ui.interaction.difficulty_adjustment"))
			: InInteractionDisplayNameStringKey);
}
