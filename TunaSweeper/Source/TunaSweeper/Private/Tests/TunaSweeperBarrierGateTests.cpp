#if WITH_DEV_AUTOMATION_TESTS
#include "Interaction/TunaSweeperBarrierGateActor.h"
#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "TimerManager.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperBarrierGateTest,
	"TunaSweeper.Interaction.BarrierGate.MotionAndProximity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaSweeperBarrierGateTest::RunTest(const FString& Parameters)
{
	const auto Values = UWorld::InitializationValues().AllowAudioPlayback(false).RequiresHitProxies(false)
		.CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, TEXT("BarrierTest"), GetTransientPackage(), true, ERHIFeatureLevel::Num, &Values);
	if (!TestNotNull(TEXT("World"), World)) { return false; }
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	ON_SCOPE_EXIT { World->DestroyWorld(false); GEngine->DestroyWorldContext(World); World->RemoveFromRoot(); };
	ATunaSweeperBarrierGateActor* Gate = World->SpawnActor<ATunaSweeperBarrierGateActor>();
	Gate->OpenGate();
	Gate->Tick(.6f);
	TestTrue(TEXT("Halfway opening"), FMath::IsNearlyEqual(Gate->GetOpenAlpha(), .5f));
	TestTrue(TEXT("Arm rises above hinge"), Gate->ArmCollision->GetComponentLocation().Z > 100);
	TestEqual(TEXT("Opening stays red"), Gate->GreenLight->Intensity, 0.f);
	Gate->OpenGate();
	TestTrue(TEXT("Repeated request preserves progress"), FMath::IsNearlyEqual(Gate->GetOpenAlpha(), .5f));
	Gate->Tick(.6f);
	TestEqual(TEXT("Open state"), Gate->GetGateState(), ETunaSweeperBarrierGateState::Open);
	TestTrue(TEXT("Open is green"), Gate->GreenLight->Intensity > 0 && Gate->RedLight->Intensity == 0);
	TestFalse(TEXT("Idle gate does not tick"), Gate->IsActorTickEnabled());
	Gate->CloseGate();
	Gate->Tick(.9f);
	TestTrue(TEXT("Halfway closing"), FMath::IsNearlyEqual(Gate->GetOpenAlpha(), .5f));
	Gate->OpenGate();
	Gate->Tick(.6f);
	TestTrue(TEXT("Reversal completes smoothly"), Gate->IsPassageClear());
	Gate->CloseGate();
	Gate->Tick(10.f);
	TestEqual(TEXT("Large frame clamps closed"), Gate->GetOpenAlpha(), 0.f);

	APlayerController* Controller = World->SpawnActor<APlayerController>();
	ACharacter* Player = World->SpawnActor<ACharacter>();
	Player->SetActorLocation(FVector(170, -200, 95));
	Controller->Possess(Player);
	Gate->RefreshProximity();
	TestEqual(TEXT("Initial nearby player opens gate"), Gate->GetGateState(), ETunaSweeperBarrierGateState::Opening);
	Gate->Tick(2);
	Gate->CloseGate();
	TestEqual(TEXT("Player prevents manual closing"), Gate->GetGateState(), ETunaSweeperBarrierGateState::Open);
	Player->SetActorLocation(FVector(5000, 0, 95));
	Gate->RefreshProximity();
	TestEqual(TEXT("Exit starts delay, not immediate close"), Gate->GetGateState(), ETunaSweeperBarrierGateState::Open);
	World->GetTimerManager().Tick(0.01f); // Timer enters the active heap on its first tick.
	++GFrameCounter; // The timer manager accepts one tick per engine frame.
	World->GetTimerManager().Tick(2.f);
	TestEqual(TEXT("Delay closes empty lane"), Gate->GetGateState(), ETunaSweeperBarrierGateState::Closing);
	Gate->Tick(.5f);
	Player->SetActorLocation(FVector(170, 200, 95));
	Gate->Tick(.1f);
	TestEqual(TEXT("Approach from other side reverses closing"), Gate->GetGateState(), ETunaSweeperBarrierGateState::Opening);
	Controller->UnPossess();
	Gate->CloseGate();
	TestEqual(TEXT("Unpossessed pawn does not activate sensor"), Gate->GetGateState(), ETunaSweeperBarrierGateState::Closing);
	Gate->Tick(10);
	Gate->OpenDuration = 0;
	Gate->OpenGate();
	Gate->Tick(1);
	TestTrue(TEXT("Invalid duration is clamped"), Gate->IsPassageClear());
	Gate->Destroy();
	UClass* BlueprintClass = LoadClass<ATunaSweeperBarrierGateActor>(nullptr,
		TEXT("/Game/Environment/BarrierGate/BP_BarrierGate.BP_BarrierGate_C"));
	if (!TestNotNull(TEXT("Saved BP loads"), BlueprintClass)) { return false; }
	Gate = World->SpawnActor<ATunaSweeperBarrierGateActor>(BlueprintClass);
	TestNotNull(TEXT("BP housing assigned on construction"), Gate->Housing->GetStaticMesh().Get());
	TestNotNull(TEXT("BP arm assigned on construction"), Gate->Arm->GetStaticMesh().Get());
	TestTrue(TEXT("Hinge uses imported front side"), Gate->ArmPivot->GetRelativeLocation().Equals(FVector(0, 36, 100)));
	Player->SetActorLocation(FVector(5000, 0, 95));
	Controller->Possess(Player);
	Gate->DispatchBeginPlay();
	TestEqual(TEXT("Empty lane starts closed"), Gate->GetGateState(), ETunaSweeperBarrierGateState::Closed);
	++GFrameCounter;
	World->GetTimerManager().Tick(.01f);
	Player->SetActorLocation(FVector(170, 200, 95));
	++GFrameCounter;
	World->GetTimerManager().Tick(.2f);
	TestEqual(TEXT("BeginPlay timer opens without explicit refresh"), Gate->GetGateState(), ETunaSweeperBarrierGateState::Opening);
	Gate->Tick(2);
	Player->Destroy();
	++GFrameCounter;
	World->GetTimerManager().Tick(.2f);
	++GFrameCounter;
	World->GetTimerManager().Tick(2.f);
	TestEqual(TEXT("Destroyed occupant releases lane"), Gate->GetGateState(), ETunaSweeperBarrierGateState::Closing);
	return true;
}
#endif
