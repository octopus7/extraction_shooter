#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TunaSweeperBreakableConcreteWall.generated.h"

class UBoxComponent;
class UStaticMeshComponent;

UCLASS(Blueprintable)
class TUNASWEEPER_API ATunaSweeperBreakableConcreteWall : public AActor
{
	GENERATED_BODY()

public:
	ATunaSweeperBreakableConcreteWall();

	virtual float TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent,
		AController* EventInstigator, AActor* DamageCauser) override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wall")
	TObjectPtr<UBoxComponent> WallCollision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wall")
	TObjectPtr<UStaticMeshComponent> WallMesh;
};
