#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Debug/TunaSweeperGateCheatSubsystem.h"
#include "Debug/TunaSweeperMovementCheatSubsystem.h"
#include "Settings/TunaSweeperBuildFlavor.h"
#include "HAL/PlatformProcess.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperTunaFillCheatTest,
 "TunaSweeper.Debug.TunaFill.TransactionAndProtocol", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTunaSweeperTunaFillCheatTest::RunTest(const FString& Parameters)
{
 const FString Root = FPaths::ProjectSavedDir() / TEXT("Automation/WaveDefense") / FGuid::NewGuid().ToString();
 if (!TestTrue(TEXT("Isolated saves"), TunaSweeperBuildFlavor::SetWaveDefenseTestSaveDirectory(Root))) return false;
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
 TestFalse(TEXT("Uninitialized API fails without initializing save"), Game->TryFillRaidStorageForTrueEndingTest());
 TestFalse(TEXT("No setup side effect"), Game->IsGateCheatSaveReady());
 Game->bInventoryStateInitialized = true;
 Game->ResetPlayerSlotArrays(); Game->StorageSlots.SetNum(100); Game->RaidStorageSlots.SetNum(10);
 const FGuid Carried = Game->CreateItemInstance(3004, 2);
 const FGuid Base = Game->CreateItemInstance(6035, 3);
 const FGuid Old = Game->CreateItemInstance(6035, 4);
 const FGuid Child = Game->CreateItemInstance(6035, 1);
 const FGuid Equipped = Game->CreateItemInstance(6035, 1);
 const FGuid Quick = Game->CreateItemInstance(3004, 3);
 Game->EquipmentSlots[0].ItemUid = Equipped; Game->UsableQuickSlots[0].ItemUid = Quick;
 Game->ItemInstancesByUid[Old].AttachmentSlots.Add(TEXT("test.child"), Child);
 Game->PlayerInventorySlots[0].ItemUid = Carried; Game->StorageSlots[0].ItemUid = Base;
 Game->RaidStorageSlots[0].ItemUid = Old; Game->RaidStorageSlots[1].ItemUid = Base;
 Game->ActiveLootContainerSlots = Game->RaidStorageSlots;
 Game->bHasActiveLootContainer = true; Game->bActiveLootContainerIsRaidStorage = true;
 Game->ActiveLootContainerCapacity = 10;
 Game->bPendingBunkerItemStateSave = true; Game->bPendingRaidStorageSave = true;
 Game->TotalExperiencePoints = 100; Game->PendingRaidExperiencePoints = 30;
 Game->CompletedScenarioFlags.Add(TEXT("test.unrelated.flag"));
 const int32 BeforeCount = Game->ItemInstancesByUid.Num();
 auto* Cheats = Game->GetSubsystem<UTunaSweeperGateCheatSubsystem>();
 if (!TestNotNull(TEXT("Subsystem"), Cheats)) return false;
 TunaSweeperMovementCheatControl.EnableToken = 0x54534D43; Cheats->Tick(0);
 auto& Wire = TunaSweeperGateCheatControl;
 uint64 Sequence = Wire.Snapshot.AckSequence;
 auto Request = [&](uint64 Epoch, int64 Offset = 4000, uint64 Key = 0) {
  Wire.Request = {}; const FDateTime Now = FDateTime::UtcNow();
  Wire.Request.DeadlineUtcMs = Now.ToUnixTimestamp() * 1000 + Now.GetMillisecond() + Offset;
  Wire.Request.ContextEpoch = Epoch; Wire.Request.RowKey = Key; Wire.Request.Operation = 3;
  Wire.Request.ProcessId = FPlatformProcess::GetCurrentProcessId(); Wire.Request.Sequence = ++Sequence;
  Cheats->Tick(0); return Wire.Snapshot.Result;
 };
 uint64 Epoch = Wire.Snapshot.ContextEpoch;
 TestEqual(TEXT("Expired"), Request(Epoch, -1000), uint32(ETunaSweeperGateCheatResult::Expired));
 TestEqual(TEXT("Stale"), Request(Epoch + 100), uint32(ETunaSweeperGateCheatResult::StaleContext));
 TestEqual(TEXT("Nonzero row rejected"), Request(Epoch, 4000, 1), uint32(ETunaSweeperGateCheatResult::InvalidRequest));
 TunaSweeperMovementCheatControl.EnableToken = 0;
 TestEqual(TEXT("Disabled"), Request(Epoch), uint32(ETunaSweeperGateCheatResult::Disabled));
 TunaSweeperMovementCheatControl.EnableToken = 0x54534D43; Cheats->Tick(0); Epoch = Wire.Snapshot.ContextEpoch;
 Game->bSuppressSaveWritesForTests = true;
 TestEqual(TEXT("Save failure"), Request(Epoch), uint32(ETunaSweeperGateCheatResult::SaveFailed));
 TestEqual(TEXT("Rollback item count"), Game->ItemInstancesByUid.Num(), BeforeCount);
 TestTrue(TEXT("Rollback restores old graph"), Game->ItemInstancesByUid.Contains(Old) && Game->ItemInstancesByUid.Contains(Child));
 TestEqual(TEXT("Rollback warehouse"), Game->RaidStorageSlots[0].ItemUid, Old);
 TestEqual(TEXT("Rollback open warehouse"), Game->ActiveLootContainerSlots[0].ItemUid, Old);
 TestFalse(TEXT("Rollback initialized flag"), Game->bRaidStorageInitialized);
 TestTrue(TEXT("Rollback dirty flags"), Game->bPendingBunkerItemStateSave && Game->bPendingRaidStorageSave);
 Game->bSuppressSaveWritesForTests = false;
 TestEqual(TEXT("Fill acknowledged"), Request(Epoch), uint32(14));
 TestTrue(TEXT("Normal ending predicate qualifies"), Game->IsRaidStorageReadyForTrueEnding());
 TSet<FGuid> Unique;
 for (int32 I = 0; I < Game->RaidStorageSlots.Num(); ++I) {
  const FGuid Uid = Game->RaidStorageSlots[I].ItemUid; Unique.Add(Uid);
  TestEqual(TEXT("Open warehouse synchronized"), Game->ActiveLootContainerSlots[I].ItemUid, Uid);
  TestEqual(TEXT("Tuna item"), Game->ItemInstancesByUid[Uid].ItemId, 3004);
  TestEqual(TEXT("Full stack"), Game->ItemInstancesByUid[Uid].Quantity, 10);
 }
 TestEqual(TEXT("Ten distinct stacks"), Unique.Num(), 10);
 TestFalse(TEXT("Old root removed"), Game->ItemInstancesByUid.Contains(Old));
 TestFalse(TEXT("Orphan child removed"), Game->ItemInstancesByUid.Contains(Child));
 TestTrue(TEXT("Shared base reference preserved"), Game->ItemInstancesByUid.Contains(Base));
 TestEqual(TEXT("Carried root preserved"), Game->PlayerInventorySlots[0].ItemUid, Carried);
 TestEqual(TEXT("Base root preserved"), Game->StorageSlots[0].ItemUid, Base);
 TestEqual(TEXT("Equipment preserved"), Game->EquipmentSlots[0].ItemUid, Equipped);
 TestEqual(TEXT("Quick slot preserved"), Game->UsableQuickSlots[0].ItemUid, Quick);
 TestEqual(TEXT("XP unchanged"), Game->TotalExperiencePoints, int64(100));
 TestEqual(TEXT("Pending XP unchanged"), Game->PendingRaidExperiencePoints, int64(30));
 TestEqual(TEXT("No ending flags granted"), Game->CompletedScenarioFlags.Num(), 1);
 const FGuid Filled = Game->RaidStorageSlots[0].ItemUid;
 Wire.Request.Sequence = Sequence - 1; Cheats->Tick(0);
 TestEqual(TEXT("Replay rejected"), Wire.Snapshot.Result, uint32(ETunaSweeperGateCheatResult::Replay));
 TestEqual(TEXT("Replay does not refill"), Game->RaidStorageSlots[0].ItemUid, Filled);
 Game->bActiveLootContainerIsRaidStorage = false; Game->bHasActiveLootContainer = false;
 TestTrue(TEXT("Reload"), Game->LoadGameState());
 TestTrue(TEXT("Reload eligible"), Game->IsRaidStorageReadyForTrueEnding());
 TestEqual(TEXT("Reload same stacks"), Game->RaidStorageSlots[0].ItemUid, Filled);
 TestEqual(TEXT("Reload carried quantity preserved"), Game->ItemInstancesByUid[Carried].Quantity, 2);
 TestEqual(TEXT("Reload XP unchanged"), Game->TotalExperiencePoints, int64(100));
 TestEqual(TEXT("Quick slot persisted"), Game->UsableQuickSlots[0].ItemUid, Quick);
 // A different container must not receive warehouse contents or lose its graph.
 const FGuid ContainerItem = Game->CreateItemInstance(6035, 7);
 Game->ActiveLootContainerSlots.SetNum(1); Game->ActiveLootContainerSlots[0].ItemUid = ContainerItem;
 Game->bHasActiveLootContainer = true; Game->bActiveLootContainerIsRaidStorage = false;
 TestTrue(TEXT("Fill with other container open"), Game->TryFillRaidStorageForTrueEndingTest());
 TestEqual(TEXT("Other container unchanged"), Game->ActiveLootContainerSlots[0].ItemUid, ContainerItem);
 TestEqual(TEXT("Other container quantity unchanged"), Game->ItemInstancesByUid[ContainerItem].Quantity, 7);
 return true;
}
#endif
