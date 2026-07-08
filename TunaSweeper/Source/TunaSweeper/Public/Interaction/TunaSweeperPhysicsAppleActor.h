#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TunaSweeperPhysicsAppleActor.generated.h"

class USphereComponent;
class UStaticMesh;
class UStaticMeshComponent;

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperPhysicsAppleActor : public AActor
{
	GENERATED_BODY()

public:
	ATunaSweeperPhysicsAppleActor();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Physics Apple")
	void ConfigurePhysicsAppleDefaults(
		const TSoftObjectPtr<UStaticMesh>& InAppleMesh,
		float InCollisionRadiusCm,
		float InVisualScale,
		float InLifeSeconds);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Physics Apple")
	void LaunchApple(const FVector& LinearVelocityCmPerSecond, const FVector& AngularVelocityDegreesPerSecond);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> CollisionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> AppleMeshComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics Apple|Visual")
	TSoftObjectPtr<UStaticMesh> AppleMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics Apple|Collision", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float CollisionRadiusCm = 14.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics Apple|Collision")
	bool bUseMeshBoundsForCollisionRadius = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics Apple|Visual")
	bool bCenterMeshOnCollision = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics Apple|Visual", meta = (ClampMin = "0.01", UIMin = "0.01"))
	float VisualScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics Apple|Physics", meta = (ClampMin = "0.01", UIMin = "0.01"))
	float MassKg = 0.18f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics Apple|Physics", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float LinearDamping = 0.18f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics Apple|Physics", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float AngularDamping = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics Apple|Lifetime", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float LifeSeconds = 12.0f;

private:
	void ApplyAppleDefaults();
};
