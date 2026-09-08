#include "Combat/TunaSweeperBossEncounter.h"

#include "AI/TunaSweeperEnemyCharacter.h"
#include "Character/TunaSweeperTopDownCharacter.h"
#include "Component/TunaSweeperCombatPatternComponent.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

DEFINE_LOG_CATEGORY_STATIC(LogTunaBossEncounter, Log, All);

ATunaSweeperBossEncounter::ATunaSweeperBossEncounter()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;
	CombatBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("CombatBounds"));
	SetRootComponent(CombatBounds);
	CombatBounds->InitBoxExtent(FVector(1800.0f, 1800.0f, 400.0f));
	CombatBounds->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CombatBounds->SetCollisionObjectType(ECC_WorldDynamic);
	CombatBounds->SetCollisionResponseToAllChannels(ECR_Ignore);
	CombatBounds->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	CombatBounds->SetGenerateOverlapEvents(true);
	CombatBounds->SetCanEverAffectNavigation(false);
	CombatBounds->SetHiddenInGame(true);
	DisplayName = NSLOCTEXT("TunaSweeper", "BossEncounterDefaultName", "Boss encounter");
}

void ATunaSweeperBossEncounter::BeginPlay()
{
	Super::BeginPlay();
	CombatBounds->OnComponentBeginOverlap.AddDynamic(this, &ATunaSweeperBossEncounter::OnCombatBeginOverlap);
	CombatBounds->OnComponentEndOverlap.AddDynamic(this, &ATunaSweeperBossEncounter::OnCombatEndOverlap);
	ActorSpawnedHandle = GetWorld()->AddOnActorSpawnedHandler(
		FOnActorSpawned::FDelegate::CreateUObject(this, &ATunaSweeperBossEncounter::TrackSpawnedActor));
	UpdateOccupancy();
}

void ATunaSweeperBossEncounter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateOccupancy();
	for (auto It = EncounterActors.CreateIterator(); It; ++It)
	{
		if (!It->IsValid()) { It.RemoveCurrent(); }
	}
}

bool ATunaSweeperBossEncounter::ContainsWorldPoint(FVector WorldPoint) const
{
	if (!CombatBounds) { return false; }
	const FVector Local = CombatBounds->GetComponentTransform().InverseTransformPosition(WorldPoint);
	const FVector Extent = CombatBounds->GetUnscaledBoxExtent();
	return FMath::Abs(Local.X) <= Extent.X && FMath::Abs(Local.Y) <= Extent.Y && FMath::Abs(Local.Z) <= Extent.Z;
}

bool ATunaSweeperBossEncounter::IsLivingPlayer(const APawn* Pawn) const
{
	if (!IsValid(Pawn) || Pawn->IsActorBeingDestroyed() || !Pawn->IsPlayerControlled()) { return false; }
	if (const ATunaSweeperEnemyCharacter* Enemy = Cast<ATunaSweeperEnemyCharacter>(Pawn); Enemy && Enemy->IsDead()) { return false; }
	const ATunaSweeperTopDownCharacter* Player = Cast<ATunaSweeperTopDownCharacter>(Pawn);
	return !Player || !Player->IsDead();
}

APawn* ATunaSweeperBossEncounter::FindPlayerInside() const
{
	const UWorld* World = GetWorld();
	if (!World) { return nullptr; }
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		const APlayerController* Controller = It->Get();
		APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
		if (IsLivingPlayer(Pawn) && ContainsWorldPoint(Pawn->GetActorLocation())) { return Pawn; }
	}
	return nullptr;
}

bool ATunaSweeperBossEncounter::IsPlayerInside() const
{
	return FindPlayerInside() != nullptr;
}

ATunaSweeperEnemyCharacter* ATunaSweeperBossEncounter::GetBoss() const
{
	return Boss.IsValid() && !Boss->IsActorBeingDestroyed() ? Boss.Get() : nullptr;
}

void ATunaSweeperBossEncounter::UpdateOccupancy()
{
	if (bCleaningUp || bUpdatingOccupancy || !HasActorBegunPlay()) { return; }
	TGuardValue<bool> UpdateGuard(bUpdatingOccupancy, true);
	APawn* Player = FindPlayerInside();
	if (!Player)
	{
		if (State != ETunaSweeperBossEncounterState::Ready || Occupant.IsValid() || !EncounterActors.IsEmpty())
		{
			CleanupEncounterActors();
			SetState(ETunaSweeperBossEncounterState::Ready, TEXT("Player left, died, or became unavailable"));
		}
		Occupant.Reset();
		bAwaitingExit = false;
		return;
	}

	// A newly possessed pawn must not inherit another pawn's fight.
	if (Occupant.IsValid() && Occupant.Get() != Player)
	{
		CleanupEncounterActors();
		SetState(ETunaSweeperBossEncounterState::Ready, TEXT("Player pawn changed"));
		bAwaitingExit = true;
	}
	Occupant = Player;
	if (State == ETunaSweeperBossEncounterState::Active)
	{
		ATunaSweeperEnemyCharacter* CurrentBoss = Boss.Get();
		if (IsValid(CurrentBoss) && CurrentBoss->IsDead()) { ClearEncounter(); }
		else if (!IsValid(CurrentBoss) || CurrentBoss->IsActorBeingDestroyed())
		{
			CleanupEncounterActors();
			SetState(ETunaSweeperBossEncounterState::Ready, TEXT("Boss became unavailable"));
			bAwaitingExit = true;
		}
	}
	else if (State == ETunaSweeperBossEncounterState::Ready && !bAwaitingExit)
	{
		StartEncounter(Player);
	}
}

void ATunaSweeperBossEncounter::StartEncounter(APawn* Player)
{
	bAwaitingExit = true;
	if (!BossClass || BossClass->HasAnyClassFlags(CLASS_Abstract) || !IsLivingPlayer(Player))
	{
		UE_LOG(LogTunaBossEncounter, Warning, TEXT("Encounter %s cannot start: missing/abstract boss class or unavailable player."), *EncounterId.ToString());
		return;
	}
	const FVector Location = GetActorTransform().TransformPosition(BossSpawnOffset);
	if (!ContainsWorldPoint(Location))
	{
		UE_LOG(LogTunaBossEncounter, Warning, TEXT("Encounter %s cannot start: boss spawn is outside CombatBounds."), *EncounterId.ToString());
		return;
	}
	const FQuat Rotation = GetActorQuat() * BossSpawnRotation.Quaternion();
	FActorSpawnParameters Parameters;
	Parameters.Owner = this;
	Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::DontSpawnIfColliding;
	ATunaSweeperEnemyCharacter* SpawnedBoss = GetWorld()->SpawnActor<ATunaSweeperEnemyCharacter>(
		BossClass, Location, Rotation.Rotator(), Parameters);
	if (!IsValid(SpawnedBoss))
	{
		CleanupEncounterActors();
		UE_LOG(LogTunaBossEncounter, Warning, TEXT("Encounter %s cannot start: boss spawn is blocked. Leave and reenter to retry."), *EncounterId.ToString());
		return;
	}
	Boss = SpawnedBoss;
	EncounterActors.Add(SpawnedBoss);
	SpawnedBoss->OnDestroyed.AddDynamic(this, &ATunaSweeperBossEncounter::OnBossDestroyed);
	SetState(ETunaSweeperBossEncounterState::Active, TEXT("Player crossed combat boundary"));
}

bool ATunaSweeperBossEncounter::BelongsToEncounter(const AActor* Actor) const
{
	// Pawn possession replaces Owner with the controller. UE 5.7 can delay the spawn
	// notification until after possession, so summons also retain their source as Instigator.
	TArray<const AActor*> Pending;
	if (Actor)
	{
		Pending.Add(Actor->GetOwner());
		if (Actor->GetInstigator() != Actor) { Pending.Add(Actor->GetInstigator()); }
	}
	TSet<const AActor*> Visited;
	while (!Pending.IsEmpty())
	{
		const AActor* Source = Pending.Pop();
		if (!Source || Visited.Contains(Source)) { continue; }
		if (Source == this || EncounterActors.Contains(TWeakObjectPtr<AActor>(const_cast<AActor*>(Source)))) { return true; }
		Visited.Add(Source);
		Pending.Add(Source->GetOwner());
		if (Source->GetInstigator() != Source) { Pending.Add(Source->GetInstigator()); }
	}
	return false;
}
void ATunaSweeperBossEncounter::TrackSpawnedActor(AActor* Actor)
{
	if (IsValid(Actor) && Actor != this && BelongsToEncounter(Actor))
	{
		EncounterActors.Add(Actor);
	}
}

void ATunaSweeperBossEncounter::CleanupEncounterActors()
{
	if (bCleaningUp) { return; }
	TGuardValue<bool> CleanupGuard(bCleaningUp, true);
	if (UWorld* World = GetWorld())
	{
		// Also catches actors whose Blueprint assigned Owner after the spawn notification.
		for (TActorIterator<AActor> It(World); It; ++It) { TrackSpawnedActor(*It); }
	}
	if (ATunaSweeperEnemyCharacter* CurrentBoss = Boss.Get())
	{
		CurrentBoss->OnDestroyed.RemoveDynamic(this, &ATunaSweeperBossEncounter::OnBossDestroyed);
	}
	// Stop every damage producer before destroying any owner. CancelPatterns can destroy siblings.
	TArray<TWeakObjectPtr<AActor>> Pending = EncounterActors.Array();
	for (const TWeakObjectPtr<AActor>& Entry : Pending)
	{
		AActor* Actor = Entry.Get();
		if (!IsValid(Actor)) { continue; }
		Actor->SetActorTickEnabled(false);
		Actor->SetActorEnableCollision(false);
		if (APawn* Pawn = Cast<APawn>(Actor))
		{
			if (AController* Controller = Pawn->GetController(); Controller && !Controller->IsPlayerController())
			{
				Controller->StopMovement();
				Controller->Destroy();
			}
		}
		if (UTunaSweeperCombatPatternComponent* Patterns = Actor->FindComponentByClass<UTunaSweeperCombatPatternComponent>())
		{
			Patterns->CancelPatterns(true);
		}
	}
	// Include any ownership-preserving effect spawned during cancellation/EndPlay.
	TSet<TWeakObjectPtr<AActor>> Processed;
	while (true)
	{
		Pending = EncounterActors.Array();
		Pending.RemoveAll([&Processed](const TWeakObjectPtr<AActor>& Entry) { return Processed.Contains(Entry); });
		if (Pending.IsEmpty()) { break; }
		for (const TWeakObjectPtr<AActor>& Entry : Pending)
		{
			Processed.Add(Entry);
			if (AActor* Actor = Entry.Get(); IsValid(Actor) && !Actor->IsActorBeingDestroyed()) { Actor->Destroy(); }
		}
	}
	EncounterActors.Reset();
	Boss.Reset();
}

void ATunaSweeperBossEncounter::ClearEncounter()
{
	CleanupEncounterActors();
	bAwaitingExit = true;
	SetState(ETunaSweeperBossEncounterState::Cleared, TEXT("Boss defeated"));
}

void ATunaSweeperBossEncounter::ResetEncounter()
{
	CleanupEncounterActors();
	Occupant = FindPlayerInside();
	bAwaitingExit = Occupant.IsValid();
	SetState(ETunaSweeperBossEncounterState::Ready, TEXT("Encounter reset"));
}

void ATunaSweeperBossEncounter::OnBossDestroyed(AActor* DestroyedActor)
{
	if (bCleaningUp || State != ETunaSweeperBossEncounterState::Active) { return; }
	const ATunaSweeperEnemyCharacter* Enemy = Cast<ATunaSweeperEnemyCharacter>(DestroyedActor);
	const bool bDefeated = Enemy && Enemy->IsDead();
	CleanupEncounterActors();
	bAwaitingExit = true;
	SetState(bDefeated ? ETunaSweeperBossEncounterState::Cleared : ETunaSweeperBossEncounterState::Ready,
		bDefeated ? TEXT("Boss defeated") : TEXT("Boss destroyed externally"));
}

void ATunaSweeperBossEncounter::OnCombatBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (const APawn* Pawn = Cast<APawn>(OtherActor); Pawn && Pawn->IsPlayerControlled()) { UpdateOccupancy(); }
}

void ATunaSweeperBossEncounter::OnCombatEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex)
{
	if (OtherActor == Occupant.Get()) { UpdateOccupancy(); }
}

void ATunaSweeperBossEncounter::SetState(ETunaSweeperBossEncounterState NewState, const TCHAR* Reason)
{
	if (State == NewState) { return; }
	State = NewState;
	UE_LOG(LogTunaBossEncounter, Display, TEXT("Encounter %s -> %s: %s"), *EncounterId.ToString(),
		*UEnum::GetValueAsString(State), Reason);
}

void ATunaSweeperBossEncounter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	CleanupEncounterActors();
	if (UWorld* World = GetWorld())
	{
		World->RemoveOnActorSpawnedHandler(ActorSpawnedHandle);
	}
	Super::EndPlay(EndPlayReason);
}
