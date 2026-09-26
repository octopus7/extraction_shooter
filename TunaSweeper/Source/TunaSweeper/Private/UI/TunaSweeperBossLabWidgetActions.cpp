#include "UI/TunaSweeperBossLabWidget.h"

#include "BossLab/TunaSweeperBossLabGameMode.h"
#include "BossLab/TunaSweeperBossLabSubsystem.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/EditableTextBox.h"
#include "Components/TextBlock.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformProcess.h"
#include "Misc/Paths.h"

void UTunaSweeperBossLabWidget::RequestConfirmation(ETunaSweeperBossLabAction Action, const TCHAR* Key)
{
	PendingAction = Action;
	ConfirmationText->SetText(Text(Key));
	WorkingLayer->SetIsEnabled(false);
	ConfirmationLayer->SetVisibility(ESlateVisibility::Visible);
	SetKeyboardFocus();
}

void UTunaSweeperBossLabWidget::CloseConfirmation()
{
	PendingAction = ETunaSweeperBossLabAction::None;
	ConfirmationLayer->SetVisibility(ESlateVisibility::Collapsed);
	WorkingLayer->SetIsEnabled(true);
}

void UTunaSweeperBossLabWidget::HandleAction(ETunaSweeperBossLabAction Action)
{
	if (!Session() || (LabMode() && LabMode()->IsBattleActive())) return;
	if (Action == ETunaSweeperBossLabAction::Confirm)
	{
		const ETunaSweeperBossLabAction ConfirmedAction = PendingAction;
		CloseConfirmation();
		ExecuteAction(ConfirmedAction, true);
	}
	else if (Action == ETunaSweeperBossLabAction::Cancel)
	{
		CloseConfirmation();
	}
	else if (PendingAction == ETunaSweeperBossLabAction::None)
	{
		ExecuteAction(Action, false);
	}
}

void UTunaSweeperBossLabWidget::ExecuteAction(ETunaSweeperBossLabAction Action, bool bConfirmed)
{
	UTunaSweeperBossLabSubsystem* Library = Session();
	if (!Library) return;
	const int32 SlotIndex = Library->GetSelectedSlot();
	FName Error;
	const bool bEditorAction = Action == ETunaSweeperBossLabAction::New || Action == ETunaSweeperBossLabAction::Save ||
		Action == ETunaSweeperBossLabAction::Duplicate || Action == ETunaSweeperBossLabAction::Delete ||
		Action == ETunaSweeperBossLabAction::AddPart || Action == ETunaSweeperBossLabAction::RemoveBranch;
	if (bEditorAction && !Library->IsDevelopmentMode()) return;
	if (!bConfirmed)
	{
		if ((Action == ETunaSweeperBossLabAction::Load || Action == ETunaSweeperBossLabAction::New ||
			Action == ETunaSweeperBossLabAction::ReturnTitle) && Library->IsDirty())
		{
			RequestConfirmation(Action, Action == ETunaSweeperBossLabAction::ReturnTitle ? TEXT("confirm_leave") : TEXT("confirm_discard"));
			return;
		}
		if (Action == ETunaSweeperBossLabAction::Save && Library->SlotExists(SlotIndex))
		{
			RequestConfirmation(Action, TEXT("confirm_overwrite"));
			return;
		}
		if (Action == ETunaSweeperBossLabAction::Delete && Library->SlotExists(SlotIndex))
		{
			RequestConfirmation(Action, TEXT("confirm_delete"));
			return;
		}
		if (Action == ETunaSweeperBossLabAction::RemoveBranch)
		{
			const FTunaSweeperBossPart* Selected = Library->GetDraft().Parts.FindByPredicate(
				[this](const FTunaSweeperBossPart& Part) { return Part.InstanceId == SelectedPartId; });
			if (!Selected || Selected->ParentId == INDEX_NONE)
			{
				SetStatusKey(TEXT("core_cannot_remove"), true);
				return;
			}
			RequestConfirmation(Action, TEXT("confirm_remove_branch"));
			return;
		}
	}
	switch (Action)
	{
	case ETunaSweeperBossLabAction::Refresh:
		// Explicit reload is required after external replacement of a selected file.
		if (!Library->IsDevelopmentMode()) LoadedSlot = INDEX_NONE;
		RefreshLibrary();
		RefreshSummary();
		SetStatusKey(Library->IsDevelopmentMode() ? TEXT("status_ready") : TEXT("select_saved_slot"));
		break;
	case ETunaSweeperBossLabAction::New:
		Library->SetDraft(TunaSweeperBossDefinition::MakeDefault());
		Library->SetDirty(true);
		LoadedSlot = INDEX_NONE;
		SelectedPartId = Library->GetDraft().Parts.IsEmpty() ? INDEX_NONE : Library->GetDraft().Parts[0].InstanceId;
		RefreshDraftControls();
		RefreshPreview();
		SetStatusKey(TEXT("status_dirty"));
		break;
	case ETunaSweeperBossLabAction::Load:
		LoadSelectedSlot();
		break;
	case ETunaSweeperBossLabAction::Save:
		if (!Library->SaveSlot(SlotIndex, Library->GetDraft(), Error)) { SetStatus(Resolve(Error), true); break; }
		Library->SetDirty(false);
		LoadedSlot = SlotIndex;
		RefreshLibrary();
		SetStatusKey(TEXT("status_saved"));
		break;
	case ETunaSweeperBossLabAction::Duplicate:
	{
		int32 EmptyIndex = INDEX_NONE;
		for (int32 Index = 0; Index < TunaSweeperBossDefinition::SlotCount; ++Index)
			if (!Library->SlotExists(Index)) { EmptyIndex = Index; break; }
		if (EmptyIndex == INDEX_NONE) { SetStatusKey(TEXT("no_free_slot"), true); break; }
		FTunaSweeperBossDefinition Copy = Library->GetDraft();
		Copy.BossId = FGuid::NewGuid();
		if (!Library->SaveSlot(EmptyIndex, Copy, Error)) { SetStatus(Resolve(Error), true); break; }
		Library->SetDraft(Copy);
		Library->SetSelectedSlot(EmptyIndex);
		Library->SetDirty(false);
		LoadedSlot = EmptyIndex;
		RefreshLibrary();
		RefreshSummary();
		SetStatusKey(TEXT("status_copied"));
		break;
	}
	case ETunaSweeperBossLabAction::Delete:
		if (!Library->DeleteSlot(SlotIndex, Error)) { SetStatus(Resolve(Error), true); break; }
		if (LoadedSlot == SlotIndex) { LoadedSlot = INDEX_NONE; Library->SetDirty(true); }
		RefreshLibrary();
		RefreshSummary();
		SetStatusKey(TEXT("status_deleted"));
		break;
	case ETunaSweeperBossLabAction::OpenFolder:
	{
		const FString Directory = FPaths::ConvertRelativePathToFull(Library->GetLibraryDirectory());
		if (!IFileManager::Get().MakeDirectory(*Directory, true))
		{
			SetStatus(Resolve(TEXT("ui.boss_lab.error.write")), true);
			break;
		}
		FPlatformProcess::ExploreFolder(*Directory);
		break;
	}
	case ETunaSweeperBossLabAction::ReturnTitle:
		if (LabMode()) LabMode()->ReturnToTitle();
		break;
	case ETunaSweeperBossLabAction::Play:
		if (!Library->IsDevelopmentMode() && (LoadedSlot != SlotIndex || !Library->SlotExists(SlotIndex)))
		{
			SetStatusKey(TEXT("select_saved_slot"), true);
			break;
		}
		if (!TunaSweeperBossDefinition::Validate(Library->GetDraft(), Error)) { SetStatus(Resolve(Error), true); break; }
		if (LabMode()) LabMode()->StartBattle();
		break;
	case ETunaSweeperBossLabAction::AddPart:
		AddPart();
		break;
	case ETunaSweeperBossLabAction::RemoveBranch:
	{
		FTunaSweeperBossDefinition Candidate = Library->GetDraft();
		const FTunaSweeperBossPart* Selected = Candidate.Parts.FindByPredicate(
			[this](const FTunaSweeperBossPart& Part) { return Part.InstanceId == SelectedPartId; });
		if (!Selected || Selected->ParentId == INDEX_NONE) { SetStatusKey(TEXT("core_cannot_remove"), true); break; }
		const int32 ParentId = Selected->ParentId;
		TunaSweeperBossDefinition::RemoveBranch(Candidate, SelectedPartId);
		if (ApplyDraft(Candidate))
		{
			SelectedPartId = ParentId;
			RefreshDraftControls();
			if (LabMode()) LabMode()->SetPreviewSelectedPart(SelectedPartId);
		}
		break;
	}
	case ETunaSweeperBossLabAction::RotateLeft:
		if (LabMode()) LabMode()->RotatePreview(-30.f);
		break;
	case ETunaSweeperBossLabAction::RotateRight:
		if (LabMode()) LabMode()->RotatePreview(30.f);
		break;
	default: break;
	}
}

void UTunaSweeperBossLabWidget::LoadSelectedSlot()
{
	UTunaSweeperBossLabSubsystem* Library = Session();
	FTunaSweeperBossDefinition Loaded;
	FName Error;
	if (!Library || !Library->LoadSlot(Library->GetSelectedSlot(), Loaded, Error))
	{
		SetStatus(Resolve(Error), true);
		return;
	}
	// Assignment happens only after complete parsing and validation; failed loads preserve the draft.
	Library->SetDraft(Loaded);
	Library->SetDirty(false);
	LoadedSlot = Library->GetSelectedSlot();
	SelectedPartId = Loaded.Parts[0].InstanceId;
	RefreshLibrary();
	RefreshDraftControls();
	RefreshPreview();
	SetStatusKey(TEXT("status_loaded"));
}

bool UTunaSweeperBossLabWidget::ApplyDraft(const FTunaSweeperBossDefinition& Candidate, bool bRefreshControls)
{
	UTunaSweeperBossLabSubsystem* Library = Session();
	if (!Library || !Library->IsDevelopmentMode()) return false;
	FName Error;
	if (!TunaSweeperBossDefinition::Validate(Candidate, Error))
	{
		RefreshDraftControls();
		SetStatus(Resolve(Error), true);
		return false;
	}
	Library->SetDraft(Candidate);
	Library->SetDirty(true);
	if (bRefreshControls) RefreshDraftControls();
	RefreshPreview();
	SetStatusKey(TEXT("status_dirty"));
	return true;
}

void UTunaSweeperBossLabWidget::AddPart()
{
	UTunaSweeperBossLabSubsystem* Library = Session();
	const int32 ParentIndex = ParentCombo->GetSelectedIndex();
	const int32 ModuleIndex = ModuleCombo->GetSelectedIndex();
	if (!Library || !PartIds.IsValidIndex(ParentIndex) || !ModuleIds.IsValidIndex(ModuleIndex)) return;
	FTunaSweeperBossDefinition Candidate = Library->GetDraft();
	const int32 ParentId = PartIds[ParentIndex];
	const int32 SocketIndex = SocketCombo->GetSelectedIndex();
	for (const FTunaSweeperBossPart& Part : Candidate.Parts)
	{
		if (Part.ParentId == ParentId && Part.SocketIndex == SocketIndex)
		{
			SetStatusKey(TEXT("socket_occupied"), true);
			return;
		}
	}
	FTunaSweeperBossPart Added;
	Added.InstanceId = 1;
	// Choose an available ID even when an imported file uses sparse, high IDs.
	while (Candidate.Parts.ContainsByPredicate([&Added](const FTunaSweeperBossPart& Part) { return Part.InstanceId == Added.InstanceId; }))
		++Added.InstanceId;
	Added.ModuleId = ModuleIds[ModuleIndex];
	Added.ParentId = ParentId;
	Added.SocketIndex = SocketIndex;
	Added.YawSteps = YawCombo->GetSelectedIndex();
	Candidate.Parts.Add(Added);
	if (ApplyDraft(Candidate))
	{
		SelectedPartId = Added.InstanceId;
		RefreshDraftControls();
		if (LabMode()) LabMode()->SetPreviewSelectedPart(SelectedPartId);
	}
}

void UTunaSweeperBossLabWidget::OnSlotChanged(FString Option, ESelectInfo::Type SelectionType)
{
	if (bRefreshing || !Session() || PendingAction != ETunaSweeperBossLabAction::None) return;
	const int32 Index = SlotCombo->GetSelectedIndex();
	if (Index < 0 || Index >= TunaSweeperBossDefinition::SlotCount) return;
	Session()->SetSelectedSlot(Index);
	const bool bExists = Session()->SlotExists(Index);
	LoadButton->SetIsEnabled(bExists);
	DeleteButton->SetIsEnabled(bExists);
	PlayButton->SetIsEnabled(Session()->IsDevelopmentMode() || (LoadedSlot == Index && bExists));
	if (!Session()->IsDevelopmentMode() && LoadedSlot != Index) SetStatusKey(TEXT("select_saved_slot"));
}

void UTunaSweeperBossLabWidget::OnPartChanged(FString Option, ESelectInfo::Type SelectionType)
{
	if (bRefreshing || !PartIds.IsValidIndex(PartCombo->GetSelectedIndex())) return;
	SelectedPartId = PartIds[PartCombo->GetSelectedIndex()];
	ParentCombo->SetSelectedIndex(PartCombo->GetSelectedIndex());
	if (LabMode()) LabMode()->SetPreviewSelectedPart(SelectedPartId);
}

void UTunaSweeperBossLabWidget::OnNameCommitted(const FText& Value, ETextCommit::Type CommitMethod)
{
	if (bRefreshing || !Session() || !Session()->IsDevelopmentMode()) return;
	FTunaSweeperBossDefinition Candidate = Session()->GetDraft();
	const FString Name = Value.ToString().TrimStartAndEnd();
	if (Name == Candidate.Name) return;
	Candidate.Name = Name;
	ApplyDraft(Candidate);
}

void UTunaSweeperBossLabWidget::OnTacticChanged(FString Option, ESelectInfo::Type SelectionType)
{
	if (bRefreshing || !Session() || TacticCombo->GetSelectedIndex() == INDEX_NONE) return;
	FTunaSweeperBossDefinition Candidate = Session()->GetDraft();
	Candidate.Tactic = static_cast<ETunaSweeperBossTactic>(TacticCombo->GetSelectedIndex());
	ApplyDraft(Candidate, false);
}

void UTunaSweeperBossLabWidget::OnIntervalChanged(float Value)
{
	if (bRefreshing || !Session()) return;
	FTunaSweeperBossDefinition Candidate = Session()->GetDraft();
	if (FMath::IsNearlyEqual(Candidate.AttackInterval, Value)) return;
	Candidate.AttackInterval = Value;
	ApplyDraft(Candidate, false);
}

void UTunaSweeperBossLabWidget::OnPhaseChanged(float Value)
{
	if (bRefreshing || !Session()) return;
	FTunaSweeperBossDefinition Candidate = Session()->GetDraft();
	const float Threshold = FMath::Clamp(Value / 100.f, 0.1f, 0.9f);
	if (FMath::IsNearlyEqual(Candidate.PhaseThreshold, Threshold)) return;
	Candidate.PhaseThreshold = Threshold;
	ApplyDraft(Candidate, false);
}

void UTunaSweeperBossLabWidget::OnAlternateChanged(bool bChecked)
{
	if (bRefreshing || !Session()) return;
	FTunaSweeperBossDefinition Candidate = Session()->GetDraft();
	Candidate.bAlternateWeapons = bChecked;
	ApplyDraft(Candidate, false);
}
