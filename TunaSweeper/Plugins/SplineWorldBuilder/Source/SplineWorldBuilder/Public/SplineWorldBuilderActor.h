#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SplineWorldBuilderActor.generated.h"

class ASplineWorldJunctionActor;
class UHierarchicalInstancedStaticMeshComponent;
class USceneComponent;
class USplineComponent;
class USplineWorldBuilderProfile;
class UStaticMesh;

/** A single non-branching spline chain. Branches are represented by junction actors. */
UCLASS(BlueprintType, Blueprintable)
class SPLINEWORLDBUILDER_API ASplineWorldBuilderActor : public AActor
{
	GENERATED_BODY()

public:
	ASplineWorldBuilderActor();

	virtual void OnConstruction(const FTransform& Transform) override;

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Spline World Builder")
	void RebuildGenerated();

	UFUNCTION(BlueprintPure, Category = "Spline World Builder")
	USplineComponent* GetBuilderSpline() const { return Spline; }

	UFUNCTION(BlueprintPure, Category = "Spline World Builder")
	int32 GetStraightInstanceCount() const;

	UFUNCTION(BlueprintPure, Category = "Spline World Builder")
	int32 GetCornerInstanceCount() const;

	UFUNCTION(BlueprintPure, Category = "Spline World Builder")
	int32 GetEndInstanceCount() const;

	bool GetStraightInstanceTransform(int32 InstanceIndex, FTransform& OutTransform, bool bWorldSpace = true) const;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spline World Builder")
	TObjectPtr<USplineWorldBuilderProfile> Profile;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Spline World Builder|Connections")
	TObjectPtr<ASplineWorldJunctionActor> StartJunction;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Spline World Builder|Connections")
	TObjectPtr<ASplineWorldJunctionActor> EndJunction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spline World Builder")
	bool bAutoRebuild = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spline World Builder")
	int32 Seed = 12345;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spline World Builder")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spline World Builder")
	TObjectPtr<USplineComponent> Spline;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spline World Builder|Generated")
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> StraightInstances;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spline World Builder|Generated")
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> CornerInstances;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spline World Builder|Generated")
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> EndInstances;

private:
	bool IsCornerPoint(int32 PointIndex) const;
	void AddCornerInstance(int32 PointIndex);
	void AddEndInstance(bool bAtStart);
	double GetTrimAtPoint(int32 PointIndex) const;
	void ConfigureGeneratedComponent(UHierarchicalInstancedStaticMeshComponent* Component, UStaticMesh* Mesh) const;
};
