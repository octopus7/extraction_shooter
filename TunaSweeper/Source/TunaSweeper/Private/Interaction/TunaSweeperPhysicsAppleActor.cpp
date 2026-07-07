#include "Interaction/TunaSweeperPhysicsAppleActor.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "TunaSweeperCollisionChannels.h"

namespace
{
	const TCHAR* DefaultAppleMeshPath = TEXT("/Game/AXTemp/SM_Apple.SM_Apple");
}

ATunaSweeperPhysicsAppleActor::ATunaSweeperPhysicsAppleActor()
{
	PrimaryActorTick.bCanEverTick = false;

	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	CollisionComponent->InitSphereRadius(CollisionRadiusCm);
	CollisionComponent->SetNotifyRigidBodyCollision(false);
	CollisionComponent->SetCanEverAffectNavigation(false);
	RootComponent = CollisionComponent;

	AppleMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AppleMesh"));
	AppleMeshComponent->SetupAttachment(RootComponent);
	AppleMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	AppleMeshComponent->SetGenerateOverlapEvents(false);
	AppleMeshComponent->SetCanEverAffectNavigation(false);
	AppleMeshComponent->SetCastShadow(true);

	AppleMesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(DefaultAppleMeshPath));
}

void ATunaSweeperPhysicsAppleActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	CollisionRadiusCm = FMath::Max(1.0f, CollisionRadiusCm);
	VisualScale = FMath::Max(0.01f, VisualScale);
	MassKg = FMath::Max(0.01f, MassKg);
	LinearDamping = FMath::Max(0.0f, LinearDamping);
	AngularDamping = FMath::Max(0.0f, AngularDamping);
	LifeSeconds = FMath::Max(0.0f, LifeSeconds);

	ApplyAppleDefaults();
}

void ATunaSweeperPhysicsAppleActor::BeginPlay()
{
	Super::BeginPlay();

	ApplyAppleDefaults();
	if (LifeSeconds > 0.0f && GetWorld() && GetWorld()->IsGameWorld())
	{
		SetLifeSpan(LifeSeconds);
	}
}

void ATunaSweeperPhysicsAppleActor::ConfigurePhysicsAppleDefaults(
	const TSoftObjectPtr<UStaticMesh>& InAppleMesh,
	float InCollisionRadiusCm,
	float InVisualScale,
	float InLifeSeconds)
{
	if (!InAppleMesh.IsNull())
	{
		AppleMesh = InAppleMesh;
	}
	CollisionRadiusCm = FMath::Max(1.0f, InCollisionRadiusCm);
	VisualScale = FMath::Max(0.01f, InVisualScale);
	LifeSeconds = FMath::Max(0.0f, InLifeSeconds);

	ApplyAppleDefaults();
	if (LifeSeconds > 0.0f && GetWorld() && GetWorld()->IsGameWorld())
	{
		SetLifeSpan(LifeSeconds);
	}
}

void ATunaSweeperPhysicsAppleActor::LaunchApple(
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

void ATunaSweeperPhysicsAppleActor::ApplyAppleDefaults()
{
	const float SafeVisualScale = FMath::Max(0.01f, VisualScale);
	UStaticMesh* LoadedAppleMesh = AppleMesh.LoadSynchronous();
	float EffectiveCollisionRadius = FMath::Max(1.0f, CollisionRadiusCm);
	FVector VisualMeshOffset = FVector::ZeroVector;

	if (LoadedAppleMesh)
	{
		const FBoxSphereBounds MeshBounds = LoadedAppleMesh->GetBounds();
		if (bCenterMeshOnCollision)
		{
			VisualMeshOffset = -MeshBounds.Origin * SafeVisualScale;
		}
		if (bUseMeshBoundsForCollisionRadius)
		{
			EffectiveCollisionRadius = FMath::Max(
				EffectiveCollisionRadius,
				MeshBounds.BoxExtent.GetMax() * SafeVisualScale);
		}
	}

	if (CollisionComponent)
	{
		CollisionComponent->SetSphereRadius(EffectiveCollisionRadius, true);
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

	if (AppleMeshComponent)
	{
		AppleMeshComponent->SetStaticMesh(LoadedAppleMesh);
		AppleMeshComponent->SetRelativeLocation(VisualMeshOffset);
		AppleMeshComponent->SetRelativeRotation(FRotator::ZeroRotator);
		AppleMeshComponent->SetRelativeScale3D(FVector(SafeVisualScale));
		AppleMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		AppleMeshComponent->SetGenerateOverlapEvents(false);
		AppleMeshComponent->SetCanEverAffectNavigation(false);
		AppleMeshComponent->SetHiddenInGame(false);
		AppleMeshComponent->SetVisibility(true, true);
	}
}
