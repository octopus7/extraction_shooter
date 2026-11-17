#include "Debug/TunaSweeperGateCheatSubsystem.h"
#include "Debug/TunaSweeperMovementCheatSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/Level.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Game/TunaSweeperGameInstance.h"
#include "HAL/PlatformProcess.h"
#include "HAL/PlatformMisc.h"
#include "Interaction/TunaSweeperChainLinkGate.h"
#include "Misc/DateTime.h"

FTunaSweeperGateCheatControl TunaSweeperGateCheatControl;
static_assert(sizeof(FTunaSweeperGateCheatRequest) == 64);
static_assert(sizeof(FTunaSweeperGateCheatRow) == 400);
static_assert(sizeof(FTunaSweeperGateCheatSnapshot) == 25776);
static_assert(sizeof(FTunaSweeperGateCheatControl) == 25864);
static_assert(STRUCT_OFFSET(FTunaSweeperGateCheatControl, Request) == 24);
static_assert(STRUCT_OFFSET(FTunaSweeperGateCheatControl, Snapshot) == 88);
static_assert(STRUCT_OFFSET(FTunaSweeperGateCheatSnapshot, Rows) == 176);

namespace
{
 using EResult = ETunaSweeperGateCheatResult;
 uint64 HighSequence = 0;
 uint64 LastObservedSequence = 0;
 uint64 ContextEpoch = 0;
 uint64 NextRowKey = 0;
 TWeakObjectPtr<UWorld> PreviousWorld;
 TWeakObjectPtr<UTunaSweeperGameInstance> PreviousGame;
 uint64 PreviousSaveEpoch = 0;
 int32 PreviousSlot = 0;
 uint32 PreviousState = uint32(EResult::NoContext);
 TMap<TWeakObjectPtr<ATunaSweeperChainLinkGate>, uint64> RowKeys;

 int64 UtcMs()
 {
  const FDateTime Now = FDateTime::UtcNow();
  return Now.ToUnixTimestamp() * 1000 + Now.GetMillisecond();
 }
 template <SIZE_T N> bool CopyUtf8(char (&Dest)[N], const FString& Value)
 {
  const FTCHARToUTF8 Utf8(*Value);
  if (Utf8.Length() >= int32(N)) return false;
  FMemory::Memcpy(Dest, Utf8.Get(), Utf8.Length());
  return true;
 }
 bool ReadRequest(FTunaSweeperGateCheatRequest& Out)
 {
  const auto* Source = &TunaSweeperGateCheatControl.Request;
  const uint64 Before = *reinterpret_cast<const volatile uint64*>(&Source->Sequence);
  if (!Before || Before == LastObservedSequence) return false;
  FPlatformMisc::MemoryBarrier();
  FMemory::Memcpy(&Out, Source, sizeof(Out));
  FPlatformMisc::MemoryBarrier();
  return Before == Out.Sequence && Before == *reinterpret_cast<const volatile uint64*>(&Source->Sequence);
 }
 void Publish(FTunaSweeperGateCheatSnapshot& Snapshot)
 {
  auto& Destination = TunaSweeperGateCheatControl.Snapshot;
  const uint64 Revision = (Destination.Revision & ~uint64(1)) + 2;
  *reinterpret_cast<volatile uint64*>(&Destination.Revision) = Revision - 1;
  FPlatformMisc::MemoryBarrier();
  FMemory::Memcpy(reinterpret_cast<uint8*>(&Destination) + 8, reinterpret_cast<const uint8*>(&Snapshot) + 8, sizeof(Snapshot) - 8);
  FPlatformMisc::MemoryBarrier();
  *reinterpret_cast<volatile uint64*>(&Destination.Revision) = Revision;
 }
}

void UTunaSweeperGateCheatSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
 Super::Initialize(Collection);
 Collection.InitializeDependency<UTunaSweeperMovementCheatSubsystem>();
 TunaSweeperGateCheatControl.ProcessId = FPlatformProcess::GetCurrentProcessId();
}
void UTunaSweeperGateCheatSubsystem::Deinitialize()
{
 if (PreviousGame.Get() == GetGameInstance())
 {
  PreviousGame.Reset(); PreviousWorld.Reset(); ++ContextEpoch;
  FTunaSweeperGateCheatSnapshot Snapshot;
  Snapshot.AckSequence = TunaSweeperGateCheatControl.Snapshot.AckSequence;
  Snapshot.Result = TunaSweeperGateCheatControl.Snapshot.Result;
  Snapshot.ContextEpoch = ContextEpoch;
  Snapshot.State = uint32(EResult::NoContext); Snapshot.PublishedUtcMs = UtcMs();
  Publish(Snapshot);
 }
 Super::Deinitialize();
}
TStatId UTunaSweeperGateCheatSubsystem::GetStatId() const
{
 RETURN_QUICK_DECLARE_CYCLE_STAT(UTunaSweeperGateCheatSubsystem, STATGROUP_Tickables);
}
void UTunaSweeperGateCheatSubsystem::Tick(float DeltaTime)
{
 check(IsInGameThread());
 auto& Control = TunaSweeperGateCheatControl;
 // A modified/unsupported header must never result in a game mutation.
 if (Control.Magic != 0x5453474154454348ULL || Control.Version != 3 || Control.Size != sizeof(Control)
  || Control.ProcessId != FPlatformProcess::GetCurrentProcessId() || Control.Reserved != 0) return;
 UWorld* World = nullptr;
 UTunaSweeperGameInstance* Game = nullptr;
 int32 ContextCount = 0;
 if (GEngine)
 {
  for (const FWorldContext& Context : GEngine->GetWorldContexts())
  {
   UWorld* Candidate = Context.World();
   if (!Candidate || !Candidate->IsGameWorld()) continue;
   ++ContextCount;
   World = Candidate;
   Game = Candidate->GetGameInstance<UTunaSweeperGameInstance>();
  }
 }
 uint32 State = 0;
 if (ContextCount > 1) State = uint32(EResult::AmbiguousContext);
 else if (ContextCount != 1 || !World || !Game || World->GetNetMode() != NM_Standalone || !Game->IsGateCheatSaveReady() || Game->IsCombatTestSession()) State = uint32(EResult::NoContext);
 else
 {
  const auto* Movement = Game->GetSubsystem<UTunaSweeperMovementCheatSubsystem>();
  if (!Movement || !Movement->IsEnabled()) State = uint32(EResult::Disabled);
 }
 const uint64 SaveEpoch = Game ? Game->GetGateCheatSaveEpoch() : 0;
 const int32 Slot = Game ? Game->GetActiveSaveSlotIndex() : 0;
 if (PreviousWorld.Get() != World || PreviousGame.Get() != Game || PreviousSaveEpoch != SaveEpoch || PreviousSlot != Slot || PreviousState != State)
 {
  ++ContextEpoch;
  PreviousWorld = World; PreviousGame = Game; PreviousSaveEpoch = SaveEpoch; PreviousSlot = Slot; PreviousState = State;
 }
 FTunaSweeperGateCheatSnapshot Snapshot;
 Snapshot.AckSequence = Control.Snapshot.AckSequence;
 Snapshot.Result = Control.Snapshot.Result;
 Snapshot.ContextEpoch = ContextEpoch;
 Snapshot.State = State;
 Snapshot.Slot = Slot;
 Snapshot.PublishedUtcMs = UtcMs();
 TMap<uint64, ATunaSweeperChainLinkGate*> LiveRows;
 TMap<FName, int32> IdCounts;
 TArray<ATunaSweeperChainLinkGate*> Gates;
 if (State == 0)
 {
  CopyUtf8(Snapshot.Map, UWorld::RemovePIEPrefix(World->GetOutermost()->GetName()));
  for (TActorIterator<ATunaSweeperChainLinkGate> It(World); It; ++It)
  {
   if (!IsValid(*It) || !It->HasActorBegunPlay()) continue;
   Gates.Add(*It);
   ++IdCounts.FindOrAdd(It->GetPersistentUnlockFlag());
  }
  Gates.Sort([](const ATunaSweeperChainLinkGate& A, const ATunaSweeperChainLinkGate& B) { return A.GetPathName() < B.GetPathName(); });
  for (auto It = RowKeys.CreateIterator(); It; ++It) if (!It.Key().IsValid()) It.RemoveCurrent();
  for (ATunaSweeperChainLinkGate* Gate : Gates)
  {
   if (Snapshot.Count >= UE_ARRAY_COUNT(Snapshot.Rows)) break;
   uint64& Key = RowKeys.FindOrAdd(Gate);
   if (!Key) Key = ++NextRowKey;
   auto& Row = Snapshot.Rows[Snapshot.Count++];
   Row.Key = Key;
   Row.Flags = Gate->IsUnlocked() ? 1 : 0;
   const FName Flag = Gate->GetPersistentUnlockFlag();
   if (IdCounts.FindRef(Flag) > 1) Row.Flags |= 2;
   if (Flag.IsNone() || !CopyUtf8(Row.GateId, Gate->GetGateId().ToString()) || !CopyUtf8(Row.Level, UWorld::RemovePIEPrefix(Gate->GetLevel()->GetOutermost()->GetName()))) Row.Flags |= 4;
   LiveRows.Add(Key, Gate);
  }
  if (Gates.Num() > UE_ARRAY_COUNT(Snapshot.Rows))
  {
   State = uint32(EResult::Overflow);
   Snapshot.State = State;
   Snapshot.Count = 0; // Never present a silently truncated selectable list.
  }
 }
 FTunaSweeperGateCheatRequest Request;
 if (ReadRequest(Request))
 {
  LastObservedSequence = Request.Sequence;
  EResult Result = EResult::InvalidRequest;
  if (Request.Sequence <= HighSequence) Result = EResult::Replay;
  else
  {
   HighSequence = Request.Sequence;
   const int64 Now = UtcMs();
   if (Request.ProcessId != Control.ProcessId || Request.Reserved[0] || Request.Reserved[1] || Request.Reserved[2] || Request.Operation < 1 || Request.Operation > 4 || (Request.Operation == 3 && Request.RowKey != 0)) Result = EResult::InvalidRequest;
   else if (Request.DeadlineUtcMs <= Now || Request.DeadlineUtcMs > Now + 5000) Result = EResult::Expired;
   else if (State != 0) Result = static_cast<EResult>(State);
   else if (Request.ContextEpoch != ContextEpoch) Result = EResult::StaleContext;
   else if (Request.Operation == 1) Result = EResult::Refreshed;
   else if (Request.Operation == 4)
   {
    if (Control.Request.Sequence != Request.Sequence || Control.Request.DeadlineUtcMs <= UtcMs()) Result = EResult::Expired;
    else Result = static_cast<EResult>(Game->TryGrantItemForHelper(int32(uint32(Request.RowKey)), int32(uint32(Request.RowKey >> 32))));
   }
   else if (Request.Operation == 3)
   {
    if (Control.Request.Sequence != Request.Sequence || Control.Request.DeadlineUtcMs <= UtcMs()) Result = EResult::Expired;
    else Result = Game->TryFillRaidStorageForTrueEndingTest() ? EResult::TunaFilled : EResult::SaveFailed;
   }
   else if (Gates.Num() > UE_ARRAY_COUNT(Snapshot.Rows)) Result = EResult::Overflow;
   else if (ATunaSweeperChainLinkGate** Found = LiveRows.Find(Request.RowKey))
   {
    ATunaSweeperChainLinkGate* Gate = *Found;
    const FName Flag = Gate->GetPersistentUnlockFlag();
    bool bInvalidRow = false;
    for (uint32 Index = 0; Index < Snapshot.Count; ++Index)
     if (Snapshot.Rows[Index].Key == Request.RowKey && (Snapshot.Rows[Index].Flags & 4)) bInvalidRow = true;
    if (Flag.IsNone() || bInvalidRow) Result = EResult::MissingGate;
    else if (IdCounts.FindRef(Flag) != 1) Result = EResult::DuplicateId;
    else if (Control.Request.Sequence != Request.Sequence || Control.Request.DeadlineUtcMs <= UtcMs()) Result = EResult::Expired;
    else Result = Gate->TryResetPersistently() ? EResult::Reset : EResult::SaveFailed;
    if (Result == EResult::Reset)
     for (uint32 Index = 0; Index < Snapshot.Count; ++Index) if (Snapshot.Rows[Index].Key == Request.RowKey) Snapshot.Rows[Index].Flags &= ~uint32(1);
   }
   else Result = EResult::MissingGate;
  }
  Snapshot.AckSequence = HighSequence; // Replayed sequence numbers never move the acknowledged watermark backward.
  Snapshot.Result = uint32(Result);
 }
 Snapshot.PublishedUtcMs = UtcMs();
 Publish(Snapshot);
}
