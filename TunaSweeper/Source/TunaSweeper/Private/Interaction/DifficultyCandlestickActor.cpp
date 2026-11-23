#include "Interaction/DifficultyCandlestickActor.h"

#include "Components/StaticMeshComponent.h"
#include "Effect/CandlestickFlameComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

ADifficultyCandlestickActor::ADifficultyCandlestickActor()
{
	static ConstructorHelpers::FObjectFinderOptional<UStaticMesh> Candlestick(
		TEXT("/Game/Interaction/Difficulty/SM_BedsideCandlestick.SM_BedsideCandlestick"));
	VisualMesh->SetStaticMesh(Candlestick.Get());
	VisualMesh->SetRelativeTransform(FTransform::Identity);
	VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	VisualMesh->SetCanEverAffectNavigation(false);
	Flame = CreateDefaultSubobject<UCandlestickFlameComponent>(TEXT("Flame"));
	Flame->SetupAttachment(VisualMesh);
	Flame->SetRelativeLocation(FVector(0.0, 0.0, 20.8));
	InteractableComponent->SetRelativeLocation(FVector(0.0, 0.0, 32.0));
	ConfigureInteractionDefaults(ETunaSweeperInteractionType::DifficultyAdjustment, FText::GetEmpty(),
		TSoftClassPtr<UTunaSweeperInteractionMarkerWidget>(FSoftObjectPath(TEXT("/Game/UI/WBP_InteractionMarker.WBP_InteractionMarker_C"))),
		TEXT("ui.interaction.difficulty_adjustment"));
}
