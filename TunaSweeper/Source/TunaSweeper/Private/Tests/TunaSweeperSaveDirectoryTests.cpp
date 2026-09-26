#if WITH_DEV_AUTOMATION_TESTS

#include "Game/TunaSweeperSaveDirectory.h"
#include "HAL/FileManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperFullGameDirectoryMigrationTest,
	"TunaSweeper.Save.DirectoryMigration", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaSweeperFullGameDirectoryMigrationTest::RunTest(const FString& Parameters)
{
	const FString Sandbox = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Automation"),
		TEXT("SaveDirectory_") + FGuid::NewGuid().ToString(EGuidFormats::Digits));
	const FString Legacy = FPaths::Combine(Sandbox, TEXT("SaveGames/Main"));
	const FString Destination = FPaths::Combine(Sandbox, TEXT("SaveGames/FullGame"));
	IFileManager::Get().MakeDirectory(*FPaths::Combine(Legacy, TEXT("Backups")), true);
	const TArray<FString> Files = {TEXT("TunaSweeperSave_Slot01.sav"), TEXT("TunaSweeperSave_Slot01.sav.previous"),
		TEXT("TunaSweeperSaveSettings.sav"), TEXT("Achievements_Steam.sav"), TEXT("Backups/SaveSlot01_test.sav")};
	for (const FString& File : Files)
	{
		TestTrue(TEXT("Write legacy fixture"), FFileHelper::SaveStringToFile(TEXT("legacy"), *FPaths::Combine(Legacy, File)));
	}
	TunaSweeperSaveDirectory::ResolveLocalDirectory(Sandbox, true);
	TestTrue(TEXT("Demo never migrates full game saves"), FPaths::DirectoryExists(Legacy));
	TestEqual(TEXT("Full game resolves renamed root"), TunaSweeperSaveDirectory::ResolveLocalDirectory(Sandbox, false), Destination);
	TestFalse(TEXT("Old root was moved, preventing deleted slots from being imported again"), FPaths::DirectoryExists(Legacy));
	for (const FString& File : Files)
	{
		FString Contents;
		TestTrue(TEXT("All save artifacts survived rename"), FFileHelper::LoadFileToString(Contents, *FPaths::Combine(Destination, File)));
		TestEqual(TEXT("File contents preserved"), Contents, FString(TEXT("legacy")));
	}
	IFileManager::Get().Delete(*FPaths::Combine(Destination, Files[0]));
	TunaSweeperSaveDirectory::ResolveLocalDirectory(Sandbox, false);
	TestFalse(TEXT("Repeated lookup does not resurrect deleted data"), FPaths::FileExists(FPaths::Combine(Destination, Files[0])));
	IFileManager::Get().MakeDirectory(*Legacy, true);
	FFileHelper::SaveStringToFile(TEXT("old-install"), *FPaths::Combine(Legacy, Files[0]));
	TunaSweeperSaveDirectory::ResolveLocalDirectory(Sandbox, false);
	TestFalse(TEXT("Existing destination is never merged with a leftover Main directory"), FPaths::FileExists(FPaths::Combine(Destination, Files[0])));
	TestTrue(TEXT("Conflicting legacy data is retained"), FPaths::FileExists(FPaths::Combine(Legacy, Files[0])));
	IFileManager::Get().DeleteDirectory(*Sandbox, false, true);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperStoveDirectoryRoutingTest,
	"TunaSweeper.Save.StoveDirectoryRouting", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaSweeperStoveDirectoryRoutingTest::RunTest(const FString& Parameters)
{
	const FString Saved = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir());
	const auto Resolve = [&](bool bPackaged, const FString& Channel, bool bDemo, const FString& CloudRoot)
	{
		return TunaSweeperSaveDirectory::ResolveStoveDirectory(Saved, bPackaged, Channel, bDemo, CloudRoot, TEXT("12345"));
	};
	TestEqual(TEXT("Stove trusts SDK root and appends FullGame"), Resolve(true, TEXT("Stove"), false, TEXT("C:/Cloud/12345")),
		FString(TEXT("C:/Cloud/12345/FullGame")));
	TestEqual(TEXT("Stove demo is a separate flavor"), Resolve(true, TEXT("Stove"), true, TEXT("C:/Cloud/12345")),
		FString(TEXT("C:/Cloud/12345/Demo")));
	for (const FString& Root : {FString(), FString(TEXT("relative/path")), FString(TEXT("C:/Cloud/../Other"))})
	{
		TestEqual(TEXT("Unavailable or invalid SDK path falls back to account-local storage"), Resolve(true, TEXT("Stove"), false, Root),
			FPaths::Combine(Saved, TEXT("SaveGames/Stove/LocalOnly/12345/FullGame")));
	}
	TestTrue(TEXT("Steam cannot use Stove cloud paths"), Resolve(true, TEXT("Steam"), false, TEXT("C:/Cloud/12345")).IsEmpty());
	TestTrue(TEXT("Editor cannot use Stove cloud paths"), Resolve(false, TEXT("Stove"), false, TEXT("C:/Cloud/12345")).IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperLocalDirectoryFailurePinTest,
	"TunaSweeper.Save.DirectoryFailureRemainsPinned", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaSweeperLocalDirectoryFailurePinTest::RunTest(const FString& Parameters)
{
	const FString Sandbox = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Automation"),
		TEXT("SaveDirectoryFailure_") + FGuid::NewGuid().ToString(EGuidFormats::Digits)));
	const FString Legacy = FPaths::Combine(Sandbox, TEXT("SaveGames/Main"));
	const FString Destination = FPaths::Combine(Sandbox, TEXT("SaveGames/FullGame"));
	IFileManager::Get().MakeDirectory(*Legacy, true);
	TestTrue(TEXT("Create legacy save"), FFileHelper::SaveStringToFile(TEXT("progress"), *FPaths::Combine(Legacy, TEXT("Slot.sav"))));
	TestTrue(TEXT("Create temporary rename obstruction"), FFileHelper::SaveStringToFile(TEXT("blocked"), *Destination));
	AddExpectedMessagePlain(TEXT("Cannot rename save directory"), ELogVerbosity::Warning);
	TestEqual(TEXT("Failed rename keeps existing saves accessible"),
		TunaSweeperSaveDirectory::ResolveLocalDirectory(Sandbox, false), Legacy);
	TestTrue(TEXT("Remove temporary obstruction"), IFileManager::Get().Delete(*Destination));
	TestEqual(TEXT("Backup lookup cannot switch roots after primary path was captured"),
		TunaSweeperSaveDirectory::ResolveLocalDirectory(Sandbox, false), Legacy);
	TestTrue(TEXT("Captured primary path remains valid for the entire session"), FPaths::FileExists(FPaths::Combine(Legacy, TEXT("Slot.sav"))));
	IFileManager::Get().DeleteDirectory(*Sandbox, false, true);
	return true;
}

#endif
