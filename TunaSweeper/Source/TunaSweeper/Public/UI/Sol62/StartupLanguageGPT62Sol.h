#pragma once

#include "Blueprint/UserWidget.h"
#include "Subsystem/TunaSweeperItemDataSubsystem.h"
#include "StartupLanguageGPT62Sol.generated.h"

class UButton;
class UTextBlock;

DECLARE_DELEGATE_OneParam(FStartupLanguageSelectedGPT62Sol, ETunaSweeperItemTextLanguage);

// Authored with gpt-6.2-sol, medium reasoning.
UCLASS()
class TUNASWEEPER_API UStartupLanguageGPT62Sol : public UUserWidget
{
	GENERATED_BODY()

public:
	FStartupLanguageSelectedGPT62Sol OnLanguageSelected;
	void EnableSelection();

protected:
	virtual void NativeOnInitialized() override;

private:
	void SelectLanguage(ETunaSweeperItemTextLanguage Language, UButton& Button);
	UFUNCTION() void ConfirmSelection();
	UFUNCTION() void SelectEnglish();
	UFUNCTION() void SelectJapanese();
	UFUNCTION() void SelectKorean();
	UFUNCTION() void SelectSimplifiedChinese();
	UFUNCTION() void SelectTraditionalChinese();
	UFUNCTION() void SelectRussian();

	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> PromptText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> EnglishButton;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> JapaneseButton;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> KoreanButton;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> SimplifiedChineseButton;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> TraditionalChineseButton;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> RussianButton;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> EnglishLabel;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> JapaneseLabel;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> KoreanLabel;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> SimplifiedChineseLabel;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> TraditionalChineseLabel;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> RussianLabel;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> ConfirmButton;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> ConfirmLabel;
	UPROPERTY(Transient) TObjectPtr<UButton> SelectedButton;
	TOptional<ETunaSweeperItemTextLanguage> SelectedLanguage;
};
