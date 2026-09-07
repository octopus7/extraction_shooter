#if WITH_DEV_AUTOMATION_TESTS

#include "Subsystem/TunaSweeperDifficultySubsystem.h"
#include "Subsystem/TunaSweeperTextSubsystem.h"

#include "Inventory/TunaSweeperSaveGame.h"
#include "Engine/GameInstance.h"
#include "Misc/AutomationTest.h"

namespace TunaSweeperDifficultyTests
{
	constexpr EAutomationTestFlags TestFlags =
		EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTunaSweeperDifficultyDamageScalingTest,
	"TunaSweeper.Difficulty.EnemyIncomingDamageScaling",
	TunaSweeperDifficultyTests::TestFlags)

bool FTunaSweeperDifficultyDamageScalingTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	TestEqual(TEXT("Stage 1 raw multiplier halves enemy damage"), UTunaSweeperDifficultySubsystem::ScaleEnemyIncomingDamage(10.0f, 5000), 5.0f);
	TestEqual(TEXT("Stage 2 raw multiplier preserves enemy damage"), UTunaSweeperDifficultySubsystem::ScaleEnemyIncomingDamage(10.0f, 10000), 10.0f);
	TestEqual(TEXT("Stage 3 raw multiplier doubles enemy damage"), UTunaSweeperDifficultySubsystem::ScaleEnemyIncomingDamage(10.0f, 20000), 20.0f);
	TestEqual(TEXT("Scaled damage rounds once before defense"), UTunaSweeperDifficultySubsystem::ScaleEnemyIncomingDamage(11.0f, 5000), 6.0f);
	TestEqual(TEXT("Multiplier is not probability-clamped"), UTunaSweeperDifficultySubsystem::ScaleEnemyIncomingDamage(10.0f, 35000), 35.0f);
	TestEqual(TEXT("Enemy multiplier is rounded before defense"), UTunaSweeperDifficultySubsystem::ResolveAppliedPlayerDamage(11.0f, 2, true, 5000), 4.0f);
	TestEqual(TEXT("Enemy defense can absorb the scaled result"), UTunaSweeperDifficultySubsystem::ResolveAppliedPlayerDamage(10.0f, 6, true, 5000), 0.0f);
	TestEqual(TEXT("Player friendly and environmental damage is not difficulty-scaled"), UTunaSweeperDifficultySubsystem::ResolveAppliedPlayerDamage(11.0f, 2, false, 20000), 9.0f);

	UTunaSweeperSaveGame* SaveGame = NewObject<UTunaSweeperSaveGame>();
	SaveGame->DifficultyStage = 3;
	TestEqual(TEXT("Difficulty stage remains slot-persisted"), SaveGame->DifficultyStage, 3);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTunaSweeperDifficultyAndNarrativeTextDataTest,
	"TunaSweeper.Difficulty.DefinitionsAndLocalizedTextData",
	TunaSweeperDifficultyTests::TestFlags)

bool FTunaSweeperDifficultyAndNarrativeTextDataTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	UGameInstance* GameInstance = NewObject<UGameInstance>();
	UTunaSweeperDifficultySubsystem* DifficultySubsystem = NewObject<UTunaSweeperDifficultySubsystem>(GameInstance);
	TestTrue(TEXT("Difficulty definitions load"), DifficultySubsystem->LoadDifficultyData());
	const int32 ExpectedMultipliers[] = {5000, 10000, 20000};
	for (int32 Stage = 1; Stage <= 3; ++Stage)
	{
		FTunaSweeperDifficultyDefinition Definition;
		TestTrue(FString::Printf(TEXT("Difficulty stage %d exists"), Stage), DifficultySubsystem->TryGetDifficultyDefinition(Stage, Definition));
		TestEqual(FString::Printf(TEXT("Difficulty stage %d multiplier"), Stage), Definition.EnemyIncomingDamageMultiplier, ExpectedMultipliers[Stage - 1]);
		TestFalse(FString::Printf(TEXT("Difficulty stage %d title key is set"), Stage), Definition.TitleStringKey.IsNone());
		TestFalse(FString::Printf(TEXT("Difficulty stage %d description key is set"), Stage), Definition.DescriptionStringKey.IsNone());
	}

	UTunaSweeperTextSubsystem* TextSubsystem = NewObject<UTunaSweeperTextSubsystem>(GameInstance);
	TestTrue(TEXT("UI, memo, and difficulty text CSV files load together"), TextSubsystem->LoadTextData());
	FText ResolvedText;
	TestTrue(TEXT("Memo Korean title resolves"), TextSubsystem->TryGetTextByKey(TEXT("memo.1.title"), ETunaSweeperItemTextLanguage::Korean, ResolvedText));
	TestTrue(TEXT("Memo English body resolves"), TextSubsystem->TryGetTextByKey(TEXT("memo.20.body"), ETunaSweeperItemTextLanguage::English, ResolvedText));
	TestTrue(TEXT("Memo Japanese body resolves"), TextSubsystem->TryGetTextByKey(TEXT("memo.10.body"), ETunaSweeperItemTextLanguage::Japanese, ResolvedText));
	TestTrue(TEXT("Difficulty Korean title resolves"), TextSubsystem->TryGetTextByKey(TEXT("difficulty.stage.1.title"), ETunaSweeperItemTextLanguage::Korean, ResolvedText));
	TestTrue(TEXT("Difficulty English description resolves"), TextSubsystem->TryGetTextByKey(TEXT("difficulty.stage.2.description"), ETunaSweeperItemTextLanguage::English, ResolvedText));
	TestTrue(TEXT("Difficulty Japanese title resolves"), TextSubsystem->TryGetTextByKey(TEXT("difficulty.stage.3.title"), ETunaSweeperItemTextLanguage::Japanese, ResolvedText));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
