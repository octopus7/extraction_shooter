#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TunaSweeperCookableChickenActor.generated.h"

class UBoxComponent;
class USceneComponent;
class UStaticMesh;
class UStaticMeshComponent;

/** A prop chicken that is cooked once when it receives damage from an explosive barrel. */
UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperCookableChickenActor : public AActor
{
	GENERATED_BODY()

public:
	ATunaSweeperCookableChickenActor();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual float TakeDamage(
		float DamageAmount,
		struct FDamageEvent const& DamageEvent,
		AController* EventInstigator,
		AActor* DamageCauser) override;

	/** Changes this chicken to its cooked mesh. Safe to call from Blueprint for scripted gags. */
	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Cookable Chicken")
	void CookChicken();

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Cookable Chicken")
	void ResetToRawChicken();

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Cookable Chicken")
	bool IsCooked() const { return bCooked; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> BlockingCollision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> ChickenMeshComponent;

	/** Mesh shown before an explosive-barrel blast cooks this actor. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cookable Chicken|Meshes")
	TSoftObjectPtr<UStaticMesh> RawChickenMesh;

	/** Mesh shown after an explosive-barrel blast cooks this actor. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cookable Chicken|Meshes")
	TSoftObjectPtr<UStaticMesh> CookedChickenMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cookable Chicken|Collision", meta = (ClampMin = "1.0", UIMin = "1.0"))
	FVector CollisionExtent = FVector(36.0f, 28.0f, 24.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cookable Chicken|Collision")
	FVector CollisionCenterOffset = FVector(0.0f, 0.0f, 24.0f);

	/** Read-only runtime state. The actor only cooks once until ResetToRawChicken is called. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Cookable Chicken")
	bool bCooked = false;

private:
	void ApplyActorDefaults();
	void ApplyCurrentMesh();
};
