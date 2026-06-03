#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"
#include "TunaSweeperHousingGridVisualActor.generated.h"

class ATunaSweeperHousingAreaActor;
class UMaterialInstanceDynamic;
class UProceduralMeshComponent;
class USceneComponent;

UCLASS(NotPlaceable)
class TUNASWEEPER_API ATunaSweeperHousingGridVisualActor : public AActor
{
	GENERATED_BODY()

public:
	ATunaSweeperHousingGridVisualActor();

	void RefreshVisual(
		const ATunaSweeperHousingAreaActor* HousingArea,
		const TSet<FIntPoint>& BaseCells,
		const TArray<FIntPoint>& HighlightCells,
		bool bHighlightValid);
	void ClearVisual();

private:
	void BuildCellMeshSection(
		UProceduralMeshComponent* MeshComponent,
		const ATunaSweeperHousingAreaActor* HousingArea,
		const TArray<FIntPoint>& Cells,
		const FLinearColor& Color,
		float StrokeWidth,
		float ZOffset) const;
	void AppendRoundedCell(
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
		TArray<FProcMeshTangent>& Tangents) const;
	void AppendRibbonSegment(
		const FVector& Start,
		const FVector& End,
		const FLinearColor& Color,
		float HalfStrokeWidth,
		TArray<FVector>& Vertices,
		TArray<int32>& Triangles,
		TArray<FVector>& Normals,
		TArray<FVector2D>& UV0,
		TArray<FLinearColor>& VertexColors,
		TArray<FProcMeshTangent>& Tangents) const;
	void ApplyVisualMaterials();
	void ApplyMaterialColor(UMaterialInstanceDynamic* DynamicMaterial, const FLinearColor& Color) const;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UProceduralMeshComponent> BaseGridMesh;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UProceduralMeshComponent> HighlightGridMesh;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> BaseGridMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> HighlightGridMaterial;
};
