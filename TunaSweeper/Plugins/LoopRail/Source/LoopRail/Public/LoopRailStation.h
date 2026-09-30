#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LoopRailStation.generated.h"
class ALoopRailTrack;
class USceneComponent;
class UArrowComponent;
class UStaticMeshComponent;

UCLASS(Blueprintable)
class LOOPRAIL_API ALoopRailStation : public AActor
{
    GENERATED_BODY()
public:
    ALoopRailStation();
    virtual void OnConstruction(const FTransform& Transform) override;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LoopRail") TObjectPtr<USceneComponent> Root;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LoopRail") TObjectPtr<UArrowComponent> StopMarker;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LoopRail") TObjectPtr<UStaticMeshComponent> Platform;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LoopRail") TObjectPtr<UStaticMeshComponent> AccessRamp;
    UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category="LoopRail") TObjectPtr<ALoopRailTrack> Track;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LoopRail") FName StationId;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LoopRail") bool bEnabled = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LoopRail", meta=(ClampMin="0", Units="s")) float DwellSeconds = 8;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LoopRail") bool bShowPlatform = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LoopRail", meta=(ClampMin="200", Units="cm")) float PlatformLength = 3800;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LoopRail", meta=(ClampMin="100", Units="cm")) float PlatformWidth = 250;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LoopRail") bool bPlatformOnRight = true;
    UFUNCTION(BlueprintCallable, CallInEditor, Category="LoopRail") void AlignToTrack();
    UFUNCTION(BlueprintPure, Category="LoopRail") double GetStopDistance() const;
};
