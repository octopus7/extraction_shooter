#pragma once

#include "CoreMinimal.h"
#include "BossLab/TunaSweeperBossDefinition.h"
#include "Game/TunaSweeperGameMode.h"
#include "GameFramework/HUD.h"
#include "Player/TunaSweeperPlayerController.h"
#include "TunaSweeperBossLabGameMode.generated.h"

class ACameraActor;
class UFont;
class ATunaSweeperModularBoss;
class UTunaSweeperBossLabWidget;

/** Local workshop and battle share one world and a retained definition in the lab subsystem. */
UCLASS()
class TUNASWEEPER_API ATunaSweeperBossLabGameMode : public ATunaSweeperGameMode
{
	GENERATED_BODY()
public:
	ATunaSweeperBossLabGameMode();
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void StartPlay() override;
	virtual void RestartPlayer(AController* NewPlayer) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void RefreshPreview(const FTunaSweeperBossDefinition& Definition);
	void SetPreviewSelectedPart(int32 InstanceId);
	void RotatePreview(float Degrees);
	void StartBattle();
	void StopBattle();
	void ReturnToTitle();
	bool IsBattleActive() const { return bBattleActive; }
	FText GetStatusText() const;
	ATunaSweeperModularBoss* GetBoss() const { return Boss; }

private:
	void OpenWorkshop();
	void FinishBattle(FName ResultKey);
	void CleanupPlayer();
	void SetWorkshopView(bool bWorkshop);
	bool PreparePlayer();

	UPROPERTY(Transient) TObjectPtr<ATunaSweeperModularBoss> Boss;
	UPROPERTY(Transient) TObjectPtr<ACameraActor> PreviewCamera;
	UPROPERTY(Transient) TObjectPtr<UTunaSweeperBossLabWidget> WorkshopWidget;
	FName StatusKey = TEXT("ui.boss_lab.battle.ready");
	bool bBattleActive = false;
	bool bLeaving = false;
	bool bWorkshopReady = false;
	float PreviewYaw = 0.f;
	float AmmoCheckSeconds = 0.f;
};

UCLASS()
class TUNASWEEPER_API ATunaSweeperBossLabPlayerController : public ATunaSweeperPlayerController
{
	GENERATED_BODY()
public:
	ATunaSweeperBossLabPlayerController();
	virtual void SetupInputComponent() override;
private:
	void HandleLabBack();
};

UCLASS()
class TUNASWEEPER_API ATunaSweeperBossLabHUD : public AHUD
{
	GENERATED_BODY()
public:
	virtual void DrawHUD() override;
private:
	UPROPERTY(Transient) TObjectPtr<UFont> CanvasFont;
};
