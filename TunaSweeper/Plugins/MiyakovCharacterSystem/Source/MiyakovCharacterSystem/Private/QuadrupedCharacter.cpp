// Fill out your copyright notice in the Description page of Project Settings.

#include "QuadrupedCharacter.h"

#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "QuadrupedComponent.h"

AQuadrupedCharacter::AQuadrupedCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	QuadrupedComponent = CreateDefaultSubobject<UQuadrupedComponent>(TEXT("QuadrupedComponent"));

	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCapsuleHalfHeight(40.0f);
		Capsule->SetCapsuleRadius(30.0f);
	}

	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->bOrientRotationToMovement = true;
		Movement->RotationRate = FRotator(0.0f, 400.0f, 0.0f);
		Movement->bUseControllerDesiredRotation = false;
		Movement->MaxWalkSpeed = WalkSpeed;
		Movement->bConstrainToPlane = true;
		Movement->bSnapToPlaneAtStart = true;
	}

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;
}

#if WITH_EDITOR
void AQuadrupedCharacter::PostEditMove(bool bFinished)
{
	Super::PostEditMove(bFinished);

	if (QuadrupedComponent)
	{
		QuadrupedComponent->RefreshLegsInEditor();
	}
}
#endif

void AQuadrupedCharacter::BeginPlay()
{
	Super::BeginPlay();

	ReinitializeLegs();

	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->MaxWalkSpeed = WalkSpeed;
	}
}

void AQuadrupedCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

FVector AQuadrupedCharacter::GetLegIKTarget(int32 LegIndex) const
{
	if (QuadrupedComponent)
	{
		return QuadrupedComponent->GetFootPosition(LegIndex);
	}
	return GetActorLocation();
}

void AQuadrupedCharacter::GetAllLegIKTargets(FVector& OutFrontLeft, FVector& OutFrontRight, FVector& OutBackLeft, FVector& OutBackRight) const
{
	if (QuadrupedComponent)
	{
		OutFrontLeft = QuadrupedComponent->GetFootPosition(0);
		OutFrontRight = QuadrupedComponent->GetFootPosition(1);
		OutBackLeft = QuadrupedComponent->GetFootPosition(2);
		OutBackRight = QuadrupedComponent->GetFootPosition(3);
	}
	else
	{
		const FVector Location = GetActorLocation();
		OutFrontLeft = Location;
		OutFrontRight = Location;
		OutBackLeft = Location;
		OutBackRight = Location;
	}
}

bool AQuadrupedCharacter::IsMoving() const
{
	return GetVelocity().Size2D() > 10.0f;
}

void AQuadrupedCharacter::ReinitializeLegs()
{
	if (QuadrupedComponent)
	{
		QuadrupedComponent->InitializeDefaultLegs(BodyLength, BodyWidth);
	}
}
