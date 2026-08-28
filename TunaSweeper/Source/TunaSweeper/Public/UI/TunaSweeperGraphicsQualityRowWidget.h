#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "Settings/TunaSweeperGraphicsSettingsTypes.h"
#include "TunaSweeperGraphicsQualityRowWidget.generated.h"

class UButton;
class UTextBlock;

DECLARE_MULTICAST_DELEGATE_TwoParams(
	FTunaSweeperGraphicsQualityStepRequested,
	ETunaSweeperScalabilityOption,
	int32);

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API UTunaSweeperGraphicsQualityRowWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void Configure(ETunaSweeperScalabilityOption InOption, const FText& InLabel);
	void SetQualityLevel(int32 InQualityLevel, const FText& InQualityText);
	ETunaSweeperScalabilityOption GetOption() const { return Option; }

	FTunaSweeperGraphicsQualityStepRequested OnQualityStepRequested;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	void BuildRuntimeWidgetTree();

	UPROPERTY(BlueprintReadOnly, Category="Graphics", meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> OptionLabelText;

	UPROPERTY(BlueprintReadOnly, Category="Graphics", meta=(BindWidgetOptional))
	TObjectPtr<UButton> PreviousButton;

	UPROPERTY(BlueprintReadOnly, Category="Graphics", meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> PreviousButtonText;

	UPROPERTY(BlueprintReadOnly, Category="Graphics", meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> QualityValueText;

	UPROPERTY(BlueprintReadOnly, Category="Graphics", meta=(BindWidgetOptional))
	TObjectPtr<UButton> NextButton;

	UPROPERTY(BlueprintReadOnly, Category="Graphics", meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> NextButtonText;

private:
	UFUNCTION()
	void HandlePreviousClicked();

	UFUNCTION()
	void HandleNextClicked();

	ETunaSweeperScalabilityOption Option = ETunaSweeperScalabilityOption::Texture;
	int32 QualityLevel = 0;
	FText Label;
};
