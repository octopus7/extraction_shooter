#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Component/TunaSweeperFactionTypes.h"
#include "TunaSweeperFactionComponent.generated.h"

UCLASS(ClassGroup = (TunaSweeper), meta = (BlueprintSpawnableComponent))
class TUNASWEEPER_API UTunaSweeperFactionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTunaSweeperFactionComponent();

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Faction")
	uint8 GetFactionId() const { return FactionId; }

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Faction")
	void SetFactionId(uint8 InFactionId);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Faction")
	FName GetSquadId() const { return SquadId; }

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Faction")
	void SetSquadId(FName InSquadId) { SquadId = InSquadId; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Faction")
	int32 GetSquadSlot() const { return SquadSlot; }

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Faction")
	void SetSquadSlot(int32 InSquadSlot) { SquadSlot = InSquadSlot; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Faction")
	bool CanBeCombatTarget() const { return bCanBeCombatTarget; }

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Faction")
	void SetCanBeCombatTarget(bool bInCanBeCombatTarget) { bCanBeCombatTarget = bInCanBeCombatTarget; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Faction", meta = (ClampMin = "1", ClampMax = "255", UIMin = "1", UIMax = "255"))
	uint8 FactionId = TunaSweeperFactionIds::NoFaction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Faction")
	FName SquadId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Faction", meta = (ClampMin = "-1", UIMin = "-1"))
	int32 SquadSlot = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Faction")
	bool bCanBeCombatTarget = true;
};
