#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Subsystems/SubsystemCollection.h"
#include "TunaSweeperOcclusionRevealSubsystem.generated.h"

/** Projects the local player and cursor world-radius reveals into the shared screen-space MPC. */
UCLASS()
class TUNASWEEPER_API UTunaSweeperOcclusionRevealSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickable() const override;

private:
	struct FScreenRevealCircle
	{
		FVector2D CenterUv = FVector2D::ZeroVector;
		float InnerRadiusScreenWidth = 0.0f;
		float OuterRadiusScreenWidth = 0.0f;
	};

	bool ResolveCursorWorldPoint(class APlayerController* PlayerController, float PlaneZ, FVector& OutCursorWorldPoint) const;
	bool ProjectWorldRevealCircle(class APlayerController* PlayerController, const FVector& WorldCenter, float InnerRadiusCm, float OuterRadiusCm, FScreenRevealCircle& OutCircle) const;
	void UpdateRevealParameters();
};
