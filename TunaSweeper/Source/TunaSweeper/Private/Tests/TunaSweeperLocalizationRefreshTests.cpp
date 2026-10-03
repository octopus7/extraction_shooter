#if WITH_DEV_AUTOMATION_TESTS

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Game/TunaSweeperGameInstance.h"
#include "GameFramework/PlayerController.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Fonts/FontCache.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/SlateRenderer.h"
#include "UI/TunaSweeperUIFont.h"
#include "Subsystem/TunaSweeperAdditionalTranslations.h"
#include "HAL/FileManager.h"
#include "Internationalization/Internationalization.h"
#include "Internationalization/Culture.h"
#include "Settings/TunaSweeperLanguage.h"
#include "Settings/TunaSweeperBuildTargetSettings.h"
#include "Subsystem/TunaSweeperQuestSubsystem.h"
#include "Subsystem/TunaSweeperScenarioSubsystem.h"
#include "Subsystem/TunaSweeperTextSubsystem.h"
#include "UI/TunaSweeperGraphicsQualityRowWidget.h"
#include "UI/TunaSweeperGraphicsSettingsWidget.h"
#include "UI/TunaSweeperIntroMenuWidget.h"
#include "UI/TunaSweeperOptionRowWidget.h"
#include "UObject/StrongObjectPtr.h"

namespace TunaSweeperLocalizationRefreshTests
{
	constexpr EAutomationTestFlags TestFlags =
		EAutomationTestFlags::EditorContext |
		EAutomationTestFlags::ClientContext |
		EAutomationTestFlags::EngineFilter;

	struct FWorldContextAccess : UGameInstance
	{
		static void Attach(UGameInstance* Instance, FWorldContext* Context)
		{
			auto Member = &FWorldContextAccess::WorldContext;
			Instance->*Member = Context;
		}
	};

	UWidget* FindNestedWidget(UUserWidget* Root, FName Name)
	{
		if (!Root || !Root->WidgetTree)
		{
			return nullptr;
		}
		if (UWidget* Direct = Root->WidgetTree->FindWidget(Name))
		{
			return Direct;
		}

		UWidget* Result = nullptr;
		Root->WidgetTree->ForEachWidget([&](UWidget* Widget)
		{
			if (!Result)
			{
				Result = FindNestedWidget(Cast<UUserWidget>(Widget), Name);
			}
		});
		return Result;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTunaSweeperLocalizationRefreshTest,
	"TunaSweeper.UI.Localization.RefreshesTitleAndGraphicsLabels",
	TunaSweeperLocalizationRefreshTests::TestFlags)

bool FTunaSweeperLocalizationRefreshTest::RunTest(const FString& Parameters)
{
	const FString OriginalCulture = FInternationalization::Get().GetCurrentCulture()->GetName();
	ON_SCOPE_EXIT { FInternationalization::Get().SetCurrentCulture(OriginalCulture); };
	using namespace TunaSweeperLocalizationRefreshTests;
	(void)Parameters;

	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	if (!TestNotNull(TEXT("Transient world exists"), World)) return false;
	FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
	Context.SetCurrentWorld(World);
	TStrongObjectPtr<UTunaSweeperGameInstance> Instance(NewObject<UTunaSweeperGameInstance>(GEngine));
	FWorldContextAccess::Attach(Instance.Get(), &Context);
	Context.OwningGameInstance = Instance.Get();
	World->SetGameInstance(Instance.Get());
	Instance->UGameInstance::Init();
	ON_SCOPE_EXIT
	{
		Instance->UGameInstance::Shutdown();
		World->SetGameInstance(nullptr);
		Context.OwningGameInstance = nullptr;
		FWorldContextAccess::Attach(Instance.Get(), nullptr);
		World->DestroyWorld(false);
		GEngine->DestroyWorldContext(World);
		World->RemoveFromRoot();
	};

	World->InitializeActorsForPlay(FURL());
	APlayerController* Controller = World->SpawnActor<APlayerController>();
	if (!TestNotNull(TEXT("Controller exists"), Controller)) return false;
	ULocalPlayer* LocalPlayer = NewObject<ULocalPlayer>(GEngine);
	LocalPlayer->SetControllerId(0);
	Controller->SetPlayer(LocalPlayer);
	UTunaSweeperTextSubsystem* Strings = Instance->GetSubsystem<UTunaSweeperTextSubsystem>();
	if (!TestNotNull(TEXT("Localization subsystem exists"), Strings) ||
		!TestTrue(TEXT("Localization CSV loads"), Strings->LoadTextData())) return false;

	Instance->SetCurrentTextLanguage(ETunaSweeperItemTextLanguage::English, false);
	UClass* IntroClass = LoadClass<UTunaSweeperIntroMenuWidget>(
		nullptr,
		TEXT("/Game/UI/WBP_IntroMenu.WBP_IntroMenu_C"));
	if (!TestNotNull(TEXT("Authored intro menu class loads"), IntroClass)) return false;
	TStrongObjectPtr<UTunaSweeperIntroMenuWidget> Intro(
		CreateWidget<UTunaSweeperIntroMenuWidget>(Controller, IntroClass));
	if (!TestNotNull(TEXT("Authored intro menu is created"), Intro.Get())) return false;
	Intro->PrepareForPauseSettings();
	TSharedPtr<SWidget> IntroSlate = Intro->TakeWidget();

	for (const TPair<FName, FString>& Expected : {
		TPair<FName, FString>(TEXT("SettingsButtonText"), TEXT("Settings")),
		TPair<FName, FString>(TEXT("QuitButtonText"), TEXT("Quit")) })
	{
		UTextBlock* Label = Cast<UTextBlock>(FindNestedWidget(Intro.Get(), Expected.Key));
		if (!TestNotNull(*Expected.Key.ToString(), Label)) return false;
		TestEqual(TEXT("Title action label follows the selected language"), Label->GetText().ToString(), Expected.Value);
	}

	UTunaSweeperOptionRowWidget* LanguageRow = Cast<UTunaSweeperOptionRowWidget>(
		FindNestedWidget(Intro.Get(), TEXT("InterfaceLanguageOptionRow")));
	if (!TestNotNull(TEXT("Language selector exists"), LanguageRow)) return false;
	const TCHAR* LanguageNames[] = {TEXT("English"), TEXT("한국어"), TEXT("日本語"),
		TEXT("简体中文"), TEXT("繁體中文"), TEXT("Русский"), TEXT("Português (Brasil)")};
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(LanguageNames); ++Index)
	{
		LanguageRow->OnStepRequested.Broadcast(Index == 0 ? 0 : 1);
		UTextBlock* Value = Cast<UTextBlock>(FindNestedWidget(LanguageRow, TEXT("OptionValueText")));
		if (!TestNotNull(TEXT("Language value label exists"), Value)) return false;
		TestEqual(TEXT("Selector reaches each native language name"), Value->GetText().ToString(), FString(LanguageNames[Index]));
	}
	UButton* Next = Cast<UButton>(FindNestedWidget(LanguageRow, TEXT("NextButton")));
	if (!TestNotNull(TEXT("Language next button exists"), Next)) return false;
	TestFalse(TEXT("Last language disables next"), Next->GetIsEnabled());
	for (int32 Index = UE_ARRAY_COUNT(LanguageNames) - 2; Index >= 0; --Index)
	{
		LanguageRow->OnStepRequested.Broadcast(-1);
		UTextBlock* Value = Cast<UTextBlock>(FindNestedWidget(LanguageRow, TEXT("OptionValueText")));
		TestEqual(TEXT("Selector steps backward"), Value->GetText().ToString(), FString(LanguageNames[Index]));
	}

	Instance->SetCurrentTextLanguage(ETunaSweeperItemTextLanguage::Korean, false);
	TStrongObjectPtr<UTunaSweeperGraphicsSettingsWidget> Graphics(
		CreateWidget<UTunaSweeperGraphicsSettingsWidget>(Controller, UTunaSweeperGraphicsSettingsWidget::StaticClass()));
	if (!TestNotNull(TEXT("Graphics settings widget is created"), Graphics.Get())) return false;
	TSharedPtr<SWidget> GraphicsSlate = Graphics->TakeWidget();
	Instance->SetCurrentTextLanguage(ETunaSweeperItemTextLanguage::English, false);
	Graphics->RefreshFromSettings();

	UTunaSweeperGraphicsQualityRowWidget* TextureRow = Cast<UTunaSweeperGraphicsQualityRowWidget>(
		Graphics->WidgetTree->FindWidget(TEXT("TextureQualityRow")));
	if (!TestNotNull(TEXT("Texture quality row exists"), TextureRow)) return false;
	UTextBlock* TextureLabel = Cast<UTextBlock>(TextureRow->WidgetTree->FindWidget(TEXT("OptionLabelText")));
	if (!TestNotNull(TEXT("Texture quality label exists"), TextureLabel)) return false;
	TestEqual(TEXT("Graphics quality label refreshes after a language change"), TextureLabel->GetText().ToString(), FString(TEXT("Texture")));

	GraphicsSlate.Reset();
	IntroSlate.Reset();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTunaSweeperAdditionalLanguagesTest,
	"TunaSweeper.UI.Localization.AdditionalLanguages",
	TunaSweeperLocalizationRefreshTests::TestFlags)

bool FTunaSweeperAdditionalLanguagesTest::RunTest(const FString& Parameters)
{
	const UTunaSweeperTextSubsystem* Strings = GetDefault<UTunaSweeperTextSubsystem>();
	if (!TestTrue(TEXT("Text data loads"), Strings->LoadTextData(true))) return false;
	// Existing enum values 0-2 must remain stable for serialized references.
	const TCHAR* Expected[] = { TEXT("下车"), TEXT("下車"), TEXT("Выйти"), TEXT("Descer") };
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Expected); ++Index)
	{
		FText Text;
		TestTrue(TEXT("Additional language resolves"), Strings->TryGetTextByKey(
			TEXT("ui.vehicle.dismount"), static_cast<ETunaSweeperItemTextLanguage>(Index + 3), Text));
		TestEqual(TEXT("Selected translation is returned"), Text.ToString(), FString(Expected[Index]));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTunaSweeperLanguageCodesTest,
	"TunaSweeper.UI.Localization.CodesAndPersistence",
	TunaSweeperLocalizationRefreshTests::TestFlags)

bool FTunaSweeperLanguageCodesTest::RunTest(const FString& Parameters)
{
	const FString OriginalCulture = FInternationalization::Get().GetCurrentCulture()->GetName();
	ON_SCOPE_EXIT { FInternationalization::Get().SetCurrentCulture(OriginalCulture); };
	using namespace TunaSweeperLanguage;
	const TPair<const TCHAR*, ETunaSweeperItemTextLanguage> Cases[] = {
		{TEXT("zh-CN"), ETunaSweeperItemTextLanguage::SimplifiedChinese},
		{TEXT("zh_SG"), ETunaSweeperItemTextLanguage::SimplifiedChinese},
		{TEXT("zh"), ETunaSweeperItemTextLanguage::SimplifiedChinese},
		{TEXT("zh-TW"), ETunaSweeperItemTextLanguage::TraditionalChinese},
		{TEXT("zh-HK"), ETunaSweeperItemTextLanguage::TraditionalChinese},
		{TEXT("zh-MO"), ETunaSweeperItemTextLanguage::TraditionalChinese},
		{TEXT("zh-Hant-CN"), ETunaSweeperItemTextLanguage::TraditionalChinese},
		{TEXT("zh-Hans-HK"), ETunaSweeperItemTextLanguage::SimplifiedChinese},
		{TEXT(" RU-ru "), ETunaSweeperItemTextLanguage::Russian},
		{TEXT("pt-BR"), ETunaSweeperItemTextLanguage::BrazilianPortuguese},
		{TEXT(" PT_br "), ETunaSweeperItemTextLanguage::BrazilianPortuguese},
		{TEXT("pt"), ETunaSweeperItemTextLanguage::BrazilianPortuguese},
		{TEXT("ja-JP"), ETunaSweeperItemTextLanguage::Japanese},
		{TEXT("ko-KR"), ETunaSweeperItemTextLanguage::Korean},
		{TEXT("en-US"), ETunaSweeperItemTextLanguage::English}};
	for (const auto& Pair : Cases)
	{
		auto Parsed = ETunaSweeperItemTextLanguage::English;
		TestTrue(Pair.Key, TryParseLanguageCode(Pair.Key, Parsed));
		TestTrue(TEXT("Locale resolves to expected language"), Parsed == Pair.Value);
	}
	for (const TCHAR* Invalid : {TEXT(""), TEXT("fr-FR"), TEXT("rubbish"), TEXT("english"), TEXT("pt-PT")})
	{
		auto Parsed = ETunaSweeperItemTextLanguage::Korean;
		TestFalse(TEXT("Unsupported code is rejected"), TryParseLanguageCode(Invalid, Parsed));
		TestTrue(TEXT("Invalid code leaves output unchanged"), Parsed == ETunaSweeperItemTextLanguage::Korean);
	}
	const FString TestIni = FConfigCacheIni::NormalizeConfigIniPath(
		FPaths::CreateTempFilename(*FPaths::ProjectSavedDir(), TEXT("LanguageTest"), TEXT(".ini")));
	// SetString only writes registered config files in UE 5.7.
	GConfig->Add(TestIni, FConfigFile());
	TGuardValue<FString> IniGuard(GGameUserSettingsIni, TestIni);
	ON_SCOPE_EXIT { GConfig->UnloadFile(TestIni); IFileManager::Get().Delete(*TestIni); };
	TStrongObjectPtr<UTunaSweeperGameInstance> Instance(NewObject<UTunaSweeperGameInstance>());
	for (const auto Language : SupportedLanguages)
	{
		TestTrue(TEXT("Engine culture is available"), FInternationalization::Get().GetCulture(ToLanguageCode(Language)).IsValid());
		Instance->SetCurrentTextLanguage(Language, true);
		FConfigFile SavedConfig;
		SavedConfig.Read(TestIni);
		FString SavedCode;
		TestTrue(TEXT("Language is written to disk"), SavedConfig.GetString(SectionName, LanguageKey, SavedCode));
		TestEqual(TEXT("Saved code is canonical"), SavedCode, FString(ToLanguageCode(Language)));
		auto Restored = ETunaSweeperItemTextLanguage::English;
		TestTrue(TEXT("Saved language can be restored"), TryParseLanguageCode(SavedCode, Restored));
		TestTrue(TEXT("Restored language matches selection"), Restored == Language);
	}
	FTunaSweeperLocalizedTextString MissingTranslation;
	MissingTranslation.English = FText::FromString(TEXT("English fallback"));
	for (const auto Language : SupportedLanguages)
	{
		TestEqual(TEXT("Missing translation falls back to English"), Resolve(MissingTranslation, Language).ToString(), FString(TEXT("English fallback")));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTunaSweeperLanguageDatasetsTest,
	"TunaSweeper.UI.Localization.AllTextDatasets",
	TunaSweeperLocalizationRefreshTests::TestFlags)

bool FTunaSweeperLanguageDatasetsTest::RunTest(const FString& Parameters)
{
	const FString OriginalCulture = FInternationalization::Get().GetCurrentCulture()->GetName();
	ON_SCOPE_EXIT { FInternationalization::Get().SetCurrentCulture(OriginalCulture); };
	TGuardValue<ETunaSweeperBuildTarget> DemoGuard(GetMutableDefault<UTunaSweeperBuildTargetSettings>()->BuildTarget,
		ETunaSweeperBuildTarget::SteamDemo);
	TStrongObjectPtr<UTunaSweeperGameInstance> Instance(NewObject<UTunaSweeperGameInstance>());
	UTunaSweeperItemDataSubsystem* Items = NewObject<UTunaSweeperItemDataSubsystem>(Instance.Get());
	UTunaSweeperQuestSubsystem* Quests = NewObject<UTunaSweeperQuestSubsystem>(Instance.Get());
	UTunaSweeperScenarioSubsystem* Scenarios = NewObject<UTunaSweeperScenarioSubsystem>(Instance.Get());
	UTunaSweeperTextSubsystem* Strings = NewObject<UTunaSweeperTextSubsystem>(Instance.Get());
	if (!TestTrue(TEXT("Items load"), Items->LoadItemData()) ||
		!TestTrue(TEXT("Quests load"), Quests->LoadQuestData()) ||
		!TestTrue(TEXT("Scenarios load"), Scenarios->LoadScenarioData()) ||
		!TestTrue(TEXT("UI, memo and difficulty load"), Strings->LoadTextData())) return false;
	const TCHAR* ItemExpected[] = {TEXT("手枪"), TEXT("手槍"), TEXT("Пистолет"), TEXT("Pistola")};
	const TCHAR* QuestExpected[] = {TEXT("任务"), TEXT("任務"), TEXT("Задания"), TEXT("Missões")};
	const TCHAR* MemoExpected[] = {TEXT("另一片森林"), TEXT("另一片森林"), TEXT("Другой лес"), TEXT("Outra floresta")};
	const TCHAR* DifficultyExpected[] = {TEXT("搜集"), TEXT("蒐集"), TEXT("Сбор ресурсов"), TEXT("Coleta")};
	const TCHAR* SpeakerExpected[] = {TEXT("露娜"), TEXT("露娜"), TEXT("Луна"), TEXT("Luna")};
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(ItemExpected); ++Index)
	{
		const auto Language = static_cast<ETunaSweeperItemTextLanguage>(Index + 3);
		Instance->SetCurrentTextLanguage(Language, false);
		FText Text;
		TestTrue(TEXT("Item key resolves"), Items->TryGetItemTextByKey(TEXT("item.pistol"), Language, Text));
		TestEqual(TEXT("Item translation"), Text.ToString(), FString(ItemExpected[Index]));
		TestTrue(TEXT("Quest key resolves"), Quests->TryGetQuestTextByKey(TEXT("quest.ui.title"), Language, Text));
		TestEqual(TEXT("Quest translation"), Text.ToString(), FString(QuestExpected[Index]));
		TestTrue(TEXT("Memo key resolves"), Strings->TryGetTextByKey(TEXT("memo.1.title"), Language, Text));
		TestEqual(TEXT("Memo translation"), Text.ToString(), FString(MemoExpected[Index]));
		TestTrue(TEXT("Difficulty key resolves"), Strings->TryGetTextByKey(TEXT("difficulty.stage.1.title"), Language, Text));
		TestEqual(TEXT("Difficulty translation"), Text.ToString(), FString(DifficultyExpected[Index]));
		FTunaSweeperScenarioPresentation Presentation;
		TestTrue(TEXT("Scenario resolves"), Scenarios->TryResolveScenario(TEXT("interaction.mole"), TEXT("BunkerMap"), false, Presentation));
		if (!TestTrue(TEXT("Scenario has dialogue"), !Presentation.DialogueLines.IsEmpty())) return false;
		TestEqual(TEXT("Scenario translation"), Presentation.DialogueLines[0].SpeakerName.ToString(), FString(SpeakerExpected[Index]));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTunaSweeperTranslationCompatibilityTest,
	"TunaSweeper.UI.Localization.TranslationCompatibility",
	TunaSweeperLocalizationRefreshTests::TestFlags)

bool FTunaSweeperTranslationCompatibilityTest::RunTest(const FString& Parameters)
{
	const FString Directory = FPaths::ProjectSavedDir() / (TEXT("TranslationTest-") + FGuid::NewGuid().ToString());
	const FString TranslationDirectory = Directory / TEXT("Translations");
	const FString SourcePath = Directory / TEXT("Strings.csv");
	const FString TranslationPath = TranslationDirectory / TEXT("Strings.csv");
	IFileManager::Get().MakeDirectory(*TranslationDirectory, true);
	ON_SCOPE_EXIT
	{
		IFileManager::Get().Delete(*TranslationPath);
		IFileManager::Get().DeleteDirectory(*TranslationDirectory);
		IFileManager::Get().DeleteDirectory(*Directory);
	};
	FTunaSweeperAdditionalTranslations Translations;
	TestTrue(TEXT("Missing translation pack is optional"), Translations.Load(SourcePath));
	const TCHAR* CsvCases[] = {
		TEXT("string_key,zh-Hans,zh-Hant,ru\nkey,简,繁,Привет\n"),
		TEXT("string_key,zh-Hans,zh-Hant,ru,pt-BR\nkey,简,繁,Привет,\n"),
		TEXT("string_key,zh-Hans,zh-Hant,ru,pt-BR\nkey,简,繁,Привет,Olá\n")};
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(CsvCases); ++Index)
	{
		if (!TestTrue(TEXT("Fixture CSV written"), FFileHelper::SaveStringToFile(CsvCases[Index], *TranslationPath))) return false;
		TestTrue(TEXT("Legacy and extended translation packs load"), Translations.Load(SourcePath));
		FTunaSweeperLocalizedTextString Text;
		Text.English = FText::FromString(TEXT("Hello"));
		Translations.Apply(TEXT("key"), Text);
		TestEqual(TEXT("Portuguese missing-cell fallback"),
			TunaSweeperLanguage::Resolve(Text, ETunaSweeperItemTextLanguage::BrazilianPortuguese).ToString(),
			FString(Index == 2 ? TEXT("Olá") : TEXT("Hello")));
		TestEqual(TEXT("Existing translations remain available"), Text.Russian.ToString(), FString(TEXT("Привет")));
	}
	const FString Conflict = TEXT("string_key,zh-Hans,zh-Hant,ru,pt-BR\nkey,a,b,c,Olá\nkey,a,b,c,Outro\n");
	FFileHelper::SaveStringToFile(Conflict, *TranslationPath);
	AddExpectedError(TEXT("Conflicting translation key"), EAutomationExpectedErrorFlags::Contains, 1);
	TestFalse(TEXT("Conflicting Portuguese duplicates are rejected"), Translations.Load(SourcePath));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTunaSweeperLocalizedFontCoverageTest,
	"TunaSweeper.UI.Localization.FontCoverage",
	TunaSweeperLocalizationRefreshTests::TestFlags)

bool FTunaSweeperLocalizedFontCoverageTest::RunTest(const FString& Parameters)
{
	const TSharedRef<FSlateFontCache> FontCache = FSlateApplication::Get().GetRenderer()->GetFontCache();
	for (const ETunaSweeperUIFontWeight Weight : {ETunaSweeperUIFontWeight::Regular, ETunaSweeperUIFontWeight::Bold})
	{
		const FSlateFontInfo Font = TunaSweeperUIFont::MakeFont(nullptr, 20, Weight);
		for (const TCHAR Character : FString(TEXT("ÀÁÂÃÇÉÊÍÓÔÕÚàáâãçéêíóôõú한국日本简體Русский")))
		{
			float Scale = 1.0f;
			const FFontData& FontData = FontCache->GetFontDataForCodepoint(Font, Character, Scale);
			TestTrue(FString::Printf(TEXT("Font weight %d covers U+%04X"), static_cast<int32>(Weight), Character),
				FontCache->CanLoadCodepoint(FontData, Character, EFontFallback::FF_NoFallback));
		}
	}
	return true;
}

#endif
