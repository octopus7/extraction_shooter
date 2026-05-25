#pragma once

#include "CoreMinimal.h"
#include "Interaction/TunaSweeperInteractableActor.h"
#include "TunaSweeperWarpPointActor.generated.h"

class APawn;
class UMaterialInterface;
class UStaticMesh;
class UTunaSweeperInteractionMarkerWidget;

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperWarpPointActor : public ATunaSweeperInteractableActor
{
	GENERATED_BODY()

public:
	ATunaSweeperWarpPointActor();

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Warp Point")
	FName GetWarpPointId() const { return WarpPointId; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Warp Point")
	FName GetTargetWarpPointId() const { return TargetWarpPointId; }

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Warp Point")
	void ConfigureWarpPointDefaults(
		FName InWarpPointId,
		FName InTargetWarpPointId,
		const FText& InInteractionDisplayName,
		TSoftClassPtr<UTunaSweeperInteractionMarkerWidget> InMarkerWidgetClass,
		TSoftObjectPtr<UMaterialInterface> InVisualMaterial,
		TSoftObjectPtr<UStaticMesh> InVisualMesh,
		FVector InVisualScale,
		FVector InVisualRelativeLocation,
		FVector InExitOffset,
		bool bInUseTargetRotation);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Warp Point")
	bool WarpInstigator(APawn* InstigatorPawn);

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

private:
	void RefreshWarpPointVisual();
	ATunaSweeperWarpPointActor* ResolveTargetWarpPoint() const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Warp Point", meta = (AllowPrivateAccess = "true"))
	FName WarpPointId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Warp Point", meta = (AllowPrivateAccess = "true"))
	FName TargetWarpPointId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Warp Point", meta = (AllowPrivateAccess = "true"))
	FVector ExitOffset = FVector(160.0f, 0.0f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Warp Point", meta = (AllowPrivateAccess = "true"))
	bool bUseTargetRotation = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Warp Point|Visual", meta = (AllowPrivateAccess = "true"))
	TSoftObjectPtr<UStaticMesh> VisualMeshOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Warp Point|Visual", meta = (AllowPrivateAccess = "true"))
	TSoftObjectPtr<UMaterialInterface> VisualMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Warp Point|Visual", meta = (AllowPrivateAccess = "true"))
	FVector WarpPointVisualScale = FVector(1.2f, 1.2f, 1.2f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Warp Point|Visual", meta = (AllowPrivateAccess = "true"))
	FVector WarpPointVisualRelativeLocation = FVector::ZeroVector;
};
