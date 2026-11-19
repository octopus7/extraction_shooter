#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"
#include "CoastalFishHabitat.generated.h"

class UPoseableMeshComponent;
class UInstancedStaticMeshComponent;

UCLASS(Blueprintable)
class COASTALFISH_API ACoastalFishHabitat : public AActor
{
	GENERATED_BODY()
public:
	ACoastalFishHabitat();
	virtual void Tick(float DeltaSeconds) override;
protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

	UPROPERTY(EditAnywhere, Category="Fish") float InitialPhaseSeconds = 0.f;
	UPROPERTY(EditAnywhere, Category="Fish|Medium") bool bEnableMedium = true;
	UPROPERTY(EditAnywhere, Category="Fish|Large") bool bEnableLarge = true;
	UPROPERTY(EditAnywhere, Category="Fish|Fry") bool bEnableFry = true;
	/** In local centimeters, +X points offshore, +Y follows the shore, Z=0 is water level. */
	UPROPERTY(EditAnywhere, Category="Fish|Medium") FVector MediumCenter = FVector(500,0,-85);
	UPROPERTY(EditAnywhere, Category="Fish|Medium") FVector MediumExtent = FVector(100,225,12);
	UPROPERTY(EditAnywhere, Category="Fish|Large") FVector LargeCenter = FVector(1200,180,-240);
	UPROPERTY(EditAnywhere, Category="Fish|Large") FVector LargeExtent = FVector(240,480,24);
	UPROPERTY(EditAnywhere, Category="Fish|Fry") FVector FryCenter = FVector(230,-190,-44);
	UPROPERTY(EditAnywhere, Category="Fish|Fry") FVector FryExtent = FVector(75,160,4);
	UPROPERTY(EditAnywhere, Category="Fish|Fry", meta=(ClampMin="20",ClampMax="40"))
	int32 FryCount = 28;
	/** School splits around an obstacle at the route center. In local centimeters. */
	UPROPERTY(EditAnywhere, Category="Fish|Fry", meta=(ClampMin="0",ClampMax="100"))
	float FryObstacleRadius = 28.f;

private:
	friend class FCoastalFishTest;
	UPROPERTY(VisibleAnywhere, Category="Fish") TObjectPtr<UPoseableMeshComponent> MediumFish;
	UPROPERTY(VisibleAnywhere, Category="Fish") TObjectPtr<UPoseableMeshComponent> LargeFish;
	UPROPERTY(VisibleAnywhere, Category="Fish") TObjectPtr<UInstancedStaticMeshComponent> School;

	struct FJoint
	{
		int32 Index;
		FQuat RestRotation;
		FVector BendAxis;
	};
	TArray<FJoint> MediumJoints;
	TArray<FJoint> LargeJoints;
	TArray<FTransform> FryTransforms;
	TArray<float> FryShaderData;
	FTimerHandle ProximityTimer;
	double MediumTime = 0.;
	double LargeTime = 0.;
	double FryTime = 0.;
	bool bMediumActive = false;
	bool bLargeActive = false;
	bool bFryActive = false;

	void InitializePose();
	void ResetSchool();
	void RefreshProximity();
	void ApplyPlayerLocations(const TArray<FVector>& Players);
	void UpdateAdult(bool bLarge);
	FVector FryPosition(int32 Index, double Time) const;
	void UpdateFry();
};
