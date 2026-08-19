#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "TunaSweeperScratchComponent.generated.h"

class UMaterialInstanceDynamic;
class UMaterialInterface;
class UMeshComponent;

UENUM(BlueprintType)
enum class ETunaSweeperNearMissAttackType : uint8
{
	Projectile,
	Melee
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FTunaSweeperScratchChangedSignature,
	float,
	CurrentScratch,
	float,
	MaxScratch);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(
	FTunaSweeperNearMissSignature,
	AActor*,
	AttackSource,
	ETunaSweeperNearMissAttackType,
	AttackType,
	bool,
	bPerfectDodge,
	float,
	ClearanceCm);

UCLASS(ClassGroup = (TunaSweeper), meta = (BlueprintSpawnableComponent))
class TUNASWEEPER_API UTunaSweeperScratchComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTunaSweeperScratchComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Scratch")
	float GetCurrentScratch() const { return CurrentScratch; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Scratch")
	float GetMaxScratch() const { return FMath::Max(1.0f, MaxScratch); }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Scratch")
	float GetScratchPercent() const { return FMath::Clamp(CurrentScratch / GetMaxScratch(), 0.0f, 1.0f); }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Scratch")
	float GetProjectileNearMissMarginCm() const { return FMath::Max(0.0f, ProjectileNearMissMarginCm); }

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Scratch")
	bool TryConsumeScratch(float Amount);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Scratch")
	void ResetScratch();

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Scratch|Development")
	void SetDeveloperAlwaysSlowPresentationEnabled(bool bEnabled);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Scratch|Development")
	bool IsDeveloperAlwaysSlowPresentationEnabled() const { return bDeveloperAlwaysSlowPresentationEnabled; }

	bool TryRegisterNearMiss(
		AActor* AttackSource,
		int32 AttackId,
		ETunaSweeperNearMissAttackType AttackType,
		float ClearanceCm,
		bool bWouldHaveHit);

	UPROPERTY(BlueprintAssignable, Category = "TunaSweeper|Scratch")
	FTunaSweeperScratchChangedSignature OnScratchChanged;

	UPROPERTY(BlueprintAssignable, Category = "TunaSweeper|Scratch")
	FTunaSweeperNearMissSignature OnNearMiss;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Scratch", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float MaxScratch = 100.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Scratch", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float ProjectileScratchGain = 8.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Scratch", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float MeleeScratchGain = 12.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Scratch", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float PerfectDodgeGainMultiplier = 1.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Near Miss", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float ProjectileNearMissMarginCm = 40.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation", meta = (ClampMin = "0.01", ClampMax = "1.0", UIMin = "0.01", UIMax = "1.0"))
	float WorldSlowMotionScale = 0.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation", meta = (ClampMin = "0.01", ClampMax = "2.0", UIMin = "0.01", UIMax = "2.0"))
	float PlayerEffectiveTimeScale = 0.9f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float SlowMotionBlendInRealSeconds = 0.03f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float SlowMotionHoldRealSeconds = 0.06f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation", meta = (ClampMin = "0.01", UIMin = "0.01"))
	float SlowMotionBlendOutRealSeconds = 0.12f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float PresentationCooldownRealSeconds = 0.25f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation")
	TSoftObjectPtr<UMaterialInterface> ScratchOverlayMaterial;

private:
	void SetCurrentScratch(float NewValue);
	void TriggerPresentation(double RealTimeSeconds);
	void UpdatePresentation(double RealTimeSeconds);
	void RestorePresentation();
	void ApplyOverlayToCharacterMeshes();
	void RestoreCharacterMeshOverlays();
	void UpdateOverlayMaterial(float EffectAlpha, double RealTimeSeconds);

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> ScratchOverlayMaterialInstance;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMeshComponent>> ActiveOverlayMeshes;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInterface>> SavedOverlayMaterials;

	TMap<TWeakObjectPtr<AActor>, int32> LastProcessedAttackIds;

	float CurrentScratch = 0.0f;
	float SavedWorldTimeDilation = 1.0f;
	float SavedOwnerCustomTimeDilation = 1.0f;
	double PresentationStartRealSeconds = 0.0;
	double PresentationReleaseRealSeconds = 0.0;
	double LastPresentationRealSeconds = -1000.0;
	bool bPresentationActive = false;
	bool bDeveloperAlwaysSlowPresentationEnabled = false;
};
