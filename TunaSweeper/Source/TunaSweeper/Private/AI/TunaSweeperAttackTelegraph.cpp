#include "AI/TunaSweeperAttackTelegraph.h"

#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "ProceduralMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

namespace TunaSweeperAttackTelegraph
{
	constexpr int32 CircleSegments = 64;

	struct FWarningGeometry
	{
		TArray<FVector> Vertices;
		TArray<int32> Triangles;
		TArray<FVector> Normals;
		TArray<FVector2D> UVs;
		TArray<FLinearColor> Colors;
		TArray<FProcMeshTangent> Tangents;

		void Triangle(const FVector& A, const FVector& B, const FVector& C, const FLinearColor& Color)
		{
			const int32 First = Vertices.Num();
			Vertices.Append({A, B, C});
			// Both windings keep the warning readable from either side without a new material asset.
			Triangles.Append({First, First + 1, First + 2, First + 2, First + 1, First});
			for (int32 Index = 0; Index < 3; ++Index)
			{
				Normals.Add(FVector::UpVector);
				UVs.Add(FVector2D::ZeroVector);
				Colors.Add(Color);
				Tangents.Add(FProcMeshTangent(1.0f, 0.0f, 0.0f));
			}
		}

		void Quad(const FVector& A, const FVector& B, const FVector& C, const FVector& D, const FLinearColor& Color)
		{
			Triangle(A, B, C, Color);
			Triangle(A, C, D, Color);
		}

		void Commit(UProceduralMeshComponent* Mesh, int32 Section) const
		{
			if (Vertices.IsEmpty())
			{
				Mesh->ClearMeshSection(Section);
				return;
			}
			const FProcMeshSection* ExistingSection = Mesh->GetProcMeshSection(Section);
			if (ExistingSection && ExistingSection->ProcVertexBuffer.Num() == Vertices.Num())
			{
				Mesh->UpdateMeshSection_LinearColor(Section, Vertices, Normals, UVs, Colors, Tangents);
			}
			else
			{
				Mesh->CreateMeshSection_LinearColor(Section, Vertices, Triangles, Normals, UVs, Colors, Tangents, false);
			}
		}
	};
}

ATunaSweeperAttackTelegraph::ATunaSweeperAttackTelegraph()
{
	PrimaryActorTick.bCanEverTick = false;
	SetActorEnableCollision(false);
	WarningMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("WarningMesh"));
	SetRootComponent(WarningMesh);
	WarningMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WarningMesh->SetGenerateOverlapEvents(false);
	WarningMesh->SetCastShadow(false);
	WarningMesh->SetReceivesDecals(false);
	WarningMesh->SetCanEverAffectNavigation(false);
	WarningMesh->SetTranslucentSortPriority(80);

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> MaterialAsset(
		TEXT("/Game/Effects/M_LedExpression_VertexColorEmissive.M_LedExpression_VertexColorEmissive"));
	if (MaterialAsset.Succeeded())
	{
		WarningMaterial = MaterialAsset.Object;
	}
}

void ATunaSweeperAttackTelegraph::InitCircle(const FVector& Center, float Radius, float Duration)
{
	bCircle = true;
	Extent = FMath::Max(1.0f, Radius);
	WarningDuration = FMath::Max(0.01f, Duration);
	SetActorTransform(FTransform(FRotator::ZeroRotator, Center));
	Progress = 0.0f;
	ApplyMaterial();
	SampleGround();
	BuildBoundary();
	UpdateFill();
}

void ATunaSweeperAttackTelegraph::InitLane(const FVector& Start, const FVector& End, float HalfWidth, float Duration)
{
	bCircle = false;
	Extent = FMath::Max(1.0f, HalfWidth);
	WarningDuration = FMath::Max(0.01f, Duration);
	const FVector LaneDirection = (End - Start).GetSafeNormal2D();
	SetActorTransform(FTransform(LaneDirection.Rotation(), Start));
	LaneEndLocal = FVector(FVector::Dist2D(Start, End), 0.0f, End.Z - Start.Z);
	Progress = 0.0f;
	ApplyMaterial();
	SampleGround();
	BuildBoundary();
	UpdateFill();
}

void ATunaSweeperAttackTelegraph::SetProgress(float InProgress)
{
	const float NewProgress = FMath::IsFinite(InProgress) ? FMath::Clamp(InProgress, 0.0f, 1.0f) : 0.0f;
	if (!FMath::IsNearlyEqual(NewProgress, Progress))
	{
		Progress = NewProgress;
		UpdateFill();
	}
}

void ATunaSweeperAttackTelegraph::ApplyMaterial()
{
	if (!DynamicMaterial && WarningMaterial)
	{
		DynamicMaterial = UMaterialInstanceDynamic::Create(WarningMaterial, this);
		DynamicMaterial->SetScalarParameterValue(TEXT("Intensity"), 1.6f);
	}
	if (DynamicMaterial)
	{
		WarningMesh->SetMaterial(0, DynamicMaterial);
		WarningMesh->SetMaterial(1, DynamicMaterial);
	}
}

void ATunaSweeperAttackTelegraph::BuildBoundary()
{
	using namespace TunaSweeperAttackTelegraph;
	FWarningGeometry Geometry;
	const FVector Up(0.0f, 0.0f, GroundClearance);
	const float Width = FMath::Clamp(OutlineWidth, 0.1f, Extent);
	if (bCircle)
	{
		for (int32 Index = 0; Index < CircleSegments; ++Index)
		{
			const float AngleA = Index * 2.0f * UE_PI / CircleSegments;
			const float AngleB = (Index + 1) * 2.0f * UE_PI / CircleSegments;
			const FVector A(FMath::Cos(AngleA), FMath::Sin(AngleA), 0.0f);
			const FVector B(FMath::Cos(AngleB), FMath::Sin(AngleB), 0.0f);
			Geometry.Quad(A * Extent + Up, B * Extent + Up,
				B * (Extent - Width) + Up, A * (Extent - Width) + Up, BoundaryColor);
		}
	}
	else
	{
		const FVector Direction = LaneEndLocal.GetSafeNormal2D();
		const FVector Side(-Direction.Y, Direction.X, 0.0f);
		const FVector A = Up - Side * Extent;
		const FVector B = Up + Side * Extent;
		const FVector C = B + LaneEndLocal;
		const FVector D = A + LaneEndLocal;
		const int32 Segments = FMath::Clamp(FMath::CeilToInt(LaneEndLocal.X / 50.0f), 1, 128);
		for (int32 Segment = 0; Segment < Segments; ++Segment)
		{
			const FVector SegmentStart = LaneEndLocal * Segment / Segments;
			const FVector SegmentEnd = LaneEndLocal * (Segment + 1) / Segments;
			Geometry.Quad(A + SegmentStart, A + SegmentEnd, A + SegmentEnd + Side * Width,
				A + SegmentStart + Side * Width, BoundaryColor);
			Geometry.Quad(B + SegmentStart, B + SegmentStart - Side * Width, B + SegmentEnd - Side * Width,
				B + SegmentEnd, BoundaryColor);
		}
		Geometry.Quad(A, A + Direction * Width, B + Direction * Width, B, BoundaryColor);
		Geometry.Quad(D, C, C - Direction * Width, D - Direction * Width, BoundaryColor);
		// A direction chevron at the destination makes the impending movement unambiguous.
		const FVector Tip = LaneEndLocal + Up;
		Geometry.Triangle(Tip, Tip - Direction * Extent + Side * Extent * 0.5f,
			Tip - Direction * Extent - Side * Extent * 0.5f, BoundaryColor);
	}
	ConformToGround(Geometry.Vertices, GroundClearance);
	Geometry.Commit(WarningMesh, 0);
}

void ATunaSweeperAttackTelegraph::UpdateFill()
{
	using namespace TunaSweeperAttackTelegraph;
	FWarningGeometry Geometry;
	const FVector Up(0.0f, 0.0f, GroundClearance + 0.15f);
	if (Progress > 0.0f && bCircle)
	{
		// Fixed radial subdivisions follow ramps while retaining a stable section topology.
		constexpr int32 Rings = 8;
		const float FilledRadius = Extent * FMath::Sqrt(Progress);
		for (int32 Ring = 0; Ring < Rings; ++Ring)
		{
			const float InnerRadius = FMath::Min(Extent * Ring / Rings, FilledRadius);
			const float OuterRadius = FMath::Min(Extent * (Ring + 1) / Rings, FilledRadius);
			for (int32 Index = 0; Index < CircleSegments; ++Index)
			{
				const float AngleA = Index * 2.0f * UE_PI / CircleSegments;
				const float AngleB = (Index + 1) * 2.0f * UE_PI / CircleSegments;
				const FVector A(FMath::Cos(AngleA), FMath::Sin(AngleA), 0.0f);
				const FVector B(FMath::Cos(AngleB), FMath::Sin(AngleB), 0.0f);
				Geometry.Quad(Up + A * InnerRadius, Up + A * OuterRadius,
					Up + B * OuterRadius, Up + B * InnerRadius, FillColor);
			}
		}
	}
	else if (Progress > 0.0f)
	{
		const int32 Segments = FMath::Clamp(FMath::CeilToInt(LaneEndLocal.X / 50.0f), 1, 128);
		for (int32 Segment = 0; Segment < Segments; ++Segment)
		{
			const float StartX = FMath::Min(LaneEndLocal.X * Segment / Segments, LaneEndLocal.X * Progress);
			const float EndX = FMath::Min(LaneEndLocal.X * (Segment + 1) / Segments, LaneEndLocal.X * Progress);
			for (int32 Strip = 0; Strip < 4; ++Strip)
			{
				const float StartY = -Extent + 2.0f * Extent * Strip / 4;
				const float EndY = -Extent + 2.0f * Extent * (Strip + 1) / 4;
				Geometry.Quad(FVector(StartX, StartY, 0.0f), FVector(EndX, StartY, 0.0f),
					FVector(EndX, EndY, 0.0f), FVector(StartX, EndY, 0.0f), FillColor);
			}
		}
	}
	ConformToGround(Geometry.Vertices, GroundClearance + 0.15f);
	Geometry.Commit(WarningMesh, 1);
}

void ATunaSweeperAttackTelegraph::SampleGround()
{
	const float Length = bCircle ? Extent * 2.0f : FMath::Max(1.0f, LaneEndLocal.X);
	GroundGridMin = FVector2D(bCircle ? -Extent : 0.0f, -Extent);
	GroundGridCells = FIntPoint(FMath::Clamp(FMath::CeilToInt(Length / 50.0f), 1, 128),
		FMath::Clamp(FMath::CeilToInt(Extent * 2.0f / 50.0f), 1, 64));
	GroundGridStep = FVector2D(Length / GroundGridCells.X, Extent * 2.0f / GroundGridCells.Y);
	GroundHeights.SetNum((GroundGridCells.X + 1) * (GroundGridCells.Y + 1));
	FCollisionQueryParams Query(SCENE_QUERY_STAT(TunaSweeperWarningGround), false, this);
	TSet<const AActor*> IgnoredOwners;
	for (AActor* OwnerActor = GetOwner(); OwnerActor && !IgnoredOwners.Contains(OwnerActor); OwnerActor = OwnerActor->GetOwner())
	{
		IgnoredOwners.Add(OwnerActor);
		Query.AddIgnoredActor(OwnerActor);
	}
	FCollisionObjectQueryParams Objects;
	Objects.AddObjectTypesToQuery(ECC_WorldStatic);
	Objects.AddObjectTypesToQuery(ECC_WorldDynamic);
	for (int32 Y = 0; Y <= GroundGridCells.Y; ++Y)
	{
		for (int32 X = 0; X <= GroundGridCells.X; ++X)
		{
			FVector LocalPoint(GroundGridMin.X + X * GroundGridStep.X, GroundGridMin.Y + Y * GroundGridStep.Y, 0.0f);
			LocalPoint.Z = bCircle ? 0.0f : LaneEndLocal.Z * FMath::Clamp(LocalPoint.X / Length, 0.0f, 1.0f);
			const FVector WorldPoint = GetActorTransform().TransformPosition(LocalPoint);
			FHitResult Hit;
			if (GetWorld() && GetWorld()->LineTraceSingleByObjectType(Hit,
				WorldPoint + FVector(0.0f, 0.0f, 60.0f), WorldPoint - FVector(0.0f, 0.0f, 80.0f), Objects, Query) &&
				Hit.ImpactNormal.Z >= 0.5f)
			{
				LocalPoint.Z = GetActorTransform().InverseTransformPosition(Hit.ImpactPoint).Z;
			}
			GroundHeights[Y * (GroundGridCells.X + 1) + X] = LocalPoint.Z;
		}
	}
}

void ATunaSweeperAttackTelegraph::ConformToGround(TArray<FVector>& Vertices, float Elevation) const
{
	if (GroundHeights.IsEmpty())
	{
		return;
	}
	for (FVector& Vertex : Vertices)
	{
		const float GridX = FMath::Clamp(static_cast<float>((Vertex.X - GroundGridMin.X) / GroundGridStep.X), 0.0f, static_cast<float>(GroundGridCells.X));
		const float GridY = FMath::Clamp(static_cast<float>((Vertex.Y - GroundGridMin.Y) / GroundGridStep.Y), 0.0f, static_cast<float>(GroundGridCells.Y));
		const int32 X = FMath::Min(FMath::FloorToInt(GridX), GroundGridCells.X - 1);
		const int32 Y = FMath::Min(FMath::FloorToInt(GridY), GroundGridCells.Y - 1);
		const int32 Row = GroundGridCells.X + 1;
		const float Lower = FMath::Lerp(GroundHeights[Y * Row + X], GroundHeights[Y * Row + X + 1], GridX - X);
		const float Upper = FMath::Lerp(GroundHeights[(Y + 1) * Row + X], GroundHeights[(Y + 1) * Row + X + 1], GridX - X);
		Vertex.Z = FMath::Lerp(Lower, Upper, GridY - Y) + Elevation;
	}
}
