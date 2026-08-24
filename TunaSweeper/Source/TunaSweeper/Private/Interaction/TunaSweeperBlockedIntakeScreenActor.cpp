#include "Interaction/TunaSweeperBlockedIntakeScreenActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Interaction/TunaSweeperInteractableComponent.h"
#include "Subsystem/TunaSweeperItemDataSubsystem.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	const TCHAR* WaterIntakeMeshPath = TEXT("/Game/Meshes/Props/WaterIntake/SM_SKM_WaterIntake.SM_SKM_WaterIntake");
}

ATunaSweeperBlockedIntakeScreenActor::ATunaSweeperBlockedIntakeScreenActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
	VisualMesh->SetupAttachment(RootComponent);
	VisualMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	VisualMesh->SetCollisionObjectType(ECC_WorldStatic);
	VisualMesh->SetCollisionResponseToAllChannels(ECR_Block);
	VisualMesh->SetCanEverAffectNavigation(true);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> WaterIntakeMesh(WaterIntakeMeshPath);
	if (WaterIntakeMesh.Succeeded())
	{
		BlockedScreenMesh = WaterIntakeMesh.Object;
		ClearedScreenMesh = WaterIntakeMesh.Object;
		VisualMesh->SetStaticMesh(WaterIntakeMesh.Object);
	}

	InteractableComponent = CreateDefaultSubobject<UTunaSweeperInteractableComponent>(TEXT("Interactable"));
	InteractableComponent->SetupAttachment(RootComponent);
	InteractableComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 140.0f));

	RefreshPresentation();
}

void ATunaSweeperBlockedIntakeScreenActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	const UWorld* World = GetWorld();
	bScreenCleared = World && !World->IsGameWorld() && bPreviewClearedStateInEditor;
	ApplyVisualState();
	RefreshPresentation();
}

void ATunaSweeperBlockedIntakeScreenActor::BeginPlay()
{
	Super::BeginPlay();

	if (UTunaSweeperGameInstance* TunaGameInstance = GetTunaGameInstance())
	{
		TunaGameInstance->OnInventoryStateChanged.RemoveAll(this);
		TunaGameInstance->OnInventoryStateChanged.AddUObject(
			this,
			&ATunaSweeperBlockedIntakeScreenActor::RefreshPresentation);
		TunaGameInstance->OnLanguageChanged.RemoveAll(this);
		TunaGameInstance->OnLanguageChanged.AddUObject(
			this,
			&ATunaSweeperBlockedIntakeScreenActor::RefreshPresentation);
	}

	ApplySavedState();
}

void ATunaSweeperBlockedIntakeScreenActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UTunaSweeperGameInstance* TunaGameInstance = GetTunaGameInstance())
	{
		TunaGameInstance->OnInventoryStateChanged.RemoveAll(this);
		TunaGameInstance->OnLanguageChanged.RemoveAll(this);
	}

	Super::EndPlay(EndPlayReason);
}

bool ATunaSweeperBlockedIntakeScreenActor::CanClearScreen() const
{
	if (bScreenCleared)
	{
		return false;
	}

	if (!bRequiresItem)
	{
		return true;
	}

	UTunaSweeperGameInstance* TunaGameInstance = GetTunaGameInstance();
	return TunaGameInstance &&
		RequiredItemId != INDEX_NONE &&
		TunaGameInstance->CountInventoryItemById(RequiredItemId) >= FMath::Max(1, RequiredItemQuantity);
}

bool ATunaSweeperBlockedIntakeScreenActor::ClearScreen(bool bSaveImmediately)
{
	if (!CanClearScreen())
	{
		RefreshPresentation();
		return false;
	}

	UTunaSweeperGameInstance* TunaGameInstance = GetTunaGameInstance();
	if (!TunaGameInstance)
	{
		return false;
	}

	const int32 ClampedRequiredQuantity = FMath::Max(1, RequiredItemQuantity);
	if (bRequiresItem && bConsumeRequiredItem &&
		TunaGameInstance->ConsumeInventoryItemById(RequiredItemId, ClampedRequiredQuantity) != ClampedRequiredQuantity)
	{
		RefreshPresentation();
		return false;
	}

	if (!TunaGameInstance->UpdateWorldProgressState(
		GetEffectiveProgressObjectId(),
		ProgressInfoId,
		ETunaSweeperWorldProgressState::Completed,
		1,
		1,
		bSaveImmediately))
	{
		return false;
	}

	bScreenCleared = true;
	ApplyVisualState();
	RefreshPresentation();
	return true;
}

void ATunaSweeperBlockedIntakeScreenActor::ApplySavedState()
{
	bScreenCleared = GetOrCreateProgressState().State == ETunaSweeperWorldProgressState::Completed;
	ApplyVisualState();
	RefreshPresentation();
}

void ATunaSweeperBlockedIntakeScreenActor::ApplyVisualState()
{
	if (!VisualMesh)
	{
		return;
	}

	UStaticMesh* TargetMesh = bScreenCleared
		? ClearedScreenMesh.LoadSynchronous()
		: BlockedScreenMesh.LoadSynchronous();
	VisualMesh->SetStaticMesh(TargetMesh);
	VisualMesh->SetVisibility(TargetMesh != nullptr, true);
}

void ATunaSweeperBlockedIntakeScreenActor::RefreshPresentation()
{
	if (!InteractableComponent)
	{
		return;
	}

	InteractableComponent->SetInteractionTypeDisplayNameAndStringKey(
		bScreenCleared ? ETunaSweeperInteractionType::None : ETunaSweeperInteractionType::WorldProgress,
		bScreenCleared ? FText::GetEmpty() : ResolveInteractionDisplayName(),
		bScreenCleared ? NAME_None : InteractionDisplayNameStringKey);
	InteractableComponent->SetObjectiveEventId(ObjectiveEventId);
	InteractableComponent->SetInteractionRequirementPreview(
		bRequiresItem ? LoadRequiredItemIconTexture() : nullptr,
		FMath::Max(1, RequiredItemQuantity),
		!bScreenCleared && bRequiresItem && RequiredItemId != INDEX_NONE);
	InteractableComponent->SetMarkerCompleted(bScreenCleared);
}

FText ATunaSweeperBlockedIntakeScreenActor::ResolveInteractionDisplayName() const
{
	const UTunaSweeperGameInstance* TunaGameInstance = GetTunaGameInstance();
	return TunaGameInstance && !InteractionDisplayNameStringKey.IsNone()
		? TunaGameInstance->ResolveLocalizedText(InteractionDisplayNameStringKey, InteractionDisplayName)
		: InteractionDisplayName;
}

UTexture2D* ATunaSweeperBlockedIntakeScreenActor::LoadRequiredItemIconTexture() const
{
	UGameInstance* GameInstance = GetGameInstance();
	UTunaSweeperItemDataSubsystem* ItemDataSubsystem = GameInstance
		? GameInstance->GetSubsystem<UTunaSweeperItemDataSubsystem>()
		: nullptr;
	if (!ItemDataSubsystem)
	{
		return nullptr;
	}

	FTunaSweeperItemDefinition ItemDefinition;
	if (!ItemDataSubsystem->TryGetItemDefinition(RequiredItemId, ItemDefinition))
	{
		return nullptr;
	}

	const FString IconObjectPath = ItemDataSubsystem->BuildItemIconObjectPath(ItemDefinition);
	return IconObjectPath.IsEmpty()
		? nullptr
		: LoadObject<UTexture2D>(nullptr, *IconObjectPath);
}

FName ATunaSweeperBlockedIntakeScreenActor::GetEffectiveProgressObjectId() const
{
	return ProgressObjectId.IsNone() ? GetFName() : ProgressObjectId;
}

FTunaSweeperWorldProgressSaveData ATunaSweeperBlockedIntakeScreenActor::GetOrCreateProgressState() const
{
	UTunaSweeperGameInstance* TunaGameInstance = GetTunaGameInstance();
	if (!TunaGameInstance)
	{
		FTunaSweeperWorldProgressSaveData EmptyState;
		EmptyState.ObjectId = GetEffectiveProgressObjectId();
		EmptyState.InfoId = ProgressInfoId;
		return EmptyState;
	}

	return TunaGameInstance->GetOrCreateWorldProgressState(
		GetEffectiveProgressObjectId(),
		ProgressInfoId,
		0,
		1);
}

UTunaSweeperGameInstance* ATunaSweeperBlockedIntakeScreenActor::GetTunaGameInstance() const
{
	return GetGameInstance<UTunaSweeperGameInstance>();
}
