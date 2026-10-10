#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Subsystem/TunaSweeperQuestSubsystem.h"
#include "Settings/TunaSweeperLanguage.h"
#include "Engine/GameInstance.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperQuestTranslationTest,
    "TunaSweeper.Quest.KoreanOnlyPreview",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaSweeperQuestTranslationTest::RunTest(const FString& Parameters)
{
    TStrongObjectPtr<UGameInstance> Game(NewObject<UGameInstance>());
    TStrongObjectPtr<UTunaSweeperQuestSubsystem> Quests(NewObject<UTunaSweeperQuestSubsystem>(Game.Get()));
    if (!TestTrue(TEXT("Editor loads quest CSV including untranslated rows"), Quests->LoadQuestTextStringsCsv())) return false;
    if (!TestTrue(TEXT("Preview format exists in the project string table"), Quests->QuestTextStringsByKey.Contains(TEXT("ui.quest.editor_korean_only_format")))) return false;
    FTunaSweeperQuestTextString Row;
    Row.StringKey = TEXT("test.quest.preview");
    Row.Korean = FText::FromString(TEXT("Korean test"));
    Quests->QuestTextStringsByKey.Add(Row.StringKey, Row);
    for (const auto Language : TunaSweeperLanguage::SupportedLanguages)
    {
        FText Result;
        TestTrue(TEXT("Korean-only row resolves for every editor language"), Quests->TryGetQuestTextByKey(Row.StringKey, Language, Result));
        TestEqual(TEXT("Exactly one K prefix on returned quest text"), Result.ToString(), FString(TEXT("KKorean test")));
    }
    TestEqual(TEXT("Stored Korean source is unchanged"), Quests->QuestTextStringsByKey[Row.StringKey].Korean.ToString(), FString(TEXT("Korean test")));
    Row.English = FText::FromString(TEXT("English test"));
    Row.Japanese = FText::FromString(TEXT("Japanese test"));
    Quests->QuestTextStringsByKey[Row.StringKey] = Row;
    FText Result;
    Quests->TryGetQuestTextByKey(Row.StringKey, ETunaSweeperItemTextLanguage::Korean, Result);
    TestEqual(TEXT("Translated row has no marker"), Result.ToString(), FString(TEXT("Korean test")));
    Quests->TryGetQuestTextByKey(Row.StringKey, ETunaSweeperItemTextLanguage::English, Result);
    TestEqual(TEXT("English translation retained"), Result.ToString(), FString(TEXT("English test")));
    TestFalse(TEXT("Missing key still fails"), Quests->TryGetQuestTextByKey(TEXT("test.missing"), ETunaSweeperItemTextLanguage::Korean, Result));
    TestTrue(TEXT("Missing key returns empty text"), Result.IsEmpty());
    return true;
}
#endif
