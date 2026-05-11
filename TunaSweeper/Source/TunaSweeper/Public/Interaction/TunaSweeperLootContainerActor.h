#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Subsystem/TunaSweeperItemDataSubsystem.h"
#include "TunaSweeperLootContainerActor.generated.h"

class UTunaSweeperInteractableComponent;
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

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Loot Container")
	void SetContainerDataIds(int32 InContainerDefinitionId, int32 InContentsId);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Loot Container")
	int32 GetContainerDefinitionId() const { return ContainerDefinitionId; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Loot Container")
	int32 GetContentsId() const { return ContentsId; }

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Loot Container")
	bool BuildContainerInstance(FTunaSweeperLootContainerInstance& OutInstance) const;

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
	UTunaSweeperItemDataSubsystem* GetItemDataSubsystem() const;
};

