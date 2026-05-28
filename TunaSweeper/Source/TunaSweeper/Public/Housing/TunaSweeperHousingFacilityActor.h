#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Housing/TunaSweeperHousingTypes.h"
#include "TunaSweeperHousingFacilityActor.generated.h"

class UMaterialInterface;
class UStaticMesh;
class UStaticMeshComponent;

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperHousingFacilityActor : public AActor
{
	GENERATED_BODY()

public:
	ATunaSweeperHousingFacilityActor();

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Housing")
	void ConfigureFacilityVisual(
		const FTunaSweeperHousingFacilityDefinition& Definition,
		const FTunaSweeperHousingPlacedFacilitySaveData& Placement,
		const FTransform& WorldTransform,
		bool bPreview,
		bool bPlacementValid);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Housing")
	FGuid GetInstanceId() const { return InstanceId; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Housing")
	FName GetFacilityId() const { return FacilityId; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> FacilityMesh;

private:
	void ApplyMeshAndMaterial(const FTunaSweeperHousingFacilityDefinition& Definition);

	UPROPERTY(Transient)
	FGuid InstanceId;

	UPROPERTY(Transient)
	FName FacilityId = NAME_None;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> DefaultCubeMesh;
};
