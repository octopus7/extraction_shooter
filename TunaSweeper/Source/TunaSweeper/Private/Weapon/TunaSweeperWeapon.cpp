#include "Weapon/TunaSweeperWeapon.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Interaction/TunaSweeperSandbagCoverActor.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"
#include "Weapon/TunaSweeperProjectile.h"

namespace TunaSweeperWeaponTags
{
	const FName ShotgunWeaponTypeTag(TEXT("weapon.type.shotgun"));
}

namespace
{
	FVector ApplyRandomConeSpread(const FVector& Direction, float SpreadHalfAngleDegrees)
	{
		const FVector SafeDirection = Direction.GetSafeNormal();
		if (SafeDirection.IsNearlyZero())
		{
			return FVector::ForwardVector;
		}

		const float SafeSpreadDegrees = FMath::Max(0.0f, SpreadHalfAngleDegrees);
		if (SafeSpreadDegrees <= KINDA_SMALL_NUMBER)
		{
			return SafeDirection;
		}

		return FMath::VRandCone(SafeDirection, FMath::DegreesToRadians(SafeSpreadDegrees)).GetSafeNormal();
	}

	void IgnoreNearbyPlayerPassthroughCovers(ATunaSweeperProjectile* Projectile, APawn* InstigatorPawn)
	{
		if (!Projectile || !InstigatorPawn || !InstigatorPawn->IsPlayerControlled())
		{
			return;
		}

		UWorld* World = Projectile->GetWorld();
		if (!World)
		{
			return;
		}

		for (TActorIterator<ATunaSweeperSandbagCoverActor> It(World); It; ++It)
		{
			ATunaSweeperSandbagCoverActor* SandbagCover = *It;
			if (SandbagCover && SandbagCover->ShouldAllowPlayerProjectilePassthrough(InstigatorPawn))
			{
				Projectile->IgnoreActor(SandbagCover);
			}
		}
	}
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
	FireWithAimIntent(
		AimDirection,
		InstigatorPawn,
		ProjectileHitEffectId,
		WeaponTypeTag,
		0.0f,
		nullptr,
		nullptr,
		FVector::ZeroVector,
		false);
}

void ATunaSweeperWeapon::FireWithAimIntent(
	const FVector& AimDirection,
	APawn* InstigatorPawn,
	FName ProjectileHitEffectId,
	FName WeaponTypeTag,
	float SpreadHalfAngleDegrees,
	AActor* AimIntentActor,
	UPrimitiveComponent* AimIntentComponent,
	FVector AimIntentWorldPoint,
	bool bHasAimIntentWorldPoint)
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

	const FVector SpawnLocation = MuzzlePoint ? MuzzlePoint->GetComponentLocation() : GetActorLocation();
	FVector ShotDirection = bHasAimIntentWorldPoint
		? (AimIntentWorldPoint - SpawnLocation).GetSafeNormal()
		: AimDirection.GetSafeNormal2D();
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
		const FVector CenterDirection = ApplyRandomConeSpread(ShotDirection, SpreadHalfAngleDegrees);
		const int32 ProjectileCount = FMath::Max(1, ShotgunProjectileCount);
		const float SpreadHalfAngle = FMath::Max(0.0f, ShotgunSpreadAngleDegrees) * 0.5f;
		for (int32 ProjectileIndex = 0; ProjectileIndex < ProjectileCount; ++ProjectileIndex)
		{
			const FVector PelletDirection = ApplyRandomConeSpread(CenterDirection, SpreadHalfAngle);
			SpawnProjectile(
				*World,
				LoadedProjectileClass,
				PelletDirection,
				InstigatorPawn,
				ProjectileHitEffectId,
				AimIntentActor,
				AimIntentComponent,
				AimIntentWorldPoint,
				bHasAimIntentWorldPoint);
		}
	}
	else
	{
		const FVector SpreadDirection = ApplyRandomConeSpread(ShotDirection, SpreadHalfAngleDegrees);
		SpawnProjectile(
			*World,
			LoadedProjectileClass,
			SpreadDirection,
			InstigatorPawn,
			ProjectileHitEffectId,
			AimIntentActor,
			AimIntentComponent,
			AimIntentWorldPoint,
			bHasAimIntentWorldPoint);
	}

	LastFireTimeSeconds = CurrentTime;
}

ATunaSweeperProjectile* ATunaSweeperWeapon::SpawnProjectile(
	UWorld& World,
	TSubclassOf<ATunaSweeperProjectile> ProjectileClassToSpawn,
	const FVector& ShotDirection,
	APawn* InstigatorPawn,
	FName ProjectileHitEffectId,
	AActor* AimIntentActor,
	UPrimitiveComponent* AimIntentComponent,
	const FVector& AimIntentWorldPoint,
	bool bHasAimIntentWorldPoint)
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
		SpawnedProjectile->SetAimIntent(
			AimIntentActor,
			AimIntentComponent,
			AimIntentWorldPoint,
			bHasAimIntentWorldPoint);
		IgnoreNearbyPlayerPassthroughCovers(SpawnedProjectile, InstigatorPawn);
	}

	return SpawnedProjectile;
}
