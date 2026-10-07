#if WITH_DEV_AUTOMATION_TESTS

#include "Character/TunaSweeperTopDownCharacter.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTunaSweeperDemoStaminaTuningTest,
	"TunaSweeper.Player.Stamina.DemoTuning",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaSweeperDemoStaminaTuningTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	ON_SCOPE_EXIT
	{
		World->DestroyWorld(false);
		GEngine->DestroyWorldContext(World);
		World->RemoveFromRoot();
	};

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ATunaSweeperTopDownCharacter* Character = World->SpawnActor<ATunaSweeperTopDownCharacter>(
		ATunaSweeperTopDownCharacter::StaticClass(),
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		SpawnParameters);
	if (!TestNotNull(TEXT("Player character"), Character))
	{
		return false;
	}

	Character->MaxStamina = 100.0f;
	Character->CurrentStamina = 100.0f;
	Character->bSprintInputHeld = true;
	Character->CurrentMoveInput = FVector2D(1.0f, 0.0f);
	Character->UpdateSprintAndStamina(1.0f);
	TestEqual(TEXT("One second of sprint applies six whole stamina points"), Character->CurrentStamina, 94.0f);
	Character->UpdateSprintAndStamina(3.0f);
	TestEqual(TEXT("Four seconds preserve the exact 6.25 per-second depletion rate"), Character->CurrentStamina, 75.0f);

	Character->CurrentStamina = 0.0f;
	Character->StaminaRemainder = 0.0;
	Character->bSprintInputHeld = false;
	Character->UpdateSprintAndStamina(1.0f);
	TestEqual(TEXT("One second of demo recovery restores three times the previous stamina rate"), Character->CurrentStamina, 54.0f);
	for (const int32 FPS : {30, 60, 144})
	{
		Character->CurrentStamina = 100;
		Character->StaminaRemainder = 0;
		Character->bSprintInputHeld = true;
		for (int32 Frame = 0; Frame < FPS * 4; ++Frame)
		{
			Character->UpdateSprintAndStamina(1.f / FPS);
			TestEqual(TEXT("Stamina stays integer every frame"), Character->CurrentStamina, FMath::RoundToFloat(Character->CurrentStamina));
		}
		TestEqual(TEXT("Sprint rate is independent of frame rate"), Character->CurrentStamina, 75.f);
	}

	return true;
}

#endif
