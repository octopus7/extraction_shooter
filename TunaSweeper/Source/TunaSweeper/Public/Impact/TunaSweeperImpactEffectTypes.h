#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "TunaSweeperImpactEffectTypes.generated.h"

class UMaterialInterface;
class UNiagaraSystem;
class USoundBase;

USTRUCT(BlueprintType)
struct TUNASWEEPER_API FTunaSweeperImpactEffectSpec
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Impact")
	TSoftObjectPtr<UNiagaraSystem> NiagaraSystem;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Impact")
	TSoftObjectPtr<USoundBase> Sound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Impact|Decal")
	TSoftObjectPtr<UMaterialInterface> DecalMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Impact|Decal", meta = (ClampMin = "0.0"))
	FVector DecalSize = FVector(8.0f, 8.0f, 8.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Impact|Decal", meta = (ClampMin = "0.0"))
	float DecalLifeSpanSeconds = 10.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Impact", meta = (ClampMin = "0.0"))
	float EffectScale = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Impact", meta = (ClampMin = "0.0"))
	float SurfaceOffsetCm = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Impact|Decal")
	bool bSpawnDecal = true;

	bool IsEmpty() const
	{
		return NiagaraSystem.IsNull() && Sound.IsNull() && (!bSpawnDecal || DecalMaterial.IsNull());
	}
};

USTRUCT(BlueprintType)
struct TUNASWEEPER_API FTunaSweeperImpactSurfaceRule
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Impact")
	TEnumAsByte<EPhysicalSurface> SurfaceType = SurfaceType_Default;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Impact")
	FTunaSweeperImpactEffectSpec Effect;
};

USTRUCT(BlueprintType)
struct TUNASWEEPER_API FTunaSweeperImpactTagRule
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Impact")
	FGameplayTagContainer RequiredTags;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Impact")
	FGameplayTagContainer BlockedTags;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Impact")
	int32 Priority = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Impact")
	FTunaSweeperImpactEffectSpec Effect;
};

USTRUCT(BlueprintType)
struct TUNASWEEPER_API FTunaSweeperImpactTargetResponse
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Impact")
	bool bOverrideSurface = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Impact", meta = (EditCondition = "bOverrideSurface"))
	TEnumAsByte<EPhysicalSurface> SurfaceOverride = SurfaceType_Default;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Impact")
	FGameplayTagContainer ResponseTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Impact")
	bool bSuppressDecal = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Impact", meta = (ClampMin = "0.0"))
	float EffectScaleMultiplier = 1.0f;
};

USTRUCT(BlueprintType)
struct TUNASWEEPER_API FTunaSweeperImpactResolveContext
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Impact")
	TEnumAsByte<EPhysicalSurface> SurfaceType = SurfaceType_Default;

	UPROPERTY(BlueprintReadWrite, Category = "Impact")
	FGameplayTagContainer TargetResponseTags;

	UPROPERTY(BlueprintReadWrite, Category = "Impact")
	bool bSuppressDecal = false;

	UPROPERTY(BlueprintReadWrite, Category = "Impact", meta = (ClampMin = "0.0"))
	float EffectScaleMultiplier = 1.0f;
};
