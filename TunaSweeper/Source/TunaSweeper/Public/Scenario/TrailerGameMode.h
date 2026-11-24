#pragma once

#include "Game/TunaSweeperGameMode.h"
#include "TrailerGameMode.generated.h"

class ATunaSweeperTopDownCharacter;
class ALoopRailStation;
class ATunaSweeperSupplyDropActor;
class ATunaSweeperWheeledEnemyCharacter;
class ALoopRailTrain;
class ATunaSweeperBreakableAppleCrateActor;
class ATunaSweeperExplosiveBarrelActor;

/** Runs the title-menu trailer in disposable copies of the base and raid worlds. */
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
	enum class EShot : uint8 { TeaRoom, ExitGarage, GrayRobot, SupplyLanding, BlueRobot, Barrel, Apples, EnterGarage, Shop, Trikes, Platform, Carriage, Breakwater, Beach, Captain, End };
	bool SetUpShot();
	bool PlayShot(float DeltaSeconds);
	bool SetUpTrikes();
	bool ReleaseSupplyDrop();
	TWeakObjectPtr<ATunaSweeperSupplyDropActor> SupplyDrop;
	FVector SupplyLandingPoint;
	FVector SupplyTravelDirection;
	bool PlayTrikes(float DeltaSeconds);

	struct FTrikeRoute
	{
		TWeakObjectPtr<ATunaSweeperWheeledEnemyCharacter> Bike;
		TArray<FVector> Waypoints;
		int32 Next = 0;
		bool bFired = false;
	};
	FTrikeRoute Trikes[2];
	float TrikeClosestPass = MAX_flt;
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
