#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SSTOLaunchCinematic.generated.h"

class ATunaSweeperPlayerController;
class UCameraComponent;
class UCameraModifier;
class UDirectionalLightComponent;
class UExponentialHeightFogComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UPointLightComponent;
class UStaticMesh;
class UStaticMeshComponent;
class UTexture2D;
class UTunaSweeperScreenFadeWidget;
class UUserWidget;
enum class ESlateVisibility : uint8;

/** Local, real-time presentation of the placed spacecraft. The caller owns pause, HUD, input and the entry/exit fades. */
UCLASS(Transient, NotPlaceable)
class TUNASWEEPER_API ASSTOLaunchCinematic : public AActor
{
    GENERATED_BODY()

public:
    ASSTOLaunchCinematic();
    bool Start(ATunaSweeperPlayerController* Player, AActor* Lander);
    void Stop();
    float GetElapsedSeconds() const { return Elapsed; }
    int32 GetShotIndex() const { return ShotIndex; }

    /** Called once after the orbital hold. The camera stays on the orbit frame until Stop. */
    FSimpleDelegate OnOrbitFinished;
    FSimpleDelegate OnInterrupted;

    virtual void Tick(float DeltaSeconds) override;
    virtual void CalcCamera(float DeltaTime, FMinimalViewInfo& OutResult) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;

private:
    struct FGear
    {
        TWeakObjectPtr<UStaticMeshComponent> Component;
        FTransform Rest;
        FVector Mount;
    };
    struct FEffectCard
    {
        TWeakObjectPtr<UStaticMeshComponent> Component;
        TWeakObjectPtr<UMaterialInstanceDynamic> Material;
    };
    struct FNozzle
    {
        FVector Socket;
        double Radius;
        FVector GroundPoint;
        FVector GroundNormal;
        bool bAxial;
        float Seed;
        FEffectCard Core;
        TArray<FEffectCard> Dust;
    };
    struct FScreenWidget
    {
        TWeakObjectPtr<UUserWidget> Widget;
        ESlateVisibility Visibility;
    };

    UStaticMeshComponent* MakeMesh(UStaticMesh* Mesh, USceneComponent* Parent, bool bCastsShadow);
    FEffectCard MakeEffectCard(UStaticMesh* Plane, UMaterialInterface* Master, float Seed, int32 Priority);
    bool BuildStage(AActor* Lander);
    void HideGameplay(AActor* Lander);
    void SetUpLighting();
    void CutTo(int32 NewShot);
    void SetBoardingPose(float Time);
    void SetFlightPose(float Time);
    void DrawNozzles(float Time);
    void DrawGroundDust(FNozzle& Jet, float Time);
    void DrawVapor(float ShotFraction);
    FVector StagePosition(const FVector& LevelLocal) const;

    UPROPERTY() TObjectPtr<USceneComponent> Airframe;
    UPROPERTY() TObjectPtr<UCameraComponent> Lens;
    UPROPERTY(Transient) TArray<TObjectPtr<UActorComponent>> BuiltComponents;
    UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> Stair;
    UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> AftRail;
    UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> ForeRail;
    UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> Hatch;
    UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> Backdrop;
    UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> BackdropMaterial;
    UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> DockStrip;
    UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> DockMaterial;
    UPROPERTY(Transient) TObjectPtr<UPointLightComponent> CabinLight;
    UPROPERTY(Transient) TObjectPtr<UTexture2D> CloudPlate;
    UPROPERTY(Transient) TObjectPtr<UTexture2D> OrbitPlate;
    UPROPERTY(Transient) TObjectPtr<UTunaSweeperScreenFadeWidget> AltitudeFade;

    UPROPERTY(EditDefaultsOnly, Category="Ending|Art") TSoftObjectPtr<UStaticMesh> HatchAsset;
    UPROPERTY(EditDefaultsOnly, Category="Ending|Art") TSoftObjectPtr<UTexture2D> CloudAsset;
    UPROPERTY(EditDefaultsOnly, Category="Ending|Art") TSoftObjectPtr<UTexture2D> OrbitAsset;
    UPROPERTY(EditDefaultsOnly, Category="Ending|Art") TSoftObjectPtr<UMaterialInterface> BackdropAsset;
    UPROPERTY(EditDefaultsOnly, Category="Ending|Art") TSoftObjectPtr<UMaterialInterface> VaporAsset;
    UPROPERTY(EditDefaultsOnly, Category="Ending|Art") TSoftObjectPtr<UMaterialInterface> FlameAsset;
    UPROPERTY(EditDefaultsOnly, Category="Ending|Art") TSoftObjectPtr<UMaterialInterface> DustAsset;

    TWeakObjectPtr<ATunaSweeperPlayerController> Controller;
    TWeakObjectPtr<AActor> PreviousView;
    TWeakObjectPtr<UCameraModifier> SuspendedShake;
    TArray<TWeakObjectPtr<AActor>> HiddenForView;
    TArray<TWeakObjectPtr<UDirectionalLightComponent>> HiddenSuns;
    TArray<TWeakObjectPtr<UExponentialHeightFogComponent>> GroundFog;
    TArray<FScreenWidget> HiddenWidgets;
    TArray<FGear> Gear;
    TArray<FEffectCard> Plume;
    TArray<FNozzle> Nozzles;
    FTransform ParkedPose;
    FTransform LevelFrame;
    FTransform StairRest;
    FQuat CardRotation = FQuat::Identity;
    FName PreviousCameraStyle;
    double StartedAt = 0.0;
    float Elapsed = 0.0f;
    int32 ShotIndex = INDEX_NONE;
    bool bCameraOwned = false;
    bool bPreviousFullTick = false;
    bool bPreviousControllerPauseTick = false;
    bool bPreviousCameraMoveable = false;
    bool bOrbitReported = false;
    bool bSingleLocalWorld = false;
};
