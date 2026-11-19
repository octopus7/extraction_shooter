#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CoastalGull.generated.h"

class UStaticMeshComponent;

UCLASS(Blueprintable)
class COASTALWILDLIFE_API ACoastalGull : public AActor
{
	GENERATED_BODY()
public:
	ACoastalGull();
	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

	/** Shoreward ground anchor is the actor origin. Aim its local +X out over water. */
	UPROPERTY(EditAnywhere, Category="Coastal Gull|Route", meta=(ClampMin="1", Units="cm"))
	float FlightReach = 1050.f;
	UPROPERTY(EditAnywhere, Category="Coastal Gull|Route", meta=(ClampMin="1", Units="cm"))
	float FlightWidth = 400.f;
	UPROPERTY(EditAnywhere, Category="Coastal Gull|Route", meta=(ClampMin="1", Units="cm"))
	float FlightHeight = 400.f;
	/** Reserve this length of dry shore in local +Y for walking and takeoff. */
	UPROPERTY(EditAnywhere, Category="Coastal Gull|Route", meta=(ClampMin="1", Units="cm"))
	float WalkDistance = 100.f;
	/** Dry-sand weight used by the shore material's view-dependent height offset. */
	UPROPERTY(EditAnywhere, Category="Coastal Gull|Rendering", meta=(ClampMin="0", ClampMax="1"))
	float ShoreLiftFactor = 0.f;

private:
	friend class FCoastalGullMotionTest;
	void InitializeRoute();
	void ApplyMotion();
	void ApplyShoreLift(bool bSetDefaults);

	UPROPERTY(VisibleAnywhere, Category="Coastal Gull") TObjectPtr<UStaticMeshComponent> Body;
	UPROPERTY(VisibleAnywhere, Category="Coastal Gull") TObjectPtr<UStaticMeshComponent> WingL;
	UPROPERTY(VisibleAnywhere, Category="Coastal Gull") TObjectPtr<UStaticMeshComponent> WingR;
	UPROPERTY(VisibleAnywhere, Category="Coastal Gull") TObjectPtr<UStaticMeshComponent> LegL;
	UPROPERTY(VisibleAnywhere, Category="Coastal Gull") TObjectPtr<UStaticMeshComponent> LegR;

	FTransform ShoreFrame;
	float TakeoffGroundOffset = 0.f;
	float CycleTime = 0.f;
};
