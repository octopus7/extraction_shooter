#pragma once
#include "Game/TunaSweeperRaidPlayerStart.h"
#include "VillageTunnelPlayerStart.generated.h"

class ATunaSweeperPlayerController;
class ACharacter;

UCLASS()
class TUNASWEEPER_API AVillageTunnelPlayerStart : public ATunaSweeperRaidPlayerStart
{
    GENERATED_BODY()
public:
    AVillageTunnelPlayerStart();
    void BeginArrival(ATunaSweeperPlayerController* Player);
    virtual void CalcCamera(float DeltaSeconds, FMinimalViewInfo& View) override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void EndPlay(EEndPlayReason::Type Reason) override;

private:
    void FinishArrival();
    TWeakObjectPtr<ATunaSweeperPlayerController> Controller;
    TWeakObjectPtr<ACharacter> Walker;
    FVector Origin;
    float Elapsed = 0;
    bool bInputLocked = false;
    bool bLaunched = false;
    bool bAirborne = false;
};
