#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "TunaSweeperGazeTestRobotCharacter.generated.h"

class USceneComponent;
class UTunaSweeperGazeSkeletalMeshComponent;
class UTunaSweeperGazeTrackingComponent;

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperGazeTestRobotCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ATunaSweeperGazeTestRobotCharacter(const FObjectInitializer& ObjectInitializer);

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TunaSweeper|Gaze Test")
	TObjectPtr<UTunaSweeperGazeTrackingComponent> GazeTracking;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TunaSweeper|Gaze Test")
	TObjectPtr<USceneComponent> LeftEyeTarget;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TunaSweeper|Gaze Test")
	TObjectPtr<USceneComponent> RightEyeTarget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Gaze Test")
	bool bTrackMouseCursor = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Gaze Test", meta = (ClampMin = "100.0"))
	float CursorTargetDistance = 1000.0f;

private:
	void ConfigureGazeComponents();
	void UpdateMouseGaze();
};
