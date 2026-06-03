#include "Housing/TunaSweeperNpcFacilityActor.h"

#include "Character/TunaSweeperFacilityNpcActor.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Subsystem/TunaSweeperHousingSubsystem.h"
#include "UObject/ConstructorHelpers.h"

ATunaSweeperNpcFacilityActor::ATunaSweeperNpcFacilityActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	BaseMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BaseMesh"));
	BaseMesh->SetupAttachment(SceneRoot);
	BaseMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	BaseMesh->SetGenerateOverlapEvents(false);

	ConsoleMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ConsoleMesh"));
	ConsoleMesh->SetupAttachment(SceneRoot);
	ConsoleMesh->SetRelativeLocation(FVector(12.0f, 0.0f, 54.0f));
	ConsoleMesh->SetRelativeScale3D(FVector(0.56f, 0.72f, 0.48f));
	ConsoleMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	ConsoleMesh->SetGenerateOverlapEvents(false);

	AccentMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AccentMesh"));
	AccentMesh->SetupAttachment(SceneRoot);
	AccentMesh->SetRelativeLocation(FVector(-28.0f, 0.0f, 82.0f));
	AccentMesh->SetRelativeScale3D(FVector(0.16f, 0.16f, 0.68f));
	AccentMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	AccentMesh->SetGenerateOverlapEvents(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMeshFinder.Succeeded())
	{
		DefaultCubeMesh = CubeMeshFinder.Object;
		BaseMesh->SetStaticMesh(DefaultCubeMesh);
		ConsoleMesh->SetStaticMesh(DefaultCubeMesh);
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMeshFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderMeshFinder.Succeeded())
	{
		DefaultCylinderMesh = CylinderMeshFinder.Object;
		AccentMesh->SetStaticMesh(DefaultCylinderMesh);
	}
}

void ATunaSweeperNpcFacilityActor::BeginPlay()
{
	Super::BeginPlay();

	if (UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr)
	{
		if (UTunaSweeperHousingSubsystem* HousingSubsystem = GameInstance->GetSubsystem<UTunaSweeperHousingSubsystem>())
		{
			HousingSubsystem->OnHousingStateChanged.RemoveAll(this);
			HousingSubsystem->OnHousingStateChanged.AddUObject(
				this,
				&ATunaSweeperNpcFacilityActor::RefreshNpcForHousingState);
		}
	}

	RefreshNpcForHousingState();
}

void ATunaSweeperNpcFacilityActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr)
	{
		if (UTunaSweeperHousingSubsystem* HousingSubsystem = GameInstance->GetSubsystem<UTunaSweeperHousingSubsystem>())
		{
			HousingSubsystem->OnHousingStateChanged.RemoveAll(this);
		}
	}

	DestroySpawnedNpc();
	Super::EndPlay(EndPlayReason);
}

void ATunaSweeperNpcFacilityActor::ConfigureNpcFacility(
	const FTunaSweeperHousingFacilityDefinition& Definition,
	const FTunaSweeperHousingPlacedFacilitySaveData& Placement,
	const FTransform& WorldTransform,
	bool bPreview,
	bool bPlacementValid)
{
	InstanceId = Placement.InstanceId;
	FacilityId = Definition.FacilityId;
	bIsPreview = bPreview;
	SetActorTransform(WorldTransform);
	ApplyPreviewVisualState(bPreview, bPlacementValid);
	RefreshNpcForHousingState();
}

void ATunaSweeperNpcFacilityActor::ConfigureNpcFacilityDefaults(
	TSubclassOf<ATunaSweeperFacilityNpcActor> InNpcClass,
	FVector InNpcRelativeLocation,
	FRotator InNpcRelativeRotation)
{
	NpcClass = InNpcClass;
	NpcRelativeLocation = InNpcRelativeLocation;
	NpcRelativeRotation = InNpcRelativeRotation;
}

void ATunaSweeperNpcFacilityActor::RefreshNpcForHousingState()
{
	if (bIsPreview || IsHousingModeOpen() || !NpcClass)
	{
		DestroySpawnedNpc();
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		DestroySpawnedNpc();
		return;
	}

	if (!IsValid(SpawnedNpc) || !SpawnedNpc->IsA(NpcClass))
	{
		DestroySpawnedNpc();

		FActorSpawnParameters SpawnParameters;
		SpawnParameters.Owner = this;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SpawnedNpc = World->SpawnActor<ATunaSweeperFacilityNpcActor>(
			NpcClass,
			BuildNpcWorldTransform(),
			SpawnParameters);
	}

	if (SpawnedNpc)
	{
		SpawnedNpc->SetActorTransform(BuildNpcWorldTransform());
		SpawnedNpc->SetActorHiddenInGame(false);
		SpawnedNpc->SetActorEnableCollision(true);
	}
}

void ATunaSweeperNpcFacilityActor::DestroySpawnedNpc()
{
	if (IsValid(SpawnedNpc))
	{
		SpawnedNpc->Destroy();
	}
	SpawnedNpc = nullptr;
}

void ATunaSweeperNpcFacilityActor::ApplyPreviewVisualState(bool bPreview, bool bPlacementValid)
{
	TArray<UPrimitiveComponent*> PrimitiveComponents;
	GetComponents(PrimitiveComponents);
	for (UPrimitiveComponent* PrimitiveComponent : PrimitiveComponents)
	{
		if (!PrimitiveComponent)
		{
			continue;
		}

		PrimitiveComponent->SetCollisionEnabled(bPreview ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryAndPhysics);
		PrimitiveComponent->SetGenerateOverlapEvents(false);
		PrimitiveComponent->SetVisibility(true, true);
		PrimitiveComponent->SetHiddenInGame(false, true);
		PrimitiveComponent->SetRenderCustomDepth(bPreview);
		PrimitiveComponent->SetCustomDepthStencilValue(bPlacementValid ? 1 : 2);
	}
}

bool ATunaSweeperNpcFacilityActor::IsHousingModeOpen() const
{
	const UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	const UTunaSweeperHousingSubsystem* HousingSubsystem = GameInstance
		? GameInstance->GetSubsystem<UTunaSweeperHousingSubsystem>()
		: nullptr;
	return HousingSubsystem && HousingSubsystem->IsHousingModeOpen();
}

FTransform ATunaSweeperNpcFacilityActor::BuildNpcWorldTransform() const
{
	const FTransform ActorTransform = GetActorTransform();
	const FVector NpcLocation = ActorTransform.TransformPosition(NpcRelativeLocation);
	const FRotator NpcRotation = ActorTransform.GetRotation().Rotator() + NpcRelativeRotation;
	return FTransform(NpcRotation, NpcLocation, FVector::OneVector);
}

ATunaSweeperSignalControlFacilityActor::ATunaSweeperSignalControlFacilityActor()
{
	ConfigureNpcFacilityDefaults(
		ATunaSweeperSignalBotActor::StaticClass(),
		FVector(70.0f, 0.0f, 100.0f),
		FRotator(0.0f, 180.0f, 0.0f));

	if (BaseMesh)
	{
		BaseMesh->SetRelativeScale3D(FVector(1.0f, 1.0f, 0.42f));
	}
	if (ConsoleMesh)
	{
		ConsoleMesh->SetRelativeLocation(FVector(18.0f, 0.0f, 72.0f));
		ConsoleMesh->SetRelativeScale3D(FVector(0.78f, 0.58f, 0.56f));
	}
	if (AccentMesh)
	{
		AccentMesh->SetRelativeLocation(FVector(-30.0f, 0.0f, 116.0f));
		AccentMesh->SetRelativeScale3D(FVector(0.12f, 0.12f, 1.1f));
	}
}

ATunaSweeperSupplyFacilityActor::ATunaSweeperSupplyFacilityActor()
{
	ConfigureNpcFacilityDefaults(
		ATunaSweeperRicePotBotActor::StaticClass(),
		FVector(68.0f, 0.0f, 84.0f),
		FRotator(0.0f, 180.0f, 0.0f));

	if (BaseMesh)
	{
		BaseMesh->SetRelativeScale3D(FVector(1.0f, 1.0f, 0.38f));
	}
	if (ConsoleMesh)
	{
		ConsoleMesh->SetRelativeLocation(FVector(10.0f, 0.0f, 58.0f));
		ConsoleMesh->SetRelativeScale3D(FVector(0.72f, 0.72f, 0.44f));
	}
	if (AccentMesh)
	{
		AccentMesh->SetRelativeLocation(FVector(-30.0f, 0.0f, 74.0f));
		AccentMesh->SetRelativeScale3D(FVector(0.18f, 0.18f, 0.52f));
	}
}
