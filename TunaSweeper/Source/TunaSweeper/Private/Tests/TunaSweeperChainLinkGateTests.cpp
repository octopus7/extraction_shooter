#if WITH_DEV_AUTOMATION_TESTS

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Game/TunaSweeperGameInstance.h"
#include "GameFramework/Pawn.h"
#include "Interaction/TunaSweeperChainLinkGate.h"
#include "Interaction/TunaSweeperGateLockControl.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
#include "Settings/TunaSweeperBuildFlavor.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperChainLinkGatePersistenceTest,
	"TunaSweeper.Interaction.ChainLinkGate.Persistence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaSweeperChainLinkGatePersistenceTest::RunTest(const FString& Parameters)
{
	const FString TestRoot = FPaths::ProjectSavedDir() / TEXT("Automation/WaveDefense") / FGuid::NewGuid().ToString();
	if (!TestTrue(TEXT("Isolated save directory"), TunaSweeperBuildFlavor::SetWaveDefenseTestSaveDirectory(TestRoot)))
	{
		return false;
	}
	ON_SCOPE_EXIT { TunaSweeperBuildFlavor::SetWaveDefenseTestSaveDirectory(FString()); };

	struct FContextAccess : UGameInstance
	{
		static void Attach(UGameInstance* Instance, FWorldContext* Context)
		{
			auto Member = &FContextAccess::WorldContext;
			Instance->*Member = Context;
		}
	};
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
	Context.SetCurrentWorld(World);
	auto* Game = NewObject<UTunaSweeperGameInstance>(GEngine);
	FContextAccess::Attach(Game, &Context);
	Context.OwningGameInstance = Game;
	World->SetGameInstance(Game);
	Game->UGameInstance::Init();
	ON_SCOPE_EXIT
	{
		Game->UGameInstance::Shutdown();
		World->SetGameInstance(nullptr);
		Context.OwningGameInstance = nullptr;
		FContextAccess::Attach(Game, nullptr);
		World->DestroyWorld(false);
		GEngine->DestroyWorldContext(World);
		World->RemoveFromRoot();
	};

	const auto* GateIdProperty = FindFProperty<FNameProperty>(ATunaSweeperChainLinkGate::StaticClass(), TEXT("GateId"));
	const auto* LinkProperty = FindFProperty<FObjectPropertyBase>(ATunaSweeperGateLockControl::StaticClass(), TEXT("LinkedGate"));
	check(GateIdProperty && LinkProperty);
	auto SpawnGate = [&](const FVector& Location)
	{
		auto* Gate = World->SpawnActor<ATunaSweeperChainLinkGate>(Location, FRotator::ZeroRotator);
		GateIdProperty->SetPropertyValue_InContainer(Gate, FName(TEXT("test_gate")));
		Gate->DispatchBeginPlay();
		return Gate;
	};
	const auto GateYaw = [](const ATunaSweeperChainLinkGate* Gate)
	{
		const auto* Hinge = FindFProperty<FObjectPropertyBase>(ATunaSweeperChainLinkGate::StaticClass(), TEXT("Hinge"));
		return CastChecked<USceneComponent>(Hinge->GetObjectPropertyValue_InContainer(Gate))->GetRelativeRotation().Yaw;
	};

	TestTrue(TEXT("Select an empty isolated slot"), Game->SetActiveSaveSlotIndex(1));
	auto* Gate = SpawnGate(FVector::ZeroVector);
	auto* Control = World->SpawnActor<ATunaSweeperGateLockControl>(FVector(-150, 0, 0), FRotator::ZeroRotator);
	LinkProperty->SetObjectPropertyValue_InContainer(Control, Gate);
	Control->DispatchBeginPlay();
	auto* Pawn = World->SpawnActor<APawn>(FVector(-40, 0, 0), FRotator::ZeroRotator);

	TestFalse(TEXT("New gate is locked"), Gate->IsUnlocked());
	TestFalse(TEXT("Locked message string resolves"),
		Game->ResolveLocalizedText(TEXT("ui.gate.locked"), FText::GetEmpty()).IsEmpty());
	TestTrue(TEXT("Direct interaction inspects the lock"), Gate->RequestInteraction(Pawn));
	TestFalse(TEXT("Inspecting never unlocks it"), Gate->IsUnlocked());
	TestEqual(TEXT("Closed leaf starts at zero"), GateYaw(Gate), 0.0);

	Game->bSuppressSaveWritesForTests = true;
	TestFalse(TEXT("Control reports a failed save"), Control->RequestInteraction(Pawn));
	TestFalse(TEXT("Failed save leaves the gate locked"), Gate->IsUnlocked());
	TestFalse(TEXT("Failed save never starts animation"), Gate->IsActorTickEnabled());
	Game->bSuppressSaveWritesForTests = false;

	TestTrue(TEXT("Control commits unlock"), Control->RequestInteraction(Pawn));
	TestTrue(TEXT("Gate is unlocked"), Gate->IsUnlocked());
	TestFalse(TEXT("Control unavailable after unlock"), Control->CanOpenGate());
	TestEqual(TEXT("First unlock has no instant rotation"), GateYaw(Gate), 0.0);
	Gate->Tick(0.5f);
	TestTrue(TEXT("Leaf passes through an intermediate angle"), GateYaw(Gate) > 0.0 && GateYaw(Gate) < 90.0);
	Gate->Tick(0.5f);
	TestTrue(TEXT("Leaf reaches 90 degrees"), FMath::IsNearlyEqual(GateYaw(Gate), 90.0));
	TestFalse(TEXT("Animation stops after fully opening"), Gate->IsActorTickEnabled());

	Game->CompletedScenarioFlags.Reset();
	TestTrue(TEXT("Read the immediate write from disk"), Game->LoadGameState());
	auto* ReenteredGate = SpawnGate(FVector(0, 1000, 0));
	TestTrue(TEXT("Reloaded gate starts fully open"), FMath::IsNearlyEqual(GateYaw(ReenteredGate), 90.0));
	TestFalse(TEXT("Reload does not play the opening animation"), ReenteredGate->IsActorTickEnabled());
	TestTrue(TEXT("Select another slot"), Game->SetActiveSaveSlotIndex(2));
	auto* OtherSlotGate = SpawnGate(FVector(0, 2000, 0));
	TestFalse(TEXT("Other slot starts locked"), OtherSlotGate->IsUnlocked());
	return true;
}

#endif
