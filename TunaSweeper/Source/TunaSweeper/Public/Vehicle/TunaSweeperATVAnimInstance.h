#pragma once
#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "TunaSweeperATVAnimInstance.generated.h"
UCLASS(Transient)
class TUNASWEEPER_API UTunaSweeperATVAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
protected:
	virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;
	virtual void DestroyAnimInstanceProxy(FAnimInstanceProxy* Proxy) override;
};
