#include "Subsystem/TunaSweeperEnemySquadSubsystem.h"

#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "Stats/Stats.h"

void UTunaSweeperEnemySquadSubsystem::Deinitialize()
{
	OnMemberStateChanged.Clear();
	MemberRegistrations.Reset();
	Squads.Reset();
	Super::Deinitialize();
}

void UTunaSweeperEnemySquadSubsystem::Tick(float DeltaTime)
{
	(void)DeltaTime;

	TArray<FInvalidSlot> InvalidSlots;
	for (const TPair<FTunaSweeperEnemySquadKey, FSquadRuntime>& SquadPair : Squads)
	{
		const FSquadRuntime& Squad = SquadPair.Value;
		for (int32 SlotIndex = 0; SlotIndex < 2; ++SlotIndex)
		{
			if (Squad.bSlotOccupied[SlotIndex] && !IsValid(Squad.Members[SlotIndex].Get()))
			{
				FInvalidSlot& InvalidSlot = InvalidSlots.AddDefaulted_GetRef();
				InvalidSlot.SquadKey = SquadPair.Key;
				InvalidSlot.SlotIndex = SlotIndex;
			}
		}
	}

	for (const FInvalidSlot& InvalidSlot : InvalidSlots)
	{
		ReleaseSlot(
			InvalidSlot.SquadKey,
			InvalidSlot.SlotIndex,
			ETunaSweeperEnemySquadUpdateReason::MemberReleased);
	}

	const double CurrentTimeSeconds = GetWorldTimeSeconds();
	TArray<FTunaSweeperEnemySquadKey> InvalidatedTargetSquads;
	TArray<FTunaSweeperEnemySquadKey> ExpiredLeaseSquads;
	for (TPair<FTunaSweeperEnemySquadKey, FSquadRuntime>& SquadPair : Squads)
	{
		FSquadRuntime& Squad = SquadPair.Value;
		if (Squad.bHasSharedHostileTarget && !IsValid(Squad.SharedHostileTarget.Get()))
		{
			Squad.SharedHostileTarget.Reset();
			Squad.bHasSharedHostileTarget = false;
			InvalidatedTargetSquads.Add(SquadPair.Key);
		}

		if (IsPairActive(Squad) &&
			Squad.LeaseEndTimeSeconds > 0.0 &&
			CurrentTimeSeconds >= Squad.LeaseEndTimeSeconds)
		{
			if (Squad.bSuppressionStarted && !Squad.bLastSuppressionShotReported)
			{
				Squad.bLastSuppressionShotReported = true;
				Squad.NextFireTimeSeconds = CurrentTimeSeconds + FMath::FRandRange(
					MinNextFireDelaySeconds,
					MaxNextFireDelaySeconds);
			}
			if (AdvanceCycle(Squad, CurrentTimeSeconds))
			{
				ExpiredLeaseSquads.Add(SquadPair.Key);
			}
		}
	}

	for (const FTunaSweeperEnemySquadKey& SquadKey : InvalidatedTargetSquads)
	{
		BroadcastMembers(SquadKey, ETunaSweeperEnemySquadUpdateReason::TargetInvalidated);
	}
	for (const FTunaSweeperEnemySquadKey& SquadKey : ExpiredLeaseSquads)
	{
		BroadcastMembers(SquadKey, ETunaSweeperEnemySquadUpdateReason::LeaseExpired);
	}
}

TStatId UTunaSweeperEnemySquadSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UTunaSweeperEnemySquadSubsystem, STATGROUP_Tickables);
}

bool UTunaSweeperEnemySquadSubsystem::IsTickable() const
{
	return !HasAnyFlags(RF_ClassDefaultObject) && GetWorld() && GetWorld()->IsGameWorld();
}

bool UTunaSweeperEnemySquadSubsystem::RegisterMember(
	APawn* MemberPawn,
	const FTunaSweeperEnemySquadKey& SquadKey,
	int32 SquadSlot,
	FTunaSweeperEnemySquadState& OutState)
{
	OutState = FTunaSweeperEnemySquadState();
	if (!IsValid(MemberPawn) ||
		!SquadKey.IsValid() ||
		(SquadSlot != 0 && SquadSlot != 1) ||
		MemberPawn->GetWorld() != GetWorld())
	{
		return false;
	}

	if (const FMemberRegistration* ExistingRegistration = FindRegistration(MemberPawn))
	{
		return ExistingRegistration->SquadKey == SquadKey &&
			ExistingRegistration->SlotIndex == SquadSlot &&
			BuildMemberState(MemberPawn, *ExistingRegistration, OutState);
	}

	for (int32 SlotIndex = 0; SlotIndex < 2; ++SlotIndex)
	{
		const FSquadRuntime* ExistingSquad = Squads.Find(SquadKey);
		if (ExistingSquad &&
			ExistingSquad->bSlotOccupied[SlotIndex] &&
			!IsValid(ExistingSquad->Members[SlotIndex].Get()))
		{
			ReleaseSlot(
				SquadKey,
				SlotIndex,
				ETunaSweeperEnemySquadUpdateReason::MemberReleased);
		}
	}

	FSquadRuntime& Squad = Squads.FindOrAdd(SquadKey);
	if (Squad.bSlotOccupied[SquadSlot])
	{
		return false;
	}

	Squad.Members[SquadSlot] = MemberPawn;
	Squad.bSlotOccupied[SquadSlot] = true;
	Squad.Roles[SquadSlot] = ETunaSweeperEnemySquadRole::Solo;

	FMemberRegistration Registration;
	Registration.SquadKey = SquadKey;
	Registration.SlotIndex = SquadSlot;
	MemberRegistrations.Add(TWeakObjectPtr<APawn>(MemberPawn), Registration);

	const bool bPairFormed = IsPairActive(Squad);
	if (bPairFormed)
	{
		StartPair(Squad, GetWorldTimeSeconds());
	}
	else
	{
		Squad.LeaseEndTimeSeconds = 0.0;
		ResetSuppressionTiming(Squad, false);
		ResetContact(Squad);
	}

	if (!BuildMemberState(MemberPawn, Registration, OutState))
	{
		return false;
	}

	BroadcastMembers(
		SquadKey,
		bPairFormed
			? ETunaSweeperEnemySquadUpdateReason::PairFormed
			: ETunaSweeperEnemySquadUpdateReason::Registered);
	return true;
}

bool UTunaSweeperEnemySquadSubsystem::UnregisterMember(APawn* MemberPawn)
{
	FMemberRegistration* Registration = FindRegistration(MemberPawn);
	if (!Registration)
	{
		return false;
	}

	const FTunaSweeperEnemySquadKey SquadKey = Registration->SquadKey;
	const int32 SlotIndex = Registration->SlotIndex;
	return ReleaseSlot(
		SquadKey,
		SlotIndex,
		ETunaSweeperEnemySquadUpdateReason::MemberReleased);
}

bool UTunaSweeperEnemySquadSubsystem::GetMemberState(
	const APawn* MemberPawn,
	FTunaSweeperEnemySquadState& OutState) const
{
	OutState = FTunaSweeperEnemySquadState();
	const FMemberRegistration* Registration = FindRegistration(MemberPawn);
	return Registration && BuildMemberState(MemberPawn, *Registration, OutState);
}

bool UTunaSweeperEnemySquadSubsystem::ReportRoleCompleted(APawn* MemberPawn, int32 ExpectedCycleId)
{
	const FMemberRegistration* Registration = FindRegistration(MemberPawn);
	if (!Registration)
	{
		return false;
	}

	const FTunaSweeperEnemySquadKey SquadKey = Registration->SquadKey;
	FSquadRuntime* Squad = Squads.Find(SquadKey);
	if (!Squad ||
		!IsPairActive(*Squad) ||
		ExpectedCycleId <= 0 ||
		Squad->CycleId != ExpectedCycleId ||
		!Squad->bSlotOccupied[Registration->SlotIndex] ||
		Squad->Members[Registration->SlotIndex].Get() != MemberPawn ||
		Squad->Roles[Registration->SlotIndex] != ETunaSweeperEnemySquadRole::Reposition ||
		(Squad->bSuppressionStarted
			? !Squad->bLastSuppressionShotReported
			: !Squad->bLineOfFireRecoveryRequested) ||
		!AdvanceCycle(*Squad, GetWorldTimeSeconds()))
	{
		return false;
	}

	BroadcastMembers(SquadKey, ETunaSweeperEnemySquadUpdateReason::RoleCompleted);
	return true;
}

bool UTunaSweeperEnemySquadSubsystem::YieldSuppression(APawn* MemberPawn, int32 ExpectedCycleId)
{
	const FMemberRegistration* Registration = FindRegistration(MemberPawn);
	if (!Registration)
	{
		return false;
	}

	const FTunaSweeperEnemySquadKey SquadKey = Registration->SquadKey;
	FSquadRuntime* Squad = Squads.Find(SquadKey);
	if (!Squad || !IsPairActive(*Squad) || ExpectedCycleId <= 0 ||
		Squad->CycleId != ExpectedCycleId ||
		!Squad->bSlotOccupied[Registration->SlotIndex] ||
		Squad->Members[Registration->SlotIndex].Get() != MemberPawn ||
		Squad->Roles[Registration->SlotIndex] != ETunaSweeperEnemySquadRole::Suppress)
	{
		return false;
	}

	const double CurrentTimeSeconds = GetWorldTimeSeconds();
	if (Squad->bSuppressionStarted && !Squad->bLastSuppressionShotReported)
	{
		Squad->bLastSuppressionShotReported = true;
		Squad->NextFireTimeSeconds = CurrentTimeSeconds + FMath::FRandRange(
			MinNextFireDelaySeconds,
			MaxNextFireDelaySeconds);
	}
	if (!AdvanceCycle(*Squad, CurrentTimeSeconds))
	{
		return false;
	}

	BroadcastMembers(SquadKey, ETunaSweeperEnemySquadUpdateReason::SuppressionYielded);
	return true;
}

bool UTunaSweeperEnemySquadSubsystem::RequestLineOfFireRecovery(
	APawn* MemberPawn,
	int32 ExpectedCycleId)
{
	const FMemberRegistration* Registration = FindRegistration(MemberPawn);
	if (!Registration)
	{
		return false;
	}
	const FTunaSweeperEnemySquadKey SquadKey = Registration->SquadKey;
	FSquadRuntime* Squad = Squads.Find(SquadKey);
	if (!Squad || !IsPairActive(*Squad) || ExpectedCycleId <= 0 ||
		Squad->CycleId != ExpectedCycleId ||
		Squad->Members[Registration->SlotIndex].Get() != MemberPawn ||
		Squad->Roles[Registration->SlotIndex] != ETunaSweeperEnemySquadRole::Suppress)
	{
		return false;
	}
	if (!Squad->bLineOfFireRecoveryRequested)
	{
		const double CurrentTimeSeconds = GetWorldTimeSeconds();
		Squad->bLineOfFireRecoveryRequested = true;
		Squad->MoverStartTimeSeconds = CurrentTimeSeconds;
		Squad->LeaseEndTimeSeconds = CurrentTimeSeconds + RoleLeaseSeconds;
		BroadcastMembers(SquadKey, ETunaSweeperEnemySquadUpdateReason::LineOfFireRecovery);
	}
	return true;
}

bool UTunaSweeperEnemySquadSubsystem::ReportSuppressionShot(
	APawn* MemberPawn,
	int32 ExpectedCycleId,
	bool bFirstShot,
	bool bLastShot)
{
	if (!bFirstShot && !bLastShot)
	{
		return false;
	}

	const FMemberRegistration* Registration = FindRegistration(MemberPawn);
	if (!Registration)
	{
		return false;
	}

	const FTunaSweeperEnemySquadKey SquadKey = Registration->SquadKey;
	FSquadRuntime* Squad = Squads.Find(SquadKey);
	if (!Squad ||
		!IsPairActive(*Squad) ||
		ExpectedCycleId <= 0 ||
		Squad->CycleId != ExpectedCycleId ||
		!Squad->bSlotOccupied[Registration->SlotIndex] ||
		Squad->Members[Registration->SlotIndex].Get() != MemberPawn ||
		Squad->Roles[Registration->SlotIndex] != ETunaSweeperEnemySquadRole::Suppress ||
		(bFirstShot && Squad->bSuppressionStarted) ||
		(!bFirstShot && !Squad->bSuppressionStarted) ||
		(bLastShot && Squad->bLastSuppressionShotReported))
	{
		return false;
	}

	const double CurrentTimeSeconds = GetWorldTimeSeconds();
	if (bFirstShot)
	{
		Squad->bSuppressionStarted = true;
		Squad->LeaseEndTimeSeconds = CurrentTimeSeconds + RoleLeaseSeconds;
		Squad->MoverStartTimeSeconds = CurrentTimeSeconds + FMath::FRandRange(
			MinMoverStartDelaySeconds,
			MaxMoverStartDelaySeconds);
	}
	if (bLastShot)
	{
		Squad->bLastSuppressionShotReported = true;
		Squad->NextFireTimeSeconds = CurrentTimeSeconds + FMath::FRandRange(
			MinNextFireDelaySeconds,
			MaxNextFireDelaySeconds);
	}

	BroadcastMembers(SquadKey, ETunaSweeperEnemySquadUpdateReason::SuppressionShot);
	return true;
}

bool UTunaSweeperEnemySquadSubsystem::ReportHostileContact(
	APawn* MemberPawn,
	int32 ExpectedCycleId,
	AActor* HostileTarget,
	const FVector& LastKnownLocation)
{
	const FMemberRegistration* Registration = FindRegistration(MemberPawn);
	if (!Registration || LastKnownLocation.ContainsNaN() || (HostileTarget && !IsValid(HostileTarget)))
	{
		return false;
	}

	const FTunaSweeperEnemySquadKey SquadKey = Registration->SquadKey;
	FSquadRuntime* Squad = Squads.Find(SquadKey);
	if (!Squad || !IsPairActive(*Squad) || Squad->CycleId != ExpectedCycleId || ExpectedCycleId <= 0)
	{
		return false;
	}

	if (HostileTarget &&
		(HostileTarget == Squad->Members[0].Get() || HostileTarget == Squad->Members[1].Get()))
	{
		return false;
	}

	Squad->SharedHostileTarget = HostileTarget;
	Squad->bHasSharedHostileTarget = HostileTarget != nullptr;
	Squad->bHasSharedLastKnownLocation = true;
	Squad->SharedLastKnownLocation = LastKnownLocation;
	Squad->SharedContactTimeSeconds = GetWorldTimeSeconds();

	BroadcastMembers(SquadKey, ETunaSweeperEnemySquadUpdateReason::ContactUpdated);
	return true;
}

bool UTunaSweeperEnemySquadSubsystem::ClearSharedHostileContact(
	APawn* MemberPawn,
	int32 ExpectedCycleId)
{
	const FMemberRegistration* Registration = FindRegistration(MemberPawn);
	if (!Registration)
	{
		return false;
	}

	const FTunaSweeperEnemySquadKey SquadKey = Registration->SquadKey;
	FSquadRuntime* Squad = Squads.Find(SquadKey);
	if (!Squad || !IsPairActive(*Squad) || Squad->CycleId != ExpectedCycleId || ExpectedCycleId <= 0)
	{
		return false;
	}

	ResetContact(*Squad);
	BroadcastMembers(SquadKey, ETunaSweeperEnemySquadUpdateReason::ContactUpdated);
	return true;
}

const UTunaSweeperEnemySquadSubsystem::FMemberRegistration*
UTunaSweeperEnemySquadSubsystem::FindRegistration(const APawn* MemberPawn) const
{
	if (!MemberPawn)
	{
		return nullptr;
	}

	return MemberRegistrations.Find(TWeakObjectPtr<APawn>(const_cast<APawn*>(MemberPawn)));
}

UTunaSweeperEnemySquadSubsystem::FMemberRegistration*
UTunaSweeperEnemySquadSubsystem::FindRegistration(APawn* MemberPawn)
{
	return MemberPawn ? MemberRegistrations.Find(TWeakObjectPtr<APawn>(MemberPawn)) : nullptr;
}

bool UTunaSweeperEnemySquadSubsystem::IsPairActive(const FSquadRuntime& Squad) const
{
	return Squad.bSlotOccupied[0] &&
		Squad.bSlotOccupied[1] &&
		IsValid(Squad.Members[0].Get()) &&
		IsValid(Squad.Members[1].Get());
}

bool UTunaSweeperEnemySquadSubsystem::BuildMemberState(
	const APawn* MemberPawn,
	const FMemberRegistration& Registration,
	FTunaSweeperEnemySquadState& OutState) const
{
	const FSquadRuntime* Squad = Squads.Find(Registration.SquadKey);
	if (!IsValid(MemberPawn) ||
		!Squad ||
		Registration.SlotIndex < 0 ||
		Registration.SlotIndex >= 2 ||
		!Squad->bSlotOccupied[Registration.SlotIndex] ||
		Squad->Members[Registration.SlotIndex].Get() != MemberPawn)
	{
		return false;
	}

	const double CurrentTimeSeconds = GetWorldTimeSeconds();
	OutState = FTunaSweeperEnemySquadState();
	OutState.bRegistered = true;
	OutState.SquadKey = Registration.SquadKey;
	OutState.SlotIndex = Registration.SlotIndex;
	OutState.Role = Squad->Roles[Registration.SlotIndex];
	OutState.CycleId = Squad->CycleId;
	OutState.LeaseRemainingSeconds = IsPairActive(*Squad)
		? static_cast<float>(FMath::Max(0.0, Squad->LeaseEndTimeSeconds - CurrentTimeSeconds))
		: 0.0f;
	OutState.bSuppressionStarted = IsPairActive(*Squad) && Squad->bSuppressionStarted;
	OutState.bSuppressionFinished = IsPairActive(*Squad) && Squad->bLastSuppressionShotReported;
	OutState.bLineOfFireRecoveryRequested = IsPairActive(*Squad) && Squad->bLineOfFireRecoveryRequested;
	OutState.MoverStartRemainingSeconds =
		(OutState.bSuppressionStarted || OutState.bLineOfFireRecoveryRequested)
		? static_cast<float>(FMath::Max(0.0, Squad->MoverStartTimeSeconds - CurrentTimeSeconds))
		: 0.0f;
	OutState.NextFireRemainingSeconds = IsPairActive(*Squad)
		? static_cast<float>(FMath::Max(0.0, Squad->NextFireTimeSeconds - CurrentTimeSeconds))
		: 0.0f;
	OutState.SharedHostileTarget = Squad->bHasSharedHostileTarget
		? Squad->SharedHostileTarget.Get()
		: nullptr;
	OutState.bHasSharedLastKnownLocation = Squad->bHasSharedLastKnownLocation;
	OutState.SharedLastKnownLocation = Squad->SharedLastKnownLocation;
	OutState.SharedContactAgeSeconds = Squad->bHasSharedLastKnownLocation
		? static_cast<float>(FMath::Max(0.0, CurrentTimeSeconds - Squad->SharedContactTimeSeconds))
		: 0.0f;
	return true;
}

void UTunaSweeperEnemySquadSubsystem::StartPair(FSquadRuntime& Squad, double CurrentTimeSeconds)
{
	Squad.CycleId = NextCycleId(Squad.CycleId);
	Squad.Roles[0] = ETunaSweeperEnemySquadRole::Suppress;
	Squad.Roles[1] = ETunaSweeperEnemySquadRole::Reposition;
	Squad.LeaseEndTimeSeconds = CurrentTimeSeconds + RoleLeaseSeconds;
	ResetSuppressionTiming(Squad, false);
	ResetContact(Squad);
}

bool UTunaSweeperEnemySquadSubsystem::AdvanceCycle(FSquadRuntime& Squad, double CurrentTimeSeconds)
{
	if (!IsPairActive(Squad))
	{
		return false;
	}

	if (Squad.Roles[0] == ETunaSweeperEnemySquadRole::Solo ||
		Squad.Roles[1] == ETunaSweeperEnemySquadRole::Solo ||
		Squad.Roles[0] == Squad.Roles[1])
	{
		Squad.Roles[0] = ETunaSweeperEnemySquadRole::Suppress;
		Squad.Roles[1] = ETunaSweeperEnemySquadRole::Reposition;
	}
	else
	{
		Swap(Squad.Roles[0], Squad.Roles[1]);
	}

	Squad.CycleId = NextCycleId(Squad.CycleId);
	Squad.LeaseEndTimeSeconds = CurrentTimeSeconds + RoleLeaseSeconds;
	ResetSuppressionTiming(Squad, true);
	return true;
}

bool UTunaSweeperEnemySquadSubsystem::ReleaseSlot(
	const FTunaSweeperEnemySquadKey& SquadKey,
	int32 SlotIndex,
	ETunaSweeperEnemySquadUpdateReason Reason)
{
	FSquadRuntime* Squad = Squads.Find(SquadKey);
	if (!Squad || SlotIndex < 0 || SlotIndex >= 2 || !Squad->bSlotOccupied[SlotIndex])
	{
		return false;
	}

	APawn* RemovedPawn = Squad->Members[SlotIndex].Get();
	RemoveRegistrationForSlot(SquadKey, SlotIndex);
	Squad->Members[SlotIndex].Reset();
	Squad->bSlotOccupied[SlotIndex] = false;
	Squad->Roles[0] = ETunaSweeperEnemySquadRole::Solo;
	Squad->Roles[1] = ETunaSweeperEnemySquadRole::Solo;
	Squad->CycleId = NextCycleId(Squad->CycleId);
	Squad->LeaseEndTimeSeconds = 0.0;
	ResetSuppressionTiming(*Squad, false);
	ResetContact(*Squad);

	const bool bHasRemainingMember = Squad->bSlotOccupied[0] || Squad->bSlotOccupied[1];
	if (!bHasRemainingMember)
	{
		Squads.Remove(SquadKey);
	}
	else
	{
		BroadcastMembers(SquadKey, Reason);
	}

	if (IsValid(RemovedPawn))
	{
		FTunaSweeperEnemySquadState ReleasedState;
		ReleasedState.SquadKey = SquadKey;
		ReleasedState.SlotIndex = SlotIndex;
		ReleasedState.Role = ETunaSweeperEnemySquadRole::Solo;
		OnMemberStateChanged.Broadcast(RemovedPawn, ReleasedState, Reason);
	}
	return true;
}

void UTunaSweeperEnemySquadSubsystem::RemoveRegistrationForSlot(
	const FTunaSweeperEnemySquadKey& SquadKey,
	int32 SlotIndex)
{
	for (auto RegistrationIt = MemberRegistrations.CreateIterator(); RegistrationIt; ++RegistrationIt)
	{
		if (RegistrationIt.Value().SquadKey == SquadKey && RegistrationIt.Value().SlotIndex == SlotIndex)
		{
			RegistrationIt.RemoveCurrent();
			return;
		}
	}
}

void UTunaSweeperEnemySquadSubsystem::ResetContact(FSquadRuntime& Squad)
{
	Squad.SharedHostileTarget.Reset();
	Squad.bHasSharedHostileTarget = false;
	Squad.bHasSharedLastKnownLocation = false;
	Squad.SharedLastKnownLocation = FVector::ZeroVector;
	Squad.SharedContactTimeSeconds = 0.0;
}

void UTunaSweeperEnemySquadSubsystem::ResetSuppressionTiming(
	FSquadRuntime& Squad,
	bool bPreserveNextFireGate)
{
	Squad.bSuppressionStarted = false;
	Squad.bLastSuppressionShotReported = false;
	Squad.bLineOfFireRecoveryRequested = false;
	Squad.MoverStartTimeSeconds = 0.0;
	if (!bPreserveNextFireGate)
	{
		Squad.NextFireTimeSeconds = 0.0;
	}
}

void UTunaSweeperEnemySquadSubsystem::BroadcastMembers(
	const FTunaSweeperEnemySquadKey& SquadKey,
	ETunaSweeperEnemySquadUpdateReason Reason)
{
	TWeakObjectPtr<APawn> MemberSnapshots[2];
	if (const FSquadRuntime* Squad = Squads.Find(SquadKey))
	{
		MemberSnapshots[0] = Squad->Members[0];
		MemberSnapshots[1] = Squad->Members[1];
	}

	for (int32 SlotIndex = 0; SlotIndex < 2; ++SlotIndex)
	{
		APawn* MemberPawn = MemberSnapshots[SlotIndex].Get();
		FTunaSweeperEnemySquadState State;
		if (IsValid(MemberPawn) && GetMemberState(MemberPawn, State))
		{
			OnMemberStateChanged.Broadcast(MemberPawn, State, Reason);
		}
	}
}

double UTunaSweeperEnemySquadSubsystem::GetWorldTimeSeconds() const
{
	const UWorld* World = GetWorld();
	return World ? static_cast<double>(World->GetTimeSeconds()) : 0.0;
}

int32 UTunaSweeperEnemySquadSubsystem::NextCycleId(int32 CurrentCycleId)
{
	return CurrentCycleId <= 0 || CurrentCycleId >= MAX_int32
		? 1
		: CurrentCycleId + 1;
}
