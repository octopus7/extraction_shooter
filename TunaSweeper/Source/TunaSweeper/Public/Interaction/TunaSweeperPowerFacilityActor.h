#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Subsystem/TunaSweeperItemDataSubsystem.h"
#include "TunaSweeperPowerFacilityActor.generated.h"

class UParticleSystemComponent;
class UStaticMeshComponent;
class UTunaSweeperInteractableComponent;
class UTunaSweeperGameInstance;

/** A persistent, single-repair facility. Completion keeps the facility and stops its fault effect. */
UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperPowerFacilityActor : public AActor
{
	GENERATED_BODY()
	friend class FTunaSweeperPowerFacilityMaterialsTest;
public:
	ATunaSweeperPowerFacilityActor();
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintPure, Category = "Power Facility")
	bool IsRepaired() const { return bRepaired; }
	UFUNCTION(BlueprintPure, Category = "Power Facility")
	bool CanRepair() const;
	// Normal player interactions report the configured quest event through InteractionSubsystem.
	UFUNCTION(BlueprintCallable, Category = "Power Facility")
	bool Repair(bool bSaveImmediately = true);
	UFUNCTION(BlueprintCallable, Category = "Power Facility")
	virtual void RefreshFacilityState();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> FacilityMesh;
	// Assign a looping particle system and edit its relative transform in the BP component tree.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UParticleSystemComponent> FaultSparks;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UTunaSweeperInteractableComponent> Interactable;

	// Required: stable and unique per placed facility. No actor-name fallback or shared demo key.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Power Facility|Persistence")
	FName ProgressObjectId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Power Facility|Quest")
	bool bRequireActiveQuest = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Power Facility|Quest", meta = (ClampMin = "0", EditCondition = "bRequireActiveQuest"))
	int32 RepairQuestId = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Power Facility|Quest", meta = (EditCondition = "bRequireActiveQuest"))
	FName RepairObjectiveId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Power Facility|Quest")
	FName RepairObjectiveEventId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Power Facility|Repair")
	bool bRequiresItem = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Power Facility|Repair", meta = (ClampMin = "0", EditCondition = "bRequiresItem"))
	int32 RequiredItemId = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Power Facility|Repair", meta = (ClampMin = "1", EditCondition = "bRequiresItem"))
	int32 RequiredItemQuantity = 1;
	// Nonempty arrays override the legacy item/quantity pair; duplicate item IDs are summed.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Power Facility|Repair", meta = (EditCondition = "bRequiresItem"))
	TArray<FTunaSweeperItemStack> RequiredItems;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Power Facility|Repair", meta = (EditCondition = "bRequiresItem"))
	bool bConsumeRequiredItem = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Power Facility|Presentation")
	FName InteractionStringKey = TEXT("ui.interaction.repair");
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Power Facility|Preview")
	bool bPreviewRepairedInEditor = false;

private:
	bool BuildRepairRequirements(TArray<FTunaSweeperItemStack>& OutRequirements) const;
	bool IsRepairInteractionAvailable() const;
	void ApplyPresentation();
	UTunaSweeperGameInstance* GetTunaGameInstance() const;
	UPROPERTY(Transient)
	bool bRepaired = false;
	bool bRepairInProgress = false;
};
