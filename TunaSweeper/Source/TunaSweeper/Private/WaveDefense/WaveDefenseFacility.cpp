#include "WaveDefense/WaveDefenseFacility.h"
#include "WaveDefense/WaveDefenseDirector.h"
#include "WaveDefense/WaveDefenseCore.h"
#include "WaveDefense/WaveDefenseDefinition.h"
#include "WaveDefense/WaveDefenseParticipantComponent.h"
#include "AI/TunaSweeperEnemyCharacter.h"
#include "Component/TunaSweeperFactionComponent.h"
#include "Subsystem/TunaSweeperFactionSubsystem.h"
#include "Weapon/TunaSweeperProjectile.h"
#include "Engine/World.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
AWaveDefenseFacility::AWaveDefenseFacility()
{
    PrimaryActorTick.bCanEverTick = true;
    VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FacilityMesh")); SetRootComponent(VisualMesh);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (Cube.Succeeded()) VisualMesh->SetStaticMesh(Cube.Object); VisualMesh->SetCollisionProfileName(TEXT("BlockAll"));
    FactionComponent = CreateDefaultSubobject<UTunaSweeperFactionComponent>(TEXT("Faction"));
    FactionComponent->SetFactionId(TunaSweeperFactionIds::Player);
    ProjectileClass = TSoftClassPtr<ATunaSweeperProjectile>(FSoftObjectPath(TEXT("/Game/Weapons/BP_TunaSweeperProjectile.BP_TunaSweeperProjectile_C")));
}
void AWaveDefenseFacility::InitializeForRun(AWaveDefenseDirector* InDirector, FGuid InRunId, FName InSlotId, EWaveDefenseFacilityKind InKind)
{
    Director = InDirector; RunId = InRunId; SlotId = InSlotId; Kind = InKind;
    VisualMesh->SetRelativeScale3D(Kind == EWaveDefenseFacilityKind::Barrier ? FVector(0.4f, 3.0f, 1.5f) : FVector(0.8f, 0.8f, 1.0f));
}
void AWaveDefenseFacility::Configure(const UWaveDefenseDefinition& Definition)
{
    Health = MaxHealth = Kind == EWaveDefenseFacilityKind::Barrier ? Definition.BarrierHealth : Definition.TurretHealth;
    Ammo = MaxAmmo = Definition.TurretAmmo; Range = Definition.TurretRange; Interval = Definition.TurretInterval; Damage = Definition.TurretDamage;
}
bool AWaveDefenseFacility::CanDamageTarget(const AActor* Target) const
{
    const auto* Participant = Target ? Target->FindComponentByClass<UWaveDefenseParticipantComponent>() : nullptr;
    return Director && Participant && Participant->GetRunId() == RunId && Director->IsActiveParticipant(RunId, Participant->GetToken(), Target);
}
void AWaveDefenseFacility::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (Kind != EWaveDefenseFacilityKind::Turret || Health <= 0 || Ammo <= 0 || !Director || !GetWorld()) return;
    Cooldown -= DeltaSeconds;
    if (Cooldown > 0) return;
    TArray<ATunaSweeperEnemyCharacter*> Targets; Director->GetLivingEnemies(Targets);
    Targets.Sort([this](const ATunaSweeperEnemyCharacter& A, const ATunaSweeperEnemyCharacter& B) { return FVector::DistSquared(GetActorLocation(), A.GetActorLocation()) < FVector::DistSquared(GetActorLocation(), B.GetActorLocation()); });
    for (ATunaSweeperEnemyCharacter* Target : Targets)
    {
        if (!CanDamageTarget(Target) || FVector::DistSquared(GetActorLocation(), Target->GetActorLocation()) > FMath::Square(Range)) continue;
        const FVector Origin = GetActorLocation() + FVector(0,0,100), Destination = Target->GetActorLocation();
        FHitResult Hit; FCollisionQueryParams Query(SCENE_QUERY_STAT(WaveDefenseTurret), false, this);
        if (GetWorld()->LineTraceSingleByChannel(Hit, Origin, Destination, ECC_Visibility, Query) && Hit.GetActor() != Target) continue;
        UClass* Class = ProjectileClass.LoadSynchronous();
        if (!Class) return;
        const FTransform Transform((Destination - Origin).Rotation(), Origin);
        ATunaSweeperProjectile* Projectile = GetWorld()->SpawnActorDeferred<ATunaSweeperProjectile>(Class, Transform, this, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
        if (!Projectile) return;
        Projectile->SetDamageAmount(Damage); Projectile->IgnoreActor(this); Projectile->FinishSpawning(Transform);
        if (!IsValid(Projectile)) return;
        Director->RegisterOwnedActor(Projectile); --Ammo; Cooldown = Interval; return;
    }
}
float AWaveDefenseFacility::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
    if (!Director || !GetWorld() || Health <= 0) return 0.0f;
    const AActor* Source = EventInstigator && EventInstigator->GetPawn() ? EventInstigator->GetPawn() : DamageCauser;
    const auto* Participant = Source ? Source->FindComponentByClass<UWaveDefenseParticipantComponent>() : nullptr;
    if (!Participant && DamageCauser && DamageCauser->GetInstigator()) Participant = DamageCauser->GetInstigator()->FindComponentByClass<UWaveDefenseParticipantComponent>();
    const auto* Factions = GetWorld()->GetSubsystem<UTunaSweeperFactionSubsystem>();
    const bool bHostile = Factions && Source && Factions->AreActorsHostile(Source, this);
    const float Applied = Participant ? FWaveDefenseDamageRules::Resolve(DamageAmount, Director->GetDamageMultiplier(), RunId, Participant->GetRunId(), bHostile, Director->IsActiveParticipant(Participant->GetRunId(), Participant->GetToken(), Participant->GetOwner())) : 0.0f;
    const float Clamped = FMath::Min<float>(Health, Applied); Health -= FMath::RoundToInt(Clamped);
    if (Health <= 0) Destroy();
    return Clamped;
}
bool AWaveDefenseFacility::Repair() { if (!CanRepair()) return false; Health = FMath::Min(MaxHealth, Health + FMath::RoundToInt(MaxHealth * 0.5f)); return true; }
bool AWaveDefenseFacility::Reload() { if (!CanReload()) return false; Ammo = MaxAmmo; return true; }
