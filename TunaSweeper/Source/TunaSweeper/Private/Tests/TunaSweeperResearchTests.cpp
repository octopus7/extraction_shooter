#if WITH_DEV_AUTOMATION_TESTS

#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTunaSweeperResearchJsonContractTest,
	"TunaSweeper.Research.JsonContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaSweeperResearchJsonContractTest::RunTest(const FString& Parameters)
{
	FString Json;
	const FString Path = FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Data/StatResearchNodes.json"));
	TestTrue(TEXT("Research JSON exists"), FFileHelper::LoadFileToString(Json, *Path));
	TArray<TSharedPtr<FJsonValue>> Values;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);
	TestTrue(TEXT("Research JSON parses as an array"), FJsonSerializer::Deserialize(Reader, Values));
	TSet<FString> NodeIds;
	TMap<int32, int32> RowCounts;
	int32 InitialNodeCount = 0;
	int32 MaximumDurationSeconds = 0;
	for (const TSharedPtr<FJsonValue>& Value : Values)
	{
		const TSharedPtr<FJsonObject> Object = Value.IsValid() ? Value->AsObject() : nullptr;
		if (!Object.IsValid()) { AddError(TEXT("Every research entry must be an object.")); continue; }
		const FString NodeId = Object->GetStringField(TEXT("node_id"));
		TestFalse(FString::Printf(TEXT("Node ID is unique: %s"), *NodeId), NodeIds.Contains(NodeId));
		NodeIds.Add(NodeId);
		const int32 Row = FMath::RoundToInt(Object->GetNumberField(TEXT("row")));
		const int32 Required = FMath::RoundToInt(Object->GetNumberField(TEXT("required_applied_node_count")));
		const int32 Duration = FMath::RoundToInt(Object->GetNumberField(TEXT("duration_seconds")));
		++RowCounts.FindOrAdd(Row);
		if (Required == 0) ++InitialNodeCount;
		MaximumDurationSeconds = FMath::Max(MaximumDurationSeconds, Duration);
		TestTrue(FString::Printf(TEXT("Duration is within 1..3600: %s"), *NodeId), Duration >= 1 && Duration <= 3600);
	}
	for (const TPair<int32, int32>& Pair : RowCounts)
	{
		TestTrue(FString::Printf(TEXT("Row %d has at most three nodes"), Pair.Key), Pair.Value <= 3);
	}
	TestTrue(TEXT("At least one node is initially available"), InitialNodeCount > 0);
	TestEqual(TEXT("Final research duration reaches one hour"), MaximumDurationSeconds, 3600);
	return true;
}

#endif
