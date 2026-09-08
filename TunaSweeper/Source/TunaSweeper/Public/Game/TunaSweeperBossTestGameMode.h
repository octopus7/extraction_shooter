#pragma once

#include "CoreMinimal.h"
#include "Game/TunaSweeperGameMode.h"
#include "GameFramework/HUD.h"
#include "Player/TunaSweeperPlayerController.h"
#include "TunaSweeperBossTestGameMode.generated.h"

/** Playable local test harness. Encounters and portals remain ordinary level actors. */
UCLASS()
class TUNASWEEPER_API ATunaSweeperBossTestGameMode : public ATunaSweeperGameMode
{
	GENERATED_BODY()
public:
	ATunaSweeperBossTestGameMode();
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintCallable, Category = "Boss Test")
	void ReturnToHub();
private:
	void PreparePlayer();
};

UCLASS()
class TUNASWEEPER_API ATunaSweeperBossTestHUD : public AHUD
{
	GENERATED_BODY()
public:
	virtual void DrawHUD() override;
};

/** Adds optional click navigation for inspecting the lab while preserving normal combat controls. */
UCLASS()
class TUNASWEEPER_API ATunaSweeperBossTestPlayerController : public ATunaSweeperPlayerController
{
	GENERATED_BODY()
public:
	virtual void SetupInputComponent() override;
	virtual void PlayerTick(float DeltaSeconds) override;
private:
	void MoveToClickedFloor();
};
