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
	InitialLifeSpan = TotalDurationSeconds * ResidualSmokeDurationMultiplier + 0.08f;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	DistortionSprite = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DistortionSprite"));
	DistortionSprite->SetupAttachment(RootComponent);

	ShockwaveSprite = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShockwaveSprite"));
	ShockwaveSprite->SetupAttachment(RootComponent);

	GroundSmokeSpriteA = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GroundSmokeSpriteA"));
	GroundSmokeSpriteA->SetupAttachment(RootComponent);

	GroundSmokeSpriteB = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GroundSmokeSpriteB"));
	GroundSmokeSpriteB->SetupAttachment(RootComponent);

	GroundSmokeSpriteC = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GroundSmokeSpriteC"));
	GroundSmokeSpriteC->SetupAttachment(RootComponent);

	GroundSmokeSpriteD = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GroundSmokeSpriteD"));
	GroundSmokeSpriteD->SetupAttachment(RootComponent);

	GroundSmokeSpriteE = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GroundSmokeSpriteE"));
	GroundSmokeSpriteE->SetupAttachment(RootComponent);

	GroundSmokeSpriteF = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GroundSmokeSpriteF"));
	GroundSmokeSpriteF->SetupAttachment(RootComponent);

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
	ConfigureSpriteComponent(GroundSmokeSpriteA, 2);
	ConfigureSpriteComponent(GroundSmokeSpriteB, 3);
	ConfigureSpriteComponent(GroundSmokeSpriteC, 4);
	ConfigureSpriteComponent(GroundSmokeSpriteD, 5);
	ConfigureSpriteComponent(GroundSmokeSpriteE, 6);
	ConfigureSpriteComponent(GroundSmokeSpriteF, 7);
	ConfigureSpriteComponent(FireSprite, 8);
	ConfigureSpriteComponent(SmokeSprite, 9);
	ConfigureSpriteComponent(SmokeOffsetSpriteA, 10);
	ConfigureSpriteComponent(SmokeOffsetSpriteB, 11);
	if (SpriteMesh)
	{
		DistortionSprite->SetStaticMesh(SpriteMesh);
		ShockwaveSprite->SetStaticMesh(SpriteMesh);
		GroundSmokeSpriteA->SetStaticMesh(SpriteMesh);
		GroundSmokeSpriteB->SetStaticMesh(SpriteMesh);
		GroundSmokeSpriteC->SetStaticMesh(SpriteMesh);
		GroundSmokeSpriteD->SetStaticMesh(SpriteMesh);
		GroundSmokeSpriteE->SetStaticMesh(SpriteMesh);
		GroundSmokeSpriteF->SetStaticMesh(SpriteMesh);
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
	SetLifeSpan(FMath::Max(0.08f, TotalDurationSeconds * FMath::Max(1.0f, ResidualSmokeDurationMultiplier) + 0.08f));
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
	SetLifeSpan(TotalDurationSeconds * FMath::Max(1.0f, ResidualSmokeDurationMultiplier) + 0.08f);
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
		if (GroundSmokeSpriteA)
		{
			GroundSmokeDynamicMaterialA = UMaterialInstanceDynamic::Create(LoadedSmokeMaterial, this);
			GroundSmokeSpriteA->SetMaterial(0, GroundSmokeDynamicMaterialA);
		}

		if (GroundSmokeSpriteB)
		{
			GroundSmokeDynamicMaterialB = UMaterialInstanceDynamic::Create(LoadedSmokeMaterial, this);
			GroundSmokeSpriteB->SetMaterial(0, GroundSmokeDynamicMaterialB);
		}

		if (GroundSmokeSpriteC)
		{
			GroundSmokeDynamicMaterialC = UMaterialInstanceDynamic::Create(LoadedSmokeMaterial, this);
			GroundSmokeSpriteC->SetMaterial(0, GroundSmokeDynamicMaterialC);
		}

		if (GroundSmokeSpriteD)
		{
			GroundSmokeDynamicMaterialD = UMaterialInstanceDynamic::Create(LoadedSmokeMaterial, this);
			GroundSmokeSpriteD->SetMaterial(0, GroundSmokeDynamicMaterialD);
		}

		if (GroundSmokeSpriteE)
		{
			GroundSmokeDynamicMaterialE = UMaterialInstanceDynamic::Create(LoadedSmokeMaterial, this);
			GroundSmokeSpriteE->SetMaterial(0, GroundSmokeDynamicMaterialE);
		}

		if (GroundSmokeSpriteF)
		{
			GroundSmokeDynamicMaterialF = UMaterialInstanceDynamic::Create(LoadedSmokeMaterial, this);
			GroundSmokeSpriteF->SetMaterial(0, GroundSmokeDynamicMaterialF);
		}

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
	const float ResidualDuration = Duration * FMath::Max(1.0f, ResidualSmokeDurationMultiplier);
	const float NormalizedTime = FMath::Clamp(ElapsedSeconds / Duration, 0.0f, 1.0f);
	const float ResidualNormalizedTime = FMath::Clamp(ElapsedSeconds / ResidualDuration, 0.0f, 1.0f);
	const int32 SafeFrameCount = FMath::Max(1, FlipbookFrameCount);
	const int32 FireFrame = FMath::Clamp(
		FMath::FloorToInt(NormalizedTime * static_cast<float>(SafeFrameCount)),
		0,
		SafeFrameCount - 1);
	const int32 SmokeStartFrame = FMath::Clamp(SafeFrameCount / 2, 0, SafeFrameCount - 1);
	const int32 SmokeFrame = FMath::Clamp(
		SmokeStartFrame + FMath::FloorToInt(RangeAlpha(0.16f, 1.0f, ResidualNormalizedTime) * static_cast<float>(SafeFrameCount - SmokeStartFrame)),
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

	const float GroundSmokeReveal = SmoothStep01(RangeAlpha(0.0f, 0.045f, ResidualNormalizedTime));
	const float GroundSmokeFade = FadeOutAfter(0.72f, 1.0f, ResidualNormalizedTime);
	const float GroundSmokeEarlyDensity = FMath::Lerp(1.55f, 1.0f, SmoothStep01(RangeAlpha(0.0f, 0.36f, ResidualNormalizedTime)));
	const float GroundSmokeOpacity = GroundSmokeReveal * GroundSmokeFade * GroundSmokeEarlyDensity;
	const float GroundSmokeGrowth = SmoothStep01(RangeAlpha(0.0f, 1.0f, ResidualNormalizedTime));
	const auto UpdateGroundSmokeLayer =
		[this, GroundSmokeOpacity, GroundSmokeGrowth](
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
			const float LayerOpacity = GroundSmokeOpacity * FMath::Max(0.0f, OpacityScale);
			if (SpriteComponent)
			{
				SpriteComponent->SetRelativeLocation(Offset);
				SetSpriteDiameter(SpriteComponent, FMath::Lerp(StartDiameter, EndDiameter, GroundSmokeGrowth));
				SpriteComponent->SetRelativeRotation(FRotator(0.0f, 0.0f, RotationDegrees + GroundSmokeGrowth * RotationTravelDegrees));
				SpriteComponent->SetVisibility(LayerOpacity > 0.01f);
			}

			UpdateSpriteMaterial(
				DynamicMaterial,
				FMath::Clamp(8 + FrameOffset + FMath::FloorToInt(GroundSmokeGrowth * 4.0f), 0, FMath::Max(0, FlipbookFrameCount - 1)),
				TintColor,
				0.0f,
				LayerOpacity);
		};

	UpdateGroundSmokeLayer(
		GroundSmokeSpriteA,
		GroundSmokeDynamicMaterialA,
		0,
		FVector(0.0f, 0.0f, SpriteBaseHeightCm - 22.0f),
		EffectRadiusCm * 1.10f,
		EffectRadiusCm * 2.80f,
		14.0f,
		-54.0f,
		FLinearColor(0.075f, 0.067f, 0.055f, 1.0f),
		1.30f);
	UpdateGroundSmokeLayer(
		GroundSmokeSpriteB,
		GroundSmokeDynamicMaterialB,
		1,
		FVector(EffectRadiusCm * -0.18f * GroundSmokeGrowth, EffectRadiusCm * 0.10f * GroundSmokeGrowth, SpriteBaseHeightCm - 17.0f),
		EffectRadiusCm * 0.78f,
		EffectRadiusCm * 2.10f,
		-43.0f,
		82.0f,
		FLinearColor(0.095f, 0.086f, 0.070f, 1.0f),
		1.02f);
	UpdateGroundSmokeLayer(
		GroundSmokeSpriteC,
		GroundSmokeDynamicMaterialC,
		2,
		FVector(EffectRadiusCm * 0.20f * GroundSmokeGrowth, EffectRadiusCm * -0.16f * GroundSmokeGrowth, SpriteBaseHeightCm - 13.0f),
		EffectRadiusCm * 0.66f,
		EffectRadiusCm * 1.70f,
		68.0f,
		-96.0f,
		FLinearColor(0.055f, 0.052f, 0.046f, 1.0f),
		0.82f);
	UpdateGroundSmokeLayer(
		GroundSmokeSpriteD,
		GroundSmokeDynamicMaterialD,
		3,
		FVector(EffectRadiusCm * -0.10f * GroundSmokeGrowth, EffectRadiusCm * -0.22f * GroundSmokeGrowth, SpriteBaseHeightCm - 19.0f),
		EffectRadiusCm * 0.56f,
		EffectRadiusCm * 1.45f,
		132.0f,
		118.0f,
		FLinearColor(0.115f, 0.105f, 0.086f, 1.0f),
		0.64f);
	UpdateGroundSmokeLayer(
		GroundSmokeSpriteE,
		GroundSmokeDynamicMaterialE,
		4,
		FVector(EffectRadiusCm * 0.28f * GroundSmokeGrowth, EffectRadiusCm * 0.00f, SpriteBaseHeightCm - 15.0f),
		EffectRadiusCm * 0.48f,
		EffectRadiusCm * 1.28f,
		-118.0f,
		-74.0f,
		FLinearColor(0.075f, 0.071f, 0.060f, 1.0f),
		0.56f);
	UpdateGroundSmokeLayer(
		GroundSmokeSpriteF,
		GroundSmokeDynamicMaterialF,
		5,
		FVector(EffectRadiusCm * -0.30f * GroundSmokeGrowth, EffectRadiusCm * -0.02f, SpriteBaseHeightCm - 12.0f),
		EffectRadiusCm * 0.42f,
		EffectRadiusCm * 1.18f,
		92.0f,
		-132.0f,
		FLinearColor(0.040f, 0.038f, 0.035f, 1.0f),
		0.48f);

	const float SmokeReveal = SmoothStep01(RangeAlpha(0.07f, 0.18f, ResidualNormalizedTime));
	const float SmokeFade = FadeOutAfter(0.70f, 1.0f, ResidualNormalizedTime);
	const float SmokeEarlyDensity = FMath::Lerp(1.35f, 1.0f, SmoothStep01(RangeAlpha(0.0f, 0.34f, ResidualNormalizedTime)));
	const float SmokeOpacity = SmokeReveal * SmokeFade * SmokeEarlyDensity;
	const float SmokeGrowth = SmoothStep01(RangeAlpha(0.10f, 1.0f, ResidualNormalizedTime));
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
