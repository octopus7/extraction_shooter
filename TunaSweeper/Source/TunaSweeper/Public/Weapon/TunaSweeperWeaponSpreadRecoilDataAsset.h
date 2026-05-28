#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "TunaSweeperWeaponSpreadRecoilDataAsset.generated.h"

USTRUCT(BlueprintType)
struct TUNASWEEPER_API FTunaSweeperWeaponSpreadRecoilDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TunaSweeper|Weapon Spread Recoil")
	FName WeaponTypeTag = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TunaSweeper|Weapon Spread Recoil", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float IncreasePerShot = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TunaSweeper|Weapon Spread Recoil", meta = (ClampMin = "0.01", UIMin = "0.01"))
	float MinimumSpreadHalfAngleDegrees = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TunaSweeper|Weapon Spread Recoil", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float MaximumSpreadHalfAngleDegrees = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TunaSweeper|Weapon Spread Recoil", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float DecreasePerSecond = 5.0f;
};

UCLASS(BlueprintType)
class TUNASWEEPER_API UTunaSweeperWeaponSpreadRecoilDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Weapon Spread Recoil")
	bool TryGetDefinition(FName WeaponTypeTag, FTunaSweeperWeaponSpreadRecoilDefinition& OutDefinition) const;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TunaSweeper|Weapon Spread Recoil")
	TArray<FTunaSweeperWeaponSpreadRecoilDefinition> WeaponTypeDefinitions;
};
