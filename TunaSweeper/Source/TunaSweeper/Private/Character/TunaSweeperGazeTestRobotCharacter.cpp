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
	GazeTracking->SetEyeAxes(-FVector::ForwardVector, FVector::UpVector);
	GazeTracking->SetGazeEnabled(true);
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
	const FVector TargetLocation = MouseWorldOrigin + NormalizedDirection * CursorTargetDistance;
	GazeTracking->SetGazeTargetWorldTransform(FTransform(NormalizedDirection.Rotation(), TargetLocation));
}
