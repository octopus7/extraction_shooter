#include "AI/TunaSweeperPetCompanionCharacter.h"

#include "AI/TunaSweeperPetCompanionAIController.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

ATunaSweeperPetCompanionCharacter::ATunaSweeperPetCompanionCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	AIControllerClass = ATunaSweeperPetCompanionAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	if (UCapsuleComponent* PetCapsuleComponent = GetCapsuleComponent())
	{
		PetCapsuleComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
		PetCapsuleComponent->SetCanEverAffectNavigation(false);
	}

	if (USkeletalMeshComponent* MeshComponent = GetMesh())
	{
		MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		MeshComponent->SetCanEverAffectNavigation(false);
	}

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->bOrientRotationToMovement = true;
		MovementComponent->RotationRate = FRotator(0.0f, 540.0f, 0.0f);
		MovementComponent->bUseControllerDesiredRotation = false;
		MovementComponent->bConstrainToPlane = true;
		MovementComponent->bSnapToPlaneAtStart = true;
		MovementComponent->MaxWalkSpeed = PetWalkSpeed;
	}
}

void ATunaSweeperPetCompanionCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->MaxWalkSpeed = PetWalkSpeed;
	}
}

void ATunaSweeperPetCompanionCharacter::SetFollowTarget(AActor* NewTarget)
{
	FollowTarget = NewTarget;

	if (ATunaSweeperPetCompanionAIController* PetAIController = Cast<ATunaSweeperPetCompanionAIController>(GetController()))
	{
		PetAIController->OnFollowTargetChanged();
	}
}

float ATunaSweeperPetCompanionCharacter::GetDistanceToFollowTarget() const
{
	if (!HasValidFollowTarget())
	{
		return -1.0f;
	}

	return FVector::Dist(GetActorLocation(), GetFollowTarget()->GetActorLocation());
}

bool ATunaSweeperPetCompanionCharacter::HasValidFollowTarget() const
{
	return FollowTarget.Get() != nullptr && IsValid(FollowTarget.Get());
}
