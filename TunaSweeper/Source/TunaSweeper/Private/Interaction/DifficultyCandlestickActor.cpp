#include "Interaction/DifficultyCandlestickActor.h"

#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
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
	// Create the light with the actor so quest refreshes can reach it in any BeginPlay order.
	auto* WickLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("WickLight"));
	WickLight->SetupAttachment(Flame);
	WickLight->SetRelativeLocation(FVector(0, 0, 2.8));
	WickLight->SetMobility(EComponentMobility::Movable);
	WickLight->SetUseInverseSquaredFalloff(false);
	WickLight->SetIntensityUnits(ELightUnits::Unitless);
	WickLight->SetIntensity(0.16f);
	WickLight->SetLightFalloffExponent(2.0f);
	WickLight->SetAttenuationRadius(240.0f);
	WickLight->SetSourceRadius(5.0f);
	WickLight->SetLightColor(FLinearColor(1.0f, 0.50f, 0.20f));
	WickLight->SetCastShadows(false);
	InteractableComponent->SetRelativeLocation(FVector(0.0, 0.0, 32.0));
	ConfigureInteractionDefaults(ETunaSweeperInteractionType::DifficultyAdjustment, FText::GetEmpty(),
		TSoftClassPtr<UTunaSweeperInteractionMarkerWidget>(FSoftObjectPath(TEXT("/Game/UI/WBP_InteractionMarker.WBP_InteractionMarker_C"))),
		TEXT("ui.interaction.difficulty_adjustment"));
}
