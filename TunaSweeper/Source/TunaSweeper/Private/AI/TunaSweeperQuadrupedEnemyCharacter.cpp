#include "AI/TunaSweeperQuadrupedEnemyCharacter.h"

#include "Animation/AnimInstance.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "QuadrupedComponent.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
bool HasValidQuadrupedLegLayout(const UQuadrupedComponent* Component)
{
	if (!Component || Component->Legs.Num() != 4)
	{
		return false;
	}

	FVector2D MinOffset(TNumericLimits<double>::Max(), TNumericLimits<double>::Max());
	FVector2D MaxOffset(TNumericLimits<double>::Lowest(), TNumericLimits<double>::Lowest());
	for (const FQuadrupedLegData& Leg : Component->Legs)
	{
		if (Leg.DefaultOffset.ContainsNaN())
		{
			return false;
		}

		MinOffset.X = FMath::Min(MinOffset.X, Leg.DefaultOffset.X);
		MinOffset.Y = FMath::Min(MinOffset.Y, Leg.DefaultOffset.Y);
		MaxOffset.X = FMath::Max(MaxOffset.X, Leg.DefaultOffset.X);
		MaxOffset.Y = FMath::Max(MaxOffset.Y, Leg.DefaultOffset.Y);
	}

	return MaxOffset.X - MinOffset.X > 1.0 && MaxOffset.Y - MinOffset.Y > 1.0;
}
}

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
	QuadrupedComponent->LookAheadSeconds = 0.10f;
	QuadrupedComponent->StepThreshold = 30.0f;
	QuadrupedComponent->StepHeight = 26.0f;
	QuadrupedComponent->StepDuration = 0.12f;
	QuadrupedComponent->MaxStepDistance = 80.0f;
	QuadrupedComponent->GroupStepThresholdScale = 0.35f;
	QuadrupedComponent->MaxLegReach = 150.0f;
	QuadrupedComponent->GroundProbeRadius = 6.0f;
	QuadrupedComponent->MinGroundNormalZ = 0.65f;
	QuadrupedComponent->bMoveGaitGroupTogether = true;
	QuadrupedComponent->bLogDiagnostics = true;
}

void ATunaSweeperQuadrupedEnemyCharacter::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	EnsureValidQuadrupedLegLayout();
}

void ATunaSweeperQuadrupedEnemyCharacter::BeginPlay()
{
	EnsureValidQuadrupedLegLayout();
	Super::BeginPlay();
}

void ATunaSweeperQuadrupedEnemyCharacter::EnsureValidQuadrupedLegLayout()
{
	if (QuadrupedComponent && !HasValidQuadrupedLegLayout(QuadrupedComponent))
	{
		QuadrupedComponent->InitializeDefaultLegs(60.0f, 30.0f);
	}
}

#if WITH_EDITOR
void ATunaSweeperQuadrupedEnemyCharacter::PostEditMove(bool bFinished)
{
	Super::PostEditMove(bFinished);

	if (QuadrupedComponent)
	{
		QuadrupedComponent->RefreshLegsInEditor();
	}
}
#endif
