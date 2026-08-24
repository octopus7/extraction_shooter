#if WITH_DEV_AUTOMATION_TESTS

#include "Game/TunaSweeperGameInstanceShared.h"
#include "Misc/AutomationTest.h"

namespace TunaSweeperSaveVersionTests
{
	constexpr EAutomationTestFlags TestFlags =
		EAutomationTestFlags::EditorContext |
		EAutomationTestFlags::ClientContext |
		EAutomationTestFlags::EngineFilter;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTunaSweeperSaveVersionPolicyTest,
	"TunaSweeper.Save.VersionPolicy",
	TunaSweeperSaveVersionTests::TestFlags)

bool FTunaSweeperSaveVersionPolicyTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	TestEqual(TEXT("Current save version"), TunaSweeperSave::CurrentSaveVersion, 20);
	TestEqual(TEXT("Minimum supported save version"), TunaSweeperSave::MinimumSupportedSaveVersion, 20);
	TestTrue(TEXT("Version 19 is outdated"), TunaSweeperSave::IsOutdatedSaveVersion(19));
	TestFalse(TEXT("Version 20 is supported"), TunaSweeperSave::IsOutdatedSaveVersion(20));
	TestFalse(TEXT("Future versions are not auto-deleted"), TunaSweeperSave::IsOutdatedSaveVersion(21));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTunaSweeperOutdatedSaveCleanupTest,
	"TunaSweeper.Save.OutdatedCleanup",
	TunaSweeperSaveVersionTests::TestFlags)

bool FTunaSweeperOutdatedSaveCleanupTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	const FString TestDirectory = FPaths::Combine(
		FPaths::ProjectSavedDir(),
		TEXT("Automation"),
		FString::Printf(TEXT("SaveVersionCleanup_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits)));
	IFileManager::Get().MakeDirectory(*TestDirectory, true);

	auto WriteSaveFile = [this, &TestDirectory](const TCHAR* FileName, int32 SaveVersion)
	{
		UTunaSweeperSaveGame* SaveGame = NewObject<UTunaSweeperSaveGame>();
		SaveGame->SaveVersion = SaveVersion;
		TArray<uint8> SaveData;
		const bool bSerialized = UGameplayStatics::SaveGameToMemory(SaveGame, SaveData);
		TestTrue(FString::Printf(TEXT("Serialize %s"), FileName), bSerialized);
		const FString FilePath = FPaths::Combine(TestDirectory, FileName);
		TestTrue(
			FString::Printf(TEXT("Write %s"), FileName),
			bSerialized && FFileHelper::SaveArrayToFile(SaveData, *FilePath));
		return FilePath;
	};

	const FString OldSavePath = WriteSaveFile(TEXT("OldSave.sav"), 19);
	const FString CurrentSavePath = WriteSaveFile(TEXT("CurrentSave.sav"), 20);
	const FString FutureSavePath = WriteSaveFile(TEXT("FutureSave.sav"), 21);

	TestTrue(
		TEXT("Version 19 cleanup result"),
		TunaSweeperSave::DeleteOutdatedSaveFileIfNeeded(OldSavePath, TestDirectory) ==
			TunaSweeperSave::EOutdatedSaveCleanupResult::Deleted);
	TestFalse(TEXT("Version 19 file was deleted"), FPaths::FileExists(OldSavePath));

	TestTrue(
		TEXT("Version 20 cleanup result"),
		TunaSweeperSave::DeleteOutdatedSaveFileIfNeeded(CurrentSavePath, TestDirectory) ==
			TunaSweeperSave::EOutdatedSaveCleanupResult::NotOutdated);
	TestTrue(TEXT("Version 20 file remains"), FPaths::FileExists(CurrentSavePath));

	TestTrue(
		TEXT("Future version cleanup result"),
		TunaSweeperSave::DeleteOutdatedSaveFileIfNeeded(FutureSavePath, TestDirectory) ==
			TunaSweeperSave::EOutdatedSaveCleanupResult::NotOutdated);
	TestTrue(TEXT("Future version file remains"), FPaths::FileExists(FutureSavePath));

	FString LogText;
	const FString DeletionLogPath = FPaths::Combine(TestDirectory, TunaSweeperSave::AutoDeletedSaveLogFileName);
	TestTrue(TEXT("Deletion log exists"), FFileHelper::LoadFileToString(LogText, *DeletionLogPath));
	TestTrue(TEXT("Deletion log records version"), LogText.Contains(TEXT("version=19")));
	TestTrue(TEXT("Deletion log records file"), LogText.Contains(TEXT("OldSave.sav")));

	IFileManager::Get().DeleteDirectory(*TestDirectory, false, true);
	return true;
}

#endif
