#pragma once

#include "Interaction/TunaSweeperLootContainerActor.h"
#include "TunaSweeperSupplyDropActor.generated.h"

class UAudioComponent;
class UTunaSweeperSupplyDropDataAsset;

UCLASS(NotPlaceable, NotBlueprintable)
class TUNASWEEPER_API ATunaSweeperSupplyDropActor : public ATunaSweeperLootContainerActor
{
	GENERATED_BODY()

public:
	ATunaSweeperSupplyDropActor();

	void ConfigureDrop(const UTunaSweeperSupplyDropDataAsset& Assets, int32 DefinitionId, int32 InContentsId,
		const FVector& InLandingLocation);
	virtual bool OpenRuntimeContainer(UTunaSweeperGameInstance* Game, FTunaSweeperLootContainerInstance& OutInstance) override;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void RefreshInteractionPrompt() override;

private:
	bool IsLandingOccupied() const;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Parachute;

	UPROPERTY(Transient)
	TObjectPtr<const UTunaSweeperSupplyDropDataAsset> DropAssets;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> TransportAudio;

	FVector ReleaseLocation = FVector::ZeroVector;
	FVector LandingLocation = FVector::ZeroVector;
	float LandingYaw = 0.0f;
	float Elapsed = 0.0f;
	bool bLanded = false;
	bool bReadyToLoot = false;
};
