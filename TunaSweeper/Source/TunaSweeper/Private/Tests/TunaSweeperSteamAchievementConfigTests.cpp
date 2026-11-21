#if WITH_DEV_AUTOMATION_TESTS
#include "Achievement/TunaSweeperAchievementModel.h"
#include "Engine/GameInstance.h"
#include "Misc/AutomationTest.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/Paths.h"
#include "Subsystem/TunaSweeperAchievementSubsystem.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSteamAchievementConfigTest,
    "TunaSweeper.Achievement.SteamConfig", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTunaSteamAchievementConfigTest::RunTest(const FString& Parameters)
{
    TStrongObjectPtr<UGameInstance> Owner(NewObject<UGameInstance>());
    TStrongObjectPtr<UTunaSweeperAchievementSubsystem> Subject(NewObject<UTunaSweeperAchievementSubsystem>(Owner.Get()));
    if (!TestTrue(TEXT("Catalog loads"), Subject->LoadAchievementDefinitions())) return false;
    const auto Definitions = Subject->GetAchievementDefinitions();
    FConfigFile Base, Full;
    Base.Read(FPaths::ProjectConfigDir() / TEXT("DefaultEngine.ini"));
    Full.Read(FPaths::ProjectConfigDir() / TEXT("Custom/Full/DefaultEngine.ini"));
    bool Enabled = true;
    TestTrue(TEXT("Base sync policy exists"), Base.GetBool(TEXT("TunaSweeperAchievements"), TEXT("bEnableSteamSync"), Enabled));
    TestFalse(TEXT("Editor and non-Full configs do not publish Full achievements"), Enabled);
    TestTrue(TEXT("Full sync policy exists"), Full.GetBool(TEXT("TunaSweeperAchievements"), TEXT("bEnableSteamSync"), Enabled));
    TestTrue(TEXT("Full Steam sync enabled"), Enabled);
    int32 AppId = 0;
    Full.GetInt(TEXT("OnlineSubsystemSteam"), TEXT("SteamDevAppId"), AppId);
    TestEqual(TEXT("Registered full game app"), AppId, 5137900);
    TArray<FString> ConfiguredIds;
    bool Gap = false;
    for (int32 Index = 0; Index < 100; ++Index)
    {
        FString Id;
        Full.GetString(TEXT("OnlineSubsystemSteam"), *FString::Printf(TEXT("Achievement_%d_Id"), Index), Id);
        if (Id.IsEmpty()) { Gap = true; continue; }
        TestFalse(TEXT("Steam config indices are contiguous"), Gap);
        ConfiguredIds.Add(Id);
    }
    FString Error;
    TestTrue(TEXT("Configured IDs exactly match enabled mappings"),
        TunaSweeperAchievementModel::ValidateConfiguredPlatformIds(Definitions, TEXT("Steam"), ConfiguredIds, Error));
    TestEqual(TEXT("Full config has 19 active achievements"), ConfiguredIds.Num(), 19);
    FTunaSweeperAchievementProgressState State;
    for (const auto& Definition : Definitions)
    {
        const FString Expected = TEXT("ACH_") + Definition.AchievementId.ToString();
        TestEqual(TEXT("Registered API name"), Definition.PlatformIds.FindRef(TEXT("Steam")), Expected);
        State.UnlockedAchievementIds.Add(Definition.AchievementId);
    }
    TSet<FName> Attempted;
    FName InternalId;
    FString PlatformId;
    while (TunaSweeperAchievementModel::FindNextPendingUnlock(Definitions, State, TEXT("Steam"), Attempted, InternalId, PlatformId))
    {
        TestTrue(TEXT("Pending upload uses a configured API name"), ConfiguredIds.Contains(PlatformId));
        TestNotEqual(TEXT("Disabled 05 never publishes even with a stale local unlock"), InternalId, FName(TEXT("05")));
        Attempted.Add(InternalId);
    }
    TestEqual(TEXT("Existing local unlocks can publish through new mappings"), Attempted.Num(), 19);
    return true;
}
#endif
