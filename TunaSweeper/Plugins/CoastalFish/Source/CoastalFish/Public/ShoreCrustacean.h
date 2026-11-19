#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"
#include "ShoreCrustacean.generated.h"

class UPoseableMeshComponent;
class USkeletalMesh;

UCLASS(Blueprintable)
class COASTALFISH_API AShoreCrustacean : public AActor
{
	GENERATED_BODY()
public:
	AShoreCrustacean();
	virtual void Tick(float DeltaSeconds) override;
protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

	UPROPERTY(EditAnywhere, Category="Crustacean") bool bIsCrayfish = false;
	UPROPERTY(EditAnywhere, Category="Crustacean|Route", meta=(ClampMin="1")) FVector2D RouteExtent = FVector2D(65,28);
	UPROPERTY(EditAnywhere, Category="Crustacean|Route", meta=(ClampMin="0")) float InitialPhaseSeconds = 0.f;
	/** Optional 3x3 visual bed heights, relative to anchor world Z. X varies fastest, from -Extent to +Extent. */
	UPROPERTY(EditAnywhere, Category="Crustacean|Ground") TArray<float> BedHeightOffsets;
	UPROPERTY(EditAnywhere, Category="Crustacean|Ground", meta=(ClampMin="1",Units="cm")) float GroundProbeHeight = 45.f;
	UPROPERTY(EditAnywhere, Category="Crustacean|Ground", meta=(ClampMin="1",Units="cm")) float GroundProbeDepth = 70.f;
	UPROPERTY(EditAnywhere, Category="Crustacean|Ground", meta=(Units="cm")) float GroundOffset = 0.f;
	UPROPERTY(EditAnywhere, Category="Crustacean|Ground", meta=(ClampMin="0",ClampMax="1")) float ShoreLiftFactor = 0.f;

private:
	friend class FShoreCrustaceanTest;
	UPROPERTY(VisibleAnywhere, Category="Crustacean") TObjectPtr<UPoseableMeshComponent> Shell;
	UPROPERTY() TSoftObjectPtr<USkeletalMesh> CrabMesh;
	UPROPERTY() TSoftObjectPtr<USkeletalMesh> CrayfishMesh;
	TArray<int32> Joints;
	FTimerHandle ProximityTimer;
	double MotionTime = 0.;

	void InitializeAnimal();
	void RefreshProximity();
	void ApplyPlayerLocations(const TArray<FVector>& Players);
	void FindBed(const FVector2D& LocalXY, FVector& Position, FVector& Normal) const;
	void UpdatePose();
};
