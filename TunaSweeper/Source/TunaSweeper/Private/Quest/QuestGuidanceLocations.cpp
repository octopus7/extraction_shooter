#include "Quest/QuestGuidanceLocations.h"
#include "AI/TunaSweeperEnemyCharacter.h"
#include "EngineUtils.h"
#include "Vehicle/TunaSweeperATVActor.h"
#include "Interaction/TunaSweeperEthanolFacilityActor.h"

#include "Engine/World.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Settings/TunaSweeperBuildFlavor.h"
#include "Subsystem/TunaSweeperEnemySpawnSubsystem.h"
#include "Subsystem/TunaSweeperInteractionSubsystem.h"
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
	struct FRepairCost
	{
		FName Kind;
		int32 ItemId;
		int32 Quantity;
	};
	using FRepairCosts = TMultiMap<FName, FRepairCost>;
	struct FProductionSource
	{
		FName Level;
		FVector Location;
		int32 InputId;
		int32 InputQuantity;
		int32 OutputQuantity;
	};

	TMultiMap<int32, FProductionSource> IndexProduction(const FBakedMaps& Maps)
	{
		TMultiMap<int32, FProductionSource> Sources;
		for (const auto& Map : Maps)
			for (const auto& Row : Map.Value)
			{
				if (Row.Kind != TEXT("production")) continue;
				TArray<FString> Parts;
				Row.Detail.ToString().ParseIntoArray(Parts, TEXT(":"));
				if (Parts.Num() == 3 && FCString::Atoi(*Parts[2]) > 0)
					Sources.Add(Row.NumericKey, {Map.Key, Row.Location, FCString::Atoi(*Parts[0]),
						FCString::Atoi(*Parts[1]), FCString::Atoi(*Parts[2])});
			}
		return Sources;
	}

	FRepairCosts IndexRepairCosts(const FBakedMaps& Maps)
	{
		FRepairCosts Costs;
		for (const auto& Map : Maps)
			for (const auto& Row : Map.Value)
			{
				if (Row.Kind != TEXT("repair_cost") && Row.Kind != TEXT("repair_tool") && Row.Kind != TEXT("repair_deposit")) continue;
				FString Id, Count;
				if (Row.Detail.ToString().Split(TEXT(":"), &Id, &Count))
					Costs.Add(Row.Key, {Row.Kind, FCString::Atoi(*Id), FCString::Atoi(*Count)});
			}
		return Costs;
	}

	void AddRefuelLocations(UWorld* World, FName EventId, bool bCarryingEthanol, TArray<FVector>& Locations)
	{
		if (bCarryingEthanol)
		{
			for (TActorIterator<ATunaSweeperATVActor> Vehicle(World); Vehicle; ++Vehicle)
				if (Vehicle->RefuelObjectiveEventId == EventId && Vehicle->CanReceiveFuel())
					Locations.AddUnique(Vehicle->GetActorLocation());
		}
		else
		{
			for (TActorIterator<ATunaSweeperEthanolFacilityActor> Facility(World); Facility; ++Facility)
				if (Facility->IsRepaired()) Locations.AddUnique(Facility->GetActorLocation());
		}
	}

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

	// Workbench stock includes the auxiliary bag and accessible storage, just like crafting.
	struct FCraftingGuidance
	{
		UTunaSweeperGameInstance& Game;
		UTunaSweeperItemDataSubsystem& Items;
		const TArray<FTunaSweeperWorkbenchRecipeDefinition>& Recipes;
		TSet<int32> MissingItems;
		TSet<int32> Workbenches;
		TMap<int32, int32> Reserved;
		TSet<FName> Visiting;

		const FTunaSweeperWorkbenchRecipeDefinition* FindRecipe(int32 OutputId) const
		{
			return Recipes.FindByPredicate([OutputId](const auto& Recipe) { return Recipe.OutputItemId == OutputId; });
		}

		void NeedItem(int32 ItemId, int32 Quantity)
		{
			if (const auto* Recipe = FindRecipe(ItemId))
				NeedRecipe(*Recipe, FMath::DivideAndRoundUp(Quantity, Recipe->OutputQuantity));
			else
				MissingItems.Add(ItemId);
		}

		void NeedRecipe(const FTunaSweeperWorkbenchRecipeDefinition& Recipe, int32 Batches)
		{
			if (Visiting.Contains(Recipe.RecipeId)) return;
			Visiting.Add(Recipe.RecipeId);
			bool bReady = true;
			for (const auto& Ingredient : Recipe.Ingredients)
			{
				const int32 Required = IntCastChecked<int32>(FMath::Min<int64>(int64(Ingredient.Quantity) * Batches, MAX_int32));
				int32& Used = Reserved.FindOrAdd(Ingredient.ItemId);
				const int32 Available = FMath::Max(0, Game.CountWorkbenchIngredientItemById(Ingredient.ItemId) - Used);
				Used += FMath::Min(Required, Available);
				if (Available < Required)
				{
					bReady = false;
					NeedItem(Ingredient.ItemId, Required - Available);
				}
			}
			if (!Recipe.bAutoUnlocked && !Game.IsWorkbenchRecipeUnlocked(Recipe.RecipeId))
			{
				TArray<int32> Blueprints;
				Items.GetBlueprintItemsForRecipe(Recipe.RecipeId, Blueprints);
				const bool bCanRegister = Blueprints.ContainsByPredicate([this](int32 Id)
				{ return Game.CountWorkbenchIngredientItemById(Id) > 0; });
				if (!bCanRegister)
				{
					bReady = false;
					for (int32 Id : Blueprints) MissingItems.Add(Id);
				}
			}
			if (bReady) Workbenches.Add(Recipe.WorkbenchId);
			Visiting.Remove(Recipe.RecipeId);
		}
	};

	void AddItemSources(UWorld* World, UTunaSweeperItemDataSubsystem& Items, FObjectiveLocations& Match,
		const TArray<FBakedLocation>* Rows, TArray<FVector>& Locations)
	{
		Items.GetGuaranteedContentsForItem(Match.Objective.ItemId, Match.Contents);
		if (auto* Spawns = World->GetGameInstance()->GetSubsystem<UTunaSweeperEnemySpawnSubsystem>())
			Spawns->GetGuaranteedLootLocations(World, Match.Contents, Locations);
		if (auto* Raid = World->GetGameInstance()->GetSubsystem<UTunaSweeperRaidPlacementSubsystem>())
			Raid->GetItemGuidanceAnchorIds(World, Match.Contents, Match.EnemyAnchors, Match.LootAnchors);
		if (Rows)
			for (const auto& Row : *Rows)
				if (Match.Matches(Row)) Locations.AddUnique(Row.Location);
	}
}

void QuestGuidanceLocations::Get(UWorld* World, TArray<FVector>& OutLocations)
{
	OutLocations.Reset();
	auto* Game = World ? Cast<UTunaSweeperGameInstance>(World->GetGameInstance()) : nullptr;
	if (!Game || Game->GetActiveSaveSlotDifficultyStage() != 1 || Game->IsCombatTestSession()) return;
	auto* Quests = Game->GetSubsystem<UTunaSweeperQuestSubsystem>();
	if (!Quests) return;
	const auto AddLocalOffers = [&] {
		if (auto* Interactions = World->GetSubsystem<UTunaSweeperInteractionSubsystem>())
			Interactions->AddAvailableQuestGiverLocations(*Quests, OutLocations);
	};
	const int32 QuestId = Quests->GetTrackedQuestId();
	if (QuestId <= 0 || Quests->GetQuestState(QuestId) != ETunaSweeperQuestState::Accepted)
	{
		AddLocalOffers();
		return;
	}
	const auto* Quest = Quests->FindQuestDefinition(QuestId);
	if (!Quest) return;

	static const FBakedMaps Maps = LoadBakedMaps();
	static const FRepairCosts RepairCosts = IndexRepairCosts(Maps);
	static const auto Production = IndexProduction(Maps);
	const FName Level = ShortLevel(FName(*World->GetMapName()));
	const auto* Rows = Maps.Find(Level);
	auto* Items = Game->GetSubsystem<UTunaSweeperItemDataSubsystem>();
	if (!Items) return;
	TArray<FTunaSweeperWorkbenchRecipeDefinition> Recipes;
	Items->GetAllWorkbenchRecipeDefinitions(Recipes);
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
		if (Level == TEXT("RaidVillage") && Objective.Type == EType::InteractionCompleted &&
			Objective.InteractionEventId == TEXT("vehicle.atv.refuel"))
		{
			AddRefuelLocations(World, Objective.InteractionEventId,
				Game->CountInventoryItemById(ATunaSweeperATVActor::EthanolItemId) > 0, OutLocations);
			continue;
		}
		const bool bDelivery = Objective.Type == EType::ItemSubmitted &&
			Game->CountQuestSubmissionItemById(Objective.ItemId) >= State.RequiredCount - State.CurrentCount;
		FCraftingGuidance Craft{*Game, *Items, Recipes};
		bool bGuideSupplies = false;
		if (Objective.Type == EType::WorkbenchRecipeCrafted)
		{
			FTunaSweeperWorkbenchRecipeDefinition Recipe;
			if (Items->TryGetWorkbenchRecipeDefinition(Objective.RecipeId, Recipe))
				Craft.NeedRecipe(Recipe, State.RequiredCount - State.CurrentCount);
			bGuideSupplies = true;
		}
		if (Objective.Type == EType::WorldProgressCompleted)
		{
			FTunaSweeperWorldProgressSaveData SavedProgress;
			Game->TryGetWorldProgressState(Objective.WorldProgressId, SavedProgress);
			// The materials and the repair site can belong to different maps.
			for (auto Cost = RepairCosts.CreateConstKeyIterator(Objective.WorldProgressId); Cost; ++Cost)
			{
				const FRepairCost& Requirement = Cost.Value();
				const int32 Owned = Requirement.Kind == TEXT("repair_tool") ? Game->CountCarriedItemById(Requirement.ItemId)
					: Game->CountInventoryItemById(Requirement.ItemId);
				const int32 Deposited = Requirement.Kind == TEXT("repair_deposit") ? SavedProgress.ProgressQuantity : 0;
				const int32 Missing = Requirement.Quantity - Owned - Deposited;
				if (Missing > 0)
				{
					bGuideSupplies = true;
					Craft.NeedItem(Requirement.ItemId, Missing);
				}
			}
		}
		FObjectiveLocations Match{ Objective, QuestId, *Quests, INDEX_NONE, bDelivery, {}, {}, {} };
		const bool bFindItems = Objective.Type == EType::ItemAcquired || Objective.Type == EType::ItemPossessed ||
			(Objective.Type == EType::ItemSubmitted && !bDelivery);
		if (bFindItems)
		{
			const int32 Remaining = Objective.Type == EType::ItemSubmitted ?
				State.RequiredCount - State.CurrentCount - Game->CountQuestSubmissionItemById(Objective.ItemId) :
				State.RequiredCount - State.CurrentCount;
			if (Production.Contains(Objective.ItemId))
			{
				for (auto It = Production.CreateConstKeyIterator(Objective.ItemId); It; ++It)
				{
					const auto& Source = It.Value();
					const int32 Batches = FMath::DivideAndRoundUp(Remaining, Source.OutputQuantity);
					const int32 Missing = Batches * Source.InputQuantity - Game->CountInventoryItemById(Source.InputId);
					if (Missing > 0) Craft.NeedItem(Source.InputId, Missing);
					else if (Level == Source.Level) OutLocations.AddUnique(Source.Location);
				}
			}
			else if (Craft.FindRecipe(Objective.ItemId))
				Craft.NeedItem(Objective.ItemId, Remaining);
			else
			{
				AddItemSources(World, *Items, Match, Rows, OutLocations);
				continue;
			}
			bGuideSupplies = true;
		}
		if (bGuideSupplies)
		{
			for (int32 ItemId : Craft.MissingItems)
			{
				FTunaSweeperObjectiveDefinition Ingredient;
				Ingredient.Type = EType::ItemPossessed;
				Ingredient.ItemId = ItemId;
				FObjectiveLocations Source{ Ingredient, QuestId, *Quests, INDEX_NONE, false, {}, {}, {} };
				AddItemSources(World, *Items, Source, Rows, OutLocations);
			}
			if (Rows)
				for (const auto& Row : *Rows)
					if (Row.Kind == TEXT("workbench") && Craft.Workbenches.Contains(Row.NumericKey))
						OutLocations.AddUnique(Row.Location);
			continue;
		}
		if (Rows)
			for (const auto& Row : *Rows)
				if (Match.Matches(Row)) OutLocations.AddUnique(Row.Location);
	}
	if (OutLocations.IsEmpty()) AddLocalOffers();
}

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FATVRefuelGuidanceTest, "TunaSweeper.Quest.Guidance.VillageRefuel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FATVRefuelGuidanceTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	ON_SCOPE_EXIT { World->DestroyWorld(false); };
	UClass* Class = LoadClass<ATunaSweeperATVActor>(nullptr, TEXT("/Game/Blueprints/Vehicles/ATV/BP_ATV_TypeA.BP_ATV_TypeA_C"));
	if (!TestNotNull(TEXT("Village repaired vehicle class"), Class)) return false;
	auto* Vehicle = World->SpawnActor<ATunaSweeperATVActor>(Class);
	if (!TestNotNull(TEXT("Spawned replacement ATV"), Vehicle)) return false;
	const FName Event = TEXT("vehicle.atv.refuel");
	Vehicle->SetActorLocation(FVector(1200, 700, 160));
	TArray<FVector> Locations;
	AddRefuelLocations(World, Event, false, Locations);
	TestTrue(TEXT("Without ethanol the empty ATV is not a target"), Locations.IsEmpty());
	AddRefuelLocations(World, Event, true, Locations);
	TestTrue(TEXT("Carried ethanol points at the spawned vehicle"), Locations.Num() == 1 && Locations[0] == Vehicle->GetActorLocation());
	Vehicle->FuelSeconds = 60.f;
	Locations.Reset();
	AddRefuelLocations(World, Event, true, Locations);
	TestTrue(TEXT("A fueled ATV cannot satisfy refueling"), Locations.IsEmpty());
	return true;
}

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
	TArray<FTunaSweeperWorkbenchRecipeDefinition> Recipes;
	if (!TestTrue(TEXT("Load workbench recipes"), Items->GetAllWorkbenchRecipeDefinitions(Recipes))) return false;
	auto* EmptyInventory = NewObject<UTunaSweeperGameInstance>();
	FCraftingGuidance Craft{*EmptyInventory, *Items, Recipes};
	Craft.NeedItem(6031, 1);
	TestTrue(TEXT("Explosive guidance points to both raw materials"),
		Craft.MissingItems.Contains(6028) && Craft.MissingItems.Contains(6032));
	TestFalse(TEXT("Crafted explosive is not a loot marker"), Craft.MissingItems.Contains(6031));
	TestTrue(TEXT("Missing materials do not point to a workbench"), Craft.Workbenches.IsEmpty());
	const auto Costs = IndexRepairCosts(Maps);
	const FRepairCost* Rockfall = Costs.Find(TEXT("region.forest.village.obstacle"));
	if (!TestNotNull(TEXT("Rockfall recipe requirement baked from the actual actor"), Rockfall)) return false;
	TestEqual(TEXT("Rockfall requires the crafted explosive"), Rockfall->ItemId, 6031);
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
