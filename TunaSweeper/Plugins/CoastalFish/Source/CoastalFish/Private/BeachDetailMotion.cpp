#include "BeachDetailMotion.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

ABeachDetailMotion::ABeachDetailMotion()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("ShoreAnchor")));
	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DetailMesh"));
	Ripple = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WaterRipple"));
	Pellets = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("SandGrains"));
	for (UStaticMeshComponent* Part : {Body.Get(),Ripple.Get(),static_cast<UStaticMeshComponent*>(Pellets.Get())})
	{
		Part->SetupAttachment(RootComponent);
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Part->SetCanEverAffectNavigation(false);
		Part->SetGenerateOverlapEvents(false);
		Part->PrimaryComponentTick.bCanEverTick = false;
	}
	Ripple->SetCastShadow(false);
	Ripple->SetTranslucentSortPriority(1);
	Pellets->SetCastShadow(false);
	BuoyMesh = FSoftObjectPath(TEXT("/Game/Environment/BeachDetails/SM_OrangeBuoy.SM_OrangeBuoy"));
	RippleMesh = FSoftObjectPath(TEXT("/Game/Environment/BeachDetails/SM_BuoyRipple.SM_BuoyRipple"));
	BurrowMesh = FSoftObjectPath(TEXT("/Game/Environment/BeachDetails/SM_CrabBurrow.SM_CrabBurrow"));
	GrainMesh = FSoftObjectPath(TEXT("/Game/Environment/BeachDetails/SM_SandPellet.SM_SandPellet"));
}

void ABeachDetailMotion::InitializeDetail()
{
	Random.Initialize(GetTypeHash(GetActorLocation()));
	BuoyTime = FMath::Max(0.f, InitialPhaseSeconds) + Random.FRandRange(0.f,14.f);
	EventAge = 0.;
	bNearby = false;
	SetActorTickEnabled(false);
	GetWorldTimerManager().ClearTimer(EventTimer);
	Emission.Reset();
	GrainTransforms.Reset();
	Pellets->ClearInstances();
	Body->SetStaticMesh((bIsBurrow ? BurrowMesh : BuoyMesh).LoadSynchronous());
	Ripple->SetStaticMesh(bIsBurrow ? nullptr : RippleMesh.LoadSynchronous());
	Ripple->SetVisibility(!bIsBurrow);
	Pellets->SetStaticMesh(bIsBurrow ? GrainMesh.LoadSynchronous() : nullptr);
	Pellets->SetVisibility(false);
	if (bIsBurrow)
	{
		Body->SetRelativeTransform(FTransform::Identity);
	}
	else
	{
		UpdateBuoy();
	}
}

void ABeachDetailMotion::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	const float Weight = FMath::Clamp(ShoreLiftFactor,0.f,1.f);
	Body->SetDefaultCustomPrimitiveDataFloat(0,Weight);
	Pellets->SetDefaultCustomPrimitiveDataFloat(0,Weight);
	InitializeDetail();
}

void ABeachDetailMotion::BeginPlay()
{
	Super::BeginPlay();
	if (GetNetMode() == NM_DedicatedServer) return;
	InitializeDetail();
	const float Weight = FMath::Clamp(ShoreLiftFactor,0.f,1.f);
	Body->SetCustomPrimitiveDataFloat(0,Weight);
	Pellets->SetCustomPrimitiveDataFloat(0,Weight);
	RefreshProximity();
	GetWorldTimerManager().SetTimer(ProximityTimer,this,&ThisClass::RefreshProximity,0.25f,true);
}

void ABeachDetailMotion::EndPlay(const EEndPlayReason::Type Reason)
{
	GetWorldTimerManager().ClearTimer(ProximityTimer);
	GetWorldTimerManager().ClearTimer(EventTimer);
	Super::EndPlay(Reason);
}

void ABeachDetailMotion::RefreshProximity()
{
	TArray<FVector> Players;
	for (auto It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		const APlayerController* Controller = It->Get();
		const APawn* Pawn = Controller ? Controller->GetPawnOrSpectator() : nullptr;
		if (Pawn) Players.Add(Pawn->GetActorLocation());
	}
	ApplyPlayerLocations(Players);
}

void ABeachDetailMotion::ApplyPlayerLocations(const TArray<FVector>& Players)
{
	const FVector Position = GetActorLocation();
	const bool bInRange = Players.ContainsByPredicate([&](const FVector& Player)
	{
		return FVector::DistSquared(Player,Position) < 10000. * 10000.;
	});
	if (bNearby == bInRange) return;
	bNearby = bInRange;
	if (bIsBurrow)
	{
		FTimerManager& Timers = GetWorldTimerManager();
		if (!bNearby)
		{
			Timers.PauseTimer(EventTimer);
		}
		else if (Timers.IsTimerPaused(EventTimer))
		{
			Timers.UnPauseTimer(EventTimer);
		}
		else if (Emission.IsEmpty())
		{
			ScheduleBurrowEvent(true);
		}
		Pellets->SetVisibility(bNearby && !Emission.IsEmpty());
	}
	SetActorTickEnabled(bNearby && (!bIsBurrow || !Emission.IsEmpty()));
}

void ABeachDetailMotion::UpdateBuoy()
{
	const double T = BuoyTime;
	const FVector Offset(1.65*FMath::Sin(0.93*T)+0.46*FMath::Sin(1.89*T),
		1.2*FMath::Sin(0.77*T+0.8), 1.5*FMath::Sin(1.86*T)+0.48*FMath::Sin(3.38*T+0.5));
	const FRotator Rock(2.2*FMath::Sin(1.86*T+0.6), 0.7*FMath::Sin(0.75*T), 2.8*FMath::Sin(1.65*T));
	Body->SetRelativeLocationAndRotation(Offset, Rock);
	// The water response follows the ring's center without inheriting its heave or tilt.
	Ripple->SetRelativeLocation(FVector(Offset.X,Offset.Y,0.7));
	Ripple->SetCustomPrimitiveDataFloat(1,static_cast<float>(T));
}

void ABeachDetailMotion::ScheduleBurrowEvent(bool bFirst)
{
	const float Delay = bFirst ? Random.FRandRange(2.2f,8.5f) : Random.FRandRange(7.f,22.f);
	GetWorldTimerManager().SetTimer(EventTimer,this,&ThisClass::StartBurrowEvent,Delay,false);
}

void ABeachDetailMotion::StartBurrowEvent()
{
	if (!bIsBurrow || !bNearby || !Emission.IsEmpty()) return;
	GetWorldTimerManager().ClearTimer(EventTimer);
	const int32 Count = Random.RandRange(3,7);
	const double Bearing = Random.FRandRange(0.f,2.f*UE_PI);
	for (int32 Index = 0; Index < Count; ++Index)
	{
		const double Angle = Bearing + Random.FRandRange(-0.55f,0.55f);
		const FVector Dir(FMath::Cos(Angle),FMath::Sin(Angle),0);
		const FVector Start(Dir.X*3.2,Dir.Y*2.25,1.4);
		Emission.Add({Start,Dir,Random.FRandRange(7.f,15.f),Random.FRandRange(4.f,9.f),
			Index*Random.FRandRange(0.018f,0.045f),Random.FRandRange(0.28f,0.48f),Random.FRandRange(0.64f,1.28f)});
	}
	EventAge = 0.;
	GrainTransforms.Init(FTransform::Identity,Count);
	Pellets->AddInstances(GrainTransforms,false,false,false);
	UpdateGrains();
	Pellets->SetVisibility(true);
	SetActorTickEnabled(true);
}

void ABeachDetailMotion::UpdateGrains()
{
	bool bAnyRemaining = false;
	for (int32 I = 0; I < Emission.Num(); ++I)
	{
		const FGrain& Grain = Emission[I];
		const double Age = EventAge - Grain.Delay;
		const double Arc = FMath::Clamp(Age / Grain.Duration,0.,1.);
		const double Settle = FMath::Clamp((Age - Grain.Duration) / 0.20,0.,1.);
		bAnyRemaining |= Settle < 1.;
		FVector Pos = Grain.Start + Grain.Direction * Grain.Distance * Arc;
		Pos.Z = FMath::Lerp(Grain.Start.Z,0.55,Arc) + 4. * Grain.Height * Arc * (1. - Arc);
		const double Scale = Age < 0. ? 0. : Grain.Scale*(1.-FMath::SmoothStep(0.,1.,Settle));
		GrainTransforms[I] = FTransform(FRotator(Arc*95.,I*52.,Arc*80.),Pos,FVector(Scale));
	}
	if (bAnyRemaining)
	{
		Pellets->BatchUpdateInstancesTransforms(0,GrainTransforms,false,true,true);
	}
	else
	{
		Pellets->ClearInstances();
		Pellets->SetVisibility(false);
		Emission.Reset();
		GrainTransforms.Reset();
		SetActorTickEnabled(false);
		ScheduleBurrowEvent(false);
	}
}

void ABeachDetailMotion::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bNearby) return;
	if (!bIsBurrow)
	{
		BuoyTime += DeltaSeconds;
		UpdateBuoy();
	}
	else if (!Emission.IsEmpty())
	{
		EventAge += DeltaSeconds;
		UpdateGrains();
	}
}
