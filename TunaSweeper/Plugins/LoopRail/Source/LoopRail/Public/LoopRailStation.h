#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LoopRailStation.generated.h"
class ALoopRailTrack;
class USceneComponent;
class UArrowComponent;
class UStaticMeshComponent;
class UWidgetComponent;
class UCapsuleComponent;

UCLASS(Blueprintable)
class LOOPRAIL_API ALoopRailStation : public AActor
{
    GENERATED_BODY()
public:
    ALoopRailStation();
    virtual void OnConstruction(const FTransform& Transform) override;
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LoopRail") TObjectPtr<USceneComponent> Root;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LoopRail") TObjectPtr<UArrowComponent> StopMarker;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LoopRail") TObjectPtr<UStaticMeshComponent> Platform;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LoopRail") TObjectPtr<UStaticMeshComponent> AccessRamp;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LoopRail|Arrival") TObjectPtr<UWidgetComponent> ArrivalDisplay;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LoopRail|Arrival") TObjectPtr<USceneComponent> ArrivalMarkerPivot;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LoopRail|Arrival") TObjectPtr<UStaticMeshComponent> ArrivalMarkerBody;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LoopRail|Arrival") TObjectPtr<UCapsuleComponent> ArrivalMarkerCollision;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LoopRail|Arrival") bool bShowArrivalDisplay = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LoopRail|Arrival", meta=(ClampMin="120", Units="cm")) float ArrivalMarkerHeight = 220;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LoopRail|Arrival", meta=(ClampMin="40", Units="cm")) float ArrivalMarkerWidth = 100;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LoopRail|Arrival", meta=(ClampMin="10", Units="cm")) float ArrivalMarkerDepth = 25;
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
    UFUNCTION(BlueprintPure, Category="LoopRail|Arrival") bool TryGetNextArrivalSeconds(double& OutSeconds) const;
    // The host project supplies these through its localization/string-key workflow.
    UFUNCTION(BlueprintCallable, Category="LoopRail|Arrival") void SetArrivalTextFormats(const FText& TimeFormat, const FText& UnavailableText);
    UFUNCTION(BlueprintCallable, Category="LoopRail|Arrival") void RefreshArrivalDisplay();
    UFUNCTION(BlueprintPure, Category="LoopRail|Arrival") FText GetArrivalDisplayText() const;
private:
    void UpdateArrivalDisplayTransform();
    UPROPERTY(Transient) FText ArrivalTimeFormat;
    UPROPERTY(Transient) FText ArrivalUnavailableText;
    float DisplayRefreshRemaining = 0;
};
