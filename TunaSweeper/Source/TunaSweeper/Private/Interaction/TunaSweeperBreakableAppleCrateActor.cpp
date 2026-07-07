#include "Interaction/TunaSweeperBreakableAppleCrateActor.h"

#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DamageEvents.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Interaction/TunaSweeperPhysicsAppleActor.h"
#include "TunaSweeperCollisionChannels.h"

namespace
{
	const TCHAR* DefaultCrateMeshPath = TEXT("/Game/Nature/Wood/SM_CrateB.SM_CrateB");
	const TCHAR* DefaultAppleMeshPath = TEXT("/Game/AXTemp/SM_Apple.SM_Apple");

	FVector MakeSafeExtent(const FVector& Extent)
	{
		return FVector(
			FMath::Max(0.0f, Extent.X),
			FMath::Max(0.0f, Extent.Y),
			FMath::Max(0.0f, Extent.Z));
	}

	FVector GetSafeHorizontalDirection(const FVector& Direction, const FVector& Fallback)
	{
		FVector HorizontalDirection(Direction.X, Direction.Y, 0.0f);
		if (!HorizontalDirection.Normalize())
		{
			HorizontalDirection = FVector(Fallback.X, Fallback.Y, 0.0f);
			if (!HorizontalDirection.Normalize())
			{
				HorizontalDirection = FVector::ForwardVector;
			}
		}
		return HorizontalDirection;
	}
}

ATunaSweeperBreakableAppleCrateActor::ATunaSweeperBreakableAppleCrateActor()
{
	PrimaryActorTick.bCanEverTick = false;
	SetCanBeDamaged(true);

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	BlockingCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("BlockingCollision"));
	BlockingCollision->SetupAttachment(RootComponent);
	BlockingCollision->SetHiddenInGame(true);
	BlockingCollision->SetVisibility(false);
	BlockingCollision->SetCanEverAffectNavigation(true);

	CrateMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CrateMesh"));
	CrateMeshComponent->SetupAttachment(RootComponent);
	CrateMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CrateMeshComponent->SetGenerateOverlapEvents(false);
	CrateMeshComponent->SetCanEverAffectNavigation(false);
	CrateMeshComponent->SetCastShadow(true);

	CrateMesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(DefaultCrateMeshPath));
	AppleMesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(DefaultAppleMeshPath));
	AppleActorClass = ATunaSweeperPhysicsAppleActor::StaticClass();

	ApplyCrateDefaults();
}

void ATunaSweeperBreakableAppleCrateActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	MaxHealth = FMath::Max(1.0f, MaxHealth);
	CurrentHealth = bCrateBroken ? 0.0f : MaxHealth;
	CollisionExtent = MakeSafeExtent(CollisionExtent);
	AppleSpawnExtent = MakeSafeExtent(AppleSpawnExtent);
	MinAppleCount = FMath::Max(0, MinAppleCount);
	MaxAppleCount = FMath::Max(MinAppleCount, MaxAppleCount);
	AppleCollisionRadiusCm = FMath::Max(1.0f, AppleCollisionRadiusCm);
	AppleVisualScale = FMath::Max(0.01f, AppleVisualScale);
	AppleLifeSeconds = FMath::Max(0.0f, AppleLifeSeconds);
	HorizontalSpeedRange.X = FMath::Max(0.0f, HorizontalSpeedRange.X);
	HorizontalSpeedRange.Y = FMath::Max(HorizontalSpeedRange.X, HorizontalSpeedRange.Y);
	VerticalSpeedRange.X = FMath::Max(0.0f, VerticalSpeedRange.X);
	VerticalSpeedRange.Y = FMath::Max(VerticalSpeedRange.X, VerticalSpeedRange.Y);
	RandomScatterWeight = FMath::Max(0.0f, RandomScatterWeight);
	AngularSpeedDegrees = FMath::Max(0.0f, AngularSpeedDegrees);

	ApplyCrateDefaults();
}

void ATunaSweeperBreakableAppleCrateActor::BeginPlay()
{
	Super::BeginPlay();

	MaxHealth = FMath::Max(1.0f, MaxHealth);
	CurrentHealth = MaxHealth;
	bCrateBroken = false;
	SetCanBeDamaged(true);

	ApplyCrateDefaults();
}

float ATunaSweeperBreakableAppleCrateActor::TakeDamage(
	float DamageAmount,
	FDamageEvent const& DamageEvent,
	AController* EventInstigator,
	AActor* DamageCauser)
{
	if (bCrateBroken || DamageAmount <= 0.0f)
	{
		return 0.0f;
	}

	const float AppliedDamage = FMath::Min(CurrentHealth, DamageAmount);
	CurrentHealth = FMath::Max(0.0f, CurrentHealth - DamageAmount);
	if (CurrentHealth <= 0.0f)
	{
		BreakCrateFromDirection(ResolveSpillDirection(DamageEvent, DamageCauser));
	}

	return AppliedDamage;
}

void ATunaSweeperBreakableAppleCrateActor::BreakCrate()
{
	BreakCrateFromDirection(GetActorForwardVector());
}

void ATunaSweeperBreakableAppleCrateActor::ConfigureBreakableAppleCrateDefaults(
	FName InCrateId,
	float InMaxHealth,
	const TSoftObjectPtr<UStaticMesh>& InCrateMesh,
	TSubclassOf<ATunaSweeperPhysicsAppleActor> InAppleActorClass,
	const TSoftObjectPtr<UStaticMesh>& InAppleMesh)
{
	CrateId = InCrateId;
	MaxHealth = FMath::Max(1.0f, InMaxHealth);
	CurrentHealth = bCrateBroken ? 0.0f : MaxHealth;
	if (!InCrateMesh.IsNull())
	{
		CrateMesh = InCrateMesh;
	}
	if (InAppleActorClass)
	{
		AppleActorClass = InAppleActorClass;
	}
	if (!InAppleMesh.IsNull())
	{
		AppleMesh = InAppleMesh;
	}

	ApplyCrateDefaults();
}

void ATunaSweeperBreakableAppleCrateActor::ApplyCrateDefaults()
{
	if (BlockingCollision)
	{
		BlockingCollision->SetRelativeLocation(CollisionCenterOffset);
		BlockingCollision->SetBoxExtent(FVector(
			FMath::Max(1.0f, CollisionExtent.X),
			FMath::Max(1.0f, CollisionExtent.Y),
			FMath::Max(1.0f, CollisionExtent.Z)));
		BlockingCollision->SetCollisionEnabled(bCrateBroken ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryAndPhysics);
		BlockingCollision->SetCollisionObjectType(ECC_WorldDynamic);
		BlockingCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
		BlockingCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
		BlockingCollision->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
		BlockingCollision->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
		BlockingCollision->SetCollisionResponseToChannel(TunaSweeperCollisionChannels::Projectile, ECR_Block);
		BlockingCollision->SetCollisionResponseToChannel(TunaSweeperCollisionChannels::VisionOccluder, ECR_Block);
		BlockingCollision->SetGenerateOverlapEvents(false);
		BlockingCollision->CanCharacterStepUpOn = ECB_No;
		BlockingCollision->SetCanEverAffectNavigation(!bCrateBroken);
	}

	if (CrateMeshComponent)
	{
		CrateMeshComponent->SetStaticMesh(CrateMesh.LoadSynchronous());
		CrateMeshComponent->SetRelativeLocation(FVector::ZeroVector);
		CrateMeshComponent->SetRelativeRotation(FRotator::ZeroRotator);
		CrateMeshComponent->SetRelativeScale3D(FVector::OneVector);
		CrateMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		CrateMeshComponent->SetGenerateOverlapEvents(false);
		CrateMeshComponent->SetCanEverAffectNavigation(false);
		CrateMeshComponent->SetHiddenInGame(bCrateBroken && bHideCrateMeshOnBreak);
		CrateMeshComponent->SetVisibility(!(bCrateBroken && bHideCrateMeshOnBreak), true);
	}
}

void ATunaSweeperBreakableAppleCrateActor::BreakCrateFromDirection(const FVector& SpillDirection)
{
	if (bCrateBroken)
	{
		return;
	}

	bCrateBroken = true;
	CurrentHealth = 0.0f;
	SetCanBeDamaged(false);

	ApplyCrateDefaults();
	SpawnApples(SpillDirection);
}

FVector ATunaSweeperBreakableAppleCrateActor::ResolveSpillDirection(
	FDamageEvent const& DamageEvent,
	AActor* DamageCauser) const
{
	FVector Direction = FVector::ZeroVector;

	if (DamageEvent.IsOfType(FPointDamageEvent::ClassID))
	{
		const FPointDamageEvent* PointDamageEvent = static_cast<const FPointDamageEvent*>(&DamageEvent);
		if (PointDamageEvent)
		{
			const FVector ActorCenter = GetActorLocation() + GetActorTransform().TransformVectorNoScale(CollisionCenterOffset);
			Direction = PointDamageEvent->HitInfo.ImpactPoint - ActorCenter;
			if (Direction.SizeSquared2D() <= KINDA_SMALL_NUMBER)
			{
				Direction = PointDamageEvent->ShotDirection;
			}
		}
	}

	if (Direction.SizeSquared2D() <= KINDA_SMALL_NUMBER && DamageCauser)
	{
		Direction = GetActorLocation() - DamageCauser->GetActorLocation();
	}

	return GetSafeHorizontalDirection(Direction, GetActorForwardVector());
}

void ATunaSweeperBreakableAppleCrateActor::SpawnApples(const FVector& SpillDirection)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	TSubclassOf<ATunaSweeperPhysicsAppleActor> LoadedAppleClass = AppleActorClass;
	if (!LoadedAppleClass)
	{
		LoadedAppleClass = ATunaSweeperPhysicsAppleActor::StaticClass();
	}

	const int32 AppleCount = FMath::RandRange(FMath::Max(0, MinAppleCount), FMath::Max(MinAppleCount, MaxAppleCount));
	for (int32 AppleIndex = 0; AppleIndex < AppleCount; ++AppleIndex)
	{
		const FVector SpawnLocation = BuildRandomAppleSpawnLocation();
		const FRotator SpawnRotation(
			FMath::FRandRange(-180.0f, 180.0f),
			FMath::FRandRange(-180.0f, 180.0f),
			FMath::FRandRange(-180.0f, 180.0f));

		FActorSpawnParameters SpawnParameters;
		SpawnParameters.Owner = this;
		SpawnParameters.Instigator = GetInstigator();
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		ATunaSweeperPhysicsAppleActor* AppleActor = World->SpawnActor<ATunaSweeperPhysicsAppleActor>(
			LoadedAppleClass,
			SpawnLocation,
			SpawnRotation,
			SpawnParameters);
		if (!AppleActor)
		{
			continue;
		}

		AppleActor->ConfigurePhysicsAppleDefaults(AppleMesh, AppleCollisionRadiusCm, AppleVisualScale, AppleLifeSeconds);
		AppleActor->LaunchApple(BuildRandomAppleVelocity(SpillDirection), BuildRandomAppleAngularVelocity());
	}
}

FVector ATunaSweeperBreakableAppleCrateActor::BuildRandomAppleSpawnLocation() const
{
	const FVector LocalOffset(
		FMath::FRandRange(-AppleSpawnExtent.X, AppleSpawnExtent.X),
		FMath::FRandRange(-AppleSpawnExtent.Y, AppleSpawnExtent.Y),
		FMath::FRandRange(-AppleSpawnExtent.Z, AppleSpawnExtent.Z));
	return GetActorTransform().TransformPosition(AppleSpawnCenter + LocalOffset);
}

FVector ATunaSweeperBreakableAppleCrateActor::BuildRandomAppleVelocity(const FVector& SpillDirection) const
{
	const float RandomAngleRadians = FMath::FRandRange(0.0f, UE_PI * 2.0f);
	const FVector RandomHorizontalDirection(FMath::Cos(RandomAngleRadians), FMath::Sin(RandomAngleRadians), 0.0f);
	const FVector WeightedDirection = GetSafeHorizontalDirection(
		SpillDirection + RandomHorizontalDirection * RandomScatterWeight,
		GetActorForwardVector());
	const float HorizontalSpeed = FMath::FRandRange(HorizontalSpeedRange.X, HorizontalSpeedRange.Y);
	const float VerticalSpeed = FMath::FRandRange(VerticalSpeedRange.X, VerticalSpeedRange.Y);

	return WeightedDirection * HorizontalSpeed + FVector::UpVector * VerticalSpeed;
}

FVector ATunaSweeperBreakableAppleCrateActor::BuildRandomAppleAngularVelocity() const
{
	const float SafeAngularSpeed = FMath::Max(0.0f, AngularSpeedDegrees);
	return FVector(
		FMath::FRandRange(-SafeAngularSpeed, SafeAngularSpeed),
		FMath::FRandRange(-SafeAngularSpeed, SafeAngularSpeed),
		FMath::FRandRange(-SafeAngularSpeed, SafeAngularSpeed));
}
