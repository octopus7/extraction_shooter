#pragma once

#include "CoreMinimal.h"
#include "Interaction/TunaSweeperInteractableActor.h"
#include "TunaSweeperResearchStationActor.generated.h"

class UTunaSweeperInteractionMarkerWidget;
class UStaticMesh;
class UStaticMeshComponent;

/** Bathroom diagnostic mirror; opens research through interaction only. */
UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperResearchStationActor : public ATunaSweeperInteractableActor
{
	GENERATED_BODY()

public:
	ATunaSweeperResearchStationActor();
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;

	void ConfigureResearchStationDefaults(
		const FText& InInteractionDisplayName,
		TSoftClassPtr<UTunaSweeperInteractionMarkerWidget> InMarkerWidgetClass);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Research Station")
	TObjectPtr<UStaticMeshComponent> ScanBarMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Research Station")
	TObjectPtr<UStaticMesh> StationBodyMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Research Station")
	TObjectPtr<UStaticMesh> StationScanBarMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Research Station|Scan")
	FVector ScanOrigin = FVector(8.3f, 0.0f, 48.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Research Station|Scan", meta = (ClampMin = "0", Units = "cm"))
	float ScanTravel = 28.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Research Station|Scan", meta = (ClampMin = "0.1", Units = "s"))
	float ScanCycleSeconds = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Research Station|Scan")
	bool bAnimateScan = true;

private:
	void ApplyStationPresentation();
	double ScanElapsedSeconds = 0.0;
};
