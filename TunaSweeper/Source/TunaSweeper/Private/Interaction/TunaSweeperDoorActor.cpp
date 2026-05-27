#include "Interaction/TunaSweeperDoorActor.h"

#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Interaction/TunaSweeperInteractableComponent.h"
#include "TunaSweeperCollisionChannels.h"
#include "UObject/ConstructorHelpers.h"

ATunaSweeperDoorActor::ATunaSweeperDoorActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	HingePivot = CreateDefaultSubobject<USceneComponent>(TEXT("HingePivot"));
	HingePivot->SetupAttachment(RootComponent);

	DoorBodyCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("DoorBodyCollision"));
	DoorBodyCollision->SetupAttachment(HingePivot);
	DoorBodyCollision->SetHiddenInGame(true);
	DoorBodyCollision->SetVisibility(false);
	DoorBodyCollision->SetCanEverAffectNavigation(true);

	VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
	VisualMesh->SetupAttachment(HingePivot);
	VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		VisualMesh->SetStaticMesh(CubeMesh.Object);
	}

	InteractableComponent = CreateDefaultSubobject<UTunaSweeperInteractableComponent>(TEXT("Interactable"));
	InteractableComponent->SetupAttachment(RootComponent);

	ApplyComponentDefaults();
	ApplyDoorState(true);
}

void ATunaSweeperDoorActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	bOpen = bStartsOpen;
	ApplyComponentDefaults();
	ApplyDoorState(true);
}

void ATunaSweeperDoorActor::BeginPlay()
{
	Super::BeginPlay();

	bOpen = bStartsOpen;
	ApplyComponentDefaults();
	ApplyDoorState(true);
}

void ATunaSweeperDoorActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!HingePivot)
	{
		SetActorTickEnabled(false);
		return;
	}

	const FRotator TargetRotation = GetTargetHingeRotation();
	const FRotator CurrentRotation = HingePivot->GetRelativeRotation();
	const FRotator NewRotation = FMath::RInterpTo(
		CurrentRotation,
		TargetRotation,
		DeltaSeconds,
		FMath::Max(0.0f, RotationInterpSpeed));

	HingePivot->SetRelativeRotation(NewRotation);
	if (NewRotation.Equals(TargetRotation, RotationSnapToleranceDegrees))
	{
		HingePivot->SetRelativeRotation(TargetRotation);
		SetActorTickEnabled(false);
	}
}

bool ATunaSweeperDoorActor::ToggleDoor()
{
	SetDoorOpen(!bOpen, false);
	return true;
}

void ATunaSweeperDoorActor::SetDoorOpen(bool bInOpen, bool bInstant)
{
	if (bOpen == bInOpen && (!HingePivot || HingePivot->GetRelativeRotation().Equals(GetTargetHingeRotation(), RotationSnapToleranceDegrees)))
	{
		return;
	}

	bOpen = bInOpen;
	ApplyDoorState(bInstant);
}

void ATunaSweeperDoorActor::ApplyComponentDefaults()
{
	if (DoorBodyCollision)
	{
		DoorBodyCollision->SetRelativeLocation(DoorBodyRelativeLocation);
		DoorBodyCollision->SetBoxExtent(FVector(
			FMath::Max(1.0f, DoorBodyExtent.X),
			FMath::Max(1.0f, DoorBodyExtent.Y),
			FMath::Max(1.0f, DoorBodyExtent.Z)));
		DoorBodyCollision->SetCollisionEnabled(
			bEnableDoorBodyCollision ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
		DoorBodyCollision->SetCollisionObjectType(ECC_WorldStatic);
		DoorBodyCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
		DoorBodyCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
		DoorBodyCollision->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
		DoorBodyCollision->SetCollisionResponseToChannel(TunaSweeperCollisionChannels::Projectile, ECR_Block);
		DoorBodyCollision->SetCollisionResponseToChannel(TunaSweeperCollisionChannels::VisionOccluder, ECR_Block);
		DoorBodyCollision->SetGenerateOverlapEvents(false);
		DoorBodyCollision->CanCharacterStepUpOn = ECB_No;
		DoorBodyCollision->SetHiddenInGame(true);
		DoorBodyCollision->SetVisibility(false);
	}

	if (VisualMesh)
	{
		VisualMesh->SetRelativeLocation(DoorMeshRelativeLocation);
		VisualMesh->SetRelativeScale3D(FVector(
			FMath::Max(0.01f, DoorMeshRelativeScale.X),
			FMath::Max(0.01f, DoorMeshRelativeScale.Y),
			FMath::Max(0.01f, DoorMeshRelativeScale.Z)));
		VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (InteractableComponent)
	{
		InteractableComponent->SetRelativeLocation(InteractableRelativeLocation);
		InteractableComponent->SetInteractionTypeAndDisplayName(
			ETunaSweeperInteractionType::DoorOpen,
			InteractionDisplayName);
	}
}

void ATunaSweeperDoorActor::ApplyDoorState(bool bInstant)
{
	if (!HingePivot)
	{
		return;
	}

	const FRotator TargetRotation = GetTargetHingeRotation();
	if (bInstant || RotationInterpSpeed <= 0.0f)
	{
		HingePivot->SetRelativeRotation(TargetRotation);
		SetActorTickEnabled(false);
		return;
	}

	SetActorTickEnabled(true);
}

FRotator ATunaSweeperDoorActor::GetTargetHingeRotation() const
{
	return bOpen ? OpenHingeRelativeRotation : ClosedHingeRelativeRotation;
}
