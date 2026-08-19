#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SplineWorldBuilderJunctionActor.generated.h"

class ASplineWorldBuilderActor;
class USceneComponent;
class USplineWorldBuilderProfile;
class UStaticMesh;
class UStaticMeshComponent;

UENUM(BlueprintType)
enum class ESplineWorldEndpoint : uint8
{
	Start,
	End
};

UENUM(BlueprintType)
enum class ESplineWorldJunctionType : uint8
{
	None,
	End,
	Straight,
	Corner,
	Tee,
	Cross,
	Unsupported
};

USTRUCT(BlueprintType)
struct SPLINEWORLDBUILDER_API FSplineWorldJunctionConnection
{
	GENERATED_BODY()

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Connection")
	TObjectPtr<ASplineWorldBuilderActor> Chain;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Connection")
	ESplineWorldEndpoint Endpoint = ESplineWorldEndpoint::Start;
};

/** Graph node joining one to four non-branching spline chains. */
UCLASS(BlueprintType, Blueprintable)
class SPLINEWORLDBUILDER_API ASplineWorldJunctionActor : public AActor
{
	GENERATED_BODY()

public:
	ASplineWorldJunctionActor();

	virtual void OnConstruction(const FTransform& Transform) override;

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Spline World Builder")
	void RebuildGenerated();

	UFUNCTION(BlueprintPure, Category = "Spline World Builder")
	ESplineWorldJunctionType GetResolvedJunctionType() const { return ResolvedJunctionType; }

	UFUNCTION(BlueprintPure, Category = "Spline World Builder")
	UStaticMesh* GetDisplayedJunctionMesh() const;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spline World Builder")
	TObjectPtr<USplineWorldBuilderProfile> Profile;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Spline World Builder|Connections")
	TArray<FSplineWorldJunctionConnection> Connections;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spline World Builder")
	bool bAutoRebuild = true;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spline World Builder")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spline World Builder|Generated")
	TObjectPtr<UStaticMeshComponent> JunctionMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spline World Builder|Generated")
	ESplineWorldJunctionType ResolvedJunctionType = ESplineWorldJunctionType::None;

private:
	bool GetConnectionDirection(const FSplineWorldJunctionConnection& Connection, FVector& OutDirection) const;
	FRotator ResolveRotation(ESplineWorldJunctionType Type, const TArray<FVector>& Directions) const;
};
