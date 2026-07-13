#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "QuadrupedRigProfile.generated.h"

class USkeletalMesh;
class USkeleton;

UENUM(BlueprintType)
enum class EQuadrupedLegSlot : uint8
{
	FrontLeft = 0,
	FrontRight = 1,
	BackLeft = 2,
	BackRight = 3
};

USTRUCT(BlueprintType)
struct MIYAKOVCHARACTERSYSTEM_API FQuadrupedLimbRigBinding
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rig")
	EQuadrupedLegSlot Slot = EQuadrupedLegSlot::FrontLeft;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rig|Bones")
	FName UpperBone = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rig|Bones")
	FName LowerBone = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rig|Bones")
	FName FootBone = NAME_None;

	/** Component-space fallback direction used when the input pose is too straight to infer a bend plane. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rig|Pole")
	FVector FallbackPoleDirection = FVector::ForwardVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rig|Pole", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float PoleDistance = 75.0f;

	/** Prefer the bend direction already present in the incoming animation pose. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rig|Pole")
	bool bDerivePoleFromInputPose = true;
};

/** Skeleton binding and solve settings for the reusable two-link quadruped IK node. */
UCLASS(BlueprintType)
class MIYAKOVCHARACTERSYSTEM_API UQuadrupedRigProfile : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UQuadrupedRigProfile();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rig")
	TObjectPtr<USkeleton> TargetSkeleton;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rig")
	TObjectPtr<USkeletalMesh> PreviewMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rig|Bones")
	FName MeshRootBone = TEXT("root");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rig|Bones")
	FName BodyBone = TEXT("body");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rig|Bones", meta = (TitleProperty = "Slot"))
	TArray<FQuadrupedLimbRigBinding> Limbs;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rig|Solve")
	bool bMaintainFootRelativeRotation = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rig|Solve")
	bool bAllowStretching = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rig|Solve", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	double StartStretchRatio = 1.0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rig|Solve", meta = (ClampMin = "1.0"))
	double MaxStretchScale = 1.0;

	UFUNCTION(BlueprintCallable, Category = "Quadruped|Rig")
	void ResetToStandardTwoLinkRobot();

	const FQuadrupedLimbRigBinding* FindLimb(EQuadrupedLegSlot Slot) const;
	bool HasCompleteFourLegBinding() const;
};
