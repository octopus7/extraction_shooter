#pragma once

#include "CoreMinimal.h"
#include "AI/TunaSweeperEnemySquadTypes.h"
#include "Subsystems/WorldSubsystem.h"
#include "TunaSweeperEnemySquadSubsystem.generated.h"

class AActor;
class APawn;

DECLARE_MULTICAST_DELEGATE_ThreeParams(
	FTunaSweeperEnemySquadStateChangedSignature,
	APawn*,
	const FTunaSweeperEnemySquadState&,
	ETunaSweeperEnemySquadUpdateReason);

/**
 * World-local coordinator for fixed two-member enemy squads.
 *
 * Controllers register their pawn, consume FTunaSweeperEnemySquadState, and
 * report asynchronous results with the CycleId from that state. Reports from
 * an older cycle are rejected without changing the current pair.
 */
UCLASS()
class TUNASWEEPER_API UTunaSweeperEnemySquadSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	static constexpr float RoleLeaseSeconds = 5.0f;
	static constexpr float MinMoverStartDelaySeconds = 0.15f;
	static constexpr float MaxMoverStartDelaySeconds = 0.35f;
	static constexpr float MinNextFireDelaySeconds = 0.9f;
	static constexpr float MaxNextFireDelaySeconds = 1.2f;

	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickable() const override;

	/** Registers MemberPawn into its authored slot. Valid slots are exactly 0 and 1. */
	bool RegisterMember(
		APawn* MemberPawn,
		const FTunaSweeperEnemySquadKey& SquadKey,
		int32 SquadSlot,
		FTunaSweeperEnemySquadState& OutState);

	/**
	 * Releases a pawn immediately. Call this from an explicit death/unpossess
	 * path; destroyed pawns are also pruned automatically on Tick.
	 */
	bool UnregisterMember(APawn* MemberPawn);

	/** Returns the current role/contact snapshot for a registered pawn. */
	bool GetMemberState(const APawn* MemberPawn, FTunaSweeperEnemySquadState& OutState) const;

	/**
	 * Completes the member's current non-Solo role and swaps both pair roles.
	 * Returns false for an unregistered member or stale ExpectedCycleId.
	 */
	bool ReportRoleCompleted(APawn* MemberPawn, int32 ExpectedCycleId);

	/** Explicitly relinquishes a blocked/reloading suppressor lease. */
	bool YieldSuppression(APawn* MemberPawn, int32 ExpectedCycleId);

	/** Opens the current mover gate without swapping roles when the suppressor is blocked. */
	bool RequestLineOfFireRecovery(APawn* MemberPawn, int32 ExpectedCycleId);

	/**
	 * Reports the suppressor's first and/or last shot for the current cycle.
	 * The first shot opens the mover gate after 0.15-0.35 seconds. The last
	 * shot starts a pair-wide 0.9-1.2 second fire gate that survives role swap.
	 */
	bool ReportSuppressionShot(
		APawn* MemberPawn,
		int32 ExpectedCycleId,
		bool bFirstShot,
		bool bLastShot);

	/**
	 * Shares a hostile target and last-known location with the pair.
	 * HostileTarget may be null to report that direct contact was lost while
	 * retaining the supplied last-known location. The caller owns hostility
	 * validation; this subsystem only validates membership and CycleId.
	 */
	bool ReportHostileContact(
		APawn* MemberPawn,
		int32 ExpectedCycleId,
		AActor* HostileTarget,
		const FVector& LastKnownLocation);

	/** Clears both the shared target and its last-known location. */
	bool ClearSharedHostileContact(APawn* MemberPawn, int32 ExpectedCycleId);

	/** Native callback for controller integration; no controller dependency is required here. */
	FTunaSweeperEnemySquadStateChangedSignature OnMemberStateChanged;

private:
	struct FMemberRegistration
	{
		FTunaSweeperEnemySquadKey SquadKey;
		int32 SlotIndex = INDEX_NONE;
	};

	struct FSquadRuntime
	{
		TWeakObjectPtr<APawn> Members[2];
		bool bSlotOccupied[2] = { false, false };
		ETunaSweeperEnemySquadRole Roles[2] = {
			ETunaSweeperEnemySquadRole::Solo,
			ETunaSweeperEnemySquadRole::Solo };
		int32 CycleId = 0;
		double LeaseEndTimeSeconds = 0.0;
		bool bSuppressionStarted = false;
		bool bLastSuppressionShotReported = false;
		bool bLineOfFireRecoveryRequested = false;
		double MoverStartTimeSeconds = 0.0;
		double NextFireTimeSeconds = 0.0;
		TWeakObjectPtr<AActor> SharedHostileTarget;
		bool bHasSharedHostileTarget = false;
		bool bHasSharedLastKnownLocation = false;
		FVector SharedLastKnownLocation = FVector::ZeroVector;
		double SharedContactTimeSeconds = 0.0;
	};

	struct FInvalidSlot
	{
		FTunaSweeperEnemySquadKey SquadKey;
		int32 SlotIndex = INDEX_NONE;
	};

	const FMemberRegistration* FindRegistration(const APawn* MemberPawn) const;
	FMemberRegistration* FindRegistration(APawn* MemberPawn);
	bool IsPairActive(const FSquadRuntime& Squad) const;
	bool BuildMemberState(
		const APawn* MemberPawn,
		const FMemberRegistration& Registration,
		FTunaSweeperEnemySquadState& OutState) const;
	void StartPair(FSquadRuntime& Squad, double CurrentTimeSeconds);
	bool AdvanceCycle(FSquadRuntime& Squad, double CurrentTimeSeconds);
	bool ReleaseSlot(
		const FTunaSweeperEnemySquadKey& SquadKey,
		int32 SlotIndex,
		ETunaSweeperEnemySquadUpdateReason Reason);
	void RemoveRegistrationForSlot(const FTunaSweeperEnemySquadKey& SquadKey, int32 SlotIndex);
	void ResetContact(FSquadRuntime& Squad);
	void ResetSuppressionTiming(FSquadRuntime& Squad, bool bPreserveNextFireGate);
	void BroadcastMembers(
		const FTunaSweeperEnemySquadKey& SquadKey,
		ETunaSweeperEnemySquadUpdateReason Reason);
	double GetWorldTimeSeconds() const;
	static int32 NextCycleId(int32 CurrentCycleId);

	TMap<FTunaSweeperEnemySquadKey, FSquadRuntime> Squads;
	TMap<TWeakObjectPtr<APawn>, FMemberRegistration> MemberRegistrations;
};
