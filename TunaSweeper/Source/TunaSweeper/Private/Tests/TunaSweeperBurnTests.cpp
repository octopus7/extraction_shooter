#if WITH_DEV_AUTOMATION_TESTS

#include "AI/TunaSweeperEnemyCharacter.h"
#include "Combat/TunaSweeperBurnTypes.h"
#include "Component/TunaSweeperBurnComponent.h"
#include "Component/TunaSweeperFactionComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "RenderingThread.h"
#include "Subsystem/TunaSweeperFactionSubsystem.h"

#include <limits>

namespace TunaSweeperBurnTests
{
	constexpr EAutomationTestFlags TestFlags =
		EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;

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
				MakeUniqueObjectName(GetTransientPackage(), UWorld::StaticClass(), TEXT("BurnTestWorld")),
				GetTransientPackage(), true, ERHIFeatureLevel::Num, &Values);
			if (World && GEngine) GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
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

		ATunaSweeperEnemyCharacter* SpawnEnemy(float Health = 100.0f, uint8 Faction = TunaSweeperFactionIds::Enemy)
		{
			ATunaSweeperEnemyCharacter* Enemy = World->SpawnActorDeferred<ATunaSweeperEnemyCharacter>(
				ATunaSweeperEnemyCharacter::StaticClass(), FTransform::Identity, nullptr,
				nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
			if (!Enemy) return nullptr;
			Enemy->AutoPossessAI = EAutoPossessAI::Disabled;
			Enemy->ConfigureCombatProfile(FTunaSweeperEnemyCombatProfile(), Faction, NAME_None, INDEX_NONE);
			Enemy->ConfigureSpawnData(TSoftObjectPtr<UMaterialInterface>(), NAME_None, INDEX_NONE, INDEX_NONE, Health, 0);
			Enemy->FinishSpawning(FTransform::Identity);
			return Enemy;
		}
	};

	FTunaSweeperBurnSpec DefaultBurn()
	{
		FTunaSweeperBurnSpec Spec;
		Spec.bEnabled = true;
		return Spec;
	}

	void Advance(UTunaSweeperBurnComponent* Burn, float Seconds)
	{
		static_cast<UActorComponent*>(Burn)->TickComponent(Seconds, LEVELTICK_All, nullptr);
	}

	float ConsumeRemainingHealth(AActor* Enemy)
	{
		// TakeDamage returns only damage actually applied, allowing health verification through its public API.
		return UGameplayStatics::ApplyDamage(Enemy, 1000000.0f, nullptr, nullptr, nullptr);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperBurnTimingTest,
	"TunaSweeper.Combat.Burn.FiveTicksAndLongFrames", TunaSweeperBurnTests::TestFlags)

bool FTunaSweeperBurnTimingTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace TunaSweeperBurnTests;
	FTestWorld TestWorld;
	if (!TestNotNull(TEXT("Burn test world exists"), TestWorld.World)) return false;

	struct FScenario { float StepSeconds; int32 Steps; float ExpectedHealth; bool bExpectedBurning; };
	const FScenario Scenarios[] =
	{
		{ 0.0f, 1, 100.0f, true },
		{ 0.99f, 1, 100.0f, true },
		{ 1.0f, 1, 98.0f, true },
		{ 1.0f, 4, 92.0f, true },
		{ 1.0f, 5, 90.0f, false },
		{ 0.1f, 50, 90.0f, false },
		{ 20.0f, 1, 90.0f, false }
	};
	for (const FScenario& Scenario : Scenarios)
	{
		ATunaSweeperEnemyCharacter* Enemy = TestWorld.SpawnEnemy();
		if (!TestNotNull(TEXT("Enemy exists"), Enemy)) return false;
		UTunaSweeperBurnComponent* Burn = Enemy->FindComponentByClass<UTunaSweeperBurnComponent>();
		if (!TestNotNull(TEXT("Every enemy owns its burn component"), Burn)) return false;
		TestTrue(TEXT("Eligible enemy accepts burn"), Burn->TryApplyBurn(DefaultBurn(), nullptr, nullptr));
		for (int32 Step = 0; Step < Scenario.Steps; ++Step) Advance(Burn, Scenario.StepSeconds);
		const FString Label = FString::Printf(TEXT("%.2f seconds x %d"), Scenario.StepSeconds, Scenario.Steps);
		TestEqual(*FString::Printf(TEXT("%s active state"), *Label), Burn->IsBurning(), Scenario.bExpectedBurning);
		TestTrue(*FString::Printf(TEXT("%s exact tick damage"), *Label),
			FMath::IsNearlyEqual(ConsumeRemainingHealth(Enemy), Scenario.ExpectedHealth));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperBurnRefreshTest,
	"TunaSweeper.Combat.Burn.RefreshAndResearchStrength", TunaSweeperBurnTests::TestFlags)

bool FTunaSweeperBurnRefreshTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace TunaSweeperBurnTests;
	FTestWorld TestWorld;
	if (!TestNotNull(TEXT("Burn test world exists"), TestWorld.World)) return false;
	ATunaSweeperEnemyCharacter* Enemy = TestWorld.SpawnEnemy();
	if (!TestNotNull(TEXT("Enemy exists"), Enemy)) return false;
	UTunaSweeperBurnComponent* Burn = Enemy->FindComponentByClass<UTunaSweeperBurnComponent>();
	if (!TestNotNull(TEXT("Burn component exists"), Burn)) return false;
	FTunaSweeperBurnSpec Strong = DefaultBurn();
	Strong.DamageMultiplier = 3.0f;
	TestTrue(TEXT("Strong burn starts"), Burn->TryApplyBurn(Strong, nullptr, nullptr));
	Advance(Burn, 0.75f);
	TestTrue(TEXT("Weak reapplication refreshes burn"), Burn->TryApplyBurn(DefaultBurn(), nullptr, nullptr));
	TestEqual(TEXT("Weak reapplication stacks the strongest researched damage"), Burn->GetDamagePerTick(), 9.0f);
	TestEqual(TEXT("Reapplication grants a full five-second duration"), Burn->GetRemainingSeconds(), 5.0f);
	Advance(Burn, 0.25f);
	Advance(Burn, 4.0f);
	TestTrue(TEXT("Flames remain for the refreshed fractional duration"), Burn->IsBurning());
	Advance(Burn, 0.75f);
	TestFalse(TEXT("Refreshed burn still expires"), Burn->IsBurning());
	TestEqual(TEXT("Two stacks share one damage stream at one-and-a-half strength"), ConsumeRemainingHealth(Enemy), 55.0f);

	ATunaSweeperEnemyCharacter* PhaseEnemy = TestWorld.SpawnEnemy(1.0f);
	if (!TestNotNull(TEXT("Phase enemy exists"), PhaseEnemy)) return false;
	UTunaSweeperBurnComponent* PhaseBurn = PhaseEnemy->FindComponentByClass<UTunaSweeperBurnComponent>();
	PhaseBurn->TryApplyBurn(DefaultBurn(), nullptr, nullptr);
	Advance(PhaseBurn, 0.75f);
	PhaseBurn->TryApplyBurn(DefaultBurn(), nullptr, nullptr);
	Advance(PhaseBurn, 0.25f);
	TestTrue(TEXT("Rapid reapplication preserves progress to the first tick"), PhaseEnemy->IsDead());
	TestFalse(TEXT("Lethal tick clears burn during the damage callback"), PhaseBurn->IsBurning());
	TestEqual(TEXT("Lethal tick clears every stack"), PhaseBurn->GetStackCount(), 0);
	TestEqual(TEXT("Lethal tick clears effective damage"), PhaseBurn->GetDamagePerTick(), 0.0f);

	ATunaSweeperEnemyCharacter* ResearchEnemy = TestWorld.SpawnEnemy();
	if (!TestNotNull(TEXT("Research enemy exists"), ResearchEnemy)) return false;
	UTunaSweeperBurnComponent* ResearchBurn = ResearchEnemy->FindComponentByClass<UTunaSweeperBurnComponent>();
	FTunaSweeperBurnSpec Researched = DefaultBurn();
	Researched.TickCount = 8;
	Researched.DamageMultiplier = 2.0f;
	ResearchBurn->TryApplyBurn(Researched, nullptr, nullptr);
	Advance(ResearchBurn, 1.0f);
	ResearchBurn->TryApplyBurn(DefaultBurn(), nullptr, nullptr);
	TestEqual(TEXT("Short weak hit cannot shorten remaining research duration"), ResearchBurn->GetRemainingSeconds(), 7.0f);
	Advance(ResearchBurn, 20.0f);
	TestFalse(TEXT("Research extension remains finite"), ResearchBurn->IsBurning());
	TestEqual(TEXT("Research applies one four-damage tick and seven six-damage stacked ticks"), ConsumeRemainingHealth(ResearchEnemy), 54.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperBurnStackingTest,
	"TunaSweeper.Combat.Burn.ThreeStacksAndShotGrouping", TunaSweeperBurnTests::TestFlags)

bool FTunaSweeperBurnStackingTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace TunaSweeperBurnTests;
	FTestWorld TestWorld;
	if (!TestNotNull(TEXT("Stack test world exists"), TestWorld.World)) return false;
	ATunaSweeperEnemyCharacter* Enemy = TestWorld.SpawnEnemy();
	if (!TestNotNull(TEXT("Stack enemy exists"), Enemy)) return false;
	UTunaSweeperBurnComponent* Burn = Enemy->FindComponentByClass<UTunaSweeperBurnComponent>();
	if (!TestNotNull(TEXT("Stack burn component exists"), Burn)) return false;
	const FGuid ShotA = FGuid::NewGuid();
	const FGuid ShotB = FGuid::NewGuid();
	const FGuid ShotC = FGuid::NewGuid();

	TestTrue(TEXT("First shot ignites"), Burn->TryApplyBurn(DefaultBurn(), nullptr, nullptr, ShotA));
	TestEqual(TEXT("First shot grants one stack"), Burn->GetStackCount(), 1);
	TestEqual(TEXT("One stack deals base damage"), Burn->GetDamagePerTick(), 2.0f);
	Advance(Burn, 1.0f);
	TestTrue(TEXT("Another pellet from the same shot is accepted"), Burn->TryApplyBurn(DefaultBurn(), nullptr, nullptr, ShotA));
	TestEqual(TEXT("Repeated pellet cannot add a stack"), Burn->GetStackCount(), 1);
	TestEqual(TEXT("Repeated pellet refreshes the finite duration"), Burn->GetRemainingSeconds(), 5.0f);
	Burn->TryApplyBurn(DefaultBurn(), nullptr, nullptr, ShotB);
	TestEqual(TEXT("Second shot grants a second stack"), Burn->GetStackCount(), 2);
	TestEqual(TEXT("Two stacks deal one-and-a-half base damage"), Burn->GetDamagePerTick(), 3.0f);
	Advance(Burn, 1.0f);
	Burn->TryApplyBurn(DefaultBurn(), nullptr, nullptr, ShotA);
	Burn->TryApplyBurn(DefaultBurn(), nullptr, nullptr, ShotB);
	TestEqual(TEXT("Interleaved pellets from A and B cannot grant a third stack"), Burn->GetStackCount(), 2);
	Burn->TryApplyBurn(DefaultBurn(), nullptr, nullptr, ShotC);
	TestEqual(TEXT("Third distinct shot reaches the cap"), Burn->GetStackCount(), 3);
	TestEqual(TEXT("Three stacks deal twice base damage"), Burn->GetDamagePerTick(), 4.0f);
	Advance(Burn, 0.5f);
	for (int32 Shot = 0; Shot < 32; ++Shot)
	{
		Burn->TryApplyBurn(DefaultBurn(), nullptr, nullptr, FGuid::NewGuid());
	}
	TestEqual(TEXT("Further shots cannot exceed three stacks"), Burn->GetStackCount(), 3);
	TestEqual(TEXT("Capped damage remains twice base damage"), Burn->GetDamagePerTick(), 4.0f);
	TestEqual(TEXT("Repeated capped hits refresh to five seconds without adding duration"), Burn->GetRemainingSeconds(), 5.0f);
	Advance(Burn, 0.5f);
	Advance(Burn, 4.5f);
	TestFalse(TEXT("The entire stack expires five seconds after the final hit"), Burn->IsBurning());
	TestEqual(TEXT("Expiry clears all stacks"), Burn->GetStackCount(), 0);
	TestEqual(TEXT("Expiry clears effective damage"), Burn->GetDamagePerTick(), 0.0f);
	TestEqual(TEXT("Expiry clears remaining time"), Burn->GetRemainingSeconds(), 0.0f);
	Burn->TryApplyBurn(DefaultBurn(), nullptr, nullptr, ShotA);
	TestEqual(TEXT("A fresh ignition can reuse an expired application ID"), Burn->GetStackCount(), 1);
	TestEqual(TEXT("Fresh ignition starts at base damage"), Burn->GetDamagePerTick(), 2.0f);
	Burn->ClearBurn();
	TestEqual(TEXT("Explicit clearing removes the new stack"), Burn->GetStackCount(), 0);
	TestEqual(TEXT("Stack changes retain the one-second phase and deal exactly two plus three plus five four-damage ticks"),
		ConsumeRemainingHealth(Enemy), 75.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperBurnStackStrengthTest,
	"TunaSweeper.Combat.Burn.StackStrengthAndSourceAttribution", TunaSweeperBurnTests::TestFlags)

bool FTunaSweeperBurnStackStrengthTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace TunaSweeperBurnTests;
	FTestWorld TestWorld;
	if (!TestNotNull(TEXT("Stack attribution test world exists"), TestWorld.World)) return false;

	// Equal and stronger researched hits transfer attribution even below the existing stacked total;
	// weaker hits preserve the strongest source. Faction changes make source selection observable.
	const float IncomingBaseDamages[] = { 2.0f, 3.0f, 1.0f };
	for (float IncomingBaseDamage : IncomingBaseDamages)
	{
		ATunaSweeperEnemyCharacter* Enemy = TestWorld.SpawnEnemy();
		APawn* FirstSource = TestWorld.World->SpawnActor<APawn>();
		APawn* IncomingSource = TestWorld.World->SpawnActor<APawn>();
		if (!Enemy || !FirstSource || !IncomingSource) { AddError(TEXT("Stack attribution actors failed to spawn")); return false; }
		auto AddPlayerFaction = [](APawn* Source)
		{
			UTunaSweeperFactionComponent* Faction = NewObject<UTunaSweeperFactionComponent>(Source);
			Source->AddInstanceComponent(Faction);
			Faction->SetFactionId(TunaSweeperFactionIds::Player);
			Faction->RegisterComponent();
			return Faction;
		};
		UTunaSweeperFactionComponent* FirstFaction = AddPlayerFaction(FirstSource);
		UTunaSweeperFactionComponent* IncomingFaction = AddPlayerFaction(IncomingSource);
		UTunaSweeperBurnComponent* Burn = Enemy->FindComponentByClass<UTunaSweeperBurnComponent>();
		if (!TestNotNull(TEXT("Stack attribution burn exists"), Burn)) return false;
		for (int32 Stack = 0; Stack < 3; ++Stack)
		{
			Burn->TryApplyBurn(DefaultBurn(), nullptr, FirstSource);
		}
		TestEqual(TEXT("Independent applications without IDs reach three stacks"), Burn->GetStackCount(), 3);
		FTunaSweeperBurnSpec Incoming = DefaultBurn();
		Incoming.BaseDamagePerTick = IncomingBaseDamage;
		Burn->TryApplyBurn(Incoming, nullptr, IncomingSource);
		const float ExpectedBaseDamage = FMath::Max(2.0f, IncomingBaseDamage);
		TestEqual(TEXT("Pre-stack strength is retained and the multiplier is applied once"),
			Burn->GetDamagePerTick(), ExpectedBaseDamage * 2.0f);
		const bool bIncomingSourceRetained = IncomingBaseDamage >= 2.0f;
		UTunaSweeperFactionComponent* RetainedFaction = bIncomingSourceRetained ? IncomingFaction : FirstFaction;
		UTunaSweeperFactionComponent* DiscardedFaction = bIncomingSourceRetained ? FirstFaction : IncomingFaction;
		DiscardedFaction->SetFactionId(TunaSweeperFactionIds::Enemy);
		Advance(Burn, 1.0f);
		TestTrue(TEXT("A discarded source becoming friendly does not cancel burn"), Burn->IsBurning());
		RetainedFaction->SetFactionId(TunaSweeperFactionIds::Enemy);
		Advance(Burn, 1.0f);
		TestFalse(TEXT("The retained source becoming friendly cancels burn"), Burn->IsBurning());
		TestEqual(TEXT("Source rejection clears all stacks"), Burn->GetStackCount(), 0);
		TestEqual(TEXT("Exactly one tick uses the strongest base with the stack multiplier"),
			ConsumeRemainingHealth(Enemy), 100.0f - ExpectedBaseDamage * 2.0f);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperBurnEligibilityTest,
	"TunaSweeper.Combat.Burn.EnemiesOnlyAndCleanup", TunaSweeperBurnTests::TestFlags)

bool FTunaSweeperBurnEligibilityTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace TunaSweeperBurnTests;
	FTestWorld TestWorld;
	if (!TestNotNull(TEXT("Burn test world exists"), TestWorld.World)) return false;
	AActor* Prop = TestWorld.World->SpawnActor<AActor>();
	ATunaSweeperEnemyCharacter* Enemy = TestWorld.SpawnEnemy();
	ATunaSweeperEnemyCharacter* Friendly = TestWorld.SpawnEnemy();
	ATunaSweeperEnemyCharacter* PlayerFaction = TestWorld.SpawnEnemy(100.0f, TunaSweeperFactionIds::Player);
	if (!Prop || !Enemy || !Friendly || !PlayerFaction) { AddError(TEXT("Eligibility actors failed to spawn")); return false; }
	TestFalse(TEXT("Props cannot burn"), UTunaSweeperBurnComponent::CanBurnActor(Prop));
	TestFalse(TEXT("Player-faction actors cannot burn even with the enemy class"), UTunaSweeperBurnComponent::CanBurnActor(PlayerFaction));
	UTunaSweeperBurnComponent* PropBurn = NewObject<UTunaSweeperBurnComponent>(Prop);
	Prop->AddInstanceComponent(PropBurn);
	PropBurn->RegisterComponent();
	TestFalse(TEXT("Adding the component to a prop cannot bypass eligibility"), PropBurn->TryApplyBurn(DefaultBurn(), nullptr, nullptr));
	UTunaSweeperBurnComponent* Burn = Enemy->FindComponentByClass<UTunaSweeperBurnComponent>();
	TestFalse(TEXT("Same-faction sources cannot ignite an enemy"), Burn->TryApplyBurn(DefaultBurn(), nullptr, Friendly));
	TestFalse(TEXT("Disabled shot specifications do not ignite"), Burn->TryApplyBurn(FTunaSweeperBurnSpec(), nullptr, nullptr));
	TestTrue(TEXT("Hostile player source can ignite"), Burn->TryApplyBurn(DefaultBurn(), nullptr, PlayerFaction));
	Enemy->GetFactionComponent()->SetCanBeCombatTarget(false);
	Advance(Burn, 1.0f);
	TestFalse(TEXT("Loss of combat eligibility clears burn before damage"), Burn->IsBurning());
	Enemy->GetFactionComponent()->SetCanBeCombatTarget(true);
	TestEqual(TEXT("Ineligible tick does no damage"), ConsumeRemainingHealth(Enemy), 100.0f);
	TestFalse(TEXT("Dead enemies reject burn"), Burn->TryApplyBurn(DefaultBurn(), nullptr, PlayerFaction));

	UTunaSweeperBurnComponent* FriendlyBurn = Friendly->FindComponentByClass<UTunaSweeperBurnComponent>();
	FriendlyBurn->TryApplyBurn(DefaultBurn(), nullptr, PlayerFaction);
	ConsumeRemainingHealth(Friendly);
	TestFalse(TEXT("External lethal damage clears burn immediately without another component tick"), FriendlyBurn->IsBurning());
	TestEqual(TEXT("External lethal damage clears the stack count"), FriendlyBurn->GetStackCount(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperBurnSourceLifetimeTest,
	"TunaSweeper.Combat.Burn.SourceSurvivesProjectile", TunaSweeperBurnTests::TestFlags)

bool FTunaSweeperBurnSourceLifetimeTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace TunaSweeperBurnTests;
	FTestWorld TestWorld;
	if (!TestNotNull(TEXT("Burn test world exists"), TestWorld.World)) return false;
	ATunaSweeperEnemyCharacter* Enemy = TestWorld.SpawnEnemy();
	APawn* Shooter = TestWorld.World->SpawnActor<APawn>();
	APlayerController* Controller = TestWorld.World->SpawnActor<APlayerController>();
	AActor* Projectile = TestWorld.World->SpawnActor<AActor>();
	if (!Enemy || !Shooter || !Controller || !Projectile) { AddError(TEXT("Attribution actors failed to spawn")); return false; }
	UTunaSweeperFactionComponent* ShooterFaction = NewObject<UTunaSweeperFactionComponent>(Shooter);
	Shooter->AddInstanceComponent(ShooterFaction);
	ShooterFaction->SetFactionId(TunaSweeperFactionIds::Player);
	ShooterFaction->RegisterComponent();
	Controller->Possess(Shooter);
	Projectile->SetOwner(Shooter);
	Projectile->SetInstigator(Shooter);
	UTunaSweeperBurnComponent* Burn = Enemy->FindComponentByClass<UTunaSweeperBurnComponent>();
	TestTrue(TEXT("Projectile starts player-attributed burn"), Burn->TryApplyBurn(DefaultBurn(), Controller, Projectile));
	Projectile->Destroy();
	Advance(Burn, 1.0f);
	TestTrue(TEXT("Projectile destruction does not cancel burn"), Burn->IsBurning());
	ShooterFaction->SetFactionId(TunaSweeperFactionIds::Enemy);
	Advance(Burn, 1.0f);
	TestFalse(TEXT("Later faction checks retain the original firing pawn after projectile destruction"), Burn->IsBurning());
	TestEqual(TEXT("Only the tick before the source became friendly deals damage"), ConsumeRemainingHealth(Enemy), 98.0f);
	Controller->UnPossess();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperBurnSanitizationTest,
	"TunaSweeper.Combat.Burn.FiniteSpecification", TunaSweeperBurnTests::TestFlags)

bool FTunaSweeperBurnSanitizationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace TunaSweeperBurnTests;
	FTunaSweeperBurnSpec Spec = DefaultBurn();
	Spec.TickCount = MAX_int32;
	Spec.BaseDamagePerTick = MAX_flt;
	Spec.DamageMultiplier = MAX_flt;
	Spec.Normalize();
	TestEqual(TEXT("Excessive duration is bounded"), Spec.TickCount, FTunaSweeperBurnSpec::MaxTickCount);
	TestEqual(TEXT("Damage multiplication remains finite and bounded"), Spec.GetDamagePerTick(), FTunaSweeperBurnSpec::MaxDamagePerTick);
	Spec.BaseDamagePerTick = std::numeric_limits<float>::quiet_NaN();
	Spec.Normalize();
	TestFalse(TEXT("NaN damage cannot start a burn"), Spec.bEnabled);
	Spec = DefaultBurn();
	Spec.DamageMultiplier = std::numeric_limits<float>::infinity();
	Spec.Normalize();
	TestFalse(TEXT("Infinite research multiplier cannot start a burn"), Spec.bEnabled);
	Spec = DefaultBurn();
	Spec.BaseDamagePerTick = -2.0f;
	Spec.Normalize();
	TestFalse(TEXT("Negative damage cannot heal through burn"), Spec.bEnabled);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperBurnVisualLifecycleTest,
	"TunaSweeper.Combat.Burn.FlameLifecycle", TunaSweeperBurnTests::TestFlags | EAutomationTestFlags::NonNullRHI)

bool FTunaSweeperBurnVisualLifecycleTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace TunaSweeperBurnTests;
	FTestWorld TestWorld;
	if (!TestNotNull(TEXT("Burn presentation world exists"), TestWorld.World)) return false;
	ATunaSweeperEnemyCharacter* Enemy = TestWorld.SpawnEnemy();
	if (!TestNotNull(TEXT("Presentation enemy exists"), Enemy)) return false;
	UTunaSweeperBurnComponent* Burn = Enemy->FindComponentByClass<UTunaSweeperBurnComponent>();
	if (!TestNotNull(TEXT("Presentation burn component exists"), Burn)) return false;
	UNiagaraSystem* Effect = LoadObject<UNiagaraSystem>(nullptr,
		TEXT("/Game/Effects/NS_ExplosiveBarrel_Burning.NS_ExplosiveBarrel_Burning"));
	if (!TestNotNull(TEXT("Default rising-flame asset loads"), Effect)) return false;
	Effect->EnsureFullyLoaded();
#if WITH_EDITORONLY_DATA
	Effect->WaitForCompilationComplete(true, false);
#endif
	// Explicit gameplay and component ticks avoid starting unrelated AI or game-instance systems.
	TestWorld.World->SetBegunPlay(true);
	FlushRenderingCommands();
	TestTrue(TEXT("Burn starts the real Niagara presentation"), Burn->TryApplyBurn(DefaultBurn(), nullptr, nullptr));
	TArray<UNiagaraComponent*> Flames;
	Enemy->GetComponents(Flames);
	Flames.RemoveAll([Effect](const UNiagaraComponent* Component) { return Component->GetAsset() != Effect; });
	if (!TestEqual(TEXT("One enemy-owned flame component exists"), Flames.Num(), 1)) return false;
	UNiagaraComponent* Flame = Flames[0];
	TestTrue(TEXT("Flame is owned by the enemy for visibility filtering"), Flame->GetOwner() == Enemy);
	TestTrue(TEXT("Flame follows the enemy root"), Flame->GetAttachParent() == Enemy->GetRootComponent());
	TestTrue(TEXT("Flame is registered for rendering"), Flame->IsRegistered());
	bool bFadeIsValid = false;
	TestEqual(TEXT("Flame fade parameter is fully visible"), Flame->GetVariableFloat(TEXT("User.Fade"), bFadeIsValid), 1.0f);
	TestTrue(TEXT("Flame asset exposes the expected fade parameter"), bFadeIsValid);
	Enemy->SetActorRotation(FRotator(25.0f, 65.0f, 15.0f));
	TestTrue(TEXT("Flames stay upright while the enemy rotates"), Flame->GetComponentRotation().IsNearlyZero());
	if (TestTrue(TEXT("Niagara simulation initialized"), Flame->GetSystemInstanceController().IsValid()))
	{
		Flame->AdvanceSimulation(480, 1.0f / 60.0f);
		TestFalse(TEXT("Rising flames continue beyond the default five-second lifetime"), Flame->IsComplete());
	}
	Burn->TryApplyBurn(DefaultBurn(), nullptr, nullptr);
	TArray<UNiagaraComponent*> RefreshedFlames;
	Enemy->GetComponents(RefreshedFlames);
	RefreshedFlames.RemoveAll([Effect](const UNiagaraComponent* Component) { return Component->GetAsset() != Effect; });
	TestEqual(TEXT("Reapplication preserves one flame instance"), RefreshedFlames.Num(), 1);
	TestTrue(TEXT("Reapplication does not restart the effect"), RefreshedFlames.Num() == 1 && RefreshedFlames[0] == Flame);
	Advance(Burn, 5.0f);
	TestFalse(TEXT("Expiry stops burn"), Burn->IsBurning());
	TestFalse(TEXT("Expiry unregisters the flame immediately"), Flame->IsRegistered());
	Burn->TryApplyBurn(DefaultBurn(), nullptr, nullptr);
	TArray<UNiagaraComponent*> ReignitedFlames;
	Enemy->GetComponents(ReignitedFlames);
	ReignitedFlames.RemoveAll([Effect](const UNiagaraComponent* Component) { return Component->GetAsset() != Effect; });
	TestEqual(TEXT("A later ignition creates one new flame"), ReignitedFlames.Num(), 1);
	ConsumeRemainingHealth(Enemy);
	TestFalse(TEXT("Death immediately clears burn presentation"), Burn->IsBurning());
	if (ReignitedFlames.Num() == 1)
	{
		TestFalse(TEXT("Death unregisters the flame"), ReignitedFlames[0]->IsRegistered());
	}
	TestWorld.World->SetBegunPlay(false);
	return true;
}
#endif // WITH_DEV_AUTOMATION_TESTS
