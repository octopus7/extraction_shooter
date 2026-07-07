#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TunaSweeperBreakableAppleCrateActor.generated.h"

class ATunaSweeperPhysicsAppleActor;
class ATunaSweeperPhysicsCrateFragmentActor;
class UBoxComponent;
class UGeometryCollection;
class UGeometryCollectionComponent;
class UMaterialInterface;
class USceneComponent;
class UStaticMesh;
class UStaticMeshComponent;

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperBreakableAppleCrateActor : public AActor
{
	GENERATED_BODY()

public:
	ATunaSweeperBreakableAppleCrateActor();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual float TakeDamage(
		float DamageAmount,
		struct FDamageEvent const& DamageEvent,
		AController* EventInstigator,
		AActor* DamageCauser) override;

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Breakable Apple Crate")
	void BreakCrate();

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Breakable Apple Crate")
	void ConfigureBreakableAppleCrateDefaults(
		FName InCrateId,
		float InMaxHealth,
		const TSoftObjectPtr<UStaticMesh>& InCrateMesh,
		const TSoftObjectPtr<UGeometryCollection>& InCrateGeometryCollection,
		TSubclassOf<ATunaSweeperPhysicsAppleActor> InAppleActorClass,
		const TSoftObjectPtr<UStaticMesh>& InAppleMesh,
		TSubclassOf<ATunaSweeperPhysicsCrateFragmentActor> InCrateFragmentActorClass,
		const TSoftObjectPtr<UStaticMesh>& InCrateFragmentMesh);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Breakable Apple Crate")
	float GetHealth() const { return CurrentHealth; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Breakable Apple Crate")
	float GetMaxHealth() const { return MaxHealth; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Breakable Apple Crate")
	bool IsCrateBroken() const { return bCrateBroken; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> BlockingCollision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> CrateMeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UGeometryCollectionComponent> CrateGeometryCollectionComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apple Crate")
	FName CrateId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apple Crate", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float MaxHealth = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apple Crate|Visual")
	TSoftObjectPtr<UStaticMesh> CrateMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apple Crate|Visual")
	TSoftObjectPtr<UGeometryCollection> CrateGeometryCollection;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apple Crate|Collision", meta = (ClampMin = "1.0", UIMin = "1.0"))
	FVector CollisionExtent = FVector(48.0f, 48.0f, 42.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apple Crate|Collision")
	FVector CollisionCenterOffset = FVector(0.0f, 0.0f, 42.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apple Crate|Break")
	bool bHideCrateMeshOnBreak = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apple Crate|Break")
	bool bUseGeometryCollectionOnBreak = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apple Crate|Break")
	bool bSpawnCrateFragmentsOnBreak = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apple Crate|Break", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float GeometryCollectionBreakRadius = 95.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apple Crate|Break", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float GeometryCollectionRadialImpulse = 120.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apple Crate|Break", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float GeometryCollectionDirectionalImpulse = 2400.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apple Crate|Break", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float GeometryCollectionUpwardImpulse = 350.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apple Crate|Break", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float GeometryCollectionDamageThreshold = 25.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apple Crate|Apple Spawn")
	TSubclassOf<ATunaSweeperPhysicsAppleActor> AppleActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apple Crate|Apple Spawn")
	TSoftObjectPtr<UStaticMesh> AppleMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apple Crate|Apple Spawn", meta = (ClampMin = "0", UIMin = "0"))
	int32 MinAppleCount = 5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apple Crate|Apple Spawn", meta = (ClampMin = "0", UIMin = "0"))
	int32 MaxAppleCount = 8;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apple Crate|Apple Spawn")
	FVector AppleSpawnCenter = FVector(0.0f, 0.0f, 36.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apple Crate|Apple Spawn", meta = (ClampMin = "0.0", UIMin = "0.0"))
	FVector AppleSpawnExtent = FVector(30.0f, 30.0f, 18.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apple Crate|Apple Spawn", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float AppleCollisionRadiusCm = 14.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apple Crate|Apple Spawn", meta = (ClampMin = "0.01", UIMin = "0.01"))
	float AppleVisualScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apple Crate|Apple Spawn", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float AppleLifeSeconds = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apple Crate|Apple Launch", meta = (ClampMin = "0.0", UIMin = "0.0"))
	FVector2D HorizontalSpeedRange = FVector2D(180.0f, 420.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apple Crate|Apple Launch", meta = (ClampMin = "0.0", UIMin = "0.0"))
	FVector2D VerticalSpeedRange = FVector2D(80.0f, 180.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apple Crate|Apple Launch", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float RandomScatterWeight = 0.55f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apple Crate|Apple Launch", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float AngularSpeedDegrees = 780.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apple Crate|Fragment Spawn")
	TSubclassOf<ATunaSweeperPhysicsCrateFragmentActor> CrateFragmentActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apple Crate|Fragment Spawn")
	TSoftObjectPtr<UStaticMesh> CrateFragmentMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apple Crate|Fragment Spawn", meta = (ClampMin = "0", UIMin = "0"))
	int32 MinCrateFragmentCount = 8;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apple Crate|Fragment Spawn", meta = (ClampMin = "0", UIMin = "0"))
	int32 MaxCrateFragmentCount = 14;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apple Crate|Fragment Spawn", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float CrateFragmentLifeSeconds = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apple Crate|Fragment Shape", meta = (ClampMin = "0.5", UIMin = "0.5"))
	FVector2D FragmentThicknessRange = FVector2D(3.5f, 8.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apple Crate|Fragment Shape", meta = (ClampMin = "0.5", UIMin = "0.5"))
	FVector2D FragmentWidthRange = FVector2D(10.0f, 26.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apple Crate|Fragment Shape", meta = (ClampMin = "0.5", UIMin = "0.5"))
	FVector2D FragmentLengthRange = FVector2D(32.0f, 78.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apple Crate|Fragment Launch", meta = (ClampMin = "0.0", UIMin = "0.0"))
	FVector2D FragmentHorizontalSpeedRange = FVector2D(220.0f, 560.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apple Crate|Fragment Launch", meta = (ClampMin = "0.0", UIMin = "0.0"))
	FVector2D FragmentVerticalSpeedRange = FVector2D(140.0f, 320.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apple Crate|Fragment Launch", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float FragmentRandomScatterWeight = 0.9f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apple Crate|Fragment Launch", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float FragmentAngularSpeedDegrees = 1040.0f;

private:
	void ApplyCrateDefaults();
	void BreakCrateFromDirection(const FVector& SpillDirection);
	bool BreakGeometryCollection(const FVector& SpillDirection);
	FVector ResolveSpillDirection(FDamageEvent const& DamageEvent, AActor* DamageCauser) const;
	FVector GetCrateCenterWorldLocation() const;
	void SpawnApples(const FVector& SpillDirection);
	void SpawnCrateFragments(const FVector& SpillDirection);
	FVector BuildRandomCrateFragmentSpawnLocation() const;
	FVector BuildRandomCrateFragmentHalfExtent() const;
	FVector BuildRandomCrateFragmentVelocity(const FVector& SpillDirection) const;
	FVector BuildRandomCrateFragmentAngularVelocity() const;
	UMaterialInterface* ResolveCrateFragmentMaterial() const;
	FVector BuildRandomAppleSpawnLocation() const;
	FVector BuildRandomAppleVelocity(const FVector& SpillDirection) const;
	FVector BuildRandomAppleAngularVelocity() const;

	UPROPERTY(Transient)
	float CurrentHealth = 1.0f;

	UPROPERTY(Transient)
	bool bCrateBroken = false;
};
