#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WallCopingSplineActor.generated.h"

class UHierarchicalInstancedStaticMeshComponent;
class UMaterialInterface;
class USceneComponent;
class USplineComponent;
class UStaticMesh;

UENUM(BlueprintType)
enum class EWallCopingAlignmentPolicy : uint8
{
	/** Keep whole rigid modules and split the unused length equally between both ends. */
	Centered,
	/** Start with the first module flush to spline distance zero and leave remainder at the end. */
	StartAligned
};

/**
 * Repeats rigid wall-coping blocks along spline arc length using a single HISM component.
 * Local X is the module length axis, local Y is wall width, and local Z is up.
 */
UCLASS(BlueprintType, Blueprintable)
class SPLINEWORLDBUILDER_API AWallCopingSplineActor : public AActor
{
	GENERATED_BODY()

public:
	AWallCopingSplineActor();

	virtual void OnConstruction(const FTransform& Transform) override;

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Wall Coping")
	void RebuildCoping();

	UFUNCTION(BlueprintPure, Category = "Wall Coping")
	USplineComponent* GetCopingSpline() const { return Spline; }

	UFUNCTION(BlueprintPure, Category = "Wall Coping")
	int32 GetInstanceCount() const;

	UFUNCTION(BlueprintPure, Category = "Wall Coping")
	double GetResolvedModuleLength() const;

	bool GetInstanceTransform(int32 InstanceIndex, FTransform& OutTransform, bool bWorldSpace = true) const;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wall Coping|Module")
	TObjectPtr<UStaticMesh> CopingMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wall Coping|Module")
	TObjectPtr<UMaterialInterface> MaterialOverride;

	/** When enabled, the unscaled mesh X bounds determine the rigid repeat length. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wall Coping|Module")
	bool bUseMeshBoundsForModuleLength = true;

	/** Explicit fallback/override length in centimeters. X scale is never changed to fit. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wall Coping|Module", meta = (ClampMin = "1.0", Units = "cm"))
	double ModuleLength = 50.0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wall Coping|Module", meta = (ClampMin = "0.01"))
	double WidthScale = 1.0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wall Coping|Module", meta = (ClampMin = "0.01"))
	double HeightScale = 1.0;

	/** Additional empty distance between neighboring module end planes. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wall Coping|Placement", meta = (Units = "cm"))
	double Gap = 0.0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wall Coping|Placement")
	EWallCopingAlignmentPolicy AlignmentPolicy = EWallCopingAlignmentPolicy::Centered;

	/** A spline shorter than one module receives one centered rigid block instead of disappearing. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wall Coping|Placement")
	bool bPlaceSingleModuleOnShortSpline = true;

	/** Default false keeps coping horizontal and follows only spline XY yaw. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wall Coping|Orientation")
	bool bFollowSplinePitchAndRoll = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wall Coping|Orientation", meta = (Units = "cm"))
	double VerticalOffset = 0.0;

	/** Deterministic source for optional geometric variation. Material PerInstanceRandom remains automatic. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wall Coping|Variation")
	int32 Seed = 1337;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wall Coping|Variation", meta = (ClampMin = "0.0", ClampMax = "10.0", Units = "deg"))
	double MaxYawJitterDegrees = 0.0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wall Coping|Variation", meta = (ClampMin = "0.0", ClampMax = "0.25"))
	double MaxWidthScaleVariation = 0.025;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wall Coping|Variation", meta = (ClampMin = "0.0", ClampMax = "0.25"))
	double MaxHeightScaleVariation = 0.015;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wall Coping")
	bool bAutoRebuild = true;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wall Coping")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wall Coping")
	TObjectPtr<USplineComponent> Spline;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wall Coping|Generated")
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> CopingInstances;
};
