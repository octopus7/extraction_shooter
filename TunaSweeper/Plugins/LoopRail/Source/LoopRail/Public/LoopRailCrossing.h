#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LoopRailCrossing.generated.h"
class ALoopRailTrack;
class USceneComponent;
class UStaticMeshComponent;
class UPointLightComponent;
class UAudioComponent;
class USoundBase;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLoopRailCrossingEvent, bool, bWarning);

UCLASS(Blueprintable)
class LOOPRAIL_API ALoopRailCrossing : public AActor
{
    GENERATED_BODY()
public:
    ALoopRailCrossing();
    virtual void OnConstruction(const FTransform& Transform) override;
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category="LoopRail") TObjectPtr<ALoopRailTrack> Track;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LoopRail", meta=(ClampMin="0", Units="cm")) float WarningDistance = 1800;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LoopRail", meta=(ClampMin="0", Units="s")) float WarningLeadSeconds = 3;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LoopRail", meta=(ClampMin="0.1", Units="s")) float BarrierSeconds = 2;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LoopRail", meta=(ClampMin="0", Units="cm")) float RearClearance = 350;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LoopRail") TObjectPtr<USoundBase> WarningSound;
    UPROPERTY(BlueprintAssignable, Category="LoopRail") FLoopRailCrossingEvent OnWarningChanged;
    UFUNCTION(BlueprintPure, Category="LoopRail") bool IsWarningActive() const { return bWarning; }
    UFUNCTION(BlueprintPure, Category="LoopRail") float GetBarrierOpenAlpha() const { return OpenAlpha; }
    UFUNCTION(BlueprintCallable, CallInEditor, Category="LoopRail") void AlignToTrack();
private:
    UPROPERTY() TObjectPtr<USceneComponent> Root;
    UPROPERTY() TArray<TObjectPtr<USceneComponent>> Hinges;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Barriers;
    UPROPERTY() TArray<TObjectPtr<UPointLightComponent>> Lamps;
    UPROPERTY() TObjectPtr<UAudioComponent> Audio;
    bool bWarning = false;
    float OpenAlpha = 1;
    void UpdatePresentation();
};
