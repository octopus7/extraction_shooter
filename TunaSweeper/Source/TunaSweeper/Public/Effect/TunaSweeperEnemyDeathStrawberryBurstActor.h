#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TunaSweeperEnemyDeathStrawberryBurstActor.generated.h"

class UBillboardComponent;
class UTexture2D;

/** A short-lived, gravity-driven burst of pixel-art strawberry sprites used for enemy deaths. */
UCLASS()
class TUNASWEEPER_API ATunaSweeperEnemyDeathStrawberryBurstActor : public AActor
{
	GENERATED_BODY()

public:
	ATunaSweeperEnemyDeathStrawberryBurstActor();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	void CreateBurstParticles();
	bool TryResolveGroundHeight(float& OutGroundHeight) const;
	void UpdateParticle(int32 ParticleIndex, float DeltaSeconds);

	UPROPERTY(EditDefaultsOnly, Category = "TunaSweeper|Enemy Death|Strawberry Burst")
	TSoftObjectPtr<UTexture2D> StrawberryTexture;

	UPROPERTY(EditDefaultsOnly, Category = "TunaSweeper|Enemy Death|Strawberry Burst", meta = (ClampMin = "1", ClampMax = "32"))
	int32 ParticleCount = 10;

	UPROPERTY(EditDefaultsOnly, Category = "TunaSweeper|Enemy Death|Strawberry Burst", meta = (ClampMin = "0.1", UIMin = "0.1"))
	float ParticleLifetimeSeconds = 2.0f;

	UPROPERTY(EditDefaultsOnly, Category = "TunaSweeper|Enemy Death|Strawberry Burst", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float GravityCmPerSecondSquared = 980.0f;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UBillboardComponent>> ParticleSprites;

	TArray<FVector> ParticleLocations;
	TArray<FVector> ParticleVelocities;
	TArray<float> ParticleRotationDegrees;
	TArray<float> ParticleRotationSpeeds;
	TArray<bool> ParticleSettled;

	float GroundHeight = 0.0f;
	bool bHasGround = false;
};
