#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "TunaSweeperWeaponPresentationDataAsset.generated.h"

class UNiagaraSystem;
class USoundBase;

/** Shared visual and audio feedback for one weapon type. */
UCLASS(BlueprintType)
class TUNASWEEPER_API UTunaSweeperWeaponPresentationDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TunaSweeper|Weapon Presentation")
	FName WeaponTypeTag = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TunaSweeper|Weapon Presentation|Fire")
	TSoftObjectPtr<UNiagaraSystem> MuzzleFlashEffect;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TunaSweeper|Weapon Presentation|Fire")
	TSoftObjectPtr<USoundBase> FireSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TunaSweeper|Weapon Presentation|Fire")
	TSoftObjectPtr<USoundBase> EmptyFireSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TunaSweeper|Weapon Presentation|Reload")
	TSoftObjectPtr<USoundBase> ReloadStartSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TunaSweeper|Weapon Presentation|Reload")
	TSoftObjectPtr<USoundBase> ReloadCompleteSound;
};
