#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TunaSweeperBreakableAppleCrateActor.generated.h"

class ATunaSweeperPhysicsAppleActor;
class UBoxComponent;
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
		TSubclassOf<ATunaSweeperPhysicsAppleActor> InAppleActorClass,
		const TSoftObjectPtr<UStaticMesh>& InAppleMesh);

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apple Crate")
	FName CrateId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apple Crate", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float MaxHealth = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apple Crate|Visual")
	TSoftObjectPtr<UStaticMesh> CrateMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apple Crate|Collision", meta = (ClampMin = "1.0", UIMin = "1.0"))
	FVector CollisionExtent = FVector(48.0f, 48.0f, 42.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apple Crate|Collision")
	FVector CollisionCenterOffset = FVector(0.0f, 0.0f, 42.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apple Crate|Break")
	bool bHideCrateMeshOnBreak = true;

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

private:
	void ApplyCrateDefaults();
	void BreakCrateFromDirection(const FVector& SpillDirection);
	FVector ResolveSpillDirection(FDamageEvent const& DamageEvent, AActor* DamageCauser) const;
	void SpawnApples(const FVector& SpillDirection);
	FVector BuildRandomAppleSpawnLocation() const;
	FVector BuildRandomAppleVelocity(const FVector& SpillDirection) const;
	FVector BuildRandomAppleAngularVelocity() const;

	UPROPERTY(Transient)
	float CurrentHealth = 1.0f;

	UPROPERTY(Transient)
	bool bCrateBroken = false;
};
