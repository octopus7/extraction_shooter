#include "Character/TunaSweeperGazeTestRobotCharacter.h"

#include "Component/TunaSweeperGazeSkeletalMeshComponent.h"
#include "Component/TunaSweeperGazeTrackingComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

ATunaSweeperGazeTestRobotCharacter::ATunaSweeperGazeTestRobotCharacter(
	const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UTunaSweeperGazeSkeletalMeshComponent>(ACharacter::MeshComponentName))
{
	PrimaryActorTick.bCanEverTick = true;
	SetCanBeDamaged(false);
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCapsuleComponent()->InitCapsuleSize(46.0f, 105.0f);
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->GravityScale = 0.0f;
		Movement->DefaultLandMovementMode = MOVE_None;
		Movement->DefaultWaterMovementMode = MOVE_None;
		Movement->DisableMovement();
	}
	GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -105.0f));
	GetMesh()->SetRelativeRotation(FRotator::ZeroRotator);
	// The Blender source is authored in metres, but this minimal FBX imports its
	// raw geometry in centimetre-sized units. Scale the complete skeletal mesh
	// component so the 1.78-unit rig appears as a 178 cm test character while
	// preserving its stable six-bone import and skin binding.
	GetMesh()->SetRelativeScale3D(FVector(100.0f));
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetGenerateOverlapEvents(false);
	GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> RobotMeshFinder(
		TEXT("/Game/Characters/Test/GazeRobot/SKM_GazeTestRobot.SKM_GazeTestRobot"));
	if (RobotMeshFinder.Succeeded())
	{
		GetMesh()->SetSkeletalMeshAsset(RobotMeshFinder.Object);
	}

	GazeTracking = CreateDefaultSubobject<UTunaSweeperGazeTrackingComponent>(TEXT("GazeTracking"));
	GazeTracking->SetupAttachment(GetRootComponent());
	GazeTracking->AddTickPrerequisiteActor(this);

	LeftEyeTarget = CreateDefaultSubobject<USceneComponent>(TEXT("LeftEyeTarget"));
	LeftEyeTarget->SetupAttachment(GazeTracking);
	LeftEyeTarget->SetRelativeLocation(FVector(0.0f, -20.0f, 0.0f));

	RightEyeTarget = CreateDefaultSubobject<USceneComponent>(TEXT("RightEyeTarget"));
	RightEyeTarget->SetupAttachment(GazeTracking);
	RightEyeTarget->SetRelativeLocation(FVector(0.0f, 20.0f, 0.0f));

	ConfigureGazeComponents();
}

void ATunaSweeperGazeTestRobotCharacter::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ConfigureGazeComponents();
}

void ATunaSweeperGazeTestRobotCharacter::BeginPlay()
{
	Super::BeginPlay();
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
		Movement->DisableMovement();
	}
	ConfigureGazeComponents();
}

void ATunaSweeperGazeTestRobotCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bTrackMouseCursor)
	{
		UpdateMouseGaze();
	}
}

void ATunaSweeperGazeTestRobotCharacter::ConfigureGazeComponents()
{
	if (!GazeTracking)
	{
		return;
	}

	GazeTracking->SetTrackedMesh(GetMesh());
	GazeTracking->SetEyeTargetComponents(LeftEyeTarget, RightEyeTarget);
	GazeTracking->SetEyeBoneNames(TEXT("left_eye"), TEXT("right_eye"));
	// The imported pupils face the mesh component's local +Y direction. The
	// eye-bone basis maps local -Y to that visual forward direction.
	GazeTracking->SetEyeAxes(-FVector::RightVector, FVector::UpVector);
	GazeTracking->SetGazeEnabled(true);
}

FVector ATunaSweeperGazeTestRobotCharacter::CalculateCursorTargetWorldLocation(
	const FVector& CursorRayOrigin,
	const FVector& CursorRayDirection,
	const FVector& EyeCenterWorldLocation,
	float TargetFrontOffset,
	float MinimumRayDistance)
{
	const FVector NormalizedDirection = CursorRayDirection.GetSafeNormal();
	if (NormalizedDirection.IsNearlyZero())
	{
		return CursorRayOrigin;
	}

	const float EyeRayDistance = FVector::DotProduct(
		EyeCenterWorldLocation - CursorRayOrigin,
		NormalizedDirection);
	const float TargetRayDistance = FMath::Max(
		FMath::Max(1.0f, MinimumRayDistance),
		EyeRayDistance - FMath::Max(0.0f, TargetFrontOffset));
	return CursorRayOrigin + NormalizedDirection * TargetRayDistance;
}

void ATunaSweeperGazeTestRobotCharacter::UpdateMouseGaze()
{
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	if (!PlayerController || !GazeTracking)
	{
		return;
	}

	FVector MouseWorldOrigin = FVector::ZeroVector;
	FVector MouseWorldDirection = FVector::ForwardVector;
	if (!PlayerController->DeprojectMousePositionToWorld(MouseWorldOrigin, MouseWorldDirection))
	{
		return;
	}

	const FVector NormalizedDirection = MouseWorldDirection.GetSafeNormal();
	FVector EyeCenterWorldLocation = GetMesh()->GetComponentLocation();
	const int32 LeftEyeBoneIndex = GetMesh()->GetBoneIndex(TEXT("left_eye"));
	const int32 RightEyeBoneIndex = GetMesh()->GetBoneIndex(TEXT("right_eye"));
	if (LeftEyeBoneIndex != INDEX_NONE && RightEyeBoneIndex != INDEX_NONE)
	{
		EyeCenterWorldLocation =
			(GetMesh()->GetBoneLocation(TEXT("left_eye")) + GetMesh()->GetBoneLocation(TEXT("right_eye"))) * 0.5f;
	}
	const FVector TargetLocation = CalculateCursorTargetWorldLocation(
		MouseWorldOrigin,
		NormalizedDirection,
		EyeCenterWorldLocation,
		CursorTargetFrontOffset,
		CursorTargetMinimumRayDistance);
	GazeTracking->SetGazeTargetWorldTransform(FTransform(NormalizedDirection.Rotation(), TargetLocation));
}
