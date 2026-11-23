#pragma once
#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "TunaSweeperATVRiderAnimInstance.generated.h"

/** Relaxed Luna riding posture; saddle and footplates do not constrain the limbs. */
UCLASS(Transient)
class TUNASWEEPER_API UTunaSweeperATVRiderAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
protected:
	virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;
	virtual void DestroyAnimInstanceProxy(FAnimInstanceProxy* Proxy) override;
};

/** Uses the seated limb solver with a cushion and armrests in the chair's frame. */
UCLASS(Transient)
class TUNASWEEPER_API UTunaSweeperLiftChairAnimInstance : public UTunaSweeperATVRiderAnimInstance
{
	GENERATED_BODY()
};
