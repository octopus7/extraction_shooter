#if WITH_DEV_AUTOMATION_TESTS

#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Engine/GameInstance.h"
#include "Misc/AutomationTest.h"
#include "Interaction/TunaSweeperResearchStationActor.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Subsystem/TunaSweeperResearchSubsystem.h"

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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTunaSweeperResearchInteractionDefaultsTest,
	"TunaSweeper.Research.InteractionDefaults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaSweeperResearchInteractionDefaultsTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	const ATunaSweeperResearchStationActor* Defaults = GetDefault<ATunaSweeperResearchStationActor>();
	TestNotNull(TEXT("Research station CDO"), Defaults);
	if (!Defaults)
	{
		return false;
	}

	TestNotNull(TEXT("Research station has an interactable component"), Defaults->GetInteractableComponent());
	TestEqual(TEXT("Research station interaction type"), Defaults->GetInteractionType(), ETunaSweeperInteractionType::Research);
	TestFalse(TEXT("Research station has a visible interaction label"), Defaults->GetInteractionDisplayName().IsEmpty());
	UClass* BlueprintClass = LoadClass<ATunaSweeperResearchStationActor>(
		nullptr,
		TEXT("/Game/Interaction/BP_ResearchSinkInteraction.BP_ResearchSinkInteraction_C"));
	TestNotNull(TEXT("Placeable research sink Blueprint loads"), BlueprintClass);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTunaSweeperResearchDeferredInitializationNotificationsTest,
	"TunaSweeper.Research.DeferredInitializationNotifications",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaSweeperResearchDeferredInitializationNotificationsTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	UGameInstance* GameInstance = NewObject<UGameInstance>();
	TestNotNull(TEXT("Game instance outer"), GameInstance);
	UTunaSweeperResearchSubsystem* ResearchSubsystem =
		NewObject<UTunaSweeperResearchSubsystem>(GameInstance);
	TestNotNull(TEXT("Research subsystem instance"), ResearchSubsystem);
	if (!ResearchSubsystem)
	{
		return false;
	}

	int32 EffectsNotificationCount = 0;
	int32 StateNotificationCount = 0;
	ResearchSubsystem->OnResearchEffectsChanged.AddLambda([&EffectsNotificationCount]()
	{
		++EffectsNotificationCount;
	});
	ResearchSubsystem->OnResearchStateChanged.AddLambda([&StateNotificationCount]()
	{
		++StateNotificationCount;
	});

	ResearchSubsystem->ResetResearchProgressForNewGame(
		ETunaSweeperResearchNotificationMode::Deferred);
	ResearchSubsystem->ResetResearchProgressForNewGame(
		ETunaSweeperResearchNotificationMode::Deferred);
	TestEqual(TEXT("Deferred effects notification is suppressed"), EffectsNotificationCount, 0);
	TestEqual(TEXT("Deferred state notification is suppressed"), StateNotificationCount, 0);

	ResearchSubsystem->FlushDeferredResearchNotifications();
	TestEqual(TEXT("Deferred effects notifications are coalesced"), EffectsNotificationCount, 1);
	TestEqual(TEXT("Deferred state notifications are coalesced"), StateNotificationCount, 1);

	ResearchSubsystem->FlushDeferredResearchNotifications();
	TestEqual(TEXT("Second flush has no effects notification"), EffectsNotificationCount, 1);
	TestEqual(TEXT("Second flush has no state notification"), StateNotificationCount, 1);

	ResearchSubsystem->ResetResearchProgressForNewGame(
		ETunaSweeperResearchNotificationMode::Immediate);
	TestEqual(TEXT("Immediate effects notification remains immediate"), EffectsNotificationCount, 2);
	TestEqual(TEXT("Immediate state notification remains immediate"), StateNotificationCount, 2);
	return true;
}

#endif
