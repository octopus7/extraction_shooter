#include "Interaction/TunaSweeperInteractableActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

ATunaSweeperInteractableActor::ATunaSweeperInteractableActor()
{
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
	VisualMesh->SetupAttachment(RootComponent);
	VisualMesh->SetRelativeScale3D(FVector(0.75f, 0.75f, 0.75f));
	VisualMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		VisualMesh->SetStaticMesh(CubeMesh.Object);
	}

	InteractableComponent = CreateDefaultSubobject<UTunaSweeperInteractableComponent>(TEXT("Interactable"));
	InteractableComponent->SetupAttachment(RootComponent);
	InteractableComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 120.0f));
}

ETunaSweeperInteractionType ATunaSweeperInteractableActor::GetInteractionType() const
{
	return InteractableComponent
		? InteractableComponent->GetInteractionType()
		: ETunaSweeperInteractionType::Dialogue;
}

FText ATunaSweeperInteractableActor::GetInteractionDisplayName() const
{
	return InteractableComponent
		? InteractableComponent->GetInteractionDisplayName()
		: FText::GetEmpty();
}

float ATunaSweeperInteractableActor::GetInteractionDistance() const
{
	return InteractableComponent ? InteractableComponent->GetInteractionDistance() : 0.0f;
}

bool ATunaSweeperInteractableActor::IsWithinInteractionDistance(const AActor* OtherActor) const
{
	return InteractableComponent && InteractableComponent->IsWithinInteractionDistance(OtherActor);
}

float ATunaSweeperInteractableActor::GetSquaredDistance2DTo(const AActor* OtherActor) const
{
	return InteractableComponent
		? InteractableComponent->GetSquaredDistance2DTo(OtherActor)
		: TNumericLimits<float>::Max();
}

bool ATunaSweeperInteractableActor::RequestInteraction(APawn* InstigatorPawn)
{
	return InteractableComponent && InteractableComponent->RequestInteraction(InstigatorPawn);
}

void ATunaSweeperInteractableActor::ConfigureInteractionDefaults(
	ETunaSweeperInteractionType InInteractionType,
	const FText& InInteractionDisplayName,
	TSoftClassPtr<UTunaSweeperInteractionMarkerWidget> InMarkerWidgetClass)
{
	if (InteractableComponent)
	{
		InteractableComponent->ConfigureInteractionDefaults(InInteractionType, InInteractionDisplayName, InMarkerWidgetClass);
	}
}
