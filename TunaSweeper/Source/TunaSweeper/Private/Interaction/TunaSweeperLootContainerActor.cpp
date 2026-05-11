#include "Interaction/TunaSweeperLootContainerActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Interaction/TunaSweeperInteractableComponent.h"
#include "UObject/ConstructorHelpers.h"

ATunaSweeperLootContainerActor::ATunaSweeperLootContainerActor()
{
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
	VisualMesh->SetupAttachment(RootComponent);
	VisualMesh->SetRelativeScale3D(FVector(1.1f, 0.8f, 0.55f));
	VisualMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		VisualMesh->SetStaticMesh(CubeMesh.Object);
	}

	InteractableComponent = CreateDefaultSubobject<UTunaSweeperInteractableComponent>(TEXT("Interactable"));
	InteractableComponent->SetupAttachment(RootComponent);
	InteractableComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 90.0f));
	InteractableComponent->SetInteractionTypeAndDisplayName(
		ETunaSweeperInteractionType::LootContainerOpen,
		FText::FromString(TEXT("\uC5F4\uAE30")));
}

void ATunaSweeperLootContainerActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RefreshContainerPresentation();
}

void ATunaSweeperLootContainerActor::BeginPlay()
{
	Super::BeginPlay();
	RefreshContainerPresentation();
}

void ATunaSweeperLootContainerActor::SetContainerDataIds(int32 InContainerDefinitionId, int32 InContentsId)
{
	Modify();
	ContainerDefinitionId = InContainerDefinitionId;
	ContentsId = InContentsId;
	RefreshContainerPresentation();
}

bool ATunaSweeperLootContainerActor::BuildContainerInstance(FTunaSweeperLootContainerInstance& OutInstance) const
{
	UTunaSweeperItemDataSubsystem* ItemDataSubsystem = GetItemDataSubsystem();
	if (!ItemDataSubsystem)
	{
		OutInstance = FTunaSweeperLootContainerInstance();
		return false;
	}

	return ItemDataSubsystem->TryBuildLootContainerInstance(
		ContainerDefinitionId,
		ContentsId,
		DisplayLanguage,
		OutInstance);
}

void ATunaSweeperLootContainerActor::ConfigureLootContainerDefaults(int32 InContainerDefinitionId, int32 InContentsId)
{
	Modify();
	ContainerDefinitionId = InContainerDefinitionId;
	ContentsId = InContentsId;
	RefreshContainerPresentation();
}

void ATunaSweeperLootContainerActor::RefreshContainerPresentation()
{
	if (InteractableComponent)
	{
		InteractableComponent->SetInteractionTypeAndDisplayName(
			ETunaSweeperInteractionType::LootContainerOpen,
			FText::FromString(TEXT("\uC5F4\uAE30")));
	}

	UTunaSweeperItemDataSubsystem* ItemDataSubsystem = GetItemDataSubsystem();
	if (!ItemDataSubsystem || !VisualMesh)
	{
		return;
	}

	FTunaSweeperLootContainerDefinition Definition;
	if (!ItemDataSubsystem->TryGetLootContainerDefinition(ContainerDefinitionId, Definition))
	{
		return;
	}

	if (UStaticMesh* LoadedMesh = Cast<UStaticMesh>(FSoftObjectPath(Definition.StaticMeshPath).TryLoad()))
	{
		VisualMesh->SetStaticMesh(LoadedMesh);
	}
	VisualMesh->SetRelativeScale3D(Definition.MeshScale);
}

UTunaSweeperItemDataSubsystem* ATunaSweeperLootContainerActor::GetItemDataSubsystem() const
{
	const UGameInstance* GameInstance = GetGameInstance();
	return GameInstance
		? GameInstance->GetSubsystem<UTunaSweeperItemDataSubsystem>()
		: nullptr;
}

