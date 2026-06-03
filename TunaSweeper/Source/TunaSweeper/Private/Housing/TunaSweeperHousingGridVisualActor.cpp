#include "Housing/TunaSweeperHousingGridVisualActor.h"

#include "Components/SceneComponent.h"
#include "Housing/TunaSweeperHousingAreaActor.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "ProceduralMeshComponent.h"

namespace TunaSweeperHousingGridVisual
{
	const TCHAR* GridMaterialPath = TEXT("/Game/Effects/M_LedExpression_VertexColorEmissive.M_LedExpression_VertexColorEmissive");
	constexpr float CellInsetRatio = 0.145f;
	constexpr float CellInsetMin = 14.0f;
	constexpr float CellInsetMax = 22.0f;
	constexpr float CellCornerRadiusRatio = 0.17f;
	constexpr float CellCornerRadiusMax = 18.0f;
	constexpr int32 CornerSegmentCount = 6;
	constexpr float BaseStrokeWidth = 4.2f;
	constexpr float HighlightStrokeWidth = 6.4f;
	constexpr float BaseZOffset = 10.0f;
	constexpr float ValidHighlightZOffset = 22.0f;
	constexpr float InvalidHighlightZOffset = 26.0f;
	const FLinearColor EmptyCellColor(0.38f, 0.38f, 0.36f, 0.72f);
	const FLinearColor ValidCellColor(0.12f, 0.78f, 1.0f, 0.92f);
	const FLinearColor InvalidCellColor(1.0f, 0.04f, 0.035f, 0.96f);

	void ConfigureMeshComponent(UProceduralMeshComponent* MeshComponent, int32 TranslucentSortPriority)
	{
		if (!MeshComponent)
		{
			return;
		}

		MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		MeshComponent->SetGenerateOverlapEvents(false);
		MeshComponent->SetCastShadow(false);
		MeshComponent->SetReceivesDecals(false);
		MeshComponent->SetCanEverAffectNavigation(false);
		MeshComponent->SetTranslucentSortPriority(TranslucentSortPriority);
	}
}

ATunaSweeperHousingGridVisualActor::ATunaSweeperHousingGridVisualActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	BaseGridMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("BaseGridMesh"));
	BaseGridMesh->SetupAttachment(SceneRoot);
	TunaSweeperHousingGridVisual::ConfigureMeshComponent(BaseGridMesh, 50);

	HighlightGridMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("HighlightGridMesh"));
	HighlightGridMesh->SetupAttachment(SceneRoot);
	TunaSweeperHousingGridVisual::ConfigureMeshComponent(HighlightGridMesh, 100);

	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
}

void ATunaSweeperHousingGridVisualActor::RefreshVisual(
	const ATunaSweeperHousingAreaActor* HousingArea,
	const TSet<FIntPoint>& BaseCells,
	const TArray<FIntPoint>& HighlightCells,
	bool bHighlightValid)
{
	if (!HousingArea)
	{
		ClearVisual();
		return;
	}

	SetActorTransform(HousingArea->GetActorTransform());
	SetActorHiddenInGame(false);
	SetActorEnableCollision(false);
	ApplyVisualMaterials();

	const FLinearColor HighlightColor = bHighlightValid
		? TunaSweeperHousingGridVisual::ValidCellColor
		: TunaSweeperHousingGridVisual::InvalidCellColor;
	ApplyMaterialColor(HighlightGridMaterial, HighlightColor);

	TArray<FIntPoint> OrderedBaseCells;
	OrderedBaseCells.Reserve(BaseCells.Num());
	for (const FIntPoint& Cell : BaseCells)
	{
		OrderedBaseCells.Add(Cell);
	}

	BuildCellMeshSection(
		BaseGridMesh,
		HousingArea,
		OrderedBaseCells,
		TunaSweeperHousingGridVisual::EmptyCellColor,
		TunaSweeperHousingGridVisual::BaseStrokeWidth,
		TunaSweeperHousingGridVisual::BaseZOffset);

	BuildCellMeshSection(
		HighlightGridMesh,
		HousingArea,
		HighlightCells,
		HighlightColor,
		TunaSweeperHousingGridVisual::HighlightStrokeWidth,
		bHighlightValid
			? TunaSweeperHousingGridVisual::ValidHighlightZOffset
			: TunaSweeperHousingGridVisual::InvalidHighlightZOffset);
}

void ATunaSweeperHousingGridVisualActor::ClearVisual()
{
	if (BaseGridMesh)
	{
		BaseGridMesh->ClearAllMeshSections();
	}
	if (HighlightGridMesh)
	{
		HighlightGridMesh->ClearAllMeshSections();
	}
	SetActorHiddenInGame(true);
}

void ATunaSweeperHousingGridVisualActor::BuildCellMeshSection(
	UProceduralMeshComponent* MeshComponent,
	const ATunaSweeperHousingAreaActor* HousingArea,
	const TArray<FIntPoint>& Cells,
	const FLinearColor& Color,
	float StrokeWidth,
	float ZOffset) const
{
	if (!MeshComponent)
	{
		return;
	}

	MeshComponent->ClearAllMeshSections();
	if (!HousingArea || Cells.IsEmpty())
	{
		return;
	}

	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UV0;
	TArray<FLinearColor> VertexColors;
	TArray<FProcMeshTangent> Tangents;

	const int32 SegmentCountPerCell = (TunaSweeperHousingGridVisual::CornerSegmentCount * 4) + 4;
	const int32 VerticesPerCell = SegmentCountPerCell * 4;
	const int32 TrianglesPerCell = SegmentCountPerCell * 6;
	Vertices.Reserve(Cells.Num() * VerticesPerCell);
	Triangles.Reserve(Cells.Num() * TrianglesPerCell);
	Normals.Reserve(Cells.Num() * VerticesPerCell);
	UV0.Reserve(Cells.Num() * VerticesPerCell);
	VertexColors.Reserve(Cells.Num() * VerticesPerCell);
	Tangents.Reserve(Cells.Num() * VerticesPerCell);

	for (const FIntPoint& Cell : Cells)
	{
		AppendRoundedCell(
			HousingArea,
			Cell,
			Color,
			StrokeWidth,
			ZOffset,
			Vertices,
			Triangles,
			Normals,
			UV0,
			VertexColors,
			Tangents);
	}

	if (Vertices.IsEmpty())
	{
		return;
	}

	MeshComponent->CreateMeshSection_LinearColor(
		0,
		Vertices,
		Triangles,
		Normals,
		UV0,
		VertexColors,
		Tangents,
		false);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MeshComponent->SetGenerateOverlapEvents(false);
	MeshComponent->SetCastShadow(false);
}

void ATunaSweeperHousingGridVisualActor::AppendRoundedCell(
	const ATunaSweeperHousingAreaActor* HousingArea,
	const FIntPoint& Cell,
	const FLinearColor& Color,
	float StrokeWidth,
	float ZOffset,
	TArray<FVector>& Vertices,
	TArray<int32>& Triangles,
	TArray<FVector>& Normals,
	TArray<FVector2D>& UV0,
	TArray<FLinearColor>& VertexColors,
	TArray<FProcMeshTangent>& Tangents) const
{
	if (!HousingArea)
	{
		return;
	}

	const float CellSize = FMath::Max(1.0f, HousingArea->GetCellSize());
	const float HalfWidth = static_cast<float>(FMath::Max(1, HousingArea->GetGridSizeX())) * CellSize * 0.5f;
	const float HalfHeight = static_cast<float>(FMath::Max(1, HousingArea->GetGridSizeY())) * CellSize * 0.5f;
	const float MaxInset = FMath::Min(TunaSweeperHousingGridVisual::CellInsetMax, CellSize * 0.22f);
	const float MinInset = FMath::Min(TunaSweeperHousingGridVisual::CellInsetMin, MaxInset);
	const float Inset = FMath::Clamp(
		CellSize * TunaSweeperHousingGridVisual::CellInsetRatio,
		MinInset,
		MaxInset);

	const float MinX = -HalfWidth + static_cast<float>(Cell.X) * CellSize + Inset;
	const float MaxX = -HalfWidth + static_cast<float>(Cell.X + 1) * CellSize - Inset;
	const float MinY = -HalfHeight + static_cast<float>(Cell.Y) * CellSize + Inset;
	const float MaxY = -HalfHeight + static_cast<float>(Cell.Y + 1) * CellSize - Inset;
	if (MaxX <= MinX || MaxY <= MinY)
	{
		return;
	}

	const float Radius = FMath::Clamp(
		CellSize * TunaSweeperHousingGridVisual::CellCornerRadiusRatio,
		2.0f,
		FMath::Min(
			TunaSweeperHousingGridVisual::CellCornerRadiusMax,
			FMath::Min(MaxX - MinX, MaxY - MinY) * 0.5f));

	TArray<FVector> OutlinePoints;
	OutlinePoints.Reserve((TunaSweeperHousingGridVisual::CornerSegmentCount + 1) * 4);

	auto AddArc = [&OutlinePoints, ZOffset](
		const FVector2D& Center,
		float InRadius,
		float StartDegrees,
		float EndDegrees)
	{
		for (int32 SegmentIndex = 0; SegmentIndex <= TunaSweeperHousingGridVisual::CornerSegmentCount; ++SegmentIndex)
		{
			const float Alpha = static_cast<float>(SegmentIndex) /
				static_cast<float>(TunaSweeperHousingGridVisual::CornerSegmentCount);
			const float AngleDegrees = FMath::Lerp(StartDegrees, EndDegrees, Alpha);
			OutlinePoints.Add(FVector(
				Center.X + FMath::Cos(FMath::DegreesToRadians(AngleDegrees)) * InRadius,
				Center.Y + FMath::Sin(FMath::DegreesToRadians(AngleDegrees)) * InRadius,
				ZOffset));
		}
	};

	AddArc(FVector2D(MinX + Radius, MinY + Radius), Radius, 180.0f, 270.0f);
	AddArc(FVector2D(MaxX - Radius, MinY + Radius), Radius, 270.0f, 360.0f);
	AddArc(FVector2D(MaxX - Radius, MaxY - Radius), Radius, 0.0f, 90.0f);
	AddArc(FVector2D(MinX + Radius, MaxY - Radius), Radius, 90.0f, 180.0f);

	const float HalfStrokeWidth = FMath::Max(0.5f, StrokeWidth * 0.5f);
	for (int32 PointIndex = 0; PointIndex < OutlinePoints.Num(); ++PointIndex)
	{
		const FVector& Start = OutlinePoints[PointIndex];
		const FVector& End = OutlinePoints[(PointIndex + 1) % OutlinePoints.Num()];
		AppendRibbonSegment(
			Start,
			End,
			Color,
			HalfStrokeWidth,
			Vertices,
			Triangles,
			Normals,
			UV0,
			VertexColors,
			Tangents);
	}
}

void ATunaSweeperHousingGridVisualActor::AppendRibbonSegment(
	const FVector& Start,
	const FVector& End,
	const FLinearColor& Color,
	float HalfStrokeWidth,
	TArray<FVector>& Vertices,
	TArray<int32>& Triangles,
	TArray<FVector>& Normals,
	TArray<FVector2D>& UV0,
	TArray<FLinearColor>& VertexColors,
	TArray<FProcMeshTangent>& Tangents) const
{
	const FVector Direction = (End - Start).GetSafeNormal();
	if (Direction.IsNearlyZero())
	{
		return;
	}

	const FVector Perpendicular(-Direction.Y, Direction.X, 0.0f);
	const int32 StartIndex = Vertices.Num();
	Vertices.Add(Start - Perpendicular * HalfStrokeWidth);
	Vertices.Add(Start + Perpendicular * HalfStrokeWidth);
	Vertices.Add(End + Perpendicular * HalfStrokeWidth);
	Vertices.Add(End - Perpendicular * HalfStrokeWidth);

	Triangles.Add(StartIndex + 0);
	Triangles.Add(StartIndex + 2);
	Triangles.Add(StartIndex + 1);
	Triangles.Add(StartIndex + 0);
	Triangles.Add(StartIndex + 3);
	Triangles.Add(StartIndex + 2);

	for (int32 VertexIndex = 0; VertexIndex < 4; ++VertexIndex)
	{
		Normals.Add(FVector::UpVector);
		UV0.Add(FVector2D(VertexIndex == 1 || VertexIndex == 2 ? 1.0f : 0.0f, VertexIndex >= 2 ? 1.0f : 0.0f));
		VertexColors.Add(Color);
		Tangents.Add(FProcMeshTangent(1.0f, 0.0f, 0.0f));
	}
}

void ATunaSweeperHousingGridVisualActor::ApplyVisualMaterials()
{
	if (BaseGridMaterial && HighlightGridMaterial)
	{
		return;
	}

	UMaterialInterface* Material = LoadObject<UMaterialInterface>(
		nullptr,
		TunaSweeperHousingGridVisual::GridMaterialPath);
	if (!Material)
	{
		return;
	}

	if (!BaseGridMaterial)
	{
		BaseGridMaterial = UMaterialInstanceDynamic::Create(Material, this);
		ApplyMaterialColor(BaseGridMaterial, TunaSweeperHousingGridVisual::EmptyCellColor);
		if (BaseGridMesh)
		{
			BaseGridMesh->SetMaterial(0, BaseGridMaterial);
		}
	}

	if (!HighlightGridMaterial)
	{
		HighlightGridMaterial = UMaterialInstanceDynamic::Create(Material, this);
		if (HighlightGridMesh)
		{
			HighlightGridMesh->SetMaterial(0, HighlightGridMaterial);
		}
	}
}

void ATunaSweeperHousingGridVisualActor::ApplyMaterialColor(UMaterialInstanceDynamic* DynamicMaterial, const FLinearColor& Color) const
{
	if (!DynamicMaterial)
	{
		return;
	}

	const FLinearColor EmissiveColor = Color * 4.0f;
	DynamicMaterial->SetVectorParameterValue(TEXT("Color"), Color);
	DynamicMaterial->SetVectorParameterValue(TEXT("BaseColor"), Color);
	DynamicMaterial->SetVectorParameterValue(TEXT("Base Color"), Color);
	DynamicMaterial->SetVectorParameterValue(TEXT("LedColor"), Color);
	DynamicMaterial->SetVectorParameterValue(TEXT("TintColor"), Color);
	DynamicMaterial->SetVectorParameterValue(TEXT("EmissiveColor"), EmissiveColor);
	DynamicMaterial->SetVectorParameterValue(TEXT("Emissive Color"), EmissiveColor);
	DynamicMaterial->SetScalarParameterValue(TEXT("EmissiveStrength"), 4.0f);
	DynamicMaterial->SetScalarParameterValue(TEXT("Emissive Strength"), 4.0f);
	DynamicMaterial->SetScalarParameterValue(TEXT("Intensity"), 4.0f);
}
