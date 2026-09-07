#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TunaSweeperMissileTurret.generated.h"

class ATunaSweeperAttackTelegraph;
class UBoxComponent;
class UDamageType;
class UMaterialInterface;
class UStaticMesh;
class USceneComponent;
class UStaticMeshComponent;
class UTunaSweeperFactionComponent;
class UTunaSweeperVisionSubjectComponent;

/** A destructible summon usable by any faction actor; each missile commits to one ground position. */
UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperMissileTurret : public AActor
{
	GENERATED_BODY()

public:
	ATunaSweeperMissileTurret();
	virtual void Tick(float DeltaSeconds) override;
	virtual float TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent,
		AController* EventInstigator, AActor* DamageCauser) override;

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Combat Pattern|Turret")
	void InitializeTurret(AActor* Source, AActor* Target);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Combat Pattern|Turret")
	bool IsWarningActive() const { return bWarningActive; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Combat Pattern|Turret")
	float GetWarningProgress() const;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Combat Pattern|Turret")
	FVector GetLockedImpactLocation() const { return LockedImpactLocation; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Combat Pattern|Turret")
	float GetCurrentHealth() const { return CurrentHealth; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Combat Pattern|Turret")
	bool IsDead() const { return bDead; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Combat Pattern|Turret", meta = (ClampMin = "1.0"))
	float MaxHealth = 45.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Combat Pattern|Turret", meta = (ClampMin = "0.0"))
	float InitialFireDelay = 0.65f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Combat Pattern|Turret", meta = (ClampMin = "0.1"))
	float WarningDuration = 1.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Combat Pattern|Turret", meta = (ClampMin = "0.1"))
	float SalvoInterval = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Combat Pattern|Turret", meta = (ClampMin = "1.0"))
	float Lifetime = 18.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Combat Pattern|Turret", meta = (ClampMin = "1.0"))
	float TargetRange = 2200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Combat Pattern|Turret", meta = (ClampMin = "1.0"))
	float ImpactRadius = 180.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Combat Pattern|Turret", meta = (ClampMin = "0.0"))
	float ImpactDamage = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Combat Pattern|Turret", meta = (ClampMin = "1.0"))
	float ImpactHeight = 180.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Combat Pattern|Turret", meta = (ClampMin = "0.01"))
	float MissileDescentDuration = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Combat Pattern|Turret", meta = (ClampMin = "1.0"))
	float MissileDropHeight = 850.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Combat Pattern|Turret")
	TSubclassOf<ATunaSweeperAttackTelegraph> TelegraphClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Combat Pattern|Turret")
	TSubclassOf<UDamageType> ImpactDamageType;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> Hurtbox;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> BaseMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> LauncherMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> LeftLaunchTube;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> RightLaunchTube;

	// The falling visual belongs to the warning so turret vision culling cannot hide it.
	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> MissileMesh;

	UPROPERTY(EditDefaultsOnly, Category = "TunaSweeper|Combat Pattern|Turret")
	TObjectPtr<UStaticMesh> MissileVisualAsset;

	UPROPERTY(EditDefaultsOnly, Category = "TunaSweeper|Combat Pattern|Turret")
	TObjectPtr<UMaterialInterface> MissileMaterial;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UTunaSweeperFactionComponent> FactionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UTunaSweeperVisionSubjectComponent> VisionSubjectComponent;

private:
	bool IsSourceActive() const;
	bool IsValidTarget(AActor* Candidate) const;
	AActor* ResolveTarget() const;
	bool BeginWarning();
	void Impact();
	void ClearWarning();
	void SpawnExplosion(const FVector& Location, float Radius) const;

	TWeakObjectPtr<AActor> SourceActor;
	TWeakObjectPtr<AActor> TargetActor;

	UPROPERTY(Transient)
	TObjectPtr<ATunaSweeperAttackTelegraph> ActiveWarning;

	FVector LockedImpactLocation = FVector::ZeroVector;
	float CurrentHealth = 45.0f;
	float Age = 0.0f;
	float TimeToNextWarning = 0.0f;
	float WarningElapsed = 0.0f;
	float ActiveWarningDuration = 1.8f;
	float ActiveImpactRadius = 180.0f;
	bool bInitialized = false;
	bool bWarningActive = false;
	bool bDead = false;
};
