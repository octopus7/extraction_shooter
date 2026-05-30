#include "Interaction/TunaSweeperLootContainerActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Interaction/TunaSweeperInteractableComponent.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

ATunaSweeperLootContainerActor::ATunaSweeperLootContainerActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
	VisualMesh->SetupAttachment(RootComponent);
	VisualMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 25.0f));
	VisualMesh->SetRelativeScale3D(FVector(1.1f, 0.8f, 0.5f));
	VisualMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	LidPivot = CreateDefaultSubobject<USceneComponent>(TEXT("LidPivot"));
	LidPivot->SetupAttachment(RootComponent);
	LidPivot->SetRelativeLocation(FVector(0.0f, -40.0f, 55.0f));

	LidMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LidMesh"));
	LidMesh->SetupAttachment(LidPivot);
	LidMesh->SetRelativeLocation(FVector(0.0f, 40.0f, 0.0f));
	LidMesh->SetRelativeScale3D(FVector(1.1f, 0.8f, 0.08f));
	LidMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		VisualMesh->SetStaticMesh(CubeMesh.Object);
		LidMesh->SetStaticMesh(CubeMesh.Object);
	}

	InteractableComponent = CreateDefaultSubobject<UTunaSweeperInteractableComponent>(TEXT("Interactable"));
	InteractableComponent->SetupAttachment(RootComponent);
	InteractableComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 90.0f));
	InteractableComponent->SetInteractionTypeDisplayNameAndStringKey(
		ETunaSweeperInteractionType::LootContainerOpen,
		FText::FromString(TEXT("\uC5F4\uAE30")),
		FName(TEXT("ui.interaction.open")));
}

void ATunaSweeperLootContainerActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RefreshContainerPresentation();
	ApplyLidRotation(bLidOpen ? OpenLidRelativeRotation : ClosedLidRelativeRotation);
}

void ATunaSweeperLootContainerActor::BeginPlay()
{
	Super::BeginPlay();
	RefreshContainerPresentation();
	ApplyLidRotation(bLidOpen ? OpenLidRelativeRotation : ClosedLidRelativeRotation);
	if (UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>())
	{
		TunaGameInstance->OnLanguageChanged.RemoveAll(this);
		TunaGameInstance->OnLanguageChanged.AddUObject(this, &ATunaSweeperLootContainerActor::HandleLanguageChanged);
	}
	SetActorTickEnabled(false);
}

void ATunaSweeperLootContainerActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bAnimatingLid)
	{
		SetActorTickEnabled(false);
		return;
	}

	LidAnimationElapsed += FMath::Max(0.0f, DeltaSeconds);
	const float RawAlpha = LidAnimationDuration > KINDA_SMALL_NUMBER
		? FMath::Clamp(LidAnimationElapsed / LidAnimationDuration, 0.0f, 1.0f)
		: 1.0f;
	const float EasedAlpha = EvaluateLidAnimationAlpha(RawAlpha, ActiveLidEasing);
	ApplyLidRotation(FQuat::Slerp(
		LidAnimationStartRotation.Quaternion(),
		LidAnimationTargetRotation.Quaternion(),
		EasedAlpha).Rotator());

	if (RawAlpha >= 1.0f)
	{
		bAnimatingLid = false;
		bLidOpen = bLidAnimationTargetOpen;
		ApplyLidRotation(LidAnimationTargetRotation);
		SetActorTickEnabled(false);
	}
}

void ATunaSweeperLootContainerActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (RuntimeGameInstance.IsValid())
	{
		RuntimeGameInstance->OnInventoryStateChanged.RemoveAll(this);
		RuntimeGameInstance->OnActiveLootContainerUiClosed.RemoveAll(this);
		RuntimeGameInstance->OnLanguageChanged.RemoveAll(this);
	}
	if (UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>())
	{
		TunaGameInstance->OnLanguageChanged.RemoveAll(this);
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

	const UTunaSweeperGameInstance* TunaGameInstance = Cast<UTunaSweeperGameInstance>(GetGameInstance());
	const ETunaSweeperItemTextLanguage Language = TunaGameInstance
		? TunaGameInstance->GetCurrentTextLanguage()
		: DisplayLanguage;

	return ItemDataSubsystem->TryBuildLootContainerInstance(
		ContainerDefinitionId,
		ContentsId,
		Language,
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
			RuntimeGameInstance->OnActiveLootContainerUiClosed.RemoveAll(this);
			RuntimeGameInstance->OnLanguageChanged.RemoveAll(this);
		}

		RuntimeGameInstance = TunaGameInstance;
		TunaGameInstance->OnInventoryStateChanged.RemoveAll(this);
		TunaGameInstance->OnInventoryStateChanged.AddUObject(this, &ATunaSweeperLootContainerActor::CaptureRuntimeContentsFromActiveContainer);
		TunaGameInstance->OnActiveLootContainerUiClosed.RemoveAll(this);
		TunaGameInstance->OnActiveLootContainerUiClosed.AddUObject(this, &ATunaSweeperLootContainerActor::HandleActiveLootContainerUiClosed);
		TunaGameInstance->OnLanguageChanged.RemoveAll(this);
		TunaGameInstance->OnLanguageChanged.AddUObject(this, &ATunaSweeperLootContainerActor::HandleLanguageChanged);
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
		ApplyOpenedMarkerState();
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
	ApplyOpenedMarkerState();
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

void ATunaSweeperLootContainerActor::PlayOpenAnimation()
{
	SetLidOpen(true, false);
}

void ATunaSweeperLootContainerActor::PlayCloseAnimation()
{
	SetLidOpen(false, false);
}

void ATunaSweeperLootContainerActor::SetLidOpen(bool bInOpen, bool bInstant)
{
	if (bInstant)
	{
		bAnimatingLid = false;
		bLidOpen = bInOpen;
		ApplyLidRotation(bLidOpen ? OpenLidRelativeRotation : ClosedLidRelativeRotation);
		SetActorTickEnabled(false);
		return;
	}

	StartLidAnimation(bInOpen);
}

void ATunaSweeperLootContainerActor::RefreshContainerPresentation()
{
	if (InteractableComponent)
	{
		InteractableComponent->SetInteractionTypeDisplayNameAndStringKey(
			ETunaSweeperInteractionType::LootContainerOpen,
			FText::FromString(TEXT("\uC5F4\uAE30")),
			FName(TEXT("ui.interaction.open")));
		ApplyOpenedMarkerState();
	}

	if (!VisualMesh)
	{
		return;
	}

	if (BodyMeshOverride)
	{
		VisualMesh->SetStaticMesh(BodyMeshOverride);
	}

	if (LidMesh && LidMeshOverride)
	{
		LidMesh->SetStaticMesh(LidMeshOverride);
	}

	UTunaSweeperItemDataSubsystem* ItemDataSubsystem = GetItemDataSubsystem();
	if (!ItemDataSubsystem)
	{
		ApplyLidRotation(bLidOpen ? OpenLidRelativeRotation : ClosedLidRelativeRotation);
		return;
	}

	FTunaSweeperLootContainerDefinition Definition;
	const bool bHasDefinition = ItemDataSubsystem->TryGetLootContainerDefinition(ContainerDefinitionId, Definition);
	if (!bHasDefinition)
	{
		ApplyLidRotation(bLidOpen ? OpenLidRelativeRotation : ClosedLidRelativeRotation);
		return;
	}

	if (!BodyMeshOverride)
	{
		if (UStaticMesh* LoadedMesh = Cast<UStaticMesh>(FSoftObjectPath(Definition.StaticMeshPath).TryLoad()))
		{
			VisualMesh->SetStaticMesh(LoadedMesh);
		}
	}

	if (!Definition.MaterialPath.IsEmpty())
	{
		if (UMaterialInterface* LoadedMaterial = Cast<UMaterialInterface>(FSoftObjectPath(Definition.MaterialPath).TryLoad()))
		{
			VisualMesh->SetMaterial(0, LoadedMaterial);
			if (LidMesh)
			{
				LidMesh->SetMaterial(0, LoadedMaterial);
			}
		}
	}

	VisualMesh->SetRelativeScale3D(Definition.MeshScale);
	ApplyLidRotation(bLidOpen ? OpenLidRelativeRotation : ClosedLidRelativeRotation);
}

void ATunaSweeperLootContainerActor::HandleLanguageChanged()
{
	if (bHasRuntimeContainerState)
	{
		FTunaSweeperLootContainerInstance UpdatedInstance;
		if (BuildContainerInstance(UpdatedInstance))
		{
			RuntimeDisplayName = UpdatedInstance.DisplayName;
			RuntimeCapacity = FMath::Max(0, UpdatedInstance.Capacity);
			if (UTunaSweeperGameInstance* TunaGameInstance = RuntimeGameInstance.Get())
			{
				if (TunaGameInstance->HasActiveLootContainer() &&
					TunaGameInstance->GetActiveLootContainerOwner() == this)
				{
					TunaGameInstance->SetActiveLootContainerRuntimeSlots(
						BuildRuntimeContainerInstance(),
						RuntimeSlots,
						this);
				}
			}
		}
	}

	RefreshContainerPresentation();
}

void ATunaSweeperLootContainerActor::ResetRuntimeContainerState()
{
	RuntimeSlots.Reset();
	RuntimeDisplayName = FText::GetEmpty();
	RuntimeCapacity = 0;
	RuntimeContainerDefinitionId = INDEX_NONE;
	RuntimeContentsId = INDEX_NONE;
	bHasRuntimeContainerState = false;
	ApplyOpenedMarkerState();
}

void ATunaSweeperLootContainerActor::ApplyOpenedMarkerState()
{
	if (InteractableComponent)
	{
		InteractableComponent->SetMarkerCompleted(bHasRuntimeContainerState);
	}
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
	ApplyOpenedMarkerState();
}

void ATunaSweeperLootContainerActor::HandleActiveLootContainerUiClosed()
{
	CaptureRuntimeContentsFromActiveContainer();

	const UTunaSweeperGameInstance* TunaGameInstance = RuntimeGameInstance.Get();
	if (!TunaGameInstance || TunaGameInstance->GetActiveLootContainerOwner() != this)
	{
		return;
	}

	PlayCloseAnimation();
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

void ATunaSweeperLootContainerActor::StartLidAnimation(bool bOpenTarget)
{
	const float Duration = bOpenTarget || !bUseSeparateCloseTiming
		? OpenAnimationDuration
		: CloseAnimationDuration;
	const ETunaSweeperLootContainerLidEasing Easing = bOpenTarget || !bUseSeparateCloseTiming
		? OpenEasing
		: CloseEasing;

	bLidAnimationTargetOpen = bOpenTarget;
	LidAnimationStartRotation = LidPivot ? LidPivot->GetRelativeRotation() : ClosedLidRelativeRotation;
	LidAnimationTargetRotation = bOpenTarget ? OpenLidRelativeRotation : ClosedLidRelativeRotation;
	LidAnimationDuration = FMath::Max(0.0f, Duration);
	LidAnimationElapsed = 0.0f;
	ActiveLidEasing = Easing;

	if (LidAnimationDuration <= KINDA_SMALL_NUMBER)
	{
		bAnimatingLid = false;
		bLidOpen = bOpenTarget;
		ApplyLidRotation(LidAnimationTargetRotation);
		SetActorTickEnabled(false);
		return;
	}

	bAnimatingLid = true;
	SetActorTickEnabled(true);
}

void ATunaSweeperLootContainerActor::ApplyLidRotation(const FRotator& Rotation)
{
	if (LidPivot)
	{
		LidPivot->SetRelativeRotation(Rotation);
	}
}

float ATunaSweeperLootContainerActor::EvaluateLidAnimationAlpha(
	float Alpha,
	ETunaSweeperLootContainerLidEasing Easing) const
{
	const float ClampedAlpha = FMath::Clamp(Alpha, 0.0f, 1.0f);
	switch (Easing)
	{
	case ETunaSweeperLootContainerLidEasing::EaseIn:
		return FMath::InterpEaseIn(0.0f, 1.0f, ClampedAlpha, 2.0f);
	case ETunaSweeperLootContainerLidEasing::EaseOut:
		return FMath::InterpEaseOut(0.0f, 1.0f, ClampedAlpha, 2.0f);
	case ETunaSweeperLootContainerLidEasing::EaseInOut:
		return FMath::InterpEaseInOut(0.0f, 1.0f, ClampedAlpha, 2.0f);
	case ETunaSweeperLootContainerLidEasing::Linear:
	default:
		return ClampedAlpha;
	}
}
