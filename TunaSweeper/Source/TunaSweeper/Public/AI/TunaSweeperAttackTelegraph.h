#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TunaSweeperAttackTelegraph.generated.h"

class UMaterialInterface;
class UMaterialInstanceDynamic;
class UProceduralMeshComponent;

/** Collisionless, packaged-game ground warning. The attack owns its timer and calls SetProgress. */
UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperAttackTelegraph : public AActor
{
	GENERATED_BODY()

public:
	ATunaSweeperAttackTelegraph();

	/** Center/Start/End are ground positions in world space. */
	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Combat Pattern|Warning")
	void InitCircle(const FVector& Center, float Radius, float Duration);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Combat Pattern|Warning")
	void InitLane(const FVector& Start, const FVector& End, float HalfWidth, float Duration);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Combat Pattern|Warning")
	void SetProgress(float InProgress);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Combat Pattern|Warning")
	float GetProgress() const { return Progress; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Combat Pattern|Warning")
	float GetWarningDuration() const { return WarningDuration; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UProceduralMeshComponent> WarningMesh;

	// A hard material reference keeps the warning available in cooked builds.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TunaSweeper|Combat Pattern|Warning")
	TObjectPtr<UMaterialInterface> WarningMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TunaSweeper|Combat Pattern|Warning")
	TObjectPtr<UMaterialInterface> AccentMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TunaSweeper|Combat Pattern|Warning")
	FLinearColor BoundaryColor = FLinearColor(1.0f, 0.20f, 0.025f, 0.90f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TunaSweeper|Combat Pattern|Warning")
	FLinearColor FillColor = FLinearColor(1.0f, 0.25f, 0.035f, 0.17f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TunaSweeper|Combat Pattern|Warning", meta = (ClampMin = "0.1"))
	float OutlineWidth = 5.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TunaSweeper|Combat Pattern|Warning", meta = (ClampMin = "0.1"))
	float GroundClearance = 3.0f;

private:
	void BuildBoundary();
	void UpdateFill();
	void ApplyMaterial();
	void SampleGround();
	void ConformToGround(TArray<FVector>& Vertices, float Elevation) const;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> DynamicMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> DynamicAccentMaterial;

	bool bCircle = true;
	float Extent = 1.0f;
	FVector LaneEndLocal = FVector::ZeroVector;
	float WarningDuration = 1.0f;
	float Progress = 0.0f;
	FVector2D GroundGridMin = FVector2D::ZeroVector;
	FVector2D GroundGridStep = FVector2D(1.0f, 1.0f);
	FIntPoint GroundGridCells = FIntPoint(1, 1);
	TArray<float> GroundHeights;
};
