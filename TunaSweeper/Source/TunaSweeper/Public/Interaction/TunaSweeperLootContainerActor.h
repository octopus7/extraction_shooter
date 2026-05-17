#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Inventory/TunaSweeperInventoryTypes.h"
#include "Subsystem/TunaSweeperItemDataSubsystem.h"
#include "TunaSweeperLootContainerActor.generated.h"

class UTunaSweeperInteractableComponent;
class UTunaSweeperGameInstance;
class USceneComponent;
class UStaticMeshComponent;

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperLootContainerActor : public AActor
{
	GENERATED_BODY()

public:
	ATunaSweeperLootContainerActor();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Loot Container")
	void SetContainerDataIds(int32 InContainerDefinitionId, int32 InContentsId);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Loot Container")
	int32 GetContainerDefinitionId() const { return ContainerDefinitionId; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Loot Container")
	int32 GetContentsId() const { return ContentsId; }

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Loot Container")
	bool BuildContainerInstance(FTunaSweeperLootContainerInstance& OutInstance) const;

	bool OpenRuntimeContainer(UTunaSweeperGameInstance* TunaGameInstance, FTunaSweeperLootContainerInstance& OutInstance);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Interaction")
	UTunaSweeperInteractableComponent* GetInteractableComponent() const { return InteractableComponent; }

	void ConfigureLootContainerDefaults(int32 InContainerDefinitionId, int32 InContentsId);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> VisualMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UTunaSweeperInteractableComponent> InteractableComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot Container")
	int32 ContainerDefinitionId = 7001;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot Container")
	int32 ContentsId = 8001;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot Container")
	ETunaSweeperItemTextLanguage DisplayLanguage = ETunaSweeperItemTextLanguage::Korean;

private:
	void RefreshContainerPresentation();
	void ResetRuntimeContainerState();
	void CaptureRuntimeContentsFromActiveContainer();
	bool IsRuntimeContainerStateValid(const UTunaSweeperGameInstance* TunaGameInstance) const;
	FTunaSweeperLootContainerInstance BuildRuntimeContainerInstance() const;
	UTunaSweeperItemDataSubsystem* GetItemDataSubsystem() const;

	UPROPERTY(Transient)
	TWeakObjectPtr<UTunaSweeperGameInstance> RuntimeGameInstance;

	UPROPERTY(Transient)
	TArray<FTunaSweeperInventorySlot> RuntimeSlots;

	UPROPERTY(Transient)
	FText RuntimeDisplayName;

	UPROPERTY(Transient)
	int32 RuntimeCapacity = 0;

	UPROPERTY(Transient)
	int32 RuntimeContainerDefinitionId = INDEX_NONE;

	UPROPERTY(Transient)
	int32 RuntimeContentsId = INDEX_NONE;

	UPROPERTY(Transient)
	bool bHasRuntimeContainerState = false;
};
