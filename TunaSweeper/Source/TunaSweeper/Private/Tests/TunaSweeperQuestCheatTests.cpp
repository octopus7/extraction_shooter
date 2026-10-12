#include "Inventory/TunaSweeperQuestCheat.h"
#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Inventory/TunaSweeperSaveGame.h"
#include "Inventory/TunaSweeperSaveTool.h"
#include "Game/TunaSweeperSafeSave.h"
#include "Subsystem/TunaSweeperQuestSubsystem.h"
#include "Settings/TunaSweeperBuildFlavor.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Dom/JsonObject.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Kismet/GameplayStatics.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperQuestCheatAuditTest, "TunaSweeper.QuestCheat.PrerequisiteAudit",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTunaSweeperQuestCheatAuditTest::RunTest(const FString& Parameters)
{
    using namespace TunaSweeperQuestCheat;
    TArray<FTunaSweeperQuestDefinition> Definitions;
    for (int32 Id = 1; Id <= 7; ++Id)
    {
        auto& D = Definitions.AddDefaulted_GetRef(); D.QuestId = Id;
        auto& O = D.Objectives.AddDefaulted_GetRef(); O.ObjectiveId = TEXT("goal"); O.RequiredCount = 3;
    }
    Definitions[1].RequiredCompletedQuestIds = {1};
    Definitions[2].RequiredCompletedQuestIds = {2};
    Definitions[3].RequiredCompletedQuestIds = {3};
    Definitions[4].RequiredCompletedQuestIds = {6}; // Unaccepted branch is not an error.
    auto* Save = NewObject<UTunaSweeperSaveGame>();
    Save->QuestCoinBalance = 42; Save->TotalExperiencePoints = 73; Save->NumericTrackedQuestId = 2;
    auto& Root = Save->NumericQuestProgressStates.AddDefaulted_GetRef(); Root.QuestId = 4; Root.State = ETunaSweeperQuestState::Accepted;
    auto& Pending = Save->NumericQuestProgressStates.AddDefaulted_GetRef(); Pending.QuestId = 2; Pending.State = ETunaSweeperQuestState::RewardAvailable;
    auto& Other = Save->NumericQuestProgressStates.AddDefaulted_GetRef(); Other.QuestId = 7; Other.State = ETunaSweeperQuestState::Accepted;
    TArray<uint8> Before, After;
    UGameplayStatics::SaveGameToMemory(Save, Before);
    FPrerequisiteAudit Report;
    TestTrue(TEXT("Audit succeeds"), Audit(*Save, Definitions, Report).IsEmpty());
    TestTrue(TEXT("Repair offered for missing ancestors"), Report.bCanFix);
    TestEqual(TEXT("Every broken edge reported once"), Report.Issues.Num(), 3);
    TestTrue(TEXT("Only three missing ancestors fixed"), Report.FixQuestIds == TArray<int32>({1, 2, 3}));
    TestTrue(TEXT("Reward pending is not completed"), Report.Issues.ContainsByPredicate([](const auto& I)
        { return I.QuestId == 3 && I.PrerequisiteId == 2 && I.PrerequisiteState == ETunaSweeperQuestState::RewardAvailable; }));
    UGameplayStatics::SaveGameToMemory(Save, After);
    TestTrue(TEXT("Audit never mutates save"), Before == After);
    TArray<int32> Fixed;
    TestTrue(TEXT("Repair succeeds"), Repair(*Save, Definitions, Fixed).IsEmpty());
    TestTrue(TEXT("Exact repair set"), Fixed == TArray<int32>({1, 2, 3}));
    auto Find = [&](int32 Id) { return Save->NumericQuestProgressStates.FindByPredicate([Id](const auto& P) { return P.QuestId == Id; }); };
    for (int32 Id : {1, 2, 3})
        if (const auto* Progress = Find(Id))
        {
            TestEqual(TEXT("Ancestor reward completed"), Progress->State, ETunaSweeperQuestState::RewardCompleted);
            TestTrue(TEXT("Ancestor objectives filled"), Progress->ObjectiveProgress.Num() == 1 && Progress->ObjectiveProgress[0].CurrentCount == 3);
        }
        else AddError(TEXT("Repaired ancestor missing"));
    TestEqual(TEXT("Started target retained"), Find(4)->State, ETunaSweeperQuestState::Accepted);
    TestEqual(TEXT("Unrelated progress retained"), Find(7)->State, ETunaSweeperQuestState::Accepted);
    TestEqual(TEXT("Completed ancestor tracking cleared"), Save->NumericTrackedQuestId, 0);
    TestEqual(TEXT("No coins awarded"), Save->QuestCoinBalance, 42);
    TestEqual(TEXT("No XP awarded"), Save->TotalExperiencePoints, int64(73));
    TestTrue(TEXT("Post repair audit succeeds"), Audit(*Save, Definitions, Report).IsEmpty());
    TestTrue(TEXT("Fixed save has no issues or fix button"), Report.Issues.IsEmpty() && !Report.bCanFix);
    TestTrue(TEXT("Second repair no-op"), Repair(*Save, Definitions, Fixed).IsEmpty() && Fixed.IsEmpty());
    UGameplayStatics::SaveGameToMemory(Save, Before);
    Definitions[0].RequiredCompletedQuestIds = {4};
    Audit(*Save, Definitions, Report);
    TestTrue(TEXT("Cycle is explained and not fixable"), !Report.bCanFix && Report.Issues.ContainsByPredicate([](const auto& I) { return I.Code == TEXT("quest_cycle"); }));
    TestFalse(TEXT("Cycle blocks repair"), Repair(*Save, Definitions, Fixed).IsEmpty());
    Definitions[0].RequiredCompletedQuestIds = {999};
    Audit(*Save, Definitions, Report);
    TestTrue(TEXT("Missing definition explained and not fixable"), !Report.bCanFix && Report.Issues.ContainsByPredicate([](const auto& I) { return I.PrerequisiteId == 999 && I.Code == TEXT("quest_prerequisite_missing"); }));
    TestFalse(TEXT("Missing definition blocks repair"), Repair(*Save, Definitions, Fixed).IsEmpty());
    UGameplayStatics::SaveGameToMemory(Save, After);
    TestTrue(TEXT("Invalid graph never partially repaired"), Before == After);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperQuestCheatAuditSaveTest, "TunaSweeper.QuestCheat.AuditTransaction",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTunaSweeperQuestCheatAuditSaveTest::RunTest(const FString& Parameters)
{
    auto* Owner = NewObject<UGameInstance>();
    auto* Quests = NewObject<UTunaSweeperQuestSubsystem>(Owner);
    TArray<FTunaSweeperQuestDefinition> Definitions;
    if (!TestTrue(TEXT("Catalog loads"), Quests->LoadQuestData() && Quests->GetAllQuestDefinitions(Definitions))) return false;
    const auto* Target = Definitions.FindByPredicate([](const auto& D) { return !D.RequiredCompletedQuestIds.IsEmpty(); });
    if (!Target) { AddWarning(TEXT("No prerequisite quest in active catalog.")); return true; }
    const FString Path = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("Automation/QuestAudit") / FGuid::NewGuid().ToString() / TEXT("TunaSweeperSave_Slot01.sav"));
    auto* Save = NewObject<UTunaSweeperSaveGame>(); Save->BuildFlavor = TunaSweeperBuildFlavor::GetName();
    Save->InventorySlots.SetNum(40); Save->EquipmentSlots.SetNum(8); Save->QuestCoinBalance = 77;
    auto& Progress = Save->NumericQuestProgressStates.AddDefaulted_GetRef(); Progress.QuestId = Target->QuestId; Progress.State = ETunaSweeperQuestState::Accepted;
    auto Validator = [](const USaveGame& S) { return S.GetClass() == UTunaSweeperSaveGame::StaticClass(); };
    if (!TestTrue(TEXT("Broken fixture saved"), TunaSweeperSafeSave::SaveGameFileFailClosed(Save, Path, Validator))) return false;
    auto Request = MakeShared<FJsonObject>();
    Request->SetNumberField(TEXT("version"), 1); Request->SetStringField(TEXT("operation"), TEXT("quest-audit"));
    Request->SetStringField(TEXT("savePath"), Path); Request->SetStringField(TEXT("flavor"), Save->BuildFlavor.ToString()); Request->SetNumberField(TEXT("slot"), 1);
    TArray<uint8> Original, Bytes; FFileHelper::LoadFileToArray(Original, *Path);
    auto Report = TunaSweeperSaveTool::Execute(Request);
    if (!TestTrue(TEXT("Audit operation supported"), Report->GetBoolField(TEXT("ok")))) return false;
    TestTrue(TEXT("Fix exposed only for repairable issues"), Report->GetBoolField(TEXT("canFix")) && !Report->GetArrayField(TEXT("issues")).IsEmpty());
    FFileHelper::LoadFileToArray(Bytes, *Path); TestTrue(TEXT("Audit read-only"), Original == Bytes);
    Request->SetStringField(TEXT("operation"), TEXT("quest-fix"));
    Request->SetStringField(TEXT("expectedHash"), TEXT("stale"));
    TestEqual(TEXT("Fix refuses stale audit"), TunaSweeperSaveTool::Execute(Request)->GetStringField(TEXT("code")), FString(TEXT("stale_save")));
    Request->SetStringField(TEXT("expectedHash"), Report->GetStringField(TEXT("sourceHash")));
    auto Result = TunaSweeperSaveTool::Execute(Request);
    if (!TestTrue(TEXT("Fix preview succeeds"), Result->GetBoolField(TEXT("ok")))) return false;
    FFileHelper::LoadFileToArray(Bytes, *Path); TestTrue(TEXT("Fix preview read-only"), Original == Bytes);
    Request->SetBoolField(TEXT("commit"), true);
    Result = TunaSweeperSaveTool::Execute(Request);
    if (!TestTrue(TEXT("Fix commits"), Result->GetBoolField(TEXT("ok")) && Result->GetBoolField(TEXT("committed")))) return false;
    FFileHelper::LoadFileToArray(Bytes, *Result->GetStringField(TEXT("backupPath"))); TestTrue(TEXT("Backup preserves broken save exactly"), Original == Bytes);
    auto* Reload = Cast<UTunaSweeperSaveGame>(TunaSweeperSafeSave::LoadVerifiedSaveFile(Path, Validator));
    if (!TestNotNull(TEXT("Repaired save reloads"), Reload)) return false;
    const auto* Root = Reload->NumericQuestProgressStates.FindByPredicate([&](const auto& P) { return P.QuestId == Target->QuestId; });
    TestTrue(TEXT("Target remains accepted"), Root && Root->State == ETunaSweeperQuestState::Accepted);
    TestEqual(TEXT("No rewards granted"), Reload->QuestCoinBalance, 77);
    Request->RemoveField(TEXT("commit")); Request->RemoveField(TEXT("expectedHash")); Request->SetStringField(TEXT("operation"), TEXT("quest-audit"));
    Report = TunaSweeperSaveTool::Execute(Request);
    if (TestTrue(TEXT("Recheck succeeds"), Report->GetBoolField(TEXT("ok"))))
        TestTrue(TEXT("Recheck clears issues and hides fix"), Report->GetArrayField(TEXT("issues")).IsEmpty() && !Report->GetBoolField(TEXT("canFix")));
    Request->SetStringField(TEXT("operation"), TEXT("quest-fix")); Request->SetStringField(TEXT("expectedHash"), Report->GetStringField(TEXT("sourceHash")));
    UWorld* World = UWorld::CreateWorld(EWorldType::PIE, false);
    FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::PIE); Context.SetCurrentWorld(World);
    TestEqual(TEXT("Fix blocked while playing"), TunaSweeperSaveTool::Execute(Request)->GetStringField(TEXT("code")), FString(TEXT("game_running")));
    World->DestroyWorld(false); GEngine->DestroyWorldContext(World); World->RemoveFromRoot();
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperQuestCheatGraphTest, "TunaSweeper.QuestCheat.Ancestors",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTunaSweeperQuestCheatGraphTest::RunTest(const FString& Parameters)
{
    TArray<FTunaSweeperQuestDefinition> Definitions;
    for (int32 Id = 1; Id <= 5; ++Id)
    {
        auto& D = Definitions.AddDefaulted_GetRef(); D.QuestId = Id;
        auto& Objective = D.Objectives.AddDefaulted_GetRef(); Objective.ObjectiveId = TEXT("goal"); Objective.RequiredCount = 3;
    }
    Definitions[1].RequiredCompletedQuestIds = {1};
    Definitions[2].RequiredCompletedQuestIds = {1};
    Definitions[3].RequiredCompletedQuestIds = {2, 3};
    auto* Save = NewObject<UTunaSweeperSaveGame>();
    Save->QuestCoinBalance = 91; Save->TotalExperiencePoints = 72;
    Save->CompletedScenarioFlags.Add(TEXT("untouched"));
    auto& Target = Save->NumericQuestProgressStates.AddDefaulted_GetRef();
    Target.QuestId = 4; Target.State = ETunaSweeperQuestState::Accepted;
    auto& Other = Save->NumericQuestProgressStates.AddDefaulted_GetRef();
    Other.QuestId = 5; Other.State = ETunaSweeperQuestState::RewardAvailable;
    Save->NumericTrackedQuestId = 4;
    TArray<int32> Completed;
    TestTrue(TEXT("Prerequisites apply"), TunaSweeperQuestCheat::Apply(*Save, Definitions, 4, false, Completed).IsEmpty());
    TestEqual(TEXT("Diamond shared ancestor processed once"), Completed.Num(), 3);
    TestFalse(TEXT("Target excluded"), Completed.Contains(4));
    auto Find = [&](int32 Id) { return Save->NumericQuestProgressStates.FindByPredicate([Id](const auto& P) { return P.QuestId == Id; }); };
    TestEqual(TEXT("Target remains accepted"), Find(4)->State, ETunaSweeperQuestState::Accepted);
    TestEqual(TEXT("Target tracking retained"), Save->NumericTrackedQuestId, 4);
    for (int32 Id : Completed)
    {
        TestEqual(TEXT("Ancestor reward-completed"), Find(Id)->State, ETunaSweeperQuestState::RewardCompleted);
        TestEqual(TEXT("Ancestor objective filled"), Find(Id)->ObjectiveProgress[0].CurrentCount, 3);
    }
    TestTrue(TEXT("Include target applies"), TunaSweeperQuestCheat::Apply(*Save, Definitions, 4, true, Completed).IsEmpty());
    TestEqual(TEXT("Completed tracking cleared"), Save->NumericTrackedQuestId, 0);
    TestEqual(TEXT("Unrelated branch unchanged"), Find(5)->State, ETunaSweeperQuestState::RewardAvailable);
    TestEqual(TEXT("No coins granted"), Save->QuestCoinBalance, 91);
    TestEqual(TEXT("No experience granted"), Save->TotalExperiencePoints, int64(72));
    TestTrue(TEXT("Scenario flags preserved"), Save->CompletedScenarioFlags.Contains(TEXT("untouched")));
    TestTrue(TEXT("Idempotent repeat"), TunaSweeperQuestCheat::Apply(*Save, Definitions, 4, true, Completed).IsEmpty());
    TestEqual(TEXT("No duplicate quest entries"), Save->NumericQuestProgressStates.Num(), 5);
    TArray<uint8> Before, After;
    UGameplayStatics::SaveGameToMemory(Save, Before);
    Definitions[0].RequiredCompletedQuestIds = {4};
    TestEqual(TEXT("Cycle rejected"), TunaSweeperQuestCheat::Apply(*Save, Definitions, 4, true, Completed), FString(TEXT("quest_cycle")));
    TestTrue(TEXT("Invalid plan not exposed"), Completed.IsEmpty());
    Definitions[0].RequiredCompletedQuestIds = {999};
    TestEqual(TEXT("Missing ancestor rejected"), TunaSweeperQuestCheat::Apply(*Save, Definitions, 4, true, Completed), FString(TEXT("quest_prerequisite_missing")));
    TestEqual(TEXT("Unknown target rejected"), TunaSweeperQuestCheat::Apply(*Save, Definitions, 999, true, Completed), FString(TEXT("quest_not_found")));
    UGameplayStatics::SaveGameToMemory(Save, After);
    TestTrue(TEXT("Invalid graph leaves entire save unchanged"), Before == After);
    Definitions.Reset();
    for (int32 Id = 1; Id <= 10000; ++Id)
    { auto& D = Definitions.AddDefaulted_GetRef(); D.QuestId = Id; if (Id > 1) D.RequiredCompletedQuestIds.Add(Id - 1); }
    TestTrue(TEXT("Long chain uses iterative traversal"), TunaSweeperQuestCheat::Apply(*Save, Definitions, 10000, false, Completed).IsEmpty());
    TestEqual(TEXT("Every ancestor included"), Completed.Num(), 9999);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperQuestCheatSaveTest, "TunaSweeper.QuestCheat.OfflineSave",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTunaSweeperQuestCheatSaveTest::RunTest(const FString& Parameters)
{
    auto* Owner = NewObject<UGameInstance>();
    auto* Quests = NewObject<UTunaSweeperQuestSubsystem>(Owner);
    TArray<FTunaSweeperQuestDefinition> Definitions;
    if (!TestTrue(TEXT("Active quest catalog loads"), Quests->LoadQuestData() && Quests->GetAllQuestDefinitions(Definitions))) return false;
    if (Definitions.IsEmpty()) { AddWarning(TEXT("No active catalog entries; transaction integration unavailable.")); return true; }
    const auto* WithParents = Definitions.FindByPredicate([](const auto& D) { return !D.RequiredCompletedQuestIds.IsEmpty(); });
    const auto& TargetDefinition = WithParents ? *WithParents : Definitions[0];
    const FString Path = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("Automation/QuestCheat") / FGuid::NewGuid().ToString() / TEXT("TunaSweeperSave_Slot01.sav"));
    auto* Save = NewObject<UTunaSweeperSaveGame>(); Save->BuildFlavor = TunaSweeperBuildFlavor::GetName();
    Save->InventorySlots.SetNum(40); Save->EquipmentSlots.SetNum(8); Save->QuestCoinBalance = 77;
    auto Validator = [](const USaveGame& S) { return S.GetClass() == UTunaSweeperSaveGame::StaticClass(); };
    if (!TestTrue(TEXT("Synthetic save written"), TunaSweeperSafeSave::SaveGameFileFailClosed(Save, Path, Validator))) return false;
    auto Request = MakeShared<FJsonObject>();
    Request->SetNumberField(TEXT("version"), 1); Request->SetStringField(TEXT("operation"), TEXT("quest-complete"));
    Request->SetStringField(TEXT("savePath"), Path); Request->SetStringField(TEXT("flavor"), Save->BuildFlavor.ToString());
    Request->SetNumberField(TEXT("slot"), 1); Request->SetNumberField(TEXT("questId"), TargetDefinition.QuestId);
    Request->SetBoolField(TEXT("includeTarget"), true);
    TArray<uint8> Original, Bytes; FFileHelper::LoadFileToArray(Original, *Path);
    auto Result = TunaSweeperSaveTool::Execute(Request);
    if (!TestTrue(TEXT("Preview succeeds"), Result->GetBoolField(TEXT("ok")))) return false;
    FFileHelper::LoadFileToArray(Bytes, *Path); TestTrue(TEXT("Preview never writes"), Original == Bytes);
    Request->SetStringField(TEXT("expectedHash"), TEXT("stale")); Request->SetBoolField(TEXT("commit"), true);
    TestEqual(TEXT("Stale save rejected"), TunaSweeperSaveTool::Execute(Request)->GetStringField(TEXT("code")), FString(TEXT("stale_save")));
    Request->SetStringField(TEXT("expectedHash"), Result->GetStringField(TEXT("sourceHash")));
    Result = TunaSweeperSaveTool::Execute(Request);
    if (!TestTrue(TEXT("Offline commit succeeds"), Result->GetBoolField(TEXT("ok")))) return false;
    FFileHelper::LoadFileToArray(Bytes, *Result->GetStringField(TEXT("backupPath")));
    TestTrue(TEXT("Backup exactly preserves original"), Bytes == Original);
    auto* Reload = Cast<UTunaSweeperSaveGame>(TunaSweeperSafeSave::LoadVerifiedSaveFile(Path, Validator));
    if (!TestNotNull(TEXT("Edited save reloads"), Reload)) return false;
    const auto* Completed = Reload->NumericQuestProgressStates.FindByPredicate([&](const auto& P) { return P.QuestId == TargetDefinition.QuestId; });
    TestTrue(TEXT("Completion survives reload"), Completed && Completed->State == ETunaSweeperQuestState::RewardCompleted);
    TestEqual(TEXT("Coins not granted"), Reload->QuestCoinBalance, 77);
    auto Inspect = MakeShared<FJsonObject>();
    Inspect->SetNumberField(TEXT("version"), 1);
    Inspect->SetStringField(TEXT("operation"), TEXT("quest-inspect"));
    Inspect->SetStringField(TEXT("savePath"), Path);
    Inspect->SetStringField(TEXT("flavor"), Save->BuildFlavor.ToString());
    Inspect->SetNumberField(TEXT("slot"), 1);
    auto View = TunaSweeperSaveTool::Execute(Inspect);
    if (TestTrue(TEXT("Quest list reads selected save"), View->GetBoolField(TEXT("ok"))))
    {
        const auto& Rows = View->GetArrayField(TEXT("quests"));
        TestEqual(TEXT("Every catalog quest listed"), Rows.Num(), Definitions.Num());
        const auto* Row = Rows.FindByPredicate([&](const auto& V) { return V->AsObject()->GetIntegerField(TEXT("questId")) == TargetDefinition.QuestId; });
        if (TestNotNull(TEXT("Completed quest listed"), Row))
        {
            TestEqual(TEXT("Title comes from definition"), (*Row)->AsObject()->GetStringField(TEXT("titleStringKey")), TargetDefinition.TitleStringKey.ToString());
            TestEqual(TEXT("Persisted completion shown"), (*Row)->AsObject()->GetStringField(TEXT("statusKey")), FString(TEXT("ui.quest_cheat.state.completed")));
        }
    }
    FFileHelper::LoadFileToArray(Bytes, *Path);
    Reload->NumericTrackedQuestId = TargetDefinition.QuestId;
    for (auto& Progress : Reload->NumericQuestProgressStates)
        if (Progress.QuestId == TargetDefinition.QuestId) Progress.State = ETunaSweeperQuestState::Accepted;
    if (!TestTrue(TEXT("Tracked accepted fixture written"), TunaSweeperSafeSave::SaveGameFileFailClosed(Reload, Path, Validator))) return false;
    FFileHelper::LoadFileToArray(Original, *Path);
    View = TunaSweeperSaveTool::Execute(Inspect);
    if (TestTrue(TEXT("Accepted fixture inspected"), View->GetBoolField(TEXT("ok"))))
    {
        const auto* Row = View->GetArrayField(TEXT("quests")).FindByPredicate([&](const auto& V) { return V->AsObject()->GetIntegerField(TEXT("questId")) == TargetDefinition.QuestId; });
        if (TestNotNull(TEXT("Accepted target listed"), Row)) TestEqual(TEXT("Accepted status displayed"), (*Row)->AsObject()->GetStringField(TEXT("statusKey")), FString(TEXT("ui.quest_cheat.state.accepted")));
    }
    FFileHelper::LoadFileToArray(Bytes, *Path);
    TestTrue(TEXT("Inspect never writes save"), Original == Bytes);
    Request->RemoveField(TEXT("expectedHash"));
    Request->SetBoolField(TEXT("commit"), false);
    Request->SetBoolField(TEXT("includeTarget"), false);
    Request->SetStringField(TEXT("operation"), TEXT("quest-reset"));
    Result = TunaSweeperSaveTool::Execute(Request);
    FFileHelper::LoadFileToArray(Bytes, *Path); TestTrue(TEXT("Reset preview never writes"), Original == Bytes);
    if (TestTrue(TEXT("Reset preview supported"), Result->GetBoolField(TEXT("ok"))))
    {
        Request->SetStringField(TEXT("expectedHash"), Result->GetStringField(TEXT("sourceHash")));
        Request->SetBoolField(TEXT("commit"), true);
        Result = TunaSweeperSaveTool::Execute(Request);
        TestTrue(TEXT("Reset commits"), Result->GetBoolField(TEXT("ok")));
        Reload = Cast<UTunaSweeperSaveGame>(TunaSweeperSafeSave::LoadVerifiedSaveFile(Path, Validator));
        if (TestNotNull(TEXT("Reset save reloads"), Reload))
        {
            TestFalse(TEXT("Target progress removed"), Reload->NumericQuestProgressStates.ContainsByPredicate([&](const auto& P) { return P.QuestId == TargetDefinition.QuestId; }));
            TestEqual(TEXT("Reset grants no coins"), Reload->QuestCoinBalance, 77);
            TestEqual(TEXT("Reset clears target tracking"), Reload->NumericTrackedQuestId, 0);
            for (int32 ParentId : TargetDefinition.RequiredCompletedQuestIds)
            {
                const auto* Parent = Reload->NumericQuestProgressStates.FindByPredicate([&](const auto& P) { return P.QuestId == ParentId; });
                TestTrue(TEXT("Reset preserves completed prerequisites"), Parent && Parent->State == ETunaSweeperQuestState::RewardCompleted);
            }
        }
        View = TunaSweeperSaveTool::Execute(Inspect);
        if (TestTrue(TEXT("Reset list reloads"), View->GetBoolField(TEXT("ok"))))
        {
            const auto* Row = View->GetArrayField(TEXT("quests")).FindByPredicate([&](const auto& V) { return V->AsObject()->GetIntegerField(TEXT("questId")) == TargetDefinition.QuestId; });
            if (TestNotNull(TEXT("Reset quest listed"), Row)) TestEqual(TEXT("Reset target available"), (*Row)->AsObject()->GetStringField(TEXT("statusKey")), FString(TEXT("ui.quest_cheat.state.available")));
        }
    }
    FFileHelper::LoadFileToArray(Original, *Path);
    UWorld* World = UWorld::CreateWorld(EWorldType::PIE, false);
    FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::PIE); Context.SetCurrentWorld(World);
    TestEqual(TEXT("Executing during play is blocked"), TunaSweeperSaveTool::Execute(Request)->GetStringField(TEXT("code")), FString(TEXT("game_running")));
    World->DestroyWorld(false); GEngine->DestroyWorldContext(World); World->RemoveFromRoot();
    FFileHelper::LoadFileToArray(Bytes, *Path); TestTrue(TEXT("Blocked edit does not touch save"), Original == Bytes);
    return true;
}
#endif
