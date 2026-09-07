#if WITH_DEV_AUTOMATION_TESTS

#include "AI/TunaSweeperAttackTelegraph.h"
#include "AI/TunaSweeperEnemyCharacter.h"
#include "AI/TunaSweeperEnemyAIController.h"
#include "AI/TunaSweeperMissileTurret.h"
#include "AI/TunaSweeperRollingRobotMinion.h"
#include "Component/TunaSweeperCombatPatternComponent.h"
#include "Component/TunaSweeperFactionComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "Subsystem/TunaSweeperFactionSubsystem.h"
#include "TunaSweeperCollisionChannels.h"

namespace TunaSweeperCombatPatternTests
{
	constexpr EAutomationTestFlags TestFlags =
		EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;

	/** Real registered collision shapes, but explicit attack ticks keep tests independent of AI and frame rate. */
	struct FTestWorld
	{
		UWorld* World = nullptr;

		FTestWorld()
		{
			const UWorld::InitializationValues Values = UWorld::InitializationValues()
				.AllowAudioPlayback(false).RequiresHitProxies(false)
				.CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false)
				.ShouldSimulatePhysics(false).EnableTraceCollision(true).SetTransactional(false);
			World = UWorld::CreateWorld(EWorldType::Game, false,
				MakeUniqueObjectName(GetTransientPackage(), UWorld::StaticClass(), TEXT("CombatPatternTestWorld")),
				GetTransientPackage(), true, ERHIFeatureLevel::Num, &Values);
			if (World)
			{
				if (GEngine) GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
				AddBlocker(FVector(0.0f, 0.0f, -20.0f), FVector(3000.0f, 3000.0f, 20.0f));
			}
		}

		~FTestWorld()
		{
			if (World)
			{
				World->DestroyWorld(false);
				if (GEngine) GEngine->DestroyWorldContext(World);
				World->RemoveFromRoot();
			}
		}

		AActor* AddBlocker(const FVector& Location, const FVector& Extent)
		{
			AActor* Actor = World->SpawnActor<AActor>();
			if (!Actor) { return nullptr; }
			UBoxComponent* Box = NewObject<UBoxComponent>(Actor);
			Actor->SetRootComponent(Box);
			Actor->AddInstanceComponent(Box);
			Box->SetBoxExtent(Extent);
			Box->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			Box->SetCollisionObjectType(ECC_WorldStatic);
			Box->SetCollisionResponseToAllChannels(ECR_Block);
			Box->SetGenerateOverlapEvents(false);
			Box->RegisterComponent();
			Actor->SetActorLocation(Location);
			return Actor;
		}

		template <typename T = ATunaSweeperEnemyCharacter>
		T* SpawnEnemy(const FVector& Location, uint8 Faction = 10, float Health = 100.0f)
		{
			T* Enemy = World->SpawnActorDeferred<T>(T::StaticClass(), FTransform(Location), nullptr,
				nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
			if (!Enemy) { return nullptr; }
			Enemy->AutoPossessAI = EAutoPossessAI::Disabled;
			FTunaSweeperEnemyCombatProfile Profile;
			Profile.AttackMode = ETunaSweeperEnemyAttackMode::Melee;
			Enemy->ConfigureCombatProfile(Profile, Faction, NAME_None, INDEX_NONE);
			Enemy->ConfigureSpawnData(TSoftObjectPtr<UMaterialInterface>(), NAME_None, INDEX_NONE,
				INDEX_NONE, Health, 0);
			Enemy->FinishSpawning(FTransform(Location));
			Enemy->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
			World->GetSubsystem<UTunaSweeperFactionSubsystem>()->RegisterFactionActor(Enemy);
			return Enemy;
		}

		template <typename T>
		TArray<T*> FindActors() const
		{
			TArray<T*> Result;
			for (TActorIterator<T> It(World); It; ++It)
			{
				if (IsValid(*It) && !It->IsActorBeingDestroyed()) { Result.Add(*It); }
			}
			return Result;
		}
	};

	void TickPattern(UTunaSweeperCombatPatternComponent* Pattern, float Seconds)
	{
		// Access through the public base API while retaining virtual dispatch.
		static_cast<UActorComponent*>(Pattern)->TickComponent(Seconds, LEVELTICK_All, nullptr);
	}

	float RemainingHealth(AActor* Actor)
	{
		// Enemy TakeDamage returns the actual remaining health on a lethal hit.
		return UGameplayStatics::ApplyDamage(Actor, 100000.0f, nullptr, nullptr, nullptr);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperCombatPatternOptInAndLockedChargeTest,
	"TunaSweeper.Combat.Patterns.OptInAndLockedCharge",
	TunaSweeperCombatPatternTests::TestFlags)

bool FTunaSweeperCombatPatternOptInAndLockedChargeTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace TunaSweeperCombatPatternTests;
	FTestWorld TestWorld;
	if (!TestNotNull(TEXT("Collision test world exists"), TestWorld.World)) { return false; }
	ATunaSweeperEnemyCharacter* Enemy = TestWorld.SpawnEnemy(FVector(0.0f, 0.0f, 88.0f));
	ATunaSweeperEnemyCharacter* Target = TestWorld.SpawnEnemy(FVector(800.0f, 0.0f, 88.0f), 1);
	if (!TestNotNull(TEXT("Enemy exists"), Enemy) || !TestNotNull(TEXT("Target exists"), Target)) { return false; }
	UTunaSweeperCombatPatternComponent* Pattern = Enemy->GetCombatPatternComponent();
	if (!TestNotNull(TEXT("Ordinary enemy exposes reusable pattern component"), Pattern)) { return false; }
	TestFalse(TEXT("Existing enemies require explicit automatic-pattern opt-in"), Pattern->bAutomaticPatterns);
	TestFalse(TEXT("Disabled automatic patterns do not start against a valid hostile"), Pattern->TryStartAutomaticPattern(Target));
	TestFalse(TEXT("Idle component leaves standard combat enabled"), Enemy->IsStandardCombatSuppressed());

	Pattern->ChargeWarningSeconds = 1.0f;
	Pattern->ChargeDistance = 900.0f;
	Pattern->ChargeSpeed = 600.0f;
	const EMovementMode OriginalMovementMode = Enemy->GetCharacterMovement()->MovementMode;
	const FVector Start = Enemy->GetActorLocation();
	if (!TestTrue(TEXT("Explicitly requested charge starts"), Pattern->TryStartPattern(ETunaSweeperCombatPattern::Charge, Target))) { return false; }
	TestTrue(TEXT("Pattern suppresses ordinary combat while active"), Enemy->IsStandardCombatSuppressed());
	TestEqual(TEXT("Charge starts in warning phase"), static_cast<uint8>(Pattern->GetPhase()), static_cast<uint8>(ETunaSweeperCombatPatternPhase::Warning));
	TestFalse(TEXT("A second pattern cannot overlap the active charge"), Pattern->TryStartPattern(ETunaSweeperCombatPattern::RollingMinions, Target));
	TickPattern(Pattern, 0.5f);
	TestTrue(TEXT("Warning phase does not move the charging body"), Enemy->GetActorLocation().Equals(Start, 0.1f));
	TArray<ATunaSweeperAttackTelegraph*> Warnings = TestWorld.FindActors<ATunaSweeperAttackTelegraph>();
	TestEqual(TEXT("Charge has one visible lane"), Warnings.Num(), 1);
	if (Warnings.Num() == 1)
	{
		TestTrue(TEXT("Lane fill represents half the warning time"), FMath::IsNearlyEqual(Warnings[0]->GetProgress(), 0.5f, 0.01f));
	}
	Target->SetActorLocation(FVector(800.0f, 600.0f, 88.0f));
	TickPattern(Pattern, 0.6f);
	TickPattern(Pattern, 0.2f);
	TestTrue(TEXT("Charge advances after warning completion"), Enemy->GetActorLocation().X > Start.X + 20.0f);
	TestTrue(TEXT("Moving target cannot steer the advertised lane"), FMath::Abs(Enemy->GetActorLocation().Y - Start.Y) < 1.0f);
	Pattern->CancelPatterns();
	TestFalse(TEXT("Cancellation returns the component to idle"), Pattern->IsPatternActive());
	TestFalse(TEXT("Cancellation releases normal AI combat"), Enemy->IsStandardCombatSuppressed());
	TestEqual(TEXT("Cancellation restores the original movement mode"),
		static_cast<uint8>(Enemy->GetCharacterMovement()->MovementMode), static_cast<uint8>(OriginalMovementMode));
	TestEqual(TEXT("Cancellation removes the warning actor"), TestWorld.FindActors<ATunaSweeperAttackTelegraph>().Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperCombatPatternChargeObstacleTest,
	"TunaSweeper.Combat.Patterns.ChargeStopsAtObstacle",
	TunaSweeperCombatPatternTests::TestFlags)

bool FTunaSweeperCombatPatternChargeObstacleTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace TunaSweeperCombatPatternTests;
	FTestWorld TestWorld;
	if (!TestNotNull(TEXT("Collision test world exists"), TestWorld.World)) { return false; }
	TestWorld.AddBlocker(FVector(400.0f, 0.0f, 150.0f), FVector(20.0f, 200.0f, 150.0f));
	ATunaSweeperEnemyCharacter* Enemy = TestWorld.SpawnEnemy(FVector(0.0f, 0.0f, 88.0f));
	ATunaSweeperEnemyCharacter* Target = TestWorld.SpawnEnemy(FVector(800.0f, 0.0f, 88.0f), 1, 30.0f);
	if (!Enemy || !Target) { AddError(TEXT("Charge test combatants failed to spawn")); return false; }
	UTunaSweeperCombatPatternComponent* Pattern = Enemy->GetCombatPatternComponent();
	Pattern->ChargeWarningSeconds = 0.2f;
	Pattern->ChargeDistance = 1000.0f;
	Pattern->ChargeSpeed = 1500.0f;
	if (!TestTrue(TEXT("A blocked route still permits an honestly clipped charge"),
		Pattern->TryStartPattern(ETunaSweeperCombatPattern::Charge, Target))) { return false; }
	TickPattern(Pattern, 0.25f);
	for (int32 Step = 0; Step < 10; ++Step) { TickPattern(Pattern, 0.1f); }
	TestTrue(TEXT("Charging body travels toward the obstacle"), Enemy->GetActorLocation().X > 10.0f);
	TestTrue(TEXT("Charging capsule cannot cross the blocking wall"), Enemy->GetActorLocation().X < 350.0f);
	TestTrue(TEXT("Charge does not damage a target behind the wall"), FMath::IsNearlyEqual(RemainingHealth(Target), 30.0f));
	Pattern->CancelPatterns();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperCombatPatternOwnerDeathTest,
	"TunaSweeper.Combat.Patterns.OwnerDeathCancelsWarning",
	TunaSweeperCombatPatternTests::TestFlags)

bool FTunaSweeperCombatPatternOwnerDeathTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace TunaSweeperCombatPatternTests;
	FTestWorld TestWorld;
	if (!TestNotNull(TEXT("Collision test world exists"), TestWorld.World)) { return false; }
	ATunaSweeperEnemyCharacter* Enemy = TestWorld.SpawnEnemy(FVector(0.0f, 0.0f, 88.0f), 10, 20.0f);
	ATunaSweeperEnemyCharacter* Target = TestWorld.SpawnEnemy(FVector(600.0f, 0.0f, 88.0f), 1);
	if (!Enemy || !Target) { AddError(TEXT("Death test combatants failed to spawn")); return false; }
	UTunaSweeperCombatPatternComponent* Pattern = Enemy->GetCombatPatternComponent();
	if (!TestTrue(TEXT("Charge warning starts before owner death"), Pattern->TryStartPattern(ETunaSweeperCombatPattern::Charge, Target))) { return false; }
	TestEqual(TEXT("Warning exists before owner death"), TestWorld.FindActors<ATunaSweeperAttackTelegraph>().Num(), 1);
	UGameplayStatics::ApplyDamage(Enemy, 100.0f, nullptr, Target, nullptr);
	TestTrue(TEXT("Pattern owner dies to hostile damage"), Enemy->IsDead());
	TestFalse(TEXT("Death cancels the active pattern immediately"), Pattern->IsPatternActive());
	TestEqual(TEXT("Owner death removes warnings in the same call"), TestWorld.FindActors<ATunaSweeperAttackTelegraph>().Num(), 0);
	TestTrue(TEXT("No pending attack hits the surviving target"), FMath::IsNearlyEqual(RemainingHealth(Target), 100.0f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperRollingMinionEarlyDamageTest,
	"TunaSweeper.Combat.Patterns.RollingMinionDamageableBeforeDeployment",
	TunaSweeperCombatPatternTests::TestFlags)

bool FTunaSweeperRollingMinionEarlyDamageTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace TunaSweeperCombatPatternTests;
	FTestWorld TestWorld;
	if (!TestNotNull(TEXT("Collision test world exists"), TestWorld.World)) { return false; }
	ATunaSweeperRollingRobotMinion* Minion = TestWorld.SpawnEnemy<ATunaSweeperRollingRobotMinion>(FVector(0.0f, 0.0f, 39.0f), 10, 20.0f);
	ATunaSweeperEnemyCharacter* Attacker = TestWorld.SpawnEnemy(FVector(600.0f, 0.0f, 88.0f), 1);
	ATunaSweeperEnemyCharacter* Ally = TestWorld.SpawnEnemy(FVector(-600.0f, 0.0f, 88.0f), 10);
	if (!Minion || !Attacker || !Ally) { AddError(TEXT("Rollout test combatants failed to spawn")); return false; }
	Minion->InitializeRoll(FVector::ForwardVector, Attacker);
	TestEqual(TEXT("Newly emitted robot is still a rolling ball"),
		static_cast<uint8>(Minion->GetDeploymentPhase()), static_cast<uint8>(ETunaSweeperRollingRobotPhase::Rolling));
	TestTrue(TEXT("Rolling phase suppresses standard attacks"), Minion->IsStandardCombatSuppressed());
	TestEqual(TEXT("Rolling hurtbox blocks player projectiles"),
		static_cast<uint8>(Minion->GetCapsuleComponent()->GetCollisionResponseToChannel(TunaSweeperCollisionChannels::Projectile)),
		static_cast<uint8>(ECR_Block));
	TestTrue(TEXT("Rolling capsule remains a sphere"), FMath::IsNearlyEqual(
		Minion->GetCapsuleComponent()->GetScaledCapsuleRadius(), Minion->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()));
	TestTrue(TEXT("Friendly damage cannot hurt a rolling robot"), FMath::IsNearlyZero(
		UGameplayStatics::ApplyDamage(Minion, 5.0f, nullptr, Ally, nullptr)));
	TestTrue(TEXT("A hostile can damage the robot before it stands"), FMath::IsNearlyEqual(
		UGameplayStatics::ApplyDamage(Minion, 5.0f, nullptr, Attacker, nullptr), 5.0f));
	TestFalse(TEXT("Nonlethal rolling damage leaves the robot alive"), Minion->IsDead());
	TestTrue(TEXT("The robot can be killed during rollout"), FMath::IsNearlyEqual(
		UGameplayStatics::ApplyDamage(Minion, 100.0f, nullptr, Attacker, nullptr), 15.0f));
	TestTrue(TEXT("Lethal rollout damage reaches inherited death handling"), Minion->IsDead());
	TestEqual(TEXT("Death prevents future unfolding"),
		static_cast<uint8>(Minion->GetDeploymentPhase()), static_cast<uint8>(ETunaSweeperRollingRobotPhase::Dead));
	TestEqual(TEXT("Dead rollout body no longer blocks shots"),
		static_cast<uint8>(Minion->GetCapsuleComponent()->GetCollisionEnabled()), static_cast<uint8>(ECollisionEnabled::NoCollision));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperCombatPatternMinionWaveCleanupTest,
	"TunaSweeper.Combat.Patterns.MinionWaveStaggerAndCleanup",
	TunaSweeperCombatPatternTests::TestFlags)

bool FTunaSweeperCombatPatternMinionWaveCleanupTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace TunaSweeperCombatPatternTests;
	FTestWorld TestWorld;
	if (!TestNotNull(TEXT("Collision test world exists"), TestWorld.World)) { return false; }
	ATunaSweeperEnemyCharacter* Enemy = TestWorld.SpawnEnemy(FVector(0.0f, 0.0f, 88.0f), 10, 20.0f);
	ATunaSweeperEnemyCharacter* Target = TestWorld.SpawnEnemy(FVector(800.0f, 0.0f, 88.0f), 1);
	if (!Enemy || !Target) { AddError(TEXT("Wave test combatants failed to spawn")); return false; }
	UTunaSweeperCombatPatternComponent* Pattern = Enemy->GetCombatPatternComponent();
	Pattern->MinionsPerWave = 5;
	Pattern->MaxActiveMinions = 2;
	Pattern->MinionSpawnInterval = 0.5f;
	if (!TestTrue(TEXT("Reusable minion wave starts"), Pattern->TryStartPattern(ETunaSweeperCombatPattern::RollingMinions, Target))) { return false; }
	TickPattern(Pattern, 0.01f);
	TestEqual(TEXT("A wave starts with one damageable minion"), TestWorld.FindActors<ATunaSweeperRollingRobotMinion>().Num(), 1);
	TickPattern(Pattern, 0.1f);
	TestEqual(TEXT("Minions are staggered instead of spawning every frame"), TestWorld.FindActors<ATunaSweeperRollingRobotMinion>().Num(), 1);
	TickPattern(Pattern, 0.5f);
	TestEqual(TEXT("Wave respects the configured active-minion cap"), TestWorld.FindActors<ATunaSweeperRollingRobotMinion>().Num(), 2);
	TickPattern(Pattern, 5.0f);
	TestEqual(TEXT("A long frame cannot emit extra minions beyond the cap"), TestWorld.FindActors<ATunaSweeperRollingRobotMinion>().Num(), 2);
	UGameplayStatics::ApplyDamage(Enemy, 100.0f, nullptr, Target, nullptr);
	TestEqual(TEXT("Owner death removes its remaining summoned minions"), TestWorld.FindActors<ATunaSweeperRollingRobotMinion>().Num(), 0);
	TestFalse(TEXT("Owner death leaves no pending minion wave"), Pattern->IsPatternActive());
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperMissileWarningTimingAndDamageTest,
	"TunaSweeper.Combat.Patterns.MissileWarningTimingLockedTargetAndFaction",
	TunaSweeperCombatPatternTests::TestFlags)

bool FTunaSweeperMissileWarningTimingAndDamageTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace TunaSweeperCombatPatternTests;
	FTestWorld TestWorld;
	if (!TestNotNull(TEXT("Collision test world exists"), TestWorld.World)) { return false; }
	ATunaSweeperEnemyCharacter* Source = TestWorld.SpawnEnemy(FVector(0.0f, 0.0f, 88.0f));
	ATunaSweeperEnemyCharacter* Target = TestWorld.SpawnEnemy(FVector(800.0f, 0.0f, 88.0f), 1, 30.0f);
	ATunaSweeperEnemyCharacter* EarlyVictim = TestWorld.SpawnEnemy(FVector(800.0f, 100.0f, 88.0f), 1, 30.0f);
	if (!Source || !Target || !EarlyVictim) { AddError(TEXT("Missile test combatants failed to spawn")); return false; }
	const FTransform TurretTransform(FVector(-200.0f, 0.0f, 0.0f));
	ATunaSweeperMissileTurret* Turret = TestWorld.World->SpawnActorDeferred<ATunaSweeperMissileTurret>(
		ATunaSweeperMissileTurret::StaticClass(), TurretTransform, Source, Source, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!TestNotNull(TEXT("Destructible missile turret exists"), Turret)) { return false; }
	Turret->InitialFireDelay = 0.0f;
	Turret->WarningDuration = 1.0f;
	Turret->ImpactRadius = 120.0f;
	Turret->ImpactDamage = 7.0f;
	Turret->ImpactHeight = 180.0f;
	Turret->InitializeTurret(Source, Target);
	Turret->FinishSpawning(TurretTransform);
	Turret->DispatchBeginPlay();
	Turret->Tick(0.01f);
	if (!TestTrue(TEXT("Turret advertises its first missile before impact"), Turret->IsWarningActive())) { return false; }
	const FVector Locked = Turret->GetLockedImpactLocation();
	TestTrue(TEXT("Warning lies on the traced floor below the target"), Locked.Equals(FVector(800.0f, 0.0f, 0.0f), 0.1f));
	Turret->Tick(0.5f);
	TestTrue(TEXT("Missile warning takes the full authored duration"), FMath::IsNearlyEqual(Turret->GetWarningProgress(), 0.5f, 0.01f));
	TestTrue(TEXT("An actor inside the half-filled circle has taken no early damage"), FMath::IsNearlyEqual(RemainingHealth(EarlyVictim), 30.0f));
	Target->SetActorLocation(FVector(800.0f, 600.0f, 88.0f));
	ATunaSweeperEnemyCharacter* Victim = TestWorld.SpawnEnemy(FVector(800.0f, 100.0f, 88.0f), 1, 30.0f);
	ATunaSweeperEnemyCharacter* Friendly = TestWorld.SpawnEnemy(FVector(800.0f, -100.0f, 88.0f), 10, 30.0f);
	ATunaSweeperEnemyCharacter* UpperFloor = TestWorld.SpawnEnemy(FVector(800.0f, 0.0f, 388.0f), 1, 30.0f);
	ATunaSweeperEnemyCharacter* OutsideCircle = TestWorld.SpawnEnemy(FVector(940.0f, 140.0f, 88.0f), 1, 30.0f);
	if (!Victim || !Friendly || !UpperFloor || !OutsideCircle) { AddError(TEXT("Missile boundary targets failed to spawn")); return false; }
	Turret->Tick(0.5f);
	TestTrue(TEXT("Impact stays at the originally advertised point"), Turret->GetLockedImpactLocation().Equals(Locked, 0.1f));
	TestFalse(TEXT("Completed missile clears its warning"), Turret->IsWarningActive());
	TestEqual(TEXT("Completed missile removes the visual warning"), TestWorld.FindActors<ATunaSweeperAttackTelegraph>().Num(), 0);
	TestTrue(TEXT("Hostile standing in the circle takes exactly one missile hit"), FMath::IsNearlyEqual(RemainingHealth(Victim), 23.0f));
	TestTrue(TEXT("Moving out of the locked warning avoids damage"), FMath::IsNearlyEqual(RemainingHealth(Target), 30.0f));
	TestTrue(TEXT("Allied actors inside the circle are protected"), FMath::IsNearlyEqual(RemainingHealth(Friendly), 30.0f));
	TestTrue(TEXT("Actors on another floor are excluded"), FMath::IsNearlyEqual(RemainingHealth(UpperFloor), 30.0f));
	TestTrue(TEXT("Square overlap corners outside the advertised circle are excluded"), FMath::IsNearlyEqual(RemainingHealth(OutsideCircle), 30.0f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperMissileTurretDeathCancelsWarningTest,
	"TunaSweeper.Combat.Patterns.DestroyedTurretCancelsPendingMissile",
	TunaSweeperCombatPatternTests::TestFlags)

bool FTunaSweeperMissileTurretDeathCancelsWarningTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace TunaSweeperCombatPatternTests;
	FTestWorld TestWorld;
	if (!TestNotNull(TEXT("Collision test world exists"), TestWorld.World)) { return false; }
	ATunaSweeperEnemyCharacter* Source = TestWorld.SpawnEnemy(FVector(0.0f, 0.0f, 88.0f));
	ATunaSweeperEnemyCharacter* Target = TestWorld.SpawnEnemy(FVector(800.0f, 0.0f, 88.0f), 1, 30.0f);
	if (!Source || !Target) { AddError(TEXT("Turret death test combatants failed to spawn")); return false; }
	ATunaSweeperMissileTurret* Turret = TestWorld.World->SpawnActor<ATunaSweeperMissileTurret>(FVector(-200.0f, 0.0f, 0.0f), FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("Turret exists"), Turret)) { return false; }
	Turret->InitialFireDelay = 0.0f;
	Turret->InitializeTurret(Source, Target);
	Turret->DispatchBeginPlay();
	Turret->Tick(0.01f);
	if (!TestTrue(TEXT("Missile warning is pending before turret destruction"), Turret->IsWarningActive())) { return false; }
	TestTrue(TEXT("Turret rejects friendly fire"), FMath::IsNearlyZero(UGameplayStatics::ApplyDamage(Turret, 100.0f, nullptr, Source, nullptr)));
	TestTrue(TEXT("Hostile fire can destroy the summoned turret"), UGameplayStatics::ApplyDamage(Turret, 100.0f, nullptr, Target, nullptr) > 0.0f);
	TestTrue(TEXT("Destroyed turret records its death"), Turret->IsDead());
	TestFalse(TEXT("Destroying a turret immediately cancels its warning"), Turret->IsWarningActive());
	TestEqual(TEXT("Destroying a turret removes its pending circle"), TestWorld.FindActors<ATunaSweeperAttackTelegraph>().Num(), 0);
	TestTrue(TEXT("Cancelled missile inflicts no damage"), FMath::IsNearlyEqual(RemainingHealth(Target), 30.0f));
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperCombatPatternChargeSingleHitTest,
	"TunaSweeper.Combat.Patterns.ChargeAppliesOneBodyHit",
	TunaSweeperCombatPatternTests::TestFlags)

bool FTunaSweeperCombatPatternChargeSingleHitTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace TunaSweeperCombatPatternTests;
	FTestWorld TestWorld;
	if (!TestNotNull(TEXT("Collision test world exists"), TestWorld.World)) { return false; }
	ATunaSweeperEnemyCharacter* Enemy = TestWorld.SpawnEnemy(FVector(0.0f, 0.0f, 88.0f));
	ATunaSweeperEnemyCharacter* Target = TestWorld.SpawnEnemy(FVector(800.0f, 0.0f, 88.0f), 1);
	ATunaSweeperEnemyCharacter* Victim = TestWorld.SpawnEnemy(FVector(250.0f, 0.0f, 88.0f), 1, 30.0f);
	if (!Enemy || !Target || !Victim) { AddError(TEXT("Charge damage test combatants failed to spawn")); return false; }
	UTunaSweeperCombatPatternComponent* Pattern = Enemy->GetCombatPatternComponent();
	Pattern->ChargeWarningSeconds = 0.2f;
	Pattern->ChargeSpeed = 1000.0f;
	Pattern->ChargeDamage = 7.0f;
	if (!TestTrue(TEXT("Charge starts with a hostile in its lane"), Pattern->TryStartPattern(ETunaSweeperCombatPattern::Charge, Target))) { return false; }
	TickPattern(Pattern, 0.25f);
	for (int32 Step = 0; Step < 10; ++Step) { TickPattern(Pattern, 0.05f); }
	TestEqual(TEXT("Body contact applies one hit despite repeated attack ticks"), RemainingHealth(Victim), 23.0f);
	Pattern->CancelPatterns();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperRollingMinionUnfoldSpaceTest,
	"TunaSweeper.Combat.Patterns.RollingMinionUnfoldsOnlyInClearSpace",
	TunaSweeperCombatPatternTests::TestFlags)

bool FTunaSweeperRollingMinionUnfoldSpaceTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace TunaSweeperCombatPatternTests;
	FTestWorld TestWorld;
	if (!TestNotNull(TEXT("Collision test world exists"), TestWorld.World)) { return false; }
	ATunaSweeperRollingRobotMinion* Minion = TestWorld.SpawnEnemy<ATunaSweeperRollingRobotMinion>(FVector(0.0f, 0.0f, 39.0f), 10, 20.0f);
	AActor* LowCeiling = TestWorld.AddBlocker(FVector(0.0f, 0.0f, 110.0f), FVector(100.0f, 100.0f, 10.0f));
	if (!Minion || !LowCeiling) { AddError(TEXT("Deployment-space test failed to spawn")); return false; }
	const float OriginalFeetZ = Minion->GetActorLocation().Z - Minion->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	static_cast<AActor*>(Minion)->Tick(1.5f);
	TestEqual(TEXT("A low ceiling prevents expansion into standing space"),
		static_cast<uint8>(Minion->GetDeploymentPhase()), static_cast<uint8>(ETunaSweeperRollingRobotPhase::Rolling));
	TestEqual(TEXT("Blocked robot remains vulnerable to projectiles"),
		static_cast<uint8>(Minion->GetCapsuleComponent()->GetCollisionResponseToChannel(TunaSweeperCollisionChannels::Projectile)),
		static_cast<uint8>(ECR_Block));
	LowCeiling->Destroy();
	static_cast<AActor*>(Minion)->Tick(0.1f);
	TestEqual(TEXT("Grounded robot unfolds when standing space becomes free"),
		static_cast<uint8>(Minion->GetDeploymentPhase()), static_cast<uint8>(ETunaSweeperRollingRobotPhase::Unfolding));
	TestTrue(TEXT("Unfolding still holds ordinary melee AI"), Minion->IsStandardCombatSuppressed());
	static_cast<AActor*>(Minion)->Tick(0.5f);
	TestEqual(TEXT("Unfolding completes into a walking robot"),
		static_cast<uint8>(Minion->GetDeploymentPhase()), static_cast<uint8>(ETunaSweeperRollingRobotPhase::Walking));
	TestFalse(TEXT("Walking robot resumes standard enemy combat"), Minion->IsStandardCombatSuppressed());
	TestTrue(TEXT("Standing collision grows upward without burying the feet"), FMath::IsNearlyEqual(
		Minion->GetActorLocation().Z - Minion->GetCapsuleComponent()->GetScaledCapsuleHalfHeight(), OriginalFeetZ, 0.1f));
	TestTrue(TEXT("Walking robot has a standing capsule"),
		Minion->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() > Minion->GetCapsuleComponent()->GetScaledCapsuleRadius());
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperCombatPatternRangedBoundaryTest,
	"TunaSweeper.Combat.Patterns.RangedAttackAndReloadBoundaries",
	TunaSweeperCombatPatternTests::TestFlags)

bool FTunaSweeperCombatPatternRangedBoundaryTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace TunaSweeperCombatPatternTests;
	FTestWorld TestWorld;
	if (!TestNotNull(TEXT("Collision test world exists"), TestWorld.World)) { return false; }
	ATunaSweeperEnemyCharacter* Enemy = TestWorld.SpawnEnemy(FVector(0.0f, 0.0f, 88.0f));
	ATunaSweeperEnemyCharacter* Target = TestWorld.SpawnEnemy(FVector(800.0f, 0.0f, 88.0f), 1);
	ATunaSweeperEnemyAIController* Controller = TestWorld.World->SpawnActor<ATunaSweeperEnemyAIController>();
	if (!Enemy || !Target || !Controller) { AddError(TEXT("Ranged boundary test combatants failed to spawn")); return false; }
	FTunaSweeperEnemyCombatProfile RangedProfile;
	RangedProfile.AttackMode = ETunaSweeperEnemyAttackMode::Ranged;
	Enemy->ConfigureCombatProfile(RangedProfile, 10, NAME_None, INDEX_NONE);
	Controller->Possess(Enemy);
	UTunaSweeperCombatPatternComponent* Pattern = Enemy->GetCombatPatternComponent();
	const FVector OriginalLocation = Enemy->GetActorLocation();
	const uint8 OriginalMovementMode = static_cast<uint8>(Enemy->GetCharacterMovement()->MovementMode);
	Controller->RangedCombatState = ETunaSweeperRangedCombatState::Firing;
	TestFalse(TEXT("Manual patterns cannot interrupt an active burst"), Pattern->TryStartPattern(ETunaSweeperCombatPattern::Charge, Target));
	Controller->RangedCombatState = ETunaSweeperRangedCombatState::Reload;
	TestFalse(TEXT("Manual patterns cannot abandon the reload state"), Pattern->TryStartPattern(ETunaSweeperCombatPattern::Charge, Target));
	Controller->RangedCombatState = ETunaSweeperRangedCombatState::Observe;
	Controller->bReloadMovementSpeedReduced = true;
	TestFalse(TEXT("Pending reload-speed restoration prevents a pattern from starting"), Pattern->TryStartPattern(ETunaSweeperCombatPattern::Charge, Target));
	TestFalse(TEXT("Rejected requests leave the pattern idle"), Pattern->IsPatternActive());
	TestTrue(TEXT("Rejected requests cannot move the enemy"), Enemy->GetActorLocation().Equals(OriginalLocation, 0.1f));
	TestEqual(TEXT("Rejected requests preserve normal movement mode"),
		static_cast<uint8>(Enemy->GetCharacterMovement()->MovementMode), OriginalMovementMode);
	TestEqual(TEXT("Rejected requests create no warning actors"), TestWorld.FindActors<ATunaSweeperAttackTelegraph>().Num(), 0);
	Controller->bReloadMovementSpeedReduced = false;
	TestTrue(TEXT("An observing ranged enemy accepts a pattern at a safe boundary"), Pattern->TryStartPattern(ETunaSweeperCombatPattern::Charge, Target));
	TestTrue(TEXT("Accepted pattern records the AI suspension immediately"), Controller->bWasStandardCombatSuppressed);
	Pattern->CancelPatterns();
	Controller->UnPossess();
	return true;
}
#endif
