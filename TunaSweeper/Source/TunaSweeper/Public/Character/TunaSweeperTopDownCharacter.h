#pragma once

#include "CoreMinimal.h"
#include "Component/TunaSweeperVitalsComponent.h"
#include "GameFramework/Character.h"
#include "TunaSweeperTopDownCharacter.generated.h"

class ATunaSweeperWeapon;
class UCameraComponent;
class UInputAction;
class UInputMappingContext;
class UMediaSource;
class USceneComponent;
class USpringArmComponent;
class UStaticMeshComponent;
class UTunaSweeperLevelTransitionWidget;
struct FInputActionValue;

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperTopDownCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ATunaSweeperTopDownCharacter();

	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void PawnClientRestart() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual float TakeDamage(
		float DamageAmount,
		struct FDamageEvent const& DamageEvent,
		AController* EventInstigator,
		AActor* DamageCauser) override;

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Aiming")
	void SetAimWorldPoint(const FVector& WorldPoint);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Aiming")
	FVector GetAimDirection() const { return AimDirection; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Aiming")
	bool IsAiming() const { return bIsAiming; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Vitals")
	UTunaSweeperVitalsComponent* GetVitalsComponent() const { return VitalsComponent; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Death")
	bool IsDead() const { return bIsDead; }

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Input")
	void CancelActiveGameplayActions();

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Weapon")
	bool SelectWeaponSlot(int32 SlotNumber);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Weapon")
	int32 GetSelectedWeaponSlotNumber() const { return SelectedWeaponSlotNumber; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Weapon")
	bool IsWeaponReloading() const { return bIsReloading; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Weapon")
	float GetReloadProgress() const;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Weapon")
	bool IsAmmoSelectionOpen() const { return bAmmoSelectionOpen; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Weapon")
	int32 GetAmmoSelectionFocusIndex() const { return AmmoSelectionFocusIndex; }

	void GetAmmoSelectionItemIds(TArray<int32>& OutAmmoItemIds) const;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> VisualMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> WeaponAttachPoint;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCameraComponent> TopDownCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UTunaSweeperVitalsComponent> VitalsComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TSoftObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TSoftObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TSoftObjectPtr<UInputAction> FireAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TSoftObjectPtr<UInputAction> AimAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TSoftObjectPtr<UInputAction> InteractAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TSoftObjectPtr<UInputAction> InventoryAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TSoftObjectPtr<UInputAction> ReloadAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TSoftObjectPtr<UInputAction> AmmoSelectAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TSoftObjectPtr<UInputAction> AmmoFocusAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
	TSoftClassPtr<ATunaSweeperWeapon> DefaultWeaponClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
	float FireInterval = 0.1f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement")
	float BaseWalkSpeed = 600.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera")
	float DefaultCameraFOV = 70.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera")
	float AimCameraFOV = 55.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera")
	float AimCameraLeadDistance = 260.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera")
	float CameraInterpSpeed = 10.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Death", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float RespawnDelaySeconds = 2.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Death", meta = (ClampMin = "0.01", UIMin = "0.01"))
	float RespawnFadeToBlackDuration = 0.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Death", meta = (ClampMin = "0.01", UIMin = "0.01"))
	float RespawnFadeFromBlackDuration = 0.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Death")
	FName RespawnTargetLevelName = FName(TEXT("BunkerMap"));

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Death")
	TSoftObjectPtr<UMediaSource> RespawnMediaSource;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Death")
	TSoftClassPtr<UTunaSweeperLevelTransitionWidget> RespawnTransitionWidgetClass;

private:
	void AddDefaultInputMapping() const;
	void EnsureEquippedWeaponActor();
	void ClearEquippedWeaponActor();
	UFUNCTION()
	void HandleVitalsChanged(const FTunaSweeperVitalsState& VitalsState);
	void HandleMove(const FInputActionValue& Value);
	void BeginFire(const FInputActionValue& Value);
	void EndFire(const FInputActionValue& Value);
	void BeginAim(const FInputActionValue& Value);
	void EndAim(const FInputActionValue& Value);
	void HandleInteract(const FInputActionValue& Value);
	void HandleInventory(const FInputActionValue& Value);
	void HandleReload(const FInputActionValue& Value);
	void HandleAmmoSelect(const FInputActionValue& Value);
	void HandleAmmoFocus(const FInputActionValue& Value);
	void FireWeapon();
	bool IsGameplayActionInputLocked() const;
	bool CanUseSelectedWeaponSlot();
	void StartReload();
	void CompleteReload();
	void CancelReload();
	void OpenAmmoSelection();
	void ConfirmAmmoSelection();
	void CloseAmmoSelection();
	void MoveAmmoSelectionFocus(int32 FocusDelta);
	void RefreshSelectedWeaponAfterInventoryChanged();
	void UpdateAimingVisuals(float DeltaSeconds);
	void UpdateCarryWeightMovementSpeed();
	void HandleDeath();
	void StartRespawnTransition();

	UPROPERTY(Transient)
	TObjectPtr<ATunaSweeperWeapon> EquippedWeapon;

	FTimerHandle FireTimerHandle;
	FTimerHandle ReloadTimerHandle;
	FTimerHandle RespawnTransitionTimerHandle;
	FVector AimWorldPoint = FVector::ZeroVector;
	FVector AimDirection = FVector::ForwardVector;
	TArray<int32> AmmoSelectionItemIds;
	int32 SelectedWeaponSlotNumber = 0;
	int32 PendingReloadAmmoItemId = INDEX_NONE;
	int32 AmmoSelectionFocusIndex = INDEX_NONE;
	float ReloadStartWorldSeconds = 0.0f;
	float ReloadDurationSeconds = 0.0f;
	bool bFireHeld = false;
	bool bIsAiming = false;
	bool bIsDead = false;
	bool bIsReloading = false;
	bool bAmmoSelectionOpen = false;
};
