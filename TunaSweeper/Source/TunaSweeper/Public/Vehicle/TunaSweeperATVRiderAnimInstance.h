#pragma once
#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "TunaSweeperATVRiderAnimInstance.generated.h"

/** Seated Luna pose with handlebar and footrest contact; used only while mounted. */
UCLASS(Transient)
class TUNASWEEPER_API UTunaSweeperATVRiderAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
protected:
	virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;
	virtual void DestroyAnimInstanceProxy(FAnimInstanceProxy* Proxy) override;
};
