#if WITH_DEV_AUTOMATION_TESTS

#include "QuestDatasetSwitcher.h"

#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace TunaSweeperQuestDatasetTests
{
	constexpr EAutomationTestFlags TestFlags =
		EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTunaSweeperQuestDatasetNamespaceTest,
	"TunaSweeper.QuestDataset.Namespaces",
	TunaSweeperQuestDatasetTests::TestFlags)

bool FTunaSweeperQuestDatasetNamespaceTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FQuestDatasetDescriptor Descriptor;
	Descriptor.Kind = EQuestDatasetKind::Public;
	TestEqual(TEXT("Public namespace"), Descriptor.GetSaveNamespace(), FString(TEXT("Public")));

	Descriptor.Kind = EQuestDatasetKind::ProductionDemo;
	TestEqual(
		TEXT("Production demo namespace"),
		Descriptor.GetSaveNamespace(),
		FString(TEXT("ProductionDemo")));

	Descriptor.Kind = EQuestDatasetKind::ProductionRelease;
	TestEqual(
		TEXT("Production release namespace"),
		Descriptor.GetSaveNamespace(),
		FString(TEXT("ProductionRelease")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTunaSweeperActiveQuestDatasetTest,
	"TunaSweeper.QuestDataset.ActiveData",
	TunaSweeperQuestDatasetTests::TestFlags)

bool FTunaSweeperActiveQuestDatasetTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FQuestDatasetSwitcherModule& DatasetModule = FQuestDatasetSwitcherModule::Get();
	TestTrue(TEXT("Active dataset reload succeeds"), DatasetModule.ReloadActiveDataset());
	const FQuestDatasetDescriptor& Dataset = DatasetModule.GetActiveDataset();

	TestFalse(TEXT("Dataset id is set"), Dataset.DatasetId.IsNone());
	TestFalse(TEXT("Save compatibility id is set"), Dataset.SaveCompatibilityId.IsNone());
	TestTrue(TEXT("Quest definitions file exists"), FPaths::FileExists(Dataset.QuestDefinitionsPath));
	TestTrue(TEXT("Quest text file exists"), FPaths::FileExists(Dataset.QuestTextStringsPath));

	FString JsonText;
	if (!TestTrue(
		TEXT("Quest definitions can be read"),
		FFileHelper::LoadFileToString(JsonText, *Dataset.QuestDefinitionsPath)))
	{
		return false;
	}

	TArray<TSharedPtr<FJsonValue>> QuestValues;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonText);
	if (!TestTrue(
		TEXT("Quest definitions are valid JSON"),
		FJsonSerializer::Deserialize(Reader, QuestValues)))
	{
		return false;
	}

	if (Dataset.Kind == EQuestDatasetKind::Public)
	{
		TestEqual(TEXT("Public dataset starts with no quests"), QuestValues.Num(), 0);
		return true;
	}

	if (Dataset.Kind == EQuestDatasetKind::ProductionDemo)
	{
		TestEqual(TEXT("Production demo dataset starts with no quests"), QuestValues.Num(), 0);
		return true;
	}

	TestEqual(TEXT("Production release example contains three quests"), QuestValues.Num(), 3);
	TSet<FString> QuestIds;
	int32 PrerequisiteCount = 0;
	for (const TSharedPtr<FJsonValue>& QuestValue : QuestValues)
	{
		const TSharedPtr<FJsonObject> QuestObject = QuestValue.IsValid()
			? QuestValue->AsObject()
			: nullptr;
		if (!TestTrue(TEXT("Quest entry is an object"), QuestObject.IsValid()))
		{
			continue;
		}

		FString QuestId;
		TestTrue(TEXT("Quest id exists"), QuestObject->TryGetStringField(TEXT("quest_id"), QuestId));
		TestFalse(TEXT("Quest id is unique"), QuestIds.Contains(QuestId));
		QuestIds.Add(QuestId);
	}

	for (const TSharedPtr<FJsonValue>& QuestValue : QuestValues)
	{
		const TSharedPtr<FJsonObject> QuestObject = QuestValue.IsValid()
			? QuestValue->AsObject()
			: nullptr;
		if (!QuestObject.IsValid())
		{
			continue;
		}

		const TArray<TSharedPtr<FJsonValue>>* PrerequisiteValues = nullptr;
		if (!QuestObject->TryGetArrayField(TEXT("required_completed_quest_ids"), PrerequisiteValues))
		{
			continue;
		}

		for (const TSharedPtr<FJsonValue>& PrerequisiteValue : *PrerequisiteValues)
		{
			const FString PrerequisiteId = PrerequisiteValue.IsValid()
				? PrerequisiteValue->AsString()
				: FString();
			TestTrue(TEXT("Prerequisite points to a quest in the active dataset"), QuestIds.Contains(PrerequisiteId));
			++PrerequisiteCount;
		}
	}

	TestEqual(TEXT("Three-quest example is a single chain"), PrerequisiteCount, 2);
	return true;
}

#endif
