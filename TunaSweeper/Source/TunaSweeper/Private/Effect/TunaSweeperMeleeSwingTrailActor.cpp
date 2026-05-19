#include "Effect/TunaSweeperMeleeSwingTrailActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr float TrailLifetimeSeconds = 0.20f;
	const TCHAR* TrailMaterialPath = TEXT("/Game/Prototype/M_Voxel_VertexColor.M_Voxel_VertexColor");
	const FLinearColor TrailColor(0.0f, 0.95f, 1.0f, 1.0f);

	void ConfigureTrailSegment(
		UStaticMeshComponent* Segment,
		UStaticMesh* Mesh,
		const FVector& RelativeLocation,
		float RelativeYaw,
		const FVector& RelativeScale)
	{
		if (!Segment)
		{
			return;
		}

		Segment->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Segment->SetCastShadow(false);
		Segment->SetRelativeLocation(RelativeLocation);
		Segment->SetRelativeRotation(FRotator(0.0f, RelativeYaw, 0.0f));
		Segment->SetRelativeScale3D(RelativeScale);
		if (Mesh)
		{
			Segment->SetStaticMesh(Mesh);
		}
	}
}

ATunaSweeperMeleeSwingTrailActor::ATunaSweeperMeleeSwingTrailActor()
{
	PrimaryActorTick.bCanEverTick = false;
	InitialLifeSpan = TrailLifetimeSeconds;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	TrailSegmentA = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TrailSegmentA"));
	TrailSegmentA->SetupAttachment(RootComponent);

	TrailSegmentB = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TrailSegmentB"));
	TrailSegmentB->SetupAttachment(RootComponent);

	TrailSegmentC = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TrailSegmentC"));
	TrailSegmentC->SetupAttachment(RootComponent);

	TrailSegmentD = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TrailSegmentD"));
	TrailSegmentD->SetupAttachment(RootComponent);

	TrailSegmentE = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TrailSegmentE"));
	TrailSegmentE->SetupAttachment(RootComponent);

	TrailSegmentF = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TrailSegmentF"));
	TrailSegmentF->SetupAttachment(RootComponent);

	TrailSegmentG = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TrailSegmentG"));
	TrailSegmentG->SetupAttachment(RootComponent);

	TrailSegmentH = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TrailSegmentH"));
	TrailSegmentH->SetupAttachment(RootComponent);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	UStaticMesh* TrailMesh = CubeMesh.Succeeded() ? CubeMesh.Object : nullptr;
	ConfigureTrailSegment(TrailSegmentA, TrailMesh, FVector(56.0f, -76.0f, 58.0f), -62.0f, FVector(0.42f, 0.15f, 0.022f));
	ConfigureTrailSegment(TrailSegmentB, TrailMesh, FVector(76.0f, -60.0f, 61.0f), -45.0f, FVector(0.56f, 0.16f, 0.024f));
	ConfigureTrailSegment(TrailSegmentC, TrailMesh, FVector(94.0f, -36.0f, 64.0f), -26.0f, FVector(0.74f, 0.18f, 0.026f));
	ConfigureTrailSegment(TrailSegmentD, TrailMesh, FVector(104.0f, -6.0f, 65.0f), -8.0f, FVector(0.82f, 0.20f, 0.028f));
	ConfigureTrailSegment(TrailSegmentE, TrailMesh, FVector(102.0f, 24.0f, 64.0f), 14.0f, FVector(0.78f, 0.19f, 0.026f));
	ConfigureTrailSegment(TrailSegmentF, TrailMesh, FVector(86.0f, 52.0f, 61.0f), 36.0f, FVector(0.62f, 0.17f, 0.024f));
	ConfigureTrailSegment(TrailSegmentG, TrailMesh, FVector(62.0f, 72.0f, 58.0f), 58.0f, FVector(0.44f, 0.15f, 0.022f));
	ConfigureTrailSegment(TrailSegmentH, TrailMesh, FVector(84.0f, 0.0f, 58.0f), 0.0f, FVector(0.95f, 0.24f, 0.018f));
}

void ATunaSweeperMeleeSwingTrailActor::BeginPlay()
{
	Super::BeginPlay();

	UMaterialInterface* TrailMaterial = LoadObject<UMaterialInterface>(nullptr, TrailMaterialPath);
	UStaticMeshComponent* Segments[] =
	{
		TrailSegmentA,
		TrailSegmentB,
		TrailSegmentC,
		TrailSegmentD,
		TrailSegmentE,
		TrailSegmentF,
		TrailSegmentG,
		TrailSegmentH
	};
	for (UStaticMeshComponent* Segment : Segments)
	{
		if (!Segment)
		{
			continue;
		}

		if (TrailMaterial)
		{
			Segment->SetMaterial(0, TrailMaterial);
		}

		UMaterialInstanceDynamic* DynamicMaterial = Segment->CreateAndSetMaterialInstanceDynamic(0);
		if (DynamicMaterial)
		{
			DynamicMaterial->SetVectorParameterValue(TEXT("Color"), TrailColor);
			DynamicMaterial->SetVectorParameterValue(TEXT("BaseColor"), TrailColor);
			DynamicMaterial->SetVectorParameterValue(TEXT("EmissiveColor"), TrailColor);
		}
	}
}
