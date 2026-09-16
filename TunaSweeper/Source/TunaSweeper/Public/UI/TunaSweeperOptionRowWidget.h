#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "TunaSweeperOptionRowWidget.generated.h"

class UButton;
class UTextBlock;

DECLARE_MULTICAST_DELEGATE_OneParam(FTunaSweeperOptionStepRequested, int32);

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API UTunaSweeperOptionRowWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void Configure(const FText& InLabel);
	void SetValue(const FText& InValue);
	void SetStepEnabled(bool bCanStepPrevious, bool bCanStepNext);

	FTunaSweeperOptionStepRequested OnStepRequested;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

	UPROPERTY(BlueprintReadOnly, Category="Option", meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> OptionLabelText;

	UPROPERTY(BlueprintReadOnly, Category="Option", meta=(BindWidgetOptional))
	TObjectPtr<UButton> PreviousButton;

	UPROPERTY(BlueprintReadOnly, Category="Option", meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> PreviousButtonText;

	UPROPERTY(BlueprintReadOnly, Category="Option", meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> OptionValueText;

	UPROPERTY(BlueprintReadOnly, Category="Option", meta=(BindWidgetOptional))
	TObjectPtr<UButton> NextButton;

	UPROPERTY(BlueprintReadOnly, Category="Option", meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> NextButtonText;

private:
	void BuildRuntimeWidgetTree();
	void ApplyPresentation();

	UFUNCTION()
	void HandlePreviousClicked();

	UFUNCTION()
	void HandleNextClicked();

	FText Label;
	FText Value;
	bool bPreviousEnabled = true;
	bool bNextEnabled = true;
};
