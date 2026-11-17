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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperItemGrantCheatTest,
 "TunaSweeper.Debug.ItemGrant.TransactionAndProtocol", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTunaSweeperItemGrantCheatTest::RunTest(const FString& Parameters)
{
 using EResult = ETunaSweeperGateCheatResult;
 const FString Root = FPaths::ProjectSavedDir() / TEXT("Automation/WaveDefense") / FGuid::NewGuid().ToString();
 if (!TestTrue(TEXT("Isolated save"), TunaSweeperBuildFlavor::SetWaveDefenseTestSaveDirectory(Root))) return false;
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
 TunaSweeperMovementCheatControl.EnableToken = 0x54534D43;
 TestEqual(TEXT("No implicit save initialization"), Game->TryGrantItemForHelper(6019, 1), uint32(EResult::NoContext));
 Game->bInventoryStateInitialized = true; Game->ResetPlayerSlotArrays();
 const FGuid Carried = Game->CreateItemInstance(3004, 2);
 Game->PlayerInventorySlots[0].ItemUid = Carried;
 auto* Cheats = Game->GetSubsystem<UTunaSweeperGateCheatSubsystem>();
 if (!TestNotNull(TEXT("Subsystem"), Cheats)) return false;
 Cheats->Tick(0);
 auto& Wire = TunaSweeperGateCheatControl;
 uint64 Sequence = Wire.Snapshot.AckSequence;
 auto Request = [&](int32 Id, int32 Quantity, uint64 Epoch, int64 Offset = 4000) {
  Wire.Request = {}; const FDateTime Now = FDateTime::UtcNow();
  Wire.Request.DeadlineUtcMs = Now.ToUnixTimestamp() * 1000 + Now.GetMillisecond() + Offset;
  Wire.Request.ContextEpoch = Epoch; Wire.Request.RowKey = (uint64(uint32(Quantity)) << 32) | uint32(Id);
  Wire.Request.Operation = 4; Wire.Request.ProcessId = FPlatformProcess::GetCurrentProcessId(); Wire.Request.Sequence = ++Sequence;
  Cheats->Tick(0); return Wire.Snapshot.Result;
 };
 uint64 Epoch = Wire.Snapshot.ContextEpoch;
 TestEqual(TEXT("Unknown ID"), Request(MAX_int32, 1, Epoch), uint32(EResult::InvalidRequest));
 TestEqual(TEXT("Zero quantity"), Request(6019, 0, Epoch), uint32(EResult::InvalidRequest));
 TestEqual(TEXT("Quantity limit"), Request(6019, 1000001, Epoch), uint32(EResult::InvalidRequest));
 TestEqual(TEXT("Stale context"), Request(6019, 1, Epoch + 99), uint32(EResult::StaleContext));
 TestEqual(TEXT("Expired"), Request(6019, 1, Epoch, -1000), uint32(EResult::Expired));
 TunaSweeperMovementCheatControl.EnableToken = 0;
 TestEqual(TEXT("Disabled"), Request(6019, 1, Epoch), uint32(EResult::Disabled));
 TunaSweeperMovementCheatControl.EnableToken = 0x54534D43; Cheats->Tick(0); Epoch = Wire.Snapshot.ContextEpoch;
 Game->bSuppressSaveWritesForTests = true;
 TestEqual(TEXT("Save failure"), Request(6019, 1, Epoch), uint32(EResult::SaveFailed));
 TestEqual(TEXT("Save failure rolls back instances"), Game->ItemInstancesByUid.Num(), 1);
 TestFalse(TEXT("Save failure rolls back acquired flag"), Game->EverAcquiredItemIds.Contains(6019));
 Game->bSuppressSaveWritesForTests = false;
 TestEqual(TEXT("Rocket granted"), Request(6019, 1, Epoch), uint32(EResult::ItemGranted));
 TestEqual(TEXT("One rocket"), Game->CountInventoryItemById(6019), 1);
 Wire.Request.Sequence = Sequence - 1; Cheats->Tick(0);
 TestEqual(TEXT("Replay rejected"), Wire.Snapshot.Result, uint32(EResult::Replay));
 TestEqual(TEXT("No duplicate rocket"), Game->CountInventoryItemById(6019), 1);
 TestEqual(TEXT("Stacked material"), Request(6002, 3, Epoch), uint32(EResult::ItemGranted));
 const int32 BeforeCount = Game->ItemInstancesByUid.Num();
 TestEqual(TEXT("Full inventory fails atomically"), Request(6019, 1000000, Epoch), uint32(EResult::InventoryFull));
 TestEqual(TEXT("No partial grant"), Game->ItemInstancesByUid.Num(), BeforeCount);
 TestEqual(TEXT("Existing carried item preserved"), Game->ItemInstancesByUid[Carried].Quantity, 2);
 TestTrue(TEXT("Reload save"), Game->LoadGameState());
 TestEqual(TEXT("Rocket persisted"), Game->CountInventoryItemById(6019), 1);
 TestEqual(TEXT("Material persisted"), Game->CountInventoryItemById(6002), 3);
 return true;
}
#endif
