#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TunaSweeperQuestWidget.generated.h"

class UButton;
class UTextBlock;

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API UTunaSweeperQuestWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Quest")
	void InitializeQuest(FName InQuestId);

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Quest", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> QuestTitleText;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Quest", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> QuestDescriptionText;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Quest", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> QuestObjectiveText;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Quest", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> QuestRewardText;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Quest", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> QuestStateText;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Quest", meta = (BindWidgetOptional))
	TObjectPtr<UButton> QuestPrimaryButton;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Quest", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> QuestPrimaryButtonText;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Quest", meta = (BindWidgetOptional))
	TObjectPtr<UButton> QuestCloseButton;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Quest", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> QuestCloseButtonText;

private:
	UFUNCTION()
	void HandlePrimaryButtonClicked();

	UFUNCTION()
	void HandleCloseButtonClicked();

	void RefreshQuestView();
	FText GetStateText() const;
	FText GetPrimaryButtonText() const;
	bool IsPrimaryButtonEnabled() const;

	FName QuestId = NAME_None;
};
