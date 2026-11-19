#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"
#include "BeachDetailMotion.generated.h"

class UInstancedStaticMeshComponent;
class UStaticMesh;
class UStaticMeshComponent;

UCLASS(Blueprintable)
class COASTALFISH_API ABeachDetailMotion : public AActor
{
	GENERATED_BODY()
public:
	ABeachDetailMotion();
	virtual void Tick(float DeltaSeconds) override;
protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

	/** Place the anchor at the burrow mouth or at the buoy's neutral waterline. */
	UPROPERTY(EditAnywhere, Category="Beach Detail") bool bIsBurrow = false;
	UPROPERTY(EditAnywhere, Category="Beach Detail", meta=(ClampMin="0",ClampMax="1")) float ShoreLiftFactor = 0.f;
	UPROPERTY(EditAnywhere, Category="Beach Detail", meta=(ClampMin="0")) float InitialPhaseSeconds = 0.f;

private:
	friend class FBeachDetailMotionTest;
	UPROPERTY(VisibleAnywhere, Category="Beach Detail") TObjectPtr<UStaticMeshComponent> Body;
	UPROPERTY(VisibleAnywhere, Category="Beach Detail") TObjectPtr<UStaticMeshComponent> Ripple;
	UPROPERTY(VisibleAnywhere, Category="Beach Detail") TObjectPtr<UInstancedStaticMeshComponent> Pellets;
	UPROPERTY() TSoftObjectPtr<UStaticMesh> BuoyMesh;
	UPROPERTY() TSoftObjectPtr<UStaticMesh> RippleMesh;
	UPROPERTY() TSoftObjectPtr<UStaticMesh> BurrowMesh;
	UPROPERTY() TSoftObjectPtr<UStaticMesh> GrainMesh;

	struct FGrain
	{
		FVector Start;
		FVector Direction;
		double Distance;
		double Height;
		double Delay;
		double Duration;
		double Scale;
	};
	TArray<FGrain> Emission;
	TArray<FTransform> GrainTransforms;
	FRandomStream Random;
	FTimerHandle ProximityTimer;
	FTimerHandle EventTimer;
	double BuoyTime = 0.;
	double EventAge = 0.;
	bool bNearby = false;

	void InitializeDetail();
	void RefreshProximity();
	void ApplyPlayerLocations(const TArray<FVector>& Players);
	void UpdateBuoy();
	void ScheduleBurrowEvent(bool bFirst);
	void StartBurrowEvent();
	void UpdateGrains();
};
