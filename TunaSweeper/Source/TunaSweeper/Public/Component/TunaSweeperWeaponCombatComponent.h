#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Weapon/TunaSweeperWeaponSpreadRecoilDataAsset.h"
#include "TunaSweeperWeaponCombatComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FTunaSweeperWeaponReloadStateChangedSignature,
	bool,
	bIsReloading,
	float,
	ReloadDurationSeconds);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FTunaSweeperWeaponReloadProgressChangedSignature,
	float,
	ReloadProgress);

UCLASS(ClassGroup = (TunaSweeper), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class TUNASWEEPER_API UTunaSweeperWeaponCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTunaSweeperWeaponCombatComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Weapon|Spread")
	void ConfigureSpreadRecoilDefinition(
		FName InWeaponTypeTag,
		const FTunaSweeperWeaponSpreadRecoilDefinition& InDefinition);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Weapon|Spread")
	void ClearSpreadRecoilDefinition();

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Weapon|Spread")
	void ResetSpreadRecoil();

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Weapon|Spread")
	float GetSpreadHalfAngleDegrees() const;

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Weapon|Spread")
	void AddSpreadRecoilShot();

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Weapon|Reload")
	bool StartReload(float ReloadSeconds);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Weapon|Reload")
	void FinishReload();

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Weapon|Reload")
	void CancelReload();

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Weapon|Reload")
	bool IsReloading() const { return bIsReloading; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Weapon|Reload")
	bool HasReloadFinished() const;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Weapon|Reload")
	float GetReloadProgress() const;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Weapon|Reload")
	float GetReloadDurationSeconds() const { return ReloadDurationSeconds; }

	UPROPERTY(BlueprintAssignable, Category = "TunaSweeper|Weapon|Reload")
	FTunaSweeperWeaponReloadStateChangedSignature OnReloadStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "TunaSweeper|Weapon|Reload")
	FTunaSweeperWeaponReloadProgressChangedSignature OnReloadProgressChanged;

private:
	void DecaySpreadRecoil(float DeltaSeconds);
	void BroadcastReloadProgressIfNeeded(bool bForceBroadcast = false);

	FTunaSweeperWeaponSpreadRecoilDefinition SpreadRecoilDefinition;
	FVector2D SpreadRecoilOffsetDegrees = FVector2D::ZeroVector;
	FName SpreadRecoilWeaponTypeTag = NAME_None;
	float ReloadStartWorldSeconds = 0.0f;
	float ReloadDurationSeconds = 0.0f;
	float LastBroadcastReloadProgress = -1.0f;
	bool bHasSpreadRecoilDefinition = false;
	bool bIsReloading = false;
};
