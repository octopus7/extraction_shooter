#include "Interaction/TunaSweeperPhysicsCrateFragmentActor.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "TunaSweeperCollisionChannels.h"

namespace
{
	const TCHAR* DefaultFragmentMeshPath = TEXT("/Engine/BasicShapes/Cube.Cube");

	FVector MakeSafeHalfExtent(const FVector& HalfExtent)
	{
		return FVector(
			FMath::Max(0.5f, HalfExtent.X),
			FMath::Max(0.5f, HalfExtent.Y),
			FMath::Max(0.5f, HalfExtent.Z));
	}
}

ATunaSweeperPhysicsCrateFragmentActor::ATunaSweeperPhysicsCrateFragmentActor()
{
	PrimaryActorTick.bCanEverTick = false;

	CollisionComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("Collision"));
	CollisionComponent->SetBoxExtent(HalfExtentCm);
	CollisionComponent->SetNotifyRigidBodyCollision(false);
	CollisionComponent->SetCanEverAffectNavigation(false);
	RootComponent = CollisionComponent;

	FragmentMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FragmentMesh"));
	FragmentMeshComponent->SetupAttachment(RootComponent);
	FragmentMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FragmentMeshComponent->SetGenerateOverlapEvents(false);
	FragmentMeshComponent->SetCanEverAffectNavigation(false);
	FragmentMeshComponent->SetCastShadow(true);

	FragmentMesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(DefaultFragmentMeshPath));
}

void ATunaSweeperPhysicsCrateFragmentActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	HalfExtentCm = MakeSafeHalfExtent(HalfExtentCm);
	MassKg = FMath::Max(0.01f, MassKg);
	LinearDamping = FMath::Max(0.0f, LinearDamping);
	AngularDamping = FMath::Max(0.0f, AngularDamping);
	LifeSeconds = FMath::Max(0.0f, LifeSeconds);

	ApplyFragmentDefaults();
}

void ATunaSweeperPhysicsCrateFragmentActor::BeginPlay()
{
	Super::BeginPlay();

	ApplyFragmentDefaults();
	if (LifeSeconds > 0.0f)
	{
		SetLifeSpan(LifeSeconds);
	}
}

void ATunaSweeperPhysicsCrateFragmentActor::ConfigureCrateFragmentDefaults(
	const TSoftObjectPtr<UStaticMesh>& InFragmentMesh,
	UMaterialInterface* InFragmentMaterial,
	const FVector& InHalfExtentCm,
	float InLifeSeconds)
{
	if (!InFragmentMesh.IsNull())
	{
		FragmentMesh = InFragmentMesh;
	}
	FragmentMaterial = InFragmentMaterial;
	HalfExtentCm = MakeSafeHalfExtent(InHalfExtentCm);
	LifeSeconds = FMath::Max(0.0f, InLifeSeconds);

	ApplyFragmentDefaults();
	if (LifeSeconds > 0.0f && GetWorld() && GetWorld()->IsGameWorld())
	{
		SetLifeSpan(LifeSeconds);
	}
}

void ATunaSweeperPhysicsCrateFragmentActor::LaunchFragment(
	const FVector& LinearVelocityCmPerSecond,
	const FVector& AngularVelocityDegreesPerSecond)
{
	if (!CollisionComponent)
	{
		return;
	}

	CollisionComponent->SetPhysicsLinearVelocity(LinearVelocityCmPerSecond);
	CollisionComponent->SetPhysicsAngularVelocityInDegrees(AngularVelocityDegreesPerSecond);
}

void ATunaSweeperPhysicsCrateFragmentActor::ApplyFragmentDefaults()
{
	const FVector SafeHalfExtent = MakeSafeHalfExtent(HalfExtentCm);

	if (CollisionComponent)
	{
		CollisionComponent->SetBoxExtent(SafeHalfExtent, true);
		CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		CollisionComponent->SetCollisionObjectType(ECC_PhysicsBody);
		CollisionComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
		CollisionComponent->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
		CollisionComponent->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
		CollisionComponent->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Block);
		CollisionComponent->SetCollisionResponseToChannel(TunaSweeperCollisionChannels::VisionOccluder, ECR_Ignore);
		CollisionComponent->SetCollisionResponseToChannel(TunaSweeperCollisionChannels::Projectile, ECR_Ignore);
		CollisionComponent->SetGenerateOverlapEvents(false);
		CollisionComponent->CanCharacterStepUpOn = ECB_No;
		CollisionComponent->SetLinearDamping(FMath::Max(0.0f, LinearDamping));
		CollisionComponent->SetAngularDamping(FMath::Max(0.0f, AngularDamping));
		CollisionComponent->SetMassOverrideInKg(NAME_None, FMath::Max(0.01f, MassKg), true);
		CollisionComponent->SetSimulatePhysics(true);
	}

	if (FragmentMeshComponent)
	{
		FragmentMeshComponent->SetStaticMesh(FragmentMesh.LoadSynchronous());
		FragmentMeshComponent->SetRelativeLocation(FVector::ZeroVector);
		FragmentMeshComponent->SetRelativeRotation(FRotator::ZeroRotator);
		FragmentMeshComponent->SetRelativeScale3D(SafeHalfExtent / 50.0f);
		FragmentMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		FragmentMeshComponent->SetGenerateOverlapEvents(false);
		FragmentMeshComponent->SetCanEverAffectNavigation(false);
		FragmentMeshComponent->SetHiddenInGame(false);
		FragmentMeshComponent->SetVisibility(true, true);
		if (FragmentMaterial)
		{
			FragmentMeshComponent->SetMaterial(0, FragmentMaterial);
		}
	}
}
