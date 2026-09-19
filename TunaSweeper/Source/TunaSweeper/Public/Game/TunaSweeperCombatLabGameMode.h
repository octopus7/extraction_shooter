#pragma once
#include "CoreMinimal.h"
#include "Game/TunaSweeperGameMode.h"
#include "GameFramework/HUD.h"
#include "TunaSweeperCombatLabGameMode.generated.h"
class ATunaSweeperEnemyCharacter;
class ATunaSweeperTopDownCharacter;
class UTunaSweeperCombatLabAutopilotComponent;

UCLASS()
class TUNASWEEPER_API ATunaSweeperCombatLabGameMode : public ATunaSweeperGameMode
{
	GENERATED_BODY()
public:
	ATunaSweeperCombatLabGameMode();
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	bool IsRoundActive() const { return bRoundActive; }
	int32 GetRoundNumber() const { return Round; }
	TArray<ATunaSweeperEnemyCharacter*> GetAliveEnemies() const;
	void ResetRound();
	void ToggleAutopilot();
	FText GetStatusText() const;
	FText GetMetricsText() const;
private:
	void StartRound();
	void FinishRound(FName Result);
	void CleanupRound();
	bool BelongsToRound(AActor* Actor) const;
	void HandleSpawned(AActor* Actor);
	UPROPERTY() TSubclassOf<ATunaSweeperEnemyCharacter> EnemyClass;
	UPROPERTY() TObjectPtr<UTunaSweeperCombatLabAutopilotComponent> Pilot;
	TArray<TWeakObjectPtr<ATunaSweeperEnemyCharacter>> Enemies;
	TMap<TWeakObjectPtr<ATunaSweeperEnemyCharacter>, float> EnemyHealth;
	TWeakObjectPtr<ATunaSweeperTopDownCharacter> Player;
	TSet<TWeakObjectPtr<AActor>> RoundActors;
	FDelegateHandle SpawnHandle;
	FString ReportPath;
	FName ResultKey = TEXT("ui.combat_lab.ready");
	FVector PreviousLocation = FVector::ZeroVector;
	float Elapsed = 0, Distance = 0, IdleSeconds = 0, DamageTaken = 0, DamageDealt = 0;
	float LastHealth = 100, NextRoundAt = 0, NextSampleAt = 0;
	int32 Round = 0, PlayerProjectiles = 0, EnemyProjectiles = 0, Hits = 0;
	bool bRoundActive = false, bAutopilot = true, bCleaning = false;
};

UCLASS()
class TUNASWEEPER_API ATunaSweeperCombatLabHUD : public AHUD
{
	GENERATED_BODY()
public:
	virtual void DrawHUD() override;
};
