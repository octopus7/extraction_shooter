#include "Interaction/TunaSweeperWorldProgressActor.h"

#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Interaction/TunaSweeperInteractableComponent.h"
#include "Interaction/TunaSweeperTransparentObstacleActor.h"
#include "Materials/MaterialInterface.h"
#include "Subsystem/TunaSweeperItemDataSubsystem.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	const TCHAR* BrokenBridgeVoxelMeshPath = TEXT("/Game/Interaction/SM_Bridge_Broken_Voxel.SM_Bridge_Broken_Voxel");
	const TCHAR* BrokenBridgeVoxelMaterialPath = TEXT("/Game/Prototype/M_Voxel_VertexColor.M_Voxel_VertexColor");
}

ATunaSweeperWorldProgressActor::ATunaSweeperWorldProgressActor()
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
	VisualMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 30.0f));
	VisualMesh->SetRelativeScale3D(FVector(2.2f, 0.5f, 0.22f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		VisualMesh->SetStaticMesh(CubeMesh.Object);
	}
	ApplyBridgeVisualMesh();

	InteractableComponent = CreateDefaultSubobject<UTunaSweeperInteractableComponent>(TEXT("Interactable"));
	InteractableComponent->SetupAttachment(RootComponent);
	InteractableComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 145.0f));

	ApplyCollisionDefaults();
	RefreshPresentation();
}

void ATunaSweeperWorldProgressActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyBridgeVisualMesh();
	ApplyCollisionDefaults();
	RefreshPresentation();
}

void ATunaSweeperWorldProgressActor::BeginPlay()
{
	Super::BeginPlay();

	if (UTunaSweeperGameInstance* TunaGameInstance = GetTunaGameInstance())
	{
		TunaGameInstance->OnInventoryStateChanged.RemoveAll(this);
		TunaGameInstance->OnInventoryStateChanged.AddUObject(this, &ATunaSweeperWorldProgressActor::RefreshPresentation);
	}

	ApplySavedState();
	ApplyBridgeVisualMesh();
	RefreshPresentation();
}

void ATunaSweeperWorldProgressActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UTunaSweeperGameInstance* TunaGameInstance = GetTunaGameInstance())
	{
		TunaGameInstance->OnInventoryStateChanged.RemoveAll(this);
	}

	Super::EndPlay(EndPlayReason);
}

void ATunaSweeperWorldProgressActor::ConfigureWorldProgressDefaults(
	FName InProgressObjectId,
	FName InProgressInfoId,
	const FText& InDisplayName,
	const FText& InInteractionDisplayName,
	int32 InRequiredItemId,
	int32 InRequiredQuantity,
	int32 InInitialProgressQuantity,
	const FText& InRequiredItemDisplayName,
	const FVector& InBlockingBoxExtent,
	TSoftClassPtr<AActor> InCompletedReplacementActorClass)
{
	Modify();
	ProgressObjectId = InProgressObjectId;
	ProgressInfoId = InProgressInfoId;
	DisplayName = InDisplayName.IsEmpty()
		? FText::FromString(TEXT("\uBD80\uC11C\uC9C4 \uB2E4\uB9AC"))
		: InDisplayName;
	InteractionDisplayName = InInteractionDisplayName.IsEmpty()
		? FText::FromString(TEXT("\uC218\uB9AC\uD558\uAE30"))
		: InInteractionDisplayName;
	RequiredItemId = InRequiredItemId == INDEX_NONE ? 6002 : InRequiredItemId;
	RequiredQuantity = FMath::Max(1, InRequiredQuantity);
	InitialProgressQuantity = FMath::Clamp(InInitialProgressQuantity, 0, RequiredQuantity);
	RequiredItemDisplayName = InRequiredItemDisplayName.IsEmpty()
		? FText::FromString(TEXT("\uBAA9\uC7AC"))
		: InRequiredItemDisplayName;
	BlockingBoxExtent = FVector(
		FMath::Max(1.0f, InBlockingBoxExtent.X),
		FMath::Max(1.0f, InBlockingBoxExtent.Y),
		FMath::Max(1.0f, InBlockingBoxExtent.Z));
	CompletedReplacementActorClass = InCompletedReplacementActorClass;
	ApplyCollisionDefaults();
	RefreshPresentation();
}

void ATunaSweeperWorldProgressActor::ApplyBridgeVisualMesh()
{
	if (!VisualMesh)
	{
		return;
	}

	if (UStaticMesh* BridgeMesh = LoadObject<UStaticMesh>(nullptr, BrokenBridgeVoxelMeshPath))
	{
		VisualMesh->SetStaticMesh(BridgeMesh);
		VisualMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 40.0f));
		VisualMesh->SetRelativeScale3D(FVector::OneVector);
	}

	if (UMaterialInterface* VoxelMaterial = LoadObject<UMaterialInterface>(nullptr, BrokenBridgeVoxelMaterialPath))
	{
		VisualMesh->SetMaterial(0, VoxelMaterial);
	}
}

int32 ATunaSweeperWorldProgressActor::GetProgressQuantity() const
{
	return FMath::Clamp(GetOrCreateProgressState().ProgressQuantity, 0, FMath::Max(0, RequiredQuantity));
}

int32 ATunaSweeperWorldProgressActor::GetRemainingRequiredQuantity() const
{
	return FMath::Max(0, FMath::Max(1, RequiredQuantity) - GetProgressQuantity());
}

int32 ATunaSweeperWorldProgressActor::GetOwnedRequiredItemCount() const
{
	UTunaSweeperGameInstance* TunaGameInstance = GetTunaGameInstance();
	return TunaGameInstance ? TunaGameInstance->CountInventoryItemById(RequiredItemId) : 0;
}

bool ATunaSweeperWorldProgressActor::IsRepairReady() const
{
	return !bCompleted && GetProgressQuantity() >= FMath::Max(1, RequiredQuantity);
}

int32 ATunaSweeperWorldProgressActor::UseAvailableRequiredItems(bool bSaveImmediately)
{
	if (bCompleted)
	{
		return 0;
	}

	UTunaSweeperGameInstance* TunaGameInstance = GetTunaGameInstance();
	if (!TunaGameInstance)
	{
		return 0;
	}

	const int32 CurrentProgressQuantity = GetProgressQuantity();
	const int32 RemainingQuantity = GetRemainingRequiredQuantity();
	if (RemainingQuantity <= 0)
	{
		RefreshPresentation();
		return 0;
	}

	const int32 ConsumedQuantity = TunaGameInstance->ConsumeInventoryItemById(RequiredItemId, RemainingQuantity);
	if (ConsumedQuantity <= 0)
	{
		RefreshPresentation();
		return 0;
	}

	TunaGameInstance->UpdateWorldProgressState(
		GetEffectiveProgressObjectId(),
		ProgressInfoId,
		ETunaSweeperWorldProgressState::InProgress,
		CurrentProgressQuantity + ConsumedQuantity,
		RequiredQuantity,
		bSaveImmediately);
	RefreshPresentation();
	return ConsumedQuantity;
}

bool ATunaSweeperWorldProgressActor::Repair(bool bSaveImmediately)
{
	if (!IsRepairReady())
	{
		RefreshPresentation();
		return false;
	}

	CompleteAndReplace(bSaveImmediately);
	return true;
}

bool ATunaSweeperWorldProgressActor::RepairUsingAvailableRequiredItems(bool bSaveImmediately)
{
	if (bCompleted)
	{
		return false;
	}

	const int32 RemainingQuantity = GetRemainingRequiredQuantity();
	if (RemainingQuantity > 0)
	{
		UTunaSweeperGameInstance* TunaGameInstance = GetTunaGameInstance();
		if (!TunaGameInstance || TunaGameInstance->CountInventoryItemById(RequiredItemId) < RemainingQuantity)
		{
			RefreshPresentation();
			return false;
		}

		const int32 ConsumedQuantity = TunaGameInstance->ConsumeInventoryItemById(RequiredItemId, RemainingQuantity);
		if (ConsumedQuantity != RemainingQuantity)
		{
			RefreshPresentation();
			return false;
		}
	}

	CompleteAndReplace(bSaveImmediately);
	return true;
}

void ATunaSweeperWorldProgressActor::ApplyCollisionDefaults()
{
	if (!BlockingCollision)
	{
		return;
	}

	BlockingCollision->SetBoxExtent(BlockingBoxExtent);
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

void ATunaSweeperWorldProgressActor::RefreshPresentation()
{
	if (InteractableComponent)
	{
		InteractableComponent->SetInteractionTypeAndDisplayName(
			bCompleted ? ETunaSweeperInteractionType::None : ETunaSweeperInteractionType::WorldProgress,
			bCompleted ? FText::GetEmpty() : InteractionDisplayName);
		InteractableComponent->SetInteractionRequirementPreview(
			LoadRequiredItemIconTexture(),
			GetRemainingRequiredQuantity(),
			!bCompleted && GetRemainingRequiredQuantity() > 0);
	}

	if (BlockingCollision)
	{
		BlockingCollision->SetCollisionEnabled(bCompleted ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryAndPhysics);
	}

	SetActorHiddenInGame(bCompleted);
}

void ATunaSweeperWorldProgressActor::ApplySavedState()
{
	const FTunaSweeperWorldProgressSaveData ProgressState = GetOrCreateProgressState();
	if (ProgressState.State == ETunaSweeperWorldProgressState::Completed)
	{
		CompleteAndReplace(false);
	}
}

void ATunaSweeperWorldProgressActor::CompleteAndReplace(bool bSaveImmediately)
{
	if (bCompleted)
	{
		return;
	}

	if (UTunaSweeperGameInstance* TunaGameInstance = GetTunaGameInstance())
	{
		TunaGameInstance->UpdateWorldProgressState(
			GetEffectiveProgressObjectId(),
			ProgressInfoId,
			ETunaSweeperWorldProgressState::Completed,
			RequiredQuantity,
			RequiredQuantity,
			bSaveImmediately);
	}

	bCompleted = true;
	RefreshPresentation();

	UWorld* World = GetWorld();
	if (World)
	{
		TSubclassOf<AActor> ReplacementClass = CompletedReplacementActorClass.LoadSynchronous();
		if (!ReplacementClass)
		{
			ReplacementClass = ATunaSweeperWorldProgressCompletedActor::StaticClass();
		}

		FActorSpawnParameters SpawnParameters;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		World->SpawnActor<AActor>(
			ReplacementClass,
			GetActorLocation(),
			GetActorRotation(),
			SpawnParameters);
	}

	Destroy();
}

UTexture2D* ATunaSweeperWorldProgressActor::LoadRequiredItemIconTexture() const
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

FName ATunaSweeperWorldProgressActor::GetEffectiveProgressObjectId() const
{
	return ProgressObjectId.IsNone() ? GetFName() : ProgressObjectId;
}

FTunaSweeperWorldProgressSaveData ATunaSweeperWorldProgressActor::GetOrCreateProgressState() const
{
	UTunaSweeperGameInstance* TunaGameInstance = GetTunaGameInstance();
	if (!TunaGameInstance)
	{
		FTunaSweeperWorldProgressSaveData EmptyState;
		EmptyState.ObjectId = GetEffectiveProgressObjectId();
		EmptyState.InfoId = ProgressInfoId;
		EmptyState.ProgressQuantity = InitialProgressQuantity;
		return EmptyState;
	}

	return TunaGameInstance->GetOrCreateWorldProgressState(
		GetEffectiveProgressObjectId(),
		ProgressInfoId,
		InitialProgressQuantity,
		RequiredQuantity);
}

UTunaSweeperGameInstance* ATunaSweeperWorldProgressActor::GetTunaGameInstance() const
{
	return GetGameInstance<UTunaSweeperGameInstance>();
}
