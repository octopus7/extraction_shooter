#include "Quest/QuestGuidanceLocations.h"
#include "AI/TunaSweeperEnemyCharacter.h"
#include "EngineUtils.h"

#include "Engine/World.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Settings/TunaSweeperBuildFlavor.h"
#include "Subsystem/TunaSweeperEnemySpawnSubsystem.h"
#include "Subsystem/TunaSweeperItemDataSubsystem.h"
#include "Subsystem/TunaSweeperQuestSubsystem.h"
#include "Subsystem/TunaSweeperRaidPlacementSubsystem.h"

namespace
{
	struct FBakedLocation
	{
		FName Kind;
		FName Key;
		FName Detail;
		int32 NumericKey;
		int32 NumericDetail;
		FVector Location;
	};

	using FBakedMaps = TMap<FName, TArray<FBakedLocation>>;

	void AddLivingEnemyLocations(UWorld* World, const FTunaSweeperObjectiveDefinition& Objective, TArray<FVector>& Locations)
	{
		for (TActorIterator<ATunaSweeperEnemyCharacter> Enemy(World); Enemy; ++Enemy)
		{
			if (!Enemy->IsDead() && Objective.MatchesEnemyTarget(Enemy->GetEnemyId(), Enemy->GetClass()))
				Locations.AddUnique(Enemy->GetActorLocation());
		}
	}

	FBakedMaps LoadBakedMaps()
	{
		FBakedMaps Maps;
		TArray<FString> Lines;
		const FString Path = FPaths::ProjectContentDir() / TEXT("Data/QuestGuidanceLocations.tsv");
		if (!FFileHelper::LoadFileToStringArray(Lines, *Path))
		{
			UE_LOG(LogTemp, Error, TEXT("Missing quest guidance bake: %s"), *Path);
			return Maps;
		}
		for (int32 Index = 1; Index < Lines.Num(); ++Index)
		{
			TArray<FString> Fields;
			Lines[Index].ParseIntoArray(Fields, TEXT("\t"), false);
			if (Fields.Num() != 7) continue;
			Maps.FindOrAdd(FName(*Fields[0])).Add({
				FName(*Fields[1]), FName(*Fields[2]), FName(*Fields[3]), FCString::Atoi(*Fields[2]), FCString::Atoi(*Fields[3]),
				FVector(FCString::Atod(*Fields[4]), FCString::Atod(*Fields[5]), FCString::Atod(*Fields[6]))});
		}
		return Maps;
	}

	FName ShortLevel(FName Level)
	{
		return FName(*UWorld::RemovePIEPrefix(FPackageName::GetShortName(Level.ToString())));
	}

	bool MatchesLevel(FName Expected, FName Actual)
	{
		return Expected.IsNone() || ShortLevel(Expected) == ShortLevel(Actual) ||
			(ShortLevel(Expected) == TEXT("RaidMap") && TunaSweeperBuildFlavor::IsRaidGameplayLevelName(Actual));
	}

	struct FObjectiveLocations
	{
		const FTunaSweeperObjectiveDefinition& Objective;
		int32 QuestId;
		const UTunaSweeperQuestSubsystem& Quests;
		int32 WorkbenchId;
		bool bReadyToDeliver;
		TSet<int32> Contents;
		TSet<int32> EnemyAnchors;
		TSet<int32> LootAnchors;

		bool Matches(const FBakedLocation& Row) const
		{
			using EType = ETunaSweeperObjectiveType;
			if (Row.Kind == TEXT("anchor"))
				return Row.Detail == TEXT("ENEMY") ? EnemyAnchors.Contains(Row.NumericKey) : LootAnchors.Contains(Row.NumericKey);

			switch (Objective.Type)
			{
			case EType::ItemSubmitted:
				if (bReadyToDeliver)
					return (Row.Kind == TEXT("delivery") && Row.NumericKey == QuestId && Row.Detail == Objective.ObjectiveId) ||
						(Row.Kind == TEXT("provider") && !Row.Key.IsNone() && Row.Key == Objective.TargetProviderId);
				if (Row.Kind == TEXT("parcel")) return Row.NumericKey == QuestId && Row.Detail == Objective.ObjectiveId;
				[[fallthrough]];
			case EType::ItemAcquired:
			case EType::ItemPossessed:
				return ((Row.Kind == TEXT("item") || (Row.Kind == TEXT("quest_item") &&
					Quests.GetQuestState(Row.NumericDetail) == ETunaSweeperQuestState::Accepted)) && Row.NumericKey == Objective.ItemId) ||
					(Row.Kind == TEXT("contents") && Contents.Contains(Row.NumericKey));
			case EType::InteractionCompleted:
				return Row.Kind == TEXT("event") && (Objective.InteractionEventId.IsNone() || Row.Key == Objective.InteractionEventId) &&
					(Objective.InteractionTypeName.IsNone() || Row.Detail == Objective.InteractionTypeName);
			case EType::WorldProgressCompleted:
				return Row.Kind == TEXT("progress") && Row.Key == Objective.WorldProgressId;
			case EType::LocationReached:
			case EType::VehicleLocationReached:
				return Row.Kind == (Objective.Type == EType::LocationReached ? TEXT("arrival") : TEXT("vehicle_arrival")) && Row.Key == Objective.LocationId;
			case EType::WarpPointUsed:
				return Row.Kind == TEXT("warp") && (Objective.WarpPointId.IsNone() || Row.Key == Objective.WarpPointId) &&
					(Objective.TargetWarpPointId.IsNone() || Row.Detail == Objective.TargetWarpPointId);
			case EType::LevelTravel:
			case EType::ExtractionReturn:
				return (Row.Kind == TEXT("extract") || (Objective.Type == EType::LevelTravel && Row.Kind == TEXT("travel"))) &&
					MatchesLevel(Objective.TargetLevelName, TunaSweeperBuildFlavor::ResolveGameplayLevelName(Row.Key));
			case EType::WorkbenchRecipeCrafted:
				return Row.Kind == TEXT("workbench") && Row.NumericKey == WorkbenchId;
			default:
				return false;
			}
		}
	};
}

void QuestGuidanceLocations::Get(UWorld* World, TArray<FVector>& OutLocations)
{
	OutLocations.Reset();
	auto* Game = World ? Cast<UTunaSweeperGameInstance>(World->GetGameInstance()) : nullptr;
	if (!Game || Game->GetActiveSaveSlotDifficultyStage() != 1 || Game->IsCombatTestSession()) return;
	auto* Quests = Game->GetSubsystem<UTunaSweeperQuestSubsystem>();
	const int32 QuestId = Quests ? Quests->GetTrackedQuestId() : 0;
	if (QuestId <= 0 || Quests->GetQuestState(QuestId) != ETunaSweeperQuestState::Accepted) return;
	const auto* Quest = Quests->FindQuestDefinition(QuestId);
	if (!Quest) return;

	static const FBakedMaps Maps = LoadBakedMaps();
	const FName Level = ShortLevel(FName(*World->GetMapName()));
	const auto* Rows = Maps.Find(Level);
	TArray<FTunaSweeperObjectiveProgressView> Progress;
	if (!Quests->GetQuestObjectiveProgress(QuestId, Progress)) return;
	for (int32 Index = 0; Index < Quest->Objectives.Num(); ++Index)
	{
		const auto& Objective = Quest->Objectives[Index];
		const auto& State = Progress[Index];
		if (State.bCompleted || !MatchesLevel(Objective.SourceLevelName, Level)) continue;

		using EType = ETunaSweeperObjectiveType;
		if (Objective.Type == EType::EnemyKilled)
		{
			AddLivingEnemyLocations(World, Objective, OutLocations);
			continue;
		}
		const bool bDelivery = Objective.Type == EType::ItemSubmitted &&
			Game->CountQuestSubmissionItemById(Objective.ItemId) >= State.RequiredCount - State.CurrentCount;
		int32 WorkbenchId = INDEX_NONE;
		if (Objective.Type == EType::WorkbenchRecipeCrafted)
		{
			FTunaSweeperWorkbenchRecipeDefinition Recipe;
			auto* Items = Game->GetSubsystem<UTunaSweeperItemDataSubsystem>();
			if (Items && Items->TryGetWorkbenchRecipeDefinition(Objective.RecipeId, Recipe)) WorkbenchId = Recipe.WorkbenchId;
		}
		FObjectiveLocations Match{ Objective, QuestId, *Quests, WorkbenchId, bDelivery, {}, {}, {} };
		const bool bFindItems = Objective.Type == EType::ItemAcquired || Objective.Type == EType::ItemPossessed ||
			(Objective.Type == EType::ItemSubmitted && !bDelivery);
		if (bFindItems)
		{
			if (auto* Items = Game->GetSubsystem<UTunaSweeperItemDataSubsystem>())
				Items->GetGuaranteedContentsForItem(Objective.ItemId, Match.Contents);
			if (auto* Spawns = Game->GetSubsystem<UTunaSweeperEnemySpawnSubsystem>())
				Spawns->GetGuaranteedLootLocations(World, Match.Contents, OutLocations);
			if (auto* Raid = Game->GetSubsystem<UTunaSweeperRaidPlacementSubsystem>())
				Raid->GetItemGuidanceAnchorIds(World, Match.Contents, Match.EnemyAnchors, Match.LootAnchors);
		}
		if (Rows)
			for (const auto& Row : *Rows)
				if (Match.Matches(Row)) OutLocations.AddUnique(Row.Location);
	}
}

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDeliveryRobotGuidanceTest, "TunaSweeper.Quest.Guidance.MovingDeliveryRobots",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDeliveryRobotGuidanceTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	ON_SCOPE_EXIT { World->DestroyWorld(false); };
	FTunaSweeperObjectiveDefinition Objective;
	for (const TCHAR* Side : {TEXT("Marine"), TEXT("Pirate")})
	{
		const FString Name = FString(TEXT("BP_DeliveryRobot_")) + Side;
		const FString Path = TEXT("/Game/Characters/Region2Robots/") + Name + TEXT(".") + Name + TEXT("_C");
		UClass* Class = LoadClass<ATunaSweeperEnemyCharacter>(nullptr, *Path);
		if (!TestNotNull(TEXT("Delivery robot class"), Class)) return false;
		ATunaSweeperEnemyCharacter* Robot = World->SpawnActor<ATunaSweeperEnemyCharacter>(Class);
		if (!TestNotNull(TEXT("Delivery robot"), Robot)) return false;
		Objective.EnemyClassPaths = {FSoftClassPath(Class)};
		Robot->SetActorLocation(FVector(530, -720, 220));
		TArray<FVector> Locations;
		AddLivingEnemyLocations(World, Objective, Locations);
		TestTrue(TEXT("Marker follows this faction to its current position"),
			Locations.Num() == 1 && Locations[0] == Robot->GetActorLocation());
		Robot->Destroy();
		Locations.Reset();
		AddLivingEnemyLocations(World, Objective, Locations);
		TestTrue(TEXT("Removed robot leaves no marker"), Locations.IsEmpty());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FQuestGuidanceBakeTest, "TunaSweeper.Quest.Guidance.BakedLocations",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FQuestGuidanceBakeTest::RunTest(const FString& Parameters)
{
	const FBakedMaps Maps = LoadBakedMaps();
	const auto* Forest = Maps.Find(TEXT("RaidForest"));
	if (!TestNotNull(TEXT("Forest map is baked"), Forest)) return false;
	auto* Items = NewObject<UTunaSweeperItemDataSubsystem>(NewObject<UGameInstance>());
	FTunaSweeperObjectiveDefinition Objective;
	Objective.Type = ETunaSweeperObjectiveType::ItemPossessed;
	Objective.ItemId = 3007;
	FObjectiveLocations Match{ Objective, 0, *GetDefault<UTunaSweeperQuestSubsystem>(), INDEX_NONE, false, {}, {}, {} };
	Items->GetGuaranteedContentsForItem(Objective.ItemId, Match.Contents);
	TSet<FVector> Locations;
	for (const auto& Row : *Forest)
		if (Match.Matches(Row)) Locations.Add(Row.Location);
	TestTrue(TEXT("Both the supply crate and wild sweet potato are guaranteed sources"), Locations.Num() >= 2);
	TSet<int32> OptionalDrops;
	Items->GetGuaranteedContentsForItem(6004, OptionalDrops);
	TestFalse(TEXT("Chance-only enemy drops do not guide item farming"), OptionalDrops.Contains(8009));

	Objective.Type = ETunaSweeperObjectiveType::ItemSubmitted;
	Objective.ObjectiveId = TEXT("test_parcel");
	Match.QuestId = 43;
	const FBakedLocation Pickup{ TEXT("parcel"), TEXT("43"), Objective.ObjectiveId, 43, 0, FVector::ZeroVector };
	const FBakedLocation Delivery{ TEXT("delivery"), TEXT("43"), Objective.ObjectiveId, 43, 0, FVector::ZeroVector };
	TestTrue(TEXT("Missing parcel guides to pickup"), Match.Matches(Pickup));
	TestFalse(TEXT("Missing parcel hides destination"), Match.Matches(Delivery));
	Match.bReadyToDeliver = true;
	TestFalse(TEXT("Held parcel hides pickup"), Match.Matches(Pickup));
	TestTrue(TEXT("Held parcel guides to destination"), Match.Matches(Delivery));
	return true;
}
#endif
