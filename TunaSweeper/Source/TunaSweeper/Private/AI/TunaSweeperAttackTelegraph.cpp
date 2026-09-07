#include "AI/TunaSweeperAttackTelegraph.h"

#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "ProceduralMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

namespace TunaSweeperAttackTelegraph
{
	constexpr int32 CircleSegments = 64;

	FLinearColor WithAlpha(const FLinearColor& Color, float Alpha)
	{
		return FLinearColor(Color.R, Color.G, Color.B, FMath::Clamp(Alpha, 0.0f, 1.0f));
	}

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
			// The authored warning materials are two sided, avoiding doubled translucent overdraw.
			Triangles.Append({First, First + 1, First + 2});
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

		void Line(const FVector& Start, const FVector& End, float Width, const FLinearColor& Color)
		{
			const FVector Direction = (End - Start).GetSafeNormal2D();
			const FVector Side(-Direction.Y * Width * 0.5f, Direction.X * Width * 0.5f, 0.0f);
			Quad(Start - Side, End - Side, End + Side, Start + Side, Color);
		}

		void Arc(float OuterRadius, float Width, float AngleA, float AngleB, const FLinearColor& Color)
		{
			const FVector A(FMath::Cos(AngleA), FMath::Sin(AngleA), 0.0f);
			const FVector B(FMath::Cos(AngleB), FMath::Sin(AngleB), 0.0f);
			const float InnerRadius = FMath::Max(0.0f, OuterRadius - Width);
			Quad(A * OuterRadius, B * OuterRadius, B * InnerRadius, A * InnerRadius, Color);
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

	static ConstructorHelpers::FObjectFinderOptional<UMaterialInterface> MaterialAsset(
		TEXT("/Game/Characters/CombatPatterns/Materials/M_CP_Effect.M_CP_Effect"));
	static ConstructorHelpers::FObjectFinderOptional<UMaterialInterface> AccentAsset(
		TEXT("/Game/Characters/CombatPatterns/Materials/M_CP_Glow.M_CP_Glow"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> FallbackAsset(
		TEXT("/Game/Effects/M_LedExpression_VertexColorEmissive.M_LedExpression_VertexColorEmissive"));
	WarningMaterial = MaterialAsset.Get() ? MaterialAsset.Get() : FallbackAsset.Object.Get();
	AccentMaterial = AccentAsset.Get() ? AccentAsset.Get() : FallbackAsset.Object.Get();
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
		DynamicMaterial->SetScalarParameterValue(TEXT("Intensity"), 1.25f);
	}
	if (!DynamicAccentMaterial && AccentMaterial)
	{
		DynamicAccentMaterial = UMaterialInstanceDynamic::Create(AccentMaterial, this);
		DynamicAccentMaterial->SetScalarParameterValue(TEXT("Intensity"), 1.8f);
	}
	WarningMesh->SetMaterial(0, DynamicAccentMaterial);
	WarningMesh->SetMaterial(1, DynamicMaterial);
	WarningMesh->SetMaterial(2, DynamicAccentMaterial);
}

void ATunaSweeperAttackTelegraph::BuildBoundary()
{
	using namespace TunaSweeperAttackTelegraph;
	FWarningGeometry Geometry;
	const float Width = FMath::Clamp(OutlineWidth, 0.1f, Extent);
	const FLinearColor QuietColor = WithAlpha(BoundaryColor, BoundaryColor.A * 0.42f);
	if (bCircle)
	{
		for (int32 Index = 0; Index < CircleSegments; ++Index)
		{
			const float AngleA = Index * 2.0f * UE_PI / CircleSegments;
			const float AngleB = (Index + 1) * 2.0f * UE_PI / CircleSegments;
			// The continuous hairline is the exact damage radius; every decoration stays inside it.
			Geometry.Arc(Extent, Width * 0.35f, AngleA, AngleB, QuietColor);
			Geometry.Arc(Extent - Width * 0.50f, Width * 0.65f,
				AngleA + (AngleB - AngleA) * 0.12f, AngleB - (AngleB - AngleA) * 0.12f, BoundaryColor);
		}
		for (int32 Index = 0; Index < 12; ++Index)
		{
			const float Angle = Index * 2.0f * UE_PI / 12.0f;
			const FVector Direction(FMath::Cos(Angle), FMath::Sin(Angle), 0.0f);
			Geometry.Line(Direction * Extent * 0.86f, Direction * (Extent - Width * 2.0f),
				FMath::Min(Width * 0.45f, Extent * 0.012f), QuietColor);
		}
		const float Reticle = Extent * 0.035f;
		Geometry.Line(FVector(-Reticle, 0.0f, 0.0f), FVector(Reticle, 0.0f, 0.0f), Width * 0.35f, QuietColor);
		Geometry.Line(FVector(0.0f, -Reticle, 0.0f), FVector(0.0f, Reticle, 0.0f), Width * 0.35f, QuietColor);
	}
	else
	{
		const float Length = LaneEndLocal.X;
		const int32 Segments = FMath::Clamp(FMath::CeilToInt(Length / 50.0f), 1, 128);
		for (int32 Segment = 0; Segment < Segments; ++Segment)
		{
			const float StartX = Length * Segment / Segments;
			const float EndX = Length * (Segment + 1) / Segments;
			Geometry.Quad(FVector(StartX, -Extent, 0.0f), FVector(EndX, -Extent, 0.0f),
				FVector(EndX, -Extent + Width * 0.65f, 0.0f), FVector(StartX, -Extent + Width * 0.65f, 0.0f), BoundaryColor);
			Geometry.Quad(FVector(StartX, Extent - Width * 0.65f, 0.0f), FVector(EndX, Extent - Width * 0.65f, 0.0f),
				FVector(EndX, Extent, 0.0f), FVector(StartX, Extent, 0.0f), BoundaryColor);
		}
		const float CapWidth = FMath::Min(Width * 0.65f, Length * 0.5f);
		Geometry.Quad(FVector(0.0f, -Extent, 0.0f), FVector(CapWidth, -Extent, 0.0f),
			FVector(CapWidth, Extent, 0.0f), FVector(0.0f, Extent, 0.0f), BoundaryColor);
		Geometry.Quad(FVector(Length - CapWidth, -Extent, 0.0f), FVector(Length, -Extent, 0.0f),
			FVector(Length, Extent, 0.0f), FVector(Length - CapWidth, Extent, 0.0f), BoundaryColor);
		// Repeated hollow chevrons communicate travel direction without covering the target's feet.
		const int32 Arrows = FMath::Clamp(FMath::FloorToInt(Length / FMath::Max(70.0f, Extent * 1.8f)), 1, 12);
		const float ArrowLength = FMath::Min(Extent * 0.65f, Length / (Arrows + 1) * 0.55f);
		for (int32 Index = 0; Index < Arrows; ++Index)
		{
			const float X = Length * (Index + 1) / (Arrows + 1);
			const FVector Tip(X + ArrowLength * 0.5f, 0.0f, 0.0f);
			Geometry.Line(FVector(X - ArrowLength * 0.5f, -Extent * 0.36f, 0.0f), Tip, Width * 0.55f, QuietColor);
			Geometry.Line(Tip, FVector(X - ArrowLength * 0.5f, Extent * 0.36f, 0.0f), Width * 0.55f, QuietColor);
		}
	}
	ConformToGround(Geometry.Vertices, GroundClearance);
	Geometry.Commit(WarningMesh, 0);
}

void ATunaSweeperAttackTelegraph::UpdateFill()
{
	using namespace TunaSweeperAttackTelegraph;
	FWarningGeometry Geometry;
	FWarningGeometry Accent;
	const float Width = FMath::Clamp(OutlineWidth, 0.1f, Extent);
	const FLinearColor CurrentFill = WithAlpha(FillColor, FillColor.A * FMath::Lerp(0.70f, 1.20f, Progress));
	const FLinearColor FrontierColor(1.0f, FMath::Lerp(0.49f, 0.25f, Progress), 0.055f, 0.70f);
	if (Progress > 0.0f && bCircle)
	{
		// Filled area is proportional to elapsed warning time; the final radius never changes.
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
				Geometry.Arc(OuterRadius, OuterRadius - InnerRadius, AngleA, AngleB, CurrentFill);
			}
		}
		for (int32 Index = 0; Index < CircleSegments; ++Index)
		{
			const float AngleA = Index * 2.0f * UE_PI / CircleSegments;
			const float AngleB = (Index + 1) * 2.0f * UE_PI / CircleSegments;
			Accent.Arc(FilledRadius, FMath::Min(Width * 0.45f, FilledRadius), AngleA, AngleB, FrontierColor);
		}
		// Four short moving ticks follow the expanding front, kept inside the advertised area.
		for (int32 Index = 0; Index < 4; ++Index)
		{
			const float Angle = Index * UE_PI * 0.5f + Progress * UE_PI * 0.15f;
			const FVector Direction(FMath::Cos(Angle), FMath::Sin(Angle), 0.0f);
			Accent.Line(Direction * FMath::Max(0.0f, FilledRadius - Width * 3.0f),
				Direction * FMath::Max(0.0f, FilledRadius - Width), Width * 0.38f, FrontierColor);
		}
	}
	else if (Progress > 0.0f)
	{
		const float FilledLength = LaneEndLocal.X * Progress;
		const int32 Segments = FMath::Clamp(FMath::CeilToInt(LaneEndLocal.X / 50.0f), 1, 128);
		for (int32 Segment = 0; Segment < Segments; ++Segment)
		{
			const float StartX = FMath::Min(LaneEndLocal.X * Segment / Segments, FilledLength);
			const float EndX = FMath::Min(LaneEndLocal.X * (Segment + 1) / Segments, FilledLength);
			for (int32 Strip = 0; Strip < 4; ++Strip)
			{
				const float StartY = -Extent + 2.0f * Extent * Strip / 4;
				const float EndY = -Extent + 2.0f * Extent * (Strip + 1) / 4;
				Geometry.Quad(FVector(StartX, StartY, 0.0f), FVector(EndX, StartY, 0.0f),
					FVector(EndX, EndY, 0.0f), FVector(StartX, EndY, 0.0f), CurrentFill);
			}
		}
		const float FrontierStart = FMath::Max(0.0f, FilledLength - Width * 0.55f);
		for (int32 Strip = 0; Strip < 4; ++Strip)
		{
			const float StartY = -Extent + 2.0f * Extent * Strip / 4;
			const float EndY = -Extent + 2.0f * Extent * (Strip + 1) / 4;
			Accent.Quad(FVector(FrontierStart, StartY, 0.0f), FVector(FilledLength, StartY, 0.0f),
				FVector(FilledLength, EndY, 0.0f), FVector(FrontierStart, EndY, 0.0f), FrontierColor);
		}
	}
	ConformToGround(Geometry.Vertices, GroundClearance + 0.15f);
	Geometry.Commit(WarningMesh, 1);
	ConformToGround(Accent.Vertices, GroundClearance + 0.35f);
	Accent.Commit(WarningMesh, 2);
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
