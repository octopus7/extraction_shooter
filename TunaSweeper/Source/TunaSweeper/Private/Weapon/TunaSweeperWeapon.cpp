#include "Weapon/TunaSweeperWeapon.h"

#include "CollisionQueryParams.h"
#include "Component/TunaSweeperLaserSightComponent.h"
#include "Component/TunaSweeperWeaponCombatComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"
#include "Interaction/TunaSweeperSandbagCoverActor.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Sound/SoundBase.h"
#include "Subsystem/TunaSweeperNoiseSubsystem.h"
#include "TunaSweeperCollisionChannels.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"
#include "Weapon/TunaSweeperProjectile.h"
#include "Weapon/TunaSweeperShellCasing.h"
#include "Weapon/TunaSweeperWeaponPresentationDataAsset.h"

DEFINE_LOG_CATEGORY_STATIC(LogTunaSweeperLaserSight, Log, All);

namespace TunaSweeperWeaponTags
{
	const FName ShotgunWeaponTypeTag(TEXT("weapon.type.shotgun"));
	const FName GunshotNoiseTag(TEXT("noise.gunshot"));
	const FName MuzzleSocketName(TEXT("MuzzleSocket"));
	const FName LaserSightSocketName(TEXT("LaserSightSocket"));
	const FName ShellEjectionSocketName(TEXT("ShellEjectionSocket"));
	constexpr float GunshotNoiseLoudness = 1.0f;
	constexpr float ShotgunNoiseLoudness = 1.15f;
	constexpr float GunshotNoiseMaxRange = 2200.0f;
	constexpr float ShotgunNoiseMaxRange = 2400.0f;
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

	bool IsLaserSightDebugEnabled(const UTunaSweeperLaserSightComponent& LaserSightComponent)
	{
		return LaserSightComponent.IsLaserSightDebugEnabled() &&
			CVarTunaSweeperLaserSightDebug.GetValueOnGameThread() != 0;
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

	MuzzleFlashLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("MuzzleFlashLight"));
	MuzzleFlashLight->SetupAttachment(RootComponent);
	MuzzleFlashLight->SetLightColor(MuzzleFlashLightColor);
	MuzzleFlashLight->SetIntensity(0.0f);
	MuzzleFlashLight->SetAttenuationRadius(MuzzleFlashLightAttenuationRadius);
	MuzzleFlashLight->SetCastShadows(false);
	MuzzleFlashLight->SetVisibility(false);

	LaserSightComponent = CreateDefaultSubobject<UTunaSweeperLaserSightComponent>(TEXT("LaserSightComponent"));
	LaserSightComponent->SetupAttachment(MuzzlePoint);

	CombatComponent = CreateDefaultSubobject<UTunaSweeperWeaponCombatComponent>(TEXT("CombatComponent"));

	ProjectileClass = TSoftClassPtr<ATunaSweeperProjectile>(FSoftObjectPath(TEXT("/Game/Weapons/BP_TunaSweeperProjectile.BP_TunaSweeperProjectile_C")));
	ShellCasingClass = ATunaSweeperShellCasing::StaticClass();
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

void ATunaSweeperWeapon::ConfigureRuntimeSpreadRecoil(
	FName WeaponTypeTag,
	const FTunaSweeperWeaponSpreadRecoilDefinition& RecoilDefinition)
{
	if (CombatComponent)
	{
		CombatComponent->ConfigureSpreadRecoilDefinition(WeaponTypeTag, RecoilDefinition);
	}
}

void ATunaSweeperWeapon::ClearRuntimeSpreadRecoil()
{
	if (CombatComponent)
	{
		CombatComponent->ClearSpreadRecoilDefinition();
	}
}

void ATunaSweeperWeapon::ResetRuntimeSpreadRecoil()
{
	if (CombatComponent)
	{
		CombatComponent->ResetSpreadRecoil();
	}
}

float ATunaSweeperWeapon::GetRuntimeSpreadHalfAngleDegrees() const
{
	return CombatComponent ? CombatComponent->GetSpreadHalfAngleDegrees() : 0.0f;
}

float ATunaSweeperWeapon::GetRuntimeAimedSpreadHalfAngleDegrees() const
{
	return CombatComponent ? CombatComponent->GetAimedSpreadHalfAngleDegrees() : 0.0f;
}

void ATunaSweeperWeapon::AddRuntimeSpreadRecoilShot()
{
	if (CombatComponent)
	{
		CombatComponent->AddSpreadRecoilShot();
	}
}

bool ATunaSweeperWeapon::StartReloadRuntime(float ReloadSeconds)
{
	const bool bStartedReload = CombatComponent && CombatComponent->StartReload(ReloadSeconds);
	if (bStartedReload)
	{
		if (UTunaSweeperWeaponPresentationDataAsset* PresentationData = WeaponPresentationDataAsset.LoadSynchronous())
		{
			PlayReloadPresentation(PresentationData->ReloadStartSound);
		}
	}

	return bStartedReload;
}

void ATunaSweeperWeapon::FinishReloadRuntime()
{
	if (CombatComponent && CombatComponent->IsReloading())
	{
		CombatComponent->FinishReload();
		if (UTunaSweeperWeaponPresentationDataAsset* PresentationData = WeaponPresentationDataAsset.LoadSynchronous())
		{
			PlayReloadPresentation(PresentationData->ReloadCompleteSound);
		}
	}
}

void ATunaSweeperWeapon::CancelReloadRuntime()
{
	if (CombatComponent)
	{
		CombatComponent->CancelReload();
	}
}

bool ATunaSweeperWeapon::IsReloadRuntimeActive() const
{
	return CombatComponent && CombatComponent->IsReloading();
}

bool ATunaSweeperWeapon::HasReloadRuntimeFinished() const
{
	return CombatComponent && CombatComponent->HasReloadFinished();
}

float ATunaSweeperWeapon::GetReloadRuntimeProgress() const
{
	return CombatComponent ? CombatComponent->GetReloadProgress() : 0.0f;
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
	return GetMuzzleWorldTransform().GetLocation();
}

bool ATunaSweeperWeapon::TryGetWeaponSocketWorldTransform(FName SocketName, FTransform& OutTransform) const
{
	if (WeaponMesh && !SocketName.IsNone() && WeaponMesh->DoesSocketExist(SocketName))
	{
		OutTransform = WeaponMesh->GetSocketTransform(SocketName, RTS_World);
		return true;
	}

	return false;
}

FTransform ATunaSweeperWeapon::GetMuzzleWorldTransform() const
{
	FTransform MuzzleTransform;
	if (TryGetWeaponSocketWorldTransform(TunaSweeperWeaponTags::MuzzleSocketName, MuzzleTransform))
	{
		return MuzzleTransform;
	}

	return MuzzlePoint ? MuzzlePoint->GetComponentTransform() : GetActorTransform();
}

FTransform ATunaSweeperWeapon::GetLaserSightWorldTransform() const
{
	FTransform LaserSightTransform;
	if (TryGetWeaponSocketWorldTransform(TunaSweeperWeaponTags::LaserSightSocketName, LaserSightTransform))
	{
		return LaserSightTransform;
	}

	return GetMuzzleWorldTransform();
}

void ATunaSweeperWeapon::UpdateLaserSightBeam(
	const FVector& AimDirection,
	const FVector& AimWorldPoint,
	bool bHasAimWorldPoint)
{
	if (!LaserSightComponent)
	{
		return;
	}

	const FTransform LaserSightWorldTransform = GetLaserSightWorldTransform();
	const FVector BeamStartWorld = LaserSightWorldTransform.GetLocation();
	LaserSightComponent->SetWorldTransform(LaserSightWorldTransform);
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
			TunaSweeperCollisionChannels::Projectile,
			QueryParams) &&
			LaserHit.bBlockingHit)
		{
			bLaserTraceHit = true;
			BeamEndWorld = LaserHit.ImpactPoint;
		}
	}

	const FVector BeamEndLocal = LaserSightComponent->GetComponentTransform().InverseTransformPosition(BeamEndWorld);
	LaserSightComponent->SetBeamEnd(BeamEndWorld);

	if (World && IsLaserSightDebugEnabled(*LaserSightComponent))
	{
		const float DebugRange = FMath::Max(1.0f, LaserSightFallbackRange);
		const FVector LaserSightDirection = ResolveMuzzleLevelAimDirection(
			BeamStartWorld,
			FVector::ZeroVector,
			false,
			AimDirection,
			GetActorForwardVector());
		FVector LaserSightForwardDirection = GetLaserSightWorldTransform().GetUnitAxis(EAxis::X).GetSafeNormal2D();
		if (LaserSightForwardDirection.IsNearlyZero())
		{
			LaserSightForwardDirection = GetActorForwardVector().GetSafeNormal2D();
		}
		if (LaserSightForwardDirection.IsNearlyZero())
		{
			LaserSightForwardDirection = FVector::ForwardVector;
		}

		DrawLaserSightDebug(
			*World,
			BeamStartWorld,
			LaserSightDirection,
			LaserSightForwardDirection,
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
				TEXT("LaserSightDebug Weapon=%s HasAimWorld=%d AimWorld=%s AimDir=%s LaserSight=%s LaserSightRot=%s LaserSightDir=%s LaserSightForwardDir=%s ResolvedLaserDir=%s TraceEnd=%s Hit=%d HitActor=%s HitComponent=%s HitPoint=%s BeamEndWorld=%s BeamEndLocal=%s LaserComponentLocation=%s LaserComponentRotation=%s"),
				*GetNameSafe(this),
				bHasAimWorldPoint ? 1 : 0,
				*AimWorldPoint.ToString(),
				*AimDirection.ToString(),
				*BeamStartWorld.ToString(),
				*LaserSightWorldTransform.Rotator().ToString(),
				*LaserSightDirection.ToString(),
				*LaserSightForwardDirection.ToString(),
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

void ATunaSweeperWeapon::SetWeaponPresentationDataAsset(
	TSoftObjectPtr<UTunaSweeperWeaponPresentationDataAsset> InWeaponPresentationDataAsset)
{
	WeaponPresentationDataAsset = MoveTemp(InWeaponPresentationDataAsset);
}

bool ATunaSweeperWeapon::Fire(
	const FVector& AimDirection,
	APawn* InstigatorPawn,
	FName ProjectileHitEffectId,
	FName WeaponTypeTag)
{
	return FireWithAimIntent(
		AimDirection,
		InstigatorPawn,
		NAME_None,
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

bool ATunaSweeperWeapon::FireWithAimIntent(
	const FVector& AimDirection,
	APawn* InstigatorPawn,
	FName ImpactProfileId,
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
		return false;
	}

	const float CurrentTime = World->GetTimeSeconds();
	if (CurrentTime - LastFireTimeSeconds < FireCooldown)
	{
		return false;
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
		bool bSpawnedAnyProjectile = false;
		for (int32 ProjectileIndex = 0; ProjectileIndex < ProjectileCount; ++ProjectileIndex)
		{
			const FVector PelletDirection = ApplyRandomConeSpread(CenterDirection, SpreadHalfAngle);
			bSpawnedAnyProjectile |= SpawnProjectile(
				*World,
				LoadedProjectileClass,
				PelletDirection,
				InstigatorPawn,
				ImpactProfileId,
				ProjectileHitEffectId,
				ProjectileDamageMultiplier,
				ProjectileDamageBonus,
				AimIntentActor,
				AimIntentComponent,
				AimIntentWorldPoint,
				bHasAimIntentWorldPoint) != nullptr;
		}
		if (!bSpawnedAnyProjectile)
		{
			return false;
		}
	}
	else
	{
		const FVector SpreadDirection = ApplyRandomConeSpread(ShotDirection, SpreadHalfAngleDegrees);
		if (!SpawnProjectile(
			*World,
			LoadedProjectileClass,
			SpreadDirection,
			InstigatorPawn,
			ImpactProfileId,
			ProjectileHitEffectId,
			ProjectileDamageMultiplier,
			ProjectileDamageBonus,
			AimIntentActor,
			AimIntentComponent,
			AimIntentWorldPoint,
			bHasAimIntentWorldPoint))
		{
			return false;
		}
	}

	if (UTunaSweeperNoiseSubsystem* NoiseSubsystem = World->GetSubsystem<UTunaSweeperNoiseSubsystem>())
	{
		const bool bIsShotgun = WeaponTypeTag == TunaSweeperWeaponTags::ShotgunWeaponTypeTag;
		AActor* NoiseInstigator = InstigatorPawn ? static_cast<AActor*>(InstigatorPawn) : GetOwner();
		NoiseSubsystem->ReportNoiseAtLocation(
			SpawnLocation,
			bIsShotgun ? TunaSweeperWeaponTags::ShotgunNoiseLoudness : TunaSweeperWeaponTags::GunshotNoiseLoudness,
			bIsShotgun ? TunaSweeperWeaponTags::ShotgunNoiseMaxRange : TunaSweeperWeaponTags::GunshotNoiseMaxRange,
			TunaSweeperWeaponTags::GunshotNoiseTag,
			this,
			NoiseInstigator);
	}

	LastFireTimeSeconds = CurrentTime;
	EjectShellCasing(*World, InstigatorPawn);
	PlayFirePresentation();
	return true;
}

void ATunaSweeperWeapon::EjectShellCasing(UWorld& World, APawn* InstigatorPawn)
{
	TSubclassOf<ATunaSweeperShellCasing> CasingClassToSpawn = ShellCasingClass;
	if (!CasingClassToSpawn)
	{
		CasingClassToSpawn = ATunaSweeperShellCasing::StaticClass();
	}

	FTransform ShellEjectionTransform;
	const bool bHasShellEjectionSocket = TryGetWeaponSocketWorldTransform(TunaSweeperWeaponTags::ShellEjectionSocketName, ShellEjectionTransform);

	FVector WeaponForward = (bHasShellEjectionSocket
		? ShellEjectionTransform.GetUnitAxis(EAxis::X)
		: GetActorForwardVector()).GetSafeNormal2D();
	if (WeaponForward.IsNearlyZero())
	{
		WeaponForward = FVector::ForwardVector;
	}

	FVector WeaponRight = (bHasShellEjectionSocket
		? ShellEjectionTransform.GetUnitAxis(EAxis::Y)
		: GetActorRightVector()).GetSafeNormal2D();
	if (WeaponRight.IsNearlyZero())
	{
		WeaponRight = FVector::RightVector;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = this;
	SpawnParameters.Instigator = InstigatorPawn;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	const FVector EjectionLocation = bHasShellEjectionSocket
		? ShellEjectionTransform.GetLocation()
		: GetMuzzleWorldLocation() - WeaponForward * 38.0f + WeaponRight * 18.0f + FVector::UpVector * 12.0f;
	const FRotator ShellEjectionRotation = bHasShellEjectionSocket
		? ShellEjectionTransform.Rotator()
		: WeaponForward.Rotation();
	const FRotator EjectionRotation(
		ShellEjectionRotation.Pitch + FMath::FRandRange(-35.0f, 35.0f),
		ShellEjectionRotation.Yaw,
		ShellEjectionRotation.Roll + FMath::FRandRange(-180.0f, 180.0f));

	ATunaSweeperShellCasing* Casing = World.SpawnActor<ATunaSweeperShellCasing>(
		CasingClassToSpawn,
		EjectionLocation,
		EjectionRotation,
		SpawnParameters);
	if (!Casing)
	{
		return;
	}

	const FVector EjectionVelocity =
		WeaponRight * FMath::FRandRange(135.0f, 180.0f) +
		FVector::UpVector * FMath::FRandRange(70.0f, 105.0f) +
		WeaponForward * FMath::FRandRange(-25.0f, 18.0f);
	const FVector AngularVelocity(
		FMath::FRandRange(-1080.0f, 1080.0f),
		FMath::FRandRange(-1080.0f, 1080.0f),
		FMath::FRandRange(-1080.0f, 1080.0f));
	Casing->LaunchCasing(EjectionVelocity, AngularVelocity);
}

void ATunaSweeperWeapon::PlayFirePresentation()
{
	TriggerMuzzleFlashLight();

	UTunaSweeperWeaponPresentationDataAsset* PresentationData = WeaponPresentationDataAsset.LoadSynchronous();
	if (!PresentationData)
	{
		return;
	}

	if (UNiagaraSystem* MuzzleFlashEffect = PresentationData->MuzzleFlashEffect.LoadSynchronous())
	{
		if (WeaponMesh && WeaponMesh->DoesSocketExist(TunaSweeperWeaponTags::MuzzleSocketName))
		{
			UNiagaraFunctionLibrary::SpawnSystemAttached(
				MuzzleFlashEffect,
				WeaponMesh,
				TunaSweeperWeaponTags::MuzzleSocketName,
				FVector::ZeroVector,
				FRotator::ZeroRotator,
				EAttachLocation::SnapToTarget,
				true);
		}
		else if (MuzzlePoint)
		{
			UNiagaraFunctionLibrary::SpawnSystemAttached(
				MuzzleFlashEffect,
				MuzzlePoint,
				NAME_None,
				FVector::ZeroVector,
				FRotator::ZeroRotator,
				EAttachLocation::SnapToTarget,
				true);
		}
	}

	if (USoundBase* FireSound = PresentationData->FireSound.LoadSynchronous())
	{
		UGameplayStatics::PlaySoundAtLocation(this, FireSound, GetMuzzleWorldLocation());
	}
}

void ATunaSweeperWeapon::TriggerMuzzleFlashLight()
{
	if (!MuzzleFlashLight)
	{
		return;
	}

	MuzzleFlashLight->SetWorldTransform(GetMuzzleWorldTransform());
	MuzzleFlashLight->SetLightColor(MuzzleFlashLightColor);
	MuzzleFlashLight->SetIntensity(FMath::Max(0.0f, MuzzleFlashLightIntensity));
	MuzzleFlashLight->SetAttenuationRadius(FMath::Max(1.0f, MuzzleFlashLightAttenuationRadius));
	MuzzleFlashLight->SetVisibility(MuzzleFlashLightIntensity > 0.0f);

	GetWorldTimerManager().ClearTimer(MuzzleFlashLightTimerHandle);
	GetWorldTimerManager().SetTimer(
		MuzzleFlashLightTimerHandle,
		this,
		&ATunaSweeperWeapon::DeactivateMuzzleFlashLight,
		FMath::Max(0.01f, MuzzleFlashLightDuration),
		false);
}

void ATunaSweeperWeapon::DeactivateMuzzleFlashLight()
{
	if (MuzzleFlashLight)
	{
		MuzzleFlashLight->SetVisibility(false);
	}
}

void ATunaSweeperWeapon::PlayReloadPresentation(TSoftObjectPtr<USoundBase> ReloadSound)
{
	if (USoundBase* LoadedReloadSound = ReloadSound.LoadSynchronous())
	{
		UGameplayStatics::PlaySoundAtLocation(this, LoadedReloadSound, GetActorLocation());
	}
}

ATunaSweeperProjectile* ATunaSweeperWeapon::SpawnProjectile(
	UWorld& World,
	TSubclassOf<ATunaSweeperProjectile> ProjectileClassToSpawn,
	const FVector& ShotDirection,
	APawn* InstigatorPawn,
	FName ImpactProfileId,
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
		SpawnedProjectile->SetImpactProfileId(ImpactProfileId);
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
