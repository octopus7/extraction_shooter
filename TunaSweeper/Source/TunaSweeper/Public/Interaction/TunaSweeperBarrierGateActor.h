#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TunaSweeperBarrierGateActor.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class UStaticMesh;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UPointLightComponent;

UENUM(BlueprintType)
enum class ETunaSweeperBarrierGateState : uint8 { Closed, Opening, Open, Closing };

/** Unmanned boom barrier. Local +X runs along the arm; traffic crosses local Y. */
UCLASS(Blueprintable)
class TUNASWEEPER_API ATunaSweeperBarrierGateActor : public AActor
{
	GENERATED_BODY()
public:
	ATunaSweeperBarrierGateActor();
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable, Category="Barrier Gate") void OpenGate();
	/** Manual close also respects the player safety zone. */
	UFUNCTION(BlueprintCallable, Category="Barrier Gate") void CloseGate();
	UFUNCTION(BlueprintCallable, Category="Barrier Gate") void RefreshProximity();
	UFUNCTION(BlueprintPure, Category="Barrier Gate") float GetOpenAlpha() const { return OpenAlpha; }
	UFUNCTION(BlueprintPure, Category="Barrier Gate") ETunaSweeperBarrierGateState GetGateState() const;
	UFUNCTION(BlueprintPure, Category="Barrier Gate") bool IsPassageClear() const { return OpenAlpha >= 1.0f; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Barrier Gate|Assets") TObjectPtr<UStaticMesh> HousingMesh;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Barrier Gate|Assets") TObjectPtr<UStaticMesh> ArmMesh;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Barrier Gate|Assets") TObjectPtr<UStaticMesh> RedLensMesh;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Barrier Gate|Assets") TObjectPtr<UStaticMesh> GreenLensMesh;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Barrier Gate|Assets") TObjectPtr<UMaterialInterface> IndicatorMaterial;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Barrier Gate|Motion", meta=(ClampMin="10", ClampMax="90", Units="deg")) float OpenAngle = 85;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Barrier Gate|Motion", meta=(ClampMin="0.05", Units="s")) float OpenDuration = 1.2f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Barrier Gate|Motion", meta=(ClampMin="0.05", Units="s")) float CloseDuration = 1.8f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Barrier Gate|Proximity", meta=(ClampMin="0", Units="s")) float AutoCloseDelay = 1.5f;
	/** Half-depth on BOTH sides of the lane. Minimum protects the arm's sweep. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Barrier Gate|Proximity", meta=(ClampMin="100", Units="cm")) float DetectionDistance = 300;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Barrier Gate|Proximity") bool bAutoOpen = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Barrier Gate|Motion") bool bStartsOpen = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Barrier Gate|Lights", meta=(ClampMin="0")) float LEDIntensity = 8;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Barrier Gate|Components") TObjectPtr<UStaticMeshComponent> Housing;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Barrier Gate|Components") TObjectPtr<USceneComponent> ArmPivot;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Barrier Gate|Components") TObjectPtr<UStaticMeshComponent> Arm;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Barrier Gate|Components") TObjectPtr<UBoxComponent> ArmCollision;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Barrier Gate|Components") TObjectPtr<UBoxComponent> DetectionZone;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Barrier Gate|Components") TObjectPtr<UStaticMeshComponent> RedLens;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Barrier Gate|Components") TObjectPtr<UStaticMeshComponent> GreenLens;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Barrier Gate|Components") TObjectPtr<UPointLightComponent> RedLight;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Barrier Gate|Components") TObjectPtr<UPointLightComponent> GreenLight;

private:
	bool HasNearbyPlayer() const;
	void ApplyPose();
	void Configure();
	void CreateIndicatorMaterials();
	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> RedMID;
	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> GreenMID;
	float OpenAlpha = 0;
	bool bTargetOpen = false;
	FTimerHandle ProximityTimer;
	FTimerHandle CloseTimer;
};
