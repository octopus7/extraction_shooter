#pragma once

#include "CoreMinimal.h"
#include "Interaction/TunaSweeperInteractableActor.h"
#include "TunaSweeperChainLinkGate.generated.h"

class UBoxComponent;

UCLASS(Blueprintable)
class TUNASWEEPER_API ATunaSweeperChainLinkGate : public ATunaSweeperInteractableActor
{
	GENERATED_BODY()

public:
	ATunaSweeperChainLinkGate();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	bool IsUnlocked() const { return bUnlocked; }
	bool InspectLock(APawn* InstigatorPawn);
	bool Unlock();
	FName GetPersistentUnlockFlag() const { return GetUnlockFlag(); }
	FName GetGateId() const { return GateId; }
	bool TryResetPersistently();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gate")
	TObjectPtr<USceneComponent> Hinge;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gate")
	TObjectPtr<UBoxComponent> GateCollision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gate")
	TObjectPtr<UStaticMeshComponent> HingePost;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gate")
	TObjectPtr<UStaticMeshComponent> LatchPost;

	// Assign a unique, stable ID per level before linking the lock control.
	UPROPERTY(EditInstanceOnly, Category = "Gate")
	FName GateId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gate", meta = (ClampMin = "-180", ClampMax = "180", Units = "deg"))
	float OpenAngle = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gate", meta = (ClampMin = "0.05", Units = "s"))
	float OpeningDuration = 1.0f;

private:
	FName GetUnlockFlag() const;
	void ApplyPose();
	void DisableInteraction();

	float OpenAlpha = 0.0f;
	bool bUnlocked = false;
};
