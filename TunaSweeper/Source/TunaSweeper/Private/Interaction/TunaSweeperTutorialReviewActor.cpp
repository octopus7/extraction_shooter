#include "Interaction/TunaSweeperTutorialReviewActor.h"
#include "Components/StaticMeshComponent.h"

ATunaSweeperTutorialReviewActor::ATunaSweeperTutorialReviewActor()
{
    ConfigureInteractionDefaults(ETunaSweeperInteractionType::TutorialReview, FText::GetEmpty(),
        TSoftClassPtr<UTunaSweeperInteractionMarkerWidget>(FSoftObjectPath(TEXT("/Game/UI/WBP_InteractionMarker.WBP_InteractionMarker_C"))),
        TEXT("ui.interaction.tutorial_review"));
    VisualMesh->SetHiddenInGame(true);
    VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    VisualMesh->SetRelativeScale3D(FVector(0.3, 0.3, 0.08));
    InteractableComponent->SetRelativeLocation(FVector(0, 0, 30));
}
