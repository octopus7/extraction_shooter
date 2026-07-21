#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Subsystems/SubsystemCollection.h"
#include "TunaSweeperOcclusionRevealSubsystem.generated.h"

/** Updates the shared reveal MPC for the local player's position and cursor aim point. */
UCLASS()
class TUNASWEEPER_API UTunaSweeperOcclusionRevealSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickable() const override;

private:
	bool ResolveCursorWorldPoint(class APlayerController* PlayerController, float PlaneZ, FVector& OutCursorWorldPoint) const;
	void UpdateRevealParameters();
};
