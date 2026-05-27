#pragma once

#include "CoreMinimal.h"
#include "AI/TunaSweeperRollingBomber.h"
#include "Animation/AnimInstance.h"
#include "TunaSweeperRollingBomberAnimInstance.generated.h"

USTRUCT(BlueprintType)
struct TUNASWEEPER_API FTunaSweeperRollingBomberAnimFootIKState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Rolling Bomber|IK")
	FVector EffectorWorldLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Rolling Bomber|IK")
	FVector JointTargetWorldLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Rolling Bomber|IK")
	FVector EffectorComponentLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Rolling Bomber|IK")
	FVector JointTargetComponentLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Rolling Bomber|IK")
	bool bIsStepping = false;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Rolling Bomber|IK", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float StepAlpha = 0.0f;
};

UCLASS(BlueprintType, Blueprintable, Transient)
class TUNASWEEPER_API UTunaSweeperRollingBomberAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Rolling Bomber")
	ATunaSweeperRollingBomber* GetRollingBomberOwner() const { return RollingBomberOwner; }

protected:
	UPROPERTY(BlueprintReadOnly, Transient, Category = "TunaSweeper|Rolling Bomber")
	TObjectPtr<ATunaSweeperRollingBomber> RollingBomberOwner;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "TunaSweeper|Rolling Bomber")
	ETunaSweeperRollingBomberMode Mode = ETunaSweeperRollingBomberMode::ProjectileAttack;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "TunaSweeper|Rolling Bomber")
	bool bHasRollingBomberOwner = false;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "TunaSweeper|Rolling Bomber")
	bool bIsSpawnPhysicsMode = false;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "TunaSweeper|Rolling Bomber")
	bool bIsStandingUpFromSpawnMode = false;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "TunaSweeper|Rolling Bomber")
	bool bIsSpawnTransitionMode = false;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "TunaSweeper|Rolling Bomber")
	bool bIsProjectileAttackMode = false;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "TunaSweeper|Rolling Bomber")
	bool bIsFoldingLegsMode = false;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "TunaSweeper|Rolling Bomber")
	bool bIsRollingMode = false;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "TunaSweeper|Rolling Bomber")
	bool bIsRecoveringLegsMode = false;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "TunaSweeper|Rolling Bomber")
	bool bIsSelfDestructedMode = false;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "TunaSweeper|Rolling Bomber|Movement")
	float GroundSpeed = 0.0f;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "TunaSweeper|Rolling Bomber|Roll")
	FVector RollDirectionWorld = FVector::ForwardVector;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "TunaSweeper|Rolling Bomber|Roll")
	float RollDistanceTraveled = 0.0f;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "TunaSweeper|Rolling Bomber|Roll")
	float BodyRollDegrees = 0.0f;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "TunaSweeper|Rolling Bomber|IK")
	bool bLegIKEnabled = true;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "TunaSweeper|Rolling Bomber|IK", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float LegFoldAlpha = 0.0f;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "TunaSweeper|Rolling Bomber|IK", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SpawnStandUpAlpha = 0.0f;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "TunaSweeper|Rolling Bomber|IK")
	FTunaSweeperRollingBomberAnimFootIKState LeftFootIK;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "TunaSweeper|Rolling Bomber|IK")
	FTunaSweeperRollingBomberAnimFootIKState RightFootIK;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "TunaSweeper|Rolling Bomber|Eye")
	bool bEyeChargeWarningActive = false;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "TunaSweeper|Rolling Bomber|Eye")
	FLinearColor EyeEmissiveColor = FLinearColor::White;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "TunaSweeper|Rolling Bomber|Eye")
	float EyeEmissiveStrength = 0.0f;

private:
	void RefreshOwner();
	void ResetAnimState();
	void UpdateFootIKState(
		FTunaSweeperRollingBomberAnimFootIKState& OutAnimFootState,
		const FTunaSweeperRollingBomberFootIKState& SourceFootState) const;
};
