#include "UI/Sol62/StartupLanguageGPT62Sol.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Game/TunaSweeperGameInstance.h"

void UStartupLanguageGPT62Sol::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	const auto* Game = Cast<UTunaSweeperGameInstance>(GetGameInstance());
	if (!Game)
	{
		return;
	}
	const auto Localized = [Game](FName Key)
	{
		return Game->ResolveLocalizedText(Key, FText::GetEmpty());
	};

	EnglishLabel->SetText(Localized(TEXT("ui.language.english")));
	JapaneseLabel->SetText(Localized(TEXT("ui.language.japanese")));
	KoreanLabel->SetText(Localized(TEXT("ui.language.korean")));
	SimplifiedChineseLabel->SetText(Localized(TEXT("ui.language.simplified_chinese")));
	TraditionalChineseLabel->SetText(Localized(TEXT("ui.language.traditional_chinese")));
	RussianLabel->SetText(Localized(TEXT("ui.language.russian")));
	PromptText->SetText(Localized(TEXT("ui.startup.language.prompt")));
	ConfirmLabel->SetText(Localized(TEXT("ui.common.ok")));

	EnglishButton->OnClicked.AddUniqueDynamic(this, &ThisClass::SelectEnglish);
	JapaneseButton->OnClicked.AddUniqueDynamic(this, &ThisClass::SelectJapanese);
	KoreanButton->OnClicked.AddUniqueDynamic(this, &ThisClass::SelectKorean);
	SimplifiedChineseButton->OnClicked.AddUniqueDynamic(this, &ThisClass::SelectSimplifiedChinese);
	TraditionalChineseButton->OnClicked.AddUniqueDynamic(this, &ThisClass::SelectTraditionalChinese);
	RussianButton->OnClicked.AddUniqueDynamic(this, &ThisClass::SelectRussian);
	ConfirmButton->OnClicked.AddUniqueDynamic(this, &ThisClass::ConfirmSelection);
	ConfirmButton->SetIsEnabled(false);
}

void UStartupLanguageGPT62Sol::EnableSelection()
{
	SetIsEnabled(true);
	(SelectedButton ? SelectedButton.Get() : EnglishButton.Get())->SetKeyboardFocus();
}

void UStartupLanguageGPT62Sol::SelectLanguage(ETunaSweeperItemTextLanguage Language, UButton& Button)
{
	if (!GetIsEnabled())
	{
		return;
	}
	if (SelectedButton == &Button)
	{
		ConfirmSelection();
		return;
	}
	if (SelectedButton)
	{
		SelectedButton->SetBackgroundColor(FLinearColor::White);
	}
	SelectedLanguage = Language;
	SelectedButton = &Button;
	Button.SetBackgroundColor(FLinearColor(0.82f, 1.40f, 1.95f));
	Button.SetKeyboardFocus();
	ConfirmButton->SetIsEnabled(true);
}

void UStartupLanguageGPT62Sol::ConfirmSelection()
{
	if (GetIsEnabled() && SelectedLanguage.IsSet())
	{
		SetIsEnabled(false);
		OnLanguageSelected.ExecuteIfBound(SelectedLanguage.GetValue());
	}
}

void UStartupLanguageGPT62Sol::SelectEnglish()
{
	SelectLanguage(ETunaSweeperItemTextLanguage::English, *EnglishButton);
}

void UStartupLanguageGPT62Sol::SelectJapanese()
{
	SelectLanguage(ETunaSweeperItemTextLanguage::Japanese, *JapaneseButton);
}

void UStartupLanguageGPT62Sol::SelectKorean()
{
	SelectLanguage(ETunaSweeperItemTextLanguage::Korean, *KoreanButton);
}

void UStartupLanguageGPT62Sol::SelectSimplifiedChinese()
{
	SelectLanguage(ETunaSweeperItemTextLanguage::SimplifiedChinese, *SimplifiedChineseButton);
}

void UStartupLanguageGPT62Sol::SelectTraditionalChinese()
{
	SelectLanguage(ETunaSweeperItemTextLanguage::TraditionalChinese, *TraditionalChineseButton);
}

void UStartupLanguageGPT62Sol::SelectRussian()
{
	SelectLanguage(ETunaSweeperItemTextLanguage::Russian, *RussianButton);
}
