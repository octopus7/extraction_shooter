#include "TunaGuidedRocket.h"
#include "TunaRocketFlight.h"
#include "TunaRocketEffect.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/DamageType.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"

ATunaGuidedRocket::ATunaGuidedRocket()
{
    PrimaryActorTick.bCanEverTick = true;
    Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
    SetRootComponent(Collision);
    Collision->InitSphereRadius(6);
    Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Collision->SetCollisionObjectType(ECC_WorldDynamic);
    Collision->SetCollisionResponseToAllChannels(ECR_Ignore);
    Collision->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
    Collision->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
    Collision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
    Collision->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Block);
    Collision->SetGenerateOverlapEvents(false);
    Collision->SetCanEverAffectNavigation(false);
    RocketMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RocketMesh"));
    RocketMesh->SetupAttachment(Collision);
    RocketMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    RocketMesh->SetCanEverAffectNavigation(false);
    ExhaustMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ExhaustMesh"));
    ExhaustMesh->SetupAttachment(RocketMesh);
    ExhaustMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ExhaustMesh->SetCastShadow(false);
    ExhaustMesh->SetCanEverAffectNavigation(false);
    TrailNiagara = CreateDefaultSubobject<UNiagaraComponent>(TEXT("TrailNiagara"));
    TrailNiagara->SetupAttachment(RocketMesh);
    TrailNiagara->SetAutoActivate(false);
}

void ATunaGuidedRocket::BeginPlay()
{
    Super::BeginPlay();
    Active = Configuration ? Configuration->Settings : FTunaRocketSettings();
    Active.Normalize();
    Direction = GetActorForwardVector();
    ActiveTarget = TargetActor;
    AimOffset = TargetOffset.ContainsNaN() ? FVector::ZeroVector : TargetOffset;
    TargetActor = nullptr; // Flight holds a weak reference and never retargets.
    Collision->SetSphereRadius(Active.CollisionRadius);
    Collision->IgnoreActorWhenMoving(GetOwner(), true);
    Collision->IgnoreActorWhenMoving(GetInstigator(), true);
    if (TrailNiagara->GetAsset()) TrailNiagara->Activate(true);
}

FVector ATunaGuidedRocket::GetVelocity() const
{
    return bDetonated ? FVector::ZeroVector : Direction * Active.Speed;
}

void ATunaGuidedRocket::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (bDetonated || !FMath::IsFinite(DeltaSeconds) || DeltaSeconds <= 0) return;
    float Remaining = FMath::Min(DeltaSeconds, Active.Lifetime - Age);
    while (Remaining > SMALL_NUMBER && !bDetonated)
    {
        float Step = FMath::Min(Remaining, 1.f / 120.f);
        // Split at guidance boundaries as well as fuse expiry.
        if (Age < Active.GuidanceDelay) Step = FMath::Min(Step, Active.GuidanceDelay - Age);
        else if (Age < Active.GuidanceDelay + Active.GuidanceDuration) Step = FMath::Min(Step, Active.GuidanceDelay + Active.GuidanceDuration - Age);
        if (Age >= Active.GuidanceDelay && !bGuidanceLost)
        {
            AActor* Target = ActiveTarget.Get();
            const FVector ToTarget = Target ? Target->GetActorLocation() + AimOffset - GetActorLocation() : FVector::ZeroVector;
            if (Target && TunaRocketFlight::CanGuide(Age, Active.GuidanceDelay, Active.GuidanceDuration, false, Direction, ToTarget, Active.GuidanceConeHalfAngle))
                Direction = TunaRocketFlight::Turn(Direction, ToTarget, Active.TurnRate, Step);
            else bGuidanceLost = true;
        }
        FHitResult Hit;
        SetActorLocationAndRotation(GetActorLocation() + Direction * Active.Speed * Step, Direction.Rotation(), true, &Hit);
        Age += Step;
        Remaining -= Step;
        if (Hit.bBlockingHit) { Detonate(ETunaRocketDetonation::Impact); return; }
    }
    if (Age >= Active.Lifetime - KINDA_SMALL_NUMBER) { Detonate(ETunaRocketDetonation::Fuse); return; }
    TrailCountdown -= DeltaSeconds;
    // Emit at most once per rendered frame; a hitch must not create a burst of old particles.
    if (bSimpleTrail && TrailCountdown <= 0) { SpawnSimpleEffect(false); TrailCountdown = Active.TrailInterval; }
}

void ATunaGuidedRocket::SpawnSimpleEffect(bool bExplosion)
{
    if (!SimpleEffectClass || GetNetMode() == NM_DedicatedServer) return;
    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    const FVector Location = bExplosion ? GetActorLocation() : ExhaustMesh->GetComponentLocation();
    if (auto* Effect = GetWorld()->SpawnActor<ATunaRocketEffect>(SimpleEffectClass, Location, FRotator::ZeroRotator, Params))
        Effect->Initialize(bExplosion, bExplosion ? Active.ExplosionLifetime : Active.TrailLifetime, bExplosion ? Active.ExplosionVisualRadius : Active.TrailSize);
}

bool ATunaGuidedRocket::CanDamageActor_Implementation(AActor* Candidate) const
{
    return IsValid(Candidate) && Candidate != this && Candidate != GetOwner() && Candidate != GetInstigator() && Candidate->CanBeDamaged();
}

void ATunaGuidedRocket::ApplyExplosionDamage()
{
    if (!HasAuthority() || Active.Damage <= 0 || Active.DamageRadius <= 0) return;
    TArray<FOverlapResult> Hits;
    FCollisionObjectQueryParams Objects;
    Objects.AddObjectTypesToQuery(ECC_Pawn);
    FCollisionQueryParams Query(SCENE_QUERY_STAT(TunaRocketBlast), false, this);
    Query.AddIgnoredActor(GetOwner());
    Query.AddIgnoredActor(GetInstigator());
    GetWorld()->OverlapMultiByObjectType(Hits, GetActorLocation(), FQuat::Identity, Objects, FCollisionShape::MakeSphere(Active.DamageRadius), Query);
    TSet<AActor*> Visited;
    for (const FOverlapResult& Hit : Hits)
    {
        AActor* Victim = Hit.GetActor();
        if (!IsValid(Victim) || Visited.Contains(Victim)) continue;
        Visited.Add(Victim);
        if (!CanDamageActor(Victim)) continue;
        FHitResult Cover;
        // Respect blocking responses: overlap-only trigger volumes are not cover.
        if (GetWorld()->LineTraceSingleByChannel(Cover, GetActorLocation(), Victim->GetActorLocation(), DamageCoverChannel, Query) && Cover.GetActor() != Victim) continue;
        UGameplayStatics::ApplyDamage(Victim, Active.Damage, GetInstigatorController(), this, UDamageType::StaticClass());
    }
}

void ATunaGuidedRocket::Detonate(ETunaRocketDetonation Reason)
{
    if (bDetonated) return;
    bDetonated = true;
    SetActorTickEnabled(false);
    Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    TrailNiagara->DeactivateImmediate();
    if (GetNetMode() != NM_DedicatedServer)
    {
        if (bSimpleExplosion) SpawnSimpleEffect(true);
        if (ExplosionNiagara) UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, ExplosionNiagara, GetActorLocation());
        if (ExplosionSound) UGameplayStatics::PlaySoundAtLocation(this, ExplosionSound, GetActorLocation());
    }
    ApplyExplosionDamage();
    OnDetonated.Broadcast(Reason, GetActorLocation());
    Destroy();
}

void ATunaGuidedRocket::EndPlay(const EEndPlayReason::Type Reason)
{
    TrailNiagara->DeactivateImmediate();
    Super::EndPlay(Reason);
}
