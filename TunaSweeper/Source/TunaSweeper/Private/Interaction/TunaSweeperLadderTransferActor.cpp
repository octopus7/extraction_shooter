#include "Interaction/TunaSweeperLadderTransferActor.h"

#include "Character/TunaSweeperTopDownCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"

ATunaSweeperLadderTransferActor::ATunaSweeperLadderTransferActor()
{
	ArrivalPoint = CreateDefaultSubobject<USceneComponent>(TEXT("ArrivalPoint"));
	ArrivalPoint->SetupAttachment(RootComponent);
	VisualMesh->SetVisibility(false);
	VisualMesh->SetHiddenInGame(true);
	VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	VisualMesh->SetGenerateOverlapEvents(false);
	ConfigureInteractionDefaults(
		ETunaSweeperInteractionType::LadderTransfer,
		FText::GetEmpty(),
		TSoftClassPtr<UTunaSweeperInteractionMarkerWidget>(
			FSoftObjectPath(TEXT("/Game/UI/WBP_InteractionMarker.WBP_InteractionMarker_C"))),
		FName(TEXT("ui.interaction.ladder_transfer")));
}

void ATunaSweeperLadderTransferActor::SetTargetEndpoint(ATunaSweeperLadderTransferActor* InTargetEndpoint)
{
	TargetEndpoint = InTargetEndpoint;
}

bool ATunaSweeperLadderTransferActor::ResolveTransferDestination(
	APawn* InstigatorPawn, FVector& OutLocation, FRotator& OutRotation) const
{
	const auto* Character = Cast<ATunaSweeperTopDownCharacter>(InstigatorPawn);
	const auto* PlayerController = IsValid(Character) ? Cast<APlayerController>(Character->GetController()) : nullptr;
	const UWorld* World = GetWorld();
	if (!IsValid(Character) || !World || Character->GetWorld() != World ||
		!IsValid(PlayerController) || PlayerController->GetPawn() != Character || Character->IsDead() || Character->IsMountedInVehicle() ||
		IsActorBeingDestroyed() || IsHidden() || !IsValid(ArrivalPoint) ||
		!IsValid(TargetEndpoint) || TargetEndpoint == this || TargetEndpoint->IsActorBeingDestroyed() || TargetEndpoint->IsHidden() ||
		TargetEndpoint->GetWorld() != World || TargetEndpoint->TargetEndpoint != this ||
		!IsValid(TargetEndpoint->ArrivalPoint) || !IsWithinInteractionDistance(Character))
	{
		return false;
	}

	const UCapsuleComponent* Capsule = Character->GetCapsuleComponent();
	const UCharacterMovementComponent* Movement = Character->GetCharacterMovement();
	if (!Capsule || !Movement || !Capsule->IsQueryCollisionEnabled())
	{
		return false;
	}
	float Radius = 0.0f;
	float HalfHeight = 0.0f;
	Capsule->GetScaledCapsuleSize(Radius, HalfHeight);
	const FVector SourceFloor = ArrivalPoint->GetComponentLocation();
	const FVector TargetFloor = TargetEndpoint->ArrivalPoint->GetComponentLocation();
	const float FeetZ = Character->GetActorLocation().Z - HalfHeight;
	if (SourceFloor.ContainsNaN() || TargetFloor.ContainsNaN() ||
		!FMath::IsFinite(MaxSourceHeightDifferenceCm) ||
		!FMath::IsFinite(TargetEndpoint->ArrivalClearanceCm) ||
		FMath::Abs(FeetZ - SourceFloor.Z) > FMath::Max(0.0f, MaxSourceHeightDifferenceCm))
	{
		return false;
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(LadderDestination), false, Character);
	const FCollisionResponseParams ResponseParams(Capsule->GetCollisionResponseToChannels());
	const ECollisionChannel CapsuleChannel = Capsule->GetCollisionObjectType();
	FHitResult Ground;
	// Keep the authored floor authoritative; do not search other floors or ignore the ship hull.
	if (!World->LineTraceSingleByChannel(Ground, TargetFloor + FVector(0, 0, 20),
		TargetFloor - FVector(0, 0, 50), CapsuleChannel, QueryParams, ResponseParams) ||
		!Movement->IsWalkable(Ground))
	{
		return false;
	}
	OutLocation = Ground.ImpactPoint + FVector(0, 0, HalfHeight + FMath::Max(0.0f, TargetEndpoint->ArrivalClearanceCm));
	OutRotation = FRotator(0, TargetEndpoint->ArrivalPoint->GetComponentRotation().Yaw, 0);
	return !OutRotation.ContainsNaN() && !World->OverlapBlockingTestByChannel(
		OutLocation, FQuat::Identity, CapsuleChannel, FCollisionShape::MakeCapsule(Radius, HalfHeight),
		QueryParams, ResponseParams);
}

bool ATunaSweeperLadderTransferActor::CanTransferPlayer(APawn* InstigatorPawn) const
{
	FVector Location;
	FRotator Rotation;
	return ResolveTransferDestination(InstigatorPawn, Location, Rotation);
}

bool ATunaSweeperLadderTransferActor::TryTransferPlayer(APawn* InstigatorPawn)
{
	FVector Location;
	FRotator Rotation;
	if (!ResolveTransferDestination(InstigatorPawn, Location, Rotation) ||
		!InstigatorPawn->TeleportTo(Location, Rotation, false, false))
	{
		return false;
	}

	auto* Character = CastChecked<ATunaSweeperTopDownCharacter>(InstigatorPawn);
	Character->CancelActiveGameplayActions();
	Character->GetCharacterMovement()->StopMovementImmediately();
	Character->ConsumeMovementInputVector();
	if (AController* Controller = Character->GetController())
	{
		Controller->StopMovement();
	}
	return true;
}
