#include "Weapon/TunaSweeperWeapon.h"

#include "CollisionQueryParams.h"
#include "Component/TunaSweeperLaserSightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"
#include "Interaction/TunaSweeperSandbagCoverActor.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"
#include "Weapon/TunaSweeperProjectile.h"

DEFINE_LOG_CATEGORY_STATIC(LogTunaSweeperLaserSight, Log, All);

namespace TunaSweeperWeaponTags
{
	const FName ShotgunWeaponTypeTag(TEXT("weapon.type.shotgun"));
}

namespace
{
	TAutoConsoleVariable<int32> CVarTunaSweeperLaserSightDebug(
		TEXT("ts.LaserSight.Debug"),
		1,
		TEXT("Draw and log TunaSweeper laser sight diagnostics. 0 disables, 1 enables."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarTunaSweeperLaserSightLogInterval(
		TEXT("ts.LaserSight.LogInterval"),
		0.5f,
		TEXT("Seconds between TunaSweeper laser sight diagnostic log lines."),
		ECVF_Default);

	constexpr uint8 LaserSightDebugDepthPriority = 0;
	constexpr float LaserSightDebugLifeTime = 0.0f;

	bool IsLaserSightDebugEnabled()
	{
		return CVarTunaSweeperLaserSightDebug.GetValueOnGameThread() != 0;
	}

	void DrawLaserSightDebug(
		UWorld& World,
		const FVector& MuzzleWorldLocation,
		const FVector& MuzzleSightDirection,
		const FVector& MuzzleForwardDirection,
		const FVector& AimWorldPoint,
		bool bHasAimWorldPoint,
		const FVector& TraceEndWorld,
		const FVector& BeamEndWorld,
		const FHitResult& LaserHit,
		bool bLaserTraceHit,
		float DebugRange)
	{
		const float SafeDebugRange = FMath::Max(1.0f, DebugRange);

		// Cyan is the requested diagnostic line from the muzzle along the current sight direction.
		DrawDebugLine(
			&World,
			MuzzleWorldLocation,
			MuzzleWorldLocation + MuzzleSightDirection * SafeDebugRange,
			FColor::Cyan,
			false,
			LaserSightDebugLifeTime,
			LaserSightDebugDepthPriority,
			4.0f);

		DrawDebugLine(
			&World,
			MuzzleWorldLocation,
			TraceEndWorld,
			FColor::Yellow,
			false,
			LaserSightDebugLifeTime,
			LaserSightDebugDepthPriority,
			2.0f);

		DrawDebugLine(
			&World,
			MuzzleWorldLocation,
			BeamEndWorld,
			bLaserTraceHit ? FColor::Green : FColor::Orange,
			false,
			LaserSightDebugLifeTime,
			LaserSightDebugDepthPriority,
			3.0f);

		DrawDebugLine(
			&World,
			MuzzleWorldLocation,
			MuzzleWorldLocation + MuzzleForwardDirection * FMath::Min(SafeDebugRange, 300.0f),
			FColor::Magenta,
			false,
			LaserSightDebugLifeTime,
			LaserSightDebugDepthPriority,
			2.0f);

		DrawDebugPoint(
			&World,
			MuzzleWorldLocation,
			12.0f,
			FColor::White,
			false,
			LaserSightDebugLifeTime,
			LaserSightDebugDepthPriority);
		DrawDebugPoint(
			&World,
			BeamEndWorld,
			14.0f,
			bLaserTraceHit ? FColor::Red : FColor::Orange,
			false,
			LaserSightDebugLifeTime,
			LaserSightDebugDepthPriority);

		if (bHasAimWorldPoint)
		{
			DrawDebugPoint(
				&World,
				AimWorldPoint,
				12.0f,
				FColor::Blue,
				false,
				LaserSightDebugLifeTime,
				LaserSightDebugDepthPriority);
		}

		if (bLaserTraceHit)
		{
			DrawDebugSphere(
				&World,
				LaserHit.ImpactPoint,
				12.0f,
				12,
				FColor::Red,
				false,
				LaserSightDebugLifeTime,
				LaserSightDebugDepthPriority,
				1.5f);
		}
	}

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

	FVector ResolveMuzzleLevelAimDirection(
		const FVector& MuzzleWorldLocation,
		const FVector& AimWorldPoint,
		bool bHasAimWorldPoint,
		const FVector& AimDirection,
		const FVector& FallbackForward)
	{
		FVector ResolvedDirection = FVector::ZeroVector;
		if (bHasAimWorldPoint)
		{
			const FVector MuzzleLevelAimPoint(AimWorldPoint.X, AimWorldPoint.Y, MuzzleWorldLocation.Z);
			ResolvedDirection = (MuzzleLevelAimPoint - MuzzleWorldLocation).GetSafeNormal2D();
		}

		if (ResolvedDirection.IsNearlyZero())
		{
			ResolvedDirection = AimDirection.GetSafeNormal2D();
		}
		if (ResolvedDirection.IsNearlyZero())
		{
			ResolvedDirection = FallbackForward.GetSafeNormal2D();
		}
		if (ResolvedDirection.IsNearlyZero())
		{
			ResolvedDirection = FVector::ForwardVector;
		}

		return ResolvedDirection;
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

	LaserSightComponent = CreateDefaultSubobject<UTunaSweeperLaserSightComponent>(TEXT("LaserSightComponent"));
	LaserSightComponent->SetupAttachment(MuzzlePoint);

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

void ATunaSweeperWeapon::SetLaserSightEnabled(bool bEnabled)
{
	if (LaserSightComponent)
	{
		LaserSightComponent->SetLaserSightEnabled(bEnabled);
	}
}

bool ATunaSweeperWeapon::IsLaserSightEnabled() const
{
	return LaserSightComponent && LaserSightComponent->IsLaserSightEnabled();
}

FVector ATunaSweeperWeapon::GetMuzzleWorldLocation() const
{
	return MuzzlePoint ? MuzzlePoint->GetComponentLocation() : GetActorLocation();
}

void ATunaSweeperWeapon::UpdateLaserSightBeam(
	const FVector& AimDirection,
	const FVector& AimWorldPoint,
	bool bHasAimWorldPoint)
{
	if (!LaserSightComponent || !MuzzlePoint)
	{
		return;
	}

	const FVector BeamStartWorld = GetMuzzleWorldLocation();
	const FVector LaserDirection = ResolveMuzzleLevelAimDirection(
		BeamStartWorld,
		AimWorldPoint,
		bHasAimWorldPoint,
		AimDirection,
		GetActorForwardVector());
	const FVector TraceEndWorld =
		BeamStartWorld + LaserDirection * FMath::Max(1.0f, LaserSightFallbackRange);
	FVector BeamEndWorld = TraceEndWorld;
	FHitResult LaserHit;
	bool bLaserTraceHit = false;

	UWorld* World = GetWorld();
	if (World)
	{
		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(TunaSweeperLaserSight), false);
		QueryParams.AddIgnoredActor(this);
		if (AActor* OwnerActor = GetOwner())
		{
			QueryParams.AddIgnoredActor(OwnerActor);
		}
		if (APawn* InstigatorPawn = GetInstigator())
		{
			QueryParams.AddIgnoredActor(InstigatorPawn);
		}

		if (World->LineTraceSingleByChannel(
			LaserHit,
			BeamStartWorld,
			TraceEndWorld,
			ECC_Visibility,
			QueryParams) &&
			LaserHit.bBlockingHit)
		{
			bLaserTraceHit = true;
			BeamEndWorld = LaserHit.ImpactPoint;
		}
	}

	const FVector BeamEndLocal = LaserSightComponent->GetComponentTransform().InverseTransformPosition(BeamEndWorld);
	LaserSightComponent->SetBeamEnd(BeamEndLocal);

	if (World && IsLaserSightDebugEnabled())
	{
		const float DebugRange = FMath::Max(1.0f, LaserSightFallbackRange);
		const FVector MuzzleSightDirection = ResolveMuzzleLevelAimDirection(
			BeamStartWorld,
			FVector::ZeroVector,
			false,
			AimDirection,
			GetActorForwardVector());
		FVector MuzzleForwardDirection = MuzzlePoint->GetForwardVector().GetSafeNormal2D();
		if (MuzzleForwardDirection.IsNearlyZero())
		{
			MuzzleForwardDirection = GetActorForwardVector().GetSafeNormal2D();
		}
		if (MuzzleForwardDirection.IsNearlyZero())
		{
			MuzzleForwardDirection = FVector::ForwardVector;
		}

		DrawLaserSightDebug(
			*World,
			BeamStartWorld,
			MuzzleSightDirection,
			MuzzleForwardDirection,
			AimWorldPoint,
			bHasAimWorldPoint,
			TraceEndWorld,
			BeamEndWorld,
			LaserHit,
			bLaserTraceHit,
			DebugRange);

		const float LogIntervalSeconds = FMath::Max(0.0f, CVarTunaSweeperLaserSightLogInterval.GetValueOnGameThread());
		const float CurrentTimeSeconds = World->GetTimeSeconds();
		if (CurrentTimeSeconds - LastLaserSightDebugLogTimeSeconds >= LogIntervalSeconds)
		{
			LastLaserSightDebugLogTimeSeconds = CurrentTimeSeconds;

			const FString HitActorName = LaserHit.GetActor() ? LaserHit.GetActor()->GetName() : TEXT("None");
			const FString HitComponentName = LaserHit.GetComponent() ? LaserHit.GetComponent()->GetName() : TEXT("None");
			const FVector HitPointWorld = bLaserTraceHit ? LaserHit.ImpactPoint : FVector::ZeroVector;

			UE_LOG(
				LogTunaSweeperLaserSight,
				Display,
				TEXT("LaserSightDebug Weapon=%s HasAimWorld=%d AimWorld=%s AimDir=%s Muzzle=%s MuzzleRot=%s MuzzleSightDir=%s MuzzleForwardDir=%s ResolvedLaserDir=%s TraceEnd=%s Hit=%d HitActor=%s HitComponent=%s HitPoint=%s BeamEndWorld=%s BeamEndLocal=%s LaserComponentLocation=%s LaserComponentRotation=%s"),
				*GetNameSafe(this),
				bHasAimWorldPoint ? 1 : 0,
				*AimWorldPoint.ToString(),
				*AimDirection.ToString(),
				*BeamStartWorld.ToString(),
				*MuzzlePoint->GetComponentRotation().ToString(),
				*MuzzleSightDirection.ToString(),
				*MuzzleForwardDirection.ToString(),
				*LaserDirection.ToString(),
				*TraceEndWorld.ToString(),
				bLaserTraceHit ? 1 : 0,
				*HitActorName,
				*HitComponentName,
				*HitPointWorld.ToString(),
				*BeamEndWorld.ToString(),
				*BeamEndLocal.ToString(),
				*LaserSightComponent->GetComponentLocation().ToString(),
				*LaserSightComponent->GetComponentRotation().ToString());
		}
	}
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
		1.0f,
		0,
		0.0f,
		FVector::ZeroVector,
		false,
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
	float ProjectileDamageMultiplier,
	int32 ProjectileDamageBonus,
	float SpreadHalfAngleDegrees,
	const FVector& AimWorldPoint,
	bool bHasAimWorldPoint,
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

	const FVector SpawnLocation = GetMuzzleWorldLocation();
	const FVector ShotDirection = ResolveMuzzleLevelAimDirection(
		SpawnLocation,
		AimWorldPoint,
		bHasAimWorldPoint,
		AimDirection,
		GetActorForwardVector());

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
				ProjectileDamageMultiplier,
				ProjectileDamageBonus,
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
			ProjectileDamageMultiplier,
			ProjectileDamageBonus,
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
	float ProjectileDamageMultiplier,
	int32 ProjectileDamageBonus,
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
		const float BaseDamageAmount = SpawnedProjectile->GetDamageAmount();
		const int32 ModifiedDamageAmount = FMath::Max(
			0,
			FMath::RoundToInt(BaseDamageAmount * FMath::Max(0.0f, ProjectileDamageMultiplier)) + ProjectileDamageBonus);
		SpawnedProjectile->SetDamageAmount(static_cast<float>(ModifiedDamageAmount));
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
