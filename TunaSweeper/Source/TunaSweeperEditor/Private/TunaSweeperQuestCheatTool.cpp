#include "TunaSweeperQuestCheatTool.h"
#include "Dom/JsonObject.h"
#include "Editor.h"
#include "Engine/GameInstance.h"
#include "Framework/Docking/TabManager.h"
#include "HAL/PlatformProcess.h"
#include "Internationalization/Internationalization.h"
#include "Internationalization/Culture.h"
#include "Inventory/TunaSweeperSaveTool.h"
#include "Quest/TunaSweeperQuestId.h"
#include "Widgets/Input/NumericTypeInterface.h"
#include "Settings/TunaSweeperLanguage.h"
#include "Subsystem/TunaSweeperTextSubsystem.h"
#include "ToolMenus.h"
#include "UObject/StrongObjectPtr.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Text/STextBlock.h"

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

    TStrongObjectPtr<UGameInstance> TextOwner;
    TStrongObjectPtr<UTunaSweeperTextSubsystem> TextData;

    FText Text(const TCHAR* Key)
    {
        if (!TextData.IsValid())
        {
            TextOwner.Reset(NewObject<UGameInstance>());
            TextData.Reset(NewObject<UTunaSweeperTextSubsystem>(TextOwner.Get()));
        }
        ETunaSweeperItemTextLanguage Language = ETunaSweeperItemTextLanguage::English;
        TunaSweeperLanguage::TryParseLanguageCode(FInternationalization::Get().GetCurrentCulture()->GetTwoLetterISOLanguageName(), Language);
        return TextData->ResolveText(FName(Key), Language, FText::GetEmpty());
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
                            .OnValueChanged_Lambda([this](int32 Value) { QuestId = Value; }) ]
                        + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 6)
                        [ SNew(SButton).Text(Text(TEXT("ui.quest_cheat.complete")))
                            .OnClicked_Lambda([this] { return Apply(true); }) ]
                        + SVerticalBox::Slot().AutoHeight()
                        [ SNew(SButton).Text(Text(TEXT("ui.quest_cheat.prerequisites")))
                            .OnClicked_Lambda([this] { return Apply(false); }) ]
                    ]
                    + SVerticalBox::Slot().AutoHeight().Padding(0, 12, 0, 0)
                    [ SNew(STextBlock).Text_Lambda([this] { return Status; }).AutoWrapText(true) ]
                ]
            ];
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
        }
        FReply Apply(bool bIncludeTarget)
        {
            if (IsLocked()) { Status = Text(TEXT("ui.quest_cheat.locked")); return FReply::Handled(); }
            const auto Current = TunaSweeperSaveTool::GetQuestCheatSelection();
            if (!Current->GetBoolField(TEXT("ok")) || !Selection || !Selection->GetBoolField(TEXT("ok")) ||
                Current->GetStringField(TEXT("savePath")) != Selection->GetStringField(TEXT("savePath")))
            { RefreshSelection(); Status = Text(TEXT("ui.quest_cheat.selection_changed")); return FReply::Handled(); }
            auto Request = MakeShared<FJsonObject>();
            Request->SetNumberField(TEXT("version"), 1);
            Request->SetStringField(TEXT("operation"), TEXT("quest-complete"));
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
                Status = FText::Format(Text(TEXT("ui.quest_cheat.success")), FText::AsNumber(Result->GetArrayField(TEXT("completedQuestIds")).Num()));
            else Status = FText::Format(Text(TEXT("ui.quest_cheat.error")), FText::FromString(Result->GetStringField(TEXT("code"))));
            return FReply::Handled();
        }
        int32 QuestId = 1;
        double NextRefresh = 0;
        TSharedPtr<FJsonObject> Selection;
        FText SelectionText, Status;
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
    TextData.Reset();
    TextOwner.Reset();
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
