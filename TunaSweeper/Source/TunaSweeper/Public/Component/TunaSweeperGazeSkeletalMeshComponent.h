#pragma once

#include "CoreMinimal.h"
#include "Component/TunaSweeperGazePoseSink.h"
#include "Components/SkeletalMeshComponent.h"
#include "TunaSweeperGazeSkeletalMeshComponent.generated.h"

UCLASS(ClassGroup = (TunaSweeper), meta = (BlueprintSpawnableComponent))
class TUNASWEEPER_API UTunaSweeperGazeSkeletalMeshComponent : public USkeletalMeshComponent, public ITunaSweeperGazePoseSink
{
	GENERATED_BODY()

public:
	virtual void SetGazePoseRequest(const FTunaSweeperGazePoseRequest& Request) override;
	virtual void FinalizeBoneTransform() override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Gaze")
	bool bApplyDirectEyeGaze = true;

private:
	bool IsBoneDescendantOf(int32 BoneIndex, int32 ParentBoneIndex) const;
	void ApplyEyeGazeToEditablePose();
	void ApplyEyeGazeBranch(
		TArray<FTransform>& ComponentSpaceTransforms,
		FName EyeBoneName,
		const FVector& TargetWorldLocation,
		bool bHasTarget,
		float& InOutCurrentYawDegrees,
		float& InOutCurrentPitchDegrees);

	FTunaSweeperGazePoseRequest CurrentGazePoseRequest;
	float CurrentLeftEyeYawDegrees = 0.0f;
	float CurrentLeftEyePitchDegrees = 0.0f;
	float CurrentRightEyeYawDegrees = 0.0f;
	float CurrentRightEyePitchDegrees = 0.0f;
};
