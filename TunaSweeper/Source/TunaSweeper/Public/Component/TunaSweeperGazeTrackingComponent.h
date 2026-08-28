#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "Engine/EngineTypes.h"
#include "TunaSweeperGazeTrackingComponent.generated.h"

class USkeletalMeshComponent;

UCLASS(ClassGroup = (TunaSweeper), meta = (BlueprintSpawnableComponent))
class TUNASWEEPER_API UTunaSweeperGazeTrackingComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UTunaSweeperGazeTrackingComponent();

	virtual void OnRegister() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Gaze")
	void SetGazeEnabled(bool bEnabled);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Gaze")
	bool IsGazeEnabled() const { return bGazeEnabled; }

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Gaze")
	void SetGazeWeight(float InWeight);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Gaze")
	void SetGazeTargetWorldTransform(const FTransform& TargetTransform);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Gaze")
	void SetTrackedMesh(USkeletalMeshComponent* InTrackedMesh);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Gaze")
	void SetEyeTargetComponents(USceneComponent* InLeftEyeTarget, USceneComponent* InRightEyeTarget);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Gaze")
	void ReturnToNeutral();

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "TunaSweeper|Gaze")
	void ApplyRecommendedEyeTargetSpacing();

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Gaze")
	USceneComponent* GetLeftEyeTarget() const;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Gaze")
	USceneComponent* GetRightEyeTarget() const;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Gaze|References")
	FComponentReference TrackedMeshReference;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Gaze|References")
	FComponentReference LeftEyeTargetReference;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Gaze|References")
	FComponentReference RightEyeTargetReference;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Gaze|Bones")
	FName LeftEyeBoneName = TEXT("cc_base_l_eye");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Gaze|Bones")
	FName RightEyeBoneName = TEXT("cc_base_r_eye");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Gaze|Bones")
	FVector EyeAimAxis = -FVector::RightVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Gaze|Bones")
	FVector EyeUpAxis = FVector::UpVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Gaze|Limits", meta = (ClampMin = "0.0", ClampMax = "90.0"))
	float MaxYawDegrees = 28.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Gaze|Limits", meta = (ClampMin = "0.0", ClampMax = "90.0"))
	float MaxPitchUpDegrees = 16.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Gaze|Limits", meta = (ClampMin = "0.0", ClampMax = "90.0"))
	float MaxPitchDownDegrees = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Gaze|Smoothing", meta = (ClampMin = "0.0"))
	float TrackingInterpolationSpeed = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Gaze|Smoothing", meta = (ClampMin = "0.0"))
	float NeutralReturnInterpolationSpeed = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Gaze|Tracking", meta = (ClampMin = "0.0"))
	float MinimumTargetDistance = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Gaze|Tracking")
	bool bGazeEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Gaze|Tracking", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float GazeWeight = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Gaze|Targets", meta = (ClampMin = "0.0"))
	float RecommendedEyeTargetSpacing = 6.4f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Gaze|Debug")
	bool bDrawDebugGaze = false;

private:
	void RefreshComponentReferences();
	void RefreshTickPrerequisite();
	void SubmitPoseRequest(float DeltaTime, bool bForceNeutral = false);
	USkeletalMeshComponent* ResolveTrackedMesh() const;
	USceneComponent* ResolveSceneComponent(const FComponentReference& Reference, FName FallbackName) const;
	void DrawDebugGaze() const;

	TWeakObjectPtr<USkeletalMeshComponent> RuntimeTrackedMesh;
	TWeakObjectPtr<USceneComponent> RuntimeLeftEyeTarget;
	TWeakObjectPtr<USceneComponent> RuntimeRightEyeTarget;
	TWeakObjectPtr<USkeletalMeshComponent> PrerequisiteMesh;
	bool bWarnedMissingPoseSink = false;
};
