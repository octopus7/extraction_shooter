#include "FoldingCanopyGarageDoorActor.h"

#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "DrawDebugHelpers.h"
#include "Materials/MaterialInterface.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "TimerManager.h"

namespace FoldingCanopyGarageDoor
{
	constexpr int32 UpperPanelCount = 4;
	constexpr float FullyOpenTolerance = KINDA_SMALL_NUMBER;

	float RemapClamped(float Value, float InMin, float InMax)
	{
		if (FMath::IsNearlyEqual(InMin, InMax))
		{
			return Value >= InMax ? 1.0f : 0.0f;
		}

		return FMath::Clamp((Value - InMin) / (InMax - InMin), 0.0f, 1.0f);
	}

	float Smooth01(float Value)
	{
		const float Clamped = FMath::Clamp(Value, 0.0f, 1.0f);
		return Clamped * Clamped * (3.0f - 2.0f * Clamped);
	}

	float SmoothRemap(float Value, float InMin, float InMax)
	{
		return Smooth01(RemapClamped(Value, InMin, InMax));
	}
}

AFoldingCanopyGarageDoor::AFoldingCanopyGarageDoor()
{
	PrimaryActorTick.bCanEverTick = true;
	SetActorTickEnabled(false);

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(SceneRoot);

	FrameTopComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FrameTop"));
	FrameTopComponent->SetupAttachment(SceneRoot);
	FrameLeftComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FrameLeft"));
	FrameLeftComponent->SetupAttachment(SceneRoot);
	FrameRightComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FrameRight"));
	FrameRightComponent->SetupAttachment(SceneRoot);
	LedBarComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LedBar"));
	LedBarComponent->SetupAttachment(FrameTopComponent);

	DoorCarrier = CreateDefaultSubobject<USceneComponent>(TEXT("DoorCarrier"));
	DoorCarrier->SetupAttachment(SceneRoot);
	Hinge01 = CreateDefaultSubobject<USceneComponent>(TEXT("Hinge01"));
	Hinge01->SetupAttachment(DoorCarrier);
	UpperPanel01 = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("UpperPanel01"));
	UpperPanel01->SetupAttachment(Hinge01);
	Hinge02 = CreateDefaultSubobject<USceneComponent>(TEXT("Hinge02"));
	Hinge02->SetupAttachment(Hinge01);
	UpperPanel02 = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("UpperPanel02"));
	UpperPanel02->SetupAttachment(Hinge02);
	Hinge03 = CreateDefaultSubobject<USceneComponent>(TEXT("Hinge03"));
	Hinge03->SetupAttachment(Hinge02);
	UpperPanel03 = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("UpperPanel03"));
	UpperPanel03->SetupAttachment(Hinge03);
	Hinge04 = CreateDefaultSubobject<USceneComponent>(TEXT("Hinge04"));
	Hinge04->SetupAttachment(Hinge03);
	UpperPanel04 = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("UpperPanel04"));
	UpperPanel04->SetupAttachment(Hinge04);

	LowerPanelRoot = CreateDefaultSubobject<USceneComponent>(TEXT("LowerPanelRoot"));
	LowerPanelRoot->SetupAttachment(SceneRoot);
	LowerPanelComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LowerEmbeddedPanel"));
	LowerPanelComponent->SetupAttachment(LowerPanelRoot);

	AutoOpenTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("AutoOpenTrigger"));
	AutoOpenTrigger->SetupAttachment(SceneRoot);
	AutoOpenTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	AutoOpenTrigger->SetCollisionResponseToAllChannels(ECR_Ignore);
	AutoOpenTrigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	AutoOpenTrigger->SetGenerateOverlapEvents(true);
	AutoOpenTrigger->OnComponentBeginOverlap.AddDynamic(this, &AFoldingCanopyGarageDoor::HandleAutoOpenTriggerBegin);
	AutoOpenTrigger->OnComponentEndOverlap.AddDynamic(this, &AFoldingCanopyGarageDoor::HandleAutoOpenTriggerEnd);

	PassageBlocker = CreateDefaultSubobject<UBoxComponent>(TEXT("PassageBlocker"));
	PassageBlocker->SetupAttachment(SceneRoot);
	PassageBlocker->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	PassageBlocker->SetGenerateOverlapEvents(false);

	UpperPanelMeshes.SetNum(FoldingCanopyGarageDoor::UpperPanelCount);
	bOverrideUpperPanelHeight.Init(false, FoldingCanopyGarageDoor::UpperPanelCount);
	UpperPanelHeightOverride.Init(SharedUpperPanelHeight, FoldingCanopyGarageDoor::UpperPanelCount);
	PeakFoldAngles.Init(-100.0f, FoldingCanopyGarageDoor::UpperPanelCount);
}

void AFoldingCanopyGarageDoor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	NormalizeFixedSizeArrays();
	ApplyMeshes();
	ApplyClosedLayout();

	float PoseAlpha = OpenAlpha;
#if WITH_EDITOR
	if (bPreviewInEditor && GetWorld() && !GetWorld()->IsGameWorld())
	{
		PoseAlpha = PreviewOpenAlpha;
	}
#endif
	ApplyDoorPose(PoseAlpha);
}

void AFoldingCanopyGarageDoor::BeginPlay()
{
	Super::BeginPlay();

	NormalizeFixedSizeArrays();
	ApplyMeshes();
	ApplyClosedLayout();
	OpenAlpha = bStartsOpen ? 1.0f : 0.0f;
	DoorState = bStartsOpen ? EFoldingCanopyGarageDoorState::Open : EFoldingCanopyGarageDoorState::Closed;
	ApplyDoorPose(OpenAlpha);
	SetActorTickEnabled(false);
}

void AFoldingCanopyGarageDoor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(AutoCloseTimerHandle);
	AutoOpenPawns.Reset();

	Super::EndPlay(EndPlayReason);
}

void AFoldingCanopyGarageDoor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (DoorState != EFoldingCanopyGarageDoorState::Opening && DoorState != EFoldingCanopyGarageDoorState::Closing)
	{
		SetActorTickEnabled(false);
		return;
	}

	const float Direction = DoorState == EFoldingCanopyGarageDoorState::Opening ? 1.0f : -1.0f;
	OpenAlpha = FMath::Clamp(OpenAlpha + Direction * DeltaSeconds / FMath::Max(OpenDuration, KINDA_SMALL_NUMBER), 0.0f, 1.0f);
	ApplyDoorPose(OpenAlpha);

	if (Direction > 0.0f && IsFullyOpen())
	{
		OpenAlpha = 1.0f;
		ApplyDoorPose(OpenAlpha);
		DoorState = EFoldingCanopyGarageDoorState::Open;
		SetActorTickEnabled(false);
		OnDoorFullyOpened.Broadcast();
	}
	else if (Direction < 0.0f && IsFullyClosed())
	{
		OpenAlpha = 0.0f;
		ApplyDoorPose(OpenAlpha);
		DoorState = EFoldingCanopyGarageDoorState::Closed;
		SetActorTickEnabled(false);
		OnDoorFullyClosed.Broadcast();
	}
}

void AFoldingCanopyGarageDoor::OpenDoor()
{
	if (!bDoorEnabled || IsFullyOpen())
	{
		return;
	}

	SetDoorState(EFoldingCanopyGarageDoorState::Opening);
	SetActorTickEnabled(true);
}

void AFoldingCanopyGarageDoor::CloseDoor()
{
	if (!bDoorEnabled || IsFullyClosed())
	{
		return;
	}

	SetDoorState(EFoldingCanopyGarageDoorState::Closing);
	SetActorTickEnabled(true);
}

void AFoldingCanopyGarageDoor::ToggleDoor()
{
	if (DoorState == EFoldingCanopyGarageDoorState::Open || DoorState == EFoldingCanopyGarageDoorState::Opening)
	{
		CloseDoor();
	}
	else
	{
		OpenDoor();
	}
}

void AFoldingCanopyGarageDoor::SetOpenAlpha(float NewOpenAlpha)
{
	OpenAlpha = FMath::Clamp(NewOpenAlpha, 0.0f, 1.0f);
	ApplyDoorPose(OpenAlpha);

	if (IsFullyOpen())
	{
		DoorState = EFoldingCanopyGarageDoorState::Open;
		SetActorTickEnabled(false);
	}
	else if (IsFullyClosed())
	{
		DoorState = EFoldingCanopyGarageDoorState::Closed;
		SetActorTickEnabled(false);
	}
}

bool AFoldingCanopyGarageDoor::IsFullyOpen() const
{
	return OpenAlpha >= 1.0f - FoldingCanopyGarageDoor::FullyOpenTolerance;
}

bool AFoldingCanopyGarageDoor::IsFullyClosed() const
{
	return OpenAlpha <= FoldingCanopyGarageDoor::FullyOpenTolerance;
}

void AFoldingCanopyGarageDoor::SetDoorEnabled(bool bNewDoorEnabled)
{
	bDoorEnabled = bNewDoorEnabled;
	if (!bDoorEnabled)
	{
		SetActorTickEnabled(false);
	}
}

void AFoldingCanopyGarageDoor::ConfigureVisualDefaults(
	UStaticMesh* InFrameTopMesh,
	UStaticMesh* InFrameLeftMesh,
	UStaticMesh* InFrameRightMesh,
	UStaticMesh* InUpperPanelMesh,
	UStaticMesh* InLowerPanelMesh,
	UStaticMesh* InLedBarMesh,
	UMaterialInterface* InMetalMaterial,
	UMaterialInterface* InLedMaterial)
{
	NormalizeFixedSizeArrays();
	FrameTopMesh = InFrameTopMesh;
	FrameLeftMesh = InFrameLeftMesh;
	FrameRightMesh = InFrameRightMesh;
	LowerPanelMesh = InLowerPanelMesh;
	LedBarMesh = InLedBarMesh;
	MetalMaterial = InMetalMaterial;
	LedMaterial = InLedMaterial;
	for (TObjectPtr<UStaticMesh>& UpperPanelMesh : UpperPanelMeshes)
	{
		UpperPanelMesh = InUpperPanelMesh;
	}

	ApplyMeshes();
	ApplyClosedLayout();
	ApplyDoorPose(OpenAlpha);
}

void AFoldingCanopyGarageDoor::HandleAutoOpenTriggerBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!bAutoOpenOnPlayerProximity || !IsEligibleAutoOpenPawn(OtherActor))
	{
		return;
	}

	APawn* Pawn = CastChecked<APawn>(OtherActor);
	AutoOpenPawns.Add(Pawn);
	GetWorldTimerManager().ClearTimer(AutoCloseTimerHandle);
	OpenDoor();
}

void AFoldingCanopyGarageDoor::HandleAutoOpenTriggerEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex)
{
	if (!IsEligibleAutoOpenPawn(OtherActor))
	{
		return;
	}

	AutoOpenPawns.Remove(CastChecked<APawn>(OtherActor));
	if (!HasAutoOpenPawns())
	{
		GetWorldTimerManager().SetTimer(
			AutoCloseTimerHandle,
			this,
			&AFoldingCanopyGarageDoor::HandleDelayedAutoClose,
			AutoCloseDelaySeconds,
			false);
	}
}

void AFoldingCanopyGarageDoor::NormalizeFixedSizeArrays()
{
	UpperPanelMeshes.SetNum(FoldingCanopyGarageDoor::UpperPanelCount);

	while (bOverrideUpperPanelHeight.Num() < FoldingCanopyGarageDoor::UpperPanelCount)
	{
		bOverrideUpperPanelHeight.Add(false);
	}
	bOverrideUpperPanelHeight.SetNum(FoldingCanopyGarageDoor::UpperPanelCount);

	while (UpperPanelHeightOverride.Num() < FoldingCanopyGarageDoor::UpperPanelCount)
	{
		UpperPanelHeightOverride.Add(SharedUpperPanelHeight);
	}
	UpperPanelHeightOverride.SetNum(FoldingCanopyGarageDoor::UpperPanelCount);

	while (PeakFoldAngles.Num() < FoldingCanopyGarageDoor::UpperPanelCount)
	{
		PeakFoldAngles.Add(-100.0f);
	}
	PeakFoldAngles.SetNum(FoldingCanopyGarageDoor::UpperPanelCount);

	LowerPanelEmbedDepth = FMath::Clamp(LowerPanelEmbedDepth, 0.0f, FMath::Max(0.0f, LowerPanelHeight - KINDA_SMALL_NUMBER));
}

void AFoldingCanopyGarageDoor::ApplyMeshes()
{
	FrameTopComponent->SetStaticMesh(FrameTopMesh);
	FrameLeftComponent->SetStaticMesh(FrameLeftMesh);
	FrameRightComponent->SetStaticMesh(FrameRightMesh);
	LedBarComponent->SetStaticMesh(LedBarMesh);
	UpperPanel01->SetStaticMesh(UpperPanelMeshes[0]);
	UpperPanel02->SetStaticMesh(UpperPanelMeshes[1]);
	UpperPanel03->SetStaticMesh(UpperPanelMeshes[2]);
	UpperPanel04->SetStaticMesh(UpperPanelMeshes[3]);
	LowerPanelComponent->SetStaticMesh(LowerPanelMesh);

	for (UStaticMeshComponent* MetalComponent : { FrameTopComponent.Get(), FrameLeftComponent.Get(), FrameRightComponent.Get(), UpperPanel01.Get(), UpperPanel02.Get(), UpperPanel03.Get(), UpperPanel04.Get(), LowerPanelComponent.Get() })
	{
		MetalComponent->SetMaterial(0, MetalMaterial);
	}
	LedBarComponent->SetMaterial(0, LedMaterial);

	const FVector FrameTopDimensions(DoorWidth + FrameSideWidth * 2.0f, FrameDepth, FrameTopHeight);
	const FVector FrameSideDimensions(FrameSideWidth, FrameDepth, GetUpperTotalHeight() + LowerPanelHeight - LowerPanelEmbedDepth);
	ApplyMeshDimensions(FrameTopComponent, FrameTopDimensions);
	ApplyMeshDimensions(FrameLeftComponent, FrameSideDimensions);
	ApplyMeshDimensions(FrameRightComponent, FrameSideDimensions);
	ApplyMeshDimensions(LedBarComponent, FVector(LedBarWidth, LedBarThickness, LedBarHeight));
	ApplyMeshDimensions(UpperPanel01, FVector(DoorWidth, DoorThickness, GetEffectiveUpperPanelHeight(0)));
	ApplyMeshDimensions(UpperPanel02, FVector(DoorWidth, DoorThickness, GetEffectiveUpperPanelHeight(1)));
	ApplyMeshDimensions(UpperPanel03, FVector(DoorWidth, DoorThickness, GetEffectiveUpperPanelHeight(2)));
	ApplyMeshDimensions(UpperPanel04, FVector(DoorWidth, DoorThickness, GetEffectiveUpperPanelHeight(3)));
	ApplyMeshDimensions(LowerPanelComponent, FVector(DoorWidth, DoorThickness, LowerPanelHeight));

	for (UStaticMeshComponent* FrameComponent : { FrameTopComponent.Get(), FrameLeftComponent.Get(), FrameRightComponent.Get() })
	{
		FrameComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		FrameComponent->SetCollisionResponseToAllChannels(ECR_Block);
	}

	for (UStaticMeshComponent* MovingVisualComponent : { UpperPanel01.Get(), UpperPanel02.Get(), UpperPanel03.Get(), UpperPanel04.Get(), LowerPanelComponent.Get(), LedBarComponent.Get() })
	{
		MovingVisualComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void AFoldingCanopyGarageDoor::ApplyClosedLayout()
{
	const float UpperTotalHeight = GetUpperTotalHeight();
	const float LowerTopZ = LowerPanelHeight - LowerPanelEmbedDepth;
	const float UpperTopZ = LowerTopZ + UpperTotalHeight;
	const float LowerClosedCenterZ = LowerPanelHeight * 0.5f - LowerPanelEmbedDepth;
	const float DoorBottomZ = -LowerPanelEmbedDepth;

	FrameLeftComponent->SetRelativeLocation(FVector(-DoorWidth * 0.5f - FrameSideWidth * 0.5f, 0.0f, UpperTopZ * 0.5f));
	FrameRightComponent->SetRelativeLocation(FVector(DoorWidth * 0.5f + FrameSideWidth * 0.5f, 0.0f, UpperTopZ * 0.5f));
	FrameTopComponent->SetRelativeLocation(FVector(0.0f, 0.0f, UpperTopZ + FrameTopHeight * 0.5f));
	LedBarComponent->SetRelativeLocation(FVector(0.0f, -FrameDepth * 0.5f - LedBarThickness * 0.5f, -FrameTopHeight * 0.5f));

	DoorCarrier->SetRelativeLocation(FVector(0.0f, 0.0f, UpperTopZ));
	Hinge01->SetRelativeLocation(FVector::ZeroVector);
	Hinge02->SetRelativeLocation(FVector(0.0f, 0.0f, -GetEffectiveUpperPanelHeight(0)));
	Hinge03->SetRelativeLocation(FVector(0.0f, 0.0f, -GetEffectiveUpperPanelHeight(1)));
	Hinge04->SetRelativeLocation(FVector(0.0f, 0.0f, -GetEffectiveUpperPanelHeight(2)));
	UpperPanel01->SetRelativeLocation(FVector(0.0f, 0.0f, -GetEffectiveUpperPanelHeight(0) * 0.5f));
	UpperPanel02->SetRelativeLocation(FVector(0.0f, 0.0f, -GetEffectiveUpperPanelHeight(1) * 0.5f));
	UpperPanel03->SetRelativeLocation(FVector(0.0f, 0.0f, -GetEffectiveUpperPanelHeight(2) * 0.5f));
	UpperPanel04->SetRelativeLocation(FVector(0.0f, 0.0f, -GetEffectiveUpperPanelHeight(3) * 0.5f));
	LowerPanelRoot->SetRelativeLocation(FVector(0.0f, 0.0f, LowerClosedCenterZ));

	const float DoorSpan = UpperTopZ - DoorBottomZ;
	PassageBlocker->SetBoxExtent(FVector(DoorWidth * 0.5f, DoorThickness * 0.5f + 6.0f, DoorSpan * 0.5f));
	PassageBlocker->SetRelativeLocation(FVector(0.0f, 0.0f, (UpperTopZ + DoorBottomZ) * 0.5f));

	AutoOpenTrigger->SetBoxExtent(FVector(DoorWidth * 0.5f + AutoOpenSidePadding, AutoOpenDepth, DoorSpan * 0.5f + AutoOpenHeightPadding));
	AutoOpenTrigger->SetRelativeLocation(FVector(0.0f, 0.0f, (UpperTopZ + DoorBottomZ) * 0.5f));
}

void AFoldingCanopyGarageDoor::ApplyDoorPose(float PoseAlpha)
{
	const float ClampedAlpha = FMath::Clamp(PoseAlpha, 0.0f, 1.0f);
	const float LiftAlpha = FoldingCanopyGarageDoor::SmoothRemap(ClampedAlpha, 0.10f, 0.90f);
	const float RotationAlpha = FoldingCanopyGarageDoor::SmoothRemap(ClampedAlpha, 0.18f, 0.92f);
	const float GroundAlpha = FoldingCanopyGarageDoor::SmoothRemap(ClampedAlpha, 0.0f, GroundDropEndAlpha);
	const float LowerClosedCenterZ = LowerPanelHeight * 0.5f - LowerPanelEmbedDepth;
	const float LowerDropDistance = (LowerPanelHeight - LowerPanelEmbedDepth) + InterlockClearance;

	DoorCarrier->SetRelativeLocation(
		FVector(0.0f, CarrierForwardTravel * LiftAlpha, GetUpperTotalHeight() + LowerPanelHeight - LowerPanelEmbedDepth + CarrierVerticalTravel * LiftAlpha));
	DoorCarrier->SetRelativeRotation(FRotator(0.0f, 0.0f, CarrierFinalRollDegrees * RotationAlpha));
	Hinge01->SetRelativeRotation(FRotator(0.0f, 0.0f, GetFoldAngle(0, ClampedAlpha)));
	Hinge02->SetRelativeRotation(FRotator(0.0f, 0.0f, GetFoldAngle(1, ClampedAlpha)));
	Hinge03->SetRelativeRotation(FRotator(0.0f, 0.0f, GetFoldAngle(2, ClampedAlpha)));
	Hinge04->SetRelativeRotation(FRotator(0.0f, 0.0f, GetFoldAngle(3, ClampedAlpha)));
	LowerPanelRoot->SetRelativeLocation(FVector(0.0f, 0.0f, LowerClosedCenterZ - LowerDropDistance * GroundAlpha));

	UpdateCollisionState(ClampedAlpha);
	DrawDebugLayout();
}

void AFoldingCanopyGarageDoor::ApplyMeshDimensions(UStaticMeshComponent* Component, const FVector& TargetDimensions) const
{
	if (!bAutoFitMeshes || !Component || !Component->GetStaticMesh())
	{
		return;
	}

	const FVector SourceSize = FVector(Component->GetStaticMesh()->GetBounds().BoxExtent) * 2.0f;
	if (SourceSize.X <= KINDA_SMALL_NUMBER || SourceSize.Y <= KINDA_SMALL_NUMBER || SourceSize.Z <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	Component->SetRelativeScale3D(FVector(
		TargetDimensions.X / SourceSize.X,
		TargetDimensions.Y / SourceSize.Y,
		TargetDimensions.Z / SourceSize.Z));
}

void AFoldingCanopyGarageDoor::UpdateCollisionState(float PoseAlpha)
{
	if (!PassageBlocker)
	{
		return;
	}

	const bool bBlockPassage = PoseAlpha < PassageBlockerOpenAlpha;
	PassageBlocker->SetCollisionEnabled(bBlockPassage ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
}

void AFoldingCanopyGarageDoor::DrawDebugLayout() const
{
	if (!GetWorld())
	{
		return;
	}

	if (bDrawDebugHinges)
	{
		for (const USceneComponent* Hinge : { Hinge01.Get(), Hinge02.Get(), Hinge03.Get(), Hinge04.Get() })
		{
			DrawDebugCoordinateSystem(GetWorld(), Hinge->GetComponentLocation(), Hinge->GetComponentRotation(), 45.0f, false, -1.0f, 0, 1.5f);
		}
	}

	if (bDrawDebugInterlock)
	{
		DrawDebugBox(GetWorld(), PassageBlocker->GetComponentLocation(), PassageBlocker->GetScaledBoxExtent(), PassageBlocker->GetComponentQuat(), FColor::Orange, false, -1.0f, 0, 1.5f);
	}
}

float AFoldingCanopyGarageDoor::GetEffectiveUpperPanelHeight(int32 PanelIndex) const
{
	if (!bOverrideUpperPanelHeight.IsValidIndex(PanelIndex) || !UpperPanelHeightOverride.IsValidIndex(PanelIndex))
	{
		return SharedUpperPanelHeight;
	}

	return bOverrideUpperPanelHeight[PanelIndex]
		? FMath::Max(UpperPanelHeightOverride[PanelIndex], 1.0f)
		: SharedUpperPanelHeight;
}

float AFoldingCanopyGarageDoor::GetUpperTotalHeight() const
{
	float TotalHeight = 0.0f;
	for (int32 PanelIndex = 0; PanelIndex < FoldingCanopyGarageDoor::UpperPanelCount; ++PanelIndex)
	{
		TotalHeight += GetEffectiveUpperPanelHeight(PanelIndex);
	}
	return TotalHeight;
}

float AFoldingCanopyGarageDoor::GetFoldAngle(int32 PanelIndex, float PoseAlpha) const
{
	const float StartAlpha = 0.08f + PanelIndex * PanelFoldDelay;
	const float PeakAlpha = StartAlpha + PanelFoldDuration * 0.5f;
	const float EndAlpha = FMath::Max(CanopySettleStartAlpha, PeakAlpha + KINDA_SMALL_NUMBER);
	const float PeakAngle = PeakFoldAngles.IsValidIndex(PanelIndex) ? PeakFoldAngles[PanelIndex] : -100.0f;

	if (PoseAlpha <= PeakAlpha)
	{
		return PeakAngle * FoldingCanopyGarageDoor::SmoothRemap(PoseAlpha, StartAlpha, PeakAlpha);
	}

	return PeakAngle * (1.0f - FoldingCanopyGarageDoor::SmoothRemap(PoseAlpha, PeakAlpha, EndAlpha));
}

bool AFoldingCanopyGarageDoor::IsEligibleAutoOpenPawn(const AActor* Actor) const
{
	const APawn* Pawn = Cast<APawn>(Actor);
	return Pawn && (!bPlayerControlledPawnsOnly || Pawn->IsPlayerControlled());
}

bool AFoldingCanopyGarageDoor::HasAutoOpenPawns()
{
	for (auto PawnIt = AutoOpenPawns.CreateIterator(); PawnIt; ++PawnIt)
	{
		if (!PawnIt->IsValid())
		{
			PawnIt.RemoveCurrent();
		}
	}

	return !AutoOpenPawns.IsEmpty();
}

void AFoldingCanopyGarageDoor::HandleDelayedAutoClose()
{
	if (!HasAutoOpenPawns())
	{
		CloseDoor();
	}
}

void AFoldingCanopyGarageDoor::SetDoorState(EFoldingCanopyGarageDoorState NewState)
{
	if (DoorState == NewState)
	{
		return;
	}

	DoorState = NewState;
	if (DoorState == EFoldingCanopyGarageDoorState::Opening)
	{
		OnDoorOpeningStarted.Broadcast();
	}
	else if (DoorState == EFoldingCanopyGarageDoorState::Closing)
	{
		OnDoorClosingStarted.Broadcast();
	}
}
