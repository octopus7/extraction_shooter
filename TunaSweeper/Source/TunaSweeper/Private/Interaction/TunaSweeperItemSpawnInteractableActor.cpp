#include "Interaction/TunaSweeperItemSpawnInteractableActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Interaction/TunaSweeperInteractableComponent.h"
#include "Interaction/TunaSweeperPickupItemActor.h"
#include "UObject/ConstructorHelpers.h"

ATunaSweeperItemSpawnInteractableActor::ATunaSweeperItemSpawnInteractableActor()
{
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
	VisualMesh->SetupAttachment(RootComponent);
	VisualMesh->SetRelativeScale3D(FVector(0.7f, 0.7f, 0.7f));
	VisualMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		VisualMesh->SetStaticMesh(CubeMesh.Object);
	}

	InteractableComponent = CreateDefaultSubobject<UTunaSweeperInteractableComponent>(TEXT("Interactable"));
	InteractableComponent->SetupAttachment(RootComponent);
	InteractableComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 120.0f));
	InteractableComponent->SetInteractionTypeAndDisplayName(
		ETunaSweeperInteractionType::ItemSpawn,
		FText::FromString(TEXT("아이템스폰")));

	PickupItemActorClass = TSoftClassPtr<ATunaSweeperPickupItemActor>(
		FSoftObjectPath(TEXT("/Game/Interaction/BP_PickupItem.BP_PickupItem_C")));
}

bool ATunaSweeperItemSpawnInteractableActor::SpawnRandomPickupItem(APawn* InstigatorPawn)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	FTunaSweeperItemDefinition ItemDefinition;
	if (!PickRandomItemDefinition(ItemDefinition))
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, TEXT("[Interaction] Item spawn failed: no item data."));
		}
		return false;
	}

	TSubclassOf<ATunaSweeperPickupItemActor> LoadedPickupClass = ATunaSweeperPickupItemActor::StaticClass();
	if (UClass* SoftPickupClass = PickupItemActorClass.LoadSynchronous())
	{
		if (SoftPickupClass->IsChildOf(ATunaSweeperPickupItemActor::StaticClass()))
		{
			LoadedPickupClass = SoftPickupClass;
		}
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (IsValid(InstigatorPawn))
	{
		SpawnParameters.Instigator = InstigatorPawn;
	}

	ATunaSweeperPickupItemActor* SpawnedItem = World->SpawnActor<ATunaSweeperPickupItemActor>(
		LoadedPickupClass,
		BuildRandomSpawnLocation(),
		FRotator::ZeroRotator,
		SpawnParameters);
	if (!SpawnedItem)
	{
		return false;
	}

	SpawnedItem->SetItemId(ItemDefinition.Id);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			2.0f,
			FColor::Cyan,
			FString::Printf(TEXT("[Interaction] 아이템 스폰: %s"), *SpawnedItem->GetItemDisplayName().ToString()));
	}

	return true;
}

void ATunaSweeperItemSpawnInteractableActor::ConfigureItemSpawnDefaults(
	TSoftClassPtr<ATunaSweeperPickupItemActor> InPickupItemActorClass)
{
	Modify();
	PickupItemActorClass = InPickupItemActorClass;
}

bool ATunaSweeperItemSpawnInteractableActor::PickRandomItemDefinition(FTunaSweeperItemDefinition& OutItemDefinition) const
{
	const UGameInstance* GameInstance = GetGameInstance();
	UTunaSweeperItemDataSubsystem* ItemDataSubsystem = GameInstance
		? GameInstance->GetSubsystem<UTunaSweeperItemDataSubsystem>()
		: nullptr;
	if (!ItemDataSubsystem)
	{
		OutItemDefinition = FTunaSweeperItemDefinition();
		return false;
	}

	TArray<FTunaSweeperItemDefinition> ItemDefinitions;
	if (!ItemDataSubsystem->GetAllItemDefinitions(ItemDefinitions) || ItemDefinitions.IsEmpty())
	{
		OutItemDefinition = FTunaSweeperItemDefinition();
		return false;
	}

	OutItemDefinition = ItemDefinitions[FMath::RandRange(0, ItemDefinitions.Num() - 1)];
	return true;
}

FVector ATunaSweeperItemSpawnInteractableActor::BuildRandomSpawnLocation() const
{
	const float RadiusMin = FMath::Min(MinSpawnRadius, MaxSpawnRadius);
	const float RadiusMax = FMath::Max(MinSpawnRadius, MaxSpawnRadius);
	FVector SpawnOffset = FMath::VRand();
	SpawnOffset.Z = 0.0f;
	if (!SpawnOffset.Normalize())
	{
		SpawnOffset = FVector::ForwardVector;
	}
	FVector CandidateLocation = GetActorLocation() + SpawnOffset * FMath::FRandRange(RadiusMin, RadiusMax);

	if (UWorld* World = GetWorld())
	{
		FHitResult Hit;
		const FVector TraceStart = CandidateLocation + FVector(0.0f, 0.0f, SpawnTraceHeight);
		const FVector TraceEnd = CandidateLocation - FVector(0.0f, 0.0f, SpawnTraceHeight);
		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(TunaSweeperItemSpawnGroundTrace), false, this);
		if (World->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, QueryParams) && Hit.bBlockingHit)
		{
			CandidateLocation = Hit.ImpactPoint + FVector(0.0f, 0.0f, 4.0f);
		}
	}

	return CandidateLocation;
}
