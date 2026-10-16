#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Inventory/TunaSweeperSaveTool.h"
#include "Inventory/TunaSweeperSaveGame.h"
#include "Game/TunaSweeperSafeSave.h"
#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperWorldProgressCheatTest, "TunaSweeper.SaveTool.WorldProgress",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaSweeperWorldProgressCheatTest::RunTest(const FString& Parameters)
{
    const FString Path = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("Automation/WorldProgressCheat") /
        FGuid::NewGuid().ToString() / TEXT("TunaSweeperSave_Slot01.sav"));
    auto* Save = NewObject<UTunaSweeperSaveGame>();
    Save->BuildFlavor = TEXT("Main");
    Save->InventorySlots.SetNum(40); Save->EquipmentSlots.SetNum(8);
    Save->QuestCoinBalance = 234; Save->TotalExperiencePoints = 567;
    FTunaSweeperWorldProgressSaveData Bridge;
    Bridge.ObjectId = TEXT("test.cheat.bridge"); Bridge.InfoId = TEXT("bridge"); Bridge.ProgressQuantity = 2;
    Save->WorldProgressStates.Add(Bridge);
    auto Other = Bridge; Other.ObjectId = TEXT("test.cheat.other"); Save->WorldProgressStates.Add(Other);
    auto Demo = Bridge; Demo.ObjectId = TEXT("demo.water_intake.blocked_screen"); Save->WorldProgressStates.Add(Demo);
    FTunaSweeperNumericQuestProgressSaveData Quest;
    Quest.QuestId = 38; Quest.State = ETunaSweeperQuestState::RewardCompleted;
    Save->NumericQuestProgressStates.Add(Quest);
    auto Validator = [](const USaveGame& Object) { return Object.GetClass() == UTunaSweeperSaveGame::StaticClass(); };
    if (!TestTrue(TEXT("Synthetic save created"), TunaSweeperSafeSave::SaveGameFileFailClosed(Save, Path, Validator))) return false;
    auto Request = [&](const TCHAR* Op)
    {
        auto R = MakeShared<FJsonObject>();
        R->SetNumberField(TEXT("version"), 1); R->SetStringField(TEXT("operation"), Op);
        R->SetStringField(TEXT("savePath"), Path); R->SetStringField(TEXT("flavor"), TEXT("Main")); R->SetNumberField(TEXT("slot"), 1);
        return R;
    };
    auto Listed = TunaSweeperSaveTool::Execute(Request(TEXT("world-inspect")));
    if (!TestTrue(TEXT("World list available"), Listed->GetBoolField(TEXT("ok")))) return false;
    bool bDemoListed = false, bUnsavedListed = false;
    for (const auto& V : Listed->GetArrayField(TEXT("worldProgress")))
    {
        const auto Row = V->AsObject();
        bDemoListed |= Row->GetStringField(TEXT("objectId")).StartsWith(TEXT("demo.water_intake."));
        bUnsavedListed |= !Row->GetBoolField(TEXT("recorded"));
    }
    TestFalse(TEXT("Excluded intake does not appear"), bDemoListed);
    TestTrue(TEXT("Unrecorded placed objects are listed"), bUnsavedListed);
    TArray<uint8> Before, After;
    FFileHelper::LoadFileToArray(Before, *Path);
    auto Edit = Request(TEXT("world-set"));
    Edit->SetStringField(TEXT("objectId"), TEXT("test.cheat.bridge"));
    Edit->SetStringField(TEXT("state"), TEXT("InProgress")); Edit->SetNumberField(TEXT("quantity"), 0);
    auto Preview = TunaSweeperSaveTool::Execute(Edit);
    if (!TestTrue(TEXT("Reset preview succeeds"), Preview->GetBoolField(TEXT("ok")))) return false;
    FFileHelper::LoadFileToArray(After, *Path); TestTrue(TEXT("Preview is read only"), Before == After);
    Edit->SetBoolField(TEXT("commit"), true);
    TestFalse(TEXT("World commit requires displayed source hash"), TunaSweeperSaveTool::Execute(Edit)->GetBoolField(TEXT("ok")));
    Edit->SetStringField(TEXT("expectedHash"), TEXT("stale"));
    TestEqual(TEXT("Stale write rejected"), TunaSweeperSaveTool::Execute(Edit)->GetStringField(TEXT("code")), FString(TEXT("stale_save")));
    Edit->SetStringField(TEXT("expectedHash"), Preview->GetStringField(TEXT("sourceHash")));
    auto Committed = TunaSweeperSaveTool::Execute(Edit);
    if (!TestTrue(TEXT("Reset committed"), Committed->GetBoolField(TEXT("ok")))) return false;
    TArray<uint8> Backup; FFileHelper::LoadFileToArray(Backup, *Committed->GetStringField(TEXT("backupPath")));
    TestTrue(TEXT("Exact original backup"), Before == Backup);
    auto* Reload = Cast<UTunaSweeperSaveGame>(TunaSweeperSafeSave::LoadVerifiedSaveFile(Path, Validator));
    if (!TestNotNull(TEXT("Reload succeeds"), Reload)) return false;
    TestEqual(TEXT("Explicit reset record prevents legacy facility promotion"), Reload->WorldProgressStates[0].ProgressQuantity, 0);
    TestTrue(TEXT("Reset is explicitly incomplete"), Reload->WorldProgressStates[0].State == ETunaSweeperWorldProgressState::InProgress);
    TestEqual(TEXT("Other world progress unchanged"), Reload->WorldProgressStates[1].ProgressQuantity, 2);
    TestEqual(TEXT("Excluded intake preserved"), Reload->WorldProgressStates[2].ProgressQuantity, 2);
    TestTrue(TEXT("Quest rewards not changed"), Reload->NumericQuestProgressStates[0].State == ETunaSweeperQuestState::RewardCompleted);
    TestEqual(TEXT("Coins unchanged"), Reload->QuestCoinBalance, 234);
    TestEqual(TEXT("XP unchanged"), Reload->TotalExperiencePoints, int64(567));
    Edit->RemoveField(TEXT("commit")); Edit->RemoveField(TEXT("expectedHash"));
    Edit->SetStringField(TEXT("objectId"), TEXT("demo.water_intake.blocked_screen"));
    TestFalse(TEXT("Direct API cannot edit excluded intake"), TunaSweeperSaveTool::Execute(Edit)->GetBoolField(TEXT("ok")));
    Edit->SetStringField(TEXT("objectId"), TEXT("invented.id"));
    TestFalse(TEXT("Unknown new IDs rejected"), TunaSweeperSaveTool::Execute(Edit)->GetBoolField(TEXT("ok")));
    Edit->SetStringField(TEXT("objectId"), TEXT("test.cheat.bridge")); Edit->SetNumberField(TEXT("quantity"), -1);
    TestFalse(TEXT("Negative quantity rejected"), TunaSweeperSaveTool::Execute(Edit)->GetBoolField(TEXT("ok")));
    Edit->SetNumberField(TEXT("quantity"), 1); Edit->SetStringField(TEXT("state"), TEXT("Invented"));
    TestFalse(TEXT("Unknown state rejected"), TunaSweeperSaveTool::Execute(Edit)->GetBoolField(TEXT("ok")));
    Edit->SetStringField(TEXT("objectId"), TEXT("rail.lubrication.village_01"));
    Edit->SetStringField(TEXT("state"), TEXT("Completed")); Edit->SetNumberField(TEXT("quantity"), 2);
    TestFalse(TEXT("Known object quantity bounds enforced"), TunaSweeperSaveTool::Execute(Edit)->GetBoolField(TEXT("ok")));
    Edit->SetNumberField(TEXT("quantity"), 1);
    Preview = TunaSweeperSaveTool::Execute(Edit);
    if (!TestTrue(TEXT("Previously unrecorded station can be completed"), Preview->GetBoolField(TEXT("ok")))) return false;
    Edit->SetStringField(TEXT("expectedHash"), Preview->GetStringField(TEXT("sourceHash"))); Edit->SetBoolField(TEXT("commit"), true);
    if (!TestTrue(TEXT("Station completion committed"), TunaSweeperSaveTool::Execute(Edit)->GetBoolField(TEXT("ok")))) return false;
    Reload = Cast<UTunaSweeperSaveGame>(TunaSweeperSafeSave::LoadVerifiedSaveFile(Path, Validator));
    if (!TestNotNull(TEXT("Station save reloads"), Reload)) return false;
    TestEqual(TEXT("Exactly one new record"), Reload->WorldProgressStates.Num(), 4);
    TestTrue(TEXT("Station is completed"), Reload->WorldProgressStates.Last().State == ETunaSweeperWorldProgressState::Completed);
    TestFalse(TEXT("Tool does not implicitly complete another station"), Reload->WorldProgressStates.ContainsByPredicate(
        [](const auto& S) { return S.ObjectId == TEXT("rail.lubrication.village_02"); }));
    return true;
}
#endif
