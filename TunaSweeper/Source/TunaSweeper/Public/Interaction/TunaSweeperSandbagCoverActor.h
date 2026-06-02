#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TunaSweeperSandbagCoverActor.generated.h"

class APawn;
class UBoxComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class USceneComponent;
class UStaticMesh;
class UStaticMeshComponent;

struct FTunaSweeperSandbagCollapseState
{
	FVector StartLocation = FVector::ZeroVector;
	FVector TargetLocation = FVector::ZeroVector;
	FVector BurstOffset = FVector::ZeroVector;
	FRotator StartRotation = FRotator::ZeroRotator;
	FRotator TargetRotation = FRotator::ZeroRotator;
	FRotator BurstRotation = FRotator::ZeroRotator;
	float BurstLift = 0.0f;
	float DelaySeconds = 0.0f;
};

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperSandbagCoverActor : public AActor
{
	GENERATED_BODY()

public:
	ATunaSweeperSandbagCoverActor();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual float TakeDamage(
		float DamageAmount,
		struct FDamageEvent const& DamageEvent,
		AController* EventInstigator,
		AActor* DamageCauser) override;

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Sandbag Cover")
	void ConfigureCoverDefaults(
		FName InCoverId,
		const FVector& InBoxExtent,
		float InMaxHealth,
		float InPassthroughRadius);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Sandbag Cover")
	void ConfigureCoverVisualDefaults(
		TSoftObjectPtr<UMaterialInterface> InVisualMaterial,
		TSoftObjectPtr<UMaterialInterface> InOutlineMaterial);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Sandbag Cover")
	void ConfigureCoverMeshDefaults(TSoftObjectPtr<UStaticMesh> InSandbagMesh);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Sandbag Cover")
	FName GetCoverId() const { return CoverId; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Sandbag Cover")
	float GetHealth() const { return CurrentHealth; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Sandbag Cover")
	float GetMaxHealth() const { return MaxHealth; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Sandbag Cover")
	bool ShouldAllowPlayerProjectilePassthrough(APawn* InstigatorPawn) const;

protected:
	void ApplyCollisionDefaults();
	void RebuildMeshes();
	void ApplyMaterials();
	void UpdateDamageVisual();
	void UpdatePassthroughOutline();
	void SetOutlineActive(bool bEnabled);
	void DestroyCover();
	void BeginCollapse();
	void UpdateCollapse(float DeltaSeconds);
	void ResetCollapseState();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> BlockingCollision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TArray<TObjectPtr<UStaticMeshComponent>> SandbagMeshComponents;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sandbag Cover")
	FName CoverId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sandbag Cover", meta = (ClampMin = "1.0", UIMin = "1.0"))
	FVector BoxExtent = FVector(37.5f, 160.0f, 45.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sandbag Cover", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float MaxHealth = 70.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sandbag Cover|Passthrough", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float PassthroughRadius = 62.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sandbag Cover|Passthrough", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float PassthroughVerticalTolerance = 45.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sandbag Cover|Visual", meta = (ClampMin = "0.5", UIMin = "0.5"))
	float OutlineThickness = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sandbag Cover|Visual")
	TSoftObjectPtr<UMaterialInterface> VisualMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sandbag Cover|Visual")
	TSoftObjectPtr<UMaterialInterface> OutlineMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sandbag Cover|Visual")
	TSoftObjectPtr<UStaticMesh> SandbagStaticMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sandbag Cover|Collapse", meta = (ClampMin = "0.05", UIMin = "0.05"))
	float CollapseDurationSeconds = 1.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sandbag Cover|Collapse", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float CollapseHoldSeconds = 0.85f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sandbag Cover|Collapse", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float CollapseScatterDistance = 67.5f;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> DynamicVisualMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> DynamicOutlineMaterial;

	UPROPERTY(Transient)
	float CurrentHealth = 70.0f;

	UPROPERTY(Transient)
	bool bCoverDestroyed = false;

	UPROPERTY(Transient)
	bool bOutlineActive = false;

	UPROPERTY(Transient)
	float CollapseElapsedSeconds = 0.0f;

	TArray<FTunaSweeperSandbagCollapseState> CollapseStates;
};
