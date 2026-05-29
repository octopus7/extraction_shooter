#pragma once

#include "CoreMinimal.h"
#include "Component/TunaSweeperVitalsComponent.h"
#include "GameFramework/Character.h"
#include "TunaSweeperTopDownCharacter.generated.h"

class ATunaSweeperWeapon;
class ATunaSweeperMeleeImpactBurstActor;
class ATunaSweeperMeleeSwingTrailActor;
class UCameraComponent;
class UInputAction;
class UInputMappingContext;
class UMediaSource;
class UPrimitiveComponent;
class USceneComponent;
class USpringArmComponent;
class UStaticMeshComponent;
class UTunaSweeperStaminaGaugeWidget;
class UTunaSweeperPlayerVisionComponent;
class UTunaSweeperLevelTransitionWidget;
class UWidgetComponent;
struct FDamageEvent;
struct FInputActionValue;

UENUM(BlueprintType)
enum class ETunaSweeperHitReactionType : uint8
{
	Default,
	Melee,
	Projectile,
	Heavy
};

UENUM(BlueprintType)
enum class ETunaSweeperPlayerCameraMode : uint8
{
	Default,
	TopDown,
	LowFront
};

USTRUCT(BlueprintType)
struct TUNASWEEPER_API FTunaSweeperPlayerCameraModeSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Camera", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float TargetArmLength = 1200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Camera")
	FRotator BoomRotation = FRotator(-60.0f, 0.0f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Camera")
	FVector TargetOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Camera", meta = (ClampMin = "5.0", ClampMax = "170.0", UIMin = "5.0", UIMax = "170.0"))
	float DefaultFOV = 70.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Camera", meta = (ClampMin = "5.0", ClampMax = "170.0", UIMin = "5.0", UIMax = "170.0"))
	float AimFOV = 55.0f;
};

USTRUCT(BlueprintType)
struct TUNASWEEPER_API FTunaSweeperCameraHitReactionSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Camera", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float Duration = 0.32f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Camera", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float LocationAmplitude = 48.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Camera", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float RollAmplitudeDegrees = 0.68f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Camera", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float FOVAmplitudeDegrees = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Camera", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float Frequency = 9.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Camera", meta = (ClampMin = "0.01", UIMin = "0.01"))
	float DamageScaleReference = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Camera", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float MinDamageScale = 0.9f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Camera", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float MaxDamageScale = 1.25f;
};

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperTopDownCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ATunaSweeperTopDownCharacter();

	virtual void Tick(float DeltaSeconds) override;
	virtual void OnConstruction(const FTransform& Transform) override;
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

	void SetAimWorldHit(const FVector& WorldPoint, const FHitResult& AimHit);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Aiming")
	FVector GetAimDirection() const { return AimDirection; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Aiming")
	FVector2D GetWeaponRecoilCrosshairScreenOffset() const;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Aiming")
	bool IsAiming() const { return bIsAiming; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Vitals")
	UTunaSweeperVitalsComponent* GetVitalsComponent() const { return VitalsComponent; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Vision")
	UTunaSweeperPlayerVisionComponent* GetPlayerVisionComponent() const { return PlayerVisionComponent; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Camera")
	ETunaSweeperPlayerCameraMode GetPlayerCameraMode() const { return CurrentCameraMode; }

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Camera")
	void CyclePlayerCameraMode();

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Death")
	bool IsDead() const { return bIsDead; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Stamina")
	float GetStamina() const { return CurrentStamina; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Stamina")
	float GetMaxStamina() const { return MaxStamina; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Stamina")
	float GetStaminaPercent() const;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Stamina")
	bool IsSprinting() const { return bIsSprinting; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Roll")
	bool IsRolling() const { return bIsRolling; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Combat")
	bool IsDamageInvulnerable() const { return bIsRolling; }

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Input")
	void CancelActiveGameplayActions();

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Housing")
	void SetHousingModeVisualHidden(bool bShouldHide);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Weapon")
	bool SelectWeaponSlot(int32 SlotNumber);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Weapon")
	bool SelectMeleeWeapon();

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Weapon")
	int32 GetSelectedWeaponSlotNumber() const { return SelectedWeaponSlotNumber; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Weapon")
	bool IsMeleeWeaponSelected() const { return bMeleeWeaponSelected; }

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
	TObjectPtr<USceneComponent> RollWeaponHandAttachPoint;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCameraComponent> TopDownCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UTunaSweeperVitalsComponent> VitalsComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UTunaSweeperPlayerVisionComponent> PlayerVisionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UWidgetComponent> StaminaGaugeWidgetComponent;

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
	TSoftObjectPtr<UInputAction> MapAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TSoftObjectPtr<UInputAction> ReloadAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TSoftObjectPtr<UInputAction> AmmoSelectAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TSoftObjectPtr<UInputAction> AmmoFocusAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TSoftObjectPtr<UInputAction> CameraModeAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TSoftObjectPtr<UInputAction> SprintAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TSoftObjectPtr<UInputAction> RollAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
	TSoftClassPtr<ATunaSweeperWeapon> DefaultWeaponClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Melee")
	TSoftClassPtr<ATunaSweeperMeleeSwingTrailActor> MeleeSwingTrailActorClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Melee")
	TSoftClassPtr<ATunaSweeperMeleeImpactBurstActor> MeleeImpactBurstActorClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
	float FireInterval = 0.1f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Spread Recoil", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float WeaponRecoilScreenPixelsPerDegree = 5.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Melee", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float MeleeAttackDamage = 8.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Melee", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float MeleeAttackRange = 165.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Melee", meta = (ClampMin = "0.0", ClampMax = "180.0", UIMin = "0.0", UIMax = "180.0"))
	float MeleeAttackHalfAngleDegrees = 55.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Melee", meta = (ClampMin = "0.01", UIMin = "0.01"))
	float MeleeAttackCooldownSeconds = 0.58f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Melee", meta = (ClampMin = "0.01", UIMin = "0.01"))
	float MeleeSwingDurationSeconds = 0.28f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Melee", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float MeleeJudgementTimeSeconds = 0.11f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Melee", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float MeleeImpactHeight = 55.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement")
	float BaseWalkSpeed = 600.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Sprint", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float SprintSpeedMultiplier = 1.55f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Sprint", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float MaxStamina = 100.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Sprint", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float SprintStaminaDrainPerSecond = 25.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Sprint", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float StaminaRegenPerSecond = 18.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Sprint", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float StaminaGaugeFadeInterpSpeed = 8.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Roll", meta = (ClampMin = "0.01", UIMin = "0.01"))
	float RollDurationSeconds = 0.55f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Roll", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float RollDistance = 420.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Roll", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float RollStaminaCost = 30.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Roll")
	bool bUseTemporaryRollVisualRotation = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Roll")
	float TemporaryRollVisualRightAxisDegrees = 360.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Roll")
	FName RollWeaponHandSocketName = FName(TEXT("hand_r"));

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera")
	float DefaultCameraFOV = 70.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera")
	float AimCameraFOV = 55.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera", meta = (DisplayName = "Camera Cursor Lead Max Distance", ClampMin = "0.0", UIMin = "0.0"))
	float AimCameraLeadDistance = 200.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera")
	float CameraInterpSpeed = 10.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Modes")
	FTunaSweeperPlayerCameraModeSettings TopDownCameraModeSettings;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Modes")
	FTunaSweeperPlayerCameraModeSettings LowFrontCameraModeSettings;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Hit Reaction")
	FTunaSweeperCameraHitReactionSettings DefaultCameraHitReaction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Hit Reaction")
	TMap<ETunaSweeperHitReactionType, FTunaSweeperCameraHitReactionSettings> CameraHitReactionOverrides;

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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Death|Ragdoll")
	bool bEnableDeathRagdoll = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Death|Ragdoll")
	FName DeathRagdollCollisionProfileName = FName(TEXT("Ragdoll"));

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Death|Ragdoll", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float DeathRagdollHorizontalImpulse = 12000.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Death|Ragdoll", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float DeathRagdollUpwardImpulse = 1800.0f;

private:
	void AddDefaultInputMapping() const;
	void EnsureEquippedWeaponActor();
	void ClearEquippedWeaponActor();
	TSubclassOf<ATunaSweeperWeapon> ResolveEquippedWeaponClass() const;
	void ApplyEquippedWeaponAttachmentVisuals();
	void UpdateEquippedWeaponLaserSightBeam();
	bool IsSelectedWeaponLaserSightEquipped() const;
	UFUNCTION()
	void HandleVitalsChanged(const FTunaSweeperVitalsState& VitalsState);
	void HandleMove(const FInputActionValue& Value);
	void BeginFire(const FInputActionValue& Value);
	void EndFire(const FInputActionValue& Value);
	void BeginAim(const FInputActionValue& Value);
	void EndAim(const FInputActionValue& Value);
	void HandleInteract(const FInputActionValue& Value);
	void HandleInventory(const FInputActionValue& Value);
	void HandleMap(const FInputActionValue& Value);
	void HandleReload(const FInputActionValue& Value);
	void HandleAmmoSelect(const FInputActionValue& Value);
	void HandleAmmoFocus(const FInputActionValue& Value);
	void HandleCameraMode(const FInputActionValue& Value);
	void BeginSprint(const FInputActionValue& Value);
	void EndSprint(const FInputActionValue& Value);
	void BeginRoll(const FInputActionValue& Value);
	void HandleMoveStopped(const FInputActionValue& Value);
	void FireWeapon();
	bool IsGameplayActionInputLocked() const;
	bool CanUseSelectedWeaponSlot();
	bool CanUseSelectedMeleeWeapon();
	bool RestoreRuntimeSelectedWeaponSelection();
	void StartMeleeAttack();
	void UpdateMeleeSwing(float DeltaSeconds);
	void CancelMeleeSwing();
	void ResetEquippedWeaponRelativeTransform();
	void ApplyEquippedMeleeWeaponVisual();
	void ApplyMeleeAttackJudgement();
	void SpawnMeleeSwingEffect(const FVector& AttackDirection);
	void SpawnMeleeImpactBurst(const FVector& HitLocation, const FVector& BurstDirection);
	void StartReload();
	void CompleteReload();
	void CancelReload();
	void OpenAmmoSelection();
	void ConfirmAmmoSelection();
	void CloseAmmoSelection();
	void MoveAmmoSelectionFocus(int32 FocusDelta);
	void RefreshSelectedWeaponAfterInventoryChanged();
	void RefreshCharacterVisualVisibility();
	void CacheBaseSurvivalStats();
	void ApplyExperienceLevelStatBonuses();
	void ApplyBunkerPeaceZoneVitalsRules();
	void ApplySandbagCoverOutlinePostProcess();
	float ResolveCameraCursorLeadRatio() const;
	void UpdateAimingVisuals(float DeltaSeconds);
	void UpdateSprintAndStamina(float DeltaSeconds);
	void UpdateRoll(float DeltaSeconds);
	void UpdateMovementSpeed();
	void UpdateStaminaGauge(float DeltaSeconds);
	void UpdateWeaponSpreadRecoil(float DeltaSeconds);
	void ResetWeaponSpreadRecoil();
	bool TryGetSelectedWeaponTypeTag(FName& OutWeaponTypeTag) const;
	float ResolveWeaponSpreadHalfAngleDegrees(FName WeaponTypeTag) const;
	void AddWeaponSpreadRecoilShot(FName WeaponTypeTag);
	void FinishRoll();
	void SetRollProjectileCollisionPassthrough(bool bEnabled);
	void AttachWeaponForRoll();
	void RestoreWeaponAfterRoll();
	USceneComponent* ResolveRollWeaponAttachParent(FName& OutSocketName) const;
	void ApplyTemporaryRollVisualRotation(float NormalizedRollTime);
	void RestoreTemporaryRollVisualRotation();
	FVector ResolveRollDirection() const;
	bool HasActiveMoveInput() const;
	FTunaSweeperPlayerCameraModeSettings ResolveCurrentCameraModeSettings() const;
	void TriggerDamageCameraReaction(float DamageAmount, FDamageEvent const& DamageEvent, AActor* DamageCauser);
	ETunaSweeperHitReactionType ResolveDamageCameraReactionType(FDamageEvent const& DamageEvent, AActor* DamageCauser) const;
	FTunaSweeperCameraHitReactionSettings ResolveDamageCameraReactionSettings(FDamageEvent const& DamageEvent, AActor* DamageCauser) const;
	FVector ResolveDamageCameraReactionDirection(AActor* DamageCauser) const;
	FVector UpdateDamageCameraReaction(float DeltaSeconds, float& OutRollDegrees, float& OutFOVDegrees);
	void HandleDeath();
	void ApplyDeathRagdoll();
	FVector ResolveDeathRagdollImpulse() const;
	void StartRespawnTransition();

	UPROPERTY(Transient)
	TObjectPtr<ATunaSweeperWeapon> EquippedWeapon;

	FTimerHandle FireTimerHandle;
	FTimerHandle ReloadTimerHandle;
	FTimerHandle RespawnTransitionTimerHandle;
	FVector AimWorldPoint = FVector::ZeroVector;
	FVector AimIntentWorldPoint = FVector::ZeroVector;
	FVector AimDirection = FVector::ForwardVector;
	bool bHasAimWorldPoint = false;
	bool bHasAimIntent = false;
	FRotator DefaultCameraRelativeRotation = FRotator::ZeroRotator;
	FRotator DefaultCameraBoomRotation = FRotator(-60.0f, 0.0f, 0.0f);
	FRotator CurrentCameraBoomRotation = FRotator(-60.0f, 0.0f, 0.0f);
	FVector DefaultCameraTargetOffset = FVector::ZeroVector;
	FVector CurrentCameraModeOffset = FVector::ZeroVector;
	FVector CurrentCameraAimOffset = FVector::ZeroVector;
	TArray<int32> AmmoSelectionItemIds;
	FTunaSweeperCameraHitReactionSettings ActiveCameraHitReaction;
	ETunaSweeperPlayerCameraMode CurrentCameraMode = ETunaSweeperPlayerCameraMode::Default;
	FVector CameraHitReactionDirection = FVector::ForwardVector;
	FVector LastDamageImpulseDirection = FVector::ZeroVector;
	int32 SelectedWeaponSlotNumber = 0;
	int32 PendingReloadAmmoItemId = INDEX_NONE;
	int32 AmmoSelectionFocusIndex = INDEX_NONE;
	float DefaultCameraArmLength = 1200.0f;
	float CurrentCameraArmLength = 1200.0f;
	float ReloadStartWorldSeconds = 0.0f;
	float ReloadDurationSeconds = 0.0f;
	float CurrentCameraBaseFOV = 0.0f;
	float CurrentStamina = 100.0f;
	float BaseMaxHealth = 100.0f;
	float BaseMaxFood = 100.0f;
	float BaseMaxHydration = 100.0f;
	float BaseMaxStamina = 100.0f;
	float StaminaGaugeOpacity = 0.0f;
	float RollElapsedSeconds = 0.0f;
	float MeleeSwingElapsedSeconds = 0.0f;
	float LastMeleeAttackWorldSeconds = -1000.0f;
	float CameraHitReactionElapsed = 0.0f;
	float CameraHitReactionScale = 1.0f;
	float CameraHitReactionPhase = 0.0f;
	FVector RollDirection = FVector::ForwardVector;
	FVector2D WeaponRecoilOffsetDegrees = FVector2D::ZeroVector;
	FVector2D CurrentMoveInput = FVector2D::ZeroVector;
	FRotator DefaultSkeletalMeshRelativeRotation = FRotator::ZeroRotator;
	FRotator DefaultVisualMeshRelativeRotation = FRotator::ZeroRotator;
	TWeakObjectPtr<USceneComponent> SavedWeaponAttachParent;
	TWeakObjectPtr<AActor> AimIntentActor;
	TWeakObjectPtr<UPrimitiveComponent> AimIntentComponent;
	FName SavedWeaponAttachSocketName = NAME_None;
	FName WeaponRecoilTypeTag = NAME_None;
	FTransform SavedWeaponRelativeTransform = FTransform::Identity;
	TEnumAsByte<ECollisionResponse> SavedProjectileCollisionResponse = ECR_Block;
	bool bFireHeld = false;
	bool bIsAiming = false;
	bool bIsDead = false;
	bool bIsReloading = false;
	bool bAmmoSelectionOpen = false;
	bool bMeleeWeaponSelected = false;
	bool bMeleeSwingActive = false;
	bool bMeleeJudgementApplied = false;
	bool bCameraHitReactionActive = false;
	bool bSprintInputHeld = false;
	bool bIsSprinting = false;
	bool bSprintLockedUntilReleased = false;
	bool bIsRolling = false;
	bool bHasSavedProjectileCollisionResponse = false;
	bool bRollVisualRotationApplied = false;
	bool bWeaponAttachedForRoll = false;
	bool bHousingModeVisualHidden = false;
	bool bBaseSurvivalStatsCached = false;
};
