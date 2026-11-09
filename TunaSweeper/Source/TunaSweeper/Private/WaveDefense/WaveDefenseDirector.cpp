#include "WaveDefense/WaveDefenseDirector.h"
#include "WaveDefense/WaveDefenseCore.h"
#include "WaveDefense/WaveDefenseFacility.h"
#include "WaveDefense/WaveDefenseDefinition.h"
#include "WaveDefense/WaveDefenseParticipantComponent.h"
#include "WaveDefense/WaveDefenseSpawnPoint.h"
#include "AI/TunaSweeperEnemyCharacter.h"
#include "AIController.h"
#include "Character/TunaSweeperTopDownCharacter.h"
#include "Component/TunaSweeperFactionTypes.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Game/TunaSweeperGameInstance.h"
#include "GameFramework/PlayerController.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"
#include "Subsystem/TunaSweeperDifficultySubsystem.h"
#include "Subsystem/TunaSweeperEnemySpawnSubsystem.h"
#include "Subsystem/TunaSweeperItemDataSubsystem.h"

AWaveDefenseDirector::AWaveDefenseDirector() { PrimaryActorTick.bCanEverTick = true; }
FWaveDefenseSnapshot AWaveDefenseDirector::GetSnapshot() const
{
    FWaveDefenseSnapshot Snapshot = Run.GetSnapshot();
    if (Core) { Snapshot.CoreHealth = Core->GetHealth(); Snapshot.CoreMaxHealth = Core->GetMaxHealth(); }
    Snapshot.LeaveSecondsRemaining = OutsideSeconds > 0 && Definition ? FMath::Max(0.0f, Definition->LeaveSeconds - OutsideSeconds) : 0.0f;
    return Snapshot;
}
bool AWaveDefenseDirector::TryStart(APawn* Player)
{
    auto* Character = Cast<ATunaSweeperTopDownCharacter>(Player);
    auto* Game = GetGameInstance<UTunaSweeperGameInstance>();
    if (!Character || Character->IsDead() || Character->IsMountedInVehicle() || !Game || !Definition || !Core || RunId.IsValid() || SpawnPoints.Num() < 6) return false;
    TArray<FName> Errors;
    if (!Definition->ValidateDefinition(Errors)) return false;
    auto* Profiles = Game->GetSubsystem<UTunaSweeperEnemySpawnSubsystem>();
    auto* Items = Game->GetSubsystem<UTunaSweeperItemDataSubsystem>();
    auto* Difficulty = Game->GetSubsystem<UTunaSweeperDifficultySubsystem>();
    if (!Profiles || !Items || !Difficulty || !GetWorld() || !UNavigationSystemV1::GetCurrent(GetWorld())) return false;
    for (const FWaveDefenseWave& Wave : Definition->Waves)
    {
        for (const FWaveDefenseEnemyEntry& Entry : Wave.Enemies)
        {
            FTunaSweeperEnemyCombatProfile Profile;
            FTunaSweeperItemDefinition Weapon, Ammo;
            if (!Profiles->TryGetEnemyCombatProfile(Entry.CombatProfile, Profile) || !Entry.EnemyClass.LoadSynchronous()) return false;
            if (Profile.AttackMode == ETunaSweeperEnemyAttackMode::Ranged && (!Items->TryGetItemDefinition(Entry.WeaponId, Weapon) || !Items->TryGetItemDefinition(Entry.AmmoId, Ammo))) return false;
        }
    }
    TMap<FName, int32> Lanes;
    for (const AWaveDefenseSpawnPoint* Point : SpawnPoints)
    {
        if (!IsValid(Point) || Point->LaneId.IsNone()) return false;
        ++Lanes.FindOrAdd(Point->LaneId);
        UNavigationPath* Path = UNavigationSystemV1::FindPathToLocationSynchronously(this, Point->GetActorLocation(), Core->GetActorLocation());
        if (!Path || !Path->IsValid() || Path->IsPartial()) return false;
    }
    TSet<FName> SlotIds;
    for (const auto& Slot : FacilitySlots) { if (Slot.SlotId.IsNone() || Slot.SlotId == TEXT("core") || SlotIds.Contains(Slot.SlotId)) return false; SlotIds.Add(Slot.SlotId); }
    if (FacilitySlots.Num() != 6 || !BarrierClass || !TurretClass) return false;
    if (Lanes.Num() < 3) return false;
    for (const auto& Pair : Lanes) if (Pair.Value < 2) return false;
    for (TActorIterator<ATunaSweeperEnemyCharacter> It(GetWorld()); It; ++It)
        if (!It->IsDead() && FVector::DistSquared2D(It->GetActorLocation(), Core->GetActorLocation()) < FMath::Square(4500.0f)) return false;
    const int32 Stage = Game->GetActiveSaveSlotDifficultyStage();
    FGuid NewRunId;
    if (!Game->TryBeginWaveDefenseRun(Definition->EncounterId, Definition->RewardCoins, Stage, NewRunId)) return false;
    Run = FWaveDefenseRun();
    if (!Run.Start(*Definition, NewRunId)) { Game->TryFinalizeWaveDefenseRun(Definition->EncounterId, NewRunId, EWaveDefenseOutcome::Interrupted, 0); return false; }
    RunId = NewRunId;
    PlayerPawn = Player;
    DamageMultiplier = Difficulty->GetEnemyIncomingDamageMultiplier(Stage);
    Core->SetDirector(this); Core->ResetForRun(RunId, Definition->CoreMaxHealth);
    SpawnDelay = SpawnBlockedSeconds = OutsideSeconds = 0.0f;
    LastCompletedWave = 0;
    return true;
}
void AWaveDefenseDirector::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!RunId.IsValid() || !Definition || !Core) return;
    auto* Character = Cast<ATunaSweeperTopDownCharacter>(PlayerPawn.Get());
    if (!Character) { Abort(EWaveDefenseOutcome::Interrupted); return; }
    if (Character->IsMountedInVehicle()) { Abort(EWaveDefenseOutcome::Abandoned); return; }
    if (Character->IsDead() && Run.GetSnapshot().Phase == EWaveDefensePhase::Resolving && Run.GetSnapshot().Outcome == EWaveDefenseOutcome::Succeeded) { Abort(EWaveDefenseOutcome::Interrupted); return; }
    if (FVector::DistSquared2D(Character->GetActorLocation(), Core->GetActorLocation()) > FMath::Square(4500.0f)) OutsideSeconds += DeltaSeconds; else OutsideSeconds = 0.0f;
    if (OutsideSeconds >= Definition->LeaveSeconds) { Abort(EWaveDefenseOutcome::Abandoned); return; }
    for (const auto& Pair : Enemies)
        if (!Pair.Value.IsValid()) { Abort(EWaveDefenseOutcome::Faulted); return; }
    if (!UpdateEnemyProgress(DeltaSeconds)) { Abort(EWaveDefenseOutcome::Faulted); return; }
    Run.AdvanceTime(DeltaSeconds);
    FWaveDefenseSnapshot Before = Run.GetSnapshot();
    if (Before.Phase == EWaveDefensePhase::Active && Before.Queued > 0)
    {
        SpawnDelay -= DeltaSeconds;
        if (SpawnDelay <= 0.0f)
        {
            const int32 Cap = Definition->Waves[Before.Wave - 1].AliveCap;
            if (Before.Alive < Cap)
            {
                int32 Spawned = 0;
                while (Spawned < 2 && TrySpawnOne()) ++Spawned;
                if (Spawned) { SpawnBlockedSeconds = 0; SpawnDelay = Definition->SpawnIntervalSeconds; }
                else { SpawnBlockedSeconds += Definition->SpawnRetrySeconds; SpawnDelay = Definition->SpawnRetrySeconds; }
                if (SpawnBlockedSeconds >= Definition->SpawnFailureSeconds) { Abort(EWaveDefenseOutcome::Faulted); return; }
            }
        }
    }
    Run.ResolveFrame(Character->IsDead(), Core->GetHealth() <= 0);
    FWaveDefenseSnapshot After = Run.GetSnapshot();
    if (Before.Phase == EWaveDefensePhase::Active && After.Phase == EWaveDefensePhase::Intermission) LastCompletedWave = Before.Wave;
    if (After.Phase == EWaveDefensePhase::Resolving)
    {
        if (After.Outcome == EWaveDefenseOutcome::Succeeded) LastCompletedWave = After.TotalWaves;
        auto* Game = GetGameInstance<UTunaSweeperGameInstance>();
        if (Game && Game->TryFinalizeWaveDefenseRun(Definition->EncounterId, RunId, After.Outcome, LastCompletedWave))
        { CleanupOwnedActors(); RunId.Invalidate(); PlayerPawn.Reset(); }
    }
}
bool AWaveDefenseDirector::FindSpawnTransform(const AWaveDefenseSpawnPoint* Point, UClass* EnemyClass, FTransform& OutTransform) const
{
    if (!Point || !EnemyClass || !PlayerPawn.IsValid()) return false;
    auto* Nav = UNavigationSystemV1::GetCurrent(GetWorld());
    auto* DefaultEnemy = Cast<ATunaSweeperEnemyCharacter>(EnemyClass->GetDefaultObject());
    if (!Nav || !DefaultEnemy || !DefaultEnemy->GetCapsuleComponent()) return false;
    FNavLocation Projected;
    if (!Nav->ProjectPointToNavigation(Point->GetActorLocation(), Projected, FVector(150,150,300))) return false;
    const float Radius = DefaultEnemy->GetCapsuleComponent()->GetScaledCapsuleRadius();
    const float HalfHeight = DefaultEnemy->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
    const FVector Location = Projected.Location + FVector(0,0,HalfHeight);
    if (FVector::DistSquared2D(Location, PlayerPawn->GetActorLocation()) < FMath::Square(1500.0f)) return false;
    FCollisionQueryParams Query(SCENE_QUERY_STAT(WaveDefenseSpawn), false, this);
    if (GetWorld()->OverlapBlockingTestByChannel(Location, FQuat::Identity, ECC_Pawn, FCollisionShape::MakeCapsule(Radius,HalfHeight), Query)) return false;
    UNavigationPath* Path = UNavigationSystemV1::FindPathToLocationSynchronously(GetWorld(), Projected.Location, Core->GetActorLocation());
    if (!Path || !Path->IsValid() || Path->IsPartial()) return false;
    if (APlayerController* Controller = Cast<APlayerController>(PlayerPawn->GetController()))
    {
        FVector2D Screen; int32 Width = 0, Height = 0;
        Controller->GetViewportSize(Width, Height);
        if (Controller->ProjectWorldLocationToScreen(Location, Screen) && Screen.X >= 0 && Screen.Y >= 0 && Screen.X <= Width && Screen.Y <= Height) return false;
    }
    OutTransform = FTransform(Point->GetActorRotation(), Location);
    return true;
}
bool AWaveDefenseDirector::TrySpawnOne()
{
    const FWaveDefenseSnapshot Snapshot = Run.GetSnapshot();
    if (Snapshot.Phase != EWaveDefensePhase::Active || Snapshot.Queued <= 0 || Snapshot.Wave <= 0) return false;
    const FWaveDefenseWave& Wave = Definition->Waves[Snapshot.Wave - 1];
    if (Snapshot.Alive >= Wave.AliveCap) return false;
    int32 Index = Wave.GetCount() - Snapshot.Queued;
    const FWaveDefenseEnemyEntry* Entry = nullptr;
    for (const auto& Candidate : Wave.Enemies) { if (Index < Candidate.Count) { Entry = &Candidate; break; } Index -= Candidate.Count; }
    if (!Entry) return false;
    UClass* EnemyClass = Entry->EnemyClass.LoadSynchronous();
    auto* Game = GetGameInstance<UTunaSweeperGameInstance>();
    auto* Profiles = Game ? Game->GetSubsystem<UTunaSweeperEnemySpawnSubsystem>() : nullptr;
    FTunaSweeperEnemyCombatProfile Profile;
    if (!EnemyClass || !Profiles || !Profiles->TryGetEnemyCombatProfile(Entry->CombatProfile, Profile)) return false;
    for (const AWaveDefenseSpawnPoint* Point : SpawnPoints)
    {
        if (!Wave.Lanes.IsEmpty() && !Wave.Lanes.Contains(Point->LaneId)) continue;
        FTransform Transform;
        if (!FindSpawnTransform(Point, EnemyClass, Transform)) continue;
        ATunaSweeperEnemyCharacter* Enemy = GetWorld()->SpawnActorDeferred<ATunaSweeperEnemyCharacter>(EnemyClass, Transform, this, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
        if (!Enemy) continue;
        Enemy->ConfigureCombatProfile(Profile, TunaSweeperFactionIds::Enemy, NAME_None, INDEX_NONE);
        Enemy->ConfigureSpawnData({}, NAME_None, INDEX_NONE, INDEX_NONE, Entry->BaseHealth, 0, 0, 0, Entry->WeaponId, Entry->AmmoId, Entry->ReserveAmmo);
        const FGuid Token = FGuid::NewGuid();
        UWaveDefenseParticipantComponent* Participant = NewObject<UWaveDefenseParticipantComponent>(Enemy);
        Participant->Initialize(this, RunId, Token, Entry->Role); Participant->RegisterComponent();
        Enemies.Add(Token, Enemy); OwnedEnemies.Add(Enemy);
        FEnemyProgress& Progress = EnemyProgress.Add(Token); Progress.LastLocation = Transform.GetLocation();
        Enemy->FinishSpawning(Transform);
        if (!IsValid(Enemy) || !Run.NotifySpawned(Token)) { Enemies.Remove(Token); if (IsValid(Enemy)) Enemy->Destroy(); return false; }
        return true;
    }
    return false;
}
bool AWaveDefenseDirector::UpdateEnemyProgress(float DeltaSeconds)
{
    if (!Definition || !PlayerPawn.IsValid()) return true;
    for (const auto& Pair : Enemies)
    {
        ATunaSweeperEnemyCharacter* Enemy = Pair.Value.Get();
        FEnemyProgress* Progress = EnemyProgress.Find(Pair.Key);
        if (!Enemy || !Progress) return false;
        const FVector Location = Enemy->GetActorLocation();
        const auto* Participant = Enemy->FindComponentByClass<UWaveDefenseParticipantComponent>();
        AActor* Target = Participant ? Participant->GetPreferredTarget() : nullptr;
        AAIController* Controller = Cast<AAIController>(Enemy->GetController());
        const float EngagementRange = Enemy->UsesMeleeAttack() ? Enemy->GetMeleeAttackRange() + 100.0f : 1200.0f;
        const bool bEngaged = Target && Controller && FVector::DistSquared2D(Location, Target->GetActorLocation()) <= FMath::Square(EngagementRange) && Controller->LineOfSightTo(Target);
        if (bEngaged || FVector::DistSquared2D(Location, Progress->LastLocation) >= FMath::Square(50.0f))
        { Progress->LastLocation = Location; Progress->IdleSeconds = 0; Progress->bRepathRequested = false; continue; }
        Progress->IdleSeconds += DeltaSeconds;
        if (!Progress->bRepathRequested && Progress->IdleSeconds >= Definition->StuckSeconds)
        {
            if (Controller && Target) Controller->MoveToActor(Target, 120.0f);
            Progress->bRepathRequested = true;
        }
        if (Progress->IdleSeconds >= Definition->StuckSeconds + Definition->RepathSeconds)
        {
            if (auto* PC = Cast<APlayerController>(PlayerPawn->GetController()))
            {
                FVector2D Screen; int32 Width = 0, Height = 0; PC->GetViewportSize(Width, Height);
                if (PC->ProjectWorldLocationToScreen(Location, Screen) && Screen.X >= 0 && Screen.Y >= 0 && Screen.X <= Width && Screen.Y <= Height) return false;
            }
            bool bMoved = false;
            for (const AWaveDefenseSpawnPoint* Point : SpawnPoints)
            {
                FTransform Transform;
                if (FindSpawnTransform(Point, Enemy->GetClass(), Transform) && Enemy->SetActorLocation(Transform.GetLocation(), false, nullptr, ETeleportType::TeleportPhysics))
                { bMoved = true; Progress->LastLocation = Transform.GetLocation(); Progress->IdleSeconds = 0; Progress->bRepathRequested = false; break; }
            }
            if (!bMoved) return false;
        }
    }
    return true;
}bool AWaveDefenseDirector::SetReady()
{
    const FWaveDefenseSnapshot Snapshot = Run.GetSnapshot();
    if (Snapshot.Phase != EWaveDefensePhase::Preparing && Snapshot.Phase != EWaveDefensePhase::Intermission) return false;
    Run.AdvanceTime(Snapshot.SecondsRemaining);
    return true;
}
void AWaveDefenseDirector::NotifyEnemyDied(FGuid EventRunId, FGuid EnemyToken)
{
    if (EventRunId == RunId && Enemies.Contains(EnemyToken) && Run.NotifyEnemyDied(EnemyToken)) { Enemies.Remove(EnemyToken); EnemyProgress.Remove(EnemyToken); }
}
void AWaveDefenseDirector::CleanupOwnedActors()
{
    TGuardValue<bool> Guard(bCleaningUp, true);
    for (auto& Actor : OwnedActors) if (Actor.IsValid()) Actor->Destroy();
    OwnedActors.Reset(); Facilities.Reset();
    for (auto& Enemy : OwnedEnemies) if (Enemy.IsValid()) { if (AController* Controller = Enemy->GetController()) Controller->Destroy(); Enemy->Destroy(); }
    OwnedEnemies.Reset(); Enemies.Reset(); EnemyProgress.Reset();
}
void AWaveDefenseDirector::Abort(EWaveDefenseOutcome Reason)
{
    if (!RunId.IsValid() || !Definition) return;
    Run.Abort(Reason);
    if (auto* Game = GetGameInstance<UTunaSweeperGameInstance>()) Game->TryFinalizeWaveDefenseRun(Definition->EncounterId, RunId, Reason, LastCompletedWave);
    CleanupOwnedActors(); RunId.Invalidate(); PlayerPawn.Reset();
}
void AWaveDefenseDirector::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Abort(EWaveDefenseOutcome::Interrupted);
    Super::EndPlay(EndPlayReason);
}
bool AWaveDefenseDirector::IsActiveParticipant(FGuid EventRunId, FGuid EnemyToken, const AActor* ExpectedEnemy) const
{
    const auto* Enemy = Enemies.Find(EnemyToken);
    return Run.GetSnapshot().Phase == EWaveDefensePhase::Active && EventRunId.IsValid() && EventRunId == RunId && Enemy && Enemy->IsValid() && (!ExpectedEnemy || Enemy->Get() == ExpectedEnemy) && !(*Enemy)->IsDead();
}
AActor* AWaveDefenseDirector::GetPreferredTarget(const AActor* Enemy, FGuid EventRunId, FGuid EnemyToken, EWaveDefenseEnemyRole EnemyRole) const
{
    if (!Enemy || !IsActiveParticipant(EventRunId, EnemyToken, Enemy)) return nullptr;
    APawn* Player = PlayerPawn.Get();
    const UWaveDefenseParticipantComponent* Participant = Enemy->FindComponentByClass<UWaveDefenseParticipantComponent>();
    if (Player && (EnemyRole == EWaveDefenseEnemyRole::PlayerPressure || FVector::DistSquared2D(Player->GetActorLocation(), Enemy->GetActorLocation()) <= FMath::Square(800.0f) || (Participant && Participant->WasRecentlyHitBy(Player)))) return Player;
    return Core;
}
bool AWaveDefenseDirector::OwnsCore(const AActor* Actor) const { return Actor && Actor == Core; }
bool AWaveDefenseDirector::OwnsFacility(const AActor* Actor) const
{
    for (const auto& Pair : Facilities) if (Pair.Value.Get() == Actor) return true;
    return false;
}
void AWaveDefenseDirector::RegisterOwnedActor(AActor* Actor) { if (IsValid(Actor)) OwnedActors.Add(Actor); }
void AWaveDefenseDirector::GetLivingEnemies(TArray<ATunaSweeperEnemyCharacter*>& OutEnemies) const
{
    OutEnemies.Reset();
    for (const auto& Pair : Enemies) if (Pair.Value.IsValid() && !Pair.Value->IsDead()) OutEnemies.Add(Pair.Value.Get());
}
bool AWaveDefenseDirector::TryFacilityAction(FName SlotId, EWaveDefenseAction Action)
{
    if (!RunId.IsValid() || !Definition || !Core) return false;
    const auto Phase = Run.GetSnapshot().Phase;
    if (Phase != EWaveDefensePhase::Preparing && Phase != EWaveDefensePhase::Intermission) return false;
    if (Action == EWaveDefenseAction::RepairCore)
    {
        if (SlotId != TEXT("core") || Core->GetHealth() <= 0 || Core->GetHealth() >= Core->GetMaxHealth() || !Run.TrySpendSupplies(Definition->CoreRepairCost)) return false;
        return Core->Repair(Definition->CoreRepairAmount);
    }
    const FWaveDefenseFacilitySlot* Slot = FacilitySlots.FindByPredicate([SlotId](const FWaveDefenseFacilitySlot& Candidate) { return Candidate.SlotId == SlotId; });
    if (!Slot) return false;
    AWaveDefenseFacility* Existing = Facilities.FindRef(SlotId).Get();
    if (Action == EWaveDefenseAction::RepairFacility)
    {
        if (!Existing || !Existing->CanRepair() || !Run.TrySpendSupplies(Definition->FacilityRepairCost)) return false;
        return Existing->Repair();
    }
    if (Action == EWaveDefenseAction::ReloadTurret)
    {
        if (!Existing || !Existing->CanReload() || !Run.TrySpendSupplies(Definition->ReloadCost)) return false;
        return Existing->Reload();
    }
    const bool bTurret = Action == EWaveDefenseAction::InstallTurret;
    if (Action != EWaveDefenseAction::InstallBarrier && !bTurret) return false;
    if (Existing || Slot->Kind != (bTurret ? EWaveDefenseFacilityKind::Turret : EWaveDefenseFacilityKind::Barrier)) return false;
    int32 Turrets = 0;
    for (const auto& Pair : Facilities) if (Pair.Value.IsValid() && Pair.Value->GetKind() == EWaveDefenseFacilityKind::Turret) ++Turrets;
    if (bTurret && Turrets >= Definition->TurretLimit) return false;
    const int32 Cost = bTurret ? Definition->TurretCost : Definition->BarrierCost;
    if (Cost <= 0 || Run.GetSnapshot().Supplies < Cost) return false;
    TSubclassOf<AWaveDefenseFacility> Class = bTurret ? TurretClass : BarrierClass;
    if (!Class) return false;
    const FTransform Transform = Slot->LocalTransform * GetActorTransform();
    AWaveDefenseFacility* Facility = GetWorld()->SpawnActorDeferred<AWaveDefenseFacility>(Class, Transform, this, nullptr, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding);
    if (!Facility) return false;
    Facility->InitializeForRun(this, RunId, SlotId, Slot->Kind); Facility->Configure(*Definition); Facility->FinishSpawning(Transform);
    if (!IsValid(Facility)) return false;
    if (!Run.TrySpendSupplies(Cost)) { Facility->Destroy(); return false; }
    Facilities.Add(SlotId, Facility); RegisterOwnedActor(Facility);
    return true;
}
bool AWaveDefenseDirector::TryClaimReward()
{
    auto* Game = GetGameInstance<UTunaSweeperGameInstance>();
    if (!Game || !Definition) return false;
    const FWaveDefenseProgress* Record = Game->FindWaveDefenseProgress(Definition->EncounterId);
    return Record && Game->TryClaimWaveDefenseReward(Definition->EncounterId, Record->RunId);
}
int32 AWaveDefenseDirector::GetDifficultyStage() const
{
    const auto* Game = GetGameInstance<UTunaSweeperGameInstance>();
    return Game ? Game->GetActiveSaveSlotDifficultyStage() : 0;
}
int32 AWaveDefenseDirector::GetRewardCoins() const { return Definition ? Definition->RewardCoins : 0; }
