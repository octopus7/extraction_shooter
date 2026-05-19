#include "Effect/TunaSweeperMeleeSwingTrailActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr float TrailLifetimeSeconds = 0.18f;
	const TCHAR* TrailMaterialPath = TEXT("/Game/Prototype/M_Voxel_VertexColor.M_Voxel_VertexColor");

	void ConfigureTrailSegment(
		UStaticMeshComponent* Segment,
		UStaticMesh* Mesh,
		const FVector& RelativeLocation,
		float RelativeYaw)
	{
		if (!Segment)
		{
			return;
		}

		Segment->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Segment->SetCastShadow(false);
		Segment->SetRelativeLocation(RelativeLocation);
		Segment->SetRelativeRotation(FRotator(0.0f, RelativeYaw, 0.0f));
		Segment->SetRelativeScale3D(FVector(1.05f, 0.035f, 0.012f));
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

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	UStaticMesh* TrailMesh = CubeMesh.Succeeded() ? CubeMesh.Object : nullptr;
	ConfigureTrailSegment(TrailSegmentA, TrailMesh, FVector(74.0f, -42.0f, 58.0f), -34.0f);
	ConfigureTrailSegment(TrailSegmentB, TrailMesh, FVector(92.0f, 0.0f, 62.0f), 0.0f);
	ConfigureTrailSegment(TrailSegmentC, TrailMesh, FVector(74.0f, 42.0f, 58.0f), 34.0f);
}

void ATunaSweeperMeleeSwingTrailActor::BeginPlay()
{
	Super::BeginPlay();

	UMaterialInterface* TrailMaterial = LoadObject<UMaterialInterface>(nullptr, TrailMaterialPath);
	UStaticMeshComponent* Segments[] = { TrailSegmentA, TrailSegmentB, TrailSegmentC };
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
			const FLinearColor TrailColor(0.95f, 0.98f, 0.78f, 1.0f);
			DynamicMaterial->SetVectorParameterValue(TEXT("Color"), TrailColor);
			DynamicMaterial->SetVectorParameterValue(TEXT("BaseColor"), TrailColor);
		}
	}
}
