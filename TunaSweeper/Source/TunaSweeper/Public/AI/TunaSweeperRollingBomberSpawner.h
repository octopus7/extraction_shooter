#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TunaSweeperRollingBomberSpawner.generated.h"

class ATunaSweeperRollingBomber;
class UMaterialInterface;
class UProceduralMeshComponent;
class USceneComponent;
class USoundBase;
class USoundWaveProcedural;

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperRollingBomberSpawner : public AActor
{
	GENERATED_BODY()

public:
	ATunaSweeperRollingBomberSpawner();

	virtual float TakeDamage(
		float DamageAmount,
		struct FDamageEvent const& DamageEvent,
		AController* EventInstigator,
		AActor* DamageCauser) override;

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Rolling Bomber Spawner")
	void ConfigureSpawnerDefaults(
		const TSoftClassPtr<ATunaSweeperRollingBomber>& InRollingBomberClass,
		const TSoftObjectPtr<USoundBase>& InLaunchSound,
		int32 InInitialSpawnCount,
		int32 InMaxSpawnCount,
		float InWaveIntervalSeconds,
		float InSpawnIntervalSeconds,
		float InLaunchSpeedMin,
		float InLaunchSpeedMax,
		float InLaunchPitchMinDegrees,
		float InLaunchPitchMaxDegrees,
		float InMaxHealth);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UProceduralMeshComponent> PillarMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UProceduralMeshComponent> HexHeadMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> LaunchPoint;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber Spawner|Spawn")
	TSoftClassPtr<ATunaSweeperRollingBomber> RollingBomberClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber Spawner|Audio")
	TSoftObjectPtr<USoundBase> LaunchSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber Spawner|Visual")
	TSoftObjectPtr<UMaterialInterface> SpawnerMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber Spawner|Spawn", meta = (ClampMin = "1", UIMin = "1"))
	int32 InitialSpawnCount = 2;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber Spawner|Spawn", meta = (ClampMin = "1", UIMin = "1"))
	int32 MaxSpawnCount = 8;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber Spawner|Spawn", meta = (ClampMin = "0.01", UIMin = "0.01"))
	float WaveIntervalSeconds = 10.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber Spawner|Spawn", meta = (ClampMin = "0.01", UIMin = "0.01"))
	float SpawnIntervalSeconds = 0.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber Spawner|Launch", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float LaunchSpeedMin = 850.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber Spawner|Launch", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float LaunchSpeedMax = 1100.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber Spawner|Launch", meta = (ClampMin = "0.0", ClampMax = "89.0", UIMin = "0.0", UIMax = "89.0"))
	float LaunchPitchMinDegrees = 38.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber Spawner|Launch", meta = (ClampMin = "0.0", ClampMax = "89.0", UIMin = "0.0", UIMax = "89.0"))
	float LaunchPitchMaxDegrees = 58.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber Spawner|Health", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float MaxHealth = 80.0f;

private:
	void StartWave();
	void SpawnNextQueuedRollingBomber();
	void StopSpawning();
	void DestroySpawner();
	void BuildSpawnerMeshes();
	void BuildPillarMesh();
	void BuildHexHeadMesh();
	void ApplySpawnerMaterial();
	FLinearColor ResolveMechanicalBandColor(float U, float V, float HeightAlpha, float FaceIndex) const;
	FVector BuildLaunchVelocity(float& OutYawDegrees) const;
	void PlayLaunchSound();
	void PlayProceduralLaunchSound();
	USoundWaveProcedural* CreateProceduralLaunchSound();

	FTimerHandle WaveTimerHandle;
	FTimerHandle BurstTimerHandle;

	UPROPERTY(Transient)
	TArray<TObjectPtr<USoundWaveProcedural>> ActiveProceduralLaunchSounds;

	int32 CurrentWaveSpawnCount = 2;
	int32 PendingSpawnCount = 0;
	float CurrentHealth = 80.0f;
	bool bSpawnerDestroyed = false;
};
