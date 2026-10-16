#if WITH_DEV_AUTOMATION_TESTS
#include "TunaSweeperEditorText.h"
#include "Engine/GameInstance.h"
#include "Misc/AutomationTest.h"
#include "Subsystem/TunaSweeperTextSubsystem.h"
#include "UObject/GarbageCollection.h"
#include "UObject/UObjectIterator.h"

namespace TunaSweeperLevelOpenTool { FText MenuText(const TCHAR* Key); }
namespace TunaSweeperQuestCheatEditor { FText Text(const TCHAR* Key); }

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperEditorTextLifetimeTest,
	"TunaSweeper.Editor.TextDoesNotRetainGameInstances",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaSweeperEditorTextLifetimeTest::RunTest(const FString& Parameters)
{
	const TCHAR* Key = TEXT("ui.editor.open_level.raid_forest");
	TestFalse(TEXT("Level menu resolves its localized text"), TunaSweeperLevelOpenTool::MenuText(Key).IsEmpty());
	TestFalse(TEXT("Quest menu resolves its localized text"), TunaSweeperQuestCheatEditor::Text(TEXT("ui.quest_cheat.title")).IsEmpty());
	TestEqual(TEXT("World cheat menu resolves its string key"), TunaSweeperEditorText::Resolve(TEXT("ui.world_cheat.title"),
		ETunaSweeperItemTextLanguage::English).ToString(), FString(TEXT("World Progress Cheats")));
	const FText English = TunaSweeperEditorText::Resolve(Key, ETunaSweeperItemTextLanguage::English);
	const FText Korean = TunaSweeperEditorText::Resolve(Key, ETunaSweeperItemTextLanguage::Korean);
	TestNotEqual(TEXT("Language-specific cache preserves translations"), English.ToString(), Korean.ToString());
	TestNotEqual(TEXT("English resolves a CSV entry"), English.ToString(), FString(Key));
	TestNotEqual(TEXT("Korean resolves a CSV entry"), Korean.ToString(), FString(Key));

	TArray<TWeakObjectPtr<UGameInstance>> PreviewOwners;
	for (TObjectIterator<UTunaSweeperTextSubsystem> It; It; ++It)
	{
		UGameInstance* Owner = Cast<UGameInstance>(It->GetOuter());
		if (Owner && Owner->GetClass() == UGameInstance::StaticClass() && Owner->GetOuter() == GetTransientPackage())
		{
			PreviewOwners.AddUnique(Owner);
		}
	}
	CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
	for (const TWeakObjectPtr<UGameInstance>& Owner : PreviewOwners)
	{
		TestFalse(TEXT("Editor text must not retain a preview GameInstance across garbage collection"), Owner.IsValid());
	}
	TestEqual(TEXT("Cached text remains usable after its loader is collected"),
		TunaSweeperEditorText::Resolve(Key, ETunaSweeperItemTextLanguage::English).ToString(), English.ToString());
	return true;
}
#endif
