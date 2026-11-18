#include "ForestScatterGeometry.h"

#include "Algo/AnyOf.h"
#include "CollisionShape.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SplineComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Actor.h"
#include "Helpers/PCGHelpers.h"
#include "PhysicsEngine/BodyInstance.h"

bool FForestScatterBoundary::Build(const USplineComponent& Spline)
{
	Vertices.Reset();
	Bounds.Init();
	if (!Spline.IsClosedLoop() || Spline.GetNumberOfSplinePoints() < 3)
	{
		return false;
	}

	// Use the edited curve in world space. The native subdivision tolerance is squared centimetres.
	TArray<FVector> Polyline;
	if (!Spline.ConvertSplineToPolyLine(ESplineCoordinateSpace::World, 1.0f, Polyline))
	{
		return false;
	}
	Vertices.Reserve(Polyline.Num());
	for (const FVector& Point : Polyline)
	{
		const FVector2D XY(Point.X, Point.Y);
		Vertices.Add(XY);
		Bounds += XY;
	}
	return Vertices.Num() >= 3 && Bounds.GetArea() > 0.0;
}

bool FForestScatterBoundary::Contains(const FVector2D& Point) const
{
	if (Vertices.Num() < 3 || !Bounds.IsInsideOrOn(Point))
	{
		return false;
	}

	bool bInside = false;
	FVector2D Previous = Vertices.Last();
	for (const FVector2D& Current : Vertices)
	{
		if ((Previous.Y > Point.Y) != (Current.Y > Point.Y))
		{
			const double CrossingX = Previous.X + (Point.Y - Previous.Y) * (Current.X - Previous.X) / (Current.Y - Previous.Y);
			if (Point.X < CrossingX)
			{
				bInside = !bInside;
			}
		}
		Previous = Current;
	}
	return bInside;
}

void FForestBuildingFootprints::AddBox(const FBox& LocalBounds, const FTransform& LocalToWorld)
{
	if (LocalBounds.IsValid && LocalToWorld.GetScale3D().GetAbsMin() > UE_SMALL_NUMBER)
	{
		Boxes.Add({LocalBounds, LocalBounds.TransformBy(LocalToWorld), LocalToWorld});
	}
}

void FForestBuildingFootprints::AddBuilding(const AActor& Building)
{
	static const FName FootprintTag(TEXT("ForestScatterFootprint"));
	const TInlineComponentArray<UPrimitiveComponent*> Primitives(&Building, /*bIncludeFromChildActors=*/true);
	const bool bUsesMarkedFootprints = Algo::AnyOf(Primitives, [](const UPrimitiveComponent* Component)
	{
		return Component->ComponentHasTag(FootprintTag);
	});
	for (const UPrimitiveComponent* Component : Primitives)
	{
		const UStaticMeshComponent* MeshComponent = Cast<UStaticMeshComponent>(Component);
		const UInstancedStaticMeshComponent* Instances = Cast<UInstancedStaticMeshComponent>(Component);
		if (!bUsesMarkedFootprints)
		{
			if (MeshComponent && MeshComponent->GetStaticMesh() && MeshComponent->IsCollisionEnabled())
			{
				const int32 Count = Instances ? Instances->GetInstanceCount() : 1;
				for (int32 Index = 0; Index < Count; ++Index)
				{
					const int32 InstanceIndex = Instances ? Index : INDEX_NONE;
					const FBodyInstance* Body = MeshComponent->GetBodyInstance(NAME_None, /*bGetWelded=*/false, InstanceIndex);
					if (Body && Body->IsValidBodyInstance())
					{
						Bodies.Add({MeshComponent, InstanceIndex, Body->GetBodyBounds()});
					}
				}
			}
			continue;
		}
		if (!Component->ComponentHasTag(FootprintTag))
		{
			continue;
		}

		if (Instances)
		{
			if (const UStaticMesh* Mesh = Instances->GetStaticMesh())
			{
				const FBox MeshBounds = Mesh->GetBoundingBox();
				for (int32 Index = 0; Index < Instances->GetInstanceCount(); ++Index)
				{
					FTransform WorldTransform;
					if (Instances->GetInstanceTransform(Index, WorldTransform, /*bWorldSpace=*/true))
					{
						AddBox(MeshBounds, WorldTransform);
					}
				}
			}
		}
		else if (!MeshComponent || MeshComponent->GetStaticMesh())
		{
			AddBox(Component->CalcBounds(FTransform::Identity).GetBox(), Component->GetComponentTransform());
		}
	}
}

bool FForestBuildingFootprints::TouchesGround(const FVector& Ground, double Tolerance) const
{
	const FVector VerticalOffset(0, 0, FMath::Max(Tolerance, 0.0));
	const FVector Lower = Ground - VerticalOffset;
	const FVector Upper = Ground + VerticalOffset;
	for (const FBuildingBox& Box : Boxes)
	{
		if (!Box.WorldBounds.IsInsideOrOnXY(Ground) || Lower.Z > Box.WorldBounds.Max.Z || Upper.Z < Box.WorldBounds.Min.Z)
		{
			continue;
		}
		const FVector LocalStart = Box.LocalToWorld.InverseTransformPosition(Lower);
		const FVector LocalEnd = Box.LocalToWorld.InverseTransformPosition(Upper);
		if (FMath::LineBoxIntersection(Box.LocalBounds, LocalStart, LocalEnd, LocalEnd - LocalStart))
		{
			return true;
		}
	}
	// A narrow vertical probe also detects a point inside a solid foundation. The radius is one millimetre.
	constexpr float ProbeRadius = 0.1f;
	const FCollisionShape Probe = FCollisionShape::MakeCapsule(ProbeRadius, FMath::Max(ProbeRadius, float(VerticalOffset.Z)));
	for (const FBuildingBody& BuildingBody : Bodies)
	{
		const FBox& Bounds = BuildingBody.WorldBounds;
		if (!Bounds.IsInsideOrOnXY(Ground) || Lower.Z > Bounds.Max.Z || Upper.Z < Bounds.Min.Z)
		{
			continue;
		}
		if (const UStaticMeshComponent* Mesh = BuildingBody.Mesh.Get())
		{
			const FBodyInstance* Body = Mesh->GetBodyInstance(NAME_None, /*bGetWelded=*/false, BuildingBody.InstanceIndex);
			if (Body && Body->OverlapTest(Ground, FQuat::Identity, Probe))
			{
				return true;
			}
		}
	}
	return false;
}

bool FForestScatterGrid::Build(const FBox2D& Bounds, double InstancesPer100SquareMeters)
{
	CellCount = 0;
	if (!Bounds.bIsValid || Bounds.GetArea() <= 0 || InstancesPer100SquareMeters <= 0 || !FMath::IsFinite(InstancesPer100SquareMeters))
	{
		return false;
	}

	// A 10 m by 10 m patch has an area of 100 square metres. Density controls the cell area.
	CellSize = 1000.0 / FMath::Sqrt(InstancesPer100SquareMeters);
	const double FirstX = FMath::FloorToDouble(Bounds.Min.X / CellSize);
	const double FirstY = FMath::FloorToDouble(Bounds.Min.Y / CellSize);
	const double LastX = FMath::CeilToDouble(Bounds.Max.X / CellSize) - 1.0;
	const double LastY = FMath::CeilToDouble(Bounds.Max.Y / CellSize) - 1.0;
	const double NumColumns = LastX - FirstX + 1.0;
	const double NumRows = LastY - FirstY + 1.0;
	if (FirstX < MIN_int32 || FirstY < MIN_int32 || LastX > MAX_int32 || LastY > MAX_int32 ||
		NumColumns < 1 || NumRows < 1 || NumColumns * NumRows > MAX_int32)
	{
		return false;
	}
	FirstCell = FIntPoint(static_cast<int32>(FirstX), static_cast<int32>(FirstY));
	Columns = static_cast<int32>(NumColumns);
	CellCount = Columns * static_cast<int32>(NumRows);
	return true;
}

FForestScatterSample FForestScatterGrid::Sample(int32 Index, int32 LayerSeed) const
{
	check(Index >= 0 && Index < CellCount);
	const int32 CellX = FirstCell.X + Index % Columns;
	const int32 CellY = FirstCell.Y + Index / Columns;
	const int32 Seed = PCGHelpers::ComputeSeed(LayerSeed, CellX, CellY);
	FRandomStream Random(Seed);
	const double X = (static_cast<double>(CellX) + Random.FRand()) * CellSize;
	const double Y = (static_cast<double>(CellY) + Random.FRand()) * CellSize;
	const float Yaw = Random.FRandRange(0.0f, 360.0f);
	const float Scale = Random.FRandRange(0.95f, 1.05f);
	return {FVector2D(X, Y), Yaw, Scale, Seed};
}

int32 ForestScatter::FlowerMaterialIndex(const FVector2D& Position, double PatchSize, int32 Seed, int32 MaterialCount)
{
	check(MaterialCount > 0);
	if (MaterialCount == 1 || !(PatchSize > 0.0))
	{
		return 0;
	}
	const FVector2D Scaled = Position / PatchSize;
	const int64 CellX = FMath::FloorToInt64(Scaled.X);
	const int64 CellY = FMath::FloorToInt64(Scaled.Y);
	const FVector2D Local(Scaled.X - CellX, Scaled.Y - CellY);
	double Nearest = TNumericLimits<double>::Max();
	int32 Color = 0;
	for (int32 Y = -1; Y <= 1; ++Y)
	{
		for (int32 X = -1; X <= 1; ++X)
		{
			FRandomStream Patch(PCGHelpers::ComputeSeed(Seed, GetTypeHash(CellX + X), GetTypeHash(CellY + Y)));
			const double CenterX = X + Patch.FRandRange(0.2f, 0.8f);
			const double CenterY = Y + Patch.FRandRange(0.2f, 0.8f);
			const double Distance = FVector2D::DistSquared(Local, FVector2D(CenterX, CenterY));
			if (Distance < Nearest)
			{
				Nearest = Distance;
				Color = Patch.RandRange(0, MaterialCount - 1);
			}
		}
	}
	return Color;
}
