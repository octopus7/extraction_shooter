#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TunaSweeperBoilingPotActor.generated.h"

class UAudioComponent;
class UBoxComponent;
class UNiagaraComponent;
class UNiagaraSystem;
class USceneComponent;
class USoundBase;
class UStaticMesh;
class UStaticMeshComponent;

/** A stylized two-mesh cooking pot with deterministic lid rattles and attached steam. */
UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperBoilingPotActor : public AActor
{
	GENERATED_BODY()

public:
	ATunaSweeperBoilingPotActor();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Boiling Pot")
	void SetBoiling(bool bEnabled);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Boiling Pot")
	bool IsBoiling() const { return bBoiling; }

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Boiling Pot")
	void ConfigurePresentationDefaults(
		TSoftObjectPtr<UStaticMesh> InPotBodyMesh,
		TSoftObjectPtr<UStaticMesh> InLidMesh,
		TSoftObjectPtr<UNiagaraSystem> InSteamSystem,
		TSoftObjectPtr<USoundBase> InLidClatterSound);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> BlockingCollision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> PotBodyMeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> LidPivot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> LidMeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SteamOrigin;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UNiagaraComponent> SteamEffectComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UAudioComponent> LidClatterAudioComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boiling Pot|Presentation")
	TSoftObjectPtr<UStaticMesh> PotBodyMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boiling Pot|Presentation")
	TSoftObjectPtr<UStaticMesh> LidMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boiling Pot|Presentation")
	TSoftObjectPtr<UNiagaraSystem> SteamSystem;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boiling Pot|Presentation")
	TSoftObjectPtr<USoundBase> LidClatterSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boiling Pot|State")
	bool bStartBoiling = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boiling Pot|Rattle", meta = (ClampMin = "0.05", UIMin = "0.05"))
	float MinPauseSeconds = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boiling Pot|Rattle", meta = (ClampMin = "0.05", UIMin = "0.05"))
	float MaxPauseSeconds = 0.9f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boiling Pot|Rattle", meta = (ClampMin = "0.05", UIMin = "0.05"))
	float MinBurstDurationSeconds = 0.24f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boiling Pot|Rattle", meta = (ClampMin = "0.05", UIMin = "0.05"))
	float MaxBurstDurationSeconds = 0.42f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boiling Pot|Rattle", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float MinLidLiftCm = 0.7f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boiling Pot|Rattle", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float MaxLidLiftCm = 2.6f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boiling Pot|Rattle", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float MinTiltDegrees = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boiling Pot|Rattle", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float MaxTiltDegrees = 5.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boiling Pot|Rattle", meta = (ClampMin = "1", ClampMax = "8"))
	int32 MinHitsPerBurst = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boiling Pot|Rattle", meta = (ClampMin = "1", ClampMax = "8"))
	int32 MaxHitsPerBurst = 4;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boiling Pot|Rattle")
	int32 RattleSeed = 1337;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boiling Pot|Layout", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float LidRestHeightCm = 53.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boiling Pot|Layout", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float SteamHeightCm = 59.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boiling Pot|Layout", meta = (ClampMin = "0.01", UIMin = "0.01"))
	float SteamVisualScale = 0.18f;

private:
	void ApplyPresentation();
	void StartRattleBurst();
	void FinishRattleBurst();
	void ResetLidTransform();
	void PlayLidClatter();
	float GetRandomRange(float Minimum, float Maximum);

	FTransform LidRestTransform = FTransform::Identity;
	FRandomStream RattleRandom;
	FVector2D BurstTiltAxis = FVector2D(1.0f, 0.0f);
	float TimeUntilNextBurst = 0.0f;
	float BurstElapsedSeconds = 0.0f;
	float BurstDurationSeconds = 0.3f;
	float BurstLiftCm = 1.0f;
	float BurstTiltDegrees = 3.0f;
	int32 BurstHitCount = 3;
	int32 LastPlayedHitIndex = INDEX_NONE;
	bool bRattleBurstActive = false;
	bool bBoiling = false;
};
