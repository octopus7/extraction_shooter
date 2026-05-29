#include "Interaction/TunaSweeperInteractableComponent.h"

#include "Components/WidgetComponent.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Subsystem/TunaSweeperInteractionSubsystem.h"
#include "UI/TunaSweeperInteractionMarkerWidget.h"

namespace TunaSweeperInteractionMarkerLayout
{
	constexpr float DrawWidth = 430.0f;
	constexpr float DrawHeight = 80.0f;
	constexpr float MarkerRootWidth = 360.0f;
	constexpr float MarkerBoxWidth = 56.0f;
	constexpr float VisibleRingScale = 1.0f;
	constexpr float HiddenRingScale = 3.0f;

	FVector2D GetDrawSize()
	{
		return FVector2D(DrawWidth, DrawHeight);
	}

	FVector2D GetMarkerCenteredPivot()
	{
		const float MarkerCenterX = ((DrawWidth - MarkerRootWidth) * 0.5f) + (MarkerBoxWidth * 0.5f);
		return FVector2D(MarkerCenterX / DrawWidth, 0.5f);
	}

	FName GetDefaultInteractionTextKey(ETunaSweeperInteractionType InteractionType)
	{
		switch (InteractionType)
		{
		case ETunaSweeperInteractionType::ItemSpawn:
			return FName(TEXT("ui.interaction.item_spawn"));
		case ETunaSweeperInteractionType::LootContainerSpawn:
			return FName(TEXT("ui.interaction.container_spawn"));
		case ETunaSweeperInteractionType::LevelTravel:
			return FName(TEXT("ui.interaction.travel"));
		case ETunaSweeperInteractionType::Quest:
			return FName(TEXT("ui.quest.interaction_name"));
		case ETunaSweeperInteractionType::SelfDestruct:
			return FName(TEXT("ui.interaction.self_destruct"));
		case ETunaSweeperInteractionType::WorldProgress:
			return FName(TEXT("ui.interaction.repair"));
		case ETunaSweeperInteractionType::PersistentDoor:
		case ETunaSweeperInteractionType::DoorOpen:
			return FName(TEXT("ui.interaction.open"));
		case ETunaSweeperInteractionType::WarpPoint:
			return FName(TEXT("ui.interaction.default"));
		case ETunaSweeperInteractionType::Memo:
			return FName(TEXT("ui.interaction.memo"));
		case ETunaSweeperInteractionType::HousingManagement:
			return FName(TEXT("ui.interaction.housing_management"));
		case ETunaSweeperInteractionType::StorageOpen:
			return FName(TEXT("ui.interaction.storage_open"));
		case ETunaSweeperInteractionType::ShopOpen:
			return FName(TEXT("ui.interaction.shop_open"));
		default:
			return NAME_None;
		}
	}
}

UTunaSweeperInteractableComponent::UTunaSweeperInteractableComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;

	MarkerWidgetClass = TSoftClassPtr<UTunaSweeperInteractionMarkerWidget>(
		FSoftObjectPath(TEXT("/Game/UI/WBP_InteractionMarker.WBP_InteractionMarker_C")));
}

void UTunaSweeperInteractableComponent::OnRegister()
{
	Super::OnRegister();

	EnsureMarkerWidgetComponent();
	ApplyMarkerWidgetLayout();
	EnsureMarkerWidgetClass();
	ApplyMarkerState();
}

void UTunaSweeperInteractableComponent::OnUnregister()
{
	UnregisterFromInteractionSubsystem();

	if (MarkerWidgetComponent)
	{
		MarkerWidgetComponent->DestroyComponent();
		MarkerWidgetComponent = nullptr;
	}

	Super::OnUnregister();
}

void UTunaSweeperInteractableComponent::BeginPlay()
{
	Super::BeginPlay();

	EnsureMarkerWidgetComponent();
	ApplyMarkerWidgetLayout();
	EnsureMarkerWidgetClass();
	ApplyMarkerState();
	RegisterWithInteractionSubsystem();
}

void UTunaSweeperInteractableComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnregisterFromInteractionSubsystem();

	Super::EndPlay(EndPlayReason);
}

void UTunaSweeperInteractableComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateMarker(DeltaTime);
}

FVector UTunaSweeperInteractableComponent::GetInteractionLocation() const
{
	return GetComponentLocation();
}

bool UTunaSweeperInteractableComponent::IsWithinInteractionDistance(const AActor* OtherActor) const
{
	return InteractionType != ETunaSweeperInteractionType::None &&
		OtherActor &&
		GetSquaredDistance2DTo(OtherActor) <= FMath::Square(InteractionDistance);
}

float UTunaSweeperInteractableComponent::GetSquaredDistance2DTo(const AActor* OtherActor) const
{
	return OtherActor
		? FVector::DistSquared2D(GetInteractionLocation(), OtherActor->GetActorLocation())
		: TNumericLimits<float>::Max();
}

bool UTunaSweeperInteractableComponent::RequestInteraction(APawn* InstigatorPawn)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	if (UTunaSweeperInteractionSubsystem* InteractionSubsystem = World->GetSubsystem<UTunaSweeperInteractionSubsystem>())
	{
		return InteractionSubsystem->RequestInteraction(this, InstigatorPawn);
	}

	return false;
}

FText UTunaSweeperInteractableComponent::GetInteractionDisplayName() const
{
	return ResolveInteractionDisplayName();
}

void UTunaSweeperInteractableComponent::ConfigureInteractionDefaults(
	ETunaSweeperInteractionType InInteractionType,
	const FText& InInteractionDisplayName,
	TSoftClassPtr<UTunaSweeperInteractionMarkerWidget> InMarkerWidgetClass,
	FName InInteractionDisplayNameStringKey)
{
	Modify();
	InteractionType = InInteractionType;
	InteractionDisplayName = InInteractionDisplayName;
	InteractionDisplayNameStringKey = InInteractionDisplayNameStringKey;
	MarkerWidgetClass = InMarkerWidgetClass;
	ApplyMarkerState();
}

void UTunaSweeperInteractableComponent::SetInteractionTypeAndDisplayName(
	ETunaSweeperInteractionType InInteractionType,
	const FText& InInteractionDisplayName)
{
	Modify();
	InteractionType = InInteractionType;
	InteractionDisplayName = InInteractionDisplayName;
	InteractionDisplayNameStringKey = NAME_None;
	ApplyMarkerState();
}

void UTunaSweeperInteractableComponent::SetInteractionTypeDisplayNameAndStringKey(
	ETunaSweeperInteractionType InInteractionType,
	const FText& InInteractionDisplayName,
	FName InInteractionDisplayNameStringKey)
{
	Modify();
	InteractionType = InInteractionType;
	InteractionDisplayName = InInteractionDisplayName;
	InteractionDisplayNameStringKey = InInteractionDisplayNameStringKey;
	ApplyMarkerState();
}

void UTunaSweeperInteractableComponent::SetInteractionDisplayNameStringKey(FName InInteractionDisplayNameStringKey)
{
	Modify();
	InteractionDisplayNameStringKey = InInteractionDisplayNameStringKey;
	ApplyMarkerState();
}

void UTunaSweeperInteractableComponent::SetInteractionRequirementPreview(
	UTexture2D* InIconTexture,
	int32 InRequiredQuantity,
	bool bInShowRequirement)
{
	RequirementIconTexture = InIconTexture;
	RequirementQuantity = FMath::Max(0, InRequiredQuantity);
	bShowRequirementPreview = bInShowRequirement;
	ApplyMarkerState();
}

void UTunaSweeperInteractableComponent::SetMarkerCompleted(bool bInMarkerCompleted)
{
	bMarkerCompleted = bInMarkerCompleted;
	ApplyMarkerState();
}

void UTunaSweeperInteractableComponent::RegisterWithInteractionSubsystem()
{
	if (bRegisteredWithInteractionSubsystem)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World || !World->IsGameWorld())
	{
		return;
	}

	if (UTunaSweeperInteractionSubsystem* InteractionSubsystem = World->GetSubsystem<UTunaSweeperInteractionSubsystem>())
	{
		InteractionSubsystem->RegisterInteractable(this);
		bRegisteredWithInteractionSubsystem = true;
	}
}

void UTunaSweeperInteractableComponent::UnregisterFromInteractionSubsystem()
{
	if (!bRegisteredWithInteractionSubsystem)
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		if (UTunaSweeperInteractionSubsystem* InteractionSubsystem = World->GetSubsystem<UTunaSweeperInteractionSubsystem>())
		{
			InteractionSubsystem->UnregisterInteractable(this);
		}
	}

	bRegisteredWithInteractionSubsystem = false;
}

void UTunaSweeperInteractableComponent::EnsureMarkerWidgetComponent()
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	if (!MarkerWidgetComponent)
	{
		const FName ComponentName = MakeUniqueObjectName(Owner, UWidgetComponent::StaticClass(), TEXT("InteractionMarkerWidget"));
		MarkerWidgetComponent = NewObject<UWidgetComponent>(Owner, ComponentName, RF_Transient);
		MarkerWidgetComponent->CreationMethod = EComponentCreationMethod::Instance;
		MarkerWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
		MarkerWidgetComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		MarkerWidgetComponent->SetVisibility(false);
		MarkerWidgetComponent->SetHiddenInGame(false);
		MarkerWidgetComponent->SetupAttachment(this);
	}

	if (MarkerWidgetComponent->GetAttachParent() != this)
	{
		MarkerWidgetComponent->AttachToComponent(this, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	}

	if (!MarkerWidgetComponent->IsRegistered() && IsRegistered())
	{
		MarkerWidgetComponent->RegisterComponent();
	}
}

void UTunaSweeperInteractableComponent::ApplyMarkerWidgetLayout()
{
	if (!MarkerWidgetComponent)
	{
		return;
	}

	MarkerWidgetComponent->SetRelativeLocation(FVector::ZeroVector);
	MarkerWidgetComponent->SetDrawSize(TunaSweeperInteractionMarkerLayout::GetDrawSize());
	MarkerWidgetComponent->SetPivot(TunaSweeperInteractionMarkerLayout::GetMarkerCenteredPivot());
}

void UTunaSweeperInteractableComponent::EnsureMarkerWidgetClass()
{
	if (!MarkerWidgetComponent)
	{
		return;
	}

	if (TSubclassOf<UTunaSweeperInteractionMarkerWidget> LoadedClass = MarkerWidgetClass.LoadSynchronous())
	{
		if (MarkerWidgetComponent->GetWidgetClass() != LoadedClass)
		{
			MarkerWidgetComponent->SetWidgetClass(LoadedClass);
		}
	}
}

void UTunaSweeperInteractableComponent::UpdateMarker(float DeltaSeconds)
{
	const APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	bool bCanInteract = InteractionType != ETunaSweeperInteractionType::None;
	if (UWorld* World = GetWorld())
	{
		if (UTunaSweeperInteractionSubsystem* InteractionSubsystem = World->GetSubsystem<UTunaSweeperInteractionSubsystem>())
		{
			bCanInteract = InteractionSubsystem->CanOfferInteraction(this);
		}
	}

	const bool bInsideVisibleDistance =
		bCanInteract &&
		PlayerPawn &&
		FVector::DistSquared2D(PlayerPawn->GetActorLocation(), GetInteractionLocation()) <= FMath::Square(MarkerVisibleDistance);

	const float TargetScale = bInsideVisibleDistance
		? TunaSweeperInteractionMarkerLayout::VisibleRingScale
		: TunaSweeperInteractionMarkerLayout::HiddenRingScale;
	bool bIsFocusedInteractable = false;

	if (UWorld* World = GetWorld())
	{
		if (UTunaSweeperInteractionSubsystem* InteractionSubsystem = World->GetSubsystem<UTunaSweeperInteractionSubsystem>())
		{
			bIsFocusedInteractable =
				bCanInteract &&
				InteractionSubsystem->GetFocusedInteractable() == this &&
				PlayerPawn &&
				IsWithinInteractionDistance(PlayerPawn);
		}
	}

	const float TargetLabelAlpha = bIsFocusedInteractable ? 1.0f : 0.0f;

	MarkerRingScale = FMath::FInterpTo(MarkerRingScale, TargetScale, DeltaSeconds, MarkerScaleInterpSpeed);
	MarkerAlpha = FMath::Clamp(
		(TunaSweeperInteractionMarkerLayout::HiddenRingScale - MarkerRingScale) /
			(TunaSweeperInteractionMarkerLayout::HiddenRingScale - TunaSweeperInteractionMarkerLayout::VisibleRingScale),
		0.0f,
		1.0f);
	LabelAlpha = FMath::FInterpTo(LabelAlpha, TargetLabelAlpha, DeltaSeconds, LabelFadeInterpSpeed);

	ApplyMarkerState();
}

void UTunaSweeperInteractableComponent::ApplyMarkerState()
{
	if (!MarkerWidgetComponent)
	{
		return;
	}

	const bool bMarkerVisible = MarkerAlpha > 0.01f;
	MarkerWidgetComponent->SetVisibility(bMarkerVisible);

	if (UTunaSweeperInteractionMarkerWidget* MarkerWidget = Cast<UTunaSweeperInteractionMarkerWidget>(MarkerWidgetComponent->GetUserWidgetObject()))
	{
		MarkerWidget->SetMarkerText(ResolveInteractionDisplayName());
		MarkerWidget->SetRequirementPreview(RequirementIconTexture, RequirementQuantity, bShowRequirementPreview);
		MarkerWidget->SetMarkerOpened(bMarkerCompleted);
		MarkerWidget->SetMarkerPresentation(MarkerAlpha, MarkerRingScale, LabelAlpha);
	}
}

FText UTunaSweeperInteractableComponent::ResolveInteractionDisplayName() const
{
	const FName TextKey = InteractionDisplayNameStringKey.IsNone()
		? TunaSweeperInteractionMarkerLayout::GetDefaultInteractionTextKey(InteractionType)
		: InteractionDisplayNameStringKey;
	if (TextKey.IsNone())
	{
		return InteractionDisplayName;
	}

	const UWorld* World = GetWorld();
	const UTunaSweeperGameInstance* TunaGameInstance = World
		? Cast<UTunaSweeperGameInstance>(World->GetGameInstance())
		: nullptr;
	return TunaGameInstance
		? TunaGameInstance->ResolveLocalizedText(TextKey, InteractionDisplayName)
		: InteractionDisplayName;
}
