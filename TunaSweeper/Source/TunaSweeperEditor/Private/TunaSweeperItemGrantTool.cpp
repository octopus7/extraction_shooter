#include "TunaSweeperItemGrantTool.h"
#include "TunaSweeperEditorText.h"
#include "TunaSweeperQuestCheatTool.h"
#include "Dom/JsonObject.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Docking/TabManager.h"
#include "Inventory/TunaSweeperSaveTool.h"
#include "ToolMenus.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SWindow.h"
#include "Widgets/Text/STextBlock.h"

namespace TunaSweeperItemGrantEditor
{
	const FName TabId(TEXT("TunaSweeperItemGrant"));
	FText Text(const TCHAR* Key) { return TunaSweeperEditorText::Resolve(Key, ETunaSweeperItemTextLanguage::Korean); }
	bool IsFeaturedItem(int32 Id)
	{
		return (Id >= 6001 && Id <= 6035) || (Id >= 7001 && Id <= 7004) ||
			Id == 3002 || Id == 3004 || Id == 3005 || Id == 3006;
	}

	// Editor Slate state uses existing Slate controls; no runtime widget class or widget tree is created.
	struct FPanelState : TSharedFromThis<FPanelState>
	{
		int32 ItemId = 6019;
		int32 Quantity = 1;
		FText Status;
		TSharedPtr<FJsonObject> Selection;
		TArray<TSharedPtr<FJsonObject>> Catalog;
		TWeakPtr<SWindow> PickerWindow;

		void Refresh()
		{
			Selection = TunaSweeperSaveTool::GetQuestCheatSelection();
			auto Request = MakeShared<FJsonObject>();
			Request->SetNumberField(TEXT("version"), 1);
			Request->SetStringField(TEXT("operation"), TEXT("catalog"));
			const auto Result = TunaSweeperSaveTool::Execute(Request);
			Catalog.Reset();
			if (Result->GetBoolField(TEXT("ok")))
				for (const auto& Value : Result->GetArrayField(TEXT("items"))) Catalog.Add(Value->AsObject());
			else Status = Text(TEXT("ui.item_grant.catalog_error"));
		}
		FText ItemName(int32 Id) const
		{
			const auto* Row = Catalog.FindByPredicate([Id](const auto& Item) { return Item->GetIntegerField(TEXT("itemId")) == Id; });
			return Row ? Text(*(*Row)->GetStringField(TEXT("nameStringKey"))) : Text(TEXT("ui.item_grant.unknown"));
		}
		FText SelectionText() const
		{
			if (!Selection || !Selection->GetBoolField(TEXT("ok"))) return Text(TEXT("ui.quest_cheat.selection_invalid"));
			return FText::Format(Text(TEXT("ui.quest_cheat.slot")),
				Text(Selection->GetStringField(TEXT("flavor")) == TEXT("Demo") ? TEXT("ui.quest_cheat.flavor.demo") : TEXT("ui.quest_cheat.flavor.main")),
				FText::AsNumber(Selection->GetIntegerField(TEXT("slot"))));
		}
		bool CanGrant() const
		{
			return !FTunaSweeperQuestCheatTool::IsEditingLocked() && Selection && Selection->GetBoolField(TEXT("ok")) &&
				ItemId > 0 && Quantity > 0 && Catalog.ContainsByPredicate([this](const auto& Row)
				{ return Row->GetIntegerField(TEXT("itemId")) == ItemId && Row->GetBoolField(TEXT("canAdd")); });
		}
		FReply Grant()
		{
			if (!CanGrant()) return FReply::Handled();
			const auto Current = TunaSweeperSaveTool::GetQuestCheatSelection();
			if (!Current->GetBoolField(TEXT("ok")) || Current->GetStringField(TEXT("savePath")) != Selection->GetStringField(TEXT("savePath")))
			{
				Selection = Current; Status = Text(TEXT("ui.quest_cheat.selection_changed")); return FReply::Handled();
			}
			auto Request = MakeShared<FJsonObject>();
			Request->SetNumberField(TEXT("version"), 1);
			Request->SetStringField(TEXT("operation"), TEXT("add"));
			Request->SetStringField(TEXT("savePath"), Current->GetStringField(TEXT("savePath")));
			Request->SetStringField(TEXT("flavor"), Current->GetStringField(TEXT("flavor")));
			Request->SetNumberField(TEXT("slot"), Current->GetIntegerField(TEXT("slot")));
			auto Item = MakeShared<FJsonObject>();
			Item->SetNumberField(TEXT("itemId"), ItemId); Item->SetNumberField(TEXT("quantity"), Quantity);
			Request->SetArrayField(TEXT("items"), {MakeShared<FJsonValueObject>(Item)});
			auto Result = TunaSweeperSaveTool::Execute(Request);
			if (Result->GetBoolField(TEXT("ok")) && !FTunaSweeperQuestCheatTool::IsEditingLocked())
			{
				Request->SetStringField(TEXT("expectedHash"), Result->GetStringField(TEXT("sourceHash")));
				Request->SetBoolField(TEXT("commit"), true);
				Result = TunaSweeperSaveTool::Execute(Request);
			}
			if (Result->GetBoolField(TEXT("ok")) && Result->GetBoolField(TEXT("committed")))
				Status = FText::Format(Text(TEXT("ui.item_grant.success")), ItemName(ItemId), FText::AsNumber(Quantity));
			else if (Result->GetStringField(TEXT("code")) == TEXT("inventory_full")) Status = Text(TEXT("ui.item_grant.full"));
			else Status = Text(TEXT("ui.item_grant.failed"));
			return FReply::Handled();
		}
		FReply OpenPicker()
		{
			if (auto Existing = PickerWindow.Pin()) { Existing->BringToFront(); return FReply::Handled(); }
			TArray<TSharedPtr<FJsonObject>> Featured = Catalog.FilterByPredicate([](const auto& Item)
			{ return IsFeaturedItem(Item->GetIntegerField(TEXT("itemId"))) && Item->GetBoolField(TEXT("canAdd")); });
			Featured.Sort([](const auto& A, const auto& B)
			{
				const int32 Left = A->GetIntegerField(TEXT("itemId")), Right = B->GetIntegerField(TEXT("itemId"));
				return Left == 6019 ? Right != 6019 : Right != 6019 && Left < Right;
			});
			auto List = SNew(SScrollBox);
			const auto State = AsShared();
			for (const auto& Item : Featured)
			{
				const int32 Id = Item->GetIntegerField(TEXT("itemId"));
				List->AddSlot().Padding(0, 2)
				[ SNew(SButton).Text(FText::Format(Text(TEXT("ui.item_grant.row")), FText::FromString(LexToString(Id)), ItemName(Id)))
					.OnClicked_Lambda([State, Id]
					{
						State->ItemId = Id;
						if (auto Window = State->PickerWindow.Pin()) Window->RequestDestroyWindow();
						return FReply::Handled();
					}) ];
			}
			auto Window = SNew(SWindow).Title(Text(TEXT("ui.item_grant.picker"))).ClientSize(FVector2D(480, 620)).SupportsMaximize(false).SupportsMinimize(false)
				[ SNew(SBorder).Padding(12)[ List ] ];
			PickerWindow = Window;
			FSlateApplication::Get().AddWindow(Window);
			return FReply::Handled();
		}
	};
}

void FTunaSweeperItemGrantTool::Startup()
{
	using namespace TunaSweeperItemGrantEditor;
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(TabId, FOnSpawnTab::CreateRaw(this, &FTunaSweeperItemGrantTool::Spawn))
		.SetDisplayName(Text(TEXT("ui.item_grant.title"))).SetMenuType(ETabSpawnerMenuType::Hidden);
	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FTunaSweeperItemGrantTool::RegisterMenus));
}
void FTunaSweeperItemGrantTool::Shutdown()
{
	UToolMenus::UnRegisterStartupCallback(this); UToolMenus::UnregisterOwner(this);
	if (auto Tab = FGlobalTabmanager::Get()->FindExistingLiveTab(TunaSweeperItemGrantEditor::TabId)) Tab->RequestCloseTab();
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(TunaSweeperItemGrantEditor::TabId);
}
void FTunaSweeperItemGrantTool::RegisterMenus()
{
	FToolMenuOwnerScoped Owner(this);
	auto* Menu = UToolMenus::Get()->ExtendMenu(TEXT("LevelEditor.MainMenu.TunaSweeper"));
	auto& Entry = Menu->FindOrAddSection(TEXT("Debug")).AddMenuEntry(TEXT("ItemGrant"),
		TunaSweeperItemGrantEditor::Text(TEXT("ui.item_grant.title")), TunaSweeperItemGrantEditor::Text(TEXT("ui.item_grant.description")), FSlateIcon(),
		FUIAction(FExecuteAction::CreateRaw(this, &FTunaSweeperItemGrantTool::Open)));
	Entry.InsertPosition = FToolMenuInsert(TEXT("QuestCheat"), EToolMenuInsertType::After);
}
void FTunaSweeperItemGrantTool::Open() { FGlobalTabmanager::Get()->TryInvokeTab(TunaSweeperItemGrantEditor::TabId); }
TSharedRef<SDockTab> FTunaSweeperItemGrantTool::Spawn(const FSpawnTabArgs& Args)
{
	using namespace TunaSweeperItemGrantEditor;
	auto State = MakeShared<FPanelState>(); State->Refresh();
	auto Tab = SNew(SDockTab).TabRole(ETabRole::NomadTab)
	[ SNew(SBorder).Padding(16)
		[ SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 10)[ SNew(STextBlock).Text(Text(TEXT("ui.item_grant.description"))).AutoWrapText(true) ]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 10)[ SNew(STextBlock).Text_Lambda([State] { return State->SelectionText(); }) ]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 10)[ SNew(STextBlock).Text(Text(TEXT("ui.quest_cheat.locked")))
				.Visibility_Lambda([] { return FTunaSweeperQuestCheatTool::IsEditingLocked() ? EVisibility::Visible : EVisibility::Collapsed; }) ]
			+ SVerticalBox::Slot().AutoHeight()[ SNew(STextBlock).Text(Text(TEXT("ui.item_grant.id"))) ]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 4, 0, 6)
			[ SNew(SNumericEntryBox<int32>).MinValue(1).MaxValue(MAX_int32).AllowSpin(false)
				.Value_Lambda([State]() -> TOptional<int32> { return State->ItemId; })
				.OnValueChanged_Lambda([State](int32 Value) { State->ItemId = Value; }) ]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 8)[ SNew(STextBlock).Text_Lambda([State] { return State->ItemName(State->ItemId); }) ]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 12)[ SNew(SButton).Text(Text(TEXT("ui.item_grant.picker"))).OnClicked_Lambda([State] { return State->OpenPicker(); }) ]
			+ SVerticalBox::Slot().AutoHeight()[ SNew(STextBlock).Text(Text(TEXT("ui.item_grant.quantity"))) ]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 4, 0, 12)
			[ SNew(SNumericEntryBox<int32>).MinValue(1).MaxValue(1000000).AllowSpin(false)
				.Value_Lambda([State]() -> TOptional<int32> { return State->Quantity; })
				.OnValueChanged_Lambda([State](int32 Value) { State->Quantity = Value; }) ]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 8)[ SNew(SButton).Text(Text(TEXT("ui.item_grant.grant")))
				.IsEnabled_Lambda([State] { return State->CanGrant(); }).OnClicked_Lambda([State] { return State->Grant(); }) ]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 10)[ SNew(SButton).Text(Text(TEXT("ui.quest_cheat.refresh")))
				.IsEnabled_Lambda([] { return !FTunaSweeperQuestCheatTool::IsEditingLocked(); })
				.OnClicked_Lambda([State] { State->Refresh(); return FReply::Handled(); }) ]
			+ SVerticalBox::Slot().AutoHeight()[ SNew(STextBlock).Text_Lambda([State] { return State->Status; }).AutoWrapText(true) ]
		] ];
	Tab->SetOnTabClosed(SDockTab::FOnTabClosedCallback::CreateLambda([State](TSharedRef<SDockTab>)
	{ if (auto Window = State->PickerWindow.Pin()) Window->RequestDestroyWindow(); }));
	return Tab;
}
