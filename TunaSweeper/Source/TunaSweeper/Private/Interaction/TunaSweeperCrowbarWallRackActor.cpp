#include "Interaction/TunaSweeperCrowbarWallRackActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Interaction/TunaSweeperInteractableComponent.h"
#include "Inventory/TunaSweeperSaveGame.h"

ATunaSweeperCrowbarWallRackActor::ATunaSweeperCrowbarWallRackActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	PedestalMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PedestalMesh"));
	PedestalMesh->SetupAttachment(RootComponent);
	PedestalMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	PedestalMesh->SetCollisionObjectType(ECC_WorldStatic);
	PedestalMesh->SetCollisionResponseToAllChannels(ECR_Block);
	PedestalMesh->SetCanEverAffectNavigation(true);
	PedestalMesh->bEditableWhenInherited = true;

	CrowbarMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CrowbarMesh"));
	CrowbarMesh->SetupAttachment(RootComponent);
	CrowbarMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CrowbarMesh->SetCanEverAffectNavigation(false);
	CrowbarMesh->bEditableWhenInherited = true;

	InteractableComponent = CreateDefaultSubobject<UTunaSweeperInteractableComponent>(TEXT("Interactable"));
	InteractableComponent->SetupAttachment(RootComponent);
	InteractableComponent->SetRelativeLocation(InteractionLocation);
	InteractableComponent->ConfigureInteractionDefaults(
		ETunaSweeperInteractionType::ItemSpawn,
		InteractionDisplayName,
		TSoftClassPtr<UTunaSweeperInteractionMarkerWidget>(
			FSoftObjectPath(TEXT("/Game/UI/WBP_InteractionMarker.WBP_InteractionMarker_C"))),
		InteractionDisplayNameStringKey);

	RefreshAvailability();
}

void ATunaSweeperCrowbarWallRackActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RefreshAvailability();
}

void ATunaSweeperCrowbarWallRackActor::BeginPlay()
{
	Super::BeginPlay();

	if (UTunaSweeperGameInstance* TunaGameInstance = GetTunaGameInstance())
	{
		TunaGameInstance->OnInventoryStateChanged.RemoveAll(this);
		TunaGameInstance->OnInventoryStateChanged.AddUObject(
			this,
			&ATunaSweeperCrowbarWallRackActor::RefreshAvailability);
	}

	RefreshAvailability();
}

void ATunaSweeperCrowbarWallRackActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UTunaSweeperGameInstance* TunaGameInstance = GetTunaGameInstance())
	{
		TunaGameInstance->OnInventoryStateChanged.RemoveAll(this);
	}

	Super::EndPlay(EndPlayReason);
}

bool ATunaSweeperCrowbarWallRackActor::SupplyCrowbar(APawn* InstigatorPawn)
{
	if (!InstigatorPawn || !bCrowbarAvailable)
	{
		return false;
	}

	UTunaSweeperGameInstance* TunaGameInstance = GetTunaGameInstance();
	if (!TunaGameInstance || !TunaGameInstance->AddItemToPreferredAvailableSlot(CrowbarItemId, 1))
	{
		RefreshAvailability();
		return false;
	}

	// Inventory changes are persisted by the inventory system. This actor deliberately
	// keeps no pickup state, so entering the bunker again re-evaluates the two conditions.
	RefreshAvailability();
	return true;
}

void ATunaSweeperCrowbarWallRackActor::RefreshAvailability()
{
	bCrowbarAvailable = !DoesPlayerOwnCrowbar() && !IsWaterIntakeDebrisCleared();

	if (PedestalMesh)
	{
		PedestalMesh->SetVisibility(PedestalMesh->GetStaticMesh() != nullptr, true);
	}

	if (CrowbarMesh)
	{
		CrowbarMesh->SetVisibility(bCrowbarAvailable && CrowbarMesh->GetStaticMesh() != nullptr, true);
	}

	if (InteractableComponent)
	{
		InteractableComponent->SetRelativeLocation(InteractionLocation);
		InteractableComponent->SetInteractionTypeDisplayNameAndStringKey(
			bCrowbarAvailable ? ETunaSweeperInteractionType::ItemSpawn : ETunaSweeperInteractionType::None,
			bCrowbarAvailable ? InteractionDisplayName : FText::GetEmpty(),
			bCrowbarAvailable ? InteractionDisplayNameStringKey : NAME_None);
	}
}

bool ATunaSweeperCrowbarWallRackActor::DoesPlayerOwnCrowbar() const
{
	UTunaSweeperGameInstance* TunaGameInstance = GetTunaGameInstance();
	return TunaGameInstance && TunaGameInstance->CountInventoryItemById(CrowbarItemId) > 0;
}

bool ATunaSweeperCrowbarWallRackActor::IsWaterIntakeDebrisCleared() const
{
	UTunaSweeperGameInstance* TunaGameInstance = GetTunaGameInstance();
	if (!TunaGameInstance || WaterIntakeProgressObjectId.IsNone())
	{
		return false;
	}

	FTunaSweeperWorldProgressSaveData ProgressState;
	return TunaGameInstance->TryGetWorldProgressState(WaterIntakeProgressObjectId, ProgressState) &&
		ProgressState.State == ETunaSweeperWorldProgressState::Completed;
}

UTunaSweeperGameInstance* ATunaSweeperCrowbarWallRackActor::GetTunaGameInstance() const
{
	return GetGameInstance<UTunaSweeperGameInstance>();
}
