#pragma once

#include "Game/TunaSweeperGameMode.h"
#include "TrailerGameMode.generated.h"

class ATunaSweeperTopDownCharacter;
class ALoopRailStation;
class ALoopRailTrain;
class ATunaSweeperBreakableAppleCrateActor;
class ATunaSweeperExplosiveBarrelActor;

/** Runs the title-menu trailer in disposable copies of the two raid worlds. */
UCLASS()
class TUNASWEEPER_API ATrailerGameMode : public ATunaSweeperGameMode
{
	GENERATED_BODY()
public:
	ATrailerGameMode();
	static constexpr const TCHAR* TravelOptions = TEXT("game=/Script/TunaSweeper.TrailerGameMode");
	virtual void InitGame(const FString& MapName, const FString& Options, FString& Error) override;
	virtual void EndPlay(EEndPlayReason::Type Reason) override;
	virtual void Tick(float DeltaSeconds) override;

private:
	enum class EShot : uint8 { ExitGarage, EnterGarage, GrayRobot, BlueRobot, Barrel, Apples, Shop, Platform, Carriage, Beach, Captain, End };
	bool SetUpShot();
	bool PlayShot(float DeltaSeconds);
	void WalkTo(FVector Goal);
	void Shoot(AActor* Actor);
	FVector GroundAt(FVector Point) const;
	void PlacePlayer(FVector Feet);
	void ReturnToTitle();

	EShot Shot = EShot::ExitGarage;
	TWeakObjectPtr<ATunaSweeperTopDownCharacter> Player;
	TWeakObjectPtr<AActor> Target;
	TWeakObjectPtr<ALoopRailStation> Station;
	TWeakObjectPtr<ALoopRailTrain> Train;
	TArray<TWeakObjectPtr<ATunaSweeperBreakableAppleCrateActor>> ExtraCrates;
	TArray<TWeakObjectPtr<ACharacter>> Robots;
	TArray<TWeakObjectPtr<ATunaSweeperExplosiveBarrelActor>> Barrels;
	FVector Goal;
	float Elapsed = 0;
	float NextShot = 0;
	float HoldTime = 0;
	float FadeRemaining = 0;
	bool bReady = false;
	bool bLeaving = false;
	bool bWaitingForRail = false;
	float MechCombatTime = 0.f;
};
