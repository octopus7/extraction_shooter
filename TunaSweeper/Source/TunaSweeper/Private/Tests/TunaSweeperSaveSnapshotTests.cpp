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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperSaveSnapshotMemoTest, "TunaSweeper.SaveTool.SnapshotMemoAndDelete",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaSweeperSaveSnapshotMemoTest::RunTest(const FString& Parameters)
{
    const FString Path = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() /
        TEXT("Automation/SaveSnapshots") / FGuid::NewGuid().ToString() / TEXT("TunaSweeperSave_Slot01.sav"));
    auto* Save = NewObject<UTunaSweeperSaveGame>();
    Save->BuildFlavor = TEXT("Main"); Save->SaveSlotIndex = 1; Save->QuestCoinBalance = 321;
    auto Validator = [](const USaveGame& S) { return S.GetClass() == UTunaSweeperSaveGame::StaticClass(); };
    if (!TestTrue(TEXT("Memo fixture saved"), TunaSweeperSafeSave::SaveGameFileFailClosed(Save, Path, Validator))) return false;
    auto Request = [&](const TCHAR* Op, const FString& Id = FString())
    {
        auto R = MakeShared<FJsonObject>();
        R->SetNumberField(TEXT("version"), 1); R->SetStringField(TEXT("operation"), Op);
        R->SetStringField(TEXT("savePath"), Path); R->SetStringField(TEXT("flavor"), TEXT("Main"));
        R->SetNumberField(TEXT("slot"), 1);
        if (!Id.IsEmpty()) R->SetStringField(TEXT("snapshotId"), Id);
        return R;
    };
    const FString Memo = TEXT("한글 메모: 퀘스트 007 완료 / 낙석 폭파 전 \"기지\"");
    auto Create = Request(TEXT("snapshot-create")); Create->SetStringField(TEXT("note"), Memo);
    auto Created = TunaSweeperSaveTool::Execute(Create);
    TestTrue(TEXT("Create accepts Korean memo"), Created->GetBoolField(TEXT("ok")));
    auto Legacy = TunaSweeperSaveTool::Execute(Request(TEXT("snapshot-create")));
    if (!TestTrue(TEXT("Unannotated snapshots remain supported"), Legacy->GetBoolField(TEXT("ok")))) return false;
    const FString LegacyId = Legacy->GetStringField(TEXT("snapshotId"));
    auto Listed = TunaSweeperSaveTool::Execute(Request(TEXT("snapshot-list")));
    const FString Directory = Listed->GetStringField(TEXT("directory"));
    auto BadNote = Request(TEXT("snapshot-create")); BadNote->SetNumberField(TEXT("note"), 123);
    TestFalse(TEXT("Non-text memo is rejected"), TunaSweeperSaveTool::Execute(BadNote)->GetBoolField(TEXT("ok")));
    BadNote->SetStringField(TEXT("note"), FString::ChrN(2049, TEXT('가')));
    TestFalse(TEXT("Overlong memo is rejected before copying"), TunaSweeperSaveTool::Execute(BadNote)->GetBoolField(TEXT("ok")));
    TestEqual(TEXT("Invalid memos create no extra snapshots"), TunaSweeperSaveTool::Execute(Request(TEXT("snapshot-list")))->GetArrayField(TEXT("snapshots")).Num(), 2);
    bool bFoundMemo = false, bFoundEmpty = false;
    for (const auto& Value : Listed->GetArrayField(TEXT("snapshots")))
    {
        const auto Row = Value->AsObject(); FString Note;
        bFoundMemo |= Row->TryGetStringField(TEXT("note"), Note) && Note == Memo;
        bFoundEmpty |= Row->GetStringField(TEXT("snapshotId")) == LegacyId &&
            Row->TryGetStringField(TEXT("note"), Note) && Note.IsEmpty();
    }
    TestTrue(TEXT("Korean memo survives disk write and list reload"), bFoundMemo);
    TestTrue(TEXT("Legacy copy without metadata lists with an empty memo"), bFoundEmpty);
    TArray<uint8> Before, After;
    FFileHelper::LoadFileToArray(Before, *Path);
    TestFalse(TEXT("Delete rejects path traversal"), TunaSweeperSaveTool::Execute(Request(TEXT("snapshot-delete"), TEXT("../escape.sav")))->GetBoolField(TEXT("ok")));
    auto Other = Request(TEXT("snapshot-delete"), LegacyId); Other->SetStringField(TEXT("flavor"), TEXT("Demo"));
    TestFalse(TEXT("Delete is isolated by dataset"), TunaSweeperSaveTool::Execute(Other)->GetBoolField(TEXT("ok")));
    Other = Request(TEXT("snapshot-delete"), LegacyId);
    Other->SetStringField(TEXT("savePath"), FPaths::GetPath(Path) / TEXT("OtherRoot/TunaSweeperSave_Slot01.sav"));
    TestFalse(TEXT("Delete is isolated by source root"), TunaSweeperSaveTool::Execute(Other)->GetBoolField(TEXT("ok")));
    if (Created->GetBoolField(TEXT("ok")))
    {
        const FString Id = Created->GetStringField(TEXT("snapshotId"));
        const FString CopyPath = Created->GetStringField(TEXT("snapshotPath"));
        FFileHelper::LoadFileToArray(After, *CopyPath);
        TestTrue(TEXT("Memo does not alter saved gameplay bytes"), Before == After);
        TestTrue(TEXT("Delete annotated snapshot succeeds"), TunaSweeperSaveTool::Execute(Request(TEXT("snapshot-delete"), Id))->GetBoolField(TEXT("ok")));
        TestFalse(TEXT("Snapshot bytes removed"), IFileManager::Get().FileExists(*CopyPath));
        TestFalse(TEXT("Memo sidecar removed"), IFileManager::Get().FileExists(*(CopyPath + TEXT(".json"))));
        TestFalse(TEXT("Repeated deletion reports missing snapshot"), TunaSweeperSaveTool::Execute(Request(TEXT("snapshot-delete"), Id))->GetBoolField(TEXT("ok")));
    }
    Listed = TunaSweeperSaveTool::Execute(Request(TEXT("snapshot-list")));
    TestEqual(TEXT("Deletion preserves the other snapshot"), Listed->GetArrayField(TEXT("snapshots")).Num(), 1);
    FFileHelper::LoadFileToArray(After, *Path);
    TestTrue(TEXT("Deleting snapshots leaves the active slot untouched"), Before == After);
    // A broken current save must not prevent removing an old developer snapshot.
    FFileHelper::SaveArrayToFile(TArray<uint8>{1, 2, 3}, *Path);
    TestTrue(TEXT("Delete works with an unreadable current save"), TunaSweeperSaveTool::Execute(Request(TEXT("snapshot-delete"), LegacyId))->GetBoolField(TEXT("ok")));
    TestTrue(TEXT("Last deletion empties the list"), TunaSweeperSaveTool::Execute(Request(TEXT("snapshot-list")))->GetArrayField(TEXT("snapshots")).IsEmpty());
    IFileManager::Get().DeleteDirectory(*Directory, false, true);
    IFileManager::Get().DeleteDirectory(*FPaths::GetPath(Path), false, true);
    return true;
}

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
    TestTrue(TEXT("Automatic backups can also be deleted"), TunaSweeperSaveTool::Execute(Request(TEXT("snapshot-delete"),
        FPaths::GetCleanFilename(Restored->GetStringField(TEXT("backupPath")))))->GetBoolField(TEXT("ok")));
    IFileManager::Get().DeleteDirectory(*FPaths::GetPath(SnapshotPath), false, true);
    IFileManager::Get().DeleteDirectory(*FPaths::GetPath(Path), false, true);
    return true;
}
#endif
