#include "Interaction/TunaSweeperMemoActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Interaction/TunaSweeperInteractableComponent.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

ATunaSweeperMemoActor::ATunaSweeperMemoActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
	VisualMesh->SetupAttachment(RootComponent);
	VisualMesh->SetRelativeScale3D(VisualScale);
	VisualMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		VisualMesh->SetStaticMesh(CubeMesh.Object);
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BlueMaterial(TEXT("/Game/Characters/Enemy/M_Enemy_Blue.M_Enemy_Blue"));
	if (BlueMaterial.Succeeded())
	{
		VisualMesh->SetMaterial(0, BlueMaterial.Object);
	}

	InteractableComponent = CreateDefaultSubobject<UTunaSweeperInteractableComponent>(TEXT("Interactable"));
	InteractableComponent->SetupAttachment(RootComponent);
	InteractableComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 72.0f));
	InteractableComponent->SetInteractionTypeAndDisplayName(
		ETunaSweeperInteractionType::Memo,
		FText::FromString(TEXT("\uBA54\uBAA8")));
}

void ATunaSweeperMemoActor::BeginPlay()
{
	Super::BeginPlay();

	ApplyVisualDefaults();

	UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	if (!TunaGameInstance || MemoId == INDEX_NONE)
	{
		Destroy();
		return;
	}

	if (TunaGameInstance->IsMemoAcquired(MemoId))
	{
		Destroy();
	}
}

void ATunaSweeperMemoActor::ConfigureMemoDefaults(
	int32 InMemoId,
	const FText& InInteractionDisplayName,
	TSoftClassPtr<UTunaSweeperInteractionMarkerWidget> InMarkerWidgetClass,
	TSoftObjectPtr<UStaticMesh> InVisualMesh,
	TSoftObjectPtr<UMaterialInterface> InVisualMaterial,
	FVector InVisualScale,
	FVector InVisualRelativeLocation)
{
	Modify();
	MemoId = InMemoId;
	VisualMeshAsset = InVisualMesh;
	VisualMaterialAsset = InVisualMaterial;
	VisualScale = FVector(
		FMath::Max(0.01f, InVisualScale.X),
		FMath::Max(0.01f, InVisualScale.Y),
		FMath::Max(0.01f, InVisualScale.Z));
	VisualRelativeLocation = InVisualRelativeLocation;

	if (InteractableComponent)
	{
		InteractableComponent->ConfigureInteractionDefaults(
			ETunaSweeperInteractionType::Memo,
			InInteractionDisplayName.IsEmpty() ? FText::FromString(TEXT("\uBA54\uBAA8")) : InInteractionDisplayName,
			InMarkerWidgetClass);
	}

	ApplyVisualDefaults();
}

void ATunaSweeperMemoActor::ApplyVisualDefaults()
{
	if (!VisualMesh)
	{
		return;
	}

	if (UStaticMesh* LoadedMesh = VisualMeshAsset.LoadSynchronous())
	{
		VisualMesh->SetStaticMesh(LoadedMesh);
	}

	if (UMaterialInterface* LoadedMaterial = VisualMaterialAsset.LoadSynchronous())
	{
		VisualMesh->SetMaterial(0, LoadedMaterial);
	}

	VisualMesh->SetRelativeScale3D(VisualScale);
	VisualMesh->SetRelativeLocation(VisualRelativeLocation);
}
