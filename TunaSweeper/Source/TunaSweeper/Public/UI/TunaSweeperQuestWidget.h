#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "Quest/TunaSweeperQuestTypes.h"
#include "TunaSweeperQuestWidget.generated.h"

class UBorder;
class UButton;
class UHorizontalBox;
class UScrollBox;
class UTextBlock;
class UVerticalBox;
class UTunaSweeperQuestSubsystem;

DECLARE_DELEGATE_OneParam(FTunaSweeperQuestEntryClickedDelegate, FName);

UCLASS()
class TUNASWEEPER_API UTunaSweeperQuestListEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void InitializeQuestEntry(
		FName InQuestId,
		const FText& InLabel,
		const FText& InStateLabel,
		bool bInSelected,
		FTunaSweeperQuestEntryClickedDelegate InClickedDelegate);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

private:
	void BuildEntryWidget();
	void RefreshEntryView();

	UFUNCTION()
	void HandleEntryClicked();

	UPROPERTY(Transient)
	TObjectPtr<UButton> EntryButton;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> EntryLabelText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> EntryStateText;

	FTunaSweeperQuestEntryClickedDelegate ClickedDelegate;
	FName QuestId = NAME_None;
	FText Label;
	FText StateLabel;
	bool bSelected = false;
};

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API UTunaSweeperQuestWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UTunaSweeperQuestWidget(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Quest")
	void InitializeQuest(FName InQuestId);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Quest")
	void RefreshQuestView();

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Quest")
	void ResetQuestSelection();

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

protected:
	enum class EQuestListFilter : uint8
	{
		Available,
		InProgress,
		RewardCompleted
	};

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TunaSweeper|Quest")
	bool bShowAvailableTab = true;

	virtual EQuestListFilter GetDefaultFilter() const;
	void NormalizeActiveFilter();

private:
	UFUNCTION()
	void HandleAvailableTabClicked();

	UFUNCTION()
	void HandleInProgressTabClicked();

	UFUNCTION()
	void HandleCompletedTabClicked();

	UFUNCTION()
	void HandlePrimaryButtonClicked();

	void BuildQuestWidget();
	bool CacheBuiltQuestWidgets();
	void RebuildQuestList(const TArray<FTunaSweeperQuestDefinition>& QuestDefinitions);
	void ApplySelectedQuest(const TArray<FTunaSweeperQuestDefinition>& QuestDefinitions);
	void SetSelectedQuestId(FName InQuestId);
	void SetActiveFilter(EQuestListFilter InFilter);
	FName GetSavedSelectedQuestId(EQuestListFilter Filter) const;
	void SetSavedSelectedQuestId(EQuestListFilter Filter, FName InQuestId);
	void HandleQuestProgressChanged();
	void UpdateTabButtonStates();
	void UpdateDetailView();
	void BuildFilteredQuestDefinitions(
		const UTunaSweeperQuestSubsystem& QuestSubsystem,
		TArray<FTunaSweeperQuestDefinition>& OutQuestDefinitions) const;
	bool IsQuestVisibleInActiveFilter(
		const UTunaSweeperQuestSubsystem& QuestSubsystem,
		const FTunaSweeperQuestDefinition& QuestDefinition) const;
	FText GetQuestText(FName StringKey, const FText& FallbackText = FText::GetEmpty()) const;
	FText GetStateText(FName InQuestId) const;
	FText GetPrimaryButtonText(FName InQuestId) const;
	FText GetEmptyListText() const;
	FText BuildObjectiveText(const UTunaSweeperQuestSubsystem& QuestSubsystem, FName InQuestId) const;
	FText BuildRewardText(const UTunaSweeperQuestSubsystem& QuestSubsystem, FName InQuestId) const;
	bool IsPrimaryButtonEnabled(FName InQuestId) const;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> RootPanel;

	UPROPERTY(Transient)
	TObjectPtr<UHorizontalBox> RootColumns;

	UPROPERTY(Transient)
	TObjectPtr<UScrollBox> QuestListScrollBox;

	UPROPERTY(Transient)
	TObjectPtr<UButton> AvailableTabButton;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> AvailableTabText;

	UPROPERTY(Transient)
	TObjectPtr<UButton> InProgressTabButton;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> InProgressTabText;

	UPROPERTY(Transient)
	TObjectPtr<UButton> CompletedTabButton;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> CompletedTabText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> DetailTitleText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> DetailDescriptionText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> DetailStateText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> DetailObjectiveHeaderText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> DetailObjectiveText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> DetailRewardHeaderText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> DetailRewardText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> DetailEmptyText;

	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> DetailStack;

	UPROPERTY(Transient)
	TObjectPtr<UButton> PrimaryButton;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> PrimaryButtonText;

	EQuestListFilter ActiveFilter = EQuestListFilter::Available;
	FName QuestId = NAME_None;
	FName AvailableSelectedQuestId = NAME_None;
	FName InProgressSelectedQuestId = NAME_None;
	FName CompletedSelectedQuestId = NAME_None;
};

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API UTunaSweeperMenuQuestWidget : public UTunaSweeperQuestWidget
{
	GENERATED_BODY()

public:
	UTunaSweeperMenuQuestWidget(const FObjectInitializer& ObjectInitializer);
};

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API UTunaSweeperInteractionQuestWidget : public UTunaSweeperQuestWidget
{
	GENERATED_BODY()

public:
	UTunaSweeperInteractionQuestWidget(const FObjectInitializer& ObjectInitializer);
};
