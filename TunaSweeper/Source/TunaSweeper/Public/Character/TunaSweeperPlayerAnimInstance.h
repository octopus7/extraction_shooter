#pragma once

#include "Animation/AnimInstance.h"
#include "TunaSweeperPlayerAnimInstance.generated.h"

UCLASS(Transient, Blueprintable)
class TUNASWEEPER_API UTunaSweeperPlayerAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Locomotion")
	bool bUseUnarmedLocomotion = false;

	/** Signed degrees from the player's facing. Positive angles move to the right. */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Locomotion")
	float TravelAngle = 0.f;

	/** Blend-space rows are idle (0), walking (1), and sprinting (2). */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Locomotion")
	float UnarmedGait = 0.f;
};
