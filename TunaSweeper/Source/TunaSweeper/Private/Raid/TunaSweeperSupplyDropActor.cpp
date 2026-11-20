#include "Raid/TunaSweeperSupplyDropActor.h"

#include "Components/AudioComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Interaction/TunaSweeperInteractableComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Raid/TunaSweeperSupplyDropDataAsset.h"

namespace
{
	constexpr float DescentSeconds = 7.5f;
	constexpr float CollapseSeconds = 0.85f;
	constexpr float TransportPassSeconds = 3.0f;
}

ATunaSweeperSupplyDropActor::ATunaSweeperSupplyDropActor()
{
	bUseComponentMeshPresentation = true;
	VisualMesh->SetRelativeTransform(FTransform::Identity);
	VisualMesh->SetStaticMesh(nullptr);
	VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	VisualMesh->SetCanEverAffectNavigation(false);
	LidMesh->SetStaticMesh(nullptr);
	LidMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	OpenAnimationDuration = 0.0f;

	Parachute = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Parachute"));
	Parachute->SetupAttachment(RootComponent);
	Parachute->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Parachute->SetCanEverAffectNavigation(false);
}

void ATunaSweeperSupplyDropActor::ConfigureDrop(const UTunaSweeperSupplyDropDataAsset& Assets,
	int32 DefinitionId, int32 InContentsId, const FVector& InLandingLocation)
{
	DropAssets = &Assets;
	LandingLocation = InLandingLocation;
	VisualMesh->SetStaticMesh(Assets.ContainerMesh);
	Parachute->SetStaticMesh(Assets.ParachuteMesh);
	InteractableComponent->SetRelativeLocation(FVector(0.0, 0.0, Assets.ContainerMesh->GetBoundingBox().Max.Z + 10.0));
	ConfigureLootContainerDefaults(DefinitionId, InContentsId);
}

void ATunaSweeperSupplyDropActor::BeginPlay()
{
	Super::BeginPlay();
	ReleaseLocation = GetActorLocation();
	LandingYaw = GetActorRotation().Yaw;
	if (DropAssets)
	{
		if (DropAssets->TransportSound)
		{
			TransportAudio = UGameplayStatics::SpawnSound2D(
				this, DropAssets->TransportSound, DropAssets->TransportVolume, DropAssets->TransportPitch);
		}
		SetActorTickEnabled(true);
	}
}

void ATunaSweeperSupplyDropActor::Tick(float DeltaSeconds)
{
	Elapsed += DeltaSeconds;
	if (IsValid(TransportAudio) && Elapsed >= TransportPassSeconds && Elapsed - DeltaSeconds < TransportPassSeconds)
	{
		TransportAudio->FadeOut(1.8f, 0.0f);
	}

	if (!bLanded)
	{
		const float Progress = FMath::Clamp(Elapsed / DescentSeconds, 0.0f, 1.0f);
		const float Sway = FMath::Sin(UE_PI * Progress) * FMath::Sin(UE_TWO_PI * Progress * 2.0f);
		SetActorLocationAndRotation(FMath::Lerp(ReleaseLocation, LandingLocation, Progress),
			FRotator(3.0f * Sway, LandingYaw, 5.0f * Sway));
		bLanded = Progress >= 1.0f;
		return;
	}

	const float Collapse = FMath::Clamp((Elapsed - DescentSeconds) / CollapseSeconds, 0.0f, 1.0f);
	Parachute->SetRelativeScale3D(FVector(1.0 - 0.66 * Collapse, 1.0 - 0.66 * Collapse, 1.0 - 0.79 * Collapse));
	if (Collapse < 1.0f)
	{
		return;
	}
	Parachute->SetVisibility(false);

	// Someone may enter the touchdown footprint after the approach was planned.
	if (IsLandingOccupied())
	{
		return;
	}
	VisualMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	bReadyToLoot = true;
	RefreshInteractionPrompt();
	SetActorTickEnabled(false);
}

bool ATunaSweeperSupplyDropActor::IsLandingOccupied() const
{
	const FBox Bounds = VisualMesh->GetStaticMesh()->GetBoundingBox();
	const FQuat Facing = GetActorQuat();
	const FVector Center = GetActorLocation() + Facing.RotateVector(Bounds.GetCenter());
	const FCollisionQueryParams Query(SCENE_QUERY_STAT(SupplyDropLanding), false, this);
	return GetWorld()->OverlapAnyTestByObjectType(Center, Facing, FCollisionObjectQueryParams(ECC_Pawn),
		FCollisionShape::MakeBox(Bounds.GetExtent()), Query);
}

void ATunaSweeperSupplyDropActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (IsValid(TransportAudio))
	{
		TransportAudio->Stop();
	}
	Super::EndPlay(EndPlayReason);
}

void ATunaSweeperSupplyDropActor::RefreshInteractionPrompt()
{
	InteractableComponent->SetInteractionTypeDisplayNameAndStringKey(
		bReadyToLoot ? ETunaSweeperInteractionType::LootContainerOpen : ETunaSweeperInteractionType::None,
		FText::GetEmpty(), bReadyToLoot ? FName(TEXT("ui.interaction.supply_drop")) : NAME_None);
}

bool ATunaSweeperSupplyDropActor::OpenRuntimeContainer(
	UTunaSweeperGameInstance* Game, FTunaSweeperLootContainerInstance& OutInstance)
{
	return bReadyToLoot && Super::OpenRuntimeContainer(Game, OutInstance);
}
