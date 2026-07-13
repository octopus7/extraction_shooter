#include "AI/TunaSweeperPetCompanionAIController.h"

#include "AI/TunaSweeperPetCompanionCharacter.h"
#include "Component/TunaSweeperFactionComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "Subsystem/TunaSweeperFactionSubsystem.h"

ATunaSweeperPetCompanionAIController::ATunaSweeperPetCompanionAIController()
{
	PrimaryActorTick.bCanEverTick = true;
	bAttachToPawn = true;
}

void ATunaSweeperPetCompanionAIController::SetGenericTeamId(const FGenericTeamId& InTeamId)
{
	if (PetCharacter && PetCharacter->GetFactionComponent())
	{
		PetCharacter->GetFactionComponent()->SetFactionId(InTeamId.GetId());
	}
}

FGenericTeamId ATunaSweeperPetCompanionAIController::GetGenericTeamId() const
{
	const UTunaSweeperFactionComponent* FactionComponent =
		PetCharacter ? PetCharacter->GetFactionComponent() : nullptr;
	return FactionComponent && TunaSweeperFactionIds::IsValid(FactionComponent->GetFactionId())
		? FGenericTeamId(FactionComponent->GetFactionId())
		: FGenericTeamId::NoTeam;
}

ETeamAttitude::Type ATunaSweeperPetCompanionAIController::GetTeamAttitudeTowards(const AActor& Other) const
{
	const UWorld* World = GetWorld();
	const UTunaSweeperFactionSubsystem* FactionSubsystem =
		World ? World->GetSubsystem<UTunaSweeperFactionSubsystem>() : nullptr;
	if (!FactionSubsystem || !PetCharacter)
	{
		return ETeamAttitude::Neutral;
	}

	switch (FactionSubsystem->GetFactionAttitude(PetCharacter, &Other))
	{
	case ETunaSweeperFactionAttitude::Friendly:
		return ETeamAttitude::Friendly;
	case ETunaSweeperFactionAttitude::Hostile:
		return ETeamAttitude::Hostile;
	default:
		return ETeamAttitude::Neutral;
	}
}

void ATunaSweeperPetCompanionAIController::BeginPlay()
{
	Super::BeginPlay();

	PetCharacter = Cast<ATunaSweeperPetCompanionCharacter>(GetPawn());
}

void ATunaSweeperPetCompanionAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	PetCharacter = Cast<ATunaSweeperPetCompanionCharacter>(InPawn);
	TimeSinceLastUpdate = UpdateInterval;
}

void ATunaSweeperPetCompanionAIController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!PetCharacter)
	{
		return;
	}

	TimeSinceLastUpdate += DeltaSeconds;
	if (TimeSinceLastUpdate >= FMath::Max(0.01f, UpdateInterval))
	{
		TimeSinceLastUpdate = 0.0f;
		UpdateFollowBehavior();
	}
}

void ATunaSweeperPetCompanionAIController::OnFollowTargetChanged()
{
	bIsFollowing = false;
	StopMovement();
	TimeSinceLastUpdate = UpdateInterval;
}

void ATunaSweeperPetCompanionAIController::UpdateFollowBehavior()
{
	if (!PetCharacter || !PetCharacter->HasValidFollowTarget())
	{
		if (bIsFollowing)
		{
			bIsFollowing = false;
			StopMovement();
			ResetPetWalkSpeed();
		}
		return;
	}

	AActor* TargetActor = PetCharacter->GetFollowTarget();
	const float DistanceToTarget = PetCharacter->GetDistanceToFollowTarget();
	if (DistanceToTarget < 0.0f)
	{
		return;
	}

	if (DistanceToTarget > PetCharacter->MaxFollowDistance)
	{
		if (!bIsFollowing || GetMoveStatus() == EPathFollowingStatus::Idle)
		{
			bIsFollowing = true;
			if (UCharacterMovementComponent* MovementComponent = PetCharacter->GetCharacterMovement())
			{
				MovementComponent->MaxWalkSpeed = PetCharacter->PetRunSpeed;
			}

			MoveToActor(
				TargetActor,
				PetCharacter->FollowDistance,
				true,
				true,
				true,
				nullptr,
				true);
		}
		return;
	}

	if (DistanceToTarget < PetCharacter->StopDistance && bIsFollowing)
	{
		bIsFollowing = false;
		StopMovement();
		ResetPetWalkSpeed();
	}
}

void ATunaSweeperPetCompanionAIController::ResetPetWalkSpeed()
{
	if (!PetCharacter)
	{
		return;
	}

	if (UCharacterMovementComponent* MovementComponent = PetCharacter->GetCharacterMovement())
	{
		MovementComponent->MaxWalkSpeed = PetCharacter->PetWalkSpeed;
	}
}

void ATunaSweeperPetCompanionAIController::OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result)
{
	Super::OnMoveCompleted(RequestID, Result);

	bIsFollowing = false;
	ResetPetWalkSpeed();
}
