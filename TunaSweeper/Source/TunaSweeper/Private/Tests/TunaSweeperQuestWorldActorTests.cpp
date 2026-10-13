#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "UObject/UObjectGlobals.h"
#include "Components/PointLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/PointLight.h"
#include "Engine/SpotLight.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Environment/TunaSweeperBoilingPotActor.h"
#include "NiagaraComponent.h"
#include "Game/TunaSweeperGameInstance.h"
#include "GameFramework/Pawn.h"
#include "Interaction/TunaSweeperQuestFacilityActor.h"
#include "Interaction/TunaSweeperQuestLightController.h"
#include "Misc/ScopeExit.h"
#include "PhysicsEngine/BodySetup.h"
#include "Subsystem/TunaSweeperInteractionSubsystem.h"
#include "Subsystem/TunaSweeperQuestSubsystem.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperQuestWorldActorContractTest,
	"TunaSweeper.Quest.WorldActors.Contract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaSweeperQuestWorldActorContractTest::RunTest(const FString& Parameters)
{
	TestNotNull(TEXT("Quest gated facility is independently placeable"),
		FindObject<UClass>(nullptr, TEXT("/Script/TunaSweeper.TunaSweeperQuestFacilityActor")));
	TestNotNull(TEXT("Quest light controller is independently placeable"),
		FindObject<UClass>(nullptr, TEXT("/Script/TunaSweeper.TunaSweeperQuestLightController")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperQuestWorldActorsTest,
	"TunaSweeper.Quest.WorldActors.StateAndPersistence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaSweeperQuestWorldActorsTest::RunTest(const FString& Parameters)
{
	struct FContextAccess : UGameInstance
	{
		static void Attach(UGameInstance* Game, FWorldContext* Context)
		{ auto Member = &FContextAccess::WorldContext; Game->*Member = Context; }
	};
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
	Context.SetCurrentWorld(World);
	auto* Game = NewObject<UTunaSweeperGameInstance>(GEngine);
	FContextAccess::Attach(Game, &Context);
	Context.OwningGameInstance = Game;
	World->SetGameInstance(Game);
	Game->bInventoryStateInitialized = true;
	Game->RetiredDemoSaveSlotIndex = Game->ActiveSaveSlotIndex;
	Game->UGameInstance::Init();
	ON_SCOPE_EXIT
	{
		World->DestroyWorld(false);
		Game->UGameInstance::Shutdown(); World->SetGameInstance(nullptr); Context.OwningGameInstance = nullptr;
		FContextAccess::Attach(Game, nullptr); GEngine->DestroyWorldContext(World); World->RemoveFromRoot();
	};
	auto* Quests = Game->GetSubsystem<UTunaSweeperQuestSubsystem>();
	Quests->QuestDefinitions.Reset(); Quests->QuestProgressById.Reset(); Quests->bQuestDataLoaded = true;
	for (int32 Id : {910001, 910002})
	{
		FTunaSweeperQuestDefinition Definition; Definition.QuestId = Id; Definition.bCompleteOnAccept = true;
		Quests->QuestDefinitions.Add(Id, Definition);
	}
	FTunaSweeperQuestDefinition UseQuest; UseQuest.QuestId = 910003;
	FTunaSweeperObjectiveDefinition Objective; Objective.ObjectiveId = TEXT("use");
	Objective.Type = ETunaSweeperObjectiveType::InteractionCompleted;
	Objective.InteractionEventId = TEXT("test.facility.use");
	UseQuest.Objectives.Add(Objective); Quests->QuestDefinitions.Add(UseQuest.QuestId, UseQuest);

	auto* Facility = World->SpawnActor<ATunaSweeperQuestFacilityActor>();
	auto* Pawn = World->SpawnActor<APawn>();
	auto* PawnRoot = NewObject<USceneComponent>(Pawn); Pawn->SetRootComponent(PawnRoot); PawnRoot->RegisterComponent();
	auto* Interaction = Facility->GetInteractableComponent();
	auto* Interactions = World->GetSubsystem<UTunaSweeperInteractionSubsystem>();
	Facility->DispatchBeginPlay();
	TestFalse(TEXT("Unconfigured facility is locked"), Facility->IsFacilityUnlocked());
	FTunaSweeperFacilityQuestCondition Condition; Condition.QuestId = 910001;
	Facility->UnlockConditions.Add(Condition); Facility->InteractionObjectiveEventId = TEXT("test.facility.use");
	Facility->RefreshQuestState();
	TestFalse(TEXT("Unavailable facility rejects direct use"), Facility->TryUseFacility(Pawn));
	TestFalse(TEXT("Unavailable facility rejects standard interaction"), Interactions->RequestInteraction(Interaction, Pawn));
	TestEqual(TEXT("Locked prompt is absent"), Interaction->GetInteractionType(), ETunaSweeperInteractionType::None);
	Quests->AcceptQuest(910001);
	TestFalse(TEXT("Reward waiting is not reward completed"), Facility->IsFacilityUnlocked());

	auto* Point = World->SpawnActor<APointLight>(); Point->GetLightComponent()->SetMobility(EComponentMobility::Movable);
	auto* Spot = World->SpawnActor<ASpotLight>(); Spot->GetLightComponent()->SetMobility(EComponentMobility::Movable);
	auto* Untouched = World->SpawnActor<APointLight>(); Untouched->GetLightComponent()->SetVisibility(true);
	auto* Controller = World->SpawnActor<ATunaSweeperQuestLightController>();
	FTunaSweeperQuestLightGroup Group; Group.CompletedQuestId = 910001; Group.LightActors = {Point, Spot, nullptr};
	Controller->LightGroups.Add(Group); Controller->DispatchBeginPlay();
	TestFalse(TEXT("Before completion point light is off"), Point->GetLightComponent()->IsVisible());
	TestFalse(TEXT("Before completion spot light is off"), Spot->GetLightComponent()->IsVisible());
	TestTrue(TEXT("Unlisted light is preserved"), Untouched->GetLightComponent()->IsVisible());
	auto* Pot = World->SpawnActor<ATunaSweeperBoilingPotActor>();
	Pot->RequiredCompletedQuestId = 910001; Pot->DispatchBeginPlay();
	auto* Steam = Pot->FindComponentByClass<UNiagaraComponent>();
	TestFalse(TEXT("Reward waiting keeps pot cold"), Pot->IsBoiling());
	TestFalse(TEXT("Locked pot steam is hidden"), Steam->IsVisible());
	Pot->SetBoiling(true);
	TestFalse(TEXT("Direct boiling request cannot bypass kitchen lock"), Pot->IsBoiling());
	Quests->ClaimQuestReward(910001);
	TestTrue(TEXT("Kitchen reward starts boiling immediately"), Pot->IsBoiling());
	TestTrue(TEXT("Kitchen reward reveals steam"), Steam->IsVisible());
	TestTrue(TEXT("Reward event immediately unlocks facility"), Facility->IsFacilityUnlocked());
	TestEqual(TEXT("Reward event immediately enables prompt"), Interaction->GetInteractionType(), ETunaSweeperInteractionType::QuestFacilityUse);
	TestFalse(TEXT("Localized label resolves"), Interaction->GetInteractionDisplayName().IsEmpty());
	TestTrue(TEXT("Reward event turns on point light"), Point->GetLightComponent()->IsVisible());
	TestTrue(TEXT("Reward event turns on spot light"), Spot->GetLightComponent()->IsVisible());
	TestFalse(TEXT("Null pawn rejected"), Facility->TryUseFacility(nullptr));
	Pawn->SetActorLocation(FVector(10000, 0, 0));
	TestFalse(TEXT("Out of range use rejected"), Facility->TryUseFacility(Pawn));
	Pawn->SetActorLocation(FVector::ZeroVector);
	Quests->AcceptQuest(910003);
	TestTrue(TEXT("Unlocked facility uses real interaction route"), Interactions->RequestInteraction(Interaction, Pawn));
	TestEqual(TEXT("Successful use advances matching quest event"), Quests->GetQuestState(910003), ETunaSweeperQuestState::RewardAvailable);
	Condition.QuestId = 910002; Facility->UnlockConditions.Add(Condition);
	TestFalse(TEXT("Execution rechecks edited conditions even with stale prompt"), Interactions->RequestInteraction(Interaction, Pawn));
	Quests->AcceptQuest(910002); Quests->ClaimQuestReward(910002);
	TestTrue(TEXT("All conditions satisfied"), Facility->IsFacilityUnlocked());
	Facility->UnlockConditions[1].QuestId = 919999;
	TestFalse(TEXT("Unknown quest cannot unlock"), Facility->IsFacilityUnlocked());
	Facility->UnlockConditions[1].QuestId = 0;
	TestFalse(TEXT("Invalid quest cannot unlock"), Facility->IsFacilityUnlocked());
	Facility->UnlockConditions[1].QuestId = 910002; Facility->UnlockConditions[1].AllowedStates.Reset();
	TestFalse(TEXT("Empty allowed states keep locked"), Facility->IsFacilityUnlocked());
	Facility->UnlockConditions.RemoveAt(1);

	// Explicit accepted-only facilities may relock; permanent unlocks use all later states.
	Facility->UnlockConditions[0].QuestId = 910003;
	Facility->UnlockConditions[0].AllowedStates = {ETunaSweeperQuestState::Accepted};
	TestFalse(TEXT("Exact accepted state excludes reward waiting"), Facility->IsFacilityUnlocked());
	Facility->UnlockConditions[0].AllowedStates.Add(ETunaSweeperQuestState::RewardAvailable);
	TestTrue(TEXT("Author can allow reward waiting"), Facility->IsFacilityUnlocked());
	Facility->UnlockConditions[0] = FTunaSweeperFacilityQuestCondition(); Facility->UnlockConditions[0].QuestId = 910001;

	TArray<FTunaSweeperNumericQuestProgressSaveData> Saved; int32 Tracked = 0, Coins = 0;
	Quests->ExportQuestProgressForSave(Saved, Tracked, Coins);
	Quests->ResetQuestProgressForNewGame();
	TestFalse(TEXT("New game stops boiling"), Pot->IsBoiling());
	TestFalse(TEXT("New game hides residual steam"), Steam->IsVisible());
	TestFalse(TEXT("New game immediately relocks facility"), Facility->IsFacilityUnlocked());
	TestEqual(TEXT("New game clears prompt"), Interaction->GetInteractionType(), ETunaSweeperInteractionType::None);
	TestFalse(TEXT("New game restores lights before state"), Point->GetLightComponent()->IsVisible());
	Quests->LoadQuestProgressFromSave(Saved, Tracked, Coins);
	TestTrue(TEXT("Loaded kitchen unlock restores boiling"), Pot->IsBoiling());
	Pot->SetBoiling(false);
	Quests->LoadQuestProgressFromSave(Saved, Tracked, Coins);
	TestFalse(TEXT("Explicitly stopped pot stays stopped on refresh"), Pot->IsBoiling());
	Pot->Destroy(); Pot = World->SpawnActor<ATunaSweeperBoilingPotActor>();
	Pot->RequiredCompletedQuestId = 910001; Pot->DispatchBeginPlay();
	TestTrue(TEXT("Pot created after save load restores boiling"), Pot->IsBoiling());
	TestTrue(TEXT("Loading save restores facility prompt"), Interaction->GetInteractionType() == ETunaSweeperInteractionType::QuestFacilityUse);
	TestTrue(TEXT("Loading save restores lights without another completion event"), Point->GetLightComponent()->IsVisible());
	Controller->Destroy(); Controller = World->SpawnActor<ATunaSweeperQuestLightController>();
	Controller->LightGroups.Add(Group); Point->GetLightComponent()->SetVisibility(false); Controller->DispatchBeginPlay();
	TestTrue(TEXT("Recreated controller restores loaded quest state"), Point->GetLightComponent()->IsVisible());
	Controller->LightGroups[0].bVisibleAfterCompletion = false; Controller->RefreshQuestLights();
	TestFalse(TEXT("Completion can switch lights off"), Point->GetLightComponent()->IsVisible());
	Controller->LightGroups.Add(Group); Controller->RefreshQuestLights();
	TestTrue(TEXT("Later group wins duplicate reference"), Point->GetLightComponent()->IsVisible());
	Controller->LightGroups.Last().CompletedQuestId = 919999; Controller->RefreshQuestLights();
	TestFalse(TEXT("Unknown quest uses before state"), Point->GetLightComponent()->IsVisible());
	Point->GetLightComponent()->SetMobility(EComponentMobility::Static); Point->GetLightComponent()->SetVisibility(true);
	Controller->RefreshQuestLights(); TestTrue(TEXT("Static baked lighting is not modified"), Point->GetLightComponent()->IsVisible());
	Spot->Destroy(); Controller->RefreshQuestLights();
	TestTrue(TEXT("Destroyed and empty references are safe"), true);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperQuestFacilityAssetTest,
	"TunaSweeper.Quest.WorldActors.PigletAsset",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTunaSweeperQuestFacilityAssetTest::RunTest(const FString& Parameters)
{
	auto* CDO = GetDefault<ATunaSweeperQuestFacilityActor>();
	auto* Mesh = CDO->FindComponentByClass<UStaticMeshComponent>()->GetStaticMesh().Get();
	if (!TestNotNull(TEXT("Placeholder piglet mesh loads through facility default"), Mesh)) return false;
	TestTrue(TEXT("Piglet dimensions are in centimeters"), Mesh->GetBounds().BoxExtent.X > 35 && Mesh->GetBounds().BoxExtent.X < 50);
	TestEqual(TEXT("Voxel piglet uses six color materials"), Mesh->GetStaticMaterials().Num(), 6);
	TestEqual(TEXT("Voxel piglet triangle count survives import"), Mesh->GetNumTriangles(0), 648);
	for (const auto& Material : Mesh->GetStaticMaterials()) TestNotNull(TEXT("Piglet material resolves"), Material.MaterialInterface.Get());
	if (TestNotNull(TEXT("Piglet has collision"), Mesh->GetBodySetup()))
		TestTrue(TEXT("Piglet collision has actual shapes"), Mesh->GetBodySetup()->AggGeom.GetElementCount() > 0);
	return true;
}
#endif
