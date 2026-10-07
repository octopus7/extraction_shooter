#if WITH_DEV_AUTOMATION_TESTS

#include "Combat/TunaSweeperArmor.h"
#include "Combat/TunaSweeperProjectileDamage.h"
#include "Component/TunaSweeperVitalsComponent.h"
#include "Component/TunaSweeperScratchComponent.h"
#include "Combat/TunaSweeperCombatValue.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Interaction/TunaSweeperBreakableTomatoComponent.h"
#include "Misc/ScopeExit.h"
#include "Misc/AutomationTest.h"
#include "Subsystem/TunaSweeperDifficultySubsystem.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperIntegerCombatValuesTest,
	"TunaSweeper.Combat.IntegerValues.FinalRounding",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaSweeperIntegerCombatValuesTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Final damage below half rounds down"), TunaSweeperArmor::ApplyDefense(20, 3.75f), 16.0f);
	TestEqual(TEXT("Final damage above half rounds up"), TunaSweeperArmor::ApplyDefense(20, 3.25f), 17.0f);
	TestEqual(TEXT("Exact half rounds up"), TunaSweeperArmor::ApplyDefense(20, 3.5f), 17.0f);
	TestEqual(TEXT("Armor fractions are kept until subtraction"), TunaSweeperArmor::ApplyDefense(20.25f, 3.75f), 17.0f);
	TestEqual(TEXT("Sub-half damage becomes zero, not a minimum-one hit"), TunaSweeperArmor::ApplyDefense(.49f, 0), 0.0f);
	TestTrue(TEXT("Projectile snapshot keeps intermediate precision"), FMath::IsNearlyEqual(
		static_cast<float>(TunaSweeperProjectileDamage::Calculate(12.49f, 1.15f, 2)), 16.3635f, .0001f));
	TestEqual(TEXT("Difficulty scaling keeps intermediate precision"),
		UTunaSweeperDifficultySubsystem::ScaleEnemyIncomingDamage(11, 5000), 5.5f);
	FTunaSweeperVitalsState State;
	State.MaxHealth = 100.6f;
	State.Health = 66.5f;
	State.Food = 80.4f;
	State.Hydration = 72.6f;
	State.Normalize();
	TestEqual(TEXT("Maximum health is integer-valued"), State.MaxHealth, 101.0f);
	TestEqual(TEXT("Health is integer-valued"), State.Health, 67.0f);
	TestEqual(TEXT("Food is integer-valued"), State.Food, 80.0f);
	TestEqual(TEXT("Hydration is integer-valued"), State.Hydration, 73.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperIntegerGaugeTest,
	"TunaSweeper.Combat.IntegerValues.GaugesAndContinuousRates",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaSweeperIntegerGaugeTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	if (!TestNotNull(TEXT("Gauge world"), World)) return false;
	ON_SCOPE_EXIT { World->DestroyWorld(false); World->RemoveFromRoot(); };
	AActor* Owner = World->SpawnActor<AActor>();
	if (!TestNotNull(TEXT("Gauge owner"), Owner)) return false;
	UTunaSweeperVitalsComponent* Vitals = NewObject<UTunaSweeperVitalsComponent>(Owner);
	Vitals->RegisterComponent();
	FTunaSweeperVitalsState State;
	State.Health = State.Food = State.Hydration = 80;
	Vitals->SetVitalsState(State);
	FTunaSweeperVitalsDelta Effect;
	Effect.Health = -.5f; Effect.Food = 1.49f; Effect.Hydration = -1.5f;
	Vitals->ApplyVitalsDelta(Effect);
	TestEqual(TEXT("Discrete damage rounds magnitude once"), Vitals->GetVitalsState().Health, 79.f);
	TestEqual(TEXT("Discrete healing rounds magnitude once"), Vitals->GetVitalsState().Food, 81.f);
	TestEqual(TEXT("Half-point action cost rounds up"), Vitals->GetVitalsState().Hydration, 78.f);
	Vitals->SetMaxVitals(125.4f, 150.6f, 80.5f, true);
	TestEqual(TEXT("Maximum value rounds before percent preservation"), Vitals->GetVitalsState().MaxHydration, 81.f);
	TestEqual(TEXT("Percent preservation keeps whole health"), Vitals->GetVitalsState().Health, 99.f);

	FTunaSweeperVitalsDepletionRates Rates;
	Rates.HealthPerSecond = .1f; Rates.FoodPerSecond = .03f; Rates.HydrationPerSecond = .04f;
	Vitals->SetBaseDepletionRates(Rates);
	FTunaSweeperVitalsDepletionRates NoAdditionalRates;
	NoAdditionalRates.HealthPerSecond = NoAdditionalRates.FoodPerSecond = NoAdditionalRates.HydrationPerSecond = 0;
	Vitals->SetDepletionRateAdditions(NoAdditionalRates);
	for (const int32 FPS : {30, 60, 144})
	{
		Vitals->SetVitalsState(FTunaSweeperVitalsState());
		bool bAllWhole = true;
		for (int32 Frame = 0; Frame < FPS * 100; ++Frame)
		{
			Vitals->TickComponent(1.f / FPS, LEVELTICK_All, nullptr);
			const auto& Current = Vitals->GetVitalsState();
			bAllWhole &= Current.Health == FMath::RoundToFloat(Current.Health) &&
				Current.Food == FMath::RoundToFloat(Current.Food) && Current.Hydration == FMath::RoundToFloat(Current.Hydration);
			// Routine unchanged max-stat refreshes must not erase fractional depletion.
			Vitals->SetMaxVitals(100, 100, 100, true);
		}
		TestTrue(TEXT("All replicated gauge values are whole every frame"), bAllWhole);
		TestEqual(TEXT("Health depletion retains rate across frame rates"), Vitals->GetVitalsState().Health, 90.f);
		TestEqual(TEXT("Food depletion retains slow rate across frame rates"), Vitals->GetVitalsState().Food, 97.f);
		TestEqual(TEXT("Hydration depletion retains slow rate across frame rates"), Vitals->GetVitalsState().Hydration, 96.f);
	}
	Rates.HealthPerSecond = Rates.FoodPerSecond = Rates.HydrationPerSecond = .4f;
	Vitals->SetBaseDepletionRates(Rates);
	Vitals->TickComponent(1, LEVELTICK_All, nullptr);
	Vitals->SetVitalsState(FTunaSweeperVitalsState());
	Vitals->TickComponent(2, LEVELTICK_All, nullptr);
	TestEqual(TEXT("Loading state clears transient depletion remainder"), Vitals->GetVitalsState().Food, 100.f);
	Vitals->TickComponent(.5f, LEVELTICK_All, nullptr);
	TestEqual(TEXT("Sub-point rates eventually consume a full point"), Vitals->GetVitalsState().Food, 99.f);

	double Remainder = 0;
	float Gauge = TunaSweeperCombatValue::Accumulate(0, 100, -50.75f, Remainder);
	TestEqual(TEXT("Empty gauge discards excess drain"), Remainder, 0.0);
	Gauge = TunaSweeperCombatValue::Accumulate(Gauge, 100, .75f, Remainder);
	TestEqual(TEXT("Partial recovery is held separately"), Gauge, 0.f);
	Gauge = TunaSweeperCombatValue::Accumulate(Gauge, 100, .25f, Remainder);
	TestEqual(TEXT("Accumulated recovery applies one whole point"), Gauge, 1.f);
	Gauge = TunaSweeperCombatValue::Accumulate(99, 100, 5.25f, Remainder);
	TestEqual(TEXT("Full gauge clamps"), Gauge, 100.f);
	TestEqual(TEXT("Full gauge discards excess recovery"), Remainder, 0.0);
	Gauge = TunaSweeperCombatValue::Accumulate(Gauge, 100, -1.f, Remainder);
	TestEqual(TEXT("Capped healing cannot absorb future cost"), Gauge, 99.f);
	Remainder = 0;
	Gauge = TunaSweeperCombatValue::Accumulate(100, 100, -.9f, Remainder);
	Gauge = TunaSweeperCombatValue::Accumulate(Gauge, 100, .1f, Remainder);
	Gauge = TunaSweeperCombatValue::Accumulate(Gauge, 100, -.2f, Remainder);
	TestEqual(TEXT("A short rest at max cannot erase accumulated sprint cost"), Gauge, 99.f);
	Remainder = 0;
	Gauge = TunaSweeperCombatValue::Accumulate(0, 100, .9f, Remainder);
	Gauge = TunaSweeperCombatValue::Accumulate(Gauge, 100, -.1f, Remainder);
	Gauge = TunaSweeperCombatValue::Accumulate(Gauge, 100, .2f, Remainder);
	TestEqual(TEXT("A short drain at zero cannot erase accumulated recovery"), Gauge, 1.f);

	UTunaSweeperScratchComponent* Scratch = NewObject<UTunaSweeperScratchComponent>(Owner);
	Scratch->MaxScratch = 100.5f;
	Scratch->SetCurrentScratch(42.5f);
	TestEqual(TEXT("Scratch maximum is integer-valued"), Scratch->GetMaxScratch(), 101.f);
	TestEqual(TEXT("Scratch current value is integer-valued"), Scratch->GetCurrentScratch(), 43.f);
	TestTrue(TEXT("Scratch accepts rounded cost"), Scratch->TryConsumeScratch(1.5f));
	TestEqual(TEXT("Scratch spends exactly two whole points"), Scratch->GetCurrentScratch(), 41.f);
	TestFalse(TEXT("Sub-half scratch cost is not a free successful action"), Scratch->TryConsumeScratch(.49f));
	Scratch->ResetScratch();
	TestEqual(TEXT("Scratch reset stays whole"), Scratch->GetCurrentScratch(), 0.f);
	UTunaSweeperBreakableTomatoComponent* Tomato = NewObject<UTunaSweeperBreakableTomatoComponent>(Owner);
	Tomato->SetMaxHealth(1.5f);
	TestEqual(TEXT("Tomato maximum rounds to whole health"), Tomato->GetCurrentHealth(), 2.f);
	TestFalse(TEXT("Sub-half tomato damage does not break it"), Tomato->ApplyTomatoDamage(.49f));
	TestEqual(TEXT("Sub-half tomato damage leaves health unchanged"), Tomato->GetCurrentHealth(), 2.f);
	TestFalse(TEXT("Half-point tomato damage rounds up"), Tomato->ApplyTomatoDamage(.5f));
	TestEqual(TEXT("Tomato has whole remaining health"), Tomato->GetCurrentHealth(), 1.f);
	TestTrue(TEXT("Final whole tomato damage breaks it"), Tomato->ApplyTomatoDamage(1.f));
	TestEqual(TEXT("Broken tomato has zero health"), Tomato->GetCurrentHealth(), 0.f);
	Tomato->SetMaxHealth(.1f);
	TestEqual(TEXT("Reset tomato has at least one whole health"), Tomato->GetCurrentHealth(), 1.f);
	TestFalse(TEXT("Reset tomato is intact"), Tomato->IsBroken());
	return true;
}

#endif
