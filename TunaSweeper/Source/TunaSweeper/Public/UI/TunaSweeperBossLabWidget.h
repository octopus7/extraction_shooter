#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BossLab/TunaSweeperBossDefinition.h"
#include "Components/ComboBoxString.h"
#include "Components/EditableTextBox.h"
#include "TunaSweeperBossLabWidget.generated.h"

class UBorder;
class UButton;
class UCanvasPanel;
class UCheckBox;
class UEditableTextBox;
class USpinBox;
class UTextBlock;
class UVerticalBox;
class UTunaSweeperBossLabSubsystem;
class UTunaSweeperBossLabWidget;
class ATunaSweeperBossLabGameMode;

enum class ETunaSweeperBossLabAction : uint8
{
	None, Refresh, New, Load, Save, Duplicate, Delete, OpenFolder, ReturnTitle,
	Play, AddPart, RemoveBranch, RotateLeft, RotateRight, Confirm, Cancel
};

/** UUserWidget keeps generated dropdown rows alive while Slate owns their content. */
UCLASS()
class TUNASWEEPER_API UTunaSweeperBossLabComboLabel : public UUserWidget
{
	GENERATED_BODY()
public:
	void SetLabelText(const FText& Text);
protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
private:
	FText LabelText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> Label;
};

/** Keeps native button delegates bound to a GC-owned receiver. */
UCLASS()
class TUNASWEEPER_API UTunaSweeperBossLabButtonAction : public UObject
{
	GENERATED_BODY()
public:
	TWeakObjectPtr<UTunaSweeperBossLabWidget> Owner;
	ETunaSweeperBossLabAction Action = ETunaSweeperBossLabAction::None;
	UFUNCTION() void Execute();
};

/** Local boss library, assembly controls, and single-player launch surface. */
UCLASS()
class TUNASWEEPER_API UTunaSweeperBossLabWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	void RefreshFromSession();
	void HandleAction(ETunaSweeperBossLabAction Action);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual FReply NativeOnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;

private:
	friend class FTunaSweeperBossLabWidgetTest;
	void BuildWidgetTree();
	UTextBlock* MakeText(const FText& Text, int32 Size = 14);
	UTextBlock* AddLabel(UVerticalBox* Parent, const TCHAR* Key, int32 Size = 14);
	UButton* AddButton(UVerticalBox* Parent, const TCHAR* Key, ETunaSweeperBossLabAction Action, bool bDanger = false);
	UComboBoxString* AddCombo(UVerticalBox* Parent, const TCHAR* Key);
	USpinBox* AddNumber(UVerticalBox* Parent, const TCHAR* Key, float Minimum, float Maximum, float Step);
	FText Text(const TCHAR* Key) const;
	FText Resolve(FName Key) const;
	FText PartText(const FTunaSweeperBossPart& Part) const;
	UTunaSweeperBossLabSubsystem* Session() const;
	ATunaSweeperBossLabGameMode* LabMode() const;
	void RefreshLibrary();
	void RefreshDraftControls();
	void RefreshSummary();
	void RefreshPreview();
	void SetStatus(const FText& Message, bool bError = false);
	void SetStatusKey(const TCHAR* Key, bool bError = false);
	void RequestConfirmation(ETunaSweeperBossLabAction Action, const TCHAR* Key);
	void CloseConfirmation();
	void ExecuteAction(ETunaSweeperBossLabAction Action, bool bConfirmed);
	bool ApplyDraft(const FTunaSweeperBossDefinition& Candidate, bool bRefreshControls = true);
	void LoadSelectedSlot();
	void AddPart();

	UFUNCTION() UWidget* GenerateComboLabel(FString Option);
	UFUNCTION() void OnSlotChanged(FString Option, ESelectInfo::Type SelectionType);
	UFUNCTION() void OnPartChanged(FString Option, ESelectInfo::Type SelectionType);
	UFUNCTION() void OnTacticChanged(FString Option, ESelectInfo::Type SelectionType);
	UFUNCTION() void OnNameCommitted(const FText& Value, ETextCommit::Type CommitMethod);
	UFUNCTION() void OnIntervalChanged(float Value);
	UFUNCTION() void OnPhaseChanged(float Value);
	UFUNCTION() void OnAlternateChanged(bool bChecked);

	UPROPERTY(Transient) TObjectPtr<UCanvasPanel> WorkingLayer;
	UPROPERTY(Transient) TObjectPtr<UBorder> EditorPanel;
	UPROPERTY(Transient) TObjectPtr<UVerticalBox> LibraryEditActions;
	UPROPERTY(Transient) TObjectPtr<UBorder> ConfirmationLayer;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> ConfirmationText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> TitleText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> HintText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> SummaryText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> StatusText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> PlayText;
	UPROPERTY(Transient) TObjectPtr<UButton> PlayButton;
	UPROPERTY(Transient) TObjectPtr<UButton> LoadButton;
	UPROPERTY(Transient) TObjectPtr<UButton> DeleteButton;
	UPROPERTY(Transient) TObjectPtr<UComboBoxString> SlotCombo;
	UPROPERTY(Transient) TObjectPtr<UComboBoxString> PartCombo;
	UPROPERTY(Transient) TObjectPtr<UComboBoxString> ParentCombo;
	UPROPERTY(Transient) TObjectPtr<UComboBoxString> ModuleCombo;
	UPROPERTY(Transient) TObjectPtr<UComboBoxString> SocketCombo;
	UPROPERTY(Transient) TObjectPtr<UComboBoxString> YawCombo;
	UPROPERTY(Transient) TObjectPtr<UComboBoxString> TacticCombo;
	UPROPERTY(Transient) TObjectPtr<UEditableTextBox> NameInput;
	UPROPERTY(Transient) TObjectPtr<USpinBox> IntervalInput;
	UPROPERTY(Transient) TObjectPtr<USpinBox> PhaseInput;
	UPROPERTY(Transient) TObjectPtr<UCheckBox> AlternateInput;
	UPROPERTY(Transient) TArray<TObjectPtr<UTunaSweeperBossLabButtonAction>> ButtonActions;

	TMap<TWeakObjectPtr<UTextBlock>, FName> StaticLabels;
	TArray<int32> PartIds;
	TArray<FName> ModuleIds;
	int32 SelectedPartId = 1;
	int32 LoadedSlot = INDEX_NONE;
	ETunaSweeperBossLabAction PendingAction = ETunaSweeperBossLabAction::None;
	bool bRefreshing = false;
};
