#pragma once

#include "CoreMinimal.h"
#include "GameFramework/DamageType.h"
#include "TunaSweeperThrowableDamageType.generated.h"

UCLASS(BlueprintType)
class TUNASWEEPER_API UTunaSweeperThrowableDamageType : public UDamageType
{
	GENERATED_BODY()

public:
	UTunaSweeperThrowableDamageType();
};

UCLASS(BlueprintType)
class TUNASWEEPER_API UTunaSweeperGrenadeDamageType : public UTunaSweeperThrowableDamageType
{
	GENERATED_BODY()
};

UCLASS(BlueprintType)
class TUNASWEEPER_API UTunaSweeperDynamiteDamageType : public UTunaSweeperThrowableDamageType
{
	GENERATED_BODY()
};
