#include "Interaction/TunaSweeperBreakableTomatoActor.h"

#include "Components/BoxComponent.h"
#include "Components/DecalComponent.h"
#include "Engine/DamageEvents.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Interaction/TunaSweeperBreakableTomatoComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Materials/MaterialInterface.h"
#include "PhysicalMaterials/PhysicalMaterial.h"

namespace
{
	const TCHAR* DefaultTomatoMeshPath = TEXT("/Game/Meshes/Props/TomatoHead/SM_Tomato.SM_Tomato");
	const TCHAR* DefaultTomatoGeometryCollectionPath = TEXT("/Game/Interaction/GC_Tomato_Fractured.GC_Tomato_Fractured");
	const TCHAR* DefaultTomatoSplatterPath = TEXT("/Game/Effects/NS_Tomato_StickySplatter.NS_Tomato_StickySplatter");
	const TCHAR* DefaultTomatoPhysicalMaterialPath = TEXT("/Game/Physics/PhysicalMaterials/PM_Flesh.PM_Flesh");
	const TCHAR* DefaultTomatoGooMaterialPath = TEXT("/Game/Meshes/Props/TomatoHead/Materials/M_TomatoFlesh_Interior.M_TomatoFlesh_Interior");
	const TCHAR* DefaultTomatoGooDecalMaterialPath = TEXT("/Game/Meshes/Props/TomatoHead/Materials/M_TomatoGooSplat.M_TomatoGooSplat");
	const TCHAR* DefaultTomatoBreakSoundAPath = TEXT("/Game/Audio/Imported/SW_Tomato_A.SW_Tomato_A");
	const TCHAR* DefaultTomatoBreakSoundBPath = TEXT("/Game/Audio/Imported/SW_Tomato_B.SW_Tomato_B");
	const TCHAR* DefaultTomatoBreakSoundCPath = TEXT("/Game/Audio/Imported/SW_Tomato_C.SW_Tomato_C");
}

ATunaSweeperBreakableTomatoActor::ATunaSweeperBreakableTomatoActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.05f;
	BreakableTomatoComponent = CreateDefaultSubobject<UTunaSweeperBreakableTomatoComponent>(TEXT("BreakableTomatoComponent"));
	CrateMesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(DefaultTomatoMeshPath));
	CrateGeometryCollection = TSoftObjectPtr<UGeometryCollection>(FSoftObjectPath(DefaultTomatoGeometryCollectionPath));
	StickySplatterSystem = TSoftObjectPtr<UNiagaraSystem>(FSoftObjectPath(DefaultTomatoSplatterPath));
	TomatoPhysicalMaterial = TSoftObjectPtr<UPhysicalMaterial>(FSoftObjectPath(DefaultTomatoPhysicalMaterialPath));
	StickyGooMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(DefaultTomatoGooMaterialPath));
	StickyGooDecalMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(DefaultTomatoGooDecalMaterialPath));
	CrateId = TEXT("TS_BreakableTomato_Default");
	MaxHealth = 1.0f;
	CollisionExtent = FVector(34.0f, 34.0f, 34.0f);
	CollisionCenterOffset = FVector(0.0f, 0.0f, 34.0f);
	MinAppleCount = 0;
	MaxAppleCount = 0;
	bSpawnCrateFragmentsOnBreak = false;
	GeometryCollectionBreakRadius = 52.0f;
	GeometryCollectionRadialImpulse = 85.0f;
	GeometryCollectionDirectionalImpulse = TomatoImpactDirectionalImpulse;
	GeometryCollectionUpwardImpulse = 190.0f;
	GeometryCollectionExternalClusterStrain = 14000.0f;
	EnsureTomatoBreakSoundVariants();
}

void ATunaSweeperBreakableTomatoActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	EnsureTomatoBreakSoundVariants();
	ApplyTomatoCollisionMaterial();
}

void ATunaSweeperBreakableTomatoActor::BeginPlay()
{
	Super::BeginPlay();
	EnsureTomatoBreakSoundVariants();
	ApplyTomatoCollisionMaterial();
	InitialActorZ = GetActorLocation().Z;
	if (BreakableTomatoComponent)
	{
		BreakableTomatoComponent->ResetTomatoHealth();
	}
}

float ATunaSweeperBreakableTomatoActor::TakeDamage(
	float DamageAmount,
	FDamageEvent const& DamageEvent,
	AController* EventInstigator,
	AActor* DamageCauser)
{
	if (!BreakableTomatoComponent || IsCrateBroken() || DamageAmount <= 0.0f)
	{
		return 0.0f;
	}

	const float AppliedDamage = FMath::Min(BreakableTomatoComponent->GetCurrentHealth(), DamageAmount);
	if (BreakableTomatoComponent->ApplyTomatoDamage(DamageAmount))
	{
		GeometryCollectionDirectionalImpulse = FMath::Max(0.0f, TomatoImpactDirectionalImpulse);
		BreakCrateInDirection(-ResolveImpactDirection(DamageEvent));
		SpawnStickySplatter(DamageEvent);
		SetLifeSpan(FMath::Max(0.0f, DestroyedDebrisLifetime));
	}
	return AppliedDamage;
}

void ATunaSweeperBreakableTomatoActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateHopMovement(DeltaSeconds);
}

void ATunaSweeperBreakableTomatoActor::ConfigureBreakableTomatoDefaults(
	FName InTomatoId,
	float InMaxHealth,
	const TSoftObjectPtr<UStaticMesh>& InTomatoMesh,
	const TSoftObjectPtr<UGeometryCollection>& InTomatoGeometryCollection,
	const TSoftObjectPtr<UNiagaraSystem>& InStickySplatterSystem,
	const TSoftObjectPtr<UMaterialInterface>& InStickyGooDecalMaterial,
	const TSoftObjectPtr<UPhysicalMaterial>& InPhysicalMaterial,
	const FVector& InCollisionExtent,
	const FVector& InCollisionCenterOffset)
{
	ConfigureBreakableAppleCrateDefaults(
		InTomatoId,
		InMaxHealth,
		InTomatoMesh,
		InTomatoGeometryCollection,
		nullptr,
		TSoftObjectPtr<UStaticMesh>(),
		nullptr,
		TSoftObjectPtr<UStaticMesh>());

	StickySplatterSystem = InStickySplatterSystem;
	StickyGooDecalMaterial = InStickyGooDecalMaterial;
	bUseNiagaraStickySplatter = true;
	TomatoPhysicalMaterial = InPhysicalMaterial;
	CollisionExtent = InCollisionExtent.ComponentMax(FVector(1.0f));
	CollisionCenterOffset = InCollisionCenterOffset;
	MinAppleCount = 0;
	MaxAppleCount = 0;
	bSpawnCrateFragmentsOnBreak = false;
	GeometryCollectionBreakRadius = 52.0f;
	GeometryCollectionRadialImpulse = 85.0f;
	GeometryCollectionDirectionalImpulse = TomatoImpactDirectionalImpulse;
	GeometryCollectionUpwardImpulse = 190.0f;
	GeometryCollectionDamageThreshold = 0.0f;
	GeometryCollectionExternalClusterStrain = 14000.0f;
	if (BreakableTomatoComponent)
	{
		BreakableTomatoComponent->SetMaxHealth(InMaxHealth);
	}
	ApplyTomatoCollisionMaterial();
}

void ATunaSweeperBreakableTomatoActor::ApplyTomatoCollisionMaterial()
{
	if (BlockingCollision)
	{
		BlockingCollision->SetPhysMaterialOverride(TomatoPhysicalMaterial.LoadSynchronous());
	}
}

void ATunaSweeperBreakableTomatoActor::SpawnStickySplatter(const FDamageEvent& DamageEvent)
{
	const FVector ImpactLocation = ResolveImpactLocation(DamageEvent);
	const FVector ImpactDirection = ResolveImpactDirection(DamageEvent);
	const FVector ImpactNormal = ResolveImpactNormal(DamageEvent);
	SpawnStickyGooSplats(ImpactLocation);

	if (!bUseNiagaraStickySplatter)
	{
		return;
	}

	UNiagaraSystem* System = StickySplatterSystem.LoadSynchronous();
	if (!System)
	{
		return;
	}
	UNiagaraComponent* Effect = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		this,
		System,
		ImpactLocation,
		ImpactNormal.Rotation(),
		FVector::OneVector,
		true,
		true,
		ENCPoolMethod::AutoRelease,
		true);
	if (Effect)
	{
		Effect->SetVariableVec3(TEXT("User.ImpactDirection"), ImpactDirection);
		Effect->SetVariableVec3(TEXT("User.ImpactNormal"), ImpactNormal);
		Effect->SetVariableFloat(TEXT("User.ImpactStrength"), 1.0f);
	}
}

void ATunaSweeperBreakableTomatoActor::SpawnStickyGooSplats(const FVector& ImpactLocation)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	UMaterialInterface* DecalMaterial = StickyGooDecalMaterial.LoadSynchronous();
	if (!DecalMaterial)
	{
		return;
	}

	const int32 MinimumCount = FMath::Max(0, MinGooSplatCount);
	const int32 SplatCount = FMath::RandRange(MinimumCount, FMath::Max(MinimumCount, MaxGooSplatCount));
	const float MinimumRadius = FMath::Max(1.0f, GooRadiusRangeCm.X);
	const float MaximumRadius = FMath::Max(MinimumRadius, GooRadiusRangeCm.Y);
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(TunaSweeperTomatoGooSplat), false, this);
	for (int32 Index = 0; Index < SplatCount; ++Index)
	{
		const FVector2D Offset2D = FMath::RandPointInCircle(FMath::FRandRange(35.0f, 150.0f));
		const FVector TraceStart = ImpactLocation + FVector(Offset2D.X, Offset2D.Y, 220.0f);
		const FVector TraceEnd = TraceStart - FVector::UpVector * 520.0f;
		FHitResult GroundHit;
		if (World->LineTraceSingleByChannel(GroundHit, TraceStart, TraceEnd, ECC_Visibility, QueryParams))
		{
			const float Radius = FMath::FRandRange(MinimumRadius, MaximumRadius);
			if (UDecalComponent* Decal = UGameplayStatics::SpawnDecalAtLocation(
				World,
				DecalMaterial,
				FVector(8.0f, Radius * 2.3f, Radius * 1.65f),
				GroundHit.ImpactPoint + GroundHit.ImpactNormal * 0.5f,
				GroundHit.ImpactNormal.Rotation(),
				6.0f))
			{
				Decal->SetFadeOut(4.0f, 2.0f, false);
			}
		}
	}
}

FVector ATunaSweeperBreakableTomatoActor::ResolveImpactLocation(const FDamageEvent& DamageEvent) const
{
	if (DamageEvent.IsOfType(FPointDamageEvent::ClassID))
	{
		const FPointDamageEvent& PointDamage = static_cast<const FPointDamageEvent&>(DamageEvent);
		if (!PointDamage.HitInfo.ImpactPoint.IsNearlyZero())
		{
			return PointDamage.HitInfo.ImpactPoint;
		}
	}
	return GetActorLocation() + GetActorTransform().TransformVectorNoScale(CollisionCenterOffset);
}

FVector ATunaSweeperBreakableTomatoActor::ResolveImpactDirection(const FDamageEvent& DamageEvent) const
{
	if (DamageEvent.IsOfType(FPointDamageEvent::ClassID))
	{
		const FPointDamageEvent& PointDamage = static_cast<const FPointDamageEvent&>(DamageEvent);
		if (!PointDamage.ShotDirection.IsNearlyZero())
		{
			return PointDamage.ShotDirection.GetSafeNormal();
		}
	}
	return GetActorForwardVector();
}

FVector ATunaSweeperBreakableTomatoActor::ResolveImpactNormal(const FDamageEvent& DamageEvent) const
{
	if (DamageEvent.IsOfType(FPointDamageEvent::ClassID))
	{
		const FPointDamageEvent& PointDamage = static_cast<const FPointDamageEvent&>(DamageEvent);
		if (!PointDamage.HitInfo.ImpactNormal.IsNearlyZero())
		{
			return PointDamage.HitInfo.ImpactNormal.GetSafeNormal();
		}
	}
	return -ResolveImpactDirection(DamageEvent);
}

void ATunaSweeperBreakableTomatoActor::StartMovementSegment()
{
	bMovingTowardCharacter = true;
	SegmentRemainingSeconds = FMath::FRandRange(1.0f, 2.0f);
	HopElapsedSeconds = 0.0f;
}

void ATunaSweeperBreakableTomatoActor::StartRestSegment()
{
	bMovingTowardCharacter = false;
	SegmentRemainingSeconds = FMath::FRandRange(2.0f, 3.0f);
	HopElapsedSeconds = 0.0f;
}

void ATunaSweeperBreakableTomatoActor::UpdateHopMovement(float DeltaSeconds)
{
	if (IsCrateBroken() || !BreakableTomatoComponent || BreakableTomatoComponent->IsBroken())
	{
		return;
	}

	ACharacter* Character = UGameplayStatics::GetPlayerCharacter(this, 0);
	if (!Character)
	{
		return;
	}

	const FVector ToCharacter = Character->GetActorLocation() - GetActorLocation();
	const float Distance2D = ToCharacter.Size2D();
	if (Distance2D > FMath::Max(0.0f, ActivationRadiusCm))
	{
		bMovingTowardCharacter = false;
		SegmentRemainingSeconds = 0.0f;
		HopElapsedSeconds = 0.0f;
		SetActorLocation(FVector(GetActorLocation().X, GetActorLocation().Y, InitialActorZ));
		return;
	}

	SegmentRemainingSeconds -= DeltaSeconds;
	if (SegmentRemainingSeconds <= 0.0f)
	{
		if (bMovingTowardCharacter)
		{
			StartRestSegment();
		}
		else
		{
			StartMovementSegment();
		}
	}

	if (!bMovingTowardCharacter)
	{
		SetActorLocation(FVector(GetActorLocation().X, GetActorLocation().Y, InitialActorZ));
		return;
	}

	HopElapsedSeconds += DeltaSeconds;
	if (Distance2D > FMath::Max(0.0f, StopDistanceCm))
	{
		const FVector HorizontalDirection(ToCharacter.X, ToCharacter.Y, 0.0f);
		SetActorLocation(GetActorLocation() + HorizontalDirection.GetSafeNormal() * HopMoveSpeedCmPerSecond * DeltaSeconds);
	}

	const float HopOffset = FMath::Max(0.0f, HopHeightCm) * FMath::Abs(FMath::Sin(HopElapsedSeconds * UE_PI * 3.4f));
	SetActorLocation(FVector(GetActorLocation().X, GetActorLocation().Y, InitialActorZ + HopOffset));
}

void ATunaSweeperBreakableTomatoActor::EnsureTomatoBreakSoundVariants()
{
	if (BreakSoundVariants.Num() > 0)
	{
		return;
	}

	BreakSoundVariants = {
		TSoftObjectPtr<USoundBase>(FSoftObjectPath(DefaultTomatoBreakSoundAPath)),
		TSoftObjectPtr<USoundBase>(FSoftObjectPath(DefaultTomatoBreakSoundBPath)),
		TSoftObjectPtr<USoundBase>(FSoftObjectPath(DefaultTomatoBreakSoundCPath))
	};
}
