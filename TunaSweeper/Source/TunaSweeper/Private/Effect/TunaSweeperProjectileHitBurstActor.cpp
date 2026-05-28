#include "Effect/TunaSweeperProjectileHitBurstActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr float ProjectileHitBurstLifetimeSeconds = 0.42f;
	const TCHAR* ProjectileHitBurstMaterialPath = TEXT("/Game/Effects/M_LedExpression_VertexColorEmissive.M_LedExpression_VertexColorEmissive");

	struct FProjectileHitBurstParticleConfig
	{
		FVector TargetLocation;
		FVector Scale;
	};

	const FProjectileHitBurstParticleConfig ProjectileHitBurstParticleConfigs[] =
	{
		{ FVector(18.0f, 0.0f, 0.0f), FVector(0.09f, 0.09f, 0.09f) },
		{ FVector(24.0f, 8.0f, 4.0f), FVector(0.075f, 0.075f, 0.075f) },
		{ FVector(24.0f, -8.0f, -3.0f), FVector(0.075f, 0.075f, 0.075f) },
		{ FVector(30.0f, 0.0f, 9.0f), FVector(0.07f, 0.07f, 0.07f) },
		{ FVector(32.0f, 12.0f, -7.0f), FVector(0.06f, 0.06f, 0.06f) },
		{ FVector(32.0f, -12.0f, 7.0f), FVector(0.06f, 0.06f, 0.06f) },
		{ FVector(40.0f, 5.0f, -11.0f), FVector(0.052f, 0.052f, 0.052f) },
		{ FVector(40.0f, -5.0f, 11.0f), FVector(0.052f, 0.052f, 0.052f) }
	};

	void ConfigureProjectileHitBurstParticle(
		UStaticMeshComponent* Particle,
		UStaticMesh* Mesh,
		const FVector& RelativeLocation,
		const FVector& RelativeScale)
	{
		if (!Particle)
		{
			return;
		}

		Particle->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Particle->SetGenerateOverlapEvents(false);
		Particle->SetCastShadow(false);
		Particle->SetRelativeLocation(RelativeLocation);
		Particle->SetRelativeScale3D(RelativeScale);
		if (Mesh)
		{
			Particle->SetStaticMesh(Mesh);
		}
	}
}

ATunaSweeperProjectileHitBurstActor::ATunaSweeperProjectileHitBurstActor()
{
	PrimaryActorTick.bCanEverTick = true;
	InitialLifeSpan = ProjectileHitBurstLifetimeSeconds;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	UStaticMesh* BurstMesh = SphereMesh.Succeeded() ? SphereMesh.Object : nullptr;

	for (int32 Index = 0; Index < UE_ARRAY_COUNT(ProjectileHitBurstParticleConfigs); ++Index)
	{
		const FName ComponentName(*FString::Printf(TEXT("HitBurstParticle%d"), Index));
		UStaticMeshComponent* BurstParticle = CreateDefaultSubobject<UStaticMeshComponent>(ComponentName);
		BurstParticle->SetupAttachment(RootComponent);
		ConfigureProjectileHitBurstParticle(
			BurstParticle,
			BurstMesh,
			ProjectileHitBurstParticleConfigs[Index].TargetLocation * 0.2f,
			ProjectileHitBurstParticleConfigs[Index].Scale * 1.45f);
		BurstParticles.Add(BurstParticle);
		BurstTargetLocations.Add(ProjectileHitBurstParticleConfigs[Index].TargetLocation);
		BurstBaseScales.Add(ProjectileHitBurstParticleConfigs[Index].Scale);
	}
}

void ATunaSweeperProjectileHitBurstActor::BeginPlay()
{
	Super::BeginPlay();

	BurstDynamicMaterials.Reset();

	UMaterialInterface* BurstMaterial = LoadObject<UMaterialInterface>(nullptr, ProjectileHitBurstMaterialPath);
	for (UStaticMeshComponent* Particle : BurstParticles)
	{
		if (!Particle)
		{
			continue;
		}

		if (BurstMaterial)
		{
			Particle->SetMaterial(0, BurstMaterial);
		}

		if (UMaterialInstanceDynamic* DynamicMaterial = Particle->CreateAndSetMaterialInstanceDynamic(0))
		{
			BurstDynamicMaterials.Add(DynamicMaterial);
		}
	}

	ApplyBurstColorToMaterials();
}

void ATunaSweeperProjectileHitBurstActor::SetBurstColor(const FLinearColor& InBurstColor)
{
	BurstColor = InBurstColor;
	ApplyBurstColorToMaterials();
}

void ATunaSweeperProjectileHitBurstActor::ApplyBurstColorToMaterials()
{
	const FLinearColor EmissiveColor = BurstColor * 6.0f;
	for (UMaterialInstanceDynamic* DynamicMaterial : BurstDynamicMaterials)
	{
		if (!DynamicMaterial)
		{
			continue;
		}

		DynamicMaterial->SetVectorParameterValue(TEXT("Color"), BurstColor);
		DynamicMaterial->SetVectorParameterValue(TEXT("BaseColor"), BurstColor);
		DynamicMaterial->SetVectorParameterValue(TEXT("Base Color"), BurstColor);
		DynamicMaterial->SetVectorParameterValue(TEXT("LedColor"), BurstColor);
		DynamicMaterial->SetVectorParameterValue(TEXT("EmissiveColor"), EmissiveColor);
		DynamicMaterial->SetVectorParameterValue(TEXT("Emissive Color"), EmissiveColor);
		DynamicMaterial->SetScalarParameterValue(TEXT("EmissiveStrength"), 6.0f);
		DynamicMaterial->SetScalarParameterValue(TEXT("Emissive Strength"), 6.0f);
		DynamicMaterial->SetScalarParameterValue(TEXT("Intensity"), 6.0f);
	}
}

void ATunaSweeperProjectileHitBurstActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	ElapsedLifetimeSeconds += DeltaSeconds;
	const float Alpha = FMath::Clamp(ElapsedLifetimeSeconds / ProjectileHitBurstLifetimeSeconds, 0.0f, 1.0f);
	const float LocationScale = FMath::Lerp(0.2f, 1.2f, Alpha);
	const float ParticleScale = FMath::Lerp(1.45f, 0.25f, Alpha);

	for (int32 Index = 0; Index < BurstParticles.Num(); ++Index)
	{
		UStaticMeshComponent* Particle = BurstParticles[Index];
		if (!Particle || !BurstTargetLocations.IsValidIndex(Index) || !BurstBaseScales.IsValidIndex(Index))
		{
			continue;
		}

		Particle->SetRelativeLocation(BurstTargetLocations[Index] * LocationScale);
		Particle->SetRelativeScale3D(BurstBaseScales[Index] * ParticleScale);
	}
}
