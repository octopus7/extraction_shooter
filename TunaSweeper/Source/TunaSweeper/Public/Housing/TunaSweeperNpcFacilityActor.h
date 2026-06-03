#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Housing/TunaSweeperHousingTypes.h"
#include "TunaSweeperNpcFacilityActor.generated.h"

class ATunaSweeperFacilityNpcActor;
class USceneComponent;
class UStaticMesh;
class UStaticMeshComponent;

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperNpcFacilityActor : public AActor
{
	GENERATED_BODY()

public:
	ATunaSweeperNpcFacilityActor();

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Housing")
	void ConfigureNpcFacility(
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
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void ConfigureNpcFacilityDefaults(
		TSubclassOf<ATunaSweeperFacilityNpcActor> InNpcClass,
		FVector InNpcRelativeLocation,
		FRotator InNpcRelativeRotation);

	void RefreshNpcForHousingState();
	void DestroySpawnedNpc();
	void ApplyPreviewVisualState(bool bPreview, bool bPlacementValid);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> BaseMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> ConsoleMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> AccentMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Facility NPC")
	TSubclassOf<ATunaSweeperFacilityNpcActor> NpcClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Facility NPC")
	FVector NpcRelativeLocation = FVector(75.0f, 0.0f, 95.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Facility NPC")
	FRotator NpcRelativeRotation = FRotator::ZeroRotator;

private:
	bool IsHousingModeOpen() const;
	FTransform BuildNpcWorldTransform() const;

	UPROPERTY(Transient)
	FGuid InstanceId;

	UPROPERTY(Transient)
	FName FacilityId = NAME_None;

	UPROPERTY(Transient)
	TObjectPtr<ATunaSweeperFacilityNpcActor> SpawnedNpc;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> DefaultCubeMesh;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> DefaultCylinderMesh;

	bool bIsPreview = false;
};

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperSignalControlFacilityActor : public ATunaSweeperNpcFacilityActor
{
	GENERATED_BODY()

public:
	ATunaSweeperSignalControlFacilityActor();
};

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperSupplyFacilityActor : public ATunaSweeperNpcFacilityActor
{
	GENERATED_BODY()

public:
	ATunaSweeperSupplyFacilityActor();
};
