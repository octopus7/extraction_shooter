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
	void SpawnDefaultWeapon();
	UFUNCTION()
	void HandleVitalsChanged(const FTunaSweeperVitalsState& VitalsState);
	void HandleMove(const FInputActionValue& Value);
	void BeginFire(const FInputActionValue& Value);
	void EndFire(const FInputActionValue& Value);
	void BeginAim(const FInputActionValue& Value);
	void EndAim(const FInputActionValue& Value);
	void HandleInteract(const FInputActionValue& Value);
	void HandleInventory(const FInputActionValue& Value);
	void FireWeapon();
	void UpdateAimingVisuals(float DeltaSeconds);
	void UpdateCarryWeightMovementSpeed();
	void HandleDeath();
	void StartRespawnTransition();

	UPROPERTY(Transient)
	TObjectPtr<ATunaSweeperWeapon> EquippedWeapon;

	FTimerHandle FireTimerHandle;
	FTimerHandle RespawnTransitionTimerHandle;
	FVector AimWorldPoint = FVector::ZeroVector;
	FVector AimDirection = FVector::ForwardVector;
	bool bFireHeld = false;
	bool bIsAiming = false;
	bool bIsDead = false;
};
