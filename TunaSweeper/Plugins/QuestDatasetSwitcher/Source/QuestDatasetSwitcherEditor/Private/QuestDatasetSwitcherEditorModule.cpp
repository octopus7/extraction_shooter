#include "QuestDatasetSwitcher.h"

#include "CookOnTheSide/CookOnTheFlyServer.h"
#include "Editor.h"
#include "EditorBuildUtils.h"
#include "Editor/UnrealEdEngine.h"
#include "Framework/Docking/TabManager.h"
#include "Framework/Notifications/NotificationManager.h"
#include "HAL/PlatformFileManager.h"
#include "HAL/PlatformProcess.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/FileHelper.h"
#include "Misc/HotReloadInterface.h"
#include "Misc/MessageDialog.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Styling/AppStyle.h"
#include "ToolMenus.h"
#include "UnrealEdGlobals.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#if PLATFORM_WINDOWS
#include "ILiveCodingModule.h"
#endif

#define LOCTEXT_NAMESPACE "QuestDatasetSwitcherEditorModule"

namespace QuestDatasetSwitcherEditor
{
	const FName TabName(TEXT("QuestDatasetSwitcher"));
	const TCHAR* PluginName = TEXT("QuestDatasetSwitcher");
	const TCHAR* GeneratedDataRelativePath = TEXT("Data/QuestDatasetGenerated");
	const TCHAR* ActiveDatasetFileName = TEXT("active-dataset.json");

	FString GetDatasetArgument(const EQuestDatasetKind Kind)
	{
		switch (Kind)
		{
		case EQuestDatasetKind::Production:
			return TEXT("Production");
		case EQuestDatasetKind::Public:
		default:
			return TEXT("Public");
		}
	}

	FText GetDatasetDisplayName(const EQuestDatasetKind Kind)
	{
		switch (Kind)
		{
		case EQuestDatasetKind::Production:
			return LOCTEXT("ProductionDataset", "Production");
		case EQuestDatasetKind::Public:
		default:
			return LOCTEXT("PublicDataset", "Public");
		}
	}

	bool TryReadMaterializedDataset(EQuestDatasetKind& OutKind)
	{
		const FString GeneratedDataDirectory = FPaths::Combine(
			FPaths::ProjectContentDir(),
			GeneratedDataRelativePath);
		const FString MarkerPath = FPaths::Combine(GeneratedDataDirectory, ActiveDatasetFileName);

		if (!FPaths::FileExists(MarkerPath))
		{
			OutKind = EQuestDatasetKind::Public;
			return true;
		}

		FString JsonText;
		TSharedPtr<FJsonObject> JsonObject;
		if (!FFileHelper::LoadFileToString(JsonText, *MarkerPath) ||
			!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(JsonText), JsonObject) ||
			!JsonObject.IsValid())
		{
			return false;
		}

		FString DatasetId;
		if (!JsonObject->TryGetStringField(TEXT("dataset_id"), DatasetId))
		{
			return false;
		}

		if (DatasetId == TEXT("production"))
		{
			OutKind = EQuestDatasetKind::Production;
			return true;
		}

		return false;
	}

	bool GetBlockingReason(FText& OutReason)
	{
		if (GEditor && GEditor->IsPlaySessionInProgress())
		{
			OutReason = LOCTEXT("PlayBlocked", "게임 실행(PIE/Simulate/Standalone) 중에는 적용할 수 없습니다.");
			return true;
		}

		if (GUnrealEd && GUnrealEd->CookServer &&
			(GUnrealEd->CookServer->IsCookByTheBookRunning() || GUnrealEd->CookServer->IsCookingInEditor()))
		{
			OutReason = LOCTEXT("CookBlocked", "Cook 또는 Package 작업 중에는 적용할 수 없습니다.");
			return true;
		}

		if (FEditorBuildUtils::IsBuildCurrentlyRunning())
		{
			OutReason = LOCTEXT("BuildBlocked", "에디터 Build 작업 중에는 적용할 수 없습니다.");
			return true;
		}

		if (const IHotReloadInterface* HotReload = IHotReloadInterface::GetPtr();
			HotReload && HotReload->IsCurrentlyCompiling())
		{
			OutReason = LOCTEXT("HotReloadBlocked", "C++ Hot Reload 빌드 중에는 적용할 수 없습니다.");
			return true;
		}

#if PLATFORM_WINDOWS
		if (const ILiveCodingModule* LiveCoding =
			FModuleManager::GetModulePtr<ILiveCodingModule>(LIVE_CODING_MODULE_NAME);
			LiveCoding && LiveCoding->IsCompiling())
		{
			OutReason = LOCTEXT("LiveCodingBlocked", "Live Coding 빌드 중에는 적용할 수 없습니다.");
			return true;
		}
#endif

		OutReason = FText::GetEmpty();
		return false;
	}
}

class SQuestDatasetSwitcherPanel final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SQuestDatasetSwitcherPanel)
	{
	}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		DatasetOptions = {
			MakeShared<EQuestDatasetKind>(EQuestDatasetKind::Public),
			MakeShared<EQuestDatasetKind>(EQuestDatasetKind::Production)
		};

		RuntimeDatasetKind = FQuestDatasetSwitcherModule::Get().GetActiveDataset().Kind;
		bDiskDatasetValid = QuestDatasetSwitcherEditor::TryReadMaterializedDataset(DiskDatasetKind);
		SelectedDataset = FindOption(bDiskDatasetValid ? DiskDatasetKind : RuntimeDatasetKind);

		ChildSlot
		[
			SNew(SBorder)
			.Padding(16.0f)
			.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
			[
				SNew(SScrollBox)
				+ SScrollBox::Slot()
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 0.0f, 0.0f, 12.0f)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("PanelTitle", "퀘스트 데이터셋 전환"))
						.Font(FAppStyle::GetFontStyle("HeadingExtraSmall"))
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 0.0f, 0.0f, 4.0f)
					[
						SNew(STextBlock)
						.Text(this, &SQuestDatasetSwitcherPanel::GetRuntimeDatasetText)
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 0.0f, 0.0f, 14.0f)
					[
						SNew(STextBlock)
						.Text(this, &SQuestDatasetSwitcherPanel::GetDiskDatasetText)
						.ColorAndOpacity(this, &SQuestDatasetSwitcherPanel::GetDiskDatasetColor)
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 0.0f, 0.0f, 8.0f)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("SelectDataset", "적용할 데이터셋"))
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 0.0f, 0.0f, 14.0f)
					[
						SAssignNew(DatasetComboBox, SComboBox<TSharedPtr<EQuestDatasetKind>>)
						.OptionsSource(&DatasetOptions)
						.InitiallySelectedItem(SelectedDataset)
						.OnGenerateWidget(this, &SQuestDatasetSwitcherPanel::GenerateDatasetOption)
						.OnSelectionChanged(this, &SQuestDatasetSwitcherPanel::OnDatasetSelected)
						[
							SNew(STextBlock)
							.Text(this, &SQuestDatasetSwitcherPanel::GetSelectedDatasetText)
						]
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Top)
						.Padding(0.0f, 0.0f, 12.0f, 0.0f)
						[
							SNew(SButton)
							.Text(LOCTEXT("ApplyDataset", "적용"))
							.ToolTipText(this, &SQuestDatasetSwitcherPanel::GetApplyToolTip)
							.IsEnabled(this, &SQuestDatasetSwitcherPanel::CanApply)
							.OnClicked(this, &SQuestDatasetSwitcherPanel::ApplyDataset)
						]
						+ SHorizontalBox::Slot()
						.FillWidth(1.0f)
						[
							SNew(STextBlock)
							.Text(this, &SQuestDatasetSwitcherPanel::GetWarningText)
							.AutoWrapText(true)
							.ColorAndOpacity(FLinearColor(1.0f, 0.55f, 0.1f))
						]
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 14.0f, 0.0f, 0.0f)
					[
						SNew(STextBlock)
						.Text(this, &SQuestDatasetSwitcherPanel::GetResultText)
						.AutoWrapText(true)
						.ColorAndOpacity(this, &SQuestDatasetSwitcherPanel::GetResultColor)
					]
				]
			]
		];
	}

private:
	TSharedPtr<EQuestDatasetKind> FindOption(const EQuestDatasetKind Kind) const
	{
		for (const TSharedPtr<EQuestDatasetKind>& Option : DatasetOptions)
		{
			if (Option.IsValid() && *Option == Kind)
			{
				return Option;
			}
		}
		return DatasetOptions[0];
	}

	TSharedRef<SWidget> GenerateDatasetOption(TSharedPtr<EQuestDatasetKind> Option) const
	{
		return SNew(STextBlock)
			.Text(Option.IsValid()
				? QuestDatasetSwitcherEditor::GetDatasetDisplayName(*Option)
				: FText::GetEmpty());
	}

	void OnDatasetSelected(TSharedPtr<EQuestDatasetKind> Option, ESelectInfo::Type SelectInfo)
	{
		SelectedDataset = MoveTemp(Option);
		ResultMessage = FText::GetEmpty();
		bLastApplySucceeded = false;
	}

	FText GetSelectedDatasetText() const
	{
		return SelectedDataset.IsValid()
			? QuestDatasetSwitcherEditor::GetDatasetDisplayName(*SelectedDataset)
			: LOCTEXT("NoDatasetSelected", "선택 안 됨");
	}

	FText GetRuntimeDatasetText() const
	{
		return FText::Format(
			LOCTEXT("RuntimeDatasetFormat", "현재 에디터 런타임: {0}"),
			QuestDatasetSwitcherEditor::GetDatasetDisplayName(RuntimeDatasetKind));
	}

	FText GetDiskDatasetText() const
	{
		if (!bDiskDatasetValid)
		{
			return LOCTEXT("InvalidDiskDataset", "디스크 적용 상태: 판독 실패 (공개 적용으로 복구 가능)");
		}

		const bool bRestartRequired = DiskDatasetKind != RuntimeDatasetKind;
		return FText::Format(
			bRestartRequired
				? LOCTEXT("DiskDatasetRestartFormat", "디스크 적용 상태: {0} — 에디터 재시작 필요")
				: LOCTEXT("DiskDatasetFormat", "디스크 적용 상태: {0}"),
			QuestDatasetSwitcherEditor::GetDatasetDisplayName(DiskDatasetKind));
	}

	FSlateColor GetDiskDatasetColor() const
	{
		return bDiskDatasetValid && DiskDatasetKind == RuntimeDatasetKind
			? FSlateColor::UseForeground()
			: FSlateColor(FLinearColor(1.0f, 0.55f, 0.1f));
	}

	bool CanApply() const
	{
		FText BlockingReason;
		const bool bCanRecoverInvalidState = SelectedDataset.IsValid() &&
			*SelectedDataset == EQuestDatasetKind::Public;
		return SelectedDataset.IsValid() && (bDiskDatasetValid || bCanRecoverInvalidState) &&
			!QuestDatasetSwitcherEditor::GetBlockingReason(BlockingReason);
	}

	FText GetApplyToolTip() const
	{
		if (!bDiskDatasetValid)
		{
			return SelectedDataset.IsValid() && *SelectedDataset == EQuestDatasetKind::Public
				? LOCTEXT("RecoverDiskTooltip", "공개 데이터셋을 적용해 손상된 디스크 전환 상태를 복구합니다.")
				: LOCTEXT("InvalidDiskTooltip", "현재 디스크 데이터셋 표식을 판독할 수 없습니다. 공개 데이터셋을 먼저 적용해 복구하세요.");
		}

		FText BlockingReason;
		return QuestDatasetSwitcherEditor::GetBlockingReason(BlockingReason)
			? BlockingReason
			: LOCTEXT("ApplyTooltip", "선택한 퀘스트 데이터셋을 디스크에 적용합니다.");
	}

	FText GetWarningText() const
	{
		FText BlockingReason;
		if (QuestDatasetSwitcherEditor::GetBlockingReason(BlockingReason))
		{
			return BlockingReason;
		}

		return LOCTEXT(
			"ApplyWarning",
			"주의: 게임 실행, Cook/Package, Build 중에는 전환할 수 없습니다. 데이터셋이 바뀌면 세이브 네임스페이스도 함께 바뀝니다. 적용 후 에디터를 반드시 재시작하세요.");
	}

	FText GetResultText() const
	{
		return ResultMessage;
	}

	FSlateColor GetResultColor() const
	{
		return bLastApplySucceeded
			? FSlateColor(FLinearColor(0.2f, 0.8f, 0.3f))
			: FSlateColor(FLinearColor(0.95f, 0.25f, 0.2f));
	}

	FReply ApplyDataset()
	{
		FText BlockingReason;
		if (!SelectedDataset.IsValid() ||
			QuestDatasetSwitcherEditor::GetBlockingReason(BlockingReason))
		{
			ResultMessage = BlockingReason.IsEmpty()
				? LOCTEXT("NoSelectionError", "적용할 데이터셋을 선택하세요.")
				: BlockingReason;
			bLastApplySucceeded = false;
			return FReply::Handled();
		}

		const FText Confirmation = FText::Format(
			LOCTEXT(
				"ApplyConfirmation",
				"{0} 데이터셋을 적용합니다.\n\n세이브 네임스페이스도 해당 데이터셋용으로 전환됩니다. 적용이 끝나면 작업을 계속하지 말고 Unreal Editor를 재시작하세요."),
			QuestDatasetSwitcherEditor::GetDatasetDisplayName(*SelectedDataset));
		if (FMessageDialog::Open(EAppMsgType::OkCancel, Confirmation) != EAppReturnType::Ok)
		{
			return FReply::Handled();
		}

		if (QuestDatasetSwitcherEditor::GetBlockingReason(BlockingReason))
		{
			SetApplyFailure(BlockingReason);
			return FReply::Handled();
		}

		const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(
			QuestDatasetSwitcherEditor::PluginName);
		if (!Plugin.IsValid())
		{
			SetApplyFailure(LOCTEXT("PluginMissing", "QuestDatasetSwitcher 플러그인 경로를 찾지 못했습니다."));
			return FReply::Handled();
		}

#if PLATFORM_WINDOWS
		const FString ScriptPath = FPaths::ConvertRelativePathToFull(FPaths::Combine(
			Plugin->GetBaseDir(),
			TEXT("Scripts/SwitchQuestDataset.ps1")));
		const FString SystemRoot = FPlatformMisc::GetEnvironmentVariable(TEXT("SystemRoot"));
		const FString PowerShellPath = FPaths::Combine(
			SystemRoot,
			TEXT("System32/WindowsPowerShell/v1.0/powershell.exe"));
		const FString Arguments = FString::Printf(
			TEXT("-NoProfile -NonInteractive -ExecutionPolicy Bypass -File \"%s\" -Dataset %s"),
			*ScriptPath,
			*QuestDatasetSwitcherEditor::GetDatasetArgument(*SelectedDataset));

		int32 ReturnCode = INDEX_NONE;
		FString StandardOutput;
		FString StandardError;
		const bool bProcessStarted = FPlatformProcess::ExecProcess(
			*PowerShellPath,
			*Arguments,
			&ReturnCode,
			&StandardOutput,
			&StandardError);

		if (!bProcessStarted || ReturnCode != 0)
		{
			const FString Details = !StandardError.TrimStartAndEnd().IsEmpty()
				? StandardError.TrimStartAndEnd()
				: StandardOutput.TrimStartAndEnd();
			SetApplyFailure(FText::Format(
				LOCTEXT("ApplyFailedFormat", "데이터셋 적용 실패 (코드 {0})\n{1}"),
				FText::AsNumber(ReturnCode),
				FText::FromString(Details)));
			return FReply::Handled();
		}

		DiskDatasetKind = *SelectedDataset;
		bDiskDatasetValid = true;
		bLastApplySucceeded = true;
		ResultMessage = DiskDatasetKind == RuntimeDatasetKind
			? LOCTEXT("ApplySucceededNoRestart", "적용 및 검증이 완료됐습니다. 현재 런타임과 같은 데이터셋입니다.")
			: LOCTEXT("ApplySucceededRestart", "적용 및 검증이 완료됐습니다. 변경 내용을 사용하려면 지금 Unreal Editor를 재시작하세요.");

		FNotificationInfo Notification(ResultMessage);
		Notification.ExpireDuration = 6.0f;
		Notification.bUseSuccessFailIcons = true;
		FSlateNotificationManager::Get().AddNotification(Notification)->SetCompletionState(
			SNotificationItem::CS_Success);
#else
		SetApplyFailure(LOCTEXT("UnsupportedPlatform", "현재 전환 도구는 Windows Editor에서만 지원됩니다."));
#endif

		return FReply::Handled();
	}

	void SetApplyFailure(const FText& Message)
	{
		ResultMessage = Message;
		bLastApplySucceeded = false;
		FNotificationInfo Notification(Message);
		Notification.ExpireDuration = 8.0f;
		Notification.bUseSuccessFailIcons = true;
		FSlateNotificationManager::Get().AddNotification(Notification)->SetCompletionState(
			SNotificationItem::CS_Fail);
	}

	TArray<TSharedPtr<EQuestDatasetKind>> DatasetOptions;
	TSharedPtr<EQuestDatasetKind> SelectedDataset;
	TSharedPtr<SComboBox<TSharedPtr<EQuestDatasetKind>>> DatasetComboBox;
	EQuestDatasetKind RuntimeDatasetKind = EQuestDatasetKind::Public;
	EQuestDatasetKind DiskDatasetKind = EQuestDatasetKind::Public;
	bool bDiskDatasetValid = true;
	bool bLastApplySucceeded = false;
	FText ResultMessage;
};

class FQuestDatasetSwitcherEditorModule final : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		if (IsRunningCommandlet())
		{
			return;
		}

		FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
			QuestDatasetSwitcherEditor::TabName,
			FOnSpawnTab::CreateRaw(this, &FQuestDatasetSwitcherEditorModule::SpawnSwitcherTab))
			.SetDisplayName(LOCTEXT("TabTitle", "Quest Dataset Switcher"))
			.SetTooltipText(LOCTEXT("TabTooltip", "Public/Production 퀘스트 데이터셋을 전환합니다."))
			.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Database"))
			.SetMenuType(ETabSpawnerMenuType::Hidden);

		UToolMenus::RegisterStartupCallback(
			FSimpleMulticastDelegate::FDelegate::CreateRaw(
				this,
				&FQuestDatasetSwitcherEditorModule::RegisterMenus));
	}

	virtual void ShutdownModule() override
	{
		if (IsRunningCommandlet())
		{
			return;
		}

		UToolMenus::UnRegisterStartupCallback(this);
		UToolMenus::UnregisterOwner(this);
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(QuestDatasetSwitcherEditor::TabName);
	}

private:
	TSharedRef<SDockTab> SpawnSwitcherTab(const FSpawnTabArgs& SpawnTabArgs)
	{
		return SNew(SDockTab)
			.TabRole(ETabRole::NomadTab)
			[
				SNew(SQuestDatasetSwitcherPanel)
			];
	}

	void RegisterMenus()
	{
		FToolMenuOwnerScoped OwnerScoped(this);

		UToolMenu* MainMenu = UToolMenus::Get()->ExtendMenu(TEXT("LevelEditor.MainMenu"));
		FToolMenuSection& MainSection = MainMenu->FindOrAddSection(NAME_None);
		if (!MainSection.FindEntry(TEXT("TunaSweeper")))
		{
			FToolMenuEntry& TunaSweeperEntry = MainSection.AddSubMenu(
				TEXT("TunaSweeper"),
				LOCTEXT("TunaSweeperTopMenu", "TunaSweeper"),
				LOCTEXT("TunaSweeperTopMenuTooltip", "Open TunaSweeper editor tools."),
				FNewToolMenuChoice());
			TunaSweeperEntry.InsertPosition = FToolMenuInsert(TEXT("Tools"), EToolMenuInsertType::After);
		}

		UToolMenu* TunaSweeperMenu = UToolMenus::Get()->RegisterMenu(
			TEXT("LevelEditor.MainMenu.TunaSweeper"),
			NAME_None,
			EMultiBoxType::Menu,
			false);
		FToolMenuSection& Section = TunaSweeperMenu->FindOrAddSection(
			TEXT("DataTools"),
			LOCTEXT("DataToolsSection", "Data Tools"));
		Section.AddMenuEntry(
			TEXT("OpenQuestDatasetSwitcher"),
			LOCTEXT("OpenToolLabel", "Quest Dataset Switcher"),
			LOCTEXT("OpenToolTooltip", "퀘스트 데이터셋과 해당 세이브 네임스페이스를 전환합니다."),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Database"),
			FUIAction(FExecuteAction::CreateRaw(this, &FQuestDatasetSwitcherEditorModule::OpenSwitcherTab)));
	}

	void OpenSwitcherTab()
	{
		FGlobalTabmanager::Get()->TryInvokeTab(QuestDatasetSwitcherEditor::TabName);
	}
};

IMPLEMENT_MODULE(FQuestDatasetSwitcherEditorModule, QuestDatasetSwitcherEditor)

#undef LOCTEXT_NAMESPACE
