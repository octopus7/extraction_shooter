#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TunaSweeperMeleeSwingTrailActor.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperMeleeSwingTrailActor : public AActor
{
	GENERATED_BODY()

public:
	ATunaSweeperMeleeSwingTrailActor();

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> SwingArcMesh;

private:
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> SwingArcMaterial;

	FVector BaseArcScale = FVector::OneVector;
	float ElapsedLifetimeSeconds = 0.0f;
};
