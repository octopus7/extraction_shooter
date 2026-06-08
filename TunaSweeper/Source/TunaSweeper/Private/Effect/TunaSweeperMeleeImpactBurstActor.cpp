#include "Effect/TunaSweeperMeleeImpactBurstActor.h"

#include "Component/TunaSweeperVisionSubjectComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr float BurstLifetimeSeconds = 0.55f;
	constexpr int32 BurstParticleDensityMultiplier = 3;
	const TCHAR* BurstMaterialPath = TEXT("/Game/Prototype/M_Voxel_VertexColor.M_Voxel_VertexColor");

	struct FMeleeImpactBurstParticleConfig
	{
		FVector TargetLocation;
		FVector Scale;
	};

	const FMeleeImpactBurstParticleConfig BurstParticleConfigs[] =
	{
		{ FVector(12.0f, 0.0f, 0.0f), FVector(0.18f, 0.18f, 0.18f) },
		{ FVector(34.0f, -14.0f, 8.0f), FVector(0.14f, 0.14f, 0.14f) },
		{ FVector(44.0f, 16.0f, -6.0f), FVector(0.13f, 0.13f, 0.13f) },
		{ FVector(68.0f, -26.0f, 18.0f), FVector(0.11f, 0.11f, 0.11f) },
		{ FVector(78.0f, 28.0f, 8.0f), FVector(0.115f, 0.115f, 0.115f) },
		{ FVector(98.0f, -18.0f, -14.0f), FVector(0.095f, 0.095f, 0.095f) },
		{ FVector(112.0f, 20.0f, 22.0f), FVector(0.10f, 0.10f, 0.10f) },
		{ FVector(126.0f, -34.0f, 6.0f), FVector(0.085f, 0.085f, 0.085f) },
		{ FVector(138.0f, 30.0f, -18.0f), FVector(0.09f, 0.09f, 0.09f) },
		{ FVector(148.0f, -6.0f, 28.0f), FVector(0.08f, 0.08f, 0.08f) }
	};

	FMeleeImpactBurstParticleConfig BuildBurstParticleVariant(
		const FMeleeImpactBurstParticleConfig& Config,
		int32 VariantIndex)
	{
		if (VariantIndex <= 0)
		{
			return Config;
		}

		const float VariantSign = VariantIndex % 2 == 0 ? -1.0f : 1.0f;
		const float VariantStrength = static_cast<float>((VariantIndex + 1) / 2);
		const FVector SpreadOffset(
			0.0f,
			VariantSign * (8.0f + VariantStrength * 3.0f),
			-VariantSign * (5.0f + VariantStrength * 2.0f));
		const float ScaleMultiplier = FMath::Max(0.65f, 1.0f - VariantStrength * 0.08f);

		return FMeleeImpactBurstParticleConfig
		{
			Config.TargetLocation + SpreadOffset,
			Config.Scale * ScaleMultiplier
		};
	}

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
		for (int32 VariantIndex = 0; VariantIndex < BurstParticleDensityMultiplier; ++VariantIndex)
		{
			const FName ComponentName(*FString::Printf(TEXT("BurstParticle%d_%d"), Index, VariantIndex));
			UStaticMeshComponent* BurstParticle = CreateDefaultSubobject<UStaticMeshComponent>(ComponentName);
			BurstParticle->SetupAttachment(RootComponent);

			const FMeleeImpactBurstParticleConfig Config =
				BuildBurstParticleVariant(BurstParticleConfigs[Index], VariantIndex);
			ConfigureBurstParticle(BurstParticle, BurstMesh, Config.TargetLocation * 0.18f, Config.Scale * 1.65f);
			BurstParticles.Add(BurstParticle);
			BurstTargetLocations.Add(Config.TargetLocation);
			BurstBaseScales.Add(Config.Scale);
		}
	}

	VisionSubjectComponent = CreateDefaultSubobject<UTunaSweeperVisionSubjectComponent>(TEXT("VisionSubject"));
}

void ATunaSweeperMeleeImpactBurstActor::BeginPlay()
{
	Super::BeginPlay();

	BurstDynamicMaterials.Reset();

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
			BurstDynamicMaterials.Add(DynamicMaterial);
		}
	}

	ApplyBurstColorToMaterials();
}

void ATunaSweeperMeleeImpactBurstActor::SetBurstColor(const FLinearColor& InBurstColor)
{
	BurstColor = InBurstColor;
	ApplyBurstColorToMaterials();
}

void ATunaSweeperMeleeImpactBurstActor::ApplyBurstColorToMaterials()
{
	for (UMaterialInstanceDynamic* DynamicMaterial : BurstDynamicMaterials)
	{
		if (!DynamicMaterial)
		{
			continue;
		}

		DynamicMaterial->SetVectorParameterValue(TEXT("Color"), BurstColor);
		DynamicMaterial->SetVectorParameterValue(TEXT("BaseColor"), BurstColor);
		DynamicMaterial->SetVectorParameterValue(TEXT("EmissiveColor"), BurstColor);
	}
}

void ATunaSweeperMeleeImpactBurstActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	ElapsedLifetimeSeconds += DeltaSeconds;
	const float Alpha = FMath::Clamp(ElapsedLifetimeSeconds / BurstLifetimeSeconds, 0.0f, 1.0f);
	const float LocationScale = FMath::Lerp(0.18f, 1.35f, Alpha);
	const float ParticleScale = FMath::Lerp(1.65f, 0.35f, Alpha);

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
