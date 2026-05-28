#include "Housing/TunaSweeperHousingFacilityActor.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

ATunaSweeperHousingFacilityActor::ATunaSweeperHousingFacilityActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	FacilityMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FacilityMesh"));
	FacilityMesh->SetupAttachment(SceneRoot);
	FacilityMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	FacilityMesh->SetGenerateOverlapEvents(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMeshFinder.Succeeded())
	{
		DefaultCubeMesh = CubeMeshFinder.Object;
		FacilityMesh->SetStaticMesh(DefaultCubeMesh);
	}
}

void ATunaSweeperHousingFacilityActor::ConfigureFacilityVisual(
	const FTunaSweeperHousingFacilityDefinition& Definition,
	const FTunaSweeperHousingPlacedFacilitySaveData& Placement,
	const FTransform& WorldTransform,
	bool bPreview,
	bool bPlacementValid)
{
	InstanceId = Placement.InstanceId;
	FacilityId = Definition.FacilityId;
	SetActorTransform(WorldTransform);
	ApplyMeshAndMaterial(Definition);

	if (FacilityMesh)
	{
		FacilityMesh->SetCollisionEnabled(bPreview ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryAndPhysics);
		FacilityMesh->SetVisibility(true, true);
		FacilityMesh->SetHiddenInGame(false, true);
		FacilityMesh->SetRenderCustomDepth(bPreview);
		FacilityMesh->SetCustomDepthStencilValue(bPlacementValid ? 1 : 2);
	}

	SetActorEnableCollision(!bPreview);
}

void ATunaSweeperHousingFacilityActor::ApplyMeshAndMaterial(const FTunaSweeperHousingFacilityDefinition& Definition)
{
	if (!FacilityMesh)
	{
		return;
	}

	UStaticMesh* Mesh = DefaultCubeMesh;
	const FString TrimmedMeshPath = Definition.StaticMeshPath.TrimStartAndEnd();
	if (!TrimmedMeshPath.IsEmpty())
	{
		if (UStaticMesh* LoadedMesh = LoadObject<UStaticMesh>(nullptr, *TrimmedMeshPath))
		{
			Mesh = LoadedMesh;
		}
	}
	if (Mesh)
	{
		FacilityMesh->SetStaticMesh(Mesh);
	}

	const FString TrimmedMaterialPath = Definition.MaterialPath.TrimStartAndEnd();
	if (!TrimmedMaterialPath.IsEmpty())
	{
		if (UMaterialInterface* LoadedMaterial = LoadObject<UMaterialInterface>(nullptr, *TrimmedMaterialPath))
		{
			FacilityMesh->SetMaterial(0, LoadedMaterial);
		}
	}
}
