#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TunaSweeperPeriodicNoiseEmitterActor.generated.h"

class UMaterialInstanceDynamic;
class UProceduralMeshComponent;
class USceneComponent;

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperPeriodicNoiseEmitterActor : public AActor
{
	GENERATED_BODY()

public:
	ATunaSweeperPeriodicNoiseEmitterActor();

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Noise")
	void ConfigureNoiseEmitterDefaults(
		FName InMeshDefinitionId,
		const FString& InMeshDefinitionJsonRelativePath,
		float InNoiseIntervalSeconds,
		float InNoiseLoudness,
		float InNoiseMaxRange,
		FName InNoiseTag,
		const FVector& InNoiseSourceLocalOffset,
		bool bInStartEnabled);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Noise")
	void EmitNoise();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UProceduralMeshComponent> ProceduralMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TunaSweeper|Noise")
	FName MeshDefinitionId = FName(TEXT("mesh.test_noise_quad_horn"));

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TunaSweeper|Noise")
	FString MeshDefinitionJsonRelativePath = TEXT("Data/PeriodicNoiseEmitterMeshes.json");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TunaSweeper|Noise", meta = (ClampMin = "0.05", UIMin = "0.05"))
	float NoiseIntervalSeconds = 2.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TunaSweeper|Noise", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float NoiseLoudness = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TunaSweeper|Noise", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float NoiseMaxRange = 2600.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TunaSweeper|Noise")
	FName NoiseTag = FName(TEXT("noise.test_periodic"));

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TunaSweeper|Noise")
	FVector NoiseSourceLocalOffset = FVector(0.0f, 0.0f, 160.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TunaSweeper|Noise")
	bool bStartEnabled = true;

private:
	void RebuildProceduralMesh();
	FString ResolveMeshDefinitionJsonPath() const;
	void StartNoiseTimer();
	void StopNoiseTimer();

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> DynamicMaterials;

	FTimerHandle NoiseTimerHandle;
};
