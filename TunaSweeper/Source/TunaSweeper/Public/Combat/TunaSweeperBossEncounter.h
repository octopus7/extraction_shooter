#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TunaSweeperBossEncounter.generated.h"

class APawn;
class ATunaSweeperEnemyCharacter;
class UBoxComponent;
class UPrimitiveComponent;

UENUM(BlueprintType)
enum class ETunaSweeperBossEncounterState : uint8
{
	Ready,
	Active,
	Cleared
};

/** A local, repeatable arena. Its enemies exist only while a living player occupies CombatBounds. */
UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperBossEncounter : public AActor
{
	GENERATED_BODY()

public:
	ATunaSweeperBossEncounter();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Encounter")
	TObjectPtr<UBoxComponent> CombatBounds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Encounter")
	TSubclassOf<ATunaSweeperEnemyCharacter> BossClass;

	/** Capsule center relative to this actor, in cm. Keep it inside CombatBounds and above the floor. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Encounter", meta = (MakeEditWidget = "true", Units = "cm"))
	FVector BossSpawnOffset = FVector(700.0f, 0.0f, 110.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Encounter")
	FRotator BossSpawnRotation = FRotator(0.0f, 180.0f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Encounter")
	FName EncounterId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Encounter")
	FText DisplayName;

	UFUNCTION(BlueprintPure, Category = "Encounter")
	ETunaSweeperBossEncounterState GetState() const { return State; }

	UFUNCTION(BlueprintPure, Category = "Encounter")
	ATunaSweeperEnemyCharacter* GetBoss() const;

	UFUNCTION(BlueprintPure, Category = "Encounter")
	bool IsPlayerInside() const;

	/** Uses the pawn center, so touching the trigger with a capsule does not start combat early. */
	UFUNCTION(BlueprintPure, Category = "Encounter")
	bool ContainsWorldPoint(FVector WorldPoint) const;

	/** Cancels combat immediately. A player still inside must leave before another attempt. */
	UFUNCTION(BlueprintCallable, Category = "Encounter")
	void ResetEncounter();

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UFUNCTION()
	void OnCombatBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnCombatEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex);

	UFUNCTION()
	void OnBossDestroyed(AActor* DestroyedActor);

	APawn* FindPlayerInside() const;
	bool IsLivingPlayer(const APawn* Pawn) const;
	void UpdateOccupancy();
	void StartEncounter(APawn* Player);
	void ClearEncounter();
	void CleanupEncounterActors();
	void TrackSpawnedActor(AActor* Actor);
	bool BelongsToEncounter(const AActor* Actor) const;
	void SetState(ETunaSweeperBossEncounterState NewState, const TCHAR* Reason);

	UPROPERTY(Transient)
	ETunaSweeperBossEncounterState State = ETunaSweeperBossEncounterState::Ready;

	TWeakObjectPtr<ATunaSweeperEnemyCharacter> Boss;
	TWeakObjectPtr<APawn> Occupant;
	TSet<TWeakObjectPtr<AActor>> EncounterActors;
	FDelegateHandle ActorSpawnedHandle;
	bool bAwaitingExit = false;
	bool bCleaningUp = false;
	bool bUpdatingOccupancy = false;
};
