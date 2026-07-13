#pragma once

#include "CoreMinimal.h"
#include "Component/TunaSweeperFactionTypes.h"
#include "Subsystems/WorldSubsystem.h"
#include "TunaSweeperFactionSubsystem.generated.h"

class AActor;
class UTunaSweeperFactionComponent;

UCLASS()
class TUNASWEEPER_API UTunaSweeperFactionSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	void RegisterFactionActor(AActor* Actor);
	void UnregisterFactionActor(AActor* Actor);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Faction")
	uint8 GetFactionIdForActor(const AActor* Actor) const;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Faction")
	ETunaSweeperFactionAttitude GetFactionAttitudeById(uint8 SourceFactionId, uint8 TargetFactionId) const;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Faction")
	ETunaSweeperFactionAttitude GetFactionAttitude(const AActor* SourceActor, const AActor* TargetActor) const;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Faction")
	bool AreActorsFriendly(const AActor* SourceActor, const AActor* TargetActor) const;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Faction")
	bool AreActorsHostile(const AActor* SourceActor, const AActor* TargetActor) const;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Faction")
	bool CanTargetActor(const AActor* SourceActor, const AActor* TargetActor) const;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Faction")
	bool CanApplyCombatEffect(const AActor* SourceActor, const AActor* TargetActor) const;

	void GetActorsWithAttitude(
		const AActor* SourceActor,
		ETunaSweeperFactionAttitude Attitude,
		TArray<AActor*>& OutActors) const;

private:
	const UTunaSweeperFactionComponent* ResolveFactionComponent(const AActor* Actor) const;
	const UTunaSweeperFactionComponent* ResolveFactionComponentRecursive(
		const AActor* Actor,
		TSet<const AActor*>& VisitedActors) const;

	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<AActor>> RegisteredFactionActors;
};
