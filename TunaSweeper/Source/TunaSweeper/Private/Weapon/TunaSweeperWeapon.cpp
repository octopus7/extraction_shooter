#include "Weapon/TunaSweeperWeapon.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"
#include "Weapon/TunaSweeperProjectile.h"

namespace TunaSweeperWeaponTags
{
	const FName ShotgunWeaponTypeTag(TEXT("weapon.type.shotgun"));
}

ATunaSweeperWeapon::ATunaSweeperWeapon()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->SetupAttachment(RootComponent);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		WeaponMesh->SetStaticMesh(CubeMesh.Object);
	}
	ConfigureGunVisual();

	MuzzlePoint = CreateDefaultSubobject<USceneComponent>(TEXT("MuzzlePoint"));
	MuzzlePoint->SetupAttachment(RootComponent);
	MuzzlePoint->SetRelativeLocation(FVector(80.0f, 0.0f, 0.0f));

	ProjectileClass = TSoftClassPtr<ATunaSweeperProjectile>(FSoftObjectPath(TEXT("/Game/Weapons/BP_TunaSweeperProjectile.BP_TunaSweeperProjectile_C")));
}

void ATunaSweeperWeapon::ConfigureGunVisual()
{
	if (!WeaponMesh)
	{
		return;
	}

	WeaponMesh->SetRelativeLocation(FVector::ZeroVector);
	WeaponMesh->SetRelativeRotation(FRotator::ZeroRotator);
	WeaponMesh->SetRelativeScale3D(FVector::OneVector);
}

void ATunaSweeperWeapon::ConfigureMeleeVisual()
{
	if (!WeaponMesh)
	{
		return;
	}

	WeaponMesh->SetRelativeLocation(FVector(26.0f, 0.0f, 0.0f));
	WeaponMesh->SetRelativeRotation(FRotator::ZeroRotator);
	WeaponMesh->SetRelativeScale3D(FVector(0.52f, 0.075f, 0.075f));
}

void ATunaSweeperWeapon::SetWeaponMeshOverride(
	UStaticMesh* Mesh,
	UMaterialInterface* Material,
	FVector RelativeLocation,
	FRotator RelativeRotation,
	FVector RelativeScale)
{
	if (!WeaponMesh)
	{
		return;
	}

	if (Mesh)
	{
		WeaponMesh->SetStaticMesh(Mesh);
	}
	if (Material)
	{
		WeaponMesh->SetMaterial(0, Material);
	}
	WeaponMesh->SetRelativeLocation(RelativeLocation);
	WeaponMesh->SetRelativeRotation(RelativeRotation);
	WeaponMesh->SetRelativeScale3D(RelativeScale);
}

void ATunaSweeperWeapon::Fire(
	const FVector& AimDirection,
	APawn* InstigatorPawn,
	FName ProjectileHitEffectId,
	FName WeaponTypeTag)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const float CurrentTime = World->GetTimeSeconds();
	if (CurrentTime - LastFireTimeSeconds < FireCooldown)
	{
		return;
	}

	FVector ShotDirection = AimDirection.GetSafeNormal2D();
	if (ShotDirection.IsNearlyZero())
	{
		ShotDirection = GetActorForwardVector().GetSafeNormal2D();
	}

	TSubclassOf<ATunaSweeperProjectile> LoadedProjectileClass = ProjectileClass.LoadSynchronous();
	if (!LoadedProjectileClass)
	{
		LoadedProjectileClass = ATunaSweeperProjectile::StaticClass();
	}

	if (WeaponTypeTag == TunaSweeperWeaponTags::ShotgunWeaponTypeTag)
	{
		const int32 ProjectileCount = FMath::Max(1, ShotgunProjectileCount);
		const float SpreadHalfAngle = FMath::Max(0.0f, ShotgunSpreadAngleDegrees) * 0.5f;
		for (int32 ProjectileIndex = 0; ProjectileIndex < ProjectileCount; ++ProjectileIndex)
		{
			const float YawOffset = SpreadHalfAngle > 0.0f
				? FMath::FRandRange(-SpreadHalfAngle, SpreadHalfAngle)
				: 0.0f;
			FVector SpreadDirection = ShotDirection.RotateAngleAxis(YawOffset, FVector::UpVector).GetSafeNormal2D();
			if (SpreadDirection.IsNearlyZero())
			{
				SpreadDirection = ShotDirection;
			}
			SpawnProjectile(*World, LoadedProjectileClass, SpreadDirection, InstigatorPawn, ProjectileHitEffectId);
		}
	}
	else
	{
		SpawnProjectile(*World, LoadedProjectileClass, ShotDirection, InstigatorPawn, ProjectileHitEffectId);
	}

	LastFireTimeSeconds = CurrentTime;
}

ATunaSweeperProjectile* ATunaSweeperWeapon::SpawnProjectile(
	UWorld& World,
	TSubclassOf<ATunaSweeperProjectile> ProjectileClassToSpawn,
	const FVector& ShotDirection,
	APawn* InstigatorPawn,
	FName ProjectileHitEffectId)
{
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = this;
	SpawnParameters.Instigator = InstigatorPawn;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	const FVector SpawnLocation = MuzzlePoint ? MuzzlePoint->GetComponentLocation() : GetActorLocation();
	const FRotator SpawnRotation = ShotDirection.Rotation();
	ATunaSweeperProjectile* SpawnedProjectile =
		World.SpawnActor<ATunaSweeperProjectile>(ProjectileClassToSpawn, SpawnLocation, SpawnRotation, SpawnParameters);
	if (SpawnedProjectile)
	{
		SpawnedProjectile->SetHitEffectId(ProjectileHitEffectId);
	}

	return SpawnedProjectile;
}
