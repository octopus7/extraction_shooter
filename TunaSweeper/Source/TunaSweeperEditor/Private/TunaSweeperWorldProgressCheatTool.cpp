#include "TunaSweeperWorldProgressCheatTool.h"
#include "TunaSweeperEditorText.h"
#include "Dom/JsonObject.h"
#include "Framework/Docking/TabManager.h"
#include "HAL/FileManager.h"
#include "Inventory/TunaSweeperSaveTool.h"
#include "ToolMenus.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/SListView.h"

// Share the quest tool's pending PIE / standalone / launcher lock and process tracking.
namespace TunaSweeperQuestCheatEditor { bool IsLocked(); }

namespace TunaSweeperWorldProgressCheatEditor
{
    const FName TabId(TEXT("TunaSweeperWorldProgressCheat"));
    bool IsLocked() { return TunaSweeperQuestCheatEditor::IsLocked(); }
    FText Text(const TCHAR* Key) { return TunaSweeperEditorText::Resolve(Key, ETunaSweeperItemTextLanguage::Korean); }

    struct FRow
    {
        TSharedPtr<FJsonObject> Data;
        int32 Quantity = 0;
    };

    class SWorldProgressPanel final : public SCompoundWidget
    {
    public:
        SLATE_BEGIN_ARGS(SWorldProgressPanel) {} SLATE_END_ARGS()
        void Construct(const FArguments&)
        {
            ChildSlot
            [ SNew(SBorder).Padding(16)
                [ SNew(SVerticalBox)
                    + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 8)
                    [ SNew(STextBlock).Text(Text(TEXT("ui.world_cheat.description"))).AutoWrapText(true) ]
                    + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 8)
                    [ SNew(STextBlock).Text_Lambda([this] { return SelectionText; }) ]
                    + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 8)
                    [ SNew(STextBlock).Text(Text(TEXT("ui.world_cheat.notes"))).AutoWrapText(true) ]
                    + SVerticalBox::Slot().AutoHeight()
                    [ SNew(STextBlock).Text(Text(TEXT("ui.quest_cheat.locked")))
                        .Visibility_Lambda([] { return IsLocked() ? EVisibility::Visible : EVisibility::Collapsed; }) ]
                    + SVerticalBox::Slot().AutoHeight().Padding(0, 8)
                    [ SNew(SButton).Text(Text(TEXT("ui.quest_cheat.refresh"))).IsEnabled_Lambda([] { return !IsLocked(); })
                        .OnClicked_Lambda([this] { Refresh(true); return FReply::Handled(); }) ]
                    + SVerticalBox::Slot().FillHeight(1)
                    [ SAssignNew(List, SListView<TSharedPtr<FRow>>).ListItemsSource(&Rows).SelectionMode(ESelectionMode::None)
                        .IsEnabled_Lambda([] { return !IsLocked(); })
                        .OnGenerateRow(this, &SWorldProgressPanel::GenerateRow) ]
                    + SVerticalBox::Slot().AutoHeight().Padding(0, 8, 0, 0)
                    [ SNew(STextBlock).Text_Lambda([this] { return Status; }).AutoWrapText(true) ]
                ]
            ];
            Refresh(true);
        }
        virtual void Tick(const FGeometry& Geometry, double Time, float Delta) override
        {
            SCompoundWidget::Tick(Geometry, Time, Delta);
            if (Time >= NextRefresh && !IsLocked()) { Refresh(false); NextRefresh = Time + 1.0; }
        }
    private:
        TSharedRef<ITableRow> GenerateRow(TSharedPtr<FRow> Row, const TSharedRef<STableViewBase>& Owner)
        {
            const auto Data = Row->Data;
            const TCHAR* StateKey = !Data->GetBoolField(TEXT("recorded")) ? TEXT("ui.world_cheat.state.unrecorded") :
                Data->GetStringField(TEXT("state")) == TEXT("Completed") ? TEXT("ui.world_cheat.state.completed") : TEXT("ui.world_cheat.state.in_progress");
            return SNew(STableRow<TSharedPtr<FRow>>, Owner).Padding(8)
                [ SNew(SVerticalBox)
                    + SVerticalBox::Slot().AutoHeight()
                    [ SNew(STextBlock).Text(FText::Format(Text(TEXT("ui.world_cheat.row")),
                        Text(*Data->GetStringField(TEXT("nameKey"))), Text(StateKey), FText::AsNumber(Data->GetIntegerField(TEXT("quantity"))))) ]
                    + SVerticalBox::Slot().AutoHeight().Padding(0, 2, 0, 6)
                    [ SNew(STextBlock).Text(FText::FromString(Data->GetStringField(TEXT("objectId")))) ]
                    + SVerticalBox::Slot().AutoHeight()
                    [ SNew(SHorizontalBox)
                        + SHorizontalBox::Slot().AutoWidth()
                        [ SNew(SButton).Text(Text(TEXT("ui.world_cheat.reset")))
                            .OnClicked_Lambda([this, Row] { return Apply(Row, TEXT("InProgress"), 0); }) ]
                        + SHorizontalBox::Slot().AutoWidth().Padding(6, 0)
                        [ SNew(STextBlock).Text(Text(TEXT("ui.world_cheat.quantity"))) ]
                        + SHorizontalBox::Slot().AutoWidth()
                        [ SNew(SNumericEntryBox<int32>).MinDesiredValueWidth(70).MinValue(0).MaxValue(Data->GetIntegerField(TEXT("maxQuantity"))).AllowSpin(false)
                            .Value_Lambda([Row]() -> TOptional<int32> { return Row->Quantity; })
                            .OnValueChanged_Lambda([Row](int32 Value) { Row->Quantity = Value; }) ]
                        + SHorizontalBox::Slot().AutoWidth().Padding(6, 0)
                        [ SNew(SButton).Text(Text(TEXT("ui.world_cheat.in_progress")))
                            .OnClicked_Lambda([this, Row] { return Apply(Row, TEXT("InProgress"), Row->Quantity); }) ]
                        + SHorizontalBox::Slot().AutoWidth()
                        [ SNew(SButton).Text(Text(TEXT("ui.world_cheat.complete")))
                            .OnClicked_Lambda([this, Row] { return Apply(Row, TEXT("Completed"), FMath::Max(Row->Quantity, Row->Data->GetIntegerField(TEXT("requiredQuantity")))); }) ]
                    ]
                ];
        }
        TSharedPtr<FJsonObject> Request(const TCHAR* Op) const
        {
            auto R = MakeShared<FJsonObject>();
            R->SetNumberField(TEXT("version"), 1); R->SetStringField(TEXT("operation"), Op);
            R->SetStringField(TEXT("savePath"), Selection->GetStringField(TEXT("savePath")));
            R->SetStringField(TEXT("flavor"), Selection->GetStringField(TEXT("flavor")));
            R->SetNumberField(TEXT("slot"), Selection->GetIntegerField(TEXT("slot")));
            return R;
        }
        void Refresh(bool bForce)
        {
            if (IsLocked()) return;
            auto Current = TunaSweeperSaveTool::GetQuestCheatSelection();
            const FString Path = Current->GetBoolField(TEXT("ok")) ? Current->GetStringField(TEXT("savePath")) : FString();
            const auto Timestamp = Path.IsEmpty() ? FDateTime::MinValue() : IFileManager::Get().GetTimeStamp(*Path);
            if (!bForce && Selection && Path == ListedPath && Timestamp == ListedTimestamp) return;
            Selection = Current; ListedPath = Path; ListedTimestamp = Timestamp; SourceHash.Empty(); Rows.Reset();
            SelectionText = Path.IsEmpty() ? Text(TEXT("ui.quest_cheat.selection_invalid")) :
                FText::Format(Text(TEXT("ui.quest_cheat.slot")),
                    Text(Current->GetStringField(TEXT("flavor")) == TEXT("Main") ? TEXT("ui.quest_cheat.flavor.main") : TEXT("ui.quest_cheat.flavor.demo")),
                    FText::AsNumber(Current->GetIntegerField(TEXT("slot"))));
            if (!Path.IsEmpty())
            {
                const auto Result = TunaSweeperSaveTool::Execute(Request(TEXT("world-inspect")));
                if (Result->GetBoolField(TEXT("ok")))
                {
                    SourceHash = Result->GetStringField(TEXT("sourceHash"));
                    for (const auto& Value : Result->GetArrayField(TEXT("worldProgress")))
                    {
                        auto Row = MakeShared<FRow>(); Row->Data = Value->AsObject();
                        Row->Quantity = Row->Data->GetIntegerField(TEXT("quantity")); Rows.Add(Row);
                    }
                }
                else Status = FText::Format(Text(TEXT("ui.quest_cheat.error")), FText::FromString(Result->GetStringField(TEXT("code"))));
            }
            if (List) List->RequestListRefresh();
        }
        FReply Apply(TSharedPtr<FRow> Row, const TCHAR* State, int32 Quantity)
        {
            if (IsLocked()) { Status = Text(TEXT("ui.quest_cheat.locked")); return FReply::Handled(); }
            const auto Current = TunaSweeperSaveTool::GetQuestCheatSelection();
            if (!Selection || !Current->GetBoolField(TEXT("ok")) || SourceHash.IsEmpty() || !Rows.Contains(Row) ||
                Current->GetStringField(TEXT("savePath")) != ListedPath)
            { Refresh(true); Status = Text(TEXT("ui.quest_cheat.selection_changed")); return FReply::Handled(); }
            auto Edit = Request(TEXT("world-set"));
            Edit->SetStringField(TEXT("objectId"), Row->Data->GetStringField(TEXT("objectId")));
            Edit->SetStringField(TEXT("state"), State); Edit->SetNumberField(TEXT("quantity"), Quantity);
            Edit->SetStringField(TEXT("expectedHash"), SourceHash); Edit->SetBoolField(TEXT("commit"), true);
            const auto Result = TunaSweeperSaveTool::Execute(Edit);
            const bool bSuccess = Result->GetBoolField(TEXT("ok")) && Result->GetBoolField(TEXT("committed"));
            Refresh(true);
            Status = bSuccess ? Text(TEXT("ui.world_cheat.success")) :
                FText::Format(Text(TEXT("ui.quest_cheat.error")), FText::FromString(Result->GetStringField(TEXT("code"))));
            return FReply::Handled();
        }
        double NextRefresh = 0;
        TSharedPtr<FJsonObject> Selection;
        FString ListedPath, SourceHash;
        FDateTime ListedTimestamp = FDateTime::MinValue();
        FText SelectionText, Status;
        TArray<TSharedPtr<FRow>> Rows;
        TSharedPtr<SListView<TSharedPtr<FRow>>> List;
    };
}

void FTunaSweeperWorldProgressCheatTool::Startup()
{
    using namespace TunaSweeperWorldProgressCheatEditor;
    FGlobalTabmanager::Get()->RegisterNomadTabSpawner(TabId, FOnSpawnTab::CreateRaw(this, &FTunaSweeperWorldProgressCheatTool::Spawn))
        .SetDisplayName(Text(TEXT("ui.world_cheat.title"))).SetMenuType(ETabSpawnerMenuType::Hidden);
    UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FTunaSweeperWorldProgressCheatTool::RegisterMenus));
}
void FTunaSweeperWorldProgressCheatTool::Shutdown()
{
    using namespace TunaSweeperWorldProgressCheatEditor;
    UToolMenus::UnRegisterStartupCallback(this); UToolMenus::UnregisterOwner(this);
    if (auto Tab = FGlobalTabmanager::Get()->FindExistingLiveTab(TabId)) Tab->RequestCloseTab();
    FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(TabId);
}
void FTunaSweeperWorldProgressCheatTool::RegisterMenus()
{
    using namespace TunaSweeperWorldProgressCheatEditor;
    FToolMenuOwnerScoped Owner(this);
    UToolMenus::Get()->ExtendMenu(TEXT("LevelEditor.MainMenu.TunaSweeper"))->FindOrAddSection(TEXT("Debug")).AddMenuEntry(TEXT("WorldProgressCheat"),
        Text(TEXT("ui.world_cheat.title")), Text(TEXT("ui.world_cheat.description")), FSlateIcon(),
        FUIAction(FExecuteAction::CreateRaw(this, &FTunaSweeperWorldProgressCheatTool::Open)));
}
void FTunaSweeperWorldProgressCheatTool::Open() { FGlobalTabmanager::Get()->TryInvokeTab(TunaSweeperWorldProgressCheatEditor::TabId); }
TSharedRef<SDockTab> FTunaSweeperWorldProgressCheatTool::Spawn(const FSpawnTabArgs& Args)
{ return SNew(SDockTab).TabRole(ETabRole::NomadTab)[ SNew(TunaSweeperWorldProgressCheatEditor::SWorldProgressPanel) ]; }
