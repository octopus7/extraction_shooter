#if WITH_DEV_AUTOMATION_TESTS
#include "Game/TunaSweeperGameInstance.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSaveWriteGuardTest,"TunaSweeper.Save.AutomationWriteGuard",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTunaSaveWriteGuardTest::RunTest(const FString&)
{
    auto* GI = NewObject<UTunaSweeperGameInstance>();
    TestFalse(TEXT("Normal instances permit writes"), GI->bSuppressSaveWritesForTests);
    GI->bSuppressSaveWritesForTests = true;
    TestFalse(TEXT("Automation cannot write player saves"), GI->SaveGameStateInternal());
    GI->ResetRuntimeStateForSaveSlotSelection();
    TestTrue(TEXT("Runtime reset preserves fixture protection"), GI->bSuppressSaveWritesForTests);
    TestFalse(TEXT("Post-reset autosave remains blocked"), GI->SaveGameStateInternal());
    return true;
}
#endif
