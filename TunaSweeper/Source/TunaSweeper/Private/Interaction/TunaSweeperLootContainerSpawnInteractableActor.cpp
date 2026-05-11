#include "Interaction/TunaSweeperLootContainerSpawnInteractableActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Interaction/TunaSweeperInteractableComponent.h"
#include "Interaction/TunaSweeperLootContainerActor.h"
#include "UObject/ConstructorHelpers.h"

ATunaSweeperLootContainerSpawnInteractableActor::ATunaSweeperLootContainerSpawnInteractableActor()
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
		ETunaSweeperInteractionType::LootContainerSpawn,
		FText::FromString(TEXT("\uC0C1\uC790\uC2A4\uD3F0")));

	LootContainerActorClass = TSoftClassPtr<ATunaSweeperLootContainerActor>(
		FSoftObjectPath(TEXT("/Game/Interaction/BP_LootContainer.BP_LootContainer_C")));
}

bool ATunaSweeperLootContainerSpawnInteractableActor::SpawnRandomLootContainer(APawn* InstigatorPawn)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	int32 ContainerDefinitionId = INDEX_NONE;
	int32 ContentsId = INDEX_NONE;
	if (!PickRandomContainerData(ContainerDefinitionId, ContentsId))
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, TEXT("[Interaction] Container spawn failed: no compatible data."));
		}
		return false;
	}

	TSubclassOf<ATunaSweeperLootContainerActor> LoadedContainerClass = ATunaSweeperLootContainerActor::StaticClass();
	if (UClass* SoftContainerClass = LootContainerActorClass.LoadSynchronous())
	{
		if (SoftContainerClass->IsChildOf(ATunaSweeperLootContainerActor::StaticClass()))
		{
			LoadedContainerClass = SoftContainerClass;
		}
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (IsValid(InstigatorPawn))
	{
		SpawnParameters.Instigator = InstigatorPawn;
	}

	ATunaSweeperLootContainerActor* SpawnedContainer = World->SpawnActor<ATunaSweeperLootContainerActor>(
		LoadedContainerClass,
		BuildRandomSpawnLocation(),
		FRotator::ZeroRotator,
		SpawnParameters);
	if (!SpawnedContainer)
	{
		return false;
	}

	SpawnedContainer->SetContainerDataIds(ContainerDefinitionId, ContentsId);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			2.0f,
			FColor::Cyan,
			FString::Printf(TEXT("[Interaction] Container spawned: container=%d contents=%d"), ContainerDefinitionId, ContentsId));
	}

	return true;
}

void ATunaSweeperLootContainerSpawnInteractableActor::ConfigureLootContainerSpawnDefaults(
	TSoftClassPtr<ATunaSweeperLootContainerActor> InLootContainerActorClass)
{
	Modify();
	LootContainerActorClass = InLootContainerActorClass;
}

bool ATunaSweeperLootContainerSpawnInteractableActor::PickRandomContainerData(
	int32& OutContainerDefinitionId,
	int32& OutContentsId) const
{
	const UGameInstance* GameInstance = GetGameInstance();
	UTunaSweeperItemDataSubsystem* ItemDataSubsystem = GameInstance
		? GameInstance->GetSubsystem<UTunaSweeperItemDataSubsystem>()
		: nullptr;
	if (!ItemDataSubsystem)
	{
		return false;
	}

	TArray<FTunaSweeperLootContainerDefinition> ContainerDefinitions;
	TArray<FTunaSweeperLootContainerContents> ContentsRows;
	if (!ItemDataSubsystem->GetAllLootContainerDefinitions(ContainerDefinitions) ||
		!ItemDataSubsystem->GetAllLootContainerContents(ContentsRows) ||
		ContainerDefinitions.IsEmpty() ||
		ContentsRows.IsEmpty())
	{
		return false;
	}

	const FTunaSweeperLootContainerDefinition& ContainerDefinition =
		ContainerDefinitions[FMath::RandRange(0, ContainerDefinitions.Num() - 1)];

	TArray<FTunaSweeperLootContainerContents> CompatibleContentsRows;
	for (const FTunaSweeperLootContainerContents& Contents : ContentsRows)
	{
		if (Contents.Items.Num() <= ContainerDefinition.Capacity)
		{
			CompatibleContentsRows.Add(Contents);
		}
	}

	if (CompatibleContentsRows.IsEmpty())
	{
		return false;
	}

	const FTunaSweeperLootContainerContents& Contents =
		CompatibleContentsRows[FMath::RandRange(0, CompatibleContentsRows.Num() - 1)];
	OutContainerDefinitionId = ContainerDefinition.Id;
	OutContentsId = Contents.Id;
	return true;
}

FVector ATunaSweeperLootContainerSpawnInteractableActor::BuildRandomSpawnLocation() const
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
		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(TunaSweeperLootContainerSpawnGroundTrace), false, this);
		if (World->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, QueryParams) && Hit.bBlockingHit)
		{
			CandidateLocation = Hit.ImpactPoint + FVector(0.0f, 0.0f, 4.0f);
		}
	}

	return CandidateLocation;
}

