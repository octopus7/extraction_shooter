#include "TunaSweeperQuestCheatTool.h"
#include "TunaSweeperEditorText.h"
#include "Dom/JsonObject.h"
#include "Editor.h"
#include "Framework/Docking/TabManager.h"
#include "HAL/PlatformProcess.h"
#include "HAL/FileManager.h"
#include "Inventory/TunaSweeperSaveTool.h"
#include "Quest/TunaSweeperQuestId.h"
#include "Widgets/Input/NumericTypeInterface.h"
#include "ToolMenus.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/SListView.h"

namespace TunaSweeperQuestCheatEditor
{
    const FName TabId(TEXT("TunaSweeperQuestCheat"));
    TArray<FProcHandle> StandaloneProcesses;

    bool IsLocked()
    {
        bool bStandalone = false;
        for (int32 Index = StandaloneProcesses.Num() - 1; Index >= 0; --Index)
        {
            if (FPlatformProcess::IsProcRunning(StandaloneProcesses[Index])) bStandalone = true;
            else { FPlatformProcess::CloseProc(StandaloneProcesses[Index]); StandaloneProcesses.RemoveAt(Index); }
        }
        return !GEditor || GEditor->IsPlaySessionInProgress() || GEditor->PlayWorld ||
            GEditor->IsPlayingViaLauncher() || GEditor->IsPlayingOnLocalPCSession() || bStandalone;
    }

    FText Text(const TCHAR* Key)
    {
        return TunaSweeperEditorText::Resolve(Key);
    }

    struct FQuestIdTypeInterface : TDefaultNumericTypeInterface<int32>
    {
        virtual FString ToString(const int32& Value) const override { return TunaSweeperQuestId::Format(Value); }
    };

    class SQuestCheatPanel final : public SCompoundWidget
    {
    public:
        SLATE_BEGIN_ARGS(SQuestCheatPanel) {} SLATE_END_ARGS()
        void Construct(const FArguments&)
        {
            RefreshSelection();
            ChildSlot
            [
                SNew(SBorder).Padding(16)
                [
                    SNew(SVerticalBox)
                    + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 10)
                    [ SNew(STextBlock).Text(Text(TEXT("ui.quest_cheat.description"))).AutoWrapText(true) ]
                    + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 10)
                    [ SNew(STextBlock).Text_Lambda([this] { return SelectionText; }) ]
                    + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 10)
                    [ SNew(STextBlock).Text(Text(TEXT("ui.quest_cheat.locked")))
                        .Visibility_Lambda([] { return IsLocked() ? EVisibility::Visible : EVisibility::Collapsed; }) ]
                    + SVerticalBox::Slot().AutoHeight()
                    [
                        SNew(SVerticalBox).IsEnabled_Lambda([this] { return !IsLocked() && Selection.IsValid() && Selection->GetBoolField(TEXT("ok")); })
                        + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 6)
                        [ SNew(STextBlock).Text(Text(TEXT("ui.quest_cheat.id"))) ]
                        + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 12)
                        [ SNew(SNumericEntryBox<int32>).TypeInterface(MakeShared<FQuestIdTypeInterface>()).MinValue(1).MaxValue(MAX_int32).AllowSpin(false)
                            .Value_Lambda([this]() -> TOptional<int32> { return QuestId; })
                            .OnValueChanged_Lambda([this](int32 Value)
                            {
                                QuestId = Value;
                                if (QuestList)
                                {
                                    QuestList->ClearSelection();
                                    if (const auto* Row = QuestRows.FindByPredicate([Value](const auto& Entry) { return Entry->GetIntegerField(TEXT("questId")) == Value; }))
                                        QuestList->SetSelection(*Row);
                                }
                            }) ]
                        + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 6)
                        [ SNew(SButton).Text(Text(TEXT("ui.quest_cheat.complete")))
                            .OnClicked_Lambda([this] { return Apply(true); }) ]
                        + SVerticalBox::Slot().AutoHeight()
                        [ SNew(SButton).Text(Text(TEXT("ui.quest_cheat.prerequisites")))
                            .OnClicked_Lambda([this] { return Apply(false); }) ]
                        + SVerticalBox::Slot().AutoHeight().Padding(0, 6, 0, 0)
                        [ SNew(SButton).Text(Text(TEXT("ui.quest_cheat.reset")))
                            .ToolTipText(Text(TEXT("ui.quest_cheat.reset_help")))
                            .OnClicked_Lambda([this] { return Apply(false, true); }) ]
                    ]
                    + SVerticalBox::Slot().AutoHeight().Padding(0, 12, 0, 6)
                    [ SNew(SButton).Text(Text(TEXT("ui.quest_cheat.refresh")))
                        .IsEnabled_Lambda([] { return !IsLocked(); })
                        .OnClicked_Lambda([this] { RefreshSelection(); RefreshQuests(true); return FReply::Handled(); }) ]
                    + SVerticalBox::Slot().AutoHeight()
                    [ SNew(STextBlock).Text_Lambda([this] { return ListStatus; }).AutoWrapText(true) ]
                    + SVerticalBox::Slot().FillHeight(1).Padding(0, 6, 0, 0)
                    [ SAssignNew(QuestList, SListView<TSharedPtr<FJsonObject>>)
                        .ListItemsSource(&QuestRows).SelectionMode(ESelectionMode::Single)
                        .IsEnabled_Lambda([] { return !IsLocked(); })
                        .OnGenerateRow_Lambda([](TSharedPtr<FJsonObject> Row, const TSharedRef<STableViewBase>& Owner)
                        {
                            const FString Title = Row->GetStringField(TEXT("title"));
                            return SNew(STableRow<TSharedPtr<FJsonObject>>, Owner).Padding(6)
                                [ SNew(STextBlock).Text(FText::Format(Text(TEXT("ui.quest_cheat.row")),
                                    FText::FromString(TunaSweeperQuestId::Format(Row->GetIntegerField(TEXT("questId")))),
                                    Title.IsEmpty() ? Text(TEXT("ui.quest_cheat.untitled")) : FText::FromString(Title),
                                    Text(*Row->GetStringField(TEXT("statusKey"))))) ];
                        })
                        .OnSelectionChanged_Lambda([this](TSharedPtr<FJsonObject> Row, ESelectInfo::Type)
                        { if (Row && !IsLocked()) QuestId = Row->GetIntegerField(TEXT("questId")); }) ]
                    + SVerticalBox::Slot().AutoHeight().Padding(0, 12, 0, 0)
                    [ SNew(STextBlock).Text_Lambda([this] { return Status; }).AutoWrapText(true) ]
                ]
            ];
            RefreshQuests(true);
        }
        virtual void Tick(const FGeometry& Geometry, double Time, float Delta) override
        {
            SCompoundWidget::Tick(Geometry, Time, Delta);
            if (Time >= NextRefresh && !IsLocked()) { RefreshSelection(); NextRefresh = Time + 1.0; }
        }
    private:
        void RefreshSelection()
        {
            Selection = TunaSweeperSaveTool::GetQuestCheatSelection();
            SelectionText = Selection->GetBoolField(TEXT("ok"))
                ? FText::Format(Text(TEXT("ui.quest_cheat.slot")), FText::FromString(Selection->GetStringField(TEXT("flavor"))),
                    FText::AsNumber(Selection->GetIntegerField(TEXT("slot"))))
                : Text(TEXT("ui.quest_cheat.selection_invalid"));
            if (QuestList) RefreshQuests(false);
        }
        void RefreshQuests(bool bForce)
        {
            if (IsLocked()) return;
            const FString Path = Selection && Selection->GetBoolField(TEXT("ok")) ? Selection->GetStringField(TEXT("savePath")) : FString();
            const FDateTime Timestamp = Path.IsEmpty() ? FDateTime::MinValue() : IFileManager::Get().GetTimeStamp(*Path);
            if (!bForce && Path == ListedPath && Timestamp == ListedTimestamp) return;
            ListedPath = Path; ListedTimestamp = Timestamp;
            QuestRows.Reset();
            ListStatus = Text(TEXT("ui.quest_cheat.selection_invalid"));
            if (!Path.IsEmpty())
            {
                auto Request = MakeShared<FJsonObject>();
                Request->SetNumberField(TEXT("version"), 1);
                Request->SetStringField(TEXT("operation"), TEXT("quest-inspect"));
                Request->SetStringField(TEXT("savePath"), Path);
                Request->SetStringField(TEXT("flavor"), Selection->GetStringField(TEXT("flavor")));
                Request->SetNumberField(TEXT("slot"), Selection->GetIntegerField(TEXT("slot")));
                const auto Result = TunaSweeperSaveTool::Execute(Request);
                if (Result->GetBoolField(TEXT("ok")))
                {
                    for (const auto& Value : Result->GetArrayField(TEXT("quests"))) QuestRows.Add(Value->AsObject());
                    ListStatus = Text(TEXT("ui.quest_cheat.list_header"));
                }
                else ListStatus = FText::Format(Text(TEXT("ui.quest_cheat.error")), FText::FromString(Result->GetStringField(TEXT("code"))));
            }
            QuestList->RequestListRefresh();
            if (const auto* Selected = QuestRows.FindByPredicate([this](const auto& Row) { return Row->GetIntegerField(TEXT("questId")) == QuestId; }))
                QuestList->SetSelection(*Selected);
        }
        FReply Apply(bool bIncludeTarget, bool bReset = false)
        {
            if (IsLocked()) { Status = Text(TEXT("ui.quest_cheat.locked")); return FReply::Handled(); }
            const auto Current = TunaSweeperSaveTool::GetQuestCheatSelection();
            if (!Current->GetBoolField(TEXT("ok")) || !Selection || !Selection->GetBoolField(TEXT("ok")) ||
                Current->GetStringField(TEXT("savePath")) != Selection->GetStringField(TEXT("savePath")))
            { RefreshSelection(); Status = Text(TEXT("ui.quest_cheat.selection_changed")); return FReply::Handled(); }
            auto Request = MakeShared<FJsonObject>();
            Request->SetNumberField(TEXT("version"), 1);
            Request->SetStringField(TEXT("operation"), bReset ? TEXT("quest-reset") : TEXT("quest-complete"));
            Request->SetStringField(TEXT("savePath"), Current->GetStringField(TEXT("savePath")));
            Request->SetStringField(TEXT("flavor"), Current->GetStringField(TEXT("flavor")));
            Request->SetNumberField(TEXT("slot"), Current->GetIntegerField(TEXT("slot")));
            Request->SetNumberField(TEXT("questId"), QuestId);
            Request->SetBoolField(TEXT("includeTarget"), bIncludeTarget);
            auto Result = TunaSweeperSaveTool::Execute(Request);
            if (Result->GetBoolField(TEXT("ok")) && !IsLocked())
            {
                Request->SetStringField(TEXT("expectedHash"), Result->GetStringField(TEXT("sourceHash")));
                Request->SetBoolField(TEXT("commit"), true);
                Result = TunaSweeperSaveTool::Execute(Request);
            }
            if (Result->GetBoolField(TEXT("ok")) && Result->GetBoolField(TEXT("committed")))
                Status = bReset ? Text(TEXT("ui.quest_cheat.reset_success")) : FText::Format(Text(TEXT("ui.quest_cheat.success")), FText::AsNumber(Result->GetArrayField(TEXT("completedQuestIds")).Num()));
            else Status = FText::Format(Text(TEXT("ui.quest_cheat.error")), FText::FromString(Result->GetStringField(TEXT("code"))));
            RefreshQuests(true);
            return FReply::Handled();
        }
        int32 QuestId = 1;
        double NextRefresh = 0;
        TSharedPtr<FJsonObject> Selection;
        FText SelectionText, Status, ListStatus;
        FString ListedPath;
        FDateTime ListedTimestamp = FDateTime::MinValue();
        TArray<TSharedPtr<FJsonObject>> QuestRows;
        TSharedPtr<SListView<TSharedPtr<FJsonObject>>> QuestList;
    };
}

void FTunaSweeperQuestCheatTool::Startup()
{
    using namespace TunaSweeperQuestCheatEditor;
    StandaloneHandle = FEditorDelegates::BeginStandaloneLocalPlay.AddLambda([](uint32 Pid)
    { auto Process = FPlatformProcess::OpenProcess(Pid); if (Process.IsValid()) StandaloneProcesses.Add(Process); });
    FGlobalTabmanager::Get()->RegisterNomadTabSpawner(TabId, FOnSpawnTab::CreateRaw(this, &FTunaSweeperQuestCheatTool::Spawn))
        .SetDisplayName(Text(TEXT("ui.quest_cheat.title"))).SetMenuType(ETabSpawnerMenuType::Hidden);
    UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FTunaSweeperQuestCheatTool::RegisterMenus));
}
void FTunaSweeperQuestCheatTool::Shutdown()
{
    using namespace TunaSweeperQuestCheatEditor;
    FEditorDelegates::BeginStandaloneLocalPlay.Remove(StandaloneHandle);
    for (auto& Process : StandaloneProcesses) FPlatformProcess::CloseProc(Process);
    StandaloneProcesses.Reset();
    UToolMenus::UnRegisterStartupCallback(this);
    UToolMenus::UnregisterOwner(this);
    if (auto Tab = FGlobalTabmanager::Get()->FindExistingLiveTab(TabId)) Tab->RequestCloseTab();
    FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(TabId);
}
void FTunaSweeperQuestCheatTool::RegisterMenus()
{
    FToolMenuOwnerScoped Owner(this);
    auto* Menu = UToolMenus::Get()->ExtendMenu(TEXT("LevelEditor.MainMenu.TunaSweeper"));
    Menu->FindOrAddSection(TEXT("Debug")).AddMenuEntry(TEXT("QuestCheat"),
        TunaSweeperQuestCheatEditor::Text(TEXT("ui.quest_cheat.title")),
        TunaSweeperQuestCheatEditor::Text(TEXT("ui.quest_cheat.description")), FSlateIcon(),
        FUIAction(FExecuteAction::CreateRaw(this, &FTunaSweeperQuestCheatTool::Open)));
}
void FTunaSweeperQuestCheatTool::Open() { FGlobalTabmanager::Get()->TryInvokeTab(TunaSweeperQuestCheatEditor::TabId); }
TSharedRef<SDockTab> FTunaSweeperQuestCheatTool::Spawn(const FSpawnTabArgs& Args)
{ return SNew(SDockTab).TabRole(ETabRole::NomadTab)[ SNew(TunaSweeperQuestCheatEditor::SQuestCheatPanel) ]; }

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "PlayInEditorDataTypes.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperQuestCheatEditorLockTest, "TunaSweeper.QuestCheat.EditorLock",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTunaSweeperQuestCheatEditorLockTest::RunTest(const FString& Parameters)
{
    using namespace TunaSweeperQuestCheatEditor;
    if (!GEditor || IsLocked()) { AddError(TEXT("Requires an idle editor.")); return false; }
    TestFalse(TEXT("Editing enabled when stopped"), IsLocked());
    FRequestPlaySessionParams PlayRequest;
    PlayRequest.SessionDestination = EPlaySessionDestinationType::InProcess;
    GEditor->RequestPlaySession(PlayRequest);
    TestTrue(TEXT("UI locks as soon as play is queued"), IsLocked());
    GEditor->CancelRequestPlaySession();
    TestFalse(TEXT("UI unlocks after play request is cancelled"), IsLocked());
    const int32 PreviousCount = StandaloneProcesses.Num();
    FEditorDelegates::BeginStandaloneLocalPlay.Broadcast(FPlatformProcess::GetCurrentProcessId());
    TestEqual(TEXT("Standalone launch tracked"), StandaloneProcesses.Num(), PreviousCount + 1);
    TestTrue(TEXT("UI locks for live standalone process"), IsLocked());
    while (StandaloneProcesses.Num() > PreviousCount)
    { FPlatformProcess::CloseProc(StandaloneProcesses.Last()); StandaloneProcesses.Pop(); }
    TestFalse(TEXT("UI unlocks after standalone ends"), IsLocked());
    return true;
}
#endif
