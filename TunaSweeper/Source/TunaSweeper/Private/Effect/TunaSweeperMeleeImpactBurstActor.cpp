#include "Effect/TunaSweeperMeleeImpactBurstActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr float BurstLifetimeSeconds = 0.24f;
	const TCHAR* BurstMaterialPath = TEXT("/Game/Prototype/M_Voxel_VertexColor.M_Voxel_VertexColor");
	const FLinearColor BurstColor(0.0f, 0.92f, 1.0f, 1.0f);

	struct FMeleeImpactBurstParticleConfig
	{
		FVector TargetLocation;
		FVector Scale;
	};

	const FMeleeImpactBurstParticleConfig BurstParticleConfigs[] =
	{
		{ FVector(18.0f, 0.0f, 0.0f), FVector(0.055f, 0.055f, 0.055f) },
		{ FVector(34.0f, -10.0f, 7.0f), FVector(0.042f, 0.042f, 0.042f) },
		{ FVector(42.0f, 12.0f, -5.0f), FVector(0.038f, 0.038f, 0.038f) },
		{ FVector(58.0f, -18.0f, 12.0f), FVector(0.032f, 0.032f, 0.032f) },
		{ FVector(66.0f, 16.0f, 5.0f), FVector(0.034f, 0.034f, 0.034f) },
		{ FVector(80.0f, -8.0f, -10.0f), FVector(0.026f, 0.026f, 0.026f) },
		{ FVector(88.0f, 8.0f, 14.0f), FVector(0.028f, 0.028f, 0.028f) }
	};

	void ConfigureBurstParticle(
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
		Particle->SetCastShadow(false);
		Particle->SetRelativeLocation(RelativeLocation);
		Particle->SetRelativeScale3D(RelativeScale);
		if (Mesh)
		{
			Particle->SetStaticMesh(Mesh);
		}
	}
}

ATunaSweeperMeleeImpactBurstActor::ATunaSweeperMeleeImpactBurstActor()
{
	PrimaryActorTick.bCanEverTick = true;
	InitialLifeSpan = BurstLifetimeSeconds;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	UStaticMesh* BurstMesh = SphereMesh.Succeeded() ? SphereMesh.Object : nullptr;

	for (int32 Index = 0; Index < UE_ARRAY_COUNT(BurstParticleConfigs); ++Index)
	{
		const FName ComponentName(*FString::Printf(TEXT("BurstParticle%d"), Index));
		UStaticMeshComponent* BurstParticle = CreateDefaultSubobject<UStaticMeshComponent>(ComponentName);
		BurstParticle->SetupAttachment(RootComponent);

		const FMeleeImpactBurstParticleConfig& Config = BurstParticleConfigs[Index];
		ConfigureBurstParticle(BurstParticle, BurstMesh, Config.TargetLocation * 0.25f, Config.Scale * 1.2f);
		BurstParticles.Add(BurstParticle);
		BurstTargetLocations.Add(Config.TargetLocation);
		BurstBaseScales.Add(Config.Scale);
	}
}

void ATunaSweeperMeleeImpactBurstActor::BeginPlay()
{
	Super::BeginPlay();

	UMaterialInterface* BurstMaterial = LoadObject<UMaterialInterface>(nullptr, BurstMaterialPath);
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

		UMaterialInstanceDynamic* DynamicMaterial = Particle->CreateAndSetMaterialInstanceDynamic(0);
		if (DynamicMaterial)
		{
			DynamicMaterial->SetVectorParameterValue(TEXT("Color"), BurstColor);
			DynamicMaterial->SetVectorParameterValue(TEXT("BaseColor"), BurstColor);
			DynamicMaterial->SetVectorParameterValue(TEXT("EmissiveColor"), BurstColor);
		}
	}
}

void ATunaSweeperMeleeImpactBurstActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	ElapsedLifetimeSeconds += DeltaSeconds;
	const float Alpha = FMath::Clamp(ElapsedLifetimeSeconds / BurstLifetimeSeconds, 0.0f, 1.0f);
	const float LocationScale = FMath::Lerp(0.25f, 1.15f, Alpha);
	const float ParticleScale = FMath::Lerp(1.2f, 0.15f, Alpha);

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
