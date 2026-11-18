#pragma once

#include "CoreMinimal.h"

class AActor;
class USplineComponent;
class UStaticMeshComponent;

struct FForestScatterBoundary
{
	bool Build(const USplineComponent& Spline);
	bool Contains(const FVector2D& Point) const;
	FBox2D Bounds{ForceInit};

private:
	TArray<FVector2D> Vertices;
};

struct FForestBuildingFootprints
{
	void AddBuilding(const AActor& Building);
	void AddBox(const FBox& LocalBounds, const FTransform& LocalToWorld);
	bool TouchesGround(const FVector& Ground, double Tolerance) const;

private:
	struct FBuildingBox
	{
		FBox LocalBounds;
		FBox WorldBounds;
		FTransform LocalToWorld;
	};
	struct FBuildingBody
	{
		TWeakObjectPtr<const UStaticMeshComponent> Mesh;
		int32 InstanceIndex;
		FBox WorldBounds;
	};
	TArray<FBuildingBox> Boxes;
	TArray<FBuildingBody> Bodies;
};

struct FForestScatterSample
{
	FVector2D XY;
	float Yaw;
	float Scale;
	int32 Seed;
};

namespace ForestScatter
{
	/** Assign one palette entry to all flowers nearest the same jittered world-space patch center. */
	int32 FlowerMaterialIndex(const FVector2D& Position, double PatchSize, int32 Seed, int32 MaterialCount);
}

/** One reproducible jittered sample per world-aligned cell. Outlines and exclusions only filter those samples. */
struct FForestScatterGrid
{
	bool Build(const FBox2D& Bounds, double InstancesPer100SquareMeters);
	int32 Num() const { return CellCount; }
	FForestScatterSample Sample(int32 Index, int32 LayerSeed) const;

private:
	FIntPoint FirstCell = FIntPoint::ZeroValue;
	double CellSize = 0.0;
	int32 Columns = 0;
	int32 CellCount = 0;
};
