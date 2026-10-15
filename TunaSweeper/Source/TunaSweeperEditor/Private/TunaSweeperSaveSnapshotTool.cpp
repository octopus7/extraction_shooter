#include "TunaSweeperSaveSnapshotTool.h"
#include "TunaSweeperEditorText.h"
#include "Dom/JsonObject.h"
#include "Editor.h"
#include "Framework/Docking/TabManager.h"
#include "HAL/PlatformProcess.h"
#include "Inventory/TunaSweeperSaveTool.h"
#include "Misc/MessageDialog.h"
#include "ToolMenus.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/SListView.h"

namespace TunaSweeperSaveSnapshotEditor
{
    const FName TabId(TEXT("TunaSweeperSaveSnapshots"));
    TArray<FProcHandle> StandaloneProcesses;
    FText Text(const TCHAR* Key)
    {
        return FText::FromString(TunaSweeperEditorText::Resolve(Key, ETunaSweeperItemTextLanguage::Korean)
            .ToString().Replace(TEXT("\\n"), TEXT("\n")));
    }
    FText SnapshotNote(const TSharedPtr<FJsonObject>& Row)
    {
        FString Note; Row->TryGetStringField(TEXT("note"), Note);
        Note.ReplaceInline(TEXT("\r"), TEXT(" "));
        Note.ReplaceInline(TEXT("\n"), TEXT(" "));
        return Note.IsEmpty() ? Text(TEXT("ui.save_snapshot.note_empty")) : FText::FromString(Note);
    }
    FText SnapshotTitle(const TSharedPtr<FJsonObject>& Row)
    {
        return FText::Format(Text(TEXT("ui.save_snapshot.row")), FText::FromString(Row->GetStringField(TEXT("createdAt"))),
            SnapshotNote(Row));
    }
    bool IsLocked()
    {
        bool bStandalone = false;
        for (int32 I = StandaloneProcesses.Num() - 1; I >= 0; --I)
        {
            if (FPlatformProcess::IsProcRunning(StandaloneProcesses[I])) bStandalone = true;
            else { FPlatformProcess::CloseProc(StandaloneProcesses[I]); StandaloneProcesses.RemoveAt(I); }
        }
        return !GEditor || GEditor->IsPlaySessionInProgress() || GEditor->PlayWorld ||
            GEditor->IsPlayingViaLauncher() || GEditor->IsPlayingOnLocalPCSession() || bStandalone;
    }

    class SSnapshotPanel : public SCompoundWidget
    {
    public:
        SLATE_BEGIN_ARGS(SSnapshotPanel) {} SLATE_END_ARGS()
        void Construct(const FArguments& Args)
        {
            ChildSlot
            [ SNew(SBorder).Padding(16)
                [ SNew(SVerticalBox)
                    + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 10)
                    [ SNew(STextBlock).Text_Lambda([this] { return SelectionText; }) ]
                    + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 10)
                    [ SNew(STextBlock).Text(Text(TEXT("ui.save_snapshot.help"))).AutoWrapText(true) ]
                    + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 10)
                    [ SNew(STextBlock).Text(Text(TEXT("ui.save_snapshot.locked")))
                        .Visibility_Lambda([] { return IsLocked() ? EVisibility::Visible : EVisibility::Collapsed; }) ]
                    + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 10)
                    [ SAssignNew(NoteInput, SEditableTextBox)
                        .HintText(Text(TEXT("ui.save_snapshot.note_hint")))
                        .IsEnabled_Lambda([this] { return Ready(); })
                        .OnVerifyTextChanged_Lambda([](const FText& Value, FText& OutError)
                        {
                            if (Value.ToString().Len() <= 2048) return true;
                            OutError = Text(TEXT("ui.save_snapshot.note_too_long")); return false;
                        }) ]
                    + SVerticalBox::Slot().AutoHeight()
                    [ SNew(SHorizontalBox)
                        + SHorizontalBox::Slot().AutoWidth()
                        [ SNew(SButton).Text(Text(TEXT("ui.save_snapshot.create")))
                            .IsEnabled_Lambda([this] { return CanCreate(); })
                            .OnClicked_Lambda([this] { return Create(); }) ]
                        + SHorizontalBox::Slot().AutoWidth().Padding(8, 0)
                        [ SNew(SButton).Text(Text(TEXT("ui.save_snapshot.restore")))
                            .IsEnabled_Lambda([this] { return Ready() && Selected.IsValid(); })
                            .OnClicked_Lambda([this] { return Restore(); }) ]
                        + SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 8, 0)
                        [ SNew(SButton).Text(Text(TEXT("ui.save_snapshot.delete")))
                            .IsEnabled_Lambda([this] { return Ready() && Selected.IsValid(); })
                            .OnClicked_Lambda([this] { return Delete(); }) ]
                        + SHorizontalBox::Slot().AutoWidth()
                        [ SNew(SButton).Text(Text(TEXT("ui.quest_cheat.refresh")))
                            .IsEnabled_Lambda([] { return !IsLocked(); })
                            .OnClicked_Lambda([this] { Refresh(true); return FReply::Handled(); }) ] ]
                    + SVerticalBox::Slot().FillHeight(1).Padding(0, 12, 0, 12)
                    [ SAssignNew(List, SListView<TSharedPtr<FJsonObject>>).ListItemsSource(&Rows)
                        .SelectionMode(ESelectionMode::Single)
                        .IsEnabled_Lambda([] { return !IsLocked(); })
                        .OnGenerateRow_Lambda([](TSharedPtr<FJsonObject> Row, const TSharedRef<STableViewBase>& Owner)
                        {
                            return SNew(STableRow<TSharedPtr<FJsonObject>>, Owner).Padding(8)
                                [ SNew(STextBlock).Text(SnapshotTitle(Row)).ToolTipText(SnapshotTitle(Row)) ];
                        })
                        .OnSelectionChanged_Lambda([this](TSharedPtr<FJsonObject> Row, ESelectInfo::Type) { Selected = Row; }) ]
                    + SVerticalBox::Slot().AutoHeight()
                    [ SNew(STextBlock).Text_Lambda([this] { return Status; }).AutoWrapText(true) ]
                ]
            ];
            Refresh(true);
        }
        virtual void Tick(const FGeometry& Geometry, double Time, float Delta) override
        {
            SCompoundWidget::Tick(Geometry, Time, Delta);
            if (!IsLocked() && Time >= NextRefresh) { Refresh(false); NextRefresh = Time + 1; }
        }
    private:
        bool Ready() const { return !IsLocked() && Selection && Selection->GetBoolField(TEXT("ok")); }
        bool CanCreate() const
        {
            if (!Ready() || !NoteInput.IsValid()) return false;
            const FString Note = NoteInput->GetText().ToString();
            return Note.Len() <= 2048 && !Note.TrimStartAndEnd().IsEmpty();
        }
        TSharedPtr<FJsonObject> Request(const TCHAR* Operation) const
        {
            auto R = MakeShared<FJsonObject>();
            R->SetNumberField(TEXT("version"), 1); R->SetStringField(TEXT("operation"), Operation);
            R->SetStringField(TEXT("savePath"), Selection->GetStringField(TEXT("savePath")));
            R->SetStringField(TEXT("flavor"), Selection->GetStringField(TEXT("flavor")));
            R->SetNumberField(TEXT("slot"), Selection->GetIntegerField(TEXT("slot")));
            return R;
        }
        void Error(const TSharedPtr<FJsonObject>& Result)
        {
            Status = FText::Format(Text(TEXT("ui.save_snapshot.error")), FText::FromString(Result->GetStringField(TEXT("code"))));
        }
        void Refresh(bool bForce)
        {
            if (IsLocked()) return;
            Selection = TunaSweeperSaveTool::GetQuestCheatSelection();
            const FString Path = Selection->GetBoolField(TEXT("ok")) ? Selection->GetStringField(TEXT("savePath")) : FString();
            SelectionText = Path.IsEmpty() ? Text(TEXT("ui.quest_cheat.selection_invalid")) :
                FText::Format(Text(TEXT("ui.quest_cheat.slot")),
                    Text(Selection->GetStringField(TEXT("flavor")) == TEXT("Demo") ? TEXT("ui.quest_cheat.flavor.demo") : TEXT("ui.quest_cheat.flavor.main")),
                    FText::AsNumber(Selection->GetIntegerField(TEXT("slot"))));
            if (!bForce && Path == ListedPath) return;
            ListedPath = Path; Selected.Reset(); Rows.Reset();
            if (!Path.IsEmpty())
            {
                auto Result = TunaSweeperSaveTool::Execute(Request(TEXT("snapshot-list")));
                if (Result->GetBoolField(TEXT("ok")))
                {
                    for (const auto& Row : Result->GetArrayField(TEXT("snapshots"))) Rows.Add(Row->AsObject());
                    Status = Rows.IsEmpty() ? Text(TEXT("ui.save_snapshot.empty")) : FText::GetEmpty();
                }
                else Error(Result);
            }
            List->ClearSelection(); List->RequestListRefresh();
        }
        FReply Create()
        {
            Refresh(false);
            if (!CanCreate()) return FReply::Handled();
            auto CreateRequest = Request(TEXT("snapshot-create"));
            CreateRequest->SetStringField(TEXT("note"), NoteInput->GetText().ToString());
            auto R = TunaSweeperSaveTool::Execute(CreateRequest);
            if (R->GetBoolField(TEXT("ok"))) { NoteInput->SetText(FText::GetEmpty()); Refresh(true); Status = Text(TEXT("ui.save_snapshot.created")); }
            else Error(R);
            return FReply::Handled();
        }
        FReply Delete()
        {
            Refresh(false);
            if (!Ready() || !Selected) return FReply::Handled();
            const FString Path = ListedPath;
            const FString Id = Selected->GetStringField(TEXT("snapshotId"));
            if (FMessageDialog::Open(EAppMsgType::YesNo, FText::Format(Text(TEXT("ui.save_snapshot.delete_confirm")),
                FText::FromString(Selected->GetStringField(TEXT("createdAt"))), SnapshotNote(Selected))) != EAppReturnType::Yes) return FReply::Handled();
            Refresh(false);
            if (!Ready() || Path != ListedPath) { Status = Text(TEXT("ui.save_snapshot.selection_changed")); return FReply::Handled(); }
            auto R = Request(TEXT("snapshot-delete")); R->SetStringField(TEXT("snapshotId"), Id);
            auto Result = TunaSweeperSaveTool::Execute(R);
            Refresh(true);
            if (Result->GetBoolField(TEXT("ok"))) Status = Text(TEXT("ui.save_snapshot.deleted"));
            else Error(Result);
            return FReply::Handled();
        }
        FReply Restore()
        {
            Refresh(false);
            if (!Ready() || !Selected) return FReply::Handled();
            const FString Path = ListedPath;
            const FString Id = Selected->GetStringField(TEXT("snapshotId"));
            if (FMessageDialog::Open(EAppMsgType::YesNo, FText::Format(Text(TEXT("ui.save_snapshot.confirm")),
                SelectionText, FText::FromString(Selected->GetStringField(TEXT("createdAt"))))) != EAppReturnType::Yes) return FReply::Handled();
            Refresh(false);
            if (!Ready() || Path != ListedPath) { Status = Text(TEXT("ui.save_snapshot.selection_changed")); return FReply::Handled(); }
            auto R = Request(TEXT("snapshot-restore")); R->SetStringField(TEXT("snapshotId"), Id);
            auto Result = TunaSweeperSaveTool::Execute(R);
            if (Result->GetBoolField(TEXT("ok"))) { Refresh(true); Status = Text(TEXT("ui.save_snapshot.restored")); }
            else Error(Result);
            return FReply::Handled();
        }
        double NextRefresh = 0;
        FString ListedPath;
        FText SelectionText, Status;
        TSharedPtr<FJsonObject> Selection, Selected;
        TArray<TSharedPtr<FJsonObject>> Rows;
        TSharedPtr<SListView<TSharedPtr<FJsonObject>>> List;
        TSharedPtr<SEditableTextBox> NoteInput;
    };
}

void FTunaSweeperSaveSnapshotTool::Startup()
{
    using namespace TunaSweeperSaveSnapshotEditor;
    StandaloneHandle = FEditorDelegates::BeginStandaloneLocalPlay.AddLambda([](uint32 Pid)
    { auto Process = FPlatformProcess::OpenProcess(Pid); if (Process.IsValid()) StandaloneProcesses.Add(Process); });
    FGlobalTabmanager::Get()->RegisterNomadTabSpawner(TabId, FOnSpawnTab::CreateRaw(this, &FTunaSweeperSaveSnapshotTool::Spawn))
        .SetDisplayName(Text(TEXT("ui.save_snapshot.title"))).SetMenuType(ETabSpawnerMenuType::Hidden);
    UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FTunaSweeperSaveSnapshotTool::RegisterMenus));
}
void FTunaSweeperSaveSnapshotTool::Shutdown()
{
    using namespace TunaSweeperSaveSnapshotEditor;
    FEditorDelegates::BeginStandaloneLocalPlay.Remove(StandaloneHandle);
    for (auto& Process : StandaloneProcesses) FPlatformProcess::CloseProc(Process);
    StandaloneProcesses.Reset();
    UToolMenus::UnRegisterStartupCallback(this); UToolMenus::UnregisterOwner(this);
    if (auto Tab = FGlobalTabmanager::Get()->FindExistingLiveTab(TabId)) Tab->RequestCloseTab();
    FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(TabId);
}
void FTunaSweeperSaveSnapshotTool::RegisterMenus()
{
    FToolMenuOwnerScoped Owner(this);
    auto* Menu = UToolMenus::Get()->ExtendMenu(TEXT("LevelEditor.MainMenu.TunaSweeper"));
    Menu->FindOrAddSection(TEXT("Debug")).AddMenuEntry(TEXT("SaveSnapshots"),
        TunaSweeperSaveSnapshotEditor::Text(TEXT("ui.save_snapshot.title")),
        TunaSweeperSaveSnapshotEditor::Text(TEXT("ui.save_snapshot.help")), FSlateIcon(),
        FUIAction(FExecuteAction::CreateRaw(this, &FTunaSweeperSaveSnapshotTool::Open)));
}
void FTunaSweeperSaveSnapshotTool::Open() { FGlobalTabmanager::Get()->TryInvokeTab(TunaSweeperSaveSnapshotEditor::TabId); }
TSharedRef<SDockTab> FTunaSweeperSaveSnapshotTool::Spawn(const FSpawnTabArgs& Args)
{ return SNew(SDockTab).TabRole(ETabRole::NomadTab)[ SNew(TunaSweeperSaveSnapshotEditor::SSnapshotPanel) ]; }

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "PlayInEditorDataTypes.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperSaveSnapshotEditorTest, "TunaSweeper.SaveTool.SnapshotEditor",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTunaSweeperSaveSnapshotEditorTest::RunTest(const FString& Parameters)
{
    using namespace TunaSweeperSaveSnapshotEditor;
    if (!GEditor || IsLocked()) { AddError(TEXT("Requires an idle editor.")); return false; }
    TestTrue(TEXT("Delete confirmation renders CSV line breaks"), Text(TEXT("ui.save_snapshot.delete_confirm")).ToString().Contains(TEXT("\n")));
    auto MemoRow = MakeShared<FJsonObject>();
    MemoRow->SetStringField(TEXT("note"), TEXT("한글 메모 — 낙석 개방 전"));
    TestTrue(TEXT("List text includes the full Korean memo"), SnapshotNote(MemoRow).ToString().Contains(TEXT("한글 메모 — 낙석 개방 전")));
    MemoRow->RemoveField(TEXT("note"));
    TestTrue(TEXT("Older rows visibly identify missing memos"), SnapshotNote(MemoRow).ToString().Contains(Text(TEXT("ui.save_snapshot.note_empty")).ToString()));
    auto Tab = FGlobalTabmanager::Get()->TryInvokeTab(TabId);
    TestTrue(TEXT("Snapshot editor tab can be opened without mutating saves"), Tab.IsValid());
    if (Tab) Tab->RequestCloseTab();
    FRequestPlaySessionParams Request;
    Request.SessionDestination = EPlaySessionDestinationType::InProcess;
    GEditor->RequestPlaySession(Request);
    TestTrue(TEXT("Snapshot actions lock while play is queued"), IsLocked());
    GEditor->CancelRequestPlaySession();
    TestFalse(TEXT("Snapshot actions unlock when play is cancelled"), IsLocked());
    const int32 Before = StandaloneProcesses.Num();
    FEditorDelegates::BeginStandaloneLocalPlay.Broadcast(FPlatformProcess::GetCurrentProcessId());
    TestTrue(TEXT("Snapshot actions lock while standalone is running"), IsLocked());
    while (StandaloneProcesses.Num() > Before)
    { FPlatformProcess::CloseProc(StandaloneProcesses.Last()); StandaloneProcesses.Pop(); }
    return true;
}
#endif
