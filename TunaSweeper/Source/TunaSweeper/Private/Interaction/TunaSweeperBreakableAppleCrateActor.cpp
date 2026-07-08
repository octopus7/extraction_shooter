#include "Interaction/TunaSweeperBreakableAppleCrateActor.h"

#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DamageEvents.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Field/FieldSystemObjects.h"
#include "Chaos/Particle/ObjectState.h"
#include "GeometryCollection/GeometryCollectionComponent.h"
#include "GeometryCollection/GeometryCollectionObject.h"
#include "GeometryCollection/GeometryCollectionSimulationTypes.h"
#include "Interaction/TunaSweeperPhysicsCrateFragmentActor.h"
#include "Interaction/TunaSweeperPhysicsAppleActor.h"
#include "Materials/MaterialInterface.h"
#include "TunaSweeperCollisionChannels.h"

namespace
{
	const TCHAR* DefaultCrateMeshPath = TEXT("/Game/Nature/Wood/SM_CrateB.SM_CrateB");
	const TCHAR* DefaultCrateGeometryCollectionPath = TEXT("/Game/Interaction/GC_CrateB_Fractured.GC_CrateB_Fractured");
	const TCHAR* DefaultAppleMeshPath = TEXT("/Game/AXTemp/SM_Apple.SM_Apple");
	const TCHAR* DefaultCrateFragmentMeshPath = TEXT("/Engine/BasicShapes/Cube.Cube");

	FVector MakeSafeCrateExtent(const FVector& Extent)
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

	CrateGeometryCollectionComponent =
		CreateDefaultSubobject<UGeometryCollectionComponent>(TEXT("CrateGeometryCollection"));
	CrateGeometryCollectionComponent->SetupAttachment(RootComponent);
	CrateGeometryCollectionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CrateGeometryCollectionComponent->SetGenerateOverlapEvents(false);
	CrateGeometryCollectionComponent->SetCanEverAffectNavigation(false);
	CrateGeometryCollectionComponent->SetHiddenInGame(true);
	CrateGeometryCollectionComponent->SetVisibility(false, true);
	CrateGeometryCollectionComponent->SetNotifyBreaks(false);
	CrateGeometryCollectionComponent->ObjectType = EObjectStateTypeEnum::Chaos_Object_Dynamic;
	CrateGeometryCollectionComponent->EnableClustering = false;
	CrateGeometryCollectionComponent->MaxClusterLevel = 0;

	CrateMesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(DefaultCrateMeshPath));
	CrateGeometryCollection = TSoftObjectPtr<UGeometryCollection>(FSoftObjectPath(DefaultCrateGeometryCollectionPath));
	AppleMesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(DefaultAppleMeshPath));
	CrateFragmentMesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(DefaultCrateFragmentMeshPath));
	AppleActorClass = ATunaSweeperPhysicsAppleActor::StaticClass();
	CrateFragmentActorClass = ATunaSweeperPhysicsCrateFragmentActor::StaticClass();
}

void ATunaSweeperBreakableAppleCrateActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	MaxHealth = FMath::Max(1.0f, MaxHealth);
	CurrentHealth = bCrateBroken ? 0.0f : MaxHealth;
	CollisionExtent = MakeSafeCrateExtent(CollisionExtent);
	AppleSpawnExtent = MakeSafeCrateExtent(AppleSpawnExtent);
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
	GeometryCollectionBreakRadius = FMath::Max(0.0f, GeometryCollectionBreakRadius);
	GeometryCollectionRadialImpulse = FMath::Max(0.0f, GeometryCollectionRadialImpulse);
	GeometryCollectionDirectionalImpulse = FMath::Max(0.0f, GeometryCollectionDirectionalImpulse);
	GeometryCollectionUpwardImpulse = FMath::Max(0.0f, GeometryCollectionUpwardImpulse);
	GeometryCollectionDamageThreshold = FMath::Max(0.0f, GeometryCollectionDamageThreshold);
	GeometryCollectionExternalClusterStrain = FMath::Max(0.0f, GeometryCollectionExternalClusterStrain);
	MinCrateFragmentCount = FMath::Max(0, MinCrateFragmentCount);
	MaxCrateFragmentCount = FMath::Max(MinCrateFragmentCount, MaxCrateFragmentCount);
	CrateFragmentLifeSeconds = FMath::Max(0.0f, CrateFragmentLifeSeconds);
	FragmentThicknessRange.X = FMath::Max(0.5f, FragmentThicknessRange.X);
	FragmentThicknessRange.Y = FMath::Max(FragmentThicknessRange.X, FragmentThicknessRange.Y);
	FragmentWidthRange.X = FMath::Max(0.5f, FragmentWidthRange.X);
	FragmentWidthRange.Y = FMath::Max(FragmentWidthRange.X, FragmentWidthRange.Y);
	FragmentLengthRange.X = FMath::Max(0.5f, FragmentLengthRange.X);
	FragmentLengthRange.Y = FMath::Max(FragmentLengthRange.X, FragmentLengthRange.Y);
	FragmentHorizontalSpeedRange.X = FMath::Max(0.0f, FragmentHorizontalSpeedRange.X);
	FragmentHorizontalSpeedRange.Y = FMath::Max(FragmentHorizontalSpeedRange.X, FragmentHorizontalSpeedRange.Y);
	FragmentVerticalSpeedRange.X = FMath::Max(0.0f, FragmentVerticalSpeedRange.X);
	FragmentVerticalSpeedRange.Y = FMath::Max(FragmentVerticalSpeedRange.X, FragmentVerticalSpeedRange.Y);
	FragmentRandomScatterWeight = FMath::Max(0.0f, FragmentRandomScatterWeight);
	FragmentAngularSpeedDegrees = FMath::Max(0.0f, FragmentAngularSpeedDegrees);

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
	const TSoftObjectPtr<UGeometryCollection>& InCrateGeometryCollection,
	TSubclassOf<ATunaSweeperPhysicsAppleActor> InAppleActorClass,
	const TSoftObjectPtr<UStaticMesh>& InAppleMesh,
	TSubclassOf<ATunaSweeperPhysicsCrateFragmentActor> InCrateFragmentActorClass,
	const TSoftObjectPtr<UStaticMesh>& InCrateFragmentMesh)
{
	CrateId = InCrateId;
	MaxHealth = FMath::Max(1.0f, InMaxHealth);
	CurrentHealth = bCrateBroken ? 0.0f : MaxHealth;
	if (!InCrateMesh.IsNull())
	{
		CrateMesh = InCrateMesh;
	}
	if (!InCrateGeometryCollection.IsNull())
	{
		CrateGeometryCollection = InCrateGeometryCollection;
	}
	if (InAppleActorClass)
	{
		AppleActorClass = InAppleActorClass;
	}
	if (!InAppleMesh.IsNull())
	{
		AppleMesh = InAppleMesh;
	}
	if (InCrateFragmentActorClass)
	{
		CrateFragmentActorClass = InCrateFragmentActorClass;
	}
	if (!InCrateFragmentMesh.IsNull())
	{
		CrateFragmentMesh = InCrateFragmentMesh;
	}
	bUseGeometryCollectionOnBreak = true;
	bSpawnCrateFragmentsOnBreak = false;
	GeometryCollectionBreakRadius = 95.0f;
	GeometryCollectionRadialImpulse = 120.0f;
	GeometryCollectionDirectionalImpulse = 2400.0f;
	GeometryCollectionUpwardImpulse = 350.0f;
	GeometryCollectionDamageThreshold = 0.0f;
	GeometryCollectionExternalClusterStrain = 50000.0f;

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

	if (CrateGeometryCollectionComponent)
	{
		UGeometryCollection* LoadedGeometryCollection = CrateGeometryCollection.LoadSynchronous();
		if (CrateGeometryCollectionComponent->GetRestCollection() != LoadedGeometryCollection)
		{
			CrateGeometryCollectionComponent->SetRestCollection(LoadedGeometryCollection, true);
		}

		const bool bShowBrokenCollection =
			bCrateBroken && bUseGeometryCollectionOnBreak && LoadedGeometryCollection != nullptr;
		CrateGeometryCollectionComponent->SetRelativeLocation(FVector::ZeroVector);
		CrateGeometryCollectionComponent->SetRelativeRotation(FRotator::ZeroRotator);
		CrateGeometryCollectionComponent->SetRelativeScale3D(FVector::OneVector);
		CrateGeometryCollectionComponent->ObjectType = EObjectStateTypeEnum::Chaos_Object_Dynamic;
		CrateGeometryCollectionComponent->EnableClustering = false;
		CrateGeometryCollectionComponent->MaxClusterLevel = 0;
		CrateGeometryCollectionComponent->SetDamageThreshold({ FMath::Max(0.0f, GeometryCollectionDamageThreshold) });
		CrateGeometryCollectionComponent->SetCollisionObjectType(ECC_WorldDynamic);
		CrateGeometryCollectionComponent->SetCollisionResponseToAllChannels(ECR_Block);
		CrateGeometryCollectionComponent->SetGenerateOverlapEvents(false);
		CrateGeometryCollectionComponent->SetCanEverAffectNavigation(false);
		CrateGeometryCollectionComponent->SetHiddenInGame(!bShowBrokenCollection);
		CrateGeometryCollectionComponent->SetVisibility(bShowBrokenCollection, true);
		CrateGeometryCollectionComponent->SetCollisionEnabled(
			bShowBrokenCollection ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
		CrateGeometryCollectionComponent->SetSimulatePhysics(bShowBrokenCollection);
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
	if (!BreakGeometryCollection(SpillDirection))
	{
		SpawnCrateFragments(SpillDirection);
	}
	SpawnApples(SpillDirection);
}

bool ATunaSweeperBreakableAppleCrateActor::BreakGeometryCollection(const FVector& SpillDirection)
{
	if (!bUseGeometryCollectionOnBreak || !CrateGeometryCollectionComponent ||
		!CrateGeometryCollectionComponent->GetRestCollection())
	{
		return false;
	}

	const FVector BreakCenter = GetCrateCenterWorldLocation();
	const FVector SafeSpillDirection = GetSafeHorizontalDirection(SpillDirection, GetActorForwardVector());

	CrateGeometryCollectionComponent->SetHiddenInGame(false);
	CrateGeometryCollectionComponent->SetVisibility(true, true);
	CrateGeometryCollectionComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CrateGeometryCollectionComponent->SetSimulatePhysics(true);
	CrateGeometryCollectionComponent->SetDynamicState(Chaos::EObjectStateType::Dynamic);
	CrateGeometryCollectionComponent->ApplyKinematicField(GeometryCollectionBreakRadius, BreakCenter);

	const float StrainRadius = FMath::Max(
		GeometryCollectionBreakRadius,
		FMath::Max3(CollisionExtent.X, CollisionExtent.Y, CollisionExtent.Z) * 2.25f);
	UFieldSystemMetaDataFilter* StrainMetaData = NewObject<UFieldSystemMetaDataFilter>(this);
	URadialFalloff* StrainField = NewObject<URadialFalloff>(this);
	if (StrainMetaData && StrainField)
	{
		StrainMetaData->SetMetaDataFilterType(
			Field_Filter_All,
			Field_Object_Destruction,
			Field_Position_CenterOfMass);
		StrainField->SetRadialFalloff(
			FMath::Max(GeometryCollectionExternalClusterStrain, GeometryCollectionDamageThreshold * 10.0f),
			0.0f,
			1.0f,
			0.0f,
			StrainRadius,
			BreakCenter,
			Field_Falloff_Linear);
		CrateGeometryCollectionComponent->ApplyPhysicsField(
			true,
			EGeometryCollectionPhysicsTypeEnum::Chaos_ExternalClusterStrain,
			StrainMetaData,
			StrainField);
	}

	CrateGeometryCollectionComponent->CrumbleActiveClusters();
	CrateGeometryCollectionComponent->AddRadialImpulse(
		BreakCenter,
		GeometryCollectionBreakRadius,
		GeometryCollectionRadialImpulse,
		RIF_Linear,
		false);
	CrateGeometryCollectionComponent->AddImpulseAtLocation(
		SafeSpillDirection * GeometryCollectionDirectionalImpulse +
			FVector::UpVector * GeometryCollectionUpwardImpulse,
		BreakCenter);

	return true;
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

FVector ATunaSweeperBreakableAppleCrateActor::GetCrateCenterWorldLocation() const
{
	return GetActorTransform().TransformPosition(CollisionCenterOffset);
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

void ATunaSweeperBreakableAppleCrateActor::SpawnCrateFragments(const FVector& SpillDirection)
{
	if (!bSpawnCrateFragmentsOnBreak)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	TSubclassOf<ATunaSweeperPhysicsCrateFragmentActor> LoadedFragmentClass = CrateFragmentActorClass;
	if (!LoadedFragmentClass)
	{
		LoadedFragmentClass = ATunaSweeperPhysicsCrateFragmentActor::StaticClass();
	}

	UMaterialInterface* FragmentMaterial = ResolveCrateFragmentMaterial();
	const int32 FragmentCount = FMath::RandRange(
		FMath::Max(0, MinCrateFragmentCount),
		FMath::Max(MinCrateFragmentCount, MaxCrateFragmentCount));
	for (int32 FragmentIndex = 0; FragmentIndex < FragmentCount; ++FragmentIndex)
	{
		const FVector SpawnLocation = BuildRandomCrateFragmentSpawnLocation();
		const FRotator SpawnRotation(
			FMath::FRandRange(-180.0f, 180.0f),
			FMath::FRandRange(-180.0f, 180.0f),
			FMath::FRandRange(-180.0f, 180.0f));

		FActorSpawnParameters SpawnParameters;
		SpawnParameters.Owner = this;
		SpawnParameters.Instigator = GetInstigator();
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		ATunaSweeperPhysicsCrateFragmentActor* FragmentActor =
			World->SpawnActor<ATunaSweeperPhysicsCrateFragmentActor>(
				LoadedFragmentClass,
				SpawnLocation,
				SpawnRotation,
				SpawnParameters);
		if (!FragmentActor)
		{
			continue;
		}

		FragmentActor->ConfigureCrateFragmentDefaults(
			CrateFragmentMesh,
			FragmentMaterial,
			BuildRandomCrateFragmentHalfExtent(),
			CrateFragmentLifeSeconds);
		FragmentActor->LaunchFragment(
			BuildRandomCrateFragmentVelocity(SpillDirection),
			BuildRandomCrateFragmentAngularVelocity());
	}
}

FVector ATunaSweeperBreakableAppleCrateActor::BuildRandomCrateFragmentSpawnLocation() const
{
	const FVector LocalOffset(
		FMath::FRandRange(-CollisionExtent.X * 0.75f, CollisionExtent.X * 0.75f),
		FMath::FRandRange(-CollisionExtent.Y * 0.75f, CollisionExtent.Y * 0.75f),
		FMath::FRandRange(-CollisionExtent.Z * 0.75f, CollisionExtent.Z * 0.75f));
	return GetActorTransform().TransformPosition(CollisionCenterOffset + LocalOffset);
}

FVector ATunaSweeperBreakableAppleCrateActor::BuildRandomCrateFragmentHalfExtent() const
{
	const float Thickness = FMath::FRandRange(FragmentThicknessRange.X, FragmentThicknessRange.Y);
	const float Width = FMath::FRandRange(FragmentWidthRange.X, FragmentWidthRange.Y);
	const float Length = FMath::FRandRange(FragmentLengthRange.X, FragmentLengthRange.Y);

	switch (FMath::RandRange(0, 2))
	{
	case 0:
		return FVector(Length, Width, Thickness);
	case 1:
		return FVector(Width, Length, Thickness);
	default:
		return FVector(Thickness, Width, Length);
	}
}

FVector ATunaSweeperBreakableAppleCrateActor::BuildRandomCrateFragmentVelocity(const FVector& SpillDirection) const
{
	const float RandomAngleRadians = FMath::FRandRange(0.0f, UE_PI * 2.0f);
	const FVector RandomHorizontalDirection(FMath::Cos(RandomAngleRadians), FMath::Sin(RandomAngleRadians), 0.0f);
	const FVector WeightedDirection = GetSafeHorizontalDirection(
		SpillDirection + RandomHorizontalDirection * FragmentRandomScatterWeight,
		GetActorForwardVector());
	const float HorizontalSpeed = FMath::FRandRange(FragmentHorizontalSpeedRange.X, FragmentHorizontalSpeedRange.Y);
	const float VerticalSpeed = FMath::FRandRange(FragmentVerticalSpeedRange.X, FragmentVerticalSpeedRange.Y);

	return WeightedDirection * HorizontalSpeed + FVector::UpVector * VerticalSpeed;
}

FVector ATunaSweeperBreakableAppleCrateActor::BuildRandomCrateFragmentAngularVelocity() const
{
	const float SafeAngularSpeed = FMath::Max(0.0f, FragmentAngularSpeedDegrees);
	return FVector(
		FMath::FRandRange(-SafeAngularSpeed, SafeAngularSpeed),
		FMath::FRandRange(-SafeAngularSpeed, SafeAngularSpeed),
		FMath::FRandRange(-SafeAngularSpeed, SafeAngularSpeed));
}

UMaterialInterface* ATunaSweeperBreakableAppleCrateActor::ResolveCrateFragmentMaterial() const
{
	if (CrateMeshComponent)
	{
		if (UMaterialInterface* ComponentMaterial = CrateMeshComponent->GetMaterial(0))
		{
			return ComponentMaterial;
		}
	}

	UStaticMesh* LoadedCrateMesh = CrateMesh.LoadSynchronous();
	if (LoadedCrateMesh && LoadedCrateMesh->GetStaticMaterials().Num() > 0)
	{
		return LoadedCrateMesh->GetStaticMaterials()[0].MaterialInterface.Get();
	}

	return nullptr;
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
