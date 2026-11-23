#include "Environment/CoastalGull.h"
#include "Environment/CoastalGullProximitySubsystem.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr float WalkSeconds = 5.f;
	constexpr float AirSeconds = 12.f;
	constexpr float CycleSeconds = WalkSeconds + AirSeconds;

	float Ramp(float Start, float End, float Value)
	{
		return FMath::SmoothStep(Start, End, Value);
	}
}

ACoastalGull::ACoastalGull()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	SetCanBeDamaged(false);
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));

	auto AddPart = [this](FName Name, USceneComponent* Parent, const FVector& Hinge, UStaticMesh* Mesh)
	{
		auto* Part = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Part->SetupAttachment(Parent);
		Part->SetRelativeLocation(Hinge);
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Part->SetCanEverAffectNavigation(false);
		Part->SetGenerateOverlapEvents(false);
		Part->SetStaticMesh(Mesh);
		return Part;
	};
	static ConstructorHelpers::FObjectFinder<UStaticMesh> BodyMesh(TEXT("/Game/Environment/CoastalWildlife/SM_GullBody"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> LeftWingMesh(TEXT("/Game/Environment/CoastalWildlife/SM_GullWingL"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> RightWingMesh(TEXT("/Game/Environment/CoastalWildlife/SM_GullWingR"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> LeftLegMesh(TEXT("/Game/Environment/CoastalWildlife/SM_GullLegL"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> RightLegMesh(TEXT("/Game/Environment/CoastalWildlife/SM_GullLegR"));

	Body = AddPart(TEXT("Body"), RootComponent, FVector::ZeroVector, BodyMesh.Object);
	WingL = AddPart(TEXT("WingL"), Body, FVector(1,-7,23), LeftWingMesh.Object);
	WingR = AddPart(TEXT("WingR"), Body, FVector(1,7,23), RightWingMesh.Object);
	LegL = AddPart(TEXT("LegL"), Body, FVector(-2,-4,15), LeftLegMesh.Object);
	LegR = AddPart(TEXT("LegR"), Body, FVector(-2,4,15), RightLegMesh.Object);
	WingL->SetRelativeRotation(FRotator(0,-79,0));
	WingR->SetRelativeRotation(FRotator(0,79,0));
	WingL->SetRelativeScale3D(FVector(1,0.55,1));
	WingR->SetRelativeScale3D(FVector(1,0.55,1));
}

void ACoastalGull::ApplyShoreLift(bool bSetDefaults)
{
	const float Weight = FMath::Clamp(ShoreLiftFactor, 0.f, 1.f);
	for (UStaticMeshComponent* Part : {Body.Get(), WingL.Get(), WingR.Get(), LegL.Get(), LegR.Get()})
	{
		if (bSetDefaults)
		{
			Part->SetDefaultCustomPrimitiveDataFloat(0, Weight);
		}
		else
		{
			Part->SetCustomPrimitiveDataFloat(0, Weight);
		}
	}
}

void ACoastalGull::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyShoreLift(true);
}

void ACoastalGull::InitializeRoute()
{
	ShoreFrame = FTransform(FRotator(0,GetActorRotation().Yaw,0), GetActorLocation());
	const FVector WalkEnd = ShoreFrame.TransformPosition(FVector(0, WalkDistance, 0));
	FHitResult Hit;
	FCollisionQueryParams Query(SCENE_QUERY_STAT(CoastalGullGround), false, this);
	if (GetWorld()->LineTraceSingleByChannel(Hit, WalkEnd + FVector(0,0,60), WalkEnd - FVector(0,0,60), ECC_WorldStatic, Query))
	{
		TakeoffGroundOffset = Hit.ImpactPoint.Z - WalkEnd.Z;
	}
	// Vary the initial walking beat but always start on the authored dry strip.
	CycleTime = static_cast<float>(GetTypeHash(GetActorLocation()) % 3800) * 0.001f;
	ApplyMotion();
}

void ACoastalGull::BeginPlay()
{
	Super::BeginPlay();
	ApplyShoreLift(false);
	InitializeRoute();
	GetWorld()->GetSubsystem<UCoastalGullProximitySubsystem>()->RegisterGull(this);
}

void ACoastalGull::EndPlay(const EEndPlayReason::Type Reason)
{
	if (auto* Scheduler = GetWorld()->GetSubsystem<UCoastalGullProximitySubsystem>())
	{
		Scheduler->UnregisterGull(this);
	}
	Super::EndPlay(Reason);
}

void ACoastalGull::UpdateDisturbance(const TArray<FVector>& Players)
{
	const bool bEscaping = bPerchOccupied || EscapeBlend > 0.f;
	const FVector Origin = bEscaping ? EscapeOrigin : GetActorLocation();
	const FVector* Nearest = nullptr;
	double NearestSquared = FMath::Square(bEscaping ? 850.0 : 600.0);
	for (const FVector& Player : Players)
	{
		const double Distance = FVector::DistSquared(Origin, Player);
		if (Distance <= NearestSquared)
		{
			Nearest = &Player;
			NearestSquared = Distance;
		}
	}
	bPerchOccupied = Nearest != nullptr;
	if (Nearest && !bEscaping)
	{
		EscapeOrigin = Origin;
		EscapeDirection = (Origin - *Nearest).GetSafeNormal2D();
		if (EscapeDirection.IsNearlyZero()) EscapeDirection = ShoreFrame.GetUnitAxis(EAxis::X);
		EscapeTime = 0.f;
	}
}

void ACoastalGull::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bPerchOccupied || EscapeBlend > 0.f)
	{
		EscapeTime += DeltaSeconds;
		const bool bStayAloft = bPerchOccupied || EscapeTime < 3.f;
		EscapeBlend = FMath::FInterpConstantTo(EscapeBlend, bStayAloft ? 1.f : 0.f, DeltaSeconds, bStayAloft ? 0.58f : 0.35f);
		const FVector Previous = GetActorLocation();
		const FRotator PreviousRotation = GetActorRotation();
		ApplyMotion();
		const float Weight = Ramp(0.f, 1.f, EscapeBlend);
		const FVector Side(-EscapeDirection.Y, EscapeDirection.X, 0);
		const float Angle = EscapeTime * 1.15f;
		const FVector Away = EscapeDirection * (740.f - 210.f * FMath::Cos(Angle))
			+ Side * (210.f * FMath::Sin(Angle)) + FVector::UpVector * (270.f + 20.f * FMath::Sin(Angle));
		const FVector Position = EscapeOrigin + Weight * Away;
		const FVector Travel = (Position - Previous).GetSafeNormal2D();
		const FRotator FlightYaw = Travel.IsNearlyZero() ? PreviousRotation : Travel.Rotation();
		const float Spread = Ramp(0.f, 0.35f, EscapeBlend);
		const FRotator Target = FMath::Lerp(GetActorRotation(), FlightYaw, Spread);
		SetActorLocationAndRotation(Position, FMath::RInterpTo(PreviousRotation, Target, DeltaSeconds, 9.f));
		const float Roll = 36.f * FMath::Sin(EscapeTime * 15.f);
		for (UStaticMeshComponent* Wing : {WingL.Get(), WingR.Get()})
		{
			const float Sign = Wing == WingL.Get() ? 1.f : -1.f;
			Wing->SetRelativeRotation(FMath::Lerp(Wing->GetRelativeRotation(), FRotator(0, 0, Sign * Roll), Spread));
			Wing->SetRelativeScale3D(FMath::Lerp(Wing->GetRelativeScale3D(), FVector::OneVector, Spread));
		}
		for (UStaticMeshComponent* Leg : {LegL.Get(), LegR.Get()})
			Leg->SetRelativeRotation(FMath::Lerp(Leg->GetRelativeRotation(), FRotator(-65,0,0), Weight));
		return;
	}
	CycleTime = FMath::Fmod(CycleTime + DeltaSeconds, CycleSeconds);
	ApplyMotion();
}

void ACoastalGull::ApplyMotion()
{
	const bool bWalking = CycleTime < WalkSeconds;
	const float Walk = FMath::Min(CycleTime / WalkSeconds, 1.f);
	const float Air = FMath::Max((CycleTime - WalkSeconds) / AirSeconds, 0.f);
	const float Stride = FMath::Sin(Walk * UE_TWO_PI * 6.f);
	float Heading = 90.f;
	FVector Offset(0, Walk * WalkDistance, Walk * TakeoffGroundOffset);

	if (!bWalking)
	{
		// Slow the orbit near the dry strip. Both ends are tangent to the walking direction.
		const float Phase = Air * UE_TWO_PI;
		const float Angle = Phase - 0.88f * FMath::Sin(Phase);
		const float TurnRate = UE_TWO_PI * (1.f - 0.88f * FMath::Cos(Phase));
		const float ShoreBlend = 1.f - Ramp(0, 1, Air);
		Offset.X = FlightReach * 0.5f * (1.f - FMath::Cos(Angle));
		Offset.Y = FlightWidth * FMath::Sin(Angle) + WalkDistance * ShoreBlend;
		Offset.Z = FlightHeight * FMath::Square(FMath::Sin(UE_PI * Air)) + TakeoffGroundOffset * ShoreBlend;
		const float DX = FlightReach * 0.5f * FMath::Sin(Angle) * TurnRate;
		const float DY = FlightWidth * FMath::Cos(Angle) * TurnRate - WalkDistance * 6.f * Air * (1.f-Air);
		Heading = FMath::RadiansToDegrees(FMath::Atan2(DY, DX));
	}

	SetActorLocationAndRotation(ShoreFrame.TransformPosition(Offset),
		ShoreFrame.TransformRotation(FRotator(0,Heading,0).Quaternion()));
	const float Spread = Ramp(0, 0.10f, Air) * (1.f - Ramp(0.90f, 1.f, Air));
	const float Glide = Ramp(0.34f, 0.43f, Air) * (1.f - Ramp(0.62f, 0.70f, Air));
	const float Flap = FMath::Sin(Air * UE_TWO_PI * 24.f);
	const float WingRoll = Spread * (7.f + 38.f * Flap * (1.f - Glide));
	const float Fold = 79.f * (1.f - Spread);
	WingL->SetRelativeRotation(FRotator(0, -Fold, WingRoll));
	WingR->SetRelativeRotation(FRotator(0, Fold, -WingRoll));
	const FVector WingScale(1, FMath::Lerp(0.55f,1.f,Spread), 1);
	WingL->SetRelativeScale3D(WingScale);
	WingR->SetRelativeScale3D(WingScale);

	const float Tuck = Ramp(0.04f, 0.20f, Air) * (1.f - Ramp(0.74f, 0.91f, Air));
	const float StepPitch = bWalking ? 24.f * Stride : 0.f;
	LegL->SetRelativeRotation(FRotator(StepPitch - 65.f * Tuck, 0, 0));
	LegR->SetRelativeRotation(FRotator(-StepPitch - 65.f * Tuck, 0, 0));
	const float LiftPitch = Spread * (1.f - Ramp(0.12f, 0.30f, Air));
	const float LandingFlare = Spread * Ramp(0.75f, 0.90f, Air);
	Body->SetRelativeRotation(FRotator(12.f * LiftPitch + 9.f * LandingFlare, 0, 0));
	Body->SetRelativeLocation(FVector(0,0, bWalking ? -1.3f * Stride * Stride : 0.f));
}
