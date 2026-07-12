#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TunaSweeperShellCasing.generated.h"

class UBoxComponent;
class UMaterialInterface;
class UStaticMesh;
class UStaticMeshComponent;

/** A short-lived, physics-simulated shell casing ejected by a fired weapon. */
UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperShellCasing : public AActor
{
	GENERATED_BODY()

public:
	ATunaSweeperShellCasing();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	void LaunchCasing(const FVector& LinearVelocityCmPerSecond, const FVector& AngularVelocityDegreesPerSecond);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> CollisionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> CasingMeshComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shell Casing|Visual")
	TSoftObjectPtr<UStaticMesh> CasingMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shell Casing|Visual")
	TSoftObjectPtr<UMaterialInterface> CasingMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shell Casing|Collision", meta = (ClampMin = "0.1", UIMin = "0.1"))
	FVector CollisionHalfExtentCm = FVector(1.8f, 0.72f, 0.72f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shell Casing|Physics", meta = (ClampMin = "0.01", UIMin = "0.01"))
	float MassKg = 0.012f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shell Casing|Physics", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float LinearDamping = 0.45f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shell Casing|Physics", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float AngularDamping = 0.55f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shell Casing|Lifetime", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float LifeSeconds = 5.0f;

private:
	void ApplyCasingDefaults();
};
