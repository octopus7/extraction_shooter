#pragma once

#include "CoreMinimal.h"
#include "Interfaces/IHttpRequest.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "TunaSweeperProgressTelemetrySubsystem.generated.h"

class UTunaSweeperSaveGame;
enum class ETunaSweeperQuestState : uint8;

namespace TunaSweeperProgressTelemetry
{
	const TCHAR* QuestMilestone(ETunaSweeperQuestState Previous, ETunaSweeperQuestState Current, bool bHasObjectives);
}

enum class ETunaSweeperStartupCheckpoint : uint8
{
	DeveloperLogo,
	LanguagePicker,
	TitleScreen,
	PrologueMonologue,
	PrologueVideo
};

UCLASS()
class TUNASWEEPER_API UTunaSweeperProgressTelemetrySubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	static void Observe(UGameInstance* GameInstance, ETunaSweeperStartupCheckpoint Checkpoint);
	void QueueQuestMilestone(int32 QuestId, const TCHAR* Milestone);
	void ReportFirstRegion(FName Region);
	void ReportEnding(bool bTrueEnding);
	void ResetGameplayTracking();
	void RestoreGameplayTracking(const UTunaSweeperSaveGame& Save);
	void PrepareGameplaySave(UTunaSweeperSaveGame& Save);
	void OnGameplaySaveCommitted(const UTunaSweeperSaveGame& Save);

private:
	void Report(ETunaSweeperStartupCheckpoint Checkpoint);
	bool CanReport() const;
	bool Send(const FGuid& EventRunId, const FString& Checkpoint, const TCHAR* Category, double Seconds);
	void SendGameplay(const FString& Checkpoint, const TCHAR* Category);

	FGuid PlayerId;
	FGuid RunId;
	FString Endpoint;
	FString BuildId;
	FString Dataset;
	double StartedAt = 0.0;
	TSet<ETunaSweeperStartupCheckpoint> Observed;
	FGuid GameplayRunId;
	TSet<FName> SavedQuestMilestones;
	TMap<FName, double> PendingQuestMilestones;
	TArray<FHttpRequestPtr> Requests;
};
