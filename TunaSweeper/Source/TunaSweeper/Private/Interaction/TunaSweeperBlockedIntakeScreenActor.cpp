#include "Interaction/TunaSweeperBlockedIntakeScreenActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Interaction/TunaSweeperInteractableComponent.h"
#include "Quest/TunaSweeperQuestTypes.h"
#include "Subsystem/TunaSweeperItemDataSubsystem.h"
#include "Subsystem/TunaSweeperQuestSubsystem.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	const TCHAR* WaterIntakeMeshPath = TEXT("/Game/Meshes/Props/WaterIntake/SM_WaterIntake.SM_WaterIntake");
	const TCHAR* WaterIntakeScreenMeshPath = TEXT("/Game/Environment/Water/SM_WaterIntakeScreen.SM_WaterIntakeScreen");
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
		VisualMesh->SetStaticMesh(WaterIntakeMesh.Object);
	}

	ScreenMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ScreenMesh"));
	ScreenMesh->SetupAttachment(RootComponent);
	ScreenMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	ScreenMesh->SetCollisionObjectType(ECC_WorldStatic);
	ScreenMesh->SetCollisionResponseToAllChannels(ECR_Block);
	ScreenMesh->SetCanEverAffectNavigation(true);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> WaterIntakeScreenMesh(WaterIntakeScreenMeshPath);
	if (WaterIntakeScreenMesh.Succeeded())
	{
		ScreenMesh->SetStaticMesh(WaterIntakeScreenMesh.Object);
	}

	DebrisMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DebrisMesh"));
	DebrisMesh->SetupAttachment(RootComponent);
	DebrisMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	DebrisMesh->SetCollisionObjectType(ECC_WorldStatic);
	DebrisMesh->SetCollisionResponseToAllChannels(ECR_Block);
	DebrisMesh->SetCanEverAffectNavigation(true);

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

		if (UTunaSweeperQuestSubsystem* QuestSubsystem = TunaGameInstance->GetSubsystem<UTunaSweeperQuestSubsystem>())
		{
			QuestSubsystem->OnQuestProgressChanged.RemoveAll(this);
			QuestSubsystem->OnQuestProgressChanged.AddUObject(
				this,
				&ATunaSweeperBlockedIntakeScreenActor::RefreshPresentation);
		}
	}

	ApplySavedState();
}

void ATunaSweeperBlockedIntakeScreenActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UTunaSweeperGameInstance* TunaGameInstance = GetTunaGameInstance())
	{
		TunaGameInstance->OnInventoryStateChanged.RemoveAll(this);
		TunaGameInstance->OnLanguageChanged.RemoveAll(this);
		if (UTunaSweeperQuestSubsystem* QuestSubsystem = TunaGameInstance->GetSubsystem<UTunaSweeperQuestSubsystem>())
		{
			QuestSubsystem->OnQuestProgressChanged.RemoveAll(this);
		}
	}

	Super::EndPlay(EndPlayReason);
}

bool ATunaSweeperBlockedIntakeScreenActor::Interact(bool bSaveImmediately)
{
	switch (ActiveInteractionPhase)
	{
	case ETunaSweeperWaterIntakeInteractionPhase::Inspect:
		return true;
	case ETunaSweeperWaterIntakeInteractionPhase::ClearDebris:
		return ClearScreen(bSaveImmediately);
	case ETunaSweeperWaterIntakeInteractionPhase::RepairValve:
		return RepairValve(bSaveImmediately);
	default:
		return false;
	}
}

bool ATunaSweeperBlockedIntakeScreenActor::CanClearScreen() const
{
	if (bScreenCleared || ActiveInteractionPhase != ETunaSweeperWaterIntakeInteractionPhase::ClearDebris)
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

bool ATunaSweeperBlockedIntakeScreenActor::CanRepairValve() const
{
	if (!bScreenCleared || bValveRepaired ||
		ActiveInteractionPhase != ETunaSweeperWaterIntakeInteractionPhase::RepairValve)
	{
		return false;
	}

	UTunaSweeperGameInstance* TunaGameInstance = GetTunaGameInstance();
	return TunaGameInstance &&
		ValveRequiredItemId != INDEX_NONE &&
		TunaGameInstance->CountInventoryItemById(ValveRequiredItemId) >= FMath::Max(1, ValveRequiredItemQuantity);
}

bool ATunaSweeperBlockedIntakeScreenActor::RepairValve(bool bSaveImmediately)
{
	if (!CanRepairValve())
	{
		RefreshPresentation();
		return false;
	}

	UTunaSweeperGameInstance* TunaGameInstance = GetTunaGameInstance();
	if (!TunaGameInstance)
	{
		return false;
	}

	const int32 ClampedRequiredQuantity = FMath::Max(1, ValveRequiredItemQuantity);
	if (bConsumeValveRequiredItem &&
		TunaGameInstance->ConsumeInventoryItemById(ValveRequiredItemId, ClampedRequiredQuantity) != ClampedRequiredQuantity)
	{
		RefreshPresentation();
		return false;
	}

	if (!TunaGameInstance->UpdateWorldProgressState(
		GetEffectiveValveProgressObjectId(),
		ValveProgressInfoId,
		ETunaSweeperWorldProgressState::Completed,
		1,
		1,
		bSaveImmediately))
	{
		return false;
	}

	bValveRepaired = true;
	RefreshPresentation();
	return true;
}

void ATunaSweeperBlockedIntakeScreenActor::ApplySavedState()
{
	bScreenCleared = GetOrCreateProgressState(GetEffectiveProgressObjectId(), ProgressInfoId).State ==
		ETunaSweeperWorldProgressState::Completed;
	bValveRepaired = GetOrCreateProgressState(GetEffectiveValveProgressObjectId(), ValveProgressInfoId).State ==
		ETunaSweeperWorldProgressState::Completed;
	ApplyVisualState();
	RefreshPresentation();
}

void ATunaSweeperBlockedIntakeScreenActor::ApplyVisualState()
{
	if (VisualMesh)
	{
		VisualMesh->SetVisibility(VisualMesh->GetStaticMesh() != nullptr, true);
	}

	if (ScreenMesh)
	{
		ScreenMesh->SetVisibility(ScreenMesh->GetStaticMesh() != nullptr, true);
	}

	if (DebrisMesh)
	{
		const bool bShowDebris = !bScreenCleared && DebrisMesh->GetStaticMesh() != nullptr;
		DebrisMesh->SetVisibility(bShowDebris, true);
		DebrisMesh->SetCollisionEnabled(
			bShowDebris ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
	}
}

void ATunaSweeperBlockedIntakeScreenActor::RefreshPresentation()
{
	if (!InteractableComponent)
	{
		return;
	}

	ActiveInteractionPhase = ResolveActiveInteractionPhase();

	FText DisplayName;
	FName DisplayNameStringKey = NAME_None;
	FName ObjectiveEventId = NAME_None;
	FVector InteractionLocation = ClearDebrisInteractionLocation;
	UTexture2D* RequirementIcon = nullptr;
	int32 RequirementQuantity = 0;
	bool bShowRequirement = false;

	switch (ActiveInteractionPhase)
	{
	case ETunaSweeperWaterIntakeInteractionPhase::Inspect:
		DisplayName = ResolveLocalizedText(InspectInteractionDisplayNameStringKey, InspectInteractionDisplayName);
		DisplayNameStringKey = InspectInteractionDisplayNameStringKey;
		ObjectiveEventId = InspectObjectiveEventId;
		InteractionLocation = InspectInteractionLocation;
		break;
	case ETunaSweeperWaterIntakeInteractionPhase::ClearDebris:
		DisplayName = ResolveLocalizedText(ClearDebrisInteractionDisplayNameStringKey, ClearDebrisInteractionDisplayName);
		DisplayNameStringKey = ClearDebrisInteractionDisplayNameStringKey;
		ObjectiveEventId = ClearDebrisObjectiveEventId;
		InteractionLocation = ClearDebrisInteractionLocation;
		RequirementIcon = bRequiresItem ? LoadItemIconTexture(RequiredItemId) : nullptr;
		RequirementQuantity = FMath::Max(1, RequiredItemQuantity);
		bShowRequirement = bRequiresItem && RequiredItemId != INDEX_NONE;
		break;
	case ETunaSweeperWaterIntakeInteractionPhase::RepairValve:
		DisplayName = ResolveLocalizedText(RepairValveInteractionDisplayNameStringKey, RepairValveInteractionDisplayName);
		DisplayNameStringKey = RepairValveInteractionDisplayNameStringKey;
		ObjectiveEventId = RepairValveObjectiveEventId;
		InteractionLocation = RepairValveInteractionLocation;
		RequirementIcon = LoadItemIconTexture(ValveRequiredItemId);
		RequirementQuantity = FMath::Max(1, ValveRequiredItemQuantity);
		bShowRequirement = ValveRequiredItemId != INDEX_NONE;
		break;
	default:
		break;
	}

	const bool bHasActiveInteraction = ActiveInteractionPhase != ETunaSweeperWaterIntakeInteractionPhase::None;
	InteractableComponent->SetRelativeLocation(InteractionLocation);
	InteractableComponent->SetInteractionTypeDisplayNameAndStringKey(
		bHasActiveInteraction ? ETunaSweeperInteractionType::WorldProgress : ETunaSweeperInteractionType::None,
		bHasActiveInteraction ? DisplayName : FText::GetEmpty(),
		bHasActiveInteraction ? DisplayNameStringKey : NAME_None);
	InteractableComponent->SetObjectiveEventId(bHasActiveInteraction ? ObjectiveEventId : NAME_None);
	InteractableComponent->SetInteractionRequirementPreview(
		RequirementIcon,
		RequirementQuantity,
		bHasActiveInteraction && bShowRequirement);
	InteractableComponent->SetMarkerCompleted(!bHasActiveInteraction && bScreenCleared && bValveRepaired);
}

ETunaSweeperWaterIntakeInteractionPhase ATunaSweeperBlockedIntakeScreenActor::ResolveActiveInteractionPhase() const
{
	if (IsQuestObjectiveActive(InspectQuestId, InspectObjectiveId))
	{
		return ETunaSweeperWaterIntakeInteractionPhase::Inspect;
	}

	if (!bScreenCleared && IsQuestObjectiveActive(ClearDebrisQuestId, ClearDebrisObjectiveId))
	{
		return ETunaSweeperWaterIntakeInteractionPhase::ClearDebris;
	}

	if (bScreenCleared && !bValveRepaired && IsQuestObjectiveActive(RepairValveQuestId, RepairValveObjectiveId))
	{
		return ETunaSweeperWaterIntakeInteractionPhase::RepairValve;
	}

	return ETunaSweeperWaterIntakeInteractionPhase::None;
}

bool ATunaSweeperBlockedIntakeScreenActor::IsQuestObjectiveActive(FName QuestId, FName ObjectiveId) const
{
	UTunaSweeperGameInstance* TunaGameInstance = GetTunaGameInstance();
	UTunaSweeperQuestSubsystem* QuestSubsystem = TunaGameInstance
		? TunaGameInstance->GetSubsystem<UTunaSweeperQuestSubsystem>()
		: nullptr;
	if (!QuestSubsystem || QuestId.IsNone() || ObjectiveId.IsNone() ||
		QuestSubsystem->GetQuestState(QuestId) != ETunaSweeperQuestState::Accepted)
	{
		return false;
	}

	TArray<FTunaSweeperObjectiveProgressView> ObjectiveProgress;
	if (!QuestSubsystem->GetQuestObjectiveProgress(QuestId, ObjectiveProgress))
	{
		return false;
	}

	const FTunaSweeperObjectiveProgressView* MatchingObjective = ObjectiveProgress.FindByPredicate(
		[ObjectiveId](const FTunaSweeperObjectiveProgressView& Candidate)
		{
			return Candidate.ObjectiveId == ObjectiveId;
		});
	return MatchingObjective && !MatchingObjective->bCompleted;
}

FText ATunaSweeperBlockedIntakeScreenActor::ResolveLocalizedText(FName StringKey, const FText& FallbackText) const
{
	const UTunaSweeperGameInstance* TunaGameInstance = GetTunaGameInstance();
	return TunaGameInstance && !StringKey.IsNone()
		? TunaGameInstance->ResolveLocalizedText(StringKey, FallbackText)
		: FallbackText;
}

UTexture2D* ATunaSweeperBlockedIntakeScreenActor::LoadItemIconTexture(int32 ItemId) const
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
	if (!ItemDataSubsystem->TryGetItemDefinition(ItemId, ItemDefinition))
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

FName ATunaSweeperBlockedIntakeScreenActor::GetEffectiveValveProgressObjectId() const
{
	return ValveProgressObjectId.IsNone()
		? FName(*(GetFName().ToString() + TEXT(".valve")))
		: ValveProgressObjectId;
}

FTunaSweeperWorldProgressSaveData ATunaSweeperBlockedIntakeScreenActor::GetOrCreateProgressState(
	FName ObjectId,
	FName InfoId) const
{
	UTunaSweeperGameInstance* TunaGameInstance = GetTunaGameInstance();
	if (!TunaGameInstance)
	{
		FTunaSweeperWorldProgressSaveData EmptyState;
		EmptyState.ObjectId = ObjectId;
		EmptyState.InfoId = InfoId;
		return EmptyState;
	}

	return TunaGameInstance->GetOrCreateWorldProgressState(
		ObjectId,
		InfoId,
		0,
		1);
}

UTunaSweeperGameInstance* ATunaSweeperBlockedIntakeScreenActor::GetTunaGameInstance() const
{
	return GetGameInstance<UTunaSweeperGameInstance>();
}
