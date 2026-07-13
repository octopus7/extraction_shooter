#pragma once

#include "CoreMinimal.h"
#include "Component/TunaSweeperFactionTypes.h"
#include "TunaSweeperEnemySquadTypes.generated.h"

class AActor;

UENUM(BlueprintType)
enum class ETunaSweeperEnemySquadRole : uint8
{
	Solo = 0 UMETA(DisplayName = "Solo"),
	Suppress = 1 UMETA(DisplayName = "Suppress"),
	Reposition = 2 UMETA(DisplayName = "Reposition")
};

UENUM(BlueprintType)
enum class ETunaSweeperEnemySquadUpdateReason : uint8
{
	Registered = 0 UMETA(DisplayName = "Registered"),
	PairFormed = 1 UMETA(DisplayName = "Pair Formed"),
	RoleCompleted = 2 UMETA(DisplayName = "Role Completed"),
	LeaseExpired = 3 UMETA(DisplayName = "Lease Expired"),
	ContactUpdated = 4 UMETA(DisplayName = "Contact Updated"),
	TargetInvalidated = 5 UMETA(DisplayName = "Target Invalidated"),
	MemberReleased = 6 UMETA(DisplayName = "Member Released"),
	SuppressionShot = 7 UMETA(DisplayName = "Suppression Shot"),
	SuppressionYielded = 8 UMETA(DisplayName = "Suppression Yielded"),
	LineOfFireRecovery = 9 UMETA(DisplayName = "Line Of Fire Recovery")
};

/** Stable runtime key for one two-member squad. */
USTRUCT(BlueprintType)
struct TUNASWEEPER_API FTunaSweeperEnemySquadKey
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Enemy Squad")
	uint8 FactionId = TunaSweeperFactionIds::NoFaction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Enemy Squad")
	FName SquadId = NAME_None;

	bool IsValid() const
	{
		return TunaSweeperFactionIds::IsValid(FactionId) && !SquadId.IsNone();
	}

	friend bool operator==(const FTunaSweeperEnemySquadKey& Left, const FTunaSweeperEnemySquadKey& Right)
	{
		return Left.FactionId == Right.FactionId && Left.SquadId == Right.SquadId;
	}

	friend bool operator!=(const FTunaSweeperEnemySquadKey& Left, const FTunaSweeperEnemySquadKey& Right)
	{
		return !(Left == Right);
	}
};

FORCEINLINE uint32 GetTypeHash(const FTunaSweeperEnemySquadKey& Key)
{
	return HashCombine(::GetTypeHash(static_cast<uint32>(Key.FactionId)), GetTypeHash(Key.SquadId));
}

/** Read-only snapshot consumed by an enemy controller. */
USTRUCT(BlueprintType)
struct TUNASWEEPER_API FTunaSweeperEnemySquadState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Enemy Squad")
	bool bRegistered = false;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Enemy Squad")
	FTunaSweeperEnemySquadKey SquadKey;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Enemy Squad")
	int32 SlotIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Enemy Squad")
	ETunaSweeperEnemySquadRole Role = ETunaSweeperEnemySquadRole::Solo;

	/** Changes whenever a pair starts, swaps roles, or loses a member. */
	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Enemy Squad")
	int32 CycleId = 0;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Enemy Squad")
	float LeaseRemainingSeconds = 0.0f;

	/** True after the current cycle's suppressor reports its first shot. */
	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Enemy Squad")
	bool bSuppressionStarted = false;

	/** True after the current suppressor reports its final shot. */
	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Enemy Squad")
	bool bSuppressionFinished = false;

	/** Suppressor asked the existing mover to seek a new firing angle. */
	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Enemy Squad")
	bool bLineOfFireRecoveryRequested = false;

	/** Pair-wide delay before the current mover may begin repositioning. */
	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Enemy Squad")
	float MoverStartRemainingSeconds = 0.0f;

	/** Pair-wide delay before whichever member currently owns Suppress may fire. */
	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Enemy Squad")
	float NextFireRemainingSeconds = 0.0f;

	/** Weak internally; this snapshot never makes the subsystem retain a target. */
	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Enemy Squad")
	TObjectPtr<AActor> SharedHostileTarget = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Enemy Squad")
	bool bHasSharedLastKnownLocation = false;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Enemy Squad")
	FVector SharedLastKnownLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Enemy Squad")
	float SharedContactAgeSeconds = 0.0f;
};
