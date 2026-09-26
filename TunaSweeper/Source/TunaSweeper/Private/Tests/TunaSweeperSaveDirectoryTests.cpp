#if WITH_DEV_AUTOMATION_TESTS

#include "Game/TunaSweeperSaveDirectory.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"

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

#endif
