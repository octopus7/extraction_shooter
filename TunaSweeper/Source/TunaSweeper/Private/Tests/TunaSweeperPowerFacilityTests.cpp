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
#endif
