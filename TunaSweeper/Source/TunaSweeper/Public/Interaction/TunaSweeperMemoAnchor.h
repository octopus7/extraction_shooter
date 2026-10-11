#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TunaSweeperMemoAnchor.generated.h"

class UStaticMeshComponent;
class UTunaSweeperInteractableComponent;
class UTunaSweeperGameInstance;

/** Directly placed, save-slot-owned collectible. MemoId is its only authored identity. */
UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperMemoAnchor : public AActor
{
    GENERATED_BODY()
public:
    ATunaSweeperMemoAnchor();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UFUNCTION(BlueprintPure, Category = "TunaSweeper|Memo")
    int32 GetMemoId() const { return MemoId; }

    UFUNCTION(BlueprintPure, Category = "TunaSweeper|Memo")
    bool CanCollectMemo() const;

    /** Commits collection before hiding the actor; a failed save leaves it collectible. */
    UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Memo")
    bool CollectMemo();

    void RefreshMemoState();

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UStaticMeshComponent> VisualMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UTunaSweeperInteractableComponent> InteractableComponent;

    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Memo", meta = (ClampMin = "1", UIMin = "1"))
    int32 MemoId = 0;

private:
    TWeakObjectPtr<UTunaSweeperGameInstance> BoundGameInstance;
    FDelegateHandle MemoStateChangedHandle;
};
