#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TunaSweeperEnemySensorDebugComponent.generated.h"

class UProceduralMeshComponent;

/** Local-only visualizer for an enemy's perception tuning. It never affects AI decisions. */
UCLASS(ClassGroup = (Debug), meta = (BlueprintSpawnableComponent))
class TUNASWEEPER_API UTunaSweeperEnemySensorDebugComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTunaSweeperEnemySensorDebugComponent();

	void SetSensorDebugVisible(bool bVisible);
	bool IsSensorDebugVisible() const { return bSensorDebugVisible; }

protected:
	virtual void OnRegister() override;
	virtual void OnUnregister() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	void EnsureMeshComponents();
	void RebuildMeshes(float VisionRange, float VisionAngleDegrees);
	void BuildHearingMesh();
	void BuildVisionMesh(float VisionRange, float VisionAngleDegrees);
	void SetMeshesVisible(bool bVisible) const;

	UPROPERTY(Transient)
	TObjectPtr<UProceduralMeshComponent> HearingMesh;

	UPROPERTY(Transient)
	TObjectPtr<UProceduralMeshComponent> VisionMesh;

	UPROPERTY(EditDefaultsOnly, Category = "TunaSweeper|Debug|Sensors", meta = (ClampMin = "1.0"))
	float HearingOuterRadius = 1800.0f;

	bool bSensorDebugVisible = false;
	float CachedVisionRange = -1.0f;
	float CachedVisionAngleDegrees = -1.0f;
};
