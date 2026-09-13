#if WITH_DEV_AUTOMATION_TESTS
#include "Game/TunaSweeperGameInstance.h"
#include "Settings/TunaSweeperBuildFlavor.h"
#include "Settings/TunaSweeperBuildTargetSettings.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaDemoSaveRetirementTest,"TunaSweeper.Save.DemoEndingRetirementGuards",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTunaDemoSaveRetirementTest::RunTest(const FString&)
{
    auto* GI = NewObject<UTunaSweeperGameInstance>();
    TestFalse(TEXT("No ending completion never deletes a slot"),GI->DeleteCompletedDemoSave());
    TestEqual(TEXT("Rejected deletion leaves slot writable"),GI->RetiredDemoSaveSlotIndex,INDEX_NONE);
    {
        TGuardValue<ETunaSweeperBuildTarget> MainTarget(
            GetMutableDefault<UTunaSweeperBuildTargetSettings>()->BuildTarget,ETunaSweeperBuildTarget::NoStoreFull);
        TestFalse(TEXT("Fixture uses Main"),TunaSweeperBuildFlavor::IsDemo());
        GI->CompletedScenarioFlags.Add(TEXT("demo.ending.farewell_seen"));
        TestFalse(TEXT("Main keeps saves even with a legacy demo completion flag"),GI->DeleteCompletedDemoSave());
        TestEqual(TEXT("Main has no retired slot"),GI->RetiredDemoSaveSlotIndex,INDEX_NONE);
    }
    // Exercise the early autosave guard without creating or deleting any real save files.
    GI->RetiredDemoSaveSlotIndex = GI->ActiveSaveSlotIndex;
    TestFalse(TEXT("Late autosave cannot recreate a retired slot"),GI->SaveGameStateInternal());
    GI->ResetRuntimeStateForSaveSlotSelection();
    TestEqual(TEXT("Runtime reset does not lift the retirement barrier"),GI->RetiredDemoSaveSlotIndex,GI->ActiveSaveSlotIndex);
    TestFalse(TEXT("Post-reset autosave remains blocked"),GI->SaveGameStateInternal());
    return true;
}
#endif
