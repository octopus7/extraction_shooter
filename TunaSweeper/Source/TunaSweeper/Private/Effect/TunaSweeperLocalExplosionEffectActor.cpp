#include "Effect/TunaSweeperLocalExplosionEffectActor.h"

#include "Component/TunaSweeperVisionSubjectComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	const TCHAR* DistortionMaterialPath = TEXT("/Game/Effects/M_LocalExplosionDistortion.M_LocalExplosionDistortion");
	constexpr float PlaneMeshSizeCm = 100.0f;

	float SmoothStep01(float Alpha)
	{
		const float ClampedAlpha = FMath::Clamp(Alpha, 0.0f, 1.0f);
		return ClampedAlpha * ClampedAlpha * (3.0f - 2.0f * ClampedAlpha);
	}

	float RangeAlpha(float Start, float End, float Value)
	{
		if (FMath::IsNearlyEqual(Start, End))
		{
			return Value >= End ? 1.0f : 0.0f;
		}

		return FMath::Clamp((Value - Start) / (End - Start), 0.0f, 1.0f);
	}

	float FadeOutAfter(float Start, float End, float Value)
	{
		return 1.0f - SmoothStep01(RangeAlpha(Start, End, Value));
	}
}

ATunaSweeperLocalExplosionEffectActor::ATunaSweeperLocalExplosionEffectActor()
{
	PrimaryActorTick.bCanEverTick = true;
	InitialLifeSpan = TotalDurationSeconds + 0.08f;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	DistortionSprite = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DistortionSprite"));
	DistortionSprite->SetupAttachment(RootComponent);
	ConfigureDistortionComponent();
	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneMesh(TEXT("/Engine/BasicShapes/Plane.Plane"));
	if (PlaneMesh.Succeeded())
	{
		DistortionSprite->SetStaticMesh(PlaneMesh.Object);
	}

	FlashLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("FlashLight"));
	FlashLight->SetupAttachment(RootComponent);
	FlashLight->SetRelativeLocation(FVector(0.0f, 0.0f, 52.0f));
	FlashLight->SetCastShadows(false);
	FlashLight->SetLightColor(FLinearColor(1.0f, 0.48f, 0.13f));
	FlashLight->SetIntensity(0.0f);
	FlashLight->SetAttenuationRadius(EffectRadiusCm * 2.0f);

	VisionSubjectComponent = CreateDefaultSubobject<UTunaSweeperVisionSubjectComponent>(TEXT("VisionSubject"));
	ExplosionDistortionMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(DistortionMaterialPath));
	FireBurstNiagaraSystem = TSoftObjectPtr<UNiagaraSystem>(FSoftObjectPath(TEXT("/Game/Effects/ExplosionTuna/NS_Explosion_Tuna.NS_Explosion_Tuna")));
}

void ATunaSweeperLocalExplosionEffectActor::BeginPlay()
{
	Super::BeginPlay();
	ApplyDynamicMaterials();
	SpawnNiagaraBurstEffect();
	SetLifeSpan(FMath::Max(0.08f, TotalDurationSeconds + 0.08f));
	UpdateEffect(0.0f);
}

void ATunaSweeperLocalExplosionEffectActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateEffect(DeltaSeconds);
}

void ATunaSweeperLocalExplosionEffectActor::ConfigureExplosion(float InRadiusCm, float InDurationSeconds)
{
	EffectRadiusCm = FMath::Max(35.0f, InRadiusCm);
	TotalDurationSeconds = FMath::Max(0.18f, InDurationSeconds);
	SetLifeSpan(TotalDurationSeconds + 0.08f);
}

void ATunaSweeperLocalExplosionEffectActor::ConfigureDistortionComponent()
{
	if (!DistortionSprite)
	{
		return;
	}

	DistortionSprite->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	DistortionSprite->SetGenerateOverlapEvents(false);
	DistortionSprite->SetCastShadow(false);
	DistortionSprite->SetCanEverAffectNavigation(false);
	DistortionSprite->SetTranslucentSortPriority(0);
}

void ATunaSweeperLocalExplosionEffectActor::ApplyDynamicMaterials()
{
	if (UMaterialInterface* LoadedDistortionMaterial = ExplosionDistortionMaterial.LoadSynchronous())
	{
		if (DistortionSprite)
		{
			DistortionDynamicMaterial = UMaterialInstanceDynamic::Create(LoadedDistortionMaterial, this);
			DistortionSprite->SetMaterial(0, DistortionDynamicMaterial);
		}
	}
}

void ATunaSweeperLocalExplosionEffectActor::UpdateDistortionMaterial(
	float WavePosition,
	float WaveWidth,
	float DistortionStrength,
	float RefractionAmount,
	float Opacity) const
{
	if (!DistortionDynamicMaterial)
	{
		return;
	}

	DistortionDynamicMaterial->SetScalarParameterValue(TEXT("WavePosition"), FMath::Clamp(WavePosition, 0.0f, 1.0f));
	DistortionDynamicMaterial->SetScalarParameterValue(TEXT("WaveWidth"), FMath::Max(0.01f, WaveWidth));
	DistortionDynamicMaterial->SetScalarParameterValue(TEXT("DistortionStrength"), FMath::Max(0.0f, DistortionStrength));
	DistortionDynamicMaterial->SetScalarParameterValue(TEXT("RefractionAmount"), FMath::Max(1.0f, RefractionAmount));
	DistortionDynamicMaterial->SetScalarParameterValue(TEXT("Opacity"), FMath::Clamp(Opacity, 0.0f, 1.0f));
}

void ATunaSweeperLocalExplosionEffectActor::SetSpriteDiameter(float DiameterCm) const
{
	if (DistortionSprite)
	{
		const float Scale = FMath::Max(0.0f, DiameterCm) / PlaneMeshSizeCm;
		DistortionSprite->SetRelativeScale3D(FVector(Scale, Scale, 1.0f));
	}
}

void ATunaSweeperLocalExplosionEffectActor::SpawnNiagaraBurstEffect()
{
	if (!RootComponent)
	{
		return;
	}

	if (UNiagaraSystem* FireSystem = FireBurstNiagaraSystem.LoadSynchronous())
	{
		UNiagaraFunctionLibrary::SpawnSystemAttached(
			FireSystem,
			RootComponent,
			NAME_None,
			FVector(0.0f, 0.0f, SpriteBaseHeightCm),
			FRotator::ZeroRotator,
			FVector(EffectRadiusCm / 210.0f),
			EAttachLocation::KeepRelativeOffset,
			true,
			ENCPoolMethod::AutoRelease,
			true,
			false);
	}
}

void ATunaSweeperLocalExplosionEffectActor::UpdateEffect(float DeltaSeconds)
{
	ElapsedSeconds += FMath::Max(0.0f, DeltaSeconds);
	const float NormalizedTime = FMath::Clamp(ElapsedSeconds / FMath::Max(0.01f, TotalDurationSeconds), 0.0f, 1.0f);
	const float DistortionTravel = SmoothStep01(RangeAlpha(0.0f, 0.24f, NormalizedTime));
	const float DistortionFade = FadeOutAfter(0.02f, 0.28f, NormalizedTime);
	if (DistortionSprite)
	{
		DistortionSprite->SetRelativeLocation(FVector(0.0f, 0.0f, SpriteBaseHeightCm + 4.0f));
		SetSpriteDiameter(FMath::Lerp(EffectRadiusCm * 0.45f, EffectRadiusCm * 3.05f, DistortionTravel));
		DistortionSprite->SetRelativeRotation(FRotator(0.0f, 0.0f, NormalizedTime * 95.0f));
		DistortionSprite->SetVisibility(DistortionFade > 0.01f);
	}
	UpdateDistortionMaterial(
		FMath::Lerp(0.14f, 0.72f, DistortionTravel),
		FMath::Lerp(0.16f, 0.07f, DistortionTravel),
		0.42f * DistortionFade,
		1.0f + 0.55f * DistortionFade,
		0.34f * DistortionFade);

	if (FlashLight)
	{
		const float LightFade = FadeOutAfter(0.0f, 0.22f, NormalizedTime);
		FlashLight->SetIntensity(4200.0f * LightFade);
		FlashLight->SetAttenuationRadius(EffectRadiusCm * FMath::Lerp(1.1f, 2.35f, SmoothStep01(RangeAlpha(0.0f, 0.22f, NormalizedTime))));
	}
}
