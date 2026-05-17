#include "Interaction/TunaSweeperLootContainerActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Game/TunaSweeperGameInstance.h"
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

void ATunaSweeperLootContainerActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (RuntimeGameInstance.IsValid())
	{
		RuntimeGameInstance->OnInventoryStateChanged.RemoveAll(this);
	}

	Super::EndPlay(EndPlayReason);
}

void ATunaSweeperLootContainerActor::SetContainerDataIds(int32 InContainerDefinitionId, int32 InContentsId)
{
	Modify();
	const bool bDataIdsChanged = ContainerDefinitionId != InContainerDefinitionId || ContentsId != InContentsId;
	ContainerDefinitionId = InContainerDefinitionId;
	ContentsId = InContentsId;
	if (bDataIdsChanged)
	{
		ResetRuntimeContainerState();
	}
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

bool ATunaSweeperLootContainerActor::OpenRuntimeContainer(
	UTunaSweeperGameInstance* TunaGameInstance,
	FTunaSweeperLootContainerInstance& OutInstance)
{
	if (!TunaGameInstance)
	{
		OutInstance = FTunaSweeperLootContainerInstance();
		return false;
	}

	if (RuntimeGameInstance.Get() != TunaGameInstance)
	{
		if (RuntimeGameInstance.IsValid())
		{
			RuntimeGameInstance->OnInventoryStateChanged.RemoveAll(this);
		}

		RuntimeGameInstance = TunaGameInstance;
		TunaGameInstance->OnInventoryStateChanged.RemoveAll(this);
		TunaGameInstance->OnInventoryStateChanged.AddUObject(this, &ATunaSweeperLootContainerActor::CaptureRuntimeContentsFromActiveContainer);
	}

	if (bHasRuntimeContainerState && !IsRuntimeContainerStateValid(TunaGameInstance))
	{
		ResetRuntimeContainerState();
	}

	if (bHasRuntimeContainerState)
	{
		OutInstance = BuildRuntimeContainerInstance();
		TunaGameInstance->SetActiveLootContainerRuntimeSlots(OutInstance, RuntimeSlots, this);
		CaptureRuntimeContentsFromActiveContainer();
		return true;
	}

	if (!BuildContainerInstance(OutInstance))
	{
		return false;
	}

	TunaGameInstance->SetActiveLootContainerInstance(OutInstance, this);
	RuntimeContainerDefinitionId = OutInstance.ContainerDefinitionId;
	RuntimeContentsId = OutInstance.ContentsId;
	RuntimeDisplayName = OutInstance.DisplayName;
	RuntimeCapacity = FMath::Max(0, OutInstance.Capacity);
	RuntimeSlots = TunaGameInstance->GetActiveLootContainerSlots();
	bHasRuntimeContainerState = true;
	return true;
}

void ATunaSweeperLootContainerActor::ConfigureLootContainerDefaults(int32 InContainerDefinitionId, int32 InContentsId)
{
	Modify();
	const bool bDataIdsChanged = ContainerDefinitionId != InContainerDefinitionId || ContentsId != InContentsId;
	ContainerDefinitionId = InContainerDefinitionId;
	ContentsId = InContentsId;
	if (bDataIdsChanged)
	{
		ResetRuntimeContainerState();
	}
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

void ATunaSweeperLootContainerActor::ResetRuntimeContainerState()
{
	RuntimeSlots.Reset();
	RuntimeDisplayName = FText::GetEmpty();
	RuntimeCapacity = 0;
	RuntimeContainerDefinitionId = INDEX_NONE;
	RuntimeContentsId = INDEX_NONE;
	bHasRuntimeContainerState = false;
}

void ATunaSweeperLootContainerActor::CaptureRuntimeContentsFromActiveContainer()
{
	UTunaSweeperGameInstance* TunaGameInstance = RuntimeGameInstance.Get();
	if (!TunaGameInstance ||
		!TunaGameInstance->HasActiveLootContainer() ||
		TunaGameInstance->GetActiveLootContainerOwner() != this)
	{
		return;
	}

	RuntimeContainerDefinitionId = ContainerDefinitionId;
	RuntimeContentsId = ContentsId;
	RuntimeDisplayName = TunaGameInstance->GetActiveLootContainerDisplayName();
	RuntimeCapacity = TunaGameInstance->GetActiveLootContainerCapacity();
	RuntimeSlots = TunaGameInstance->GetActiveLootContainerSlots();
	bHasRuntimeContainerState = true;
}

bool ATunaSweeperLootContainerActor::IsRuntimeContainerStateValid(const UTunaSweeperGameInstance* TunaGameInstance) const
{
	if (!TunaGameInstance)
	{
		return false;
	}

	for (const FTunaSweeperInventorySlot& Slot : RuntimeSlots)
	{
		if (!Slot.ItemUid.IsValid())
		{
			continue;
		}

		FTunaSweeperItemInstance ItemInstance;
		if (!TunaGameInstance->TryGetItemInstance(Slot.ItemUid, ItemInstance))
		{
			return false;
		}
	}

	return true;
}

FTunaSweeperLootContainerInstance ATunaSweeperLootContainerActor::BuildRuntimeContainerInstance() const
{
	FTunaSweeperLootContainerInstance RuntimeInstance;
	RuntimeInstance.ContainerDefinitionId = RuntimeContainerDefinitionId != INDEX_NONE
		? RuntimeContainerDefinitionId
		: ContainerDefinitionId;
	RuntimeInstance.ContentsId = RuntimeContentsId != INDEX_NONE
		? RuntimeContentsId
		: ContentsId;
	RuntimeInstance.DisplayName = RuntimeDisplayName.IsEmpty()
		? FText::FromString(FString::Printf(TEXT("Container %d"), RuntimeInstance.ContainerDefinitionId))
		: RuntimeDisplayName;
	RuntimeInstance.Capacity = FMath::Max(0, RuntimeCapacity);
	return RuntimeInstance;
}

UTunaSweeperItemDataSubsystem* ATunaSweeperLootContainerActor::GetItemDataSubsystem() const
{
	const UGameInstance* GameInstance = GetGameInstance();
	return GameInstance
		? GameInstance->GetSubsystem<UTunaSweeperItemDataSubsystem>()
		: nullptr;
}
