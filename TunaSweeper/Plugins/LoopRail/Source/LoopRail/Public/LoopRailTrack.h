#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LoopRailTrack.generated.h"
class USplineComponent;
class UInstancedStaticMeshComponent;

UCLASS(Blueprintable)
class LOOPRAIL_API ALoopRailTrack : public AActor
{
    GENERATED_BODY()
public:
    ALoopRailTrack();
    virtual void OnConstruction(const FTransform& Transform) override;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LoopRail") TObjectPtr<USplineComponent> Spline;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LoopRail") TObjectPtr<UInstancedStaticMeshComponent> Rails;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LoopRail") TObjectPtr<UInstancedStaticMeshComponent> Sleepers;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LoopRail", meta=(ClampMin="50", Units="cm")) float Gauge = 150;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LoopRail", meta=(ClampMin="25", Units="cm")) float SleeperSpacing = 75;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LoopRail", meta=(ClampMin="25", Units="cm")) float RailSegmentLength = 100;
    UFUNCTION(BlueprintCallable, CallInEditor, Category="LoopRail") void RebuildTrack();
    UFUNCTION(BlueprintPure, Category="LoopRail") double GetLength() const;
    UFUNCTION(BlueprintPure, Category="LoopRail") bool IsUsableLoop() const;
    UFUNCTION(BlueprintPure, Category="LoopRail") double FindDistance(const FVector& Location) const;
    UFUNCTION(BlueprintPure, Category="LoopRail") FTransform Sample(double Distance) const;
};
