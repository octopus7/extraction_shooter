#pragma once

#include "CoreMinimal.h"
#include "TunaSweeperEnemyCombatProfile.generated.h"

UENUM(BlueprintType)
enum class ETunaSweeperEnemyAttackMode : uint8
{
	Ranged UMETA(DisplayName = "Ranged"),
	Melee UMETA(DisplayName = "Melee")
};

UENUM(BlueprintType)
enum class ETunaSweeperEnemyCombatRole : uint8
{
	Anchor UMETA(DisplayName = "Anchor"),
	Flanker UMETA(DisplayName = "Flanker"),
	Melee UMETA(DisplayName = "Melee")
};

namespace TunaSweeperEnemyCombatConstants
{
	inline constexpr float MeleeAttackRange = 150.0f;
}

/**
 * Data-driven enemy combat rhythm and movement tuning.
 * EnemyCombatProfiles.json is the source of truth for these values.
 */
USTRUCT(BlueprintType)
struct TUNASWEEPER_API FTunaSweeperEnemyCombatProfile
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Enemy Combat")
	FName ProfileId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Enemy Combat")
	ETunaSweeperEnemyAttackMode AttackMode = ETunaSweeperEnemyAttackMode::Ranged;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Enemy Combat")
	ETunaSweeperEnemyCombatRole Role = ETunaSweeperEnemyCombatRole::Anchor;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Enemy Combat", meta = (ClampMin = "0.0"))
	float MovementSpeed = 340.0f;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Enemy Combat", meta = (ClampMin = "0.0"))
	float TrackingRange = 1150.0f;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Enemy Combat", meta = (ClampMin = "0.0"))
	float PreferredRangeMin = 650.0f;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Enemy Combat", meta = (ClampMin = "0.0"))
	float PreferredRangeMax = 1000.0f;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Enemy Combat", meta = (ClampMin = "0.0"))
	float DangerRange = 430.0f;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Enemy Combat", meta = (ClampMin = "0.0"))
	float AlertSeconds = 1.4f;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Enemy Combat", meta = (ClampMin = "0.0"))
	float AimSecondsMin = 0.3f;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Enemy Combat", meta = (ClampMin = "0.0"))
	float AimSecondsMax = 0.5f;

	/** Constant yaw turn-rate cap. Unlike RInterpTo, this never accelerates on large angle changes. */
	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Enemy Combat", meta = (ClampMin = "1.0"))
	float TurnSpeedDegreesPerSecond = 180.0f;

	/** The pawn must face within this yaw angle before a ranged shot or melee hit is allowed. */
	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Enemy Combat", meta = (ClampMin = "0.0", ClampMax = "90.0"))
	float AttackFacingToleranceDegrees = 8.0f;

	/** Multiplies the weapon's current spread half-angle for enemy shots only. */
	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Enemy Combat", meta = (ClampMin = "0.01"))
	float WeaponSpreadMultiplier = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Enemy Combat", meta = (ClampMin = "0"))
	int32 FiringShotCount = 3;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Enemy Combat", meta = (ClampMin = "0"))
	int32 OpeningFiringShotCount = 2;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Enemy Combat", meta = (ClampMin = "0.0"))
	float ShotIntervalSecondsMin = 0.18f;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Enemy Combat", meta = (ClampMin = "0.0"))
	float ShotIntervalSecondsMax = 0.23f;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Enemy Combat", meta = (ClampMin = "0.0"))
	float RecoverSecondsMin = 1.2f;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Enemy Combat", meta = (ClampMin = "0.0"))
	float RecoverSecondsMax = 1.5f;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Enemy Combat", meta = (ClampMin = "0.0"))
	float ObserveSecondsMin = 1.4f;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Enemy Combat", meta = (ClampMin = "0.0"))
	float ObserveSecondsMax = 2.0f;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Enemy Combat", meta = (ClampMin = "0.0"))
	float ReloadReadySecondsMin = 0.35f;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Enemy Combat", meta = (ClampMin = "0.0"))
	float ReloadReadySecondsMax = 0.55f;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Enemy Combat", meta = (ClampMin = "0"))
	int32 PositionFiringBudgetMin = 1;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Enemy Combat", meta = (ClampMin = "0"))
	int32 PositionFiringBudgetMax = 2;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Enemy Combat", meta = (ClampMin = "0.0"))
	float RepositionDistanceMin = 180.0f;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Enemy Combat", meta = (ClampMin = "0.0"))
	float RepositionDistanceMax = 300.0f;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Enemy Combat", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float CrossRepositionChance = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Enemy Combat", meta = (ClampMin = "0.0"))
	float CrossRepositionCooldownSeconds = 6.0f;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Enemy Combat", meta = (ClampMin = "0.0"))
	float CrossRepositionOrbitRadius = 150.0f;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Enemy Combat", meta = (ClampMin = "0.0"))
	float MeleeAttackDamage = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Enemy Combat", meta = (ClampMin = "0.0"))
	float MeleeApproachStartRange = 130.0f;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Enemy Combat", meta = (ClampMin = "0.0"))
	float MeleeApproachStopRange = 95.0f;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Enemy Combat", meta = (ClampMin = "0.0"))
	float AttackCooldownSeconds = 1.25f;
};
