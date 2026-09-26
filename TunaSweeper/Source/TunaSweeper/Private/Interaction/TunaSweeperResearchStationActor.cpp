#include "Interaction/TunaSweeperResearchStationActor.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Interaction/TunaSweeperInteractableComponent.h"

ATunaSweeperResearchStationActor::ATunaSweeperResearchStationActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	ScanBarMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ScanBarMesh"));
	ScanBarMesh->SetupAttachment(RootComponent);
	ScanBarMesh->SetMobility(EComponentMobility::Movable);
	ScanBarMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ScanBarMesh->SetGenerateOverlapEvents(false);
	ScanBarMesh->SetCanEverAffectNavigation(false);

	ConfigureResearchStationDefaults(
		FText::GetEmpty(),
		TSoftClassPtr<UTunaSweeperInteractionMarkerWidget>(
			FSoftObjectPath(TEXT("/Game/UI/WBP_InteractionMarker.WBP_InteractionMarker_C"))));

	if (VisualMesh)
	{
		VisualMesh->SetStaticMesh(nullptr);
		VisualMesh->SetRelativeTransform(FTransform::Identity);
		VisualMesh->SetCollisionProfileName(TEXT("BlockAll"));
		VisualMesh->SetHiddenInGame(false);
	}

	if (InteractableComponent)
	{
		InteractableComponent->SetRelativeLocation(FVector(12.0f, 0.0f, 48.0f));
	}
}

void ATunaSweeperResearchStationActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyStationPresentation();
}

void ATunaSweeperResearchStationActor::BeginPlay()
{
	Super::BeginPlay();
	ApplyStationPresentation();
	ScanElapsedSeconds = 0.0;
	SetActorTickEnabled(bAnimateScan && IsValid(StationScanBarMesh));
}

void ATunaSweeperResearchStationActor::ApplyStationPresentation()
{
	if (VisualMesh)
	{
		VisualMesh->SetStaticMesh(StationBodyMesh);
		// Replace the legacy hidden placement puck, including serialized BP overrides.
		VisualMesh->SetRelativeTransform(FTransform::Identity);
		VisualMesh->SetHiddenInGame(false);
		VisualMesh->SetVisibility(true);
		VisualMesh->SetCollisionProfileName(TEXT("BlockAll"));
	}
	if (ScanBarMesh)
	{
		ScanBarMesh->SetStaticMesh(StationScanBarMesh);
		ScanBarMesh->SetRelativeLocation(ScanOrigin);
		ScanBarMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void ATunaSweeperResearchStationActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bAnimateScan || !ScanBarMesh || !StationScanBarMesh)
	{
		return;
	}

	const double Period = FMath::Max(0.1, static_cast<double>(ScanCycleSeconds));
	ScanElapsedSeconds = FMath::Fmod(ScanElapsedSeconds + FMath::Max(0.0f, DeltaSeconds), Period);
	const double Offset = FMath::Sin(ScanElapsedSeconds / Period * UE_TWO_PI) * FMath::Max(0.0f, ScanTravel);
	ScanBarMesh->SetRelativeLocation(ScanOrigin + FVector(0.0, 0.0, Offset));
}

void ATunaSweeperResearchStationActor::ConfigureResearchStationDefaults(
	const FText& InInteractionDisplayName,
	TSoftClassPtr<UTunaSweeperInteractionMarkerWidget> InMarkerWidgetClass)
{
	ConfigureInteractionDefaults(
		ETunaSweeperInteractionType::Research,
		InInteractionDisplayName,
		InMarkerWidgetClass,
		FName(TEXT("ui.interaction.research")));
}
