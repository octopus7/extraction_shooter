#include "Weapon/TunaSweeperShellCasing.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "TunaSweeperCollisionChannels.h"

namespace
{
	const TCHAR* DefaultShellCasingMeshPath = TEXT("/Game/Weapons/Effects/SM_WeaponShellCasing.SM_WeaponShellCasing");
	const TCHAR* DefaultShellCasingMaterialPath = TEXT("/Game/Weapons/Effects/M_WeaponShellCasing_Brass.M_WeaponShellCasing_Brass");

	FVector MakeSafeHalfExtent(const FVector& HalfExtent)
	{
		return FVector(
			FMath::Max(0.1f, HalfExtent.X),
			FMath::Max(0.1f, HalfExtent.Y),
			FMath::Max(0.1f, HalfExtent.Z));
	}
}

ATunaSweeperShellCasing::ATunaSweeperShellCasing()
{
	PrimaryActorTick.bCanEverTick = false;

	CollisionComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("Collision"));
	CollisionComponent->SetBoxExtent(CollisionHalfExtentCm);
	CollisionComponent->SetNotifyRigidBodyCollision(false);
	CollisionComponent->SetCanEverAffectNavigation(false);
	RootComponent = CollisionComponent;

	CasingMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CasingMesh"));
	CasingMeshComponent->SetupAttachment(RootComponent);
	CasingMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CasingMeshComponent->SetGenerateOverlapEvents(false);
	CasingMeshComponent->SetCanEverAffectNavigation(false);
	CasingMeshComponent->SetCastShadow(true);

	CasingMesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(DefaultShellCasingMeshPath));
	CasingMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(DefaultShellCasingMaterialPath));
}

void ATunaSweeperShellCasing::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	CollisionHalfExtentCm = MakeSafeHalfExtent(CollisionHalfExtentCm);
	MassKg = FMath::Max(0.01f, MassKg);
	LinearDamping = FMath::Max(0.0f, LinearDamping);
	AngularDamping = FMath::Max(0.0f, AngularDamping);
	LifeSeconds = FMath::Max(0.0f, LifeSeconds);

	ApplyCasingDefaults();
}

void ATunaSweeperShellCasing::BeginPlay()
{
	Super::BeginPlay();

	ApplyCasingDefaults();
	if (LifeSeconds > 0.0f && GetWorld() && GetWorld()->IsGameWorld())
	{
		SetLifeSpan(LifeSeconds);
	}
}

void ATunaSweeperShellCasing::LaunchCasing(
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

void ATunaSweeperShellCasing::ApplyCasingDefaults()
{
	if (CollisionComponent)
	{
		CollisionComponent->SetBoxExtent(MakeSafeHalfExtent(CollisionHalfExtentCm), true);
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
		CollisionComponent->SetLinearDamping(LinearDamping);
		CollisionComponent->SetAngularDamping(AngularDamping);
		CollisionComponent->SetMassOverrideInKg(NAME_None, MassKg, true);
		CollisionComponent->SetSimulatePhysics(true);
	}

	if (CasingMeshComponent)
	{
		CasingMeshComponent->SetStaticMesh(CasingMesh.LoadSynchronous());
		CasingMeshComponent->SetRelativeLocation(FVector::ZeroVector);
		CasingMeshComponent->SetRelativeRotation(FRotator::ZeroRotator);
		CasingMeshComponent->SetRelativeScale3D(FVector::OneVector);
		CasingMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		CasingMeshComponent->SetGenerateOverlapEvents(false);
		CasingMeshComponent->SetCanEverAffectNavigation(false);
		CasingMeshComponent->SetHiddenInGame(false);
		CasingMeshComponent->SetVisibility(true, true);
		CasingMeshComponent->SetMaterial(0, CasingMaterial.LoadSynchronous());
	}
}
