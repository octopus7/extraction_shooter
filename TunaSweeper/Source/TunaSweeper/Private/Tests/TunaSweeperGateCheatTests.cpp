#if WITH_DEV_AUTOMATION_TESTS
#include "Debug/TunaSweeperGateCheatSubsystem.h"
#include "Debug/TunaSweeperMovementCheatSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Game/TunaSweeperGameInstance.h"
#include "GameFramework/Pawn.h"
#include "Interaction/TunaSweeperChainLinkGate.h"
#include "Interaction/TunaSweeperGateLockControl.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
#include "HAL/PlatformProcess.h"
#include "Settings/TunaSweeperBuildFlavor.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperGateCheatTest,
 "TunaSweeper.Debug.GateCheats.TransactionAndProtocol", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTunaSweeperGateCheatTest::RunTest(const FString& Parameters)
{
 const FString Root = FPaths::ProjectSavedDir() / TEXT("Automation/WaveDefense") / FGuid::NewGuid().ToString();
 if (!TestTrue(TEXT("Isolated save root"), TunaSweeperBuildFlavor::SetWaveDefenseTestSaveDirectory(Root))) return false;
 ON_SCOPE_EXIT { TunaSweeperBuildFlavor::SetWaveDefenseTestSaveDirectory(FString()); };
 struct FAccess : UGameInstance { static void Attach(UGameInstance* G, FWorldContext* C) { auto M = &FAccess::WorldContext; G->*M = C; } };
 UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
 FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
 Context.SetCurrentWorld(World);
 auto* Game = NewObject<UTunaSweeperGameInstance>(GEngine);
 FAccess::Attach(Game, &Context); Context.OwningGameInstance = Game; World->SetGameInstance(Game);
 Game->UGameInstance::Init();
 const uint32 OldToken = TunaSweeperMovementCheatControl.EnableToken;
 ON_SCOPE_EXIT {
  TunaSweeperMovementCheatControl.EnableToken = OldToken;
  Game->UGameInstance::Shutdown(); World->SetGameInstance(nullptr); Context.OwningGameInstance = nullptr;
  FAccess::Attach(Game, nullptr); World->DestroyWorld(false); GEngine->DestroyWorldContext(World); World->RemoveFromRoot();
 };
 auto* Cheats = Game->GetSubsystem<UTunaSweeperGateCheatSubsystem>();
 if (!TestNotNull(TEXT("Subsystem"), Cheats)) return false;
 TestTrue(TEXT("Paused service"), Cheats->IsTickableWhenPaused());
 TunaSweeperMovementCheatControl.EnableToken = 0x54534D43;
 Cheats->Tick(0);
 TestFalse(TEXT("Idle polling never initializes save"), Game->IsGateCheatSaveReady());
 TestTrue(TEXT("Isolated slot selected"), Game->SetActiveSaveSlotIndex(1));
 auto* Gate = World->SpawnActor<ATunaSweeperChainLinkGate>();
 const auto* Id = FindFProperty<FNameProperty>(Gate->GetClass(), TEXT("GateId"));
 Id->SetPropertyValue_InContainer(Gate, FName(TEXT("gate_reset_test"))); Gate->DispatchBeginPlay();
 auto* Control = World->SpawnActor<ATunaSweeperGateLockControl>();
 FindFProperty<FObjectPropertyBase>(Control->GetClass(), TEXT("LinkedGate"))->SetObjectPropertyValue_InContainer(Control, Gate);
 Control->DispatchBeginPlay();
 auto* Pawn = World->SpawnActor<APawn>();
 TestTrue(TEXT("Control opens gate"), Control->RequestInteraction(Pawn)); Gate->Tick(0.5f);
 const auto Yaw = [Gate]() { return CastChecked<USceneComponent>(FindFProperty<FObjectPropertyBase>(Gate->GetClass(), TEXT("Hinge"))->GetObjectPropertyValue_InContainer(Gate))->GetRelativeRotation().Yaw; };
 const double HalfYaw = Yaw();
 const FName Other(TEXT("test.unrelated.flag")); Game->CompletedScenarioFlags.Add(Other);
 Cheats->Tick(0); auto& Wire = TunaSweeperGateCheatControl;
 TestEqual(TEXT("Ready context"), Wire.Snapshot.State, uint32(0));
 TestEqual(TEXT("One gate"), Wire.Snapshot.Count, uint32(1));
 const uint64 Key = Wire.Snapshot.Rows[0].Key;
 auto* OtherGate = World->SpawnActor<ATunaSweeperChainLinkGate>(FVector(0, 2000, 0), FRotator::ZeroRotator);
 Id->SetPropertyValue_InContainer(OtherGate, FName(TEXT("unrelated_gate"))); OtherGate->DispatchBeginPlay();
 TestTrue(TEXT("Other gate opens independently"), OtherGate->Unlock()); OtherGate->Tick(1);
 uint64 Sequence = Wire.Snapshot.AckSequence;
 auto Request = [&](uint64 Epoch, int64 Offset = 4000) {
  Wire.Request.Sequence = 0;
  const FDateTime Now = FDateTime::UtcNow();
  Wire.Request.DeadlineUtcMs = Now.ToUnixTimestamp() * 1000 + Now.GetMillisecond() + Offset;
  Wire.Request.ContextEpoch = Epoch; Wire.Request.RowKey = Key; Wire.Request.Operation = 2;
  Wire.Request.ProcessId = FPlatformProcess::GetCurrentProcessId(); Wire.Request.Sequence = ++Sequence;
  Cheats->Tick(0); return Wire.Snapshot.Result;
 };
 uint64 Epoch = Wire.Snapshot.ContextEpoch;
 TestEqual(TEXT("Expired"), Request(Epoch, -1000), uint32(ETunaSweeperGateCheatResult::Expired));
 TestEqual(TEXT("Stale"), Request(Epoch + 100), uint32(ETunaSweeperGateCheatResult::StaleContext));
 TunaSweeperMovementCheatControl.EnableToken = 0;
 TestEqual(TEXT("Disabled"), Request(Epoch), uint32(ETunaSweeperGateCheatResult::Disabled));
 TunaSweeperMovementCheatControl.EnableToken = 0x54534D43; Cheats->Tick(0); Epoch = Wire.Snapshot.ContextEpoch;
 Game->bSuppressSaveWritesForTests = true;
 TestEqual(TEXT("Save failure"), Request(Epoch), uint32(ETunaSweeperGateCheatResult::SaveFailed));
 TestTrue(TEXT("Flag rolls back"), Game->IsScenarioProgressFlagSet(Gate->GetPersistentUnlockFlag()));
 TestEqual(TEXT("Pose unchanged"), Yaw(), HalfYaw); TestTrue(TEXT("Animation unchanged"), Gate->IsActorTickEnabled());
 Game->bSuppressSaveWritesForTests = false;
 TestEqual(TEXT("Reset success"), Request(Epoch), uint32(ETunaSweeperGateCheatResult::Reset));
 TestFalse(TEXT("Locked"), Gate->IsUnlocked()); TestEqual(TEXT("Closed"), Yaw(), 0.0);
 TestFalse(TEXT("Animation stopped"), Gate->IsActorTickEnabled()); TestTrue(TEXT("Control reusable"), Control->CanOpenGate());
 TestEqual(TEXT("Interaction restored"), Gate->GetInteractionType(), ETunaSweeperInteractionType::ChainLinkGate);
 TestTrue(TEXT("Reload reset"), Game->LoadGameState());
 TestFalse(TEXT("Reset persisted"), Game->IsScenarioProgressFlagSet(Gate->GetPersistentUnlockFlag()));
 TestTrue(TEXT("Other flag persisted"), Game->IsScenarioProgressFlagSet(Other));
 TestTrue(TEXT("Other gate runtime untouched"), OtherGate->IsUnlocked());
 TestTrue(TEXT("Other gate save untouched"), Game->IsScenarioProgressFlagSet(OtherGate->GetPersistentUnlockFlag()));
 Cheats->Tick(0); TestNotEqual(TEXT("Load changes epoch"), Wire.Snapshot.ContextEpoch, Epoch);
 TestEqual(TEXT("Old context rejected"), Request(Epoch), uint32(ETunaSweeperGateCheatResult::StaleContext));
 TestTrue(TEXT("Control reopens"), Control->RequestInteraction(Pawn));
 Wire.Request.Sequence = Sequence - 1; Cheats->Tick(0);
 TestEqual(TEXT("Replay rejected"), Wire.Snapshot.Result, uint32(ETunaSweeperGateCheatResult::Replay));
 TestTrue(TEXT("Replay leaves gate open"), Gate->IsUnlocked());
 auto* Duplicate = World->SpawnActor<ATunaSweeperChainLinkGate>();
 Id->SetPropertyValue_InContainer(Duplicate, FName(TEXT("gate_reset_test"))); Duplicate->DispatchBeginPlay();
 Cheats->Tick(0);
 TestEqual(TEXT("Duplicate identity rejected"), Request(Wire.Snapshot.ContextEpoch), uint32(ETunaSweeperGateCheatResult::DuplicateId));
 Duplicate->Destroy(); Cheats->Tick(0);
 Gate->Tick(1);
 TestEqual(TEXT("Fully open reset"), Request(Wire.Snapshot.ContextEpoch), uint32(ETunaSweeperGateCheatResult::Reset));
 TestEqual(TEXT("Fully open reset closes pose"), Yaw(), 0.0);
 TestEqual(TEXT("Already closed reset"), Request(Wire.Snapshot.ContextEpoch), uint32(ETunaSweeperGateCheatResult::Reset));
 const uint64 BeforeAmbiguity = Wire.Snapshot.ContextEpoch;
 UWorld* SecondWorld = UWorld::CreateWorld(EWorldType::PIE, false);
 GEngine->CreateNewWorldContext(EWorldType::PIE).SetCurrentWorld(SecondWorld);
 Cheats->Tick(0);
 TestEqual(TEXT("Multiple gameplay worlds fail closed"), Wire.Snapshot.State, uint32(ETunaSweeperGateCheatResult::AmbiguousContext));
 TestEqual(TEXT("Ambiguous request rejected"), Request(BeforeAmbiguity), uint32(ETunaSweeperGateCheatResult::AmbiguousContext));
 SecondWorld->DestroyWorld(false); GEngine->DestroyWorldContext(SecondWorld); SecondWorld->RemoveFromRoot();
 Cheats->Tick(0);
 TestEqual(TEXT("Prior context rejected after ambiguity clears"), Request(BeforeAmbiguity), uint32(ETunaSweeperGateCheatResult::StaleContext));
 Gate->Destroy();
 auto* Replacement = World->SpawnActor<ATunaSweeperChainLinkGate>();
 Id->SetPropertyValue_InContainer(Replacement, FName(TEXT("gate_reset_test"))); Replacement->DispatchBeginPlay();
 Cheats->Tick(0);
 TestEqual(TEXT("Destroyed actor key cannot target replacement with same ID"), Request(Wire.Snapshot.ContextEpoch), uint32(ETunaSweeperGateCheatResult::MissingGate));
 const uint64 BeforeSlot = Wire.Snapshot.ContextEpoch;
 TestTrue(TEXT("Select isolated second slot"), Game->SetActiveSaveSlotIndex(2));
 Cheats->Tick(0);
 TestEqual(TEXT("Uninitialized selected slot unavailable"), Wire.Snapshot.State, uint32(ETunaSweeperGateCheatResult::NoContext));
 Game->GetInventorySlots(); Cheats->Tick(0);
 TestEqual(TEXT("Old slot selection rejected"), Request(BeforeSlot), uint32(ETunaSweeperGateCheatResult::StaleContext));
 TestFalse(TEXT("Second slot has no first slot progress"), Game->IsScenarioProgressFlagSet(Other));
 TestFalse(TEXT("Second slot has no first slot gate flag"), Game->IsScenarioProgressFlagSet(OtherGate->GetPersistentUnlockFlag()));
 TestTrue(TEXT("Return to first slot"), Game->SetActiveSaveSlotIndex(1)); Game->GetInventorySlots();
 TestTrue(TEXT("First slot unrelated progress retained"), Game->IsScenarioProgressFlagSet(Other));
 TestTrue(TEXT("First slot unrelated gate retained"), Game->IsScenarioProgressFlagSet(OtherGate->GetPersistentUnlockFlag()));
 return true;
}
#endif
