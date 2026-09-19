#include "Vehicle/TunaSweeperATVActor.h"
#include "Vehicle/TunaSweeperVehicleMountComponent.h"
#include "Vehicle/TunaSweeperATVWheel.h"
#include "Vehicle/TunaSweeperATVAnimInstance.h"
#include "Character/TunaSweeperTopDownCharacter.h"
#include "Player/TunaSweeperPlayerController.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "Components/BoxComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Sound/SoundBase.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "TunaSweeperCollisionChannels.h"
#include "UObject/ConstructorHelpers.h"

ATunaSweeperATVActor::ATunaSweeperATVActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;
	AutoPossessAI = EAutoPossessAI::Disabled;
	VehicleMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("VehicleMesh"));
	SetRootComponent(VehicleMesh);
	VehicleMesh->SetCollisionProfileName(TEXT("Vehicle"));
	VehicleMesh->SetSimulatePhysics(true);
	VehicleMesh->BodyInstance.bUseCCD = true;
	VehicleMesh->SetCanEverAffectNavigation(false);
	VehicleMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	VehicleMesh->SetAnimInstanceClass(UTunaSweeperATVAnimInstance::StaticClass());
	ChassisCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("ChassisCollision"));
	ChassisCollision->SetupAttachment(VehicleMesh);
	ChassisCollision->SetRelativeLocation(FVector(0,0,45));
	ChassisCollision->SetBoxExtent(FVector(85.0f, 55.0f, 45.0f));
	ChassisCollision->BodyInstance.bAutoWeld = false;
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> Mesh(TEXT("/Game/Meshes/Props/ATV/SKM_ATV.SKM_ATV"));
	if (Mesh.Succeeded()) VehicleMesh->SetSkeletalMesh(Mesh.Object);
	static ConstructorHelpers::FObjectFinderOptional<UPhysicsAsset> Physics(TEXT("/Game/Meshes/Props/ATV/PA_ATV.PA_ATV"));
	if (Physics.Get()) VehicleMesh->SetPhysicsAsset(Physics.Get());
	VehicleMovement = CreateDefaultSubobject<UChaosWheeledVehicleMovementComponent>(TEXT("VehicleMovement"));
	VehicleMovement->bTickBeforeOwner = false;
	VehicleMovement->SetUpdatedComponent(VehicleMesh);
	// The seated rider retains query collision for damage, but is not terrain.
	VehicleMovement->SetWheelTraceResponseToChannel(ECC_Pawn, ECR_Ignore);
	ConfigureContactCollision();
	VehicleMovement->SetRequiresControllerForInputs(false);
	VehicleMovement->Mass = 340.0f;
	VehicleMovement->bEnableCenterOfMassOverride = true;
	VehicleMovement->CenterOfMassOverride = FVector(-5,0,38);
	VehicleMovement->bReverseAsBrake = true;
	VehicleMovement->bThrottleAsBrake = true;
	VehicleMovement->DifferentialSetup.DifferentialType = EVehicleDifferential::AllWheelDrive;
	VehicleMovement->EngineSetup.MaxTorque = NormalEngineTorque;
	VehicleMovement->EngineSetup.MaxRPM = 6500;
	VehicleMovement->EngineSetup.EngineIdleRPM = 1200;
	auto* Curve = VehicleMovement->EngineSetup.TorqueCurve.GetRichCurve();
	Curve->AddKey(0, 0.5f); Curve->AddKey(2000, 0.8f); Curve->AddKey(4500, 1.0f); Curve->AddKey(6500, 0.75f);
	VehicleMovement->TransmissionSetup.FinalRatio = 4.0f;
	VehicleMovement->TransmissionSetup.ForwardGearRatios = {3.0f, 2.0f, 1.4f};
	VehicleMovement->TransmissionSetup.ReverseGearRatios = {3.0f};
	VehicleMovement->TransmissionSetup.ChangeUpRPM = 5500;
	VehicleMovement->TransmissionSetup.ChangeDownRPM = 2200;
	VehicleMovement->SteeringSetup.SteeringType = ESteeringType::Ackermann;
	auto* SteeringCurve = VehicleMovement->SteeringSetup.SteeringCurve.GetRichCurve();
	SteeringCurve->Reset(); SteeringCurve->AddKey(0, 1); SteeringCurve->AddKey(25, 0.65f); SteeringCurve->AddKey(60, 0.35f);
	const FName Bones[] = {TEXT("wheel_FL"), TEXT("wheel_FR"), TEXT("wheel_RL"), TEXT("wheel_RR")};
	for (int32 Index = 0; Index < 4; ++Index)
	{
		FChaosWheelSetup Wheel;
		Wheel.BoneName = Bones[Index];
		Wheel.WheelClass = Index < 2 ? UTunaSweeperATVFrontWheel::StaticClass() : UTunaSweeperATVRearWheel::StaticClass();
		VehicleMovement->WheelSetups.Add(Wheel);
	}
	MountComponent = CreateDefaultSubobject<UTunaSweeperVehicleMountComponent>(TEXT("MountComponent"));
	MountComponent->SetupAttachment(VehicleMesh, TEXT("seat"));
	static ConstructorHelpers::FObjectFinder<USoundBase> Start(TEXT("/Game/Audio/ATV/SW_ATV_Mount_Start.SW_ATV_Mount_Start"));
	static ConstructorHelpers::FObjectFinder<USoundBase> Idle(TEXT("/Game/Audio/ATV/SW_ATV_Idle_Loop.SW_ATV_Idle_Loop"));
	static ConstructorHelpers::FObjectFinder<USoundBase> Stop(TEXT("/Game/Audio/ATV/SW_ATV_Dismount_Stop.SW_ATV_Dismount_Stop"));
	MountComponent->EngineStartSound = Start.Object;
	MountComponent->EngineIdleSound = Idle.Object;
	MountComponent->EngineStopSound = Stop.Object;
	static ConstructorHelpers::FObjectFinder<USoundBase> Drive(TEXT("/Game/Audio/ATV/SW_ATV_Drive_Loop.SW_ATV_Drive_Loop"));
	static ConstructorHelpers::FObjectFinder<USoundBase> Boost(TEXT("/Game/Audio/ATV/SW_ATV_Boost_Loop.SW_ATV_Boost_Loop"));
	MountComponent->EngineDriveSound = Drive.Object;
	MountComponent->EngineBoostSound = Boost.Object;
	CreateDamageComponents();
}

void ATunaSweeperATVActor::BeginPlay()
{
	Super::BeginPlay();
	if (DestructionExplosion && !DestructionExplosion->GetAsset())
	{
		DestructionExplosion->SetAsset(DestructionExplosionSystem.LoadSynchronous());
	}
	MaxDurability = FMath::IsFinite(MaxDurability) ? FMath::Max(1.0f, MaxDurability) : 300.0f;
	CurrentDurability = MaxDurability;
	SmokeDurabilityRatio = FMath::Clamp(SmokeDurabilityRatio, 0.0f, 1.0f);
	HeavySmokeDurabilityRatio = FMath::Clamp(HeavySmokeDurabilityRatio, 0.0f, SmokeDurabilityRatio);
	// Apply the contact policy to existing saved Blueprint/map instances as well.
	ConfigureContactCollision();
	VehicleMovement->AddTickPrerequisiteActor(this);
	MountComponent->OnMounted.AddDynamic(this, &ATunaSweeperATVActor::HandleRiderChanged);
	MountComponent->OnDismounted.AddDynamic(this, &ATunaSweeperATVActor::HandleRiderChanged);
	ClearDriveInput();
}

void ATunaSweeperATVActor::ConfigureContactCollision()
{
	// CharacterMovement must hit a non-simulating barrier: otherwise its push forces,
	// as well as capsule depenetration in Chaos, move even a handbraked vehicle.
	VehicleMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	ChassisCollision->SetCollisionObjectType(ECC_Vehicle);
	ChassisCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
	ChassisCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	ChassisCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ChassisCollision->CanCharacterStepUpOn = ECB_No;
	// Fast-moving rounds are damage queries, never surfaces supporting the wheels.
	VehicleMovement->SetWheelTraceResponseToChannel(TunaSweeperCollisionChannels::Projectile, ECR_Ignore);
}

UPawnMovementComponent* ATunaSweeperATVActor::GetMovementComponent() const { return VehicleMovement; }

bool ATunaSweeperATVActor::CanAcceptDriveInput() const
{
	if (bVehicleDestroyed) return false;
	const auto* Rider = MountComponent->GetRider();
	if (!IsValid(Rider) || Rider->IsDead()) return false;
	const auto* PC = Cast<ATunaSweeperPlayerController>(Rider->GetController());
	return !PC || (!PC->IsInventoryUiOpen() && !PC->IsPauseMenuOpen() && !PC->IsDialogueSequenceActive() && !PC->IsHousingModeOpen());
}

void ATunaSweeperATVActor::SetDriveInput(const FVector2D& Input)
{
	DriveInput = CanAcceptDriveInput() ? FVector2D(FMath::Clamp(Input.X, -1, 1), FMath::Clamp(Input.Y, -1, 1)) : FVector2D::ZeroVector;
}

void ATunaSweeperATVActor::SetBoostInput(bool bHeld) { bBoostHeld = bHeld && CanAcceptDriveInput(); }

void ATunaSweeperATVActor::ClearDriveInput()
{
	DriveInput = FVector2D::ZeroVector;
	bBoostHeld = false;
	VehicleMovement->SetThrottleInput(0);
	VehicleMovement->SetBrakeInput(0);
	VehicleMovement->SetSteeringInput(0);
	VehicleMovement->SetHandbrakeInput(true);
}

void ATunaSweeperATVActor::HandleRiderChanged(ATunaSweeperTopDownCharacter* Rider) { ClearDriveInput(); }

void ATunaSweeperATVActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateDamageSmoke(DeltaSeconds);
	if (bVehicleDestroyed) return;
	if (!CanAcceptDriveInput())
	{
		ClearDriveInput();
		MountComponent->UpdateDrivingAudio(DeltaSeconds, 0, 0, false);
		return;
	}
	const float Speed = VehicleMovement->GetForwardSpeed();
	const float Limit = DriveInput.Y < 0 ? ReverseTopSpeed : (IsBoosting() ? BoostTopSpeed : NormalTopSpeed);
	// Opposite-direction input always remains available for braking.
	const float Limiter = Speed * DriveInput.Y > 0 ? FMath::Clamp((Limit - FMath::Abs(Speed)) / 100.0f, 0.0f, 1.0f) : 1.0f;
	VehicleMovement->SetMaxEngineTorque(IsBoosting() ? BoostEngineTorque : NormalEngineTorque);
	VehicleMovement->SetThrottleInput(FMath::Max(0.0f, float(DriveInput.Y)) * Limiter);
	VehicleMovement->SetBrakeInput(FMath::Max(0.0f, float(-DriveInput.Y)) * Limiter);
	VehicleMovement->SetSteeringInput(DriveInput.X);
	VehicleMovement->SetHandbrakeInput(false);
	MountComponent->UpdateDrivingAudio(DeltaSeconds, FMath::Abs(Speed), VehicleMovement->GetEngineRotationSpeed() / 6500.0f, IsBoosting());
}
