#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SplineWorldBuilderProfile.generated.h"

class UMaterialInterface;
class UStaticMesh;

UENUM(BlueprintType)
enum class ESplineWorldBuilderKind : uint8
{
	Trail,
	Fence,
	Wall,
	BuildingWall,
	TopTrim
};

/**
 * Reusable mesh and placement contract for one family of spline-built geometry.
 * Rigid modules keep their shape; only their length may be adjusted within the configured range.
 */
UCLASS(BlueprintType)
class SPLINEWORLDBUILDER_API USplineWorldBuilderProfile : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Builder")
	ESplineWorldBuilderKind BuilderKind = ESplineWorldBuilderKind::Wall;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Modules")
	TObjectPtr<UStaticMesh> StraightMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Modules")
	TObjectPtr<UStaticMesh> EndMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Modules")
	TObjectPtr<UStaticMesh> CornerMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Modules")
	TObjectPtr<UStaticMesh> TJunctionMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Modules")
	TObjectPtr<UStaticMesh> CrossJunctionMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Modules")
	TObjectPtr<UMaterialInterface> MaterialOverride;

	/** Nominal X-axis length of a straight module, in centimeters. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Placement", meta = (ClampMin = "1.0", Units = "cm"))
	double ModuleLength = 200.0;

	/** Amount removed from a chain end that connects to a junction. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Placement", meta = (ClampMin = "0.0", Units = "cm"))
	double JunctionTrimDistance = 100.0;

	/** Amount reserved for a free end-piece. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Placement", meta = (ClampMin = "0.0", Units = "cm"))
	double EndTrimDistance = 100.0;

	/** Amount reserved on each incident segment around a detected corner. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Placement", meta = (ClampMin = "0.0", Units = "cm"))
	double CornerTrimDistance = 100.0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Placement", meta = (ClampMin = "0.0", ClampMax = "180.0", Units = "deg"))
	double CornerAngleThreshold = 20.0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Placement", meta = (ClampMin = "0.01"))
	double MinimumLengthScale = 0.80;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Placement", meta = (ClampMin = "0.01"))
	double MaximumLengthScale = 1.20;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Placement")
	bool bAllowLengthScaling = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Placement")
	bool bPlaceFreeEndPieces = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Placement")
	bool bPlaceCornerPieces = true;

	/** Maximum deviation from 180 degrees that still counts as a straight two-arm junction. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Junction", meta = (ClampMin = "0.0", ClampMax = "90.0", Units = "deg"))
	double StraightJunctionTolerance = 15.0;
};
