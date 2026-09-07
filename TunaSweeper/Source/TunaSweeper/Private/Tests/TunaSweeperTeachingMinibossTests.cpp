#if WITH_DEV_AUTOMATION_TESTS

#include "AI/TunaSweeperAttackTelegraph.h"
#include "AI/TunaSweeperEnemyCharacter.h"
#include "AI/TunaSweeperMissileTurret.h"
#include "AI/TunaSweeperPatternEnemyCharacter.h"
#include "AI/TunaSweeperTeachingMinibossCharacters.h"
#include "Component/TunaSweeperCombatPatternComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Effect/TunaSweeperCombatPatternEffectActor.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "Subsystem/TunaSweeperFactionSubsystem.h"
#include "TunaSweeperCollisionChannels.h"
#include "UObject/UnrealType.h"

namespace TunaSweeperTeachingMinibossTests
{
	constexpr EAutomationTestFlags TestFlags =
		EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;

	/** Real collision and factions with explicit ticks instead of autonomous AI. */
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
				MakeUniqueObjectName(GetTransientPackage(), UWorld::StaticClass(), TEXT("TeachingMinibossTestWorld")),
				GetTransientPackage(), true, ERHIFeatureLevel::Num, &Values);
			if (!World) { return; }
			if (GEngine) { GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World); }
			AActor* Floor = World->SpawnActor<AActor>();
			UBoxComponent* Box = NewObject<UBoxComponent>(Floor);
			Floor->SetRootComponent(Box);
			Floor->AddInstanceComponent(Box);
			Box->SetBoxExtent(FVector(3000.0f, 3000.0f, 20.0f));
			Box->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			Box->SetCollisionObjectType(ECC_WorldStatic);
			Box->SetCollisionResponseToAllChannels(ECR_Block);
			Box->SetGenerateOverlapEvents(false);
			Box->RegisterComponent();
			Floor->SetActorLocation(FVector(0.0f, 0.0f, -20.0f));
		}

		~FTestWorld()
		{
			if (World)
			{
				World->DestroyWorld(false);
				if (GEngine) { GEngine->DestroyWorldContext(World); }
				World->RemoveFromRoot();
			}
		}

		template <typename T = ATunaSweeperEnemyCharacter>
		T* SpawnEnemy(const FVector& Location, uint8 Faction = 10, float Health = 0.0f)
		{
			T* Enemy = World->SpawnActorDeferred<T>(T::StaticClass(), FTransform(Location), nullptr,
				nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
			if (!Enemy) { return nullptr; }
			Enemy->AutoPossessAI = EAutoPossessAI::Disabled;
			const FTunaSweeperEnemyCombatProfile Profile = Enemy->GetCombatProfile();
			Enemy->ConfigureCombatProfile(Profile, Faction, NAME_None, INDEX_NONE);
			// Preserve authored subclass health in a world that has not entered BeginPlay.
			const FFloatProperty* Property = FindFProperty<FFloatProperty>(T::StaticClass(), TEXT("MaxHealth"));
			const float SpawnHealth = Health > 0.0f ? Health :
				(Property ? Property->GetPropertyValue_InContainer(Enemy) : 100.0f);
			Enemy->ConfigureSpawnData(TSoftObjectPtr<UMaterialInterface>(), NAME_None, INDEX_NONE,
				INDEX_NONE, SpawnHealth, 0);
			Enemy->FinishSpawning(FTransform(Location));
			Enemy->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
			World->GetSubsystem<UTunaSweeperFactionSubsystem>()->RegisterFactionActor(Enemy);
			return Enemy;
		}

		template <typename T>
		TArray<T*> FindActors() const
		{
			TArray<T*> Actors;
			for (TActorIterator<T> It(World); It; ++It)
			{
				if (IsValid(*It) && !It->IsActorBeingDestroyed()) { Actors.Add(*It); }
			}
			return Actors;
		}

		TArray<ATunaSweeperRollingRobotMinion*> LivingMinions() const
		{
			TArray<ATunaSweeperRollingRobotMinion*> Minions = FindActors<ATunaSweeperRollingRobotMinion>();
			Minions.RemoveAll([](const ATunaSweeperRollingRobotMinion* Minion) { return Minion->IsDead(); });
			return Minions;
		}

		int32 OwnedEffectCount(const AActor* Owner) const
		{
			int32 Count = 0;
			for (const ATunaSweeperCombatPatternEffectActor* Effect : FindActors<ATunaSweeperCombatPatternEffectActor>())
			{
				Count += Effect->GetOwner() == Owner ? 1 : 0;
			}
			return Count;
		}

		void AdvanceClock(float Seconds) const
		{
			// TimeOnly advances cooldown clocks without moving the unmanned test actors.
			for (int32 Step = 0; Step < FMath::CeilToInt(Seconds / 0.1f); ++Step)
			{
				World->Tick(LEVELTICK_TimeOnly, 0.1f);
			}
		}
	};

	void TickPattern(UTunaSweeperCombatPatternComponent* Pattern, float Seconds)
	{
		static_cast<UActorComponent*>(Pattern)->TickComponent(Seconds, LEVELTICK_All, nullptr);
	}

	float RemainingHealth(AActor* Actor)
	{
		return UGameplayStatics::ApplyDamage(Actor, 100000.0f, nullptr, nullptr, nullptr);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperTeachingMinibossPatternIsolationTest,
	"TunaSweeper.Combat.TeachingMinibosses.PatternIsolation",
	TunaSweeperTeachingMinibossTests::TestFlags)

bool FTunaSweeperTeachingMinibossPatternIsolationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace TunaSweeperTeachingMinibossTests;
	FTestWorld TestWorld;
	if (!TestNotNull(TEXT("Teaching test world exists"), TestWorld.World)) { return false; }
	ATunaSweeperEnemyCharacter* Teachers[] = {
		TestWorld.SpawnEnemy<ATunaSweeperChargeTeachingMiniboss>(FVector(0, 0, 88)),
		TestWorld.SpawnEnemy<ATunaSweeperRobotTeachingMiniboss>(FVector(0, 1000, 100)) };
	const ETunaSweeperCombatPattern Lessons[] = {
		ETunaSweeperCombatPattern::Charge, ETunaSweeperCombatPattern::RollingMinions };
	for (int32 Index = 0; Index < 2; ++Index)
	{
		ATunaSweeperEnemyCharacter* Teacher = Teachers[Index];
		ATunaSweeperEnemyCharacter* Target = TestWorld.SpawnEnemy(FVector(120, Index * 1000, 88), 1, 100.0f);
		if (!Teacher || !Target) { AddError(TEXT("Teaching combatants failed to spawn")); return false; }
		UTunaSweeperCombatPatternComponent* Pattern = Teacher->GetCombatPatternComponent();
		FTunaSweeperEnemyCombatProfile PositiveDamageProfile = Teacher->GetCombatProfile();
		PositiveDamageProfile.MeleeAttackDamage = 10.0f;
		Teacher->ConfigureCombatProfile(PositiveDamageProfile, 10, NAME_None, INDEX_NONE);
		TestFalse(TEXT("An idle teacher cannot add an unadvertised ordinary melee attack"), Teacher->AttackTarget(Target));
		TestNull(TEXT("Teaching class does not carry a missile turret class"), Pattern->MissileTurretClass.Get());
		Target->SetActorLocation(FVector(800, Index * 1000, 88));
		// Misconfigured sequences or restored assets cannot bypass allowed capabilities.
		Pattern->MissileTurretClass = ATunaSweeperMissileTurret::StaticClass();
		const ETunaSweeperCombatPattern OtherLesson = Lessons[1 - Index];
		TestFalse(TEXT("Explicit missile requests are rejected"), Pattern->TryStartPattern(ETunaSweeperCombatPattern::MissileTurret, Target));
		TestFalse(TEXT("Explicit requests for the other lesson are rejected"), Pattern->TryStartPattern(OtherLesson, Target));
		Pattern->PatternSequence = { ETunaSweeperCombatPattern::MissileTurret, OtherLesson };
		TestFalse(TEXT("Automatic selection rejects every forbidden pattern"), Pattern->TryStartAutomaticPattern(Target));
		Pattern->PatternSequence.Add(Lessons[Index]);
		TestTrue(TEXT("Automatic selection still finds its own lesson"), Pattern->TryStartAutomaticPattern(Target));
		TestEqual(TEXT("Only the assigned lesson starts"), static_cast<uint8>(Pattern->GetActivePattern()), static_cast<uint8>(Lessons[Index]));
		Pattern->CancelPatterns();
		TestFalse(TEXT("Pattern-only idle time still allows AI tracking"), Teacher->IsStandardCombatSuppressed());
		TestTrue(TEXT("Rejected ordinary attack caused no hidden damage"), FMath::IsNearlyEqual(RemainingHealth(Target), 100.0f));
	}
	TestEqual(TEXT("Neither teacher emitted a missile turret"), TestWorld.FindActors<ATunaSweeperMissileTurret>().Num(), 0);
	ATunaSweeperPatternEnemyCharacter* MainBoss = TestWorld.SpawnEnemy<ATunaSweeperPatternEnemyCharacter>(FVector(0, -1000, 88));
	ATunaSweeperEnemyCharacter* MainTarget = TestWorld.SpawnEnemy(FVector(800, -1000, 88), 1);
	if (!MainBoss || !MainTarget) { AddError(TEXT("Original boss combatants failed to spawn")); return false; }
	UTunaSweeperCombatPatternComponent* MainPatterns = MainBoss->GetCombatPatternComponent();
	TestTrue(TEXT("Original boss keeps access to all three patterns"),
		MainPatterns->EnabledPatterns.Contains(ETunaSweeperCombatPattern::MissileTurret) &&
		MainPatterns->EnabledPatterns.Contains(ETunaSweeperCombatPattern::Charge) &&
		MainPatterns->EnabledPatterns.Contains(ETunaSweeperCombatPattern::RollingMinions));
	TestFalse(TEXT("Original boss keeps ordinary attacks"), MainPatterns->bPatternAttacksOnly);
	TestTrue(TEXT("Original boss can still summon its exclusive missile turret"), MainPatterns->TryStartPattern(ETunaSweeperCombatPattern::MissileTurret, MainTarget));
	MainPatterns->CancelPatterns();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperTeachingChargeTimingTest,
	"TunaSweeper.Combat.TeachingMinibosses.ChargeWarningAndRecovery",
	TunaSweeperTeachingMinibossTests::TestFlags)

bool FTunaSweeperTeachingChargeTimingTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace TunaSweeperTeachingMinibossTests;
	FTestWorld TestWorld;
	if (!TestNotNull(TEXT("Teaching test world exists"), TestWorld.World)) { return false; }
	ATunaSweeperChargeTeachingMiniboss* Teacher = TestWorld.SpawnEnemy<ATunaSweeperChargeTeachingMiniboss>(FVector(0, 0, 88));
	ATunaSweeperEnemyCharacter* Target = TestWorld.SpawnEnemy(FVector(700, 0, 88), 1, 100.0f);
	if (!Teacher || !Target) { AddError(TEXT("Charge lesson failed to spawn")); return false; }
	UTunaSweeperCombatPatternComponent* Pattern = Teacher->GetCombatPatternComponent();
	if (!TestTrue(TEXT("Charge lesson starts"), Pattern->TryStartAutomaticPattern(Target))) { return false; }
	const FVector Start = Teacher->GetActorLocation();
	TickPattern(Pattern, 1.5f);
	const TArray<ATunaSweeperAttackTelegraph*> Warnings = TestWorld.FindActors<ATunaSweeperAttackTelegraph>();
	if (TestEqual(TEXT("Charge lesson shows one lane"), Warnings.Num(), 1))
	{
		TestTrue(TEXT("Normal boss warning duration fills only half of the teaching lane"), FMath::IsNearlyEqual(Warnings[0]->GetProgress(), 0.5f, 0.01f));
	}
	TickPattern(Pattern, 1.49f);
	TestTrue(TEXT("The body stays still for the complete three-second warning"), Teacher->GetActorLocation().Equals(Start, 0.1f));
	TestEqual(TEXT("Charge remains a warning just before three seconds"), static_cast<uint8>(Pattern->GetPhase()), static_cast<uint8>(ETunaSweeperCombatPatternPhase::Warning));
	Target->SetActorLocation(FVector(700, 500, 88));
	TickPattern(Pattern, 0.02f);
	TickPattern(Pattern, 0.2f);
	TestTrue(TEXT("Teaching charge covers 170 cm in 0.2 seconds"), FMath::IsNearlyEqual(Teacher->GetActorLocation().X - Start.X, 170.0f, 1.0f));
	TestTrue(TEXT("Dodging cannot steer the announced lane"), FMath::IsNearlyEqual(Teacher->GetActorLocation().Y, Start.Y, 0.1f));
	TickPattern(Pattern, 1.0f);
	TestEqual(TEXT("Completing the short charge starts recovery"), static_cast<uint8>(Pattern->GetPhase()), static_cast<uint8>(ETunaSweeperCombatPatternPhase::Recovery));
	TestTrue(TEXT("Teaching charge stops at its advertised eight-meter reach"), FMath::IsNearlyEqual(Teacher->GetActorLocation().X - Start.X, 800.0f, 1.0f));
	TickPattern(Pattern, 2.1f);
	TestTrue(TEXT("The player retains more than two seconds to retaliate"), Pattern->IsPatternActive());
	TickPattern(Pattern, 0.11f);
	TestFalse(TEXT("Recovery releases the teacher after 2.2 seconds"), Pattern->IsPatternActive());
	TestFalse(TEXT("Recovery cannot immediately start another charge"), Pattern->TryStartAutomaticPattern(Target));
	TestTrue(TEXT("Leaving the marked lane avoids the body attack"), FMath::IsNearlyEqual(RemainingHealth(Target), 100.0f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperTeachingRobotWaveTimingTest,
	"TunaSweeper.Combat.TeachingMinibosses.StaggeredWaveWaitsForDefeat",
	TunaSweeperTeachingMinibossTests::TestFlags)

bool FTunaSweeperTeachingRobotWaveTimingTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace TunaSweeperTeachingMinibossTests;
	FTestWorld TestWorld;
	if (!TestNotNull(TEXT("Teaching test world exists"), TestWorld.World)) { return false; }
	ATunaSweeperRobotTeachingMiniboss* Teacher = TestWorld.SpawnEnemy<ATunaSweeperRobotTeachingMiniboss>(FVector(0, 0, 100));
	ATunaSweeperEnemyCharacter* Target = TestWorld.SpawnEnemy(FVector(800, 0, 88), 1);
	if (!Teacher || !Target) { AddError(TEXT("Robot lesson failed to spawn")); return false; }
	UTunaSweeperCombatPatternComponent* Pattern = Teacher->GetCombatPatternComponent();
	if (!TestTrue(TEXT("Robot lesson starts"), Pattern->TryStartAutomaticPattern(Target))) { return false; }
	TestTrue(TEXT("Summoner visibly announces the pending wave"), TestWorld.OwnedEffectCount(Teacher) > 0);
	TickPattern(Pattern, 2.49f);
	TestEqual(TEXT("No robots emerge before the 2.5-second preparation ends"), TestWorld.LivingMinions().Num(), 0);
	TickPattern(Pattern, 0.02f);
	TestEqual(TEXT("Preparation releases only one robot"), TestWorld.LivingMinions().Num(), 1);
	TickPattern(Pattern, 0.84f);
	TestEqual(TEXT("Second robot waits for its full 0.85-second interval"), TestWorld.LivingMinions().Num(), 1);
	TickPattern(Pattern, 0.02f);
	TestEqual(TEXT("Second robot emerges after the interval"), TestWorld.LivingMinions().Num(), 2);
	TickPattern(Pattern, 0.86f);
	TArray<ATunaSweeperRollingRobotMinion*> Minions = TestWorld.LivingMinions();
	if (!TestEqual(TEXT("Teaching wave contains exactly three robots"), Minions.Num(), 3)) { return false; }
	for (const ATunaSweeperRollingRobotMinion* Minion : Minions)
	{
		TestTrue(TEXT("Every emitted robot uses the slower teaching variant"), Minion->IsA<ATunaSweeperTeachingRollingRobotMinion>());
		TestEqual(TEXT("New wave robots are damageable balls"), static_cast<uint8>(Minion->GetDeploymentPhase()), static_cast<uint8>(ETunaSweeperRollingRobotPhase::Rolling));
	}
	TickPattern(Pattern, 2.51f);
	TestFalse(TEXT("Wave recovery finishes"), Pattern->IsPatternActive());
	TestWorld.AdvanceClock(10.0f);
	UGameplayStatics::ApplyDamage(Minions[0], 100000.0f, nullptr, Target, nullptr);
	TestEqual(TEXT("One kill opens a slot under the three-robot cap"), TestWorld.LivingMinions().Num(), 2);
	TestFalse(TEXT("A free slot and expired cooldown cannot overlap a living teaching wave"), Pattern->TryStartAutomaticPattern(Target));
	for (int32 Index = 1; Index < Minions.Num(); ++Index)
	{
		UGameplayStatics::ApplyDamage(Minions[Index], 100000.0f, nullptr, Target, nullptr);
	}
	TestTrue(TEXT("Defeating the entire wave permits the next lesson"), Pattern->TryStartAutomaticPattern(Target));
	TickPattern(Pattern, 2.51f);
	TestEqual(TEXT("A fresh lesson can emit its first robot"), TestWorld.LivingMinions().Num(), 1);
	Pattern->CancelPatterns();
	TestEqual(TEXT("Cancellation removes the new teaching wave"), TestWorld.LivingMinions().Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperTeachingRobotEarlyDamageAndDeploymentTest,
	"TunaSweeper.Combat.TeachingMinibosses.VulnerableSlowRobotDeployment",
	TunaSweeperTeachingMinibossTests::TestFlags)

bool FTunaSweeperTeachingRobotEarlyDamageAndDeploymentTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace TunaSweeperTeachingMinibossTests;
	FTestWorld TestWorld;
	if (!TestNotNull(TEXT("Teaching test world exists"), TestWorld.World)) { return false; }
	ATunaSweeperTeachingRollingRobotMinion* Minion = TestWorld.SpawnEnemy<ATunaSweeperTeachingRollingRobotMinion>(FVector(0, 0, 39));
	ATunaSweeperTeachingRollingRobotMinion* EarlyVictim = TestWorld.SpawnEnemy<ATunaSweeperTeachingRollingRobotMinion>(FVector(0, 600, 39));
	ATunaSweeperEnemyCharacter* Attacker = TestWorld.SpawnEnemy(FVector(800, 0, 88), 1);
	if (!Minion || !EarlyVictim || !Attacker) { AddError(TEXT("Teaching rollout actors failed to spawn")); return false; }
	Minion->DispatchBeginPlay();
	Minion->InitializeRoll(FVector::ForwardVector, Attacker);
	TestTrue(TEXT("Initial rollout is slower than the original robot"), FMath::IsNearlyEqual(Minion->GetCharacterMovement()->Velocity.Size2D(), 350.0f, 0.1f));
	TestEqual(TEXT("Rolling hurtbox immediately blocks projectiles"), static_cast<uint8>(Minion->GetCapsuleComponent()->GetCollisionResponseToChannel(TunaSweeperCollisionChannels::Projectile)), static_cast<uint8>(ECR_Block));
	TestTrue(TEXT("Teaching ball takes hostile damage immediately"), FMath::IsNearlyEqual(UGameplayStatics::ApplyDamage(Minion, 5.0f, nullptr, Attacker, nullptr), 5.0f));
	TestTrue(TEXT("A fresh ball can be destroyed with its authored 14 health"), FMath::IsNearlyEqual(UGameplayStatics::ApplyDamage(EarlyVictim, 100000.0f, nullptr, Attacker, nullptr), 14.0f));
	TestEqual(TEXT("Killing a rolling robot prevents deployment"), static_cast<uint8>(EarlyVictim->GetDeploymentPhase()), static_cast<uint8>(ETunaSweeperRollingRobotPhase::Dead));
	// Ground the unmoved body explicitly; character physics is outside this timing test.
	Minion->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	static_cast<AActor*>(Minion)->Tick(2.9f);
	TestEqual(TEXT("Players have nearly three seconds to shoot the rolling ball"), static_cast<uint8>(Minion->GetDeploymentPhase()), static_cast<uint8>(ETunaSweeperRollingRobotPhase::Rolling));
	static_cast<AActor*>(Minion)->Tick(0.11f);
	TestEqual(TEXT("Three-second rollout starts unfolding"), static_cast<uint8>(Minion->GetDeploymentPhase()), static_cast<uint8>(ETunaSweeperRollingRobotPhase::Unfolding));
	static_cast<AActor*>(Minion)->Tick(1.0f);
	TestEqual(TEXT("Unfolding remains vulnerable and cannot attack for over a second"), static_cast<uint8>(Minion->GetDeploymentPhase()), static_cast<uint8>(ETunaSweeperRollingRobotPhase::Unfolding));
	TestTrue(TEXT("Incomplete unfolding suppresses melee"), Minion->IsStandardCombatSuppressed());
	static_cast<AActor*>(Minion)->Tick(0.21f);
	TestEqual(TEXT("The robot begins walking after the full 1.2-second unfold"), static_cast<uint8>(Minion->GetDeploymentPhase()), static_cast<uint8>(ETunaSweeperRollingRobotPhase::Walking));
	TestFalse(TEXT("Completed robot can enter normal melee AI"), Minion->IsStandardCombatSuppressed());
	TestTrue(TEXT("Unfolding restores the slow teaching walking speed"), FMath::IsNearlyEqual(Minion->GetCharacterMovement()->MaxWalkSpeed, 180.0f, 0.1f));
	TestTrue(TEXT("Deployment preserves damage inflicted during rollout"), FMath::IsNearlyEqual(RemainingHealth(Minion), 9.0f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperTeachingMinibossDeathCleanupTest,
	"TunaSweeper.Combat.TeachingMinibosses.DeathCancelsLessons",
	TunaSweeperTeachingMinibossTests::TestFlags)

bool FTunaSweeperTeachingMinibossDeathCleanupTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace TunaSweeperTeachingMinibossTests;
	FTestWorld TestWorld;
	if (!TestNotNull(TEXT("Teaching test world exists"), TestWorld.World)) { return false; }
	ATunaSweeperEnemyCharacter* Teachers[] = {
		TestWorld.SpawnEnemy<ATunaSweeperChargeTeachingMiniboss>(FVector(0, 0, 88)),
		TestWorld.SpawnEnemy<ATunaSweeperRobotTeachingMiniboss>(FVector(0, 1000, 100)) };
	for (int32 Index = 0; Index < 2; ++Index)
	{
		ATunaSweeperEnemyCharacter* Teacher = Teachers[Index];
		ATunaSweeperEnemyCharacter* Target = TestWorld.SpawnEnemy(FVector(800, Index * 1000, 88), 1, 100.0f);
		if (!Teacher || !Target) { AddError(TEXT("Cleanup lesson actors failed to spawn")); return false; }
		UTunaSweeperCombatPatternComponent* Pattern = Teacher->GetCombatPatternComponent();
		if (!TestTrue(TEXT("Lesson starts before teacher death"), Pattern->TryStartAutomaticPattern(Target))) { return false; }
		TickPattern(Pattern, 1.0f);
		UGameplayStatics::ApplyDamage(Teacher, 100000.0f, nullptr, Target, nullptr);
		TestTrue(TEXT("Teacher dies while its lesson is being announced"), Teacher->IsDead());
		TestFalse(TEXT("Death immediately cancels the announced lesson"), Pattern->IsPatternActive());
		TestEqual(TEXT("Death removes the lane warning"), TestWorld.FindActors<ATunaSweeperAttackTelegraph>().Num(), 0);
		TestEqual(TEXT("Death removes the summoner preparation cue"), TestWorld.OwnedEffectCount(Teacher), 0);
		TestFalse(TEXT("Dead owner cannot schedule further pattern ticks"),
			Pattern->IsRegistered() && Pattern->IsComponentTickEnabled());
		TestEqual(TEXT("Cancelled preparation cannot later emit robots"), TestWorld.LivingMinions().Num(), 0);
		TestTrue(TEXT("A cancelled lesson cannot injure its target"), FMath::IsNearlyEqual(RemainingHealth(Target), 100.0f));
	}
	TestEqual(TEXT("Cleanup never creates a missile turret"), TestWorld.FindActors<ATunaSweeperMissileTurret>().Num(), 0);
	return true;
}
#endif
