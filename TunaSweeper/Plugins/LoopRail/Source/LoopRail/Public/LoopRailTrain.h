#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LoopRailTrain.generated.h"
class ALoopRailTrack;
class ALoopRailStation;
class UBoxComponent;
class UStaticMeshComponent;
class UStaticMesh;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLoopRailStationEvent, ALoopRailStation*, Station);

USTRUCT()
struct FLoopRailRepState
{
    GENERATED_BODY()
    UPROPERTY() double Distance = 0;
    UPROPERTY() float Speed = 0;
    UPROPERTY() float ServerTime = 0;
    UPROPERTY() bool bMoving = false;
    UPROPERTY() float DwellSeconds = 0;
};

UCLASS(Blueprintable)
class LOOPRAIL_API ALoopRailTrain : public AActor
{
    GENERATED_BODY()
public:
    ALoopRailTrain();
    virtual void OnConstruction(const FTransform& Transform) override;
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    UPROPERTY(EditInstanceOnly, BlueprintReadWrite, ReplicatedUsing=OnRep_Configuration, Category="LoopRail") TObjectPtr<ALoopRailTrack> Track;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_Configuration, Category="LoopRail", meta=(ClampMin="1", ClampMax="16")) int32 VehicleCount = 4;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_Configuration, Category="LoopRail", meta=(ClampMin="0", Units="cm")) float VehicleGap = 80;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LoopRail", meta=(Units="cm")) double StartDistance = 4500;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LoopRail", meta=(ClampMin="0", Units="cm/s")) float CruiseSpeed = 600;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LoopRail", meta=(ClampMin="1")) float Acceleration = 120;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LoopRail", meta=(ClampMin="1")) float BrakingDeceleration = 180;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LoopRail") bool bAutoRun = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LoopRail") TSoftObjectPtr<UStaticMesh> LocomotiveMesh;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LoopRail") TSoftObjectPtr<UStaticMesh> CarriageMesh;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LoopRail") TSoftObjectPtr<UStaticMesh> CarriageRoofMesh;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LoopRail") TSoftObjectPtr<UStaticMesh> ConnectionMesh;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LoopRail") bool bShowCarriageRoofs = false;
    UPROPERTY(BlueprintAssignable, Category="LoopRail") FLoopRailStationEvent OnStationArrived;
    UPROPERTY(BlueprintAssignable, Category="LoopRail") FLoopRailStationEvent OnStationDeparted;
    UFUNCTION(BlueprintCallable, CallInEditor, Category="LoopRail") void RebuildTrain();
    UFUNCTION(BlueprintCallable, Category="LoopRail") void ResetTrain();
    UFUNCTION(BlueprintCallable, Category="LoopRail") void SetRunning(bool bEnabled);
    UFUNCTION(BlueprintPure, Category="LoopRail") double GetHeadDistance() const { return HeadDistance; }
    UFUNCTION(BlueprintPure, Category="LoopRail") float GetSpeed() const { return CurrentSpeed; }
    UFUNCTION(BlueprintPure, Category="LoopRail") bool IsDwelling() const { return DwellRemaining > 0; }
    UFUNCTION(BlueprintPure, Category="LoopRail") double GetConsistLength() const;
    UFUNCTION(BlueprintPure, Category="LoopRail") bool HasUsableTrack() const;
    UFUNCTION(BlueprintPure, Category="LoopRail") UBoxComponent* GetVehicleFloor(int32 Index) const;
    UFUNCTION(BlueprintPure, Category="LoopRail") UBoxComponent* GetConnectionFloor(int32 Index) const;
    static constexpr double VehicleLength = 900;
    static constexpr double VehicleWidth = 220;
    static constexpr double FloorHeight = 60;
private:
    UPROPERTY(Transient) TArray<TObjectPtr<UBoxComponent>> Floors;
    UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> Visuals;
    UPROPERTY(Transient) TArray<TObjectPtr<UBoxComponent>> ConnectionFloors;
    UPROPERTY(Transient) TArray<TObjectPtr<UBoxComponent>> ConnectionGuards;
    UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> ConnectionVisuals;
    UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> ConnectionGuardVisuals;
    UPROPERTY(Transient) TObjectPtr<ALoopRailStation> CurrentStation;
    UPROPERTY(ReplicatedUsing=OnRep_State) FLoopRailRepState RepState;
    UFUNCTION() void OnRep_State();
    UFUNCTION() void OnRep_Configuration();
    void UpdateVehicles();
    void UpdateConnections();
    void PublishState();
    ALoopRailStation* FindNextStation(double& Distance) const;
    double HeadDistance = 0;
    float CurrentSpeed = 0;
    float DwellRemaining = 0;
    double SinceDeparture = 100;
    bool bRunning = false;
    bool bUsesFallbackConnectionMesh = false;
};
