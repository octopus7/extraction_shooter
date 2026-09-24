#include "Misc/AutomationTest.h"
#include "Online/TunaSweeperOnlineCoopTypes.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaCoopRejectionTest, "TunaSweeper.OnlineCoop.Lifecycle.ResultRejections", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTunaCoopRejectionTest::RunTest(const FString& Parameters)
{
    using namespace TunaSweeperOnlineCoop;
    TestEqual(TEXT("Unknown code"), ClassifySession(TEXT("00000001"), 1, 1, TEXT("00000002")), ETunaSweeperOnlineCoopError::CodeNotFound);
    TestEqual(TEXT("Version rejection"), ClassifySession(TEXT("00000001"), 2, 1, TEXT("00000001")), ETunaSweeperOnlineCoopError::VersionMismatch);
    TestEqual(TEXT("Full lobby"), ClassifySession(TEXT("00000001"), 1, 0, TEXT("00000001")), ETunaSweeperOnlineCoopError::RoomFull);
    TestEqual(TEXT("Negative capacity"), ClassifySession(TEXT("00000001"), 1, -1, TEXT("00000001")), ETunaSweeperOnlineCoopError::RoomFull);
    TestEqual(TEXT("Valid lobby"), ClassifySession(TEXT("00000001"), 1, 1, TEXT("00000001")), ETunaSweeperOnlineCoopError::None);
    return true;
}
#endif

#if WITH_DEV_AUTOMATION_TESTS
#include "Online/TunaSweeperOnlineCoopSubsystem.h"
#include "Engine/GameInstance.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaCoopGenerationTest, "TunaSweeper.OnlineCoop.Lifecycle.LateAndDuplicateCallbacks", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTunaCoopGenerationTest::RunTest(const FString& Parameters)
{
    UGameInstance* Instance = NewObject<UGameInstance>();
    auto* Subsystem = NewObject<UTunaSweeperOnlineCoopSubsystem>(Instance);
    Subsystem->BeginOperation();
    const uint64 OldGeneration = Subsystem->OperationGeneration;
    Subsystem->BeginOperation();
    const uint64 CurrentGeneration = Subsystem->OperationGeneration;
    TestFalse(TEXT("Previous generation cannot complete current operation"), Subsystem->AcceptCallback(OldGeneration));
    TestTrue(TEXT("Current operation can complete"), Subsystem->AcceptCallback(CurrentGeneration));
    Subsystem->OnCreateComplete(TEXT("UnrelatedSession"), true, CurrentGeneration);
    TestTrue(TEXT("Unrelated global session callback does not consume operation"), Subsystem->bPending);
    Subsystem->Fail(ETunaSweeperOnlineCoopError::OperationTimeout);
    TestTrue(TEXT("Timeout quarantines uncancellable backend operation"), Subsystem->bPending);
    TestFalse(TEXT("Retry cannot overlap timed-out operation"), Subsystem->InitializeOnlineCoop());
    Subsystem->OnCreateComplete(FTunaSweeperOnlineCoopSettings::SessionName(), true, CurrentGeneration);
    TestFalse(TEXT("Late completion drains quarantine"), Subsystem->bPending);
    TestFalse(TEXT("Late abandoned success cannot initiate travel"), Subsystem->bTravelling);
    TestTrue(TEXT("Late abandoned success never publishes invite"), Subsystem->GetCurrentInviteCode().IsEmpty());
    TestFalse(TEXT("Duplicate callback is rejected"), Subsystem->AcceptCallback(CurrentGeneration));
    TestEqual(TEXT("Timeout error remains visible after cleanup"), Subsystem->GetLastError(), ETunaSweeperOnlineCoopError::OperationTimeout);
    return true;
}
#endif
