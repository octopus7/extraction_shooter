#if WITH_DEV_AUTOMATION_TESTS
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Interaction/TunaSweeperInteractableComponent.h"
#include "Interaction/TunaSweeperPowerFacilityActor.h"
#include "Misc/AutomationTest.h"
#include "Misc/App.h"
#include "Misc/ScopeExit.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "UObject/UnrealType.h"
#include "UObject/StrongObjectPtr.h"

namespace PowerFacilityTests
{
struct FContextAccess : UGameInstance
{
	static void Attach(UGameInstance* Instance, FWorldContext* Context)
	{
		auto Member = &FContextAccess::WorldContext;
		Instance->*Member = Context;
	}
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperPowerFacilityTest,
	"TunaSweeper.Interaction.PowerFacility.RepairAndRestore",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaSweeperPowerFacilityTest::RunTest(const FString& Parameters)
{
	UClass* Class = FindObject<UClass>(nullptr, TEXT("/Script/TunaSweeper.TunaSweeperPowerFacilityActor"));
	if (!TestNotNull(TEXT("Dedicated power facility class exists"), Class)) return false;
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	auto* Game = NewObject<UTunaSweeperGameInstance>(GEngine);
	Game->bInventoryStateInitialized = true; // Isolate from player save files and initial loadout.
	World->SetGameInstance(Game);
	ON_SCOPE_EXIT { World->SetGameInstance(nullptr); World->DestroyWorld(false); World->RemoveFromRoot(); };
	auto Bool = [Class](AActor* Actor, const TCHAR* Name, bool Value)
	{
		auto* Property = FindFProperty<FBoolProperty>(Class, Name);
		check(Property);
		Property->SetPropertyValue_InContainer(Actor, Value);
	};
	auto Name = [Class](AActor* Actor, const TCHAR* Property, FName Value)
	{
		auto* Field = FindFProperty<FNameProperty>(Class, Property);
		check(Field);
		Field->SetPropertyValue_InContainer(Actor, Value);
	};
	auto Refresh = [](AActor* Actor)
	{
		CastChecked<ATunaSweeperPowerFacilityActor>(Actor)->RefreshFacilityState();
	};
	auto Repair = [](AActor* Actor)
	{
		return CastChecked<ATunaSweeperPowerFacilityActor>(Actor)->Repair(false);
	};
	auto* Facility = World->SpawnActor<AActor>(Class);
	if (!TestNotNull(TEXT("Facility spawns"), Facility)) return false;
	Bool(Facility, TEXT("bRequireActiveQuest"), false);
	Bool(Facility, TEXT("bRequiresItem"), false);
	Refresh(Facility);
	TestFalse(TEXT("Unconfigured persistence ID blocks repair"), Repair(Facility));
	Name(Facility, TEXT("ProgressObjectId"), TEXT("test.power.a"));
	Facility->DispatchBeginPlay();
	Refresh(Facility);
	auto* Sparks = Facility->FindComponentByClass<UParticleSystemComponent>();
	auto* Interaction = Facility->FindComponentByClass<UTunaSweeperInteractableComponent>();
	if (!TestNotNull(TEXT("Fault sparks component exists"), Sparks) ||
		!TestNotNull(TEXT("Interaction component exists"), Interaction)) return false;
	auto* Effect = LoadObject<UParticleSystem>(nullptr, TEXT("/Game/Interaction/PowerFacility/P_PowerFacility_Sparks.P_PowerFacility_Sparks"));
	if (!TestNotNull(TEXT("Actual looping fault effect loads"), Effect)) return false;
	Sparks->SetTemplate(Effect);
	Refresh(Facility);
	if (FApp::CanEverRender()) TestTrue(TEXT("Broken facility activates actual spark simulation"), Sparks->IsActive());
	TestTrue(TEXT("Broken facility shows sparks"), Sparks->IsVisible());
	Bool(Facility, TEXT("bRequireActiveQuest"), true);
	Refresh(Facility);
	TestFalse(TEXT("Missing quest prevents repair"), Repair(Facility));
	TestTrue(TEXT("Quest gating never hides fault effect"), Sparks->IsVisible());
	Bool(Facility, TEXT("bRequireActiveQuest"), false);
	Bool(Facility, TEXT("bRequiresItem"), true);
	Refresh(Facility);
	TestFalse(TEXT("Missing repair material prevents repair"), Repair(Facility));
	TestTrue(TEXT("Failed repair keeps sparks"), Sparks->IsVisible());
	Bool(Facility, TEXT("bRequiresItem"), false);
	Refresh(Facility);
	TestTrue(TEXT("Configured repair succeeds"), Repair(Facility));
	TestFalse(TEXT("Successful repair immediately hides sparks"), Sparks->IsVisible());
	TestFalse(TEXT("Successful repair deactivates particle simulation"), Sparks->IsActive());
	TestFalse(TEXT("Repair keeps original actor"), Facility->IsActorBeingDestroyed());
	TestEqual(TEXT("Completed facility has no repair prompt"), Interaction->GetInteractionType(), ETunaSweeperInteractionType::None);
	TestFalse(TEXT("Repeated repair is rejected"), Repair(Facility));
	FTunaSweeperWorldProgressSaveData Saved;
	TestTrue(TEXT("Completion enters existing save state"), Game->TryGetWorldProgressState(TEXT("test.power.a"), Saved));
	TestEqual(TEXT("Saved facility is complete"), Saved.State, ETunaSweeperWorldProgressState::Completed);
	auto* Reloaded = World->SpawnActor<AActor>(Class);
	Reloaded->FindComponentByClass<UParticleSystemComponent>()->SetTemplate(Effect);
	Name(Reloaded, TEXT("ProgressObjectId"), TEXT("test.power.a"));
	Refresh(Reloaded);
	TestFalse(TEXT("Recreated facility restores sparks off"), Reloaded->FindComponentByClass<UParticleSystemComponent>()->IsVisible());
	TestFalse(TEXT("Restored completed facility never activates sparks"), Reloaded->FindComponentByClass<UParticleSystemComponent>()->IsActive());
	auto* Other = World->SpawnActor<AActor>(Class);
	Name(Other, TEXT("ProgressObjectId"), TEXT("test.power.b"));
	Refresh(Other);
	TestTrue(TEXT("Separate facility remains broken"), Other->FindComponentByClass<UParticleSystemComponent>()->IsVisible());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperPowerFacilityMaterialsTest,
	"TunaSweeper.Interaction.PowerFacility.MaterialTransaction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaSweeperPowerFacilityMaterialsTest::RunTest(const FString& Parameters)
{
	// Catches partial payment, duplicate undercharging, and completion leaking out of failed saves.
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
	Context.SetCurrentWorld(World);
	TStrongObjectPtr<UTunaSweeperGameInstance> Game(NewObject<UTunaSweeperGameInstance>(GEngine));
	PowerFacilityTests::FContextAccess::Attach(Game.Get(), &Context);
	Context.OwningGameInstance = Game.Get();
	World->SetGameInstance(Game.Get());
	Game->bInventoryStateInitialized = true;
	Game->RetiredDemoSaveSlotIndex = Game->ActiveSaveSlotIndex; // Refuse saves before touching user files.
	Game->UGameInstance::Init();
	ON_SCOPE_EXIT
	{
		World->DestroyWorld(false);
		Game->UGameInstance::Shutdown();
		World->SetGameInstance(nullptr);
		Context.OwningGameInstance = nullptr;
		PowerFacilityTests::FContextAccess::Attach(Game.Get(), nullptr);
		GEngine->DestroyWorldContext(World);
		World->RemoveFromRoot();
	};
	Game->ResetPlayerSlotArrays();
	Game->WorldProgressStatesById.Reset();
	auto* Facility = World->SpawnActor<ATunaSweeperPowerFacilityActor>();
	if (!TestNotNull(TEXT("Material facility spawns"), Facility)) return false;
	Facility->ProgressObjectId = TEXT("test.facility.materials");
	Facility->bRequireActiveQuest = false;
	Facility->RequiredItemId = 3004; // Nonempty array must override the legacy requirement.
	Facility->RequiredItems = {{6020, 1}, {6002, 1}, {6002, 1}};
	TestTrue(TEXT("Supply the first material"), Game->AddItemToFirstAvailableInventorySlot(6020, 2));
	TestTrue(TEXT("Supply only part of the second material"), Game->AddItemToFirstAvailableInventorySlot(6002, 1));
	TestFalse(TEXT("Duplicate requirements are aggregated before checking ownership"), Facility->CanRepair());
	TestFalse(TEXT("Missing material rejects repair"), Facility->Repair(false));
	TestEqual(TEXT("Missing material keeps the first stack"), Game->CountInventoryItemById(6020), 2);
	TestEqual(TEXT("Missing material keeps the second stack"), Game->CountInventoryItemById(6002), 1);
	TestTrue(TEXT("Supply all materials"), Game->AddItemToFirstAvailableInventorySlot(6002, 2));
	Facility->bRequireActiveQuest = true;
	TestFalse(TEXT("Materials never bypass the quest gate"), Facility->Repair(false));
	Facility->bRequireActiveQuest = false;
	for (const auto& Invalid : TArray<FTunaSweeperItemStack>{{0, 1}, {6020, 0}, {6020, -1}, {6020, MAX_int32}})
	{
		Facility->RequiredItems.Add(Invalid);
		TestFalse(TEXT("Invalid or overflowing requirements fail closed"), Facility->Repair(false));
		Facility->RequiredItems.Pop();
	}
	int32 ProgressNotifications = 0;
	int32 InventoryNotifications = 0;
	Game->OnWorldProgressChanged.AddLambda([&] { ++ProgressNotifications; });
	Game->OnInventoryStateChanged.AddLambda([&] { ++InventoryNotifications; });
	TestTrue(TEXT("Complete materials allow repair without the legacy item"), Facility->CanRepair());
	TestFalse(TEXT("Failed save aborts repair"), Facility->Repair(true));
	TestEqual(TEXT("Failed save restores first stack"), Game->CountInventoryItemById(6020), 2);
	TestEqual(TEXT("Failed save restores second stack"), Game->CountInventoryItemById(6002), 3);
	TestFalse(TEXT("Failed save keeps facility broken"), Facility->IsRepaired());
	TestFalse(TEXT("Failed save leaves no synthetic world progress"), Game->WorldProgressStatesById.Contains(Facility->ProgressObjectId));
	TestEqual(TEXT("Failed save publishes no completion"), ProgressNotifications, 0);
	TestEqual(TEXT("Failed save publishes no inventory changes"), InventoryNotifications, 0);
	TestTrue(TEXT("Retry repairs with every material"), Facility->Repair(false));
	TestEqual(TEXT("Repair consumes first requirement once"), Game->CountInventoryItemById(6020), 1);
	TestEqual(TEXT("Repair consumes the aggregated second requirement"), Game->CountInventoryItemById(6002), 1);
	TestEqual(TEXT("Successful repair publishes completion once"), ProgressNotifications, 1);
	TestEqual(TEXT("Successful repair publishes inventory once"), InventoryNotifications, 1);
	TestFalse(TEXT("Repeated repair consumes nothing"), Facility->Repair(false));
	TestEqual(TEXT("Repeat retains first remaining material"), Game->CountInventoryItemById(6020), 1);
	TestEqual(TEXT("Repeat retains second remaining material"), Game->CountInventoryItemById(6002), 1);
	auto* Restored = World->SpawnActor<ATunaSweeperPowerFacilityActor>();
	Restored->ProgressObjectId = Facility->ProgressObjectId;
	Restored->RefreshFacilityState();
	TestTrue(TEXT("Recreated facility restores completion"), Restored->IsRepaired());
	auto* Legacy = World->SpawnActor<ATunaSweeperPowerFacilityActor>();
	Legacy->ProgressObjectId = TEXT("test.facility.legacy");
	Legacy->bRequireActiveQuest = false;
	Legacy->RequiredItemId = 6020;
	TestTrue(TEXT("Empty array retains the legacy single-item repair"), Legacy->Repair(false));
	TestEqual(TEXT("Legacy repair consumes its single material"), Game->CountInventoryItemById(6020), 0);
	auto* Reusable = World->SpawnActor<ATunaSweeperPowerFacilityActor>();
	Reusable->ProgressObjectId = TEXT("test.facility.reusable");
	Reusable->bRequireActiveQuest = false;
	Reusable->bConsumeRequiredItem = false;
	Reusable->RequiredItems = {{6002, 1}};
	TestTrue(TEXT("Nonconsuming requirements allow repair"), Reusable->Repair(false));
	TestEqual(TEXT("Nonconsuming repair keeps its material"), Game->CountInventoryItemById(6002), 1);
	Game->OnWorldProgressChanged.Clear();
	Game->OnInventoryStateChanged.Clear();
	return true;
}
#endif
