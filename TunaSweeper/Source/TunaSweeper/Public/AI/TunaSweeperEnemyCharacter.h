#pragma once

#include "CoreMinimal.h"
#include "AI/TunaSweeperEnemyCombatProfile.h"
#include "GameFramework/Character.h"
#include "Subsystem/TunaSweeperItemDataSubsystem.h"
#include "TunaSweeperEnemyCharacter.generated.h"

class UStaticMeshComponent;
class USceneComponent;
class UTunaSweeperVisionSubjectComponent;
class UTunaSweeperEnemySensorDebugComponent;
class UTunaSweeperFactionComponent;
class UTunaSweeperSpeechBubbleWidget;
class UWidgetComponent;
class UMaterialInterface;
class UNiagaraSystem;
class ATunaSweeperProjectile;
class ATunaSweeperWeapon;
class ATunaSweeperLootContainerActor;
class ATunaSweeperMeleeImpactBurstActor;
class ATunaSweeperMeleeSwingTrailActor;
class ATunaSweeperEnemyDeathStrawberryBurstActor;

UENUM(BlueprintType)
enum class ETunaSweeperEnemyFireResult : uint8
{
	Fired,
	MagazineEmpty,
	Reloading,
	Cooldown,
	OutOfAmmo,
	FriendlyTarget,
	Blocked
};

USTRUCT(BlueprintType)
struct TUNASWEEPER_API FTunaSweeperEnemyWeaponRuntimeStatus
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Combat")
	int32 MagazineCapacity = 0;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Combat")
	int32 LoadedAmmo = 0;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Combat")
	int32 ReserveAmmo = 0;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Combat")
	bool bIsReloading = false;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Combat")
	float ReloadProgress = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Combat")
	float ReloadSeconds = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Combat")
	ETunaSweeperWeaponFireMode FireMode = ETunaSweeperWeaponFireMode::NotApplicable;
};

enum class ETunaSweeperEnemyStatusBubble : uint8
{
	None,
	Alert,
	Reload
};

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperEnemyCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ATunaSweeperEnemyCharacter();

	virtual float TakeDamage(
		float DamageAmount,
		struct FDamageEvent const& DamageEvent,
		AController* EventInstigator,
		AActor* DamageCauser) override;

	virtual FVector ResolveProjectileHitEffectLocation(const FHitResult& Hit) const;

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Combat")
	bool FireProjectileAt(AActor* TargetActor);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Combat")
	ETunaSweeperEnemyFireResult TryFireProjectileAt(AActor* TargetActor);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Combat")
	bool StartEnemyReload();

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Combat")
	FTunaSweeperEnemyWeaponRuntimeStatus GetEnemyWeaponRuntimeStatus();

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Combat")
	bool IsDead() const { return bIsDead; }

	// TEMP_VIDEO_BULLET_STORM: Remove this accessor with the matching debug checkbox after capture.
	bool IsTemporaryVideoBulletStormEnabled() const;

	bool AttackTarget(AActor* TargetActor);
	bool UsesMeleeAttack() const;
	float GetMeleeAttackRange() const;
	float GetMeleeApproachStartRange() const;
	float GetMeleeApproachStopRange() const;
	float GetMeleeTrackingRange() const;
	float GetMeleeAttackCooldownSeconds() const;
	bool TryApplyBleedTo(AActor* TargetActor) const;
	void SetAlertIndicatorVisible(bool bVisible);
	void ShowAlertSpeechBubble();
	void HideAlertSpeechBubble();
	UTunaSweeperFactionComponent* GetFactionComponent() const { return FactionComponent; }
	const FTunaSweeperEnemyCombatProfile& GetCombatProfile() const { return CombatProfile; }
	FName GetCombatProfileId() const { return CombatProfile.ProfileId; }
	void ConfigureCombatProfile(
		const FTunaSweeperEnemyCombatProfile& InCombatProfile,
		uint8 InFactionId,
		FName InSquadId,
		int32 InSquadSlot);

	void ConfigureSpawnData(
		const TSoftObjectPtr<UMaterialInterface>& InBodyMaterial,
		FName InEnemyId,
		int32 InDropContainerDefinitionId,
		int32 InDropContentsId,
		float InMaxHealth,
		int32 InExperienceValue,
		int32 InBleedingChanceBonus = 0,
		float InBleedingDurationBonusSeconds = 0.0f,
		int32 InWeaponItemId = INDEX_NONE,
		int32 InAmmoItemId = INDEX_NONE,
		int32 InReserveAmmoCount = INDEX_NONE,
		float InLootLoadedAmmoDeductionRatio = 0.35f,
		int32 InLootLoadedAmmoFlatDeduction = 0);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnDeathPresentationStarted();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> VisualMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> ForwardMarkerMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> AlertIndicatorMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> EnemyWeaponAttachPoint;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UWidgetComponent> EnemyReloadWidgetComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UWidgetComponent> EnemyStatusBubbleWidgetComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UTunaSweeperFactionComponent> FactionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UTunaSweeperVisionSubjectComponent> VisionSubjectComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UTunaSweeperEnemySensorDebugComponent> SensorDebugComponent;

public:
	UTunaSweeperEnemySensorDebugComponent* GetSensorDebugComponent() const { return SensorDebugComponent; }

protected:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
	TSoftClassPtr<ATunaSweeperProjectile> ProjectileClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Weapon")
	TSubclassOf<ATunaSweeperWeapon> EnemyWeaponClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSoftClassPtr<UTunaSweeperSpeechBubbleWidget> EnemyStatusBubbleWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
	FVector ProjectileSpawnOffset = FVector(60.0f, 0.0f, 55.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float ProjectileDamage = 10.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
	FName ProjectileHitEffectId = FName(TEXT("hit.red_burst"));

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Melee", meta = (DisplayName = "Melee Impact Niagara Effect"))
	TSoftObjectPtr<UNiagaraSystem> MeleeSwingEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Melee")
	TSoftClassPtr<ATunaSweeperMeleeImpactBurstActor> MeleeImpactBurstActorClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Melee")
	TSoftClassPtr<ATunaSweeperMeleeSwingTrailActor> MeleeSwingTrailActorClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float MaxHealth = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effects|Death")
	TSoftObjectPtr<UNiagaraSystem> DeathNiagaraEffect;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effects|Death",
		meta = (ClampMin = "0.01", UIMin = "0.01"))
	float DeathNiagaraScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effects|Death",
		meta = (ClampMin = "0.0", UIMin = "0.0"))
	float DeathRagdollDurationSeconds = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effects|Death",
		meta = (ClampMin = "0.0", UIMin = "0.0"))
	float DeathRagdollAngularSpeedDegrees = 0.0f;

	// TEMP_VIDEO_BULLET_STORM: BEGIN
	// One-off capture switch. Search for TEMP_VIDEO_BULLET_STORM to remove every related branch.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Debug|Temporary Video Capture",
		meta = (DisplayName = "Enable No-Damage Bullet Storm (Temporary)"))
	bool bEnableTemporaryVideoBulletStorm = false;
	// TEMP_VIDEO_BULLET_STORM: END

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Debuffs", meta = (ClampMin = "0", ClampMax = "10000"))
	int32 BleedingChanceBonus = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Debuffs", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float BleedingDurationBonusSeconds = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float MovementSpeed = 260.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Randomization")
	FVector2D MovementSpeedRandomOffset = FVector2D(-35.0f, 45.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Noise|Footstep", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float FootstepNoiseLoudness = 0.3f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Noise|Footstep", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float FootstepNoiseMaxRange = 2000.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Noise|Footstep", meta = (ClampMin = "0.05", UIMin = "0.05"))
	float FootstepNoiseIntervalSeconds = 0.42f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Noise|Footstep", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float FootstepNoiseMinSpeed = 70.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Noise|Footstep")
	FName FootstepNoiseTag = FName(TEXT("noise.enemy_footstep"));

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Noise|Footstep")
	FVector FootstepNoiseSourceOffset = FVector(0.0f, 0.0f, 42.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visual")
	TSoftObjectPtr<UMaterialInterface> BodyMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visual")
	TSoftObjectPtr<UMaterialInterface> ForwardMarkerMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loot")
	TSoftClassPtr<ATunaSweeperLootContainerActor> LootContainerClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
	FName EnemyId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
	int32 DropContainerDefinitionId = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
	int32 DropContentsId = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Experience", meta = (ClampMin = "0", UIMin = "0"))
	int32 ExperienceValue = 30;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Weapon")
	int32 EnemyWeaponItemId = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Weapon")
	int32 EnemyAmmoItemId = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Weapon")
	int32 EnemyReserveAmmoCount = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot|Weapon", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float LootLoadedAmmoDeductionRatio = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot|Weapon", meta = (ClampMin = "0", UIMin = "0"))
	int32 LootLoadedAmmoFlatDeduction = 0;

private:
	void ApplyVoxelVisualMeshes();
	void ApplyVisualMaterials();
	void UpdateAlertIndicatorFacing();
	void InitializeEnemyWeaponRuntime();
	bool EnsureEnemyWeaponActor();
	void CompleteEnemyReloadIfReady();
	void UpdateEnemyReloadWidget();
	void UpdateEnemyStatusSpeechBubble();
	void SetEnemyStatusSpeechBubble(ETunaSweeperEnemyStatusBubble InStatus, const FText& InText, float DisplaySeconds);
	void ClearEnemyStatusSpeechBubble(uint32 ExpectedRevision, ETunaSweeperEnemyStatusBubble ExpectedStatus);
	FText ResolveEnemyStatusText(FName TextKey, const FText& FallbackText) const;
	bool TryCreateEnemyWeaponLootInstance(
		class UTunaSweeperGameInstance* TunaGameInstance,
		class UTunaSweeperItemDataSubsystem* ItemDataSubsystem,
		FGuid& OutWeaponUid) const;
	bool TryBuildDeathLootRuntimeItemUids(
		FTunaSweeperLootContainerInstance& OutContainerInstance,
		TArray<FGuid>& OutRuntimeItemUids) const;
	int32 ResolveLootLoadedAmmoCount(int32& OutSourceLoadedAmmoCount, int32& OutDeductedLoadedAmmoCount) const;
	void HandleDeath(AController* KillerController, AActor* DamageCauser);
	bool TryStartDeathRagdoll();
	void FinalizeDeath();
	void SpawnDeathNiagaraEffect();
	FVector ResolveDeathVisualLocation() const;
	bool ApplyMeleeDamageTo(AActor* TargetActor);
	void ApplyMeleeKnockbackTo(AActor* TargetActor, const FVector& AttackDirection) const;
	void SpawnMeleeSwingEffect(const FVector& AttackDirection);
	void SpawnMeleeImpactBurst(const FVector& HitLocation, const FVector& BurstDirection);
	void SpawnDeathStrawberryBurst();
	bool SpawnDeathLootContainer(AActor* DamageCauser);
	FVector ResolveLootDropLocation(AActor* IgnoredActor) const;
	void TickFootstepNoise(float DeltaSeconds);

	UPROPERTY(Transient)
	TObjectPtr<ATunaSweeperWeapon> EnemyWeapon;

	FTunaSweeperEnemyCombatProfile CombatProfile;

	float CurrentHealth = 30.0f;
	int32 MeleeAttackSerial = 0;
	FName EnemyWeaponTypeTag = NAME_None;
	ETunaSweeperWeaponFireMode EnemyFireMode = ETunaSweeperWeaponFireMode::NotApplicable;
	FName EnemyImpactProfileId = NAME_None;
	FName EnemyProjectileHitEffectId = NAME_None;
	float EnemyProjectileDamageMultiplier = 1.0f;
	float EnemyReloadSeconds = 1.8f;
	int32 EnemyProjectileDamageBonus = 0;
	int32 EnemyMagazineCapacity = 0;
	int32 EnemyLoadedAmmoCount = 0;
	int32 PendingEnemyReloadAmmoCount = 0;
	TWeakObjectPtr<AActor> PendingDeathDamageCauser;
	FTimerHandle DeathFinalizeTimerHandle;
	bool bIsDead = false;
	bool bEnemyWeaponRuntimeInitialized = false;
	float FootstepNoiseElapsedSeconds = 0.0f;
	ETunaSweeperEnemyStatusBubble EnemyStatusBubble = ETunaSweeperEnemyStatusBubble::None;
	uint32 EnemyStatusBubbleRevision = 0;
	FTimerHandle EnemyStatusBubbleTimerHandle;
};
