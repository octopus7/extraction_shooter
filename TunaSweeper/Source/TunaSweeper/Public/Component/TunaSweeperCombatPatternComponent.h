#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TunaSweeperCombatPatternComponent.generated.h"

class ATunaSweeperAttackTelegraph;
class ATunaSweeperMissileTurret;
class ATunaSweeperRollingRobotMinion;

UENUM(BlueprintType)
enum class ETunaSweeperCombatPattern : uint8
{
	MissileTurret,
	Charge,
	RollingMinions
};

UENUM(BlueprintType)
enum class ETunaSweeperCombatPatternPhase : uint8
{
	Idle,
	Warning,
	Executing,
	Recovery
};

/** Optional reusable attacks. Existing enemies leave automatic patterns disabled. */
UCLASS(ClassGroup = (TunaSweeper), meta = (BlueprintSpawnableComponent))
class TUNASWEEPER_API UTunaSweeperCombatPatternComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTunaSweeperCombatPatternComponent();

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Patterns")
	bool TryStartPattern(ETunaSweeperCombatPattern Pattern, AActor* TargetActor);

	/** Called by the normal enemy controller only after acquiring a visible hostile target. */
	bool TryStartAutomaticPattern(AActor* TargetActor);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Patterns")
	void CancelPatterns(bool bDestroySummons = true);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Patterns")
	bool IsPatternActive() const { return Phase != ETunaSweeperCombatPatternPhase::Idle; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Patterns")
	ETunaSweeperCombatPatternPhase GetPhase() const { return Phase; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Patterns")
	ETunaSweeperCombatPattern GetActivePattern() const { return ActivePattern; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patterns")
	bool bAutomaticPatterns = false;

	/** Select one pattern for a teaching enemy, or combine patterns for a boss. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patterns")
	TArray<ETunaSweeperCombatPattern> PatternSequence;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patterns", meta = (ClampMin = "0.1", Units = "s"))
	float CooldownSeconds = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patterns", meta = (ClampMin = "0.1", Units = "s"))
	float RecoverySeconds = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patterns", meta = (ClampMin = "100.0", Units = "cm"))
	float ActivationRange = 2400.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patterns|Charge")
	TSubclassOf<ATunaSweeperAttackTelegraph> ChargeTelegraphClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patterns|Charge", meta = (ClampMin = "0.2", Units = "s"))
	float ChargeWarningSeconds = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patterns|Charge", meta = (ClampMin = "100.0", Units = "cm"))
	float ChargeDistance = 1000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patterns|Charge", meta = (ClampMin = "100.0", Units = "cm/s"))
	float ChargeSpeed = 1500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patterns|Charge", meta = (ClampMin = "0.0"))
	float ChargeDamage = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patterns|Turret")
	TSubclassOf<ATunaSweeperMissileTurret> MissileTurretClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patterns|Turret", meta = (ClampMin = "1", ClampMax = "8"))
	int32 MaxActiveTurrets = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patterns|Minions")
	TSubclassOf<ATunaSweeperRollingRobotMinion> RollingMinionClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patterns|Minions", meta = (ClampMin = "1", ClampMax = "24"))
	int32 MinionsPerWave = 5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patterns|Minions", meta = (ClampMin = "1", ClampMax = "64"))
	int32 MaxActiveMinions = 15;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patterns|Minions", meta = (ClampMin = "0.05", Units = "s"))
	float MinionSpawnInterval = 0.18f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patterns|Minions", meta = (ClampMin = "0.0", ClampMax = "160.0", Units = "deg"))
	float MinionFanAngle = 75.0f;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	bool IsOwnerAvailable() const;
	bool IsValidTarget(AActor* Actor) const;
	bool FindGround(const FVector& NearLocation, FVector& OutGround) const;
	bool PrepareCharge(AActor* TargetActor);
	void TickCharge(float DeltaTime);
	void ApplyChargeDamage(const FVector& Start, const FVector& End);
	bool SpawnTurret(AActor* TargetActor);
	void SpawnNextMinion();
	void HoldOwnerMovement();
	void RestoreOwnerMovement();
	void EnterRecovery();
	void FinishPattern();
	void RemoveExpiredSummons();

	UPROPERTY(Transient)
	TObjectPtr<ATunaSweeperAttackTelegraph> Warning;

	TWeakObjectPtr<AActor> Target;
	TArray<TWeakObjectPtr<ATunaSweeperMissileTurret>> Turrets;
	TArray<TWeakObjectPtr<ATunaSweeperRollingRobotMinion>> Minions;
	TSet<TWeakObjectPtr<AActor>> ChargeVictims;
	ETunaSweeperCombatPatternPhase Phase = ETunaSweeperCombatPatternPhase::Idle;
	ETunaSweeperCombatPattern ActivePattern = ETunaSweeperCombatPattern::Charge;
	FVector ChargeStart = FVector::ZeroVector;
	FVector ChargeEnd = FVector::ZeroVector;
	FVector LockedDirection = FVector::ForwardVector;
	float PhaseSeconds = 0.0f;
	float ChargeTrailCountdown = 0.0f;
	float NextMinionSeconds = 0.0f;
	double NextPatternTime = 0.0;
	int32 SequenceIndex = 0;
	int32 WaveSpawnIndex = 0;
	int32 WaveSize = 0;
	int32 ChargeSerial = 0;
	uint8 SavedMovementMode = 0;
	uint8 SavedCustomMovementMode = 0;
	bool bSavedOrientToMovement = false;
	bool bSavedUseControllerRotationYaw = false;
	bool bMovementHeld = false;
};
