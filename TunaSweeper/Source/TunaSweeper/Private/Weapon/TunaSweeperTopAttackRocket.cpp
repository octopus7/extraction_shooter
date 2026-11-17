#include "Weapon/TunaSweeperTopAttackRocket.h"

#include "AI/TunaSweeperEnemyCharacter.h"
#include "Component/TunaSweeperVitalsComponent.h"
#include "Components/DecalComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Subsystem/TunaSweeperFactionSubsystem.h"
#include "Weapon/TunaSweeperThrowableDamageType.h"

ATunaSweeperTopAttackRocket::ATunaSweeperTopAttackRocket()
{
	PrimaryActorTick.bCanEverTick = true;
	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	SetRootComponent(Collision);
	Collision->InitSphereRadius(6.f);
	Collision->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	Collision->SetGenerateOverlapEvents(false);
	Collision->SetCanEverAffectNavigation(false);
	RocketMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RocketMesh"));
	RocketMesh->SetupAttachment(Collision);
	Exhaust = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BlueExhaust"));
	Exhaust->SetupAttachment(RocketMesh);
	Blast = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BlueBlast"));
	Blast->SetupAttachment(Collision);
	for (UStaticMeshComponent* Component : {RocketMesh.Get(), Exhaust.Get(), Blast.Get()})
	{
		Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Component->SetCanEverAffectNavigation(false);
	}
	Exhaust->SetCastShadow(false);
	Blast->SetCastShadow(false);
	Blast->SetVisibility(false);
	GuidanceDecal = CreateDefaultSubobject<UDecalComponent>(TEXT("BlueGuidanceProjection"));
	GuidanceDecal->SetupAttachment(Collision);
	GuidanceDecal->SetAbsolute(true, true, true);
	GuidanceDecal->SetVisibility(false);
	GuidanceDecal->DecalSize = FVector(200.f, 180.f, 180.f);
	GuidanceDecal->SetFadeScreenSize(0.f);
}

void ATunaSweeperTopAttackRocket::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RocketMesh->SetStaticMesh(BodyMesh);
	Exhaust->SetStaticMesh(ExhaustShape);
	Exhaust->SetMaterial(0, ExhaustMaterial);
	Exhaust->SetRelativeLocation(ExhaustOffset + FVector(-22.f, 0, 0));
	Exhaust->SetRelativeRotation(FRotator(90, 0, 0));
	Exhaust->SetRelativeScale3D(FVector(0.13f, 0.13f, 0.5f));
	Blast->SetStaticMesh(BlastShape);
	Blast->SetMaterial(0, ExhaustMaterial);
	GuidanceDecal->SetDecalMaterial(GuidanceMaterial);
}

bool ATunaSweeperTopAttackRocket::IsValidTarget(const AActor* Source, const AActor* Candidate)
{
	if (!IsValid(Source) || !Source->GetWorld() || !IsValid(Candidate) || Candidate->IsActorBeingDestroyed()) return false;
	const auto* Factions = Source->GetWorld()->GetSubsystem<UTunaSweeperFactionSubsystem>();
	if (!Factions || !Factions->CanTargetActor(Source, Candidate)) return false;
	if (const auto* Enemy = Cast<ATunaSweeperEnemyCharacter>(Candidate); Enemy && Enemy->IsDead()) return false;
	const auto* Vitals = Candidate->FindComponentByClass<UTunaSweeperVitalsComponent>();
	return !Vitals || Vitals->GetVitalsState().Health > 0.f;
}

void ATunaSweeperTopAttackRocket::BeginPlay()
{
	Super::BeginPlay();
	AscentSeconds = FMath::Max(0.05f, AscentSeconds);
	GuidanceSeconds = FMath::Max(0.05f, GuidanceSeconds);
	DiveSeconds = FMath::Max(0.05f, DiveSeconds);
	LaunchLocation = GetActorLocation();
	AscentEnd = LaunchLocation + FVector(0, 0, FMath::Max(100.f, AscentHeight));
	ActiveTarget = TargetActor;
	TargetActor = nullptr;
	ImpactLocation = TargetLocation.ContainsNaN() ? LaunchLocation : TargetLocation;
	Collision->IgnoreActorWhenMoving(GetOwner(), true);
	Collision->IgnoreActorWhenMoving(GetInstigator(), true);
	SetActorRotation(FVector::UpVector.Rotation());
	UpdateTarget();
}

FVector ATunaSweeperTopAttackRocket::GroundBelow(const FVector& Point) const
{
	FCollisionQueryParams Query(SCENE_QUERY_STAT(RocketGround), false, this);
	Query.AddIgnoredActor(GetOwner());
	Query.AddIgnoredActor(ActiveTarget.Get());
	FHitResult Hit;
	if (GetWorld()->LineTraceSingleByChannel(Hit, Point + FVector(0, 0, 100), Point - FVector(0, 0, 10000), ECC_Visibility, Query))
		return Hit.ImpactPoint;
	return Point;
}

void ATunaSweeperTopAttackRocket::UpdateTarget()
{
	if (!IsValidTarget(this, ActiveTarget.Get()))
	{
		ActiveTarget.Reset();
		if (!bTargetLocked)
		{
			TArray<AActor*> Candidates;
			if (const auto* Factions = GetWorld()->GetSubsystem<UTunaSweeperFactionSubsystem>())
				Factions->GetActorsWithAttitude(this, ETunaSweeperFactionAttitude::Hostile, Candidates);
			float BestDistanceSquared = FMath::Square(FMath::Max(0.f, SearchRadius));
			for (AActor* Candidate : Candidates)
			{
				const float DistanceSquared = FVector::DistSquared2D(LaunchLocation, Candidate->GetActorLocation());
				if (DistanceSquared < BestDistanceSquared && IsValidTarget(this, Candidate))
				{
					ActiveTarget = Candidate;
					BestDistanceSquared = DistanceSquared;
				}
			}
		}
	}
	if (ActiveTarget.IsValid()) ImpactLocation = GroundBelow(ActiveTarget->GetActorLocation());
	else ImpactLocation = GroundBelow(ImpactLocation);
}

void ATunaSweeperTopAttackRocket::UpdateGuidanceDecal()
{
	GuidanceDecal->SetWorldLocation(ImpactLocation + FVector(0, 0, 50));
	GuidanceDecal->SetWorldRotation(FRotator(-90, 0, 0));
	GuidanceDecal->SetVisibility(true);
}

void ATunaSweeperTopAttackRocket::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!FMath::IsFinite(DeltaSeconds) || DeltaSeconds <= 0.f) return;
	if (Phase == ETopAttackRocketPhase::Detonated)
	{
		BlastAge += DeltaSeconds;
		const float Alpha = FMath::Clamp(BlastAge / 0.3f, 0.f, 1.f);
		Blast->SetWorldScale3D(FVector(FMath::Lerp(0.1f, DamageRadius / 50.f, Alpha)));
		if (Alpha >= 1.f) Destroy();
		return;
	}
	float Remaining = FMath::Min(DeltaSeconds, AscentSeconds + GuidanceSeconds + DiveSeconds - Age);
	while (Remaining > SMALL_NUMBER && Phase != ETopAttackRocketPhase::Detonated)
	{
		const float Boundary = Phase == ETopAttackRocketPhase::Ascent ? AscentSeconds :
			Phase == ETopAttackRocketPhase::Guidance ? AscentSeconds + GuidanceSeconds : AscentSeconds + GuidanceSeconds + DiveSeconds;
		const float Step = FMath::Min3(Remaining, 1.f / 120.f, FMath::Max(SMALL_NUMBER, Boundary - Age));
		Age += Step;
		Remaining -= Step;
		FVector Position;
		if (Phase == ETopAttackRocketPhase::Ascent)
		{
			UpdateTarget();
			Position = FMath::Lerp(LaunchLocation, AscentEnd, FMath::Clamp(Age / AscentSeconds, 0.f, 1.f));
		}
		else if (Phase == ETopAttackRocketPhase::Guidance)
		{
			UpdateTarget();
			UpdateGuidanceDecal();
			const FVector AboveTarget(ImpactLocation.X, ImpactLocation.Y, FMath::Max(AscentEnd.Z, ImpactLocation.Z + AscentHeight));
			Position = FMath::Lerp(AscentEnd, AboveTarget, FMath::Clamp((Age - AscentSeconds) / GuidanceSeconds, 0.f, 1.f));
		}
		else Position = FMath::Lerp(DiveStart, ImpactLocation, FMath::Clamp((Age - AscentSeconds - GuidanceSeconds) / DiveSeconds, 0.f, 1.f));
		const FVector Direction = (Position - GetActorLocation()).GetSafeNormal();
		FHitResult Hit;
		SetActorLocationAndRotation(Position, Direction.IsNearlyZero() ? GetActorRotation() : Direction.Rotation(), true, &Hit);
		if (Hit.bBlockingHit) { Detonate(); break; }
		if (Age >= Boundary - KINDA_SMALL_NUMBER)
		{
			if (Phase == ETopAttackRocketPhase::Ascent) { Phase = ETopAttackRocketPhase::Guidance; UpdateGuidanceDecal(); }
			else if (Phase == ETopAttackRocketPhase::Guidance) { Phase = ETopAttackRocketPhase::Dive; DiveStart = GetActorLocation(); }
			else Detonate();
		}
	}
	const float Pulse = 1.f + 0.15f * FMath::Sin(Age * 75.f);
	Exhaust->SetRelativeScale3D(FVector(0.13f, 0.13f, 0.5f * Pulse));
}

void ATunaSweeperTopAttackRocket::Detonate()
{
	if (Phase == ETopAttackRocketPhase::Detonated) return;
	Phase = ETopAttackRocketPhase::Detonated;
	Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RocketMesh->SetVisibility(false);
	Exhaust->SetVisibility(false);
	GuidanceDecal->SetVisibility(false);
	Blast->SetWorldRotation(FRotator::ZeroRotator);
	Blast->SetWorldScale3D(FVector(0.1f));
	Blast->SetVisibility(true);
	if (HasAuthority() && Damage > 0.f)
	{
		TArray<AActor*> Ignored;
		if (const auto* Factions = GetWorld()->GetSubsystem<UTunaSweeperFactionSubsystem>())
			Factions->GetActorsWithAttitude(this, ETunaSweeperFactionAttitude::Friendly, Ignored);
		Ignored.Add(GetOwner());
		Ignored.Add(GetInstigator());
		UGameplayStatics::ApplyRadialDamage(this, Damage, GetActorLocation(), FMath::Max(1.f, DamageRadius),
			UTunaSweeperGrenadeDamageType::StaticClass(), Ignored, this, GetInstigatorController(), true, ECC_Visibility);
	}
}
