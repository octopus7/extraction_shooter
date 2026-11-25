#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "TunaSweeperGateCheatSubsystem.generated.h"

// Process-memory ABI v4. All strings are zero-terminated UTF-8. No reflected/save/config state.
enum class ETunaSweeperGateCheatResult : uint32
{
 None, Refreshed, Reset, Disabled, NoContext, AmbiguousContext, Expired,
 StaleContext, MissingGate, DuplicateId, SaveFailed, InvalidRequest, Replay, Overflow, TunaFilled, ItemGranted, InventoryFull, KillCountSaved
};
struct alignas(8) FTunaSweeperGateCheatRequest
{
 uint64 Sequence = 0; // Writer clears first, writes payload, then publishes a monotonically increasing sequence last.
 int64 DeadlineUtcMs = 0;
 uint64 ContextEpoch = 0;
 uint64 RowKey = 0;
 uint32 Operation = 0; // 1 refresh; 2 reset gate; 3 fill tuna; 4 grant item (RowKey low32 ID, high32 quantity).
 uint32 ProcessId = 0;
 uint64 ExpectedKillCount = 0; // Operation 5 compares this value before setting the total to RowKey.
 uint64 Reserved[2] = {};
};
struct alignas(8) FTunaSweeperGateCheatRow
{
 uint64 Key = 0;
 uint32 Flags = 0; // 1 unlocked; 2 duplicate persistent ID; 4 missing/oversize ID.
 uint32 Reserved = 0;
 char GateId[128] = {};
 char Level[256] = {};
};
struct alignas(8) FTunaSweeperGateCheatSnapshot
{
 uint64 Revision = 0; // Seqlock: accept matching even revisions before/after reading entire snapshot.
 uint64 AckSequence = 0;
 uint64 ContextEpoch = 0;
 int64 PublishedUtcMs = 0;
 uint32 Result = 0;
 uint32 State = 0; // 0 ready; otherwise Disabled/NoContext/AmbiguousContext.
 int32 Slot = 0;
 uint32 Count = 0;
 char Map[128] = {};
 FTunaSweeperGateCheatRow Rows[64];
 int64 TotalEnemyKills = 0;
};
struct alignas(8) FTunaSweeperGateCheatControl
{
 uint64 Magic = 0x5453474154454348ULL;
 uint32 Version = 4;
 uint32 Size = 25872;
 uint32 ProcessId = 0;
 uint32 Reserved = 0;
 FTunaSweeperGateCheatRequest Request;
 FTunaSweeperGateCheatSnapshot Snapshot;
};
#if IS_MONOLITHIC
extern "C" DLLEXPORT FTunaSweeperGateCheatControl TunaSweeperGateCheatControl;
#else
extern "C" TUNASWEEPER_API FTunaSweeperGateCheatControl TunaSweeperGateCheatControl;
#endif

UCLASS()
class TUNASWEEPER_API UTunaSweeperGateCheatSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
 GENERATED_BODY()
public:
 virtual void Initialize(FSubsystemCollectionBase& Collection) override;
 virtual void Deinitialize() override;
 virtual void Tick(float DeltaTime) override;
 virtual bool IsTickable() const override { return !IsTemplate(); }
 virtual bool IsTickableWhenPaused() const override { return true; }
 virtual TStatId GetStatId() const override;
 virtual UWorld* GetTickableGameObjectWorld() const override { return GetWorld(); }
};
