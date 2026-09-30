#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
class IHttpRequest;
#include "TunaSweeperProgressTrackerSubsystem.generated.h"

/** Best-effort demo observations: one attempt, no response processing or replay. */
UCLASS()
class TUNASWEEPER_API UTunaSweeperProgressTrackerSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    void ReportNewGame();
    void ReportQuestRewardClaimed(FName QuestId);
    void ReportLocationReached(FName LocationId);
    void ReportDemoComplete();
private:
    void Report(const FString& CheckpointId, const FString& Category);
    FGuid PlayerId;
    FString Endpoint;
    FString BuildId;
    bool bEnabled = false;
    bool bAllowDevelopment = false;
    TArray<TSharedPtr<IHttpRequest, ESPMode::ThreadSafe>> Requests;
};
