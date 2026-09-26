#if WITH_DEV_AUTOMATION_TESTS

#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Engine/GameInstance.h"
#include "HAL/FileManager.h"
#include "Misc/AutomationTest.h"
#include "Interaction/TunaSweeperInteractableComponent.h"
#include "Interaction/TunaSweeperResearchStationActor.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/Csv/CsvParser.h"
#include "Subsystem/TunaSweeperResearchSubsystem.h"
#include "Subsystem/TunaSweeperItemDataSubsystem.h"
#include "Subsystem/TunaSweeperTextSubsystem.h"
#include "UObject/UnrealType.h"

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
	TSet<FString> UiTextKeys;
	FString UiTextCsv;
	const FString UiTextCsvPath = FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Data/UITextStrings.csv"));
	if (TestTrue(TEXT("UI text CSV exists"), FFileHelper::LoadFileToString(UiTextCsv, *UiTextCsvPath)))
	{
		const FCsvParser CsvParser(UiTextCsv);
		const FCsvParser::FRows& CsvRows = CsvParser.GetRows();
		if (TestTrue(TEXT("UI text CSV includes its header"), CsvRows.Num() > 0))
		{
			const TArray<const TCHAR*>& Header = CsvRows[0];
			TestTrue(TEXT("UI text CSV uses string_key,ko,en,ja"),
				Header.Num() >= 4 &&
				FString(Header[0]).Equals(TEXT("string_key"), ESearchCase::IgnoreCase) &&
				FString(Header[1]).Equals(TEXT("ko"), ESearchCase::IgnoreCase) &&
				FString(Header[2]).Equals(TEXT("en"), ESearchCase::IgnoreCase) &&
				FString(Header[3]).Equals(TEXT("ja"), ESearchCase::IgnoreCase));
		}
		for (int32 RowIndex = 1; RowIndex < CsvRows.Num(); ++RowIndex)
		{
			const TArray<const TCHAR*>& Row = CsvRows[RowIndex];
			if (Row.Num() >= 4 && !FString(Row[0]).TrimStartAndEnd().IsEmpty() &&
				!FString(Row[1]).TrimStartAndEnd().IsEmpty() &&
				!FString(Row[2]).TrimStartAndEnd().IsEmpty() &&
				!FString(Row[3]).TrimStartAndEnd().IsEmpty())
			{
				UiTextKeys.Add(FString(Row[0]).TrimStartAndEnd());
			}
		}
	}
	TSet<FString> ResearchTextKeys;
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

		FString DisplayNameStringKey;
		FString DescriptionStringKey;
		TestTrue(FString::Printf(TEXT("Display-name string key exists: %s"), *NodeId),
			Object->TryGetStringField(TEXT("display_name_string_key"), DisplayNameStringKey) && !DisplayNameStringKey.TrimStartAndEnd().IsEmpty());
		TestTrue(FString::Printf(TEXT("Description string key exists: %s"), *NodeId),
			Object->TryGetStringField(TEXT("description_string_key"), DescriptionStringKey) && !DescriptionStringKey.TrimStartAndEnd().IsEmpty());
		DisplayNameStringKey.TrimStartAndEndInline();
		DescriptionStringKey.TrimStartAndEndInline();
		TestTrue(FString::Printf(TEXT("Display-name key resolves through UI text CSV: %s"), *NodeId), UiTextKeys.Contains(DisplayNameStringKey));
		TestTrue(FString::Printf(TEXT("Description key resolves through UI text CSV: %s"), *NodeId), UiTextKeys.Contains(DescriptionStringKey));
		TestFalse(FString::Printf(TEXT("Display-name key is unique: %s"), *DisplayNameStringKey), ResearchTextKeys.Contains(DisplayNameStringKey));
		ResearchTextKeys.Add(DisplayNameStringKey);
		TestFalse(FString::Printf(TEXT("Description key is unique: %s"), *DescriptionStringKey), ResearchTextKeys.Contains(DescriptionStringKey));
		ResearchTextKeys.Add(DescriptionStringKey);
		TestFalse(FString::Printf(TEXT("Legacy Korean display-name text was removed: %s"), *NodeId), Object->HasField(TEXT("display_name_ko")));
		TestFalse(FString::Printf(TEXT("Legacy English display-name text was removed: %s"), *NodeId), Object->HasField(TEXT("display_name_en")));
		TestFalse(FString::Printf(TEXT("Legacy Korean description text was removed: %s"), *NodeId), Object->HasField(TEXT("description_ko")));
		TestFalse(FString::Printf(TEXT("Legacy English description text was removed: %s"), *NodeId), Object->HasField(TEXT("description_en")));
	}
	for (const TPair<int32, int32>& Pair : RowCounts)
	{
		TestTrue(FString::Printf(TEXT("Row %d has at most three nodes"), Pair.Key), Pair.Value <= 3);
	}
	TestTrue(TEXT("At least one node is initially available"), InitialNodeCount > 0);
	TestEqual(TEXT("Final research duration reaches one hour"), MaximumDurationSeconds, 3600);
	TestEqual(TEXT("Every research node owns one display-name and one description key"), ResearchTextKeys.Num(), Values.Num() * 2);

	UGameInstance* GameInstance = NewObject<UGameInstance>();
	UTunaSweeperTextSubsystem* TextSubsystem = NewObject<UTunaSweeperTextSubsystem>(GameInstance);
	if (TestNotNull(TEXT("Common UI text subsystem instance"), TextSubsystem) &&
		TestTrue(TEXT("Common UI text subsystem loads research strings"), TextSubsystem->LoadTextData(true)))
	{
		for (const FString& ResearchTextKey : ResearchTextKeys)
		{
			for (const ETunaSweeperItemTextLanguage Language : {
				ETunaSweeperItemTextLanguage::Korean,
				ETunaSweeperItemTextLanguage::English,
				ETunaSweeperItemTextLanguage::Japanese })
			{
				FText ResolvedText;
				TestTrue(
					FString::Printf(TEXT("Common UI text subsystem resolves research key %s for language %d"), *ResearchTextKey, static_cast<int32>(Language)),
					TextSubsystem->TryGetTextByKey(FName(*ResearchTextKey), Language, ResolvedText) && !ResolvedText.IsEmpty());
			}
		}
	}
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
	const FNameProperty* LabelKeyProperty = FindFProperty<FNameProperty>(
		UTunaSweeperInteractableComponent::StaticClass(), TEXT("InteractionDisplayNameStringKey"));
	if (TestNotNull(TEXT("Research station interaction has a string-key property"), LabelKeyProperty))
	{
		TestEqual(TEXT("Research station interaction uses the localized text key"),
			LabelKeyProperty->GetPropertyValue_InContainer(Defaults->GetInteractableComponent()),
			FName(TEXT("ui.interaction.research")));
	}
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTunaSweeperResearchNewGameClockResetTest,
	"TunaSweeper.Research.NewGameClockReset",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaSweeperResearchNewGameClockResetTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	UGameInstance* GameInstance = NewObject<UGameInstance>();
	UTunaSweeperResearchSubsystem* Research = NewObject<UTunaSweeperResearchSubsystem>(GameInstance);
	const int64 FutureTicks = (FDateTime::UtcNow() + FTimespan::FromDays(7)).GetTicks();
	Research->LoadResearchProgressFromSave({}, {}, FutureTicks);
	Research->ResetResearchProgressForNewGame();

	TArray<FName> Applied;
	TArray<FTunaSweeperActiveResearchSaveData> Active;
	int64 ExportedTicks = 0;
	Research->ExportResearchProgressForSave(Applied, Active, ExportedTicks);
	TestTrue(TEXT("New game clock does not inherit a future save slot clock"),
		ExportedTicks < (FDateTime::UtcNow() + FTimespan::FromMinutes(1)).GetTicks());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTunaSweeperResearchUnknownProgressRoundTripTest,
	"TunaSweeper.Research.UnknownProgressRoundTrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaSweeperResearchUnknownProgressRoundTripTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	UGameInstance* GameInstance = NewObject<UGameInstance>();
	UTunaSweeperResearchSubsystem* Research = NewObject<UTunaSweeperResearchSubsystem>(GameInstance);
	const FName UnavailableApplied(TEXT("research_saved_but_definition_unavailable"));
	const FName UnavailableActive(TEXT("research_active_but_definition_unavailable"));
	FTunaSweeperActiveResearchSaveData SavedActive;
	SavedActive.NodeId = UnavailableActive;
	SavedActive.StartUtcTicks = FDateTime::UtcNow().GetTicks();
	SavedActive.FinishUtcTicks = SavedActive.StartUtcTicks + ETimespan::TicksPerHour;
	Research->LoadResearchProgressFromSave({UnavailableApplied}, {SavedActive}, SavedActive.StartUtcTicks);

	TArray<FName> Applied;
	TArray<FTunaSweeperActiveResearchSaveData> Active;
	int64 ExportedTicks = 0;
	Research->ExportResearchProgressForSave(Applied, Active, ExportedTicks);
	TestTrue(TEXT("Unavailable applied node survives a save round trip"), Applied.Contains(UnavailableApplied));
	TestTrue(TEXT("Unavailable active node survives a save round trip"),
		Active.ContainsByPredicate([UnavailableActive](const FTunaSweeperActiveResearchSaveData& Entry)
		{
			return Entry.NodeId == UnavailableActive;
		}));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTunaSweeperResearchInvalidReloadTest,
	"TunaSweeper.Research.InvalidReloadKeepsDefinitions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaSweeperResearchInvalidReloadTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	UGameInstance* GameInstance = NewObject<UGameInstance>();
	UTunaSweeperResearchSubsystem* Research = NewObject<UTunaSweeperResearchSubsystem>(GameInstance);
	if (!TestTrue(TEXT("Original research definitions load"), Research->LoadResearchData())) return false;
	const FString TempPath = FPaths::CreateTempFilename(*FPaths::ProjectSavedDir(), TEXT("ResearchInvalid"), TEXT(".json"));
	const FString Node = TEXT(R"({"node_id":"fixture_a","row":0,"column":0,"required_applied_node_count":0,"duration_seconds":10,"effects":[{"type":"max_health","value":5}],"display_name_string_key":"research.node.vitality_1.name","description_string_key":"research.node.vitality_1.description"})");
	const FString OtherColumn = Node.Replace(TEXT("\"column\":0"), TEXT("\"column\":1"));
	const FString OtherId = Node.Replace(TEXT("fixture_a"), TEXT("fixture_b"));
	auto CheckRejectedReload = [this, Research, &TempPath](const FString& Json, const TCHAR* ExpectedLog, const TCHAR* CaseDescription)
	{
		if (!TestTrue(TEXT("Temporary invalid research fixture writes"), FFileHelper::SaveStringToFile(Json, *TempPath))) return;
		AddExpectedError(ExpectedLog, EAutomationExpectedErrorFlags::Contains, 1);
		TestFalse(CaseDescription, Research->LoadResearchDataFromFile(TempPath));
		FTunaSweeperResearchNodeView ExistingView;
		TestTrue(TEXT("Rejected reload retains the last good research tree"),
			Research->GetNodeView(FName(TEXT("vitality_1")), ExistingView));
	};
	CheckRejectedReload(TEXT("[{"), TEXT("Research data must be a nonempty JSON array"), TEXT("Malformed JSON reload is rejected"));
	CheckRejectedReload(FString(TEXT("[")) + Node.Replace(TEXT("max_health"), TEXT("unrecognized_effect")) + TEXT("]"),
		TEXT("has an invalid effect"), TEXT("Unknown effect type is rejected"));
	CheckRejectedReload(FString(TEXT("[")) + Node + TEXT(",") + OtherColumn + TEXT("]"),
		TEXT("has an invalid ID, position"), TEXT("Duplicate node ID is rejected"));
	CheckRejectedReload(FString(TEXT("[")) + Node + TEXT(",") + OtherId + TEXT("]"),
		TEXT("has an invalid ID, position"), TEXT("Duplicate grid position is rejected"));
	CheckRejectedReload(FString(TEXT("[")) + Node.Replace(TEXT("\"column\":0"), TEXT("\"column\":3")) + TEXT("]"),
		TEXT("has an invalid ID, position"), TEXT("Out-of-range grid column is rejected"));
	CheckRejectedReload(FString(TEXT("[")) + Node.Replace(TEXT("\"required_applied_node_count\":0"), TEXT("\"required_applied_node_count\":1")) +
		TEXT(",") + OtherId.Replace(TEXT("\"column\":0"), TEXT("\"column\":1"))
			.Replace(TEXT("\"required_applied_node_count\":0"), TEXT("\"required_applied_node_count\":1")) + TEXT("]"),
		TEXT("unreachable required node counts"), TEXT("Research tree with no initial node is rejected"));
	IFileManager::Get().Delete(*TempPath);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTunaSweeperResearchBurnProgressionTest,
	"TunaSweeper.Research.BurnProgression",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaSweeperResearchBurnProgressionTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	UGameInstance* GameInstance = NewObject<UGameInstance>();
	UTunaSweeperResearchSubsystem* Research = NewObject<UTunaSweeperResearchSubsystem>(GameInstance);
	if (!TestTrue(TEXT("Burn research data loads"), Research->LoadResearchData())) return false;
	const FName RifleType(TEXT("weapon.type.rifle"));
	const FName RifleAmmoType(TEXT("ammo.type.rifle"));
	const FTunaSweeperResearchBurnBonuses Base = Research->GetAppliedBurnBonuses(RifleType, RifleAmmoType);
	TestEqual(TEXT("No research adds no burn ticks"), Base.AdditionalTickCount, 0);
	TestEqual(TEXT("No research preserves base burn damage"), Base.DamageMultiplier, 1.0f);

	FTunaSweeperActiveResearchSaveData ReadyResearch;
	ReadyResearch.NodeId = FName(TEXT("ammo_burn_1"));
	ReadyResearch.bTimerCompleted = true;
	Research->LoadResearchProgressFromSave(
		{FName(TEXT("weapon_burn_1"))}, {ReadyResearch}, FDateTime::UtcNow().GetTicks());
	const FTunaSweeperResearchBurnBonuses WeaponOnly = Research->GetAppliedBurnBonuses(RifleType, RifleAmmoType);
	TestEqual(TEXT("Only claimed research extends burn"), WeaponOnly.AdditionalTickCount, 1);
	TestEqual(TEXT("Completed but unclaimed ammunition research adds no damage"), WeaponOnly.DamageMultiplier, 1.25f);
	TestTrue(TEXT("Completed ammunition research can be claimed"), Research->TryClaimResearch(ReadyResearch.NodeId));
	const FTunaSweeperResearchBurnBonuses Combined = Research->GetAppliedBurnBonuses(RifleType, RifleAmmoType);
	TestEqual(TEXT("Weapon and ammunition tick bonuses combine"), Combined.AdditionalTickCount, 2);
	TestEqual(TEXT("Weapon and ammunition damage bonuses combine"), Combined.DamageMultiplier, 1.5f);

	Research->LoadResearchProgressFromSave(
		{FName(TEXT("weapon_burn_1")), FName(TEXT("weapon_burn_2")), FName(TEXT("ammo_burn_1")), FName(TEXT("ammo_burn_2"))},
		{}, FDateTime::UtcNow().GetTicks());
	const FTunaSweeperResearchBurnBonuses Maximum = Research->GetAppliedBurnBonuses(RifleType, RifleAmmoType);
	TestEqual(TEXT("Two levels of weapon and ammunition research add four ticks"), Maximum.AdditionalTickCount, 4);
	TestEqual(TEXT("Two levels of each research double burn damage"), Maximum.DamageMultiplier, 2.0f);
	TestEqual(TEXT("Burn research does not increase maximum health"), Research->GetAppliedStatBonuses().MaxHealth, 0.0f);
	const FTunaSweeperResearchBurnBonuses Pistol = Research->GetAppliedBurnBonuses(
		FName(TEXT("weapon.type.pistol")), FName(TEXT("ammo.type.pistol")));
	TestEqual(TEXT("Rifle research does not extend pistol burn"), Pistol.AdditionalTickCount, 0);
	TestEqual(TEXT("Rifle research does not amplify pistol burn"), Pistol.DamageMultiplier, 1.0f);
	const FTunaSweeperResearchBurnBonuses MissingAmmo = Research->GetAppliedBurnBonuses(RifleType, NAME_None);
	TestEqual(TEXT("Missing ammunition receives only weapon research"), MissingAmmo.AdditionalTickCount, 2);
	TestEqual(TEXT("Missing ammunition receives only weapon damage bonuses"), MissingAmmo.DamageMultiplier, 1.5f);

	TArray<FName> SavedApplied;
	TArray<FTunaSweeperActiveResearchSaveData> SavedActive;
	int64 SavedClock = 0;
	Research->ExportResearchProgressForSave(SavedApplied, SavedActive, SavedClock);
	UTunaSweeperResearchSubsystem* Restored = NewObject<UTunaSweeperResearchSubsystem>(GameInstance);
	Restored->LoadResearchProgressFromSave(SavedApplied, SavedActive, SavedClock);
	const FTunaSweeperResearchBurnBonuses RestoredBonuses = Restored->GetAppliedBurnBonuses(RifleType, RifleAmmoType);
	TestEqual(TEXT("Save/load restores burn duration research"), RestoredBonuses.AdditionalTickCount, Maximum.AdditionalTickCount);
	TestEqual(TEXT("Save/load restores burn damage research"), RestoredBonuses.DamageMultiplier, Maximum.DamageMultiplier);
	Restored->ResetResearchProgressForNewGame();
	TestEqual(TEXT("New game clears burn research"), Restored->GetAppliedBurnBonuses(RifleType, RifleAmmoType).AdditionalTickCount, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTunaSweeperIncendiaryAmmoDataTest,
	"TunaSweeper.Research.IncendiaryAmmoData",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaSweeperIncendiaryAmmoDataTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	UGameInstance* GameInstance = NewObject<UGameInstance>();
	UTunaSweeperItemDataSubsystem* Items = NewObject<UTunaSweeperItemDataSubsystem>(GameInstance);
	FTunaSweeperItemDefinition Incendiary;
	FTunaSweeperItemDefinition Standard;
	if (!TestTrue(TEXT("Incendiary rifle ammunition is authored"), Items->TryGetItemDefinition(2023, Incendiary)) ||
		!TestTrue(TEXT("Standard rifle ammunition remains authored"), Items->TryGetItemDefinition(2002, Standard))) return false;
	TestEqual(TEXT("Incendiary ammunition enables burn damage"), Incendiary.BurnDamagePerTick, 2.0f);
	TestEqual(TEXT("Incendiary ammunition defaults to five burn ticks"), Incendiary.BurnTickCount, 5);
	TestEqual(TEXT("Incendiary and standard ammunition fit the same rifles"), Incendiary.AmmoTypeTag, Standard.AmmoTypeTag);
	TestEqual(TEXT("Ordinary ammunition does not ignite enemies"), Standard.BurnDamagePerTick, 0.0f);
	TestEqual(TEXT("Incendiary ammunition retains standard direct damage multiplier"), Incendiary.ProjectileDamageMultiplier, Standard.ProjectileDamageMultiplier);
	TestEqual(TEXT("Incendiary ammunition retains standard direct damage bonus"), Incendiary.ProjectileDamageBonus, Standard.ProjectileDamageBonus);
	return true;
}

#endif
