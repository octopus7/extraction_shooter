#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Inventory/TunaSweeperSaveTool.h"
#include "Inventory/TunaSweeperSaveGame.h"
#include "Game/TunaSweeperSafeSave.h"
#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperSaveSnapshotTest, "TunaSweeper.SaveTool.Snapshots",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaSweeperSaveSnapshotTest::RunTest(const FString& Parameters)
{
    const FString Path = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() /
        TEXT("Automation/SaveSnapshots") / FGuid::NewGuid().ToString() / TEXT("TunaSweeperSave_Slot01.sav"));
    auto* Save = NewObject<UTunaSweeperSaveGame>();
    Save->BuildFlavor = TEXT("Main"); Save->SaveSlotIndex = 1;
    Save->QuestCoinBalance = 123; Save->LastSavedAtTicks = 456789;
    Save->CompletedScenarioFlags.Add(TEXT("snapshot.fixture"));
    auto Validator = [](const USaveGame& S) { return S.GetClass() == UTunaSweeperSaveGame::StaticClass(); };
    if (!TestTrue(TEXT("Fixture saved"), TunaSweeperSafeSave::SaveGameFileFailClosed(Save, Path, Validator))) return false;
    auto Request = [&](const TCHAR* Op, const FString& Id = FString())
    {
        auto R = MakeShared<FJsonObject>();
        R->SetNumberField(TEXT("version"), 1); R->SetStringField(TEXT("operation"), Op);
        R->SetStringField(TEXT("savePath"), Path); R->SetStringField(TEXT("flavor"), TEXT("Main"));
        R->SetNumberField(TEXT("slot"), 1);
        if (!Id.IsEmpty()) R->SetStringField(TEXT("snapshotId"), Id);
        return R;
    };
    auto Created = TunaSweeperSaveTool::Execute(Request(TEXT("snapshot-create")));
    if (!TestTrue(TEXT("Create snapshot succeeds"), Created->GetBoolField(TEXT("ok")))) return false;
    const FString Id = Created->GetStringField(TEXT("snapshotId"));
    TArray<uint8> Original, Copy, After;
    FFileHelper::LoadFileToArray(Original, *Path);
    FFileHelper::LoadFileToArray(Copy, *Created->GetStringField(TEXT("snapshotPath")));
    TestTrue(TEXT("Snapshot preserves exact saved bytes"), Original == Copy);
    Save->QuestCoinBalance = 999;
    TestTrue(TEXT("Changed fixture saved"), TunaSweeperSafeSave::SaveGameFileFailClosed(Save, Path, Validator));
    TArray<uint8> Changed; FFileHelper::LoadFileToArray(Changed, *Path);
    auto Restored = TunaSweeperSaveTool::Execute(Request(TEXT("snapshot-restore"), Id));
    if (!TestTrue(TEXT("Restore succeeds"), Restored->GetBoolField(TEXT("ok")))) return false;
    FFileHelper::LoadFileToArray(Copy, *Restored->GetStringField(TEXT("backupPath")));
    TestTrue(TEXT("Pre-restore backup preserves replaced save"), Copy == Changed);
    auto* Reload = Cast<UTunaSweeperSaveGame>(TunaSweeperSafeSave::LoadVerifiedSaveFile(Path, Validator));
    if (!TestNotNull(TEXT("Restored slot reloads"), Reload)) return false;
    TestEqual(TEXT("Currency restored"), Reload->QuestCoinBalance, 123);
    TestEqual(TEXT("Original timestamp retained"), Reload->LastSavedAtTicks, int64(456789));
    TestTrue(TEXT("Other state retained"), Reload->CompletedScenarioFlags.Contains(TEXT("snapshot.fixture")));
    auto Listed = TunaSweeperSaveTool::Execute(Request(TEXT("snapshot-list")));
    TestEqual(TEXT("Manual snapshot and automatic backup listed"), Listed->GetArrayField(TEXT("snapshots")).Num(), 2);
    FFileHelper::LoadFileToArray(Original, *Path);
    auto BadSlot = Request(TEXT("snapshot-restore"), Id); BadSlot->SetNumberField(TEXT("slot"), 2);
    TestFalse(TEXT("Other slot rejected"), TunaSweeperSaveTool::Execute(BadSlot)->GetBoolField(TEXT("ok")));
    auto BadFlavor = Request(TEXT("snapshot-restore"), Id); BadFlavor->SetStringField(TEXT("flavor"), TEXT("Demo"));
    TestFalse(TEXT("Other dataset rejected"), TunaSweeperSaveTool::Execute(BadFlavor)->GetBoolField(TEXT("ok")));
    TestFalse(TEXT("Traversal rejected"), TunaSweeperSaveTool::Execute(Request(TEXT("snapshot-restore"), TEXT("../escape.sav")))->GetBoolField(TEXT("ok")));
    const FString SnapshotPath = Created->GetStringField(TEXT("snapshotPath"));
    Copy = { 1, 2, 3 }; FFileHelper::SaveArrayToFile(Copy, *SnapshotPath);
    TestFalse(TEXT("Corrupt snapshot rejected"), TunaSweeperSaveTool::Execute(Request(TEXT("snapshot-restore"), Id))->GetBoolField(TEXT("ok")));
    FFileHelper::LoadFileToArray(After, *Path);
    TestTrue(TEXT("All rejected restores preserve current slot"), After == Original);
    auto Undo = TunaSweeperSaveTool::Execute(Request(TEXT("snapshot-restore"), FPaths::GetCleanFilename(Restored->GetStringField(TEXT("backupPath")))));
    TestTrue(TEXT("Automatic backup is restorable"), Undo->GetBoolField(TEXT("ok")));
    Reload = Cast<UTunaSweeperSaveGame>(TunaSweeperSafeSave::LoadVerifiedSaveFile(Path, Validator));
    TestTrue(TEXT("Undo restores replaced state"), Reload && Reload->QuestCoinBalance == 999);
    IFileManager::Get().DeleteDirectory(*FPaths::GetPath(SnapshotPath), false, true);
    IFileManager::Get().DeleteDirectory(*FPaths::GetPath(Path), false, true);
    return true;
}
#endif
