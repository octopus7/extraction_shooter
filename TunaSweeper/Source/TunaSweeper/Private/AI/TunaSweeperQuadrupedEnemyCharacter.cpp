#include "AI/TunaSweeperQuadrupedEnemyCharacter.h"

#include "Animation/AnimInstance.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "QuadrupedComponent.h"
#include "UObject/ConstructorHelpers.h"

ATunaSweeperQuadrupedEnemyCharacter::ATunaSweeperQuadrupedEnemyCharacter()
{
	GetCapsuleComponent()->InitCapsuleSize(30.0f, 40.0f);

	if (VisualMesh)
	{
		VisualMesh->SetVisibility(false);
		VisualMesh->SetHiddenInGame(true);
	}

	if (ForwardMarkerMesh)
	{
		ForwardMarkerMesh->SetVisibility(false);
		ForwardMarkerMesh->SetHiddenInGame(true);
	}

	USkeletalMeshComponent* CharacterMesh = GetMesh();
	CharacterMesh->SetVisibility(true);
	CharacterMesh->SetHiddenInGame(false);
	CharacterMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CharacterMesh->SetGenerateOverlapEvents(false);

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> RobotMesh(
		TEXT("/Game/Characters/Robot/SKM_Robot.SKM_Robot"));
	if (RobotMesh.Succeeded())
	{
		CharacterMesh->SetSkeletalMeshAsset(RobotMesh.Object);
	}

	static ConstructorHelpers::FClassFinder<UAnimInstance> RobotAnimBlueprint(
		TEXT("/Game/Characters/Robot/ABP_RobotDog"));
	if (RobotAnimBlueprint.Succeeded())
	{
		CharacterMesh->SetAnimationMode(EAnimationMode::AnimationBlueprint);
		CharacterMesh->SetAnimInstanceClass(RobotAnimBlueprint.Class);
	}

	QuadrupedComponent = CreateDefaultSubobject<UQuadrupedComponent>(TEXT("QuadrupedComponent"));
	QuadrupedComponent->InitializeDefaultLegs(60.0f, 30.0f);
	QuadrupedComponent->LookAheadSeconds = 0.18f;
	QuadrupedComponent->StepThreshold = 42.0f;
	QuadrupedComponent->StepHeight = 26.0f;
	QuadrupedComponent->StepDuration = 0.18f;
	QuadrupedComponent->MaxStepDistance = 100.0f;
	QuadrupedComponent->MaxLegReach = 150.0f;
	QuadrupedComponent->GroundProbeRadius = 6.0f;
	QuadrupedComponent->MinGroundNormalZ = 0.65f;
	QuadrupedComponent->bMoveGaitGroupTogether = false;
}
