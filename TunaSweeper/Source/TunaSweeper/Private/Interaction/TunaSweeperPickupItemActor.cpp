#include "Interaction/TunaSweeperPickupItemActor.h"

#include "Components/SceneComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "Interaction/TunaSweeperInteractableComponent.h"
#include "Misc/Paths.h"
#include "Subsystem/TunaSweeperItemDataSubsystem.h"
#include "UI/TunaSweeperPickupItemIconWidget.h"

ATunaSweeperPickupItemActor::ATunaSweeperPickupItemActor()
{
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	FloorIconWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("FloorIconWidget"));
	FloorIconWidgetComponent->SetupAttachment(RootComponent);
	FloorIconWidgetComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 2.0f));
	FloorIconWidgetComponent->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f));
	FloorIconWidgetComponent->SetWidgetSpace(EWidgetSpace::World);
	FloorIconWidgetComponent->SetDrawSize(FVector2D(96.0f, 96.0f));
	FloorIconWidgetComponent->SetPivot(FVector2D(0.5f, 0.5f));
	FloorIconWidgetComponent->SetTwoSided(true);
	FloorIconWidgetComponent->SetCastShadow(false);
	FloorIconWidgetComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ApplyFloorIconWidgetRenderingSettings();

	InteractableComponent = CreateDefaultSubobject<UTunaSweeperInteractableComponent>(TEXT("Interactable"));
	InteractableComponent->SetupAttachment(RootComponent);
	InteractableComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 80.0f));
	InteractableComponent->SetInteractionTypeAndDisplayName(
		ETunaSweeperInteractionType::ItemPickup,
		FText::FromString(TEXT("Item")));

	FloorIconWidgetClass = TSoftClassPtr<UTunaSweeperPickupItemIconWidget>(
		FSoftObjectPath(TEXT("/Game/UI/WBP_PickupItemIcon.WBP_PickupItemIcon_C")));
	CachedItemDisplayName = BuildFallbackItemName();
}

void ATunaSweeperPickupItemActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	ApplyFloorIconWidgetRenderingSettings();
	EnsureFloorIconWidgetClass();
	RefreshItemPresentation();
}

void ATunaSweeperPickupItemActor::BeginPlay()
{
	Super::BeginPlay();

	ApplyFloorIconWidgetRenderingSettings();
	EnsureFloorIconWidgetClass();
	RefreshItemPresentation();
}

void ATunaSweeperPickupItemActor::SetItemId(int32 InItemId)
{
	if (ItemId == InItemId)
	{
		return;
	}

	Modify();
	ItemId = InItemId;
	RefreshItemPresentation();
}

void ATunaSweeperPickupItemActor::ConfigurePickupItemDefaults(
	int32 InItemId,
	TSoftClassPtr<UTunaSweeperPickupItemIconWidget> InFloorIconWidgetClass)
{
	Modify();
	ItemId = InItemId;
	FloorIconWidgetClass = InFloorIconWidgetClass;
	EnsureFloorIconWidgetClass();
	RefreshItemPresentation();
}

void ATunaSweeperPickupItemActor::ApplyFloorIconWidgetRenderingSettings()
{
	if (!FloorIconWidgetComponent)
	{
		return;
	}

	FloorIconWidgetComponent->SetBlendMode(EWidgetBlendMode::Transparent);
	FloorIconWidgetComponent->SetBackgroundColor(FLinearColor::Transparent);
	FloorIconWidgetComponent->SetTintColorAndOpacity(FLinearColor::White);
	FloorIconWidgetComponent->SetOpacityFromTexture(1.0f);
	FloorIconWidgetComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 2.0f));
	FloorIconWidgetComponent->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f));
	FloorIconWidgetComponent->SetDrawSize(FVector2D(96.0f, 96.0f));
	FloorIconWidgetComponent->SetPivot(FVector2D(0.5f, 0.5f));
	FloorIconWidgetComponent->SetTwoSided(true);
	FloorIconWidgetComponent->SetCastShadow(false);
	FloorIconWidgetComponent->SetWindowFocusable(false);
}

void ATunaSweeperPickupItemActor::EnsureFloorIconWidgetClass()
{
	if (!FloorIconWidgetComponent)
	{
		return;
	}

	if (TSubclassOf<UTunaSweeperPickupItemIconWidget> LoadedClass = FloorIconWidgetClass.LoadSynchronous())
	{
		if (FloorIconWidgetComponent->GetWidgetClass() != LoadedClass)
		{
			FloorIconWidgetComponent->SetWidgetClass(LoadedClass);
			FloorIconWidgetComponent->InitWidget();
		}
	}
}

void ATunaSweeperPickupItemActor::RefreshItemPresentation()
{
	CachedItemDisplayName = BuildFallbackItemName();
	UTexture2D* IconTexture = nullptr;

	if (const UGameInstance* GameInstance = GetGameInstance())
	{
		if (UTunaSweeperItemDataSubsystem* ItemDataSubsystem = GameInstance->GetSubsystem<UTunaSweeperItemDataSubsystem>())
		{
			FTunaSweeperItemDefinition ItemDefinition;
			if (ItemDataSubsystem->TryGetItemDefinition(ItemId, ItemDefinition))
			{
				FText LocalizedName;
				if (ItemDataSubsystem->TryGetItemNameTextByKey(ItemDefinition.NameStringKey, DisplayLanguage, LocalizedName))
				{
					CachedItemDisplayName = LocalizedName;
				}

				IconTexture = LoadIconTexture(ItemDefinition);
			}
		}
	}

	if (InteractableComponent)
	{
		InteractableComponent->SetInteractionTypeAndDisplayName(
			ETunaSweeperInteractionType::ItemPickup,
			CachedItemDisplayName);
	}

	if (FloorIconWidgetComponent)
	{
		FloorIconWidgetComponent->InitWidget();
		if (UTunaSweeperPickupItemIconWidget* IconWidget = Cast<UTunaSweeperPickupItemIconWidget>(FloorIconWidgetComponent->GetUserWidgetObject()))
		{
			IconWidget->SetIconTexture(IconTexture);
		}
	}
}

UTexture2D* ATunaSweeperPickupItemActor::LoadIconTexture(const FTunaSweeperItemDefinition& ItemDefinition) const
{
	const FString IconAssetName = FPaths::GetBaseFilename(ItemDefinition.IconFileName);
	if (IconAssetName.IsEmpty())
	{
		return nullptr;
	}

	const FString IconObjectPath = FString::Printf(
		TEXT("/Game/UI/Icons/%s.%s"),
		*IconAssetName,
		*IconAssetName);
	return Cast<UTexture2D>(FSoftObjectPath(IconObjectPath).TryLoad());
}

FText ATunaSweeperPickupItemActor::BuildFallbackItemName() const
{
	return FText::FromString(FString::Printf(TEXT("Item %d"), ItemId));
}
