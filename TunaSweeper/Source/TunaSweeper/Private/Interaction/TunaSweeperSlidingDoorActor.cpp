#include "Interaction/TunaSweeperSlidingDoorActor.h"

#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Pawn.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

ATunaSweeperSlidingDoorActor::ATunaSweeperSlidingDoorActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	DoorFrameMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorFrameMesh"));
	DoorFrameMeshComponent->SetupAttachment(SceneRoot);
	DoorFrameMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	LeftPanelRoot = CreateDefaultSubobject<USceneComponent>(TEXT("LeftPanelRoot"));
	LeftPanelRoot->SetupAttachment(SceneRoot);

	RightPanelRoot = CreateDefaultSubobject<USceneComponent>(TEXT("RightPanelRoot"));
	RightPanelRoot->SetupAttachment(SceneRoot);

	LeftDoorMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftDoorMesh"));
	LeftDoorMeshComponent->SetupAttachment(LeftPanelRoot);
	LeftDoorMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	RightDoorMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightDoorMesh"));
	RightDoorMeshComponent->SetupAttachment(RightPanelRoot);
	RightDoorMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	LeftPanelCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("LeftPanelCollision"));
	LeftPanelCollision->SetupAttachment(LeftPanelRoot);
	LeftPanelCollision->SetHiddenInGame(true);
	LeftPanelCollision->SetVisibility(false);
	LeftPanelCollision->CanCharacterStepUpOn = ECB_No;

	RightPanelCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("RightPanelCollision"));
	RightPanelCollision->SetupAttachment(RightPanelRoot);
	RightPanelCollision->SetHiddenInGame(true);
	RightPanelCollision->SetVisibility(false);
	RightPanelCollision->CanCharacterStepUpOn = ECB_No;

	ProximityTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("ProximityTrigger"));
	ProximityTrigger->SetupAttachment(SceneRoot);
	ProximityTrigger->SetHiddenInGame(true);
	ProximityTrigger->SetVisibility(false);
	ProximityTrigger->SetCanEverAffectNavigation(false);
	ProximityTrigger->OnComponentBeginOverlap.AddDynamic(this, &ATunaSweeperSlidingDoorActor::HandleProximityBeginOverlap);
	ProximityTrigger->OnComponentEndOverlap.AddDynamic(this, &ATunaSweeperSlidingDoorActor::HandleProximityEndOverlap);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> LeftDoorMeshFinder(
		TEXT("/Game/Environment/Bunker/Agit/Door/SM_SlidingDoor_Left.SM_SlidingDoor_Left"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> RightDoorMeshFinder(
		TEXT("/Game/Environment/Bunker/Agit/Door/SM_SlidingDoor_Right.SM_SlidingDoor_Right"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> DoorFrameMeshFinder(
		TEXT("/Game/Environment/Bunker/Agit/Door/SM_SlidingDoorFrame.SM_SlidingDoorFrame"));
	if (LeftDoorMeshFinder.Succeeded())
	{
		LeftDoorMesh = LeftDoorMeshFinder.Object;
	}
	if (RightDoorMeshFinder.Succeeded())
	{
		RightDoorMesh = RightDoorMeshFinder.Object;
	}
	if (DoorFrameMeshFinder.Succeeded())
	{
		DoorFrameMesh = DoorFrameMeshFinder.Object;
	}

	ApplyConfiguration();
	ApplyDoorPose();
}

void ATunaSweeperSlidingDoorActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	OpenAlpha = bStartsOpen ? 1.0f : 0.0f;
	DoorState = bStartsOpen
		? ETunaSweeperSlidingDoorState::Open
		: ETunaSweeperSlidingDoorState::Closed;
	ApplyConfiguration();
	ApplyDoorPose();
}

void ATunaSweeperSlidingDoorActor::BeginPlay()
{
	Super::BeginPlay();

	OpenAlpha = bStartsOpen ? 1.0f : 0.0f;
	DoorState = bStartsOpen
		? ETunaSweeperSlidingDoorState::Open
		: ETunaSweeperSlidingDoorState::Closed;
	ApplyConfiguration();
	ApplyDoorPose();
	SetActorTickEnabled(false);
	AddInitiallyOverlappingPlayers();
}

void ATunaSweeperSlidingDoorActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (DoorState == ETunaSweeperSlidingDoorState::Opening)
	{
		OpenAlpha = FMath::Min(1.0f, OpenAlpha + DeltaSeconds / FMath::Max(OpenDuration, 0.01f));
		if (OpenAlpha >= 1.0f)
		{
			DoorState = ETunaSweeperSlidingDoorState::Open;
			SetActorTickEnabled(false);
		}
	}
	else if (DoorState == ETunaSweeperSlidingDoorState::Closing)
	{
		OpenAlpha = FMath::Max(0.0f, OpenAlpha - DeltaSeconds / FMath::Max(CloseDuration, 0.01f));
		if (OpenAlpha <= 0.0f)
		{
			DoorState = ETunaSweeperSlidingDoorState::Closed;
			SetActorTickEnabled(false);
		}
	}
	else
	{
		SetActorTickEnabled(false);
	}

	ApplyDoorPose();
}

void ATunaSweeperSlidingDoorActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(AutoCloseTimerHandle);
	NearbyPlayers.Reset();
	Super::EndPlay(EndPlayReason);
}

void ATunaSweeperSlidingDoorActor::OpenDoor()
{
	SetDoorOpen(true, false);
}

void ATunaSweeperSlidingDoorActor::CloseDoor()
{
	SetDoorOpen(false, false);
}

void ATunaSweeperSlidingDoorActor::ToggleDoor()
{
	const bool bIsOpenOrOpening = DoorState == ETunaSweeperSlidingDoorState::Open
		|| DoorState == ETunaSweeperSlidingDoorState::Opening;
	SetDoorOpen(!bIsOpenOrOpening, false);
}

void ATunaSweeperSlidingDoorActor::SetDoorOpen(bool bInOpen, bool bInstant)
{
	if (bInOpen)
	{
		GetWorldTimerManager().ClearTimer(AutoCloseTimerHandle);
	}

	if (bInstant)
	{
		OpenAlpha = bInOpen ? 1.0f : 0.0f;
		DoorState = bInOpen
			? ETunaSweeperSlidingDoorState::Open
			: ETunaSweeperSlidingDoorState::Closed;
		ApplyDoorPose();
		SetActorTickEnabled(false);
		return;
	}

	if (bInOpen)
	{
		if (OpenAlpha >= 1.0f)
		{
			DoorState = ETunaSweeperSlidingDoorState::Open;
			return;
		}

		DoorState = ETunaSweeperSlidingDoorState::Opening;
	}
	else
	{
		if (OpenAlpha <= 0.0f)
		{
			DoorState = ETunaSweeperSlidingDoorState::Closed;
			return;
		}

		DoorState = ETunaSweeperSlidingDoorState::Closing;
	}

	SetActorTickEnabled(true);
}

void ATunaSweeperSlidingDoorActor::HandleProximityBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComponent,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (!bAutoOpenOnPlayerProximity || !IsEligiblePlayer(OtherActor))
	{
		return;
	}

	NearbyPlayers.Add(CastChecked<APawn>(OtherActor));
	GetWorldTimerManager().ClearTimer(AutoCloseTimerHandle);
	OpenDoor();
}

void ATunaSweeperSlidingDoorActor::HandleProximityEndOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComponent,
	int32 OtherBodyIndex)
{
	if (!IsEligiblePlayer(OtherActor))
	{
		return;
	}

	NearbyPlayers.Remove(CastChecked<APawn>(OtherActor));
	if (HasNearbyPlayers())
	{
		return;
	}

	if (AutoCloseDelay <= 0.0f)
	{
		CloseDoor();
		return;
	}

	GetWorldTimerManager().SetTimer(
		AutoCloseTimerHandle,
		this,
		&ATunaSweeperSlidingDoorActor::HandleDelayedAutoClose,
		AutoCloseDelay,
		false);
}

void ATunaSweeperSlidingDoorActor::ApplyConfiguration()
{
	const float SafeWidth = FMath::Max(DoorWidth, 10.0f);
	const float SafeHeight = FMath::Max(DoorHeight, 10.0f);
	const float SafeThickness = FMath::Max(DoorThickness, 1.0f);
	const FVector PanelDimensions(SafeWidth * 0.5f, SafeThickness, SafeHeight);

	LeftDoorMeshComponent->SetStaticMesh(LeftDoorMesh);
	RightDoorMeshComponent->SetStaticMesh(RightDoorMesh);
	DoorFrameMeshComponent->SetStaticMesh(DoorFrameMesh);
	LeftDoorMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RightDoorMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	DoorFrameMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ApplyMeshDimensions(LeftDoorMeshComponent, PanelDimensions);
	ApplyMeshDimensions(RightDoorMeshComponent, PanelDimensions);
	ApplyFrameMeshScale(FVector(SafeWidth, SafeThickness, SafeHeight));

	for (UBoxComponent* PanelCollision : { LeftPanelCollision.Get(), RightPanelCollision.Get() })
	{
		PanelCollision->SetBoxExtent(PanelDimensions * 0.5f);
		PanelCollision->SetRelativeLocation(FVector(0.0f, 0.0f, SafeHeight * 0.5f));
		PanelCollision->SetCollisionObjectType(ECC_WorldStatic);
		PanelCollision->SetCollisionResponseToAllChannels(ECR_Block);
		PanelCollision->SetGenerateOverlapEvents(false);
		PanelCollision->SetCollisionEnabled(
			bEnablePanelCollision ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
	}

	ProximityTrigger->SetRelativeLocation(FVector(0.0f, 0.0f, SafeHeight * 0.5f));
	ProximityTrigger->SetBoxExtent(FVector(
		SafeWidth * 0.5f + FMath::Max(0.0f, DetectionSidePadding),
		FMath::Max(1.0f, PlayerDetectionDistance),
		SafeHeight * 0.5f + FMath::Max(0.0f, DetectionHeightPadding)));
	ProximityTrigger->SetCollisionObjectType(ECC_WorldDynamic);
	ProximityTrigger->SetCollisionResponseToAllChannels(ECR_Ignore);
	ProximityTrigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	ProximityTrigger->SetGenerateOverlapEvents(bAutoOpenOnPlayerProximity);
	ProximityTrigger->SetCollisionEnabled(
		bAutoOpenOnPlayerProximity ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
}

void ATunaSweeperSlidingDoorActor::ApplyDoorPose()
{
	const float SafeWidth = FMath::Max(DoorWidth, 10.0f);
	const float ClampedAlpha = FMath::Clamp(OpenAlpha, 0.0f, 1.0f);
	const float MotionAlpha = bUseSmoothStepMotion
		? FMath::SmoothStep(0.0f, 1.0f, ClampedAlpha)
		: ClampedAlpha;
	const float PanelCenterOffset = SafeWidth * 0.25f
		+ FMath::Max(0.0f, PanelTravelDistance) * MotionAlpha;

	LeftPanelRoot->SetRelativeLocation(FVector(-PanelCenterOffset, 0.0f, 0.0f));
	RightPanelRoot->SetRelativeLocation(FVector(PanelCenterOffset, 0.0f, 0.0f));
}

void ATunaSweeperSlidingDoorActor::ApplyMeshDimensions(
	UStaticMeshComponent* MeshComponent,
	const FVector& TargetDimensions) const
{
	if (!bAutoFitPanelMeshes || !MeshComponent || !MeshComponent->GetStaticMesh())
	{
		return;
	}

	const FBoxSphereBounds SourceBounds = MeshComponent->GetStaticMesh()->GetBounds();
	const FVector SourceSize = FVector(SourceBounds.BoxExtent) * 2.0f;
	if (SourceSize.X <= KINDA_SMALL_NUMBER
		|| SourceSize.Y <= KINDA_SMALL_NUMBER
		|| SourceSize.Z <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	const FVector MeshScale(
		TargetDimensions.X / SourceSize.X,
		TargetDimensions.Y / SourceSize.Y,
		TargetDimensions.Z / SourceSize.Z);
	MeshComponent->SetRelativeScale3D(MeshScale);
	MeshComponent->SetRelativeLocation(FVector(
		-SourceBounds.Origin.X * MeshScale.X,
		-SourceBounds.Origin.Y * MeshScale.Y,
		-(SourceBounds.Origin.Z - SourceBounds.BoxExtent.Z) * MeshScale.Z));
}

void ATunaSweeperSlidingDoorActor::ApplyFrameMeshScale(const FVector& TargetDoorDimensions) const
{
	if (!bAutoFitPanelMeshes || !DoorFrameMeshComponent || !DoorFrameMeshComponent->GetStaticMesh())
	{
		return;
	}

	const UStaticMesh* LeftMesh = LeftDoorMeshComponent ? LeftDoorMeshComponent->GetStaticMesh() : nullptr;
	const UStaticMesh* RightMesh = RightDoorMeshComponent ? RightDoorMeshComponent->GetStaticMesh() : nullptr;
	if (!LeftMesh || !RightMesh)
	{
		return;
	}

	const FVector LeftSize = FVector(LeftMesh->GetBounds().BoxExtent) * 2.0f;
	const FVector RightSize = FVector(RightMesh->GetBounds().BoxExtent) * 2.0f;
	const FVector SourceDoorDimensions(
		LeftSize.X + RightSize.X,
		FMath::Max(LeftSize.Y, RightSize.Y),
		FMath::Max(LeftSize.Z, RightSize.Z));
	if (SourceDoorDimensions.X <= KINDA_SMALL_NUMBER
		|| SourceDoorDimensions.Y <= KINDA_SMALL_NUMBER
		|| SourceDoorDimensions.Z <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	const FVector FrameScale(
		TargetDoorDimensions.X / SourceDoorDimensions.X,
		TargetDoorDimensions.Y / SourceDoorDimensions.Y,
		TargetDoorDimensions.Z / SourceDoorDimensions.Z);
	const FBoxSphereBounds FrameBounds = DoorFrameMeshComponent->GetStaticMesh()->GetBounds();
	DoorFrameMeshComponent->SetRelativeScale3D(FrameScale);
	DoorFrameMeshComponent->SetRelativeLocation(FVector(
		-FrameBounds.Origin.X * FrameScale.X,
		-FrameBounds.Origin.Y * FrameScale.Y,
		-(FrameBounds.Origin.Z - FrameBounds.BoxExtent.Z) * FrameScale.Z));
}

bool ATunaSweeperSlidingDoorActor::IsEligiblePlayer(const AActor* Actor) const
{
	const APawn* Pawn = Cast<APawn>(Actor);
	return Pawn && Pawn->IsPlayerControlled();
}

bool ATunaSweeperSlidingDoorActor::HasNearbyPlayers()
{
	for (auto PlayerIt = NearbyPlayers.CreateIterator(); PlayerIt; ++PlayerIt)
	{
		if (!PlayerIt->IsValid())
		{
			PlayerIt.RemoveCurrent();
		}
	}

	return !NearbyPlayers.IsEmpty();
}

void ATunaSweeperSlidingDoorActor::HandleDelayedAutoClose()
{
	if (bAutoOpenOnPlayerProximity && !HasNearbyPlayers())
	{
		CloseDoor();
	}
}

void ATunaSweeperSlidingDoorActor::AddInitiallyOverlappingPlayers()
{
	if (!bAutoOpenOnPlayerProximity || !ProximityTrigger)
	{
		return;
	}

	TArray<AActor*> OverlappingActors;
	ProximityTrigger->GetOverlappingActors(OverlappingActors, APawn::StaticClass());
	for (AActor* OverlappingActor : OverlappingActors)
	{
		if (IsEligiblePlayer(OverlappingActor))
		{
			NearbyPlayers.Add(CastChecked<APawn>(OverlappingActor));
		}
	}

	if (HasNearbyPlayers())
	{
		OpenDoor();
	}
}
