#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TunaSweeperPhysicsCrateFragmentActor.generated.h"

class UBoxComponent;
class UMaterialInterface;
class UStaticMesh;
class UStaticMeshComponent;

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperPhysicsCrateFragmentActor : public AActor
{
	GENERATED_BODY()

public:
	ATunaSweeperPhysicsCrateFragmentActor();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Physics Crate Fragment")
	void ConfigureCrateFragmentDefaults(
		const TSoftObjectPtr<UStaticMesh>& InFragmentMesh,
		UMaterialInterface* InFragmentMaterial,
		const FVector& InHalfExtentCm,
		float InLifeSeconds);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Physics Crate Fragment")
	void LaunchFragment(const FVector& LinearVelocityCmPerSecond, const FVector& AngularVelocityDegreesPerSecond);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> CollisionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> FragmentMeshComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics Crate Fragment|Visual")
	TSoftObjectPtr<UStaticMesh> FragmentMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics Crate Fragment|Shape", meta = (ClampMin = "0.5", UIMin = "0.5"))
	FVector HalfExtentCm = FVector(6.0f, 18.0f, 44.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics Crate Fragment|Physics", meta = (ClampMin = "0.01", UIMin = "0.01"))
	float MassKg = 0.28f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics Crate Fragment|Physics", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float LinearDamping = 0.22f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics Crate Fragment|Physics", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float AngularDamping = 0.42f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics Crate Fragment|Lifetime", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float LifeSeconds = 8.0f;

private:
	void ApplyFragmentDefaults();

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> FragmentMaterial;
};
