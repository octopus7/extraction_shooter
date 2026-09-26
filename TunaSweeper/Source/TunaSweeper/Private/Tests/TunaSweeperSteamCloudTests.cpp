#if WITH_DEV_AUTOMATION_TESTS

#include "Game/TunaSweeperSteamCloud.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperSteamCloudDirectoryTest,
	"TunaSweeper.Save.SteamCloudDirectory", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaSweeperSteamCloudDirectoryTest::RunTest(const FString& Parameters)
{
	const FString Saved = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir());
	const FString Account(TEXT("76561198000000001"));
	const auto Resolve = [&](bool bPackaged, const FString& Channel, bool bDemo, const FString& Id)
	{
		return TunaSweeperSteamCloud::ResolveDirectory(Saved, bPackaged, Channel, bDemo, Id);
	};
	TestEqual(TEXT("Steam full game uses account-specific FullGame directory"), Resolve(true, TEXT("Steam"), false, Account),
		FPaths::Combine(Saved, TEXT("SaveGames/Steam/76561198000000001/FullGame")));
	TestEqual(TEXT("Steam demo stays separate"), Resolve(true, TEXT("Steam"), true, Account),
		FPaths::Combine(Saved, TEXT("SaveGames/Steam/76561198000000001/Demo")));
	TestNotEqual(TEXT("Steam accounts cannot share a local save directory"), Resolve(true, TEXT("Steam"), false, Account),
		Resolve(true, TEXT("Steam"), false, TEXT("76561198000000002")));
	for (const FString& Channel : {FString(TEXT("Stove")), FString(TEXT("None")), FString()})
	{
		TestTrue(TEXT("Other package channels never select a Steam cloud directory"), Resolve(true, Channel, false, Account).IsEmpty());
	}
	TestTrue(TEXT("Editor never writes into Steam cloud saves"), Resolve(false, TEXT("Steam"), false, Account).IsEmpty());
	for (const FString& InvalidId : {FString(), FString(TEXT("../other")), FString(TEXT("00000000000000000"))})
	{
		TestEqual(TEXT("Unavailable or invalid identity is kept outside account cloud paths"), Resolve(true, TEXT("Steam"), false, InvalidId),
			FPaths::Combine(Saved, TEXT("SaveGames/Steam/LocalOnly/FullGame")));
	}
	return true;
}

#endif
