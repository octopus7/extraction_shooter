#include "Effect/TunaSweeperMeleeSwingTrailActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

namespace
{
	constexpr float TrailLifetimeSeconds = 0.20f;
	const FLinearColor SwingTrailColor(0.0f, 0.95f, 1.0f, 1.0f);
	const TCHAR* SwingArcMeshPath = TEXT("/Game/Effects/SM_LumberjackMeleeSwingArc.SM_LumberjackMeleeSwingArc");
	const TCHAR* SwingArcMaterialPath = TEXT("/Game/Effects/M_LumberjackMeleeSwingArc.M_LumberjackMeleeSwingArc");
	const TCHAR* FallbackCubeMeshPath = TEXT("/Engine/BasicShapes/Cube.Cube");
	const TCHAR* FallbackMaterialPath = TEXT("/Game/Prototype/M_Voxel_VertexColor.M_Voxel_VertexColor");
}

ATunaSweeperMeleeSwingTrailActor::ATunaSweeperMeleeSwingTrailActor()
{
	PrimaryActorTick.bCanEverTick = true;
	InitialLifeSpan = TrailLifetimeSeconds;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	SwingArcMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SwingArcMesh"));
	SwingArcMesh->SetupAttachment(RootComponent);
	SwingArcMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SwingArcMesh->SetCastShadow(false);
}

void ATunaSweeperMeleeSwingTrailActor::BeginPlay()
{
	Super::BeginPlay();

	if (!SwingArcMesh)
	{
		return;
	}

	UStaticMesh* ArcMesh = LoadObject<UStaticMesh>(nullptr, SwingArcMeshPath);
	if (!ArcMesh)
	{
		ArcMesh = LoadObject<UStaticMesh>(nullptr, FallbackCubeMeshPath);
		SwingArcMesh->SetRelativeLocation(FVector(84.0f, 0.0f, 58.0f));
		SwingArcMesh->SetRelativeScale3D(FVector(0.95f, 0.24f, 0.018f));
	}

	if (ArcMesh)
	{
		SwingArcMesh->SetStaticMesh(ArcMesh);
	}
	BaseArcScale = SwingArcMesh->GetRelativeScale3D();

	UMaterialInterface* ArcMaterial = LoadObject<UMaterialInterface>(nullptr, SwingArcMaterialPath);
	if (!ArcMaterial)
	{
		ArcMaterial = LoadObject<UMaterialInterface>(nullptr, FallbackMaterialPath);
	}

	if (ArcMaterial)
	{
		SwingArcMesh->SetMaterial(0, ArcMaterial);
	}

	SwingArcMaterial = SwingArcMesh->CreateAndSetMaterialInstanceDynamic(0);
	if (SwingArcMaterial)
	{
		SwingArcMaterial->SetVectorParameterValue(TEXT("Color"), SwingTrailColor);
		SwingArcMaterial->SetVectorParameterValue(TEXT("BaseColor"), SwingTrailColor);
		SwingArcMaterial->SetVectorParameterValue(TEXT("EmissiveColor"), SwingTrailColor);
		SwingArcMaterial->SetScalarParameterValue(TEXT("Opacity"), 1.0f);
		SwingArcMaterial->SetScalarParameterValue(TEXT("Intensity"), 5.2f);
		SwingArcMaterial->SetScalarParameterValue(TEXT("Dissolve"), 0.0f);
		SwingArcMaterial->SetScalarParameterValue(TEXT("UVOffset"), 0.0f);
	}
}

void ATunaSweeperMeleeSwingTrailActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	ElapsedLifetimeSeconds += DeltaSeconds;
	const float LifeAlpha = FMath::Clamp(ElapsedLifetimeSeconds / TrailLifetimeSeconds, 0.0f, 1.0f);
	const float FadeAlpha = 1.0f - LifeAlpha;

	if (SwingArcMaterial)
	{
		SwingArcMaterial->SetScalarParameterValue(TEXT("Opacity"), FadeAlpha * FadeAlpha);
		SwingArcMaterial->SetScalarParameterValue(TEXT("Intensity"), FMath::Lerp(5.2f, 1.1f, LifeAlpha));
		SwingArcMaterial->SetScalarParameterValue(TEXT("Dissolve"), LifeAlpha);
		SwingArcMaterial->SetScalarParameterValue(TEXT("UVOffset"), LifeAlpha * 0.75f);
	}

	if (SwingArcMesh)
	{
		SwingArcMesh->SetRelativeScale3D(
			BaseArcScale * FVector(1.0f + LifeAlpha * 0.04f, 1.0f + LifeAlpha * 0.10f, 1.0f));
	}
}
