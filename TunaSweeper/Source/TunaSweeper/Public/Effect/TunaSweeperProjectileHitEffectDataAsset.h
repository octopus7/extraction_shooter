#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "TunaSweeperProjectileHitEffectDataAsset.generated.h"

class ATunaSweeperProjectileHitBurstActor;

USTRUCT(BlueprintType)
struct TUNASWEEPER_API FTunaSweeperProjectileHitEffectDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TunaSweeper|Projectile Hit Effect")
	FName EffectId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TunaSweeper|Projectile Hit Effect")
	TSoftClassPtr<ATunaSweeperProjectileHitBurstActor> EffectActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TunaSweeper|Projectile Hit Effect")
	FLinearColor BurstColor = FLinearColor(1.0f, 0.03f, 0.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TunaSweeper|Projectile Hit Effect")
	FVector SpawnScale = FVector::OneVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TunaSweeper|Projectile Hit Effect", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float SurfaceOffsetCm = 1.0f;
};

UCLASS(BlueprintType)
class TUNASWEEPER_API UTunaSweeperProjectileHitEffectDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Projectile Hit Effect")
	bool TryGetHitEffect(FName EffectId, FTunaSweeperProjectileHitEffectDefinition& OutDefinition) const;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TunaSweeper|Projectile Hit Effect")
	TArray<FTunaSweeperProjectileHitEffectDefinition> HitEffects;
};
