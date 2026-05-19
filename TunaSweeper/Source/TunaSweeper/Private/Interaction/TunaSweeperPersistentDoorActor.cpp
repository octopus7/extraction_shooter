#include "Interaction/TunaSweeperPersistentDoorActor.h"

#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Interaction/TunaSweeperInteractableComponent.h"
#include "UObject/ConstructorHelpers.h"

ATunaSweeperPersistentDoorActor::ATunaSweeperPersistentDoorActor()
{
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	BlockingCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("BlockingCollision"));
	BlockingCollision->SetupAttachment(RootComponent);
	BlockingCollision->SetHiddenInGame(true);
	BlockingCollision->SetVisibility(false);
	BlockingCollision->SetCanEverAffectNavigation(true);

	VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
	VisualMesh->SetupAttachment(RootComponent);
	VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		VisualMesh->SetStaticMesh(CubeMesh.Object);
	}

	InteractableComponent = CreateDefaultSubobject<UTunaSweeperInteractableComponent>(TEXT("Interactable"));
	InteractableComponent->SetupAttachment(RootComponent);

	ApplyCollisionDefaults();
	ApplyDoorState();
	RefreshPresentation();
}

void ATunaSweeperPersistentDoorActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	ApplyCollisionDefaults();
	ApplyDoorState();
	RefreshPresentation();
}

void ATunaSweeperPersistentDoorActor::BeginPlay()
{
	Super::BeginPlay();

	ApplySavedState();
	RefreshPresentation();
}

void ATunaSweeperPersistentDoorActor::ConfigurePersistentDoorDefaults(
	FName InDoorObjectId,
	FName InDoorInfoId,
	const FText& InDisplayName,
	const FText& InInteractionDisplayName,
	const FVector& InBlockingBoxExtent,
	const FVector& InDoorMeshRelativeLocation,
	const FVector& InDoorMeshRelativeScale,
	const FRotator& InClosedDoorRelativeRotation,
	const FRotator& InOpenDoorRelativeRotation)
{
	Modify();
	DoorObjectId = InDoorObjectId;
	if (!InDoorInfoId.IsNone())
	{
		DoorInfoId = InDoorInfoId;
	}
	DisplayName = InDisplayName.IsEmpty()
		? FText::FromString(TEXT("\uBB38"))
		: InDisplayName;
	InteractionDisplayName = InInteractionDisplayName.IsEmpty()
		? FText::FromString(TEXT("\uC5F4\uAE30"))
		: InInteractionDisplayName;
	BlockingBoxExtent = FVector(
		FMath::Max(1.0f, InBlockingBoxExtent.X),
		FMath::Max(1.0f, InBlockingBoxExtent.Y),
		FMath::Max(1.0f, InBlockingBoxExtent.Z));
	DoorMeshRelativeLocation = InDoorMeshRelativeLocation;
	DoorMeshRelativeScale = FVector(
		FMath::Max(0.01f, InDoorMeshRelativeScale.X),
		FMath::Max(0.01f, InDoorMeshRelativeScale.Y),
		FMath::Max(0.01f, InDoorMeshRelativeScale.Z));
	ClosedDoorRelativeRotation = InClosedDoorRelativeRotation;
	OpenDoorRelativeRotation = InOpenDoorRelativeRotation;

	ApplyCollisionDefaults();
	ApplyDoorState();
	RefreshPresentation();
}

bool ATunaSweeperPersistentDoorActor::OpenDoor(bool bSaveImmediately)
{
	if (bOpen)
	{
		return true;
	}

	if (UTunaSweeperGameInstance* TunaGameInstance = GetTunaGameInstance())
	{
		TunaGameInstance->UpdateWorldProgressState(
			GetEffectiveDoorObjectId(),
			DoorInfoId,
			ETunaSweeperWorldProgressState::Completed,
			1,
			1,
			bSaveImmediately);
	}

	bOpen = true;
	ApplyDoorState();
	RefreshPresentation();
	return true;
}

void ATunaSweeperPersistentDoorActor::ApplyCollisionDefaults()
{
	if (!BlockingCollision)
	{
		return;
	}

	BlockingCollision->SetBoxExtent(BlockingBoxExtent);
	BlockingCollision->SetCollisionObjectType(ECC_WorldStatic);
	BlockingCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
	BlockingCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	BlockingCollision->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	BlockingCollision->SetGenerateOverlapEvents(false);
	BlockingCollision->CanCharacterStepUpOn = ECB_No;
	BlockingCollision->SetHiddenInGame(true);
	BlockingCollision->SetVisibility(false);
}

void ATunaSweeperPersistentDoorActor::ApplyDoorState()
{
	if (VisualMesh)
	{
		VisualMesh->SetRelativeLocation(DoorMeshRelativeLocation);
		VisualMesh->SetRelativeScale3D(DoorMeshRelativeScale);
		VisualMesh->SetRelativeRotation(bOpen ? OpenDoorRelativeRotation : ClosedDoorRelativeRotation);
	}

	if (BlockingCollision)
	{
		BlockingCollision->SetCollisionEnabled(bOpen ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryAndPhysics);
	}

	if (InteractableComponent)
	{
		InteractableComponent->SetRelativeLocation(InteractableRelativeLocation);
	}
}

void ATunaSweeperPersistentDoorActor::RefreshPresentation()
{
	if (InteractableComponent)
	{
		const FText MarkerText = FText::Format(
			FText::FromString(TEXT("{0}    {1}")),
			InteractionDisplayName,
			DisplayName);
		InteractableComponent->SetInteractionTypeAndDisplayName(
			bOpen ? ETunaSweeperInteractionType::None : ETunaSweeperInteractionType::PersistentDoor,
			bOpen ? FText::GetEmpty() : MarkerText);
	}
}

void ATunaSweeperPersistentDoorActor::ApplySavedState()
{
	const FTunaSweeperWorldProgressSaveData DoorState = GetOrCreateDoorState();
	bOpen = DoorState.State == ETunaSweeperWorldProgressState::Completed;
	ApplyDoorState();
}

FName ATunaSweeperPersistentDoorActor::GetEffectiveDoorObjectId() const
{
	return DoorObjectId.IsNone() ? GetFName() : DoorObjectId;
}

FTunaSweeperWorldProgressSaveData ATunaSweeperPersistentDoorActor::GetOrCreateDoorState() const
{
	UTunaSweeperGameInstance* TunaGameInstance = GetTunaGameInstance();
	if (!TunaGameInstance)
	{
		FTunaSweeperWorldProgressSaveData EmptyState;
		EmptyState.ObjectId = GetEffectiveDoorObjectId();
		EmptyState.InfoId = DoorInfoId;
		EmptyState.ProgressQuantity = bOpen ? 1 : 0;
		EmptyState.State = bOpen
			? ETunaSweeperWorldProgressState::Completed
			: ETunaSweeperWorldProgressState::InProgress;
		return EmptyState;
	}

	return TunaGameInstance->GetOrCreateWorldProgressState(
		GetEffectiveDoorObjectId(),
		DoorInfoId,
		bOpen ? 1 : 0,
		1);
}

UTunaSweeperGameInstance* ATunaSweeperPersistentDoorActor::GetTunaGameInstance() const
{
	return GetGameInstance<UTunaSweeperGameInstance>();
}
