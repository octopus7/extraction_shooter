#include "Effect/TunaSweeperLocalExplosionEffectActor.h"

#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	const TCHAR* ExplosionMaterialPath = TEXT("/Game/Effects/M_LocalExplosionFlipbook.M_LocalExplosionFlipbook");
	const TCHAR* DistortionMaterialPath = TEXT("/Game/Effects/M_LocalExplosionDistortion.M_LocalExplosionDistortion");
	const TCHAR* SmokeMaterialPath = TEXT("/Game/Effects/M_LocalExplosionSmoke.M_LocalExplosionSmoke");
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

	ShockwaveSprite = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShockwaveSprite"));
	ShockwaveSprite->SetupAttachment(RootComponent);

	FireSprite = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FireSprite"));
	FireSprite->SetupAttachment(RootComponent);

	SmokeSprite = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SmokeSprite"));
	SmokeSprite->SetupAttachment(RootComponent);

	SmokeOffsetSpriteA = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SmokeOffsetSpriteA"));
	SmokeOffsetSpriteA->SetupAttachment(RootComponent);

	SmokeOffsetSpriteB = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SmokeOffsetSpriteB"));
	SmokeOffsetSpriteB->SetupAttachment(RootComponent);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneMesh(TEXT("/Engine/BasicShapes/Plane.Plane"));
	UStaticMesh* SpriteMesh = PlaneMesh.Succeeded() ? PlaneMesh.Object : nullptr;
	ConfigureSpriteComponent(DistortionSprite, 0);
	ConfigureSpriteComponent(ShockwaveSprite, 1);
	ConfigureSpriteComponent(FireSprite, 2);
	ConfigureSpriteComponent(SmokeSprite, 3);
	ConfigureSpriteComponent(SmokeOffsetSpriteA, 4);
	ConfigureSpriteComponent(SmokeOffsetSpriteB, 5);
	if (SpriteMesh)
	{
		DistortionSprite->SetStaticMesh(SpriteMesh);
		ShockwaveSprite->SetStaticMesh(SpriteMesh);
		FireSprite->SetStaticMesh(SpriteMesh);
		SmokeSprite->SetStaticMesh(SpriteMesh);
		SmokeOffsetSpriteA->SetStaticMesh(SpriteMesh);
		SmokeOffsetSpriteB->SetStaticMesh(SpriteMesh);
	}

	FlashLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("FlashLight"));
	FlashLight->SetupAttachment(RootComponent);
	FlashLight->SetRelativeLocation(FVector(0.0f, 0.0f, 52.0f));
	FlashLight->SetCastShadows(false);
	FlashLight->SetLightColor(FLinearColor(1.0f, 0.48f, 0.13f));
	FlashLight->SetIntensity(0.0f);
	FlashLight->SetAttenuationRadius(EffectRadiusCm * 2.0f);

	ExplosionFlipbookMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(ExplosionMaterialPath));
	ExplosionDistortionMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(DistortionMaterialPath));
	ExplosionSmokeMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(SmokeMaterialPath));
}

void ATunaSweeperLocalExplosionEffectActor::BeginPlay()
{
	Super::BeginPlay();

	ApplyDynamicMaterials();
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

void ATunaSweeperLocalExplosionEffectActor::ConfigureSpriteComponent(
	UStaticMeshComponent* SpriteComponent,
	int32 SortPriority)
{
	if (!SpriteComponent)
	{
		return;
	}

	SpriteComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SpriteComponent->SetGenerateOverlapEvents(false);
	SpriteComponent->SetCastShadow(false);
	SpriteComponent->SetCanEverAffectNavigation(false);
	SpriteComponent->SetTranslucentSortPriority(SortPriority);
}

void ATunaSweeperLocalExplosionEffectActor::ApplyDynamicMaterials()
{
	UMaterialInterface* LoadedMaterial = ExplosionFlipbookMaterial.LoadSynchronous();
	if (LoadedMaterial)
	{
		if (ShockwaveSprite)
		{
			ShockwaveDynamicMaterial = UMaterialInstanceDynamic::Create(LoadedMaterial, this);
			ShockwaveSprite->SetMaterial(0, ShockwaveDynamicMaterial);
		}

		if (FireSprite)
		{
			FireDynamicMaterial = UMaterialInstanceDynamic::Create(LoadedMaterial, this);
			FireSprite->SetMaterial(0, FireDynamicMaterial);
		}

	}

	UMaterialInterface* LoadedSmokeMaterial = ExplosionSmokeMaterial.LoadSynchronous();
	if (!LoadedSmokeMaterial)
	{
		LoadedSmokeMaterial = LoadedMaterial;
	}
	if (LoadedSmokeMaterial)
	{
		if (SmokeSprite)
		{
			SmokeDynamicMaterial = UMaterialInstanceDynamic::Create(LoadedSmokeMaterial, this);
			SmokeSprite->SetMaterial(0, SmokeDynamicMaterial);
		}

		if (SmokeOffsetSpriteA)
		{
			SmokeOffsetDynamicMaterialA = UMaterialInstanceDynamic::Create(LoadedSmokeMaterial, this);
			SmokeOffsetSpriteA->SetMaterial(0, SmokeOffsetDynamicMaterialA);
		}

		if (SmokeOffsetSpriteB)
		{
			SmokeOffsetDynamicMaterialB = UMaterialInstanceDynamic::Create(LoadedSmokeMaterial, this);
			SmokeOffsetSpriteB->SetMaterial(0, SmokeOffsetDynamicMaterialB);
		}
	}

	UMaterialInterface* LoadedDistortionMaterial = ExplosionDistortionMaterial.LoadSynchronous();
	if (LoadedDistortionMaterial && DistortionSprite)
	{
		DistortionDynamicMaterial = UMaterialInstanceDynamic::Create(LoadedDistortionMaterial, this);
		DistortionSprite->SetMaterial(0, DistortionDynamicMaterial);
	}
}

void ATunaSweeperLocalExplosionEffectActor::UpdateSpriteFrame(
	UMaterialInstanceDynamic* DynamicMaterial,
	int32 FrameIndex) const
{
	if (!DynamicMaterial)
	{
		return;
	}

	const int32 SafeColumns = FMath::Max(1, FlipbookColumns);
	const int32 SafeRows = FMath::Max(1, FlipbookRows);
	const int32 SafeFrameCount = FMath::Max(1, FMath::Min(FlipbookFrameCount, SafeColumns * SafeRows));
	const int32 SafeFrameIndex = FMath::Clamp(FrameIndex, 0, SafeFrameCount - 1);
	const int32 Column = SafeFrameIndex % SafeColumns;
	const int32 Row = SafeFrameIndex / SafeColumns;
	const float FrameScaleU = 1.0f / static_cast<float>(SafeColumns);
	const float FrameScaleV = 1.0f / static_cast<float>(SafeRows);

	DynamicMaterial->SetScalarParameterValue(TEXT("FrameScale"), FrameScaleU);
	DynamicMaterial->SetScalarParameterValue(TEXT("FrameU"), static_cast<float>(Column) * FrameScaleU);
	DynamicMaterial->SetScalarParameterValue(TEXT("FrameV"), static_cast<float>(Row) * FrameScaleV);
}

void ATunaSweeperLocalExplosionEffectActor::UpdateSpriteMaterial(
	UMaterialInstanceDynamic* DynamicMaterial,
	int32 FrameIndex,
	const FLinearColor& TintColor,
	float EmissiveStrength,
	float Opacity) const
{
	if (!DynamicMaterial)
	{
		return;
	}

	UpdateSpriteFrame(DynamicMaterial, FrameIndex);
	DynamicMaterial->SetVectorParameterValue(TEXT("TintColor"), TintColor);
	DynamicMaterial->SetScalarParameterValue(TEXT("EmissiveStrength"), FMath::Max(0.0f, EmissiveStrength));
	DynamicMaterial->SetScalarParameterValue(TEXT("Opacity"), FMath::Clamp(Opacity, 0.0f, 1.0f));
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

void ATunaSweeperLocalExplosionEffectActor::SetSpriteDiameter(
	UStaticMeshComponent* SpriteComponent,
	float DiameterCm) const
{
	if (!SpriteComponent)
	{
		return;
	}

	const float Scale = FMath::Max(0.0f, DiameterCm) / PlaneMeshSizeCm;
	SpriteComponent->SetRelativeScale3D(FVector(Scale, Scale, 1.0f));
}

void ATunaSweeperLocalExplosionEffectActor::UpdateEffect(float DeltaSeconds)
{
	ElapsedSeconds += FMath::Max(0.0f, DeltaSeconds);
	const float Duration = FMath::Max(0.01f, TotalDurationSeconds);
	const float NormalizedTime = FMath::Clamp(ElapsedSeconds / Duration, 0.0f, 1.0f);
	const int32 SafeFrameCount = FMath::Max(1, FlipbookFrameCount);
	const int32 FireFrame = FMath::Clamp(
		FMath::FloorToInt(NormalizedTime * static_cast<float>(SafeFrameCount)),
		0,
		SafeFrameCount - 1);
	const int32 SmokeStartFrame = FMath::Clamp(SafeFrameCount / 2, 0, SafeFrameCount - 1);
	const int32 SmokeFrame = FMath::Clamp(
		SmokeStartFrame + FMath::FloorToInt(RangeAlpha(0.28f, 1.0f, NormalizedTime) * static_cast<float>(SafeFrameCount - SmokeStartFrame)),
		SmokeStartFrame,
		SafeFrameCount - 1);

	const float DistortionTravel = SmoothStep01(RangeAlpha(0.0f, 0.24f, NormalizedTime));
	const float DistortionFade = FadeOutAfter(0.02f, 0.28f, NormalizedTime);
	const float DistortionDiameter = FMath::Lerp(EffectRadiusCm * 0.45f, EffectRadiusCm * 3.05f, DistortionTravel);
	if (DistortionSprite)
	{
		DistortionSprite->SetRelativeLocation(FVector(0.0f, 0.0f, SpriteBaseHeightCm + 4.0f));
		SetSpriteDiameter(DistortionSprite, DistortionDiameter);
		DistortionSprite->SetRelativeRotation(FRotator(0.0f, 0.0f, NormalizedTime * 95.0f));
		DistortionSprite->SetVisibility(DistortionFade > 0.01f);
	}
	UpdateDistortionMaterial(
		FMath::Lerp(0.14f, 0.72f, DistortionTravel),
		FMath::Lerp(0.16f, 0.07f, DistortionTravel),
		0.42f * DistortionFade,
		1.0f + 0.55f * DistortionFade,
		0.34f * DistortionFade);

	const float BurstGrowth = SmoothStep01(RangeAlpha(0.0f, 0.38f, NormalizedTime));
	const float FireOpacity = FadeOutAfter(0.34f, 0.78f, NormalizedTime);
	const float FireDiameter = FMath::Lerp(EffectRadiusCm * 0.55f, EffectRadiusCm * 2.08f, BurstGrowth);
	if (FireSprite)
	{
		FireSprite->SetRelativeLocation(FVector(0.0f, 0.0f, SpriteBaseHeightCm));
		SetSpriteDiameter(FireSprite, FireDiameter);
		FireSprite->SetRelativeRotation(FRotator(0.0f, 0.0f, NormalizedTime * 62.0f));
		FireSprite->SetVisibility(FireOpacity > 0.01f);
	}
	UpdateSpriteMaterial(
		FireDynamicMaterial,
		FireFrame,
		FLinearColor(1.0f, 0.72f, 0.42f, 1.0f),
		FMath::Lerp(8.5f, 1.3f, SmoothStep01(NormalizedTime)),
		FireOpacity);

	const float ShockOpacity = FadeOutAfter(0.02f, 0.18f, NormalizedTime);
	const float ShockDiameter = FMath::Lerp(EffectRadiusCm * 0.5f, EffectRadiusCm * 2.45f, SmoothStep01(RangeAlpha(0.0f, 0.18f, NormalizedTime)));
	if (ShockwaveSprite)
	{
		ShockwaveSprite->SetRelativeLocation(FVector(0.0f, 0.0f, SpriteBaseHeightCm - 7.0f));
		SetSpriteDiameter(ShockwaveSprite, ShockDiameter);
		ShockwaveSprite->SetRelativeRotation(FRotator(0.0f, 0.0f, -NormalizedTime * 35.0f));
		ShockwaveSprite->SetVisibility(ShockOpacity > 0.01f);
	}
	UpdateSpriteMaterial(
		ShockwaveDynamicMaterial,
		0,
		FLinearColor(1.0f, 0.86f, 0.58f, 1.0f),
		12.0f,
		ShockOpacity * 0.82f);

	const float SmokeReveal = SmoothStep01(RangeAlpha(0.12f, 0.32f, NormalizedTime));
	const float SmokeFade = FadeOutAfter(0.68f, 1.0f, NormalizedTime);
	const float SmokeOpacity = SmokeReveal * SmokeFade;
	const float SmokeGrowth = SmoothStep01(RangeAlpha(0.12f, 1.0f, NormalizedTime));
	const auto UpdateSmokeLayer =
		[this, SmokeFrame, SmokeOpacity, SmokeGrowth](
			UStaticMeshComponent* SpriteComponent,
			UMaterialInstanceDynamic* DynamicMaterial,
			int32 FrameOffset,
			const FVector& Offset,
			float StartDiameter,
			float EndDiameter,
			float RotationDegrees,
			float RotationTravelDegrees,
			const FLinearColor& TintColor,
			float OpacityScale)
		{
			const float LayerOpacity = SmokeOpacity * FMath::Max(0.0f, OpacityScale);
			if (SpriteComponent)
			{
				SpriteComponent->SetRelativeLocation(Offset);
				SetSpriteDiameter(SpriteComponent, FMath::Lerp(StartDiameter, EndDiameter, SmokeGrowth));
				SpriteComponent->SetRelativeRotation(FRotator(0.0f, 0.0f, RotationDegrees + SmokeGrowth * RotationTravelDegrees));
				SpriteComponent->SetVisibility(LayerOpacity > 0.01f);
			}

			UpdateSpriteMaterial(
				DynamicMaterial,
				SmokeFrame + FrameOffset,
				TintColor,
				0.02f,
				LayerOpacity);
		};

	UpdateSmokeLayer(
		SmokeSprite,
		SmokeDynamicMaterial,
		0,
		FVector(EffectRadiusCm * 0.11f * SmokeGrowth, EffectRadiusCm * -0.04f * SmokeGrowth, SpriteBaseHeightCm + 34.0f + SmokeGrowth * 22.0f),
		EffectRadiusCm * 1.10f,
		EffectRadiusCm * 2.55f,
		-16.0f,
		92.0f,
		FLinearColor(0.44f, 0.39f, 0.33f, 1.0f),
		0.92f);
	UpdateSmokeLayer(
		SmokeOffsetSpriteA,
		SmokeOffsetDynamicMaterialA,
		1,
		FVector(EffectRadiusCm * -0.24f * SmokeGrowth, EffectRadiusCm * 0.16f * SmokeGrowth, SpriteBaseHeightCm + 46.0f + SmokeGrowth * 30.0f),
		EffectRadiusCm * 0.82f,
		EffectRadiusCm * 1.72f,
		38.0f,
		-128.0f,
		FLinearColor(0.52f, 0.47f, 0.39f, 1.0f),
		0.66f);
	UpdateSmokeLayer(
		SmokeOffsetSpriteB,
		SmokeOffsetDynamicMaterialB,
		2,
		FVector(EffectRadiusCm * 0.27f * SmokeGrowth, EffectRadiusCm * 0.13f * SmokeGrowth, SpriteBaseHeightCm + 40.0f + SmokeGrowth * 25.0f),
		EffectRadiusCm * 0.72f,
		EffectRadiusCm * 1.55f,
		-72.0f,
		146.0f,
		FLinearColor(0.34f, 0.31f, 0.27f, 1.0f),
		0.58f);

	if (FlashLight)
	{
		const float LightFade = FadeOutAfter(0.0f, 0.22f, NormalizedTime);
		FlashLight->SetIntensity(4200.0f * LightFade);
		FlashLight->SetAttenuationRadius(EffectRadiusCm * FMath::Lerp(1.1f, 2.35f, SmoothStep01(RangeAlpha(0.0f, 0.22f, NormalizedTime))));
	}
}
