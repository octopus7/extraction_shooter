#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TunaSweeperSandbagCoverActor.generated.h"

class APawn;
class UBoxComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UProceduralMeshComponent;
class USceneComponent;

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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> BlockingCollision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UProceduralMeshComponent> VisualMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UProceduralMeshComponent> OutlineMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sandbag Cover")
	FName CoverId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sandbag Cover", meta = (ClampMin = "1.0", UIMin = "1.0"))
	FVector BoxExtent = FVector(75.0f, 320.0f, 90.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sandbag Cover", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float MaxHealth = 70.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sandbag Cover|Passthrough", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float PassthroughRadius = 125.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sandbag Cover|Passthrough", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float PassthroughVerticalTolerance = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sandbag Cover|Visual", meta = (ClampMin = "0.5", UIMin = "0.5"))
	float OutlineThickness = 6.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sandbag Cover|Visual")
	TSoftObjectPtr<UMaterialInterface> VisualMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sandbag Cover|Visual")
	TSoftObjectPtr<UMaterialInterface> OutlineMaterial;

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
};
