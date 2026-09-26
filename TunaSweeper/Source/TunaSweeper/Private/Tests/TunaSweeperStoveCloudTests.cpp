#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Platform/TunaSweeperStove.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperStoveCloudChannelGateTest,
    "TunaSweeper.Save.StoveCloudChannelGate", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaSweeperStoveCloudChannelGateTest::RunTest(const FString& Parameters)
{
#if !WITH_TUNASWEEPER_STOVE
    FString Directory(TEXT("stale-directory"));
    FString Account(TEXT("stale-account"));
    TestFalse(TEXT("Non-STOVE builds cannot select a STOVE cloud directory"),
        TunaSweeperStove::TryGetCloudSaveRoot(Directory));
    TestTrue(TEXT("Failed directory lookup clears stale output"), Directory.IsEmpty());
    TestFalse(TEXT("Non-STOVE builds cannot use a STOVE account"),
        TunaSweeperStove::TryGetSaveAccountId(Account));
    TestTrue(TEXT("Failed account lookup clears stale output"), Account.IsEmpty());
#endif
    return true;
}

#endif
