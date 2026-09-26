#include "UI/TunaSweeperBossLabWidget.h"

#include "Blueprint/WidgetTree.h"
#include "BossLab/TunaSweeperBossLabGameMode.h"
#include "BossLab/TunaSweeperBossLabSubsystem.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/CheckBox.h"
#include "Components/EditableTextBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/SpinBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/World.h"
#include "Game/TunaSweeperGameInstance.h"
#include "InputCoreTypes.h"
#include "UI/TunaSweeperUIFont.h"
#include "UI/TunaSweeperUIStyle.h"

void UTunaSweeperBossLabComboLabel::SetLabelText(const FText& Value)
{
	LabelText = Value;
	if (Label) Label->SetText(Value);
}

TSharedRef<SWidget> UTunaSweeperBossLabComboLabel::RebuildWidget()
{
	if (!WidgetTree) WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"));
	if (!Label)
	{
		Label = WidgetTree->ConstructWidget<UTextBlock>();
		Label->SetText(LabelText);
		Label->SetAutoWrapText(true);
		TunaSweeperUIStyle::ApplyLabel(Label, 14);
		WidgetTree->RootWidget = Label;
	}
	return Super::RebuildWidget();
}

void UTunaSweeperBossLabButtonAction::Execute()
{
	if (Owner.IsValid()) Owner->HandleAction(Action);
}

FText UTunaSweeperBossLabWidget::Resolve(FName Key) const
{
	const UTunaSweeperGameInstance* Instance = GetGameInstance<UTunaSweeperGameInstance>();
	return Instance ? Instance->ResolveLocalizedText(Key, FText::GetEmpty()) : FText::GetEmpty();
}

FText UTunaSweeperBossLabWidget::Text(const TCHAR* Key) const
{
	return Resolve(FName(*(FString(TEXT("ui.boss_lab.")) + Key)));
}

UTunaSweeperBossLabSubsystem* UTunaSweeperBossLabWidget::Session() const
{
	return GetGameInstance() ? GetGameInstance()->GetSubsystem<UTunaSweeperBossLabSubsystem>() : nullptr;
}

ATunaSweeperBossLabGameMode* UTunaSweeperBossLabWidget::LabMode() const
{
	return GetWorld() ? Cast<ATunaSweeperBossLabGameMode>(GetWorld()->GetAuthGameMode()) : nullptr;
}

TSharedRef<SWidget> UTunaSweeperBossLabWidget::RebuildWidget()
{
	BuildWidgetTree();
	return Super::RebuildWidget();
}

void UTunaSweeperBossLabWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(true);
	if (UTunaSweeperGameInstance* Instance = GetGameInstance<UTunaSweeperGameInstance>())
	{
		Instance->OnLanguageChanged.RemoveAll(this);
		Instance->OnLanguageChanged.AddUObject(this, &UTunaSweeperBossLabWidget::RefreshFromSession);
	}
	RefreshFromSession();
}

void UTunaSweeperBossLabWidget::NativeDestruct()
{
	if (UTunaSweeperGameInstance* Instance = GetGameInstance<UTunaSweeperGameInstance>())
		Instance->OnLanguageChanged.RemoveAll(this);
	Super::NativeDestruct();
}

FReply UTunaSweeperBossLabWidget::NativeOnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event)
{
	if (Event.GetKey() == EKeys::Escape)
	{
		if (PendingAction != ETunaSweeperBossLabAction::None) CloseConfirmation();
		else HandleAction(ETunaSweeperBossLabAction::ReturnTitle);
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(Geometry, Event);
}

UTextBlock* UTunaSweeperBossLabWidget::MakeText(const FText& Value, int32 Size)
{
	UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>();
	Label->SetText(Value);
	Label->SetAutoWrapText(true);
	TunaSweeperUIStyle::ApplyLabel(Label, Size);
	return Label;
}

UTextBlock* UTunaSweeperBossLabWidget::AddLabel(UVerticalBox* Parent, const TCHAR* Key, int32 Size)
{
	UTextBlock* Label = MakeText(Text(Key), Size);
	StaticLabels.Add(Label, FName(*(FString(TEXT("ui.boss_lab.")) + Key)));
	Parent->AddChildToVerticalBox(Label)->SetPadding(FMargin(0, 6, 0, 4));
	return Label;
}

UButton* UTunaSweeperBossLabWidget::AddButton(UVerticalBox* Parent, const TCHAR* Key, ETunaSweeperBossLabAction Action, bool bDanger)
{
	UButton* Button = WidgetTree->ConstructWidget<UButton>();
	UTextBlock* Label = MakeText(Text(Key), 14);
	// A fixed wrapping width avoids retaining a narrow previous-language desired width.
	Label->SetAutoWrapText(false);
	Label->SetWrapTextAt(210.f);
	Label->SetJustification(ETextJustify::Center);
	StaticLabels.Add(Label, FName(*(FString(TEXT("ui.boss_lab.")) + Key)));
	Button->SetContent(Label);
	TunaSweeperUIStyle::ApplyButton(Button, bDanger ? TunaSweeperUIStyle::EButtonRole::Danger : TunaSweeperUIStyle::EButtonRole::Secondary);
	UTunaSweeperBossLabButtonAction* Receiver = NewObject<UTunaSweeperBossLabButtonAction>(this);
	Receiver->Owner = this;
	Receiver->Action = Action;
	ButtonActions.Add(Receiver);
	Button->OnClicked.AddDynamic(Receiver, &UTunaSweeperBossLabButtonAction::Execute);
	Parent->AddChildToVerticalBox(Button)->SetPadding(FMargin(0, 3));
	return Button;
}

UWidget* UTunaSweeperBossLabWidget::GenerateComboLabel(FString Option)
{
	UTunaSweeperBossLabComboLabel* Label = CreateWidget<UTunaSweeperBossLabComboLabel>(this);
	if (Label) Label->SetLabelText(FText::FromString(Option));
	return Label;
}

UComboBoxString* UTunaSweeperBossLabWidget::AddCombo(UVerticalBox* Parent, const TCHAR* Key)
{
	if (Key) AddLabel(Parent, Key);
	UComboBoxString* Combo = WidgetTree->ConstructWidget<UComboBoxString>();
	Combo->SetContentPadding(FMargin(8, 6));
	Combo->SetMaxListHeight(300);
	Combo->OnGenerateWidgetEvent.BindDynamic(this, &UTunaSweeperBossLabWidget::GenerateComboLabel);
	Parent->AddChildToVerticalBox(Combo)->SetPadding(FMargin(0, 0, 0, 4));
	return Combo;
}

USpinBox* UTunaSweeperBossLabWidget::AddNumber(UVerticalBox* Parent, const TCHAR* Key, float Minimum, float Maximum, float Step)
{
	AddLabel(Parent, Key);
	USpinBox* Input = WidgetTree->ConstructWidget<USpinBox>();
	Input->SetMinValue(Minimum);
	Input->SetMaxValue(Maximum);
	Input->SetMinSliderValue(Minimum);
	Input->SetMaxSliderValue(Maximum);
	Input->SetDelta(Step);
	Input->SetMaxFractionalDigits(Step < 1.f ? 1 : 0);
	Input->SetFont(TunaSweeperUIFont::MakeFont(nullptr, 15));
	Parent->AddChildToVerticalBox(Input)->SetPadding(FMargin(0, 0, 0, 4));
	return Input;
}

void UTunaSweeperBossLabWidget::BuildWidgetTree()
{
	if (WidgetTree && WidgetTree->RootWidget) return;
	if (!WidgetTree) WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"));
	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>();
	WidgetTree->RootWidget = Root;
	WorkingLayer = WidgetTree->ConstructWidget<UCanvasPanel>();
	UCanvasPanelSlot* WorkingSlot = Root->AddChildToCanvas(WorkingLayer);
	WorkingSlot->SetAnchors(FAnchors(0, 0, 1, 1));
	WorkingSlot->SetOffsets(FMargin(0));

	TitleText = MakeText(FText::GetEmpty(), 26);
	TitleText->SetJustification(ETextJustify::Center);
	UCanvasPanelSlot* TitleSlot = WorkingLayer->AddChildToCanvas(TitleText);
	TitleSlot->SetAnchors(FAnchors(0, 0, 1, 0));
	TitleSlot->SetOffsets(FMargin(20, 18, 20, 42));

	auto MakePanel = [this](bool bRight, UBorder*& OutPanel) -> UVerticalBox*
	{
		OutPanel = WidgetTree->ConstructWidget<UBorder>();
		OutPanel->SetBrushColor(FLinearColor(0.015f, 0.055f, 0.07f, 0.95f));
		OutPanel->SetPadding(FMargin(14));
		UCanvasPanelSlot* PanelSlot = WorkingLayer->AddChildToCanvas(OutPanel);
		PanelSlot->SetAnchors(bRight ? FAnchors(1, 0, 1, 1) : FAnchors(0, 0, 0, 1));
		PanelSlot->SetOffsets(bRight ? FMargin(-328, 74, 310, 20) : FMargin(18, 74, 280, 20));
		UScrollBox* Scroll = WidgetTree->ConstructWidget<UScrollBox>();
		Scroll->SetScrollBarVisibility(ESlateVisibility::Visible);
		Scroll->SetScrollbarThickness(FVector2D(5));
		OutPanel->SetContent(Scroll);
		UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>();
		Scroll->AddChild(Content);
		return Content;
	};
	UBorder* LibraryPanel = nullptr;
	UVerticalBox* Library = MakePanel(false, LibraryPanel);
	AddLabel(Library, TEXT("library"), 20);
	SlotCombo = AddCombo(Library, nullptr);
	SlotCombo->OnSelectionChanged.AddDynamic(this, &UTunaSweeperBossLabWidget::OnSlotChanged);
	AddButton(Library, TEXT("refresh"), ETunaSweeperBossLabAction::Refresh);
	LoadButton = AddButton(Library, TEXT("load"), ETunaSweeperBossLabAction::Load);
	LibraryEditActions = WidgetTree->ConstructWidget<UVerticalBox>();
	Library->AddChildToVerticalBox(LibraryEditActions)->SetPadding(FMargin(0, 8));
	AddButton(LibraryEditActions, TEXT("new"), ETunaSweeperBossLabAction::New);
	AddButton(LibraryEditActions, TEXT("save"), ETunaSweeperBossLabAction::Save);
	AddButton(LibraryEditActions, TEXT("duplicate"), ETunaSweeperBossLabAction::Duplicate);
	DeleteButton = AddButton(LibraryEditActions, TEXT("delete"), ETunaSweeperBossLabAction::Delete, true);
	AddButton(Library, TEXT("open_folder"), ETunaSweeperBossLabAction::OpenFolder);
	PlayButton = AddButton(Library, TEXT("test"), ETunaSweeperBossLabAction::Play);
	PlayText = Cast<UTextBlock>(PlayButton->GetContent());
	StaticLabels.Remove(PlayText);
	TunaSweeperUIStyle::ApplyButton(PlayButton);
	AddButton(Library, TEXT("return_title"), ETunaSweeperBossLabAction::ReturnTitle);

	UBorder* EditingBorder = nullptr;
	UVerticalBox* Editing = MakePanel(true, EditingBorder);
	EditorPanel = EditingBorder;
	AddLabel(Editing, TEXT("name"));
	NameInput = WidgetTree->ConstructWidget<UEditableTextBox>();
	FEditableTextBoxStyle NameStyle = NameInput->GetWidgetStyle();
	NameStyle.SetFont(TunaSweeperUIFont::MakeFont(nullptr, 15));
	NameInput->SetWidgetStyle(NameStyle);
	NameInput->OnTextCommitted.AddDynamic(this, &UTunaSweeperBossLabWidget::OnNameCommitted);
	Editing->AddChildToVerticalBox(NameInput);
	PartCombo = AddCombo(Editing, TEXT("parts"));
	PartCombo->OnSelectionChanged.AddDynamic(this, &UTunaSweeperBossLabWidget::OnPartChanged);
	AddButton(Editing, TEXT("remove_branch"), ETunaSweeperBossLabAction::RemoveBranch, true);
	ModuleCombo = AddCombo(Editing, TEXT("module"));
	ParentCombo = AddCombo(Editing, TEXT("parent"));
	SocketCombo = AddCombo(Editing, TEXT("socket"));
	YawCombo = AddCombo(Editing, TEXT("yaw"));
	AddButton(Editing, TEXT("add_part"), ETunaSweeperBossLabAction::AddPart);
	TacticCombo = AddCombo(Editing, TEXT("tactics"));
	TacticCombo->OnSelectionChanged.AddDynamic(this, &UTunaSweeperBossLabWidget::OnTacticChanged);
	IntervalInput = AddNumber(Editing, TEXT("attack_interval"), 1.f, 8.f, 0.1f);
	IntervalInput->OnValueChanged.AddDynamic(this, &UTunaSweeperBossLabWidget::OnIntervalChanged);
	PhaseInput = AddNumber(Editing, TEXT("phase_threshold"), 10.f, 90.f, 1.f);
	PhaseInput->OnValueChanged.AddDynamic(this, &UTunaSweeperBossLabWidget::OnPhaseChanged);
	AlternateInput = WidgetTree->ConstructWidget<UCheckBox>();
	UTextBlock* AlternateLabel = MakeText(Text(TEXT("alternate_weapons")));
	StaticLabels.Add(AlternateLabel, TEXT("ui.boss_lab.alternate_weapons"));
	AlternateInput->SetContent(AlternateLabel);
	AlternateInput->OnCheckStateChanged.AddDynamic(this, &UTunaSweeperBossLabWidget::OnAlternateChanged);
	Editing->AddChildToVerticalBox(AlternateInput)->SetPadding(FMargin(0, 8));

	UVerticalBox* Info = WidgetTree->ConstructWidget<UVerticalBox>();
	UCanvasPanelSlot* InfoSlot = WorkingLayer->AddChildToCanvas(Info);
	InfoSlot->SetAnchors(FAnchors(0, 0, 1, 0));
	InfoSlot->SetOffsets(FMargin(316, 78, 346, 170));
	SummaryText = MakeText(FText::GetEmpty(), 20);
	SummaryText->SetJustification(ETextJustify::Center);
	Info->AddChildToVerticalBox(SummaryText);
	HintText = MakeText(FText::GetEmpty(), 13);
	HintText->SetJustification(ETextJustify::Center);
	Info->AddChildToVerticalBox(HintText)->SetPadding(FMargin(0, 8));
	UHorizontalBox* RotationRow = WidgetTree->ConstructWidget<UHorizontalBox>();
	Info->AddChildToVerticalBox(RotationRow)->SetHorizontalAlignment(HAlign_Center);
	for (int32 Index = 0; Index < 2; ++Index)
	{
		UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>();
		RotationRow->AddChildToHorizontalBox(Column)->SetPadding(FMargin(3, 0));
		AddButton(Column, Index == 0 ? TEXT("rotate_left") : TEXT("rotate_right"),
			Index == 0 ? ETunaSweeperBossLabAction::RotateLeft : ETunaSweeperBossLabAction::RotateRight);
	}
	StatusText = MakeText(FText::GetEmpty(), 15);
	StatusText->SetJustification(ETextJustify::Center);
	UCanvasPanelSlot* StatusSlot = WorkingLayer->AddChildToCanvas(StatusText);
	StatusSlot->SetAnchors(FAnchors(0, 1, 1, 1));
	StatusSlot->SetOffsets(FMargin(316, -126, 346, 100));

	// This full-screen hit target and disabled working layer make confirmation modal.
	ConfirmationLayer = WidgetTree->ConstructWidget<UBorder>();
	ConfirmationLayer->SetBrushColor(FLinearColor(0, 0, 0, 0.7f));
	ConfirmationLayer->SetHorizontalAlignment(HAlign_Center);
	ConfirmationLayer->SetVerticalAlignment(VAlign_Center);
	UCanvasPanelSlot* ConfirmationSlot = Root->AddChildToCanvas(ConfirmationLayer);
	ConfirmationSlot->SetAnchors(FAnchors(0, 0, 1, 1));
	ConfirmationSlot->SetOffsets(FMargin(0));
	ConfirmationSlot->SetZOrder(10);
	USizeBox* ConfirmationSize = WidgetTree->ConstructWidget<USizeBox>();
	ConfirmationSize->SetWidthOverride(440);
	ConfirmationLayer->SetContent(ConfirmationSize);
	UBorder* ConfirmationCard = WidgetTree->ConstructWidget<UBorder>();
	ConfirmationCard->SetPadding(FMargin(24));
	ConfirmationCard->SetBrushColor(FLinearColor(0.025f, 0.085f, 0.1f, 1));
	ConfirmationSize->SetContent(ConfirmationCard);
	UVerticalBox* ConfirmationContent = WidgetTree->ConstructWidget<UVerticalBox>();
	ConfirmationCard->SetContent(ConfirmationContent);
	ConfirmationText = MakeText(FText::GetEmpty(), 18);
	ConfirmationContent->AddChildToVerticalBox(ConfirmationText)->SetPadding(FMargin(0, 0, 0, 18));
	AddButton(ConfirmationContent, TEXT("confirm"), ETunaSweeperBossLabAction::Confirm, true);
	AddButton(ConfirmationContent, TEXT("cancel"), ETunaSweeperBossLabAction::Cancel);
	ConfirmationLayer->SetVisibility(ESlateVisibility::Collapsed);
}

FText UTunaSweeperBossLabWidget::PartText(const FTunaSweeperBossPart& Part) const
{
	const FTunaSweeperBossModuleDefinition* Module = TunaSweeperBossDefinition::FindModule(Part.ModuleId);
	return FText::Format(Text(TEXT("part_format")), Part.InstanceId, Module ? Resolve(Module->NameKey) : FText::GetEmpty());
}

void UTunaSweeperBossLabWidget::RefreshFromSession()
{
	if (!WorkingLayer || !Session()) return;
	for (const auto& Label : StaticLabels) if (Label.Key.IsValid()) Label.Key->SetText(Resolve(Label.Value));
	const bool bDevelopment = Session()->IsDevelopmentMode();
	EditorPanel->SetVisibility(bDevelopment ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	LibraryEditActions->SetVisibility(bDevelopment ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	TitleText->SetText(Text(bDevelopment ? TEXT("development_title") : TEXT("single_title")));
	HintText->SetText(Text(bDevelopment ? TEXT("editing_hint") : TEXT("single_hint")));
	PlayText->SetText(Text(bDevelopment ? TEXT("test") : TEXT("play")));
	RefreshLibrary();
	RefreshDraftControls();
	RefreshSummary();
	if (LabMode() && !LabMode()->GetStatusText().IsEmpty()) SetStatus(LabMode()->GetStatusText());
	else SetStatusKey(bDevelopment ? TEXT("status_ready") : TEXT("select_saved_slot"));
}

void UTunaSweeperBossLabWidget::RefreshLibrary()
{
	UTunaSweeperBossLabSubsystem* Library = Session();
	if (!Library || !SlotCombo) return;
	TGuardValue<bool> Guard(bRefreshing, true);
	SlotCombo->ClearOptions();
	for (int32 Index = 0; Index < TunaSweeperBossDefinition::SlotCount; ++Index)
	{
		FText Name = Text(TEXT("empty_slot"));
		if (Library->SlotExists(Index))
		{
			FTunaSweeperBossDefinition Definition;
			FName Error;
			Name = Library->LoadSlot(Index, Definition, Error)
				? (Definition.Name.IsEmpty() ? Text(TEXT("unnamed")) : FText::FromString(Definition.Name))
				: Text(TEXT("invalid_slot"));
		}
		SlotCombo->AddOption(FText::Format(Text(TEXT("slot_format")), Index + 1, Name).ToString());
	}
	SlotCombo->SetSelectedIndex(Library->GetSelectedSlot());
	const bool bExists = Library->SlotExists(Library->GetSelectedSlot());
	LoadButton->SetIsEnabled(bExists);
	DeleteButton->SetIsEnabled(bExists);
	PlayButton->SetIsEnabled(Library->IsDevelopmentMode() || (LoadedSlot == Library->GetSelectedSlot() && bExists));
}

void UTunaSweeperBossLabWidget::RefreshDraftControls()
{
	if (!Session() || !PartCombo) return;
	TGuardValue<bool> Guard(bRefreshing, true);
	const FTunaSweeperBossDefinition& Draft = Session()->GetDraft();
	const int32 PreviousParentIndex = ParentCombo->GetSelectedIndex();
	const int32 PreviousParentId = PartIds.IsValidIndex(PreviousParentIndex) ? PartIds[PreviousParentIndex] : 1;
	const int32 PreviousModuleIndex = ModuleCombo->GetSelectedIndex();
	const int32 PreviousSocketIndex = SocketCombo->GetSelectedIndex();
	const int32 PreviousYawIndex = YawCombo->GetSelectedIndex();
	NameInput->SetText(FText::FromString(Draft.Name));
	PartCombo->ClearOptions();
	ParentCombo->ClearOptions();
	PartIds.Reset();
	for (const FTunaSweeperBossPart& Part : Draft.Parts)
	{
		PartIds.Add(Part.InstanceId);
		const FString Label = PartText(Part).ToString();
		PartCombo->AddOption(Label);
		ParentCombo->AddOption(Label);
	}
	int32 SelectedIndex = PartIds.IndexOfByKey(SelectedPartId);
	if (SelectedIndex == INDEX_NONE && !PartIds.IsEmpty()) { SelectedIndex = 0; SelectedPartId = PartIds[0]; }
	PartCombo->SetSelectedIndex(SelectedIndex);
	ParentCombo->SetSelectedIndex(FMath::Max(0, PartIds.IndexOfByKey(PreviousParentId)));
	ModuleCombo->ClearOptions();
	ModuleIds.Reset();
	for (const FTunaSweeperBossModuleDefinition& Module : TunaSweeperBossDefinition::GetCatalog())
	{
		if (Module.Id == TEXT("core")) continue;
		ModuleIds.Add(Module.Id);
		ModuleCombo->AddOption(Resolve(Module.NameKey).ToString());
	}
	ModuleCombo->SetSelectedIndex(FMath::Clamp(PreviousModuleIndex, 0, ModuleIds.Num() - 1));
	SocketCombo->ClearOptions();
	for (const TCHAR* Key : { TEXT("front"), TEXT("back"), TEXT("left"), TEXT("right"), TEXT("up"), TEXT("down") })
		SocketCombo->AddOption(Text(Key).ToString());
	SocketCombo->SetSelectedIndex(FMath::Clamp(PreviousSocketIndex, 0, 5));
	YawCombo->ClearOptions();
	for (int32 Index = 0; Index < 4; ++Index) YawCombo->AddOption(FText::AsNumber(Index * 90).ToString());
	YawCombo->SetSelectedIndex(FMath::Clamp(PreviousYawIndex, 0, 3));
	TacticCombo->ClearOptions();
	for (const TCHAR* Key : { TEXT("balanced"), TEXT("keep_distance"), TEXT("advance") })
		TacticCombo->AddOption(Text(Key).ToString());
	TacticCombo->SetSelectedIndex(static_cast<int32>(Draft.Tactic));
	IntervalInput->SetValue(Draft.AttackInterval);
	PhaseInput->SetValue(Draft.PhaseThreshold * 100.f);
	AlternateInput->SetIsChecked(Draft.bAlternateWeapons);
}

void UTunaSweeperBossLabWidget::RefreshSummary()
{
	if (!Session() || !SummaryText) return;
	if (!Session()->IsDevelopmentMode() && LoadedSlot == INDEX_NONE)
	{
		SummaryText->SetText(Text(TEXT("select_saved_slot")));
		return;
	}
	const FTunaSweeperBossDefinition& Draft = Session()->GetDraft();
	SummaryText->SetText(FText::Format(Text(TEXT("draft_summary")),
		Draft.Name.IsEmpty() ? Text(TEXT("unnamed")) : FText::FromString(Draft.Name), Draft.Parts.Num()));
}

void UTunaSweeperBossLabWidget::RefreshPreview()
{
	if (LabMode() && Session())
	{
		LabMode()->RefreshPreview(Session()->GetDraft());
		LabMode()->SetPreviewSelectedPart(SelectedPartId);
	}
	RefreshSummary();
}

void UTunaSweeperBossLabWidget::SetStatus(const FText& Message, bool bError)
{
	if (!StatusText) return;
	StatusText->SetText(Message);
	StatusText->SetColorAndOpacity(bError ? FLinearColor(1.f, 0.42f, 0.32f) : FLinearColor(0.8f, 0.96f, 1.f));
}

void UTunaSweeperBossLabWidget::SetStatusKey(const TCHAR* Key, bool bError)
{
	SetStatus(Text(Key), bError);
}
