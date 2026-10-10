#include "Interaction/TunaSweeperPowerFacilityActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Interaction/TunaSweeperInteractableComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "Subsystem/TunaSweeperItemDataSubsystem.h"
#include "Subsystem/TunaSweeperQuestSubsystem.h"

ATunaSweeperPowerFacilityActor::ATunaSweeperPowerFacilityActor()
{
	PrimaryActorTick.bCanEverTick = false;
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
	FacilityMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FacilityMesh"));
	FacilityMesh->SetupAttachment(SceneRoot);
	FacilityMesh->SetCollisionProfileName(TEXT("BlockAll"));
	FacilityMesh->SetCanEverAffectNavigation(true);
	FaultSparks = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("FaultSparks"));
	FaultSparks->SetupAttachment(SceneRoot);
	FaultSparks->bAutoActivate = false; // Restore saved state before starting simulation.
	FaultSparks->bAutoDestroy = false;
	FaultSparks->SetRelativeLocation(FVector(0, 0, 100));
	Interactable = CreateDefaultSubobject<UTunaSweeperInteractableComponent>(TEXT("Interactable"));
	Interactable->SetupAttachment(SceneRoot);
	Interactable->SetRelativeLocation(FVector(0, 0, 140));
}

void ATunaSweeperPowerFacilityActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	if (GetWorld() && !GetWorld()->IsGameWorld())
	{
		bRepaired = bPreviewRepairedInEditor;
		ApplyPresentation();
	}
}

void ATunaSweeperPowerFacilityActor::BeginPlay()
{
	Super::BeginPlay();
	if (auto* Game = GetTunaGameInstance())
	{
		Game->OnWorldProgressChanged.AddUObject(this, &ThisClass::RefreshFacilityState);
		Game->OnInventoryStateChanged.AddUObject(this, &ThisClass::RefreshFacilityState);
		Game->OnLanguageChanged.AddUObject(this, &ThisClass::RefreshFacilityState);
		if (auto* Quests = Game->GetSubsystem<UTunaSweeperQuestSubsystem>())
			Quests->OnQuestProgressChanged.AddUObject(this, &ThisClass::RefreshFacilityState);
	}
	RefreshFacilityState();
}

void ATunaSweeperPowerFacilityActor::EndPlay(const EEndPlayReason::Type Reason)
{
	if (auto* Game = GetTunaGameInstance())
	{
		Game->OnWorldProgressChanged.RemoveAll(this);
		Game->OnInventoryStateChanged.RemoveAll(this);
		Game->OnLanguageChanged.RemoveAll(this);
		if (auto* Quests = Game->GetSubsystem<UTunaSweeperQuestSubsystem>())
			Quests->OnQuestProgressChanged.RemoveAll(this);
	}
	Super::EndPlay(Reason);
}

bool ATunaSweeperPowerFacilityActor::IsRepairInteractionAvailable() const
{
	if (bRepaired || ProgressObjectId.IsNone()) return false;
	if (!bRequireActiveQuest) return true;
	auto* Game = GetTunaGameInstance();
	auto* Quests = Game ? Game->GetSubsystem<UTunaSweeperQuestSubsystem>() : nullptr;
	if (!Quests || RepairQuestId <= 0 || RepairObjectiveId.IsNone() || RepairObjectiveEventId.IsNone() ||
		Quests->GetQuestState(RepairQuestId) != ETunaSweeperQuestState::Accepted) return false;
	TArray<FTunaSweeperObjectiveProgressView> Objectives;
	if (!Quests->GetQuestObjectiveProgress(RepairQuestId, Objectives)) return false;
	return Objectives.ContainsByPredicate([this](const FTunaSweeperObjectiveProgressView& Objective)
	{
		return Objective.ObjectiveId == RepairObjectiveId && !Objective.bCompleted;
	});
}

bool ATunaSweeperPowerFacilityActor::CanRepair() const
{
	auto* Game = GetTunaGameInstance();
	if (!Game || bRepairInProgress || !IsRepairInteractionAvailable()) return false;
	if (!bRequiresItem) return true;
	const int32 Owned = bConsumeRequiredItem ? Game->CountInventoryItemById(RequiredItemId)
		: Game->CountCarriedItemById(RequiredItemId);
	return RequiredItemId > 0 && Owned >= FMath::Max(1, RequiredItemQuantity);
}

bool ATunaSweeperPowerFacilityActor::Repair(bool bSaveImmediately)
{
	RefreshFacilityState();
	if (!CanRepair()) return false;
	TGuardValue<bool> RepairGuard(bRepairInProgress, true);
	auto* Game = GetTunaGameInstance();
	const int32 Quantity = FMath::Max(1, RequiredItemQuantity);
	if (bRequiresItem && bConsumeRequiredItem && Game->ConsumeInventoryItemById(RequiredItemId, Quantity) != Quantity)
	{
		RefreshFacilityState();
		return false;
	}
	const bool bUpdated = Game->UpdateWorldProgressState(ProgressObjectId, TEXT("world_progress.power_facility"),
		ETunaSweeperWorldProgressState::Completed, 1, 1, bSaveImmediately);
	RefreshFacilityState();
	return bUpdated;
}

void ATunaSweeperPowerFacilityActor::RefreshFacilityState()
{
	if (GetWorld() && !GetWorld()->IsGameWorld())
	{
		bRepaired = bPreviewRepairedInEditor;
	}
	else
	{
		bRepaired = false;
		if (auto* Game = GetTunaGameInstance(); Game && !ProgressObjectId.IsNone())
		{
			const auto State = Game->GetOrCreateWorldProgressState(ProgressObjectId, TEXT("world_progress.power_facility"), 0, 1);
			bRepaired = State.State == ETunaSweeperWorldProgressState::Completed;
		}
	}
	ApplyPresentation();
}

void ATunaSweeperPowerFacilityActor::ApplyPresentation()
{
	FaultSparks->SetVisibility(!bRepaired, true);
	FaultSparks->SetHiddenInGame(bRepaired, true);
	if (bRepaired)
	{
		FaultSparks->DeactivateImmediate();
	}
	else if (FaultSparks->Template && FaultSparks->IsRegistered() && !FaultSparks->IsActive())
	{
		FaultSparks->ActivateSystem(true);
	}
	const bool bAvailable = IsRepairInteractionAvailable();
	auto* Game = GetTunaGameInstance();
	const FText Label = Game && !InteractionStringKey.IsNone()
		? Game->ResolveLocalizedText(InteractionStringKey, FText::GetEmpty()) : FText::GetEmpty();
	Interactable->SetInteractionTypeDisplayNameAndStringKey(
		bAvailable ? ETunaSweeperInteractionType::WorldProgress : ETunaSweeperInteractionType::None,
		bAvailable ? Label : FText::GetEmpty(), bAvailable ? InteractionStringKey : NAME_None);
	Interactable->SetObjectiveEventId(bAvailable ? RepairObjectiveEventId : NAME_None);
	UTexture2D* Icon = nullptr;
	if (auto* Items = Game ? Game->GetSubsystem<UTunaSweeperItemDataSubsystem>() : nullptr;
		Items && bRequiresItem && RequiredItemId > 0)
	{
		FTunaSweeperItemDefinition Item;
		if (Items->TryGetItemDefinition(RequiredItemId, Item))
		{
			const FString Path = Items->BuildItemIconObjectPath(Item);
			if (!Path.IsEmpty()) Icon = LoadObject<UTexture2D>(nullptr, *Path);
		}
	}
	Interactable->SetInteractionRequirementPreview(Icon, FMath::Max(1, RequiredItemQuantity), bAvailable && bRequiresItem);
	Interactable->SetMarkerCompleted(bRepaired);
}

UTunaSweeperGameInstance* ATunaSweeperPowerFacilityActor::GetTunaGameInstance() const
{
	return GetGameInstance<UTunaSweeperGameInstance>();
}
