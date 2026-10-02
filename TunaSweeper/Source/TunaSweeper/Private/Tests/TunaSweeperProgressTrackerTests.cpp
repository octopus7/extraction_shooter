#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Inventory/TunaSweeperSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "Progress/TunaSweeperProgressTrackerModel.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaProgressTrackerContractTest, "TunaSweeper.ProgressTracker.Contract",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTunaProgressTrackerContractTest::RunTest(const FString&)
{
    FTunaSweeperProgressTrackerState State;
    State.StartNewRun();
    const FGuid FirstRun = State.RunId;
    TestTrue(TEXT("Run is a GUID"), FirstRun.IsValid());
    TestTrue(TEXT("First attempt is allowed"), State.TryRecordAttempt(TEXT("quest.demo_q2_clear_water_screen")));
    TestFalse(TEXT("No retry after an attempted checkpoint"), State.TryRecordAttempt(TEXT("quest.demo_q2_clear_water_screen")));
    TestTrue(TEXT("Later checkpoint does not require earlier delivery"), State.TryRecordAttempt(TEXT("demo.complete")));
    auto* Save = NewObject<UTunaSweeperSaveGame>();
    Save->ProgressTrackerState = State;
    TArray<uint8> Bytes;
    TestTrue(TEXT("Tracker serializes with the actual save container"), UGameplayStatics::SaveGameToMemory(Save, Bytes));
    auto* Loaded = Cast<UTunaSweeperSaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));
    TestNotNull(TEXT("Tracker save restores"), Loaded);
    if (!Loaded) return false;
    TestEqual(TEXT("Save load preserves run identity"), Loaded->ProgressTrackerState.RunId, FirstRun);
    FTunaSweeperProgressTrackerState Restored = Loaded->ProgressTrackerState;
    TestFalse(TEXT("Restored checkpoint does not retry"), Restored.TryRecordAttempt(TEXT("demo.complete")));
    Restored.StartNewRun();
    TestNotEqual(TEXT("New game has a fresh run"), Restored.RunId, FirstRun);
    TestTrue(TEXT("New run can observe the same checkpoint"), Restored.TryRecordAttempt(TEXT("demo.complete")));
    const FString Body = TunaSweeperProgressTrackerModel::SerializeEvent(FGuid::NewGuid(), FGuid::NewGuid(), FirstRun,
        TEXT("demo-test-1"), TEXT("quest.demo_q2_clear_water_screen"), TEXT("quest"), 12.5);
    TSharedPtr<FJsonObject> Json;
    TestTrue(TEXT("Event is valid JSON"), FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Body), Json));
    if (Json)
    {
        TestEqual(TEXT("Exact contract field count"), Json->Values.Num(), 8);
        TestEqual(TEXT("Dataset"), Json->GetStringField(TEXT("dataset")), FString(TEXT("demo")));
        TestEqual(TEXT("Run format"), Json->GetStringField(TEXT("runId")), FirstRun.ToString(EGuidFormats::DigitsWithHyphensLower));
        TestEqual(TEXT("Playtime"), Json->GetNumberField(TEXT("playtimeSeconds")), 12.5);
    }
    TestTrue(TEXT("Invalid elapsed time does not serialize"), TunaSweeperProgressTrackerModel::SerializeEvent(
        FGuid::NewGuid(), FGuid::NewGuid(), FirstRun, TEXT("build"), TEXT("game.start"), TEXT("start"), -1).IsEmpty());
    TestTrue(TEXT("Invalid category does not serialize"), TunaSweeperProgressTrackerModel::SerializeEvent(
        FGuid::NewGuid(), FGuid::NewGuid(), FirstRun, TEXT("build"), TEXT("game.start"), TEXT("unknown"), 0).IsEmpty());
    TestTrue(TEXT("Invalid build identifier is rejected before dispatch"), TunaSweeperProgressTrackerModel::SerializeEvent(
        FGuid::NewGuid(), FGuid::NewGuid(), FirstRun, TEXT("build with spaces"), TEXT("game.start"), TEXT("start"), 0).IsEmpty());
    TestTrue(TEXT("Invalid checkpoint identifier is rejected before dispatch"), TunaSweeperProgressTrackerModel::SerializeEvent(
        FGuid::NewGuid(), FGuid::NewGuid(), FirstRun, TEXT("build"), TEXT("location./bad"), TEXT("location"), 0).IsEmpty());
    TestTrue(TEXT("Empty location is ignored"), TunaSweeperProgressTrackerModel::LocationCheckpoint(NAME_None).IsEmpty());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaProgressTrackerScreenAttemptsTest, "TunaSweeper.ProgressTracker.ScreenAttempts",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTunaProgressTrackerScreenAttemptsTest::RunTest(const FString&)
{
    FTunaSweeperProgressTrackerState Session;
    Session.StartNewRun();
    FTunaSweeperProgressTrackerState Gameplay;
    const FGuid Player = FGuid::NewGuid();
    const FString Title = TunaSweeperProgressTrackerModel::PrepareEvent(Session, Player, TEXT("0.2.9200"),
        TEXT("location.title_screen"), TEXT("location"), 0);
    TestFalse(TEXT("Title produces an observation before a gameplay run exists"), Title.IsEmpty());
    TestFalse(TEXT("Title does not create a gameplay run"), Gameplay.RunId.IsValid());
    TestEqual(TEXT("Title does not mark gameplay checkpoints"), Gameplay.AttemptedCheckpoints.Num(), 0);
    TSharedPtr<FJsonObject> Json;
    TestTrue(TEXT("Title payload is JSON"), FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Title), Json));
    if (Json)
    {
        TestEqual(TEXT("Title uses its transient session identity"), Json->GetStringField(TEXT("runId")),
            Session.RunId.ToString(EGuidFormats::DigitsWithHyphensLower));
        TestEqual(TEXT("Title has no gameplay elapsed time"), Json->GetNumberField(TEXT("playtimeSeconds")), 0.0);
    }
    Gameplay.StartNewRun();
    TestTrue(TEXT("Returning to title during gameplay does not retry"), TunaSweeperProgressTrackerModel::PrepareEvent(
        Session, Player, TEXT("0.2.9200"), TEXT("location.title_screen"), TEXT("location"), 0).IsEmpty());
    TestFalse(TEXT("Ending records in the gameplay run"), TunaSweeperProgressTrackerModel::PrepareEvent(
        Gameplay, Player, TEXT("0.2.9200"), TEXT("location.ending_screen"), TEXT("location"), 125).IsEmpty());
    TestTrue(TEXT("Repeated ending entry does not retry"), TunaSweeperProgressTrackerModel::PrepareEvent(
        Gameplay, Player, TEXT("0.2.9200"), TEXT("location.ending_screen"), TEXT("location"), 130).IsEmpty());
    Gameplay.StartNewRun();
    TestFalse(TEXT("Another run can enter the ending"), TunaSweeperProgressTrackerModel::PrepareEvent(
        Gameplay, Player, TEXT("0.2.9200"), TEXT("location.ending_screen"), TEXT("location"), 10).IsEmpty());
    Session.StartNewRun();
    TestFalse(TEXT("Another application session can enter the title"), TunaSweeperProgressTrackerModel::PrepareEvent(
        Session, Player, TEXT("0.2.9200"), TEXT("location.title_screen"), TEXT("location"), 0).IsEmpty());
    TestTrue(TEXT("Invalid data produces no observation"), TunaSweeperProgressTrackerModel::PrepareEvent(
        Gameplay, Player, TEXT("invalid build"), TEXT("location.other_screen"), TEXT("location"), 10).IsEmpty());
    TestFalse(TEXT("Invalid data does not consume an attempt"), Gameplay.AttemptedCheckpoints.Contains(TEXT("location.other_screen")));
    return true;
}
#endif
