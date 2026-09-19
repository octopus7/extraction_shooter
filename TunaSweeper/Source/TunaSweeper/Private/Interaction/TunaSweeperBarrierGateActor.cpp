#include "Interaction/TunaSweeperBarrierGateActor.h"

#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TimerManager.h"

ATunaSweeperBarrierGateActor::ATunaSweeperBarrierGateActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Housing = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Housing"));
	Housing->SetupAttachment(RootComponent);
	Housing->SetCollisionProfileName(TEXT("BlockAll"));
	ArmPivot = CreateDefaultSubobject<USceneComponent>(TEXT("ArmPivot"));
	ArmPivot->SetupAttachment(RootComponent);
	ArmPivot->SetRelativeLocation(FVector(0, 36, 100));
	Arm = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Arm"));
	Arm->SetupAttachment(ArmPivot);
	Arm->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ArmCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("ArmCollision"));
	ArmCollision->SetupAttachment(ArmPivot);
	ArmCollision->SetRelativeLocation(FVector(175, 0, 0));
	ArmCollision->SetBoxExtent(FVector(175, 4, 6));
	ArmCollision->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	DetectionZone = CreateDefaultSubobject<UBoxComponent>(TEXT("DetectionZone"));
	DetectionZone->SetupAttachment(RootComponent);
	DetectionZone->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	DetectionZone->SetGenerateOverlapEvents(false);
	RedLens = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RedLens"));
	RedLens->SetupAttachment(RootComponent);
	RedLens->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GreenLens = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GreenLens"));
	GreenLens->SetupAttachment(RootComponent);
	GreenLens->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RedLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("RedLight"));
	RedLight->SetupAttachment(RootComponent);
	GreenLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("GreenLight"));
	GreenLight->SetupAttachment(RootComponent);
	for (UPointLightComponent* Light : {RedLight.Get(), GreenLight.Get()})
	{
		Light->SetCastShadows(false);
		Light->SetAttenuationRadius(85);
		Light->SetSourceRadius(2);
	}
	RedLight->SetRelativeLocation(FVector(-9, 29, 79));
	GreenLight->SetRelativeLocation(FVector(9, 29, 79));
	RedLight->SetLightColor(FLinearColor(1, .015f, .005f));
	GreenLight->SetLightColor(FLinearColor(.01f, 1, .06f));
}

void ATunaSweeperBarrierGateActor::Configure()
{
	Housing->SetStaticMesh(HousingMesh);
	Arm->SetStaticMesh(ArmMesh);
	RedLens->SetStaticMesh(RedLensMesh);
	GreenLens->SetStaticMesh(GreenLensMesh);
	DetectionZone->SetRelativeLocation(FVector(165, 36, 175));
	DetectionZone->SetBoxExtent(FVector(225, FMath::Max(100.f, DetectionDistance), 200));
	CreateIndicatorMaterials();
}

void ATunaSweeperBarrierGateActor::CreateIndicatorMaterials()
{
	if (!IndicatorMaterial) { return; }
	RedMID = UMaterialInstanceDynamic::Create(IndicatorMaterial, this);
	GreenMID = UMaterialInstanceDynamic::Create(IndicatorMaterial, this);
	RedLens->SetMaterial(0, RedMID);
	GreenLens->SetMaterial(0, GreenMID);
	RedMID->SetVectorParameterValue(TEXT("LEDColor"), FLinearColor(1, .015f, .005f));
	GreenMID->SetVectorParameterValue(TEXT("LEDColor"), FLinearColor(.01f, 1, .06f));
}

void ATunaSweeperBarrierGateActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Configure();
	OpenAlpha = bStartsOpen ? 1.f : 0.f;
	bTargetOpen = bStartsOpen;
	ApplyPose();
}

void ATunaSweeperBarrierGateActor::BeginPlay()
{
	Super::BeginPlay();
	Configure();
	OpenAlpha = bStartsOpen ? 1.f : 0.f;
	bTargetOpen = bStartsOpen;
	ApplyPose();
	RefreshProximity();
	GetWorldTimerManager().SetTimer(ProximityTimer, this, &ThisClass::RefreshProximity, .1f, true);
}

void ATunaSweeperBarrierGateActor::EndPlay(const EEndPlayReason::Type Reason)
{
	GetWorldTimerManager().ClearTimer(ProximityTimer);
	GetWorldTimerManager().ClearTimer(CloseTimer);
	Super::EndPlay(Reason);
}

bool ATunaSweeperBarrierGateActor::HasNearbyPlayer() const
{
	if (!GetWorld()) { return false; }
	// Geometric query also catches initial occupants, teleportation, possession
	// changes and pawns that do not generate overlap events (including vehicles).
	const FTransform ZoneTransform = DetectionZone->GetComponentTransform();
	const FVector Extent(225, FMath::Max(100.f, DetectionDistance), 200);
	for (TActorIterator<APawn> It(GetWorld()); It; ++It)
	{
		APawn* Pawn = *It;
		// IsPlayerControlled depends on PlayerState in UE 5.7. Possession is the
		// actual requirement here, including initial spawn and vehicle hand-off.
		if (!IsValid(Pawn) || !IsValid(Pawn->GetController()) || !Pawn->GetController()->IsPlayerController()) { continue; }
		FVector Center, HalfSize;
		Pawn->GetActorBounds(true, Center, HalfSize);
		if (HalfSize.IsNearlyZero()) { Center = Pawn->GetActorLocation(); }
		// Transform all bounds corners: conservative local AABB, safe under yaw/scale.
		const FBox LocalBounds = FBox(Center - HalfSize, Center + HalfSize).TransformBy(ZoneTransform.ToInverseMatrixWithScale());
		if (LocalBounds.Intersect(FBox(-Extent, Extent))) { return true; }
	}
	return false;
}

void ATunaSweeperBarrierGateActor::RefreshProximity()
{
	if (!bAutoOpen) { GetWorldTimerManager().ClearTimer(CloseTimer); return; }
	if (HasNearbyPlayer()) { OpenGate(); }
	else if (bTargetOpen && !GetWorldTimerManager().IsTimerActive(CloseTimer))
	{
		if (AutoCloseDelay <= 0) { CloseGate(); }
		else { GetWorldTimerManager().SetTimer(CloseTimer, this, &ThisClass::CloseGate, AutoCloseDelay, false); }
	}
}

void ATunaSweeperBarrierGateActor::OpenGate()
{
	GetWorldTimerManager().ClearTimer(CloseTimer);
	bTargetOpen = true;
	SetActorTickEnabled(OpenAlpha < 1.f);
	ApplyPose();
}

void ATunaSweeperBarrierGateActor::CloseGate()
{
	GetWorldTimerManager().ClearTimer(CloseTimer);
	if (HasNearbyPlayer()) { OpenGate(); return; }
	bTargetOpen = false;
	SetActorTickEnabled(OpenAlpha > 0.f);
	ApplyPose();
}

void ATunaSweeperBarrierGateActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bTargetOpen && HasNearbyPlayer()) { OpenGate(); }
	const float Duration = FMath::Max(.05f, bTargetOpen ? OpenDuration : CloseDuration);
	OpenAlpha = FMath::Clamp(OpenAlpha + (bTargetOpen ? 1.f : -1.f) * FMath::Max(0.f, DeltaSeconds) / Duration, 0.f, 1.f);
	ApplyPose();
	if (OpenAlpha == (bTargetOpen ? 1.f : 0.f)) { SetActorTickEnabled(false); }
}

ETunaSweeperBarrierGateState ATunaSweeperBarrierGateActor::GetGateState() const
{
	if (bTargetOpen) { return OpenAlpha >= 1.f ? ETunaSweeperBarrierGateState::Open : ETunaSweeperBarrierGateState::Opening; }
	return OpenAlpha <= 0.f ? ETunaSweeperBarrierGateState::Closed : ETunaSweeperBarrierGateState::Closing;
}

void ATunaSweeperBarrierGateActor::ApplyPose()
{
	const float Angle = FMath::Clamp(OpenAngle, 10.f, 90.f) * FMath::SmoothStep(0.f, 1.f, OpenAlpha);
	ArmPivot->SetRelativeRotation(FRotator(Angle, 0, 0));
	const bool bGreen = bTargetOpen && IsPassageClear();
	if (RedMID) { RedMID->SetScalarParameterValue(TEXT("Emission"), bGreen ? .08f : FMath::Max(0.f, LEDIntensity)); }
	if (GreenMID) { GreenMID->SetScalarParameterValue(TEXT("Emission"), bGreen ? FMath::Max(0.f, LEDIntensity) : .08f); }
	RedLight->SetIntensity(bGreen ? 0.f : FMath::Max(0.f, LEDIntensity) * 1.875f);
	GreenLight->SetIntensity(bGreen ? FMath::Max(0.f, LEDIntensity) * 1.875f : 0.f);
}
