#pragma once
#include "CoreMinimal.h"
#include "Interaction/TunaSweeperLevelTravelInteractableActor.h"
#include "Inventory/TunaSweeperSaveGame.h"
#include "TunaSweeperRegionLinkActor.generated.h"
class UBoxComponent;

/** A saved passage or repairable lift. The destination is selected through the shared travel resolver. */
UCLASS(Blueprintable)
class TUNASWEEPER_API ATunaSweeperRegionLinkActor : public ATunaSweeperLevelTravelInteractableActor
{
    GENERATED_BODY()
public:
    ATunaSweeperRegionLinkActor();
    virtual void OnConstruction(const FTransform& Transform) override;
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void Tick(float DeltaSeconds) override;
    virtual bool TravelToTargetLevel(APawn* Pawn) override;
    UFUNCTION(BlueprintPure) bool IsPassageUnlocked() const;
    UFUNCTION(BlueprintCallable) bool RepairPassage(bool bSaveImmediately = true);
    UFUNCTION(BlueprintCallable) void RefreshPassage();
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Passage") FName RequiredProgressId;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Passage") TArray<FTunaSweeperItemStack> RepairParts;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Passage") bool bWalkThrough = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Passage") FName TravelStringKey = TEXT("ui.interaction.travel");
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Passage") TObjectPtr<UBoxComponent> WalkArea;
private:
    bool bTravelStarted = false;
    bool bRepairing = false;
};
