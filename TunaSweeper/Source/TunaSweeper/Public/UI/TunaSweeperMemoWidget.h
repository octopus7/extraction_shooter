#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "Memo/TunaSweeperMemoTypes.h"
#include "TunaSweeperMemoWidget.generated.h"

class UBorder;
class UButton;
class UHorizontalBox;
class UScrollBox;
class UTextBlock;
class UVerticalBox;

DECLARE_DELEGATE_OneParam(FTunaSweeperMemoEntryClickedDelegate, int32);

UCLASS()
class TUNASWEEPER_API UTunaSweeperMemoListEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void InitializeMemoEntry(
		int32 InMemoId,
		const FText& InLabel,
		bool bInAcquired,
		bool bInSelected,
		FTunaSweeperMemoEntryClickedDelegate InClickedDelegate);

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

	FTunaSweeperMemoEntryClickedDelegate ClickedDelegate;
	int32 MemoId = INDEX_NONE;
	FText Label;
	bool bAcquired = false;
	bool bSelected = false;
};

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API UTunaSweeperMemoWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Memo")
	void OpenMemo(int32 MemoId);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Memo")
	void RefreshMemoView();

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	void BuildMemoWidget();
	void RebuildMemoList(const TArray<FTunaSweeperMemoListEntry>& Entries);
	void ApplySelectedMemo(const TArray<FTunaSweeperMemoListEntry>& Entries);
	void SetSelectedMemoId(int32 MemoId);
	void HandleMemoStateChanged();

	UPROPERTY(Transient)
	TObjectPtr<UBorder> RootPanel;

	UPROPERTY(Transient)
	TObjectPtr<UHorizontalBox> RootColumns;

	UPROPERTY(Transient)
	TObjectPtr<UScrollBox> MemoListScrollBox;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> DetailTitleText;

	UPROPERTY(Transient)
	TObjectPtr<UScrollBox> DetailBodyScrollBox;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> DetailBodyText;

	int32 SelectedMemoId = INDEX_NONE;
};
