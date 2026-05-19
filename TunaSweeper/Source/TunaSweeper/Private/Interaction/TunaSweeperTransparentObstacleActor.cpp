#include "Interaction/TunaSweeperTransparentObstacleActor.h"

#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	const TCHAR* RepairedBridgeVoxelMeshPath = TEXT("/Game/Interaction/SM_Bridge_Repaired_Voxel.SM_Bridge_Repaired_Voxel");
	const TCHAR* RepairedBridgeVoxelMaterialPath = TEXT("/Game/Prototype/M_Voxel_VertexColor.M_Voxel_VertexColor");
}

ATunaSweeperTransparentObstacleActor::ATunaSweeperTransparentObstacleActor()
{
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	BlockingCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("BlockingCollision"));
	BlockingCollision->SetupAttachment(RootComponent);
	BlockingCollision->SetHiddenInGame(true);
	BlockingCollision->SetVisibility(false);
	BlockingCollision->SetCanEverAffectNavigation(true);

	ApplyCollisionDefaults();
}

void ATunaSweeperTransparentObstacleActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyCollisionDefaults();
}

void ATunaSweeperTransparentObstacleActor::ConfigureObstacleDefaults(
	FName InObstacleId,
	const FVector& InBoxExtent)
{
	Modify();
	ObstacleId = InObstacleId;
	BoxExtent = FVector(
		FMath::Max(1.0f, InBoxExtent.X),
		FMath::Max(1.0f, InBoxExtent.Y),
		FMath::Max(1.0f, InBoxExtent.Z));
	ApplyCollisionDefaults();
}

void ATunaSweeperTransparentObstacleActor::ApplyCollisionDefaults()
{
	if (!BlockingCollision)
	{
		return;
	}

	BlockingCollision->SetBoxExtent(BoxExtent);
	BlockingCollision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	BlockingCollision->SetCollisionObjectType(ECC_WorldStatic);
	BlockingCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
	BlockingCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	BlockingCollision->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	BlockingCollision->SetGenerateOverlapEvents(false);
	BlockingCollision->CanCharacterStepUpOn = ECB_No;
	BlockingCollision->SetHiddenInGame(true);
	BlockingCollision->SetVisibility(false);
}

ATunaSweeperWorldProgressCompletedActor::ATunaSweeperWorldProgressCompletedActor()
{
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
	VisualMesh->SetupAttachment(RootComponent);
	VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	VisualMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 18.0f));
	VisualMesh->SetRelativeScale3D(FVector(3.0f, 0.65f, 0.18f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		VisualMesh->SetStaticMesh(CubeMesh.Object);
	}
	ApplyRepairedBridgeVisualMesh();
}

void ATunaSweeperWorldProgressCompletedActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyRepairedBridgeVisualMesh();
}

void ATunaSweeperWorldProgressCompletedActor::BeginPlay()
{
	Super::BeginPlay();
	ApplyRepairedBridgeVisualMesh();
}

void ATunaSweeperWorldProgressCompletedActor::ApplyRepairedBridgeVisualMesh()
{
	if (!VisualMesh)
	{
		return;
	}

	if (UStaticMesh* BridgeMesh = LoadObject<UStaticMesh>(nullptr, RepairedBridgeVoxelMeshPath))
	{
		VisualMesh->SetStaticMesh(BridgeMesh);
		VisualMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 40.0f));
		VisualMesh->SetRelativeScale3D(FVector::OneVector);
	}

	if (UMaterialInterface* VoxelMaterial = LoadObject<UMaterialInterface>(nullptr, RepairedBridgeVoxelMaterialPath))
	{
		VisualMesh->SetMaterial(0, VoxelMaterial);
	}
}
