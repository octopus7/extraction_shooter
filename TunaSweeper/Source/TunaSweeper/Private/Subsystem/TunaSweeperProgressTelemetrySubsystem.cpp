#include "Subsystem/TunaSweeperProgressTelemetrySubsystem.h"

#include "Engine/World.h"
#include "Game/TunaSweeperGameInstance.h"
#include "HttpModule.h"
#include "Inventory/TunaSweeperSaveGame.h"
#include "Json.h"
#include "Misc/ConfigCacheIni.h"
#include "Quest/TunaSweeperQuestTypes.h"
#include "Settings/TunaSweeperBuildFlavor.h"

namespace
{
	const TCHAR* const ProgressSection = TEXT("TunaSweeper.ProgressTracker");

	const TCHAR* CheckpointId(ETunaSweeperStartupCheckpoint Checkpoint)
	{
		switch (Checkpoint)
		{
		case ETunaSweeperStartupCheckpoint::DeveloperLogo: return TEXT("location.developer_logo");
		case ETunaSweeperStartupCheckpoint::LanguagePicker: return TEXT("location.language_picker");
		case ETunaSweeperStartupCheckpoint::TitleScreen: return TEXT("location.title_screen");
		case ETunaSweeperStartupCheckpoint::PrologueMonologue: return TEXT("location.prologue_monologue");
		case ETunaSweeperStartupCheckpoint::PrologueVideo: return TEXT("location.prologue_video");
		}
		return nullptr;
	}
}

const TCHAR* TunaSweeperProgressTelemetry::QuestMilestone(
	ETunaSweeperQuestState Previous, ETunaSweeperQuestState Current, bool bHasObjectives)
{
	using EState = ETunaSweeperQuestState;
	if (Previous == EState::Available && Current == EState::Accepted) return TEXT("accepted");
	if (Previous == EState::Accepted && Current == EState::RewardAvailable && bHasObjectives) return TEXT("objective_met");
	if (Previous == EState::RewardAvailable && Current == EState::RewardCompleted) return TEXT("reward_claimed");
	return nullptr;
}

void UTunaSweeperProgressTelemetrySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	StartedAt = FPlatformTime::Seconds();
	if (!GConfig)
	{
		return;
	}

	bool bEnabled = false;
	GConfig->GetBool(ProgressSection, TEXT("Enabled"), bEnabled, GGameIni);
	if (!bEnabled)
	{
		return;
	}
#if !UE_BUILD_SHIPPING
	bool bAllowDevelopment = false;
	GConfig->GetBool(ProgressSection, TEXT("AllowDevelopment"), bAllowDevelopment, GGameIni);
	if (!bAllowDevelopment)
	{
		return;
	}
#endif

	GConfig->GetString(ProgressSection, TEXT("Endpoint"), Endpoint, GGameIni);
	GConfig->GetString(TEXT("/Script/EngineSettings.GeneralProjectSettings"), TEXT("ProjectVersion"), BuildId, GGameIni);
	Endpoint.TrimStartAndEndInline();
	BuildId.TrimStartAndEndInline();
	if (!Endpoint.StartsWith(TEXT("https://")) || Endpoint.Len() <= 8 || BuildId.IsEmpty() || BuildId.Len() > 128)
	{
		return;
	}

	Dataset = TunaSweeperBuildFlavor::IsDemo() ? TEXT("demo") : TEXT("main");
	RunId = FGuid::NewGuid();
	FString StoredId;
	GConfig->GetString(ProgressSection, TEXT("PlayerId"), StoredId, GGameUserSettingsIni);
	if (!FGuid::Parse(StoredId, PlayerId) || !PlayerId.IsValid())
	{
		PlayerId = FGuid::NewGuid();
		GConfig->SetString(ProgressSection, TEXT("PlayerId"),
			*PlayerId.ToString(EGuidFormats::DigitsWithHyphensLower), GGameUserSettingsIni);
		GConfig->Flush(false, GGameUserSettingsIni);
	}
}

void UTunaSweeperProgressTelemetrySubsystem::Deinitialize()
{
	for (const FHttpRequestPtr& Request : Requests)
	{
		Request->OnProcessRequestComplete().Unbind();
		Request->CancelRequest();
	}
	Requests.Reset();
	Super::Deinitialize();
}

void UTunaSweeperProgressTelemetrySubsystem::Observe(UGameInstance* GameInstance, ETunaSweeperStartupCheckpoint Checkpoint)
{
	if (UTunaSweeperProgressTelemetrySubsystem* Progress = GameInstance
		? GameInstance->GetSubsystem<UTunaSweeperProgressTelemetrySubsystem>() : nullptr)
	{
		Progress->Report(Checkpoint);
	}
}

void UTunaSweeperProgressTelemetrySubsystem::Report(ETunaSweeperStartupCheckpoint Checkpoint)
{
	const TCHAR* Id = CheckpointId(Checkpoint);
	if (Id && !Observed.Contains(Checkpoint) && CanReport() &&
		Send(RunId, Id, TEXT("location"), FPlatformTime::Seconds() - StartedAt))
	{
		Observed.Add(Checkpoint);
	}
}

bool UTunaSweeperProgressTelemetrySubsystem::CanReport() const
{
	const UTunaSweeperGameInstance* Game = Cast<UTunaSweeperGameInstance>(GetGameInstance());
	return PlayerId.IsValid() && Game && !Game->IsCombatTestSession() &&
		(!GetWorld() || GetWorld()->WorldType != EWorldType::PIE);
}

void UTunaSweeperProgressTelemetrySubsystem::QueueQuestMilestone(int32 QuestId, const TCHAR* Milestone)
{
	if (!CanReport())
	{
		return;
	}
	const FName Id(*FString::Printf(TEXT("quest.%d.%s"), QuestId, Milestone));
	if (!SavedQuestMilestones.Contains(Id) && !PendingQuestMilestones.Contains(Id))
	{
		const UTunaSweeperGameInstance* Game = CastChecked<UTunaSweeperGameInstance>(GetGameInstance());
		PendingQuestMilestones.Add(Id, Game->GetCurrentActiveSlotTotalPlaySeconds());
	}
}

void UTunaSweeperProgressTelemetrySubsystem::ReportFirstRegion(FName Region)
{
	if (Region == TEXT("region.forest") || Region == TEXT("region.village") || Region == TEXT("region.plains"))
	{
		SendGameplay(TEXT("location.") + Region.ToString().ToLower(), TEXT("location"));
	}
}

void UTunaSweeperProgressTelemetrySubsystem::ReportEnding(bool bTrueEnding)
{
	SendGameplay(Dataset + (bTrueEnding ? TEXT(".complete.true") : TEXT(".complete.normal")), TEXT("complete"));
}

void UTunaSweeperProgressTelemetrySubsystem::SendGameplay(const FString& Checkpoint, const TCHAR* Category)
{
	if (!CanReport())
	{
		return;
	}
	if (!GameplayRunId.IsValid())
	{
		GameplayRunId = FGuid::NewGuid();
	}
	const UTunaSweeperGameInstance* Game = CastChecked<UTunaSweeperGameInstance>(GetGameInstance());
	Send(GameplayRunId, Checkpoint, Category, Game->GetCurrentActiveSlotTotalPlaySeconds());
}

void UTunaSweeperProgressTelemetrySubsystem::ResetGameplayTracking()
{
	GameplayRunId.Invalidate();
	SavedQuestMilestones.Reset();
	PendingQuestMilestones.Reset();
}

void UTunaSweeperProgressTelemetrySubsystem::RestoreGameplayTracking(const UTunaSweeperSaveGame& Save)
{
	ResetGameplayTracking();
	GameplayRunId = Save.ProgressTelemetryRunId;
	SavedQuestMilestones = Save.ProgressTelemetryQuestMilestones;
}

void UTunaSweeperProgressTelemetrySubsystem::PrepareGameplaySave(UTunaSweeperSaveGame& Save)
{
	if (!GameplayRunId.IsValid() && CanReport())
	{
		GameplayRunId = FGuid::NewGuid();
	}
	Save.ProgressTelemetryRunId = GameplayRunId;
	Save.ProgressTelemetryQuestMilestones = SavedQuestMilestones;
	for (const auto& Milestone : PendingQuestMilestones)
	{
		Save.ProgressTelemetryQuestMilestones.Add(Milestone.Key);
	}
}

void UTunaSweeperProgressTelemetrySubsystem::OnGameplaySaveCommitted(const UTunaSweeperSaveGame& Save)
{
	SavedQuestMilestones = Save.ProgressTelemetryQuestMilestones;
	for (auto It = PendingQuestMilestones.CreateIterator(); It; ++It)
	{
		if (SavedQuestMilestones.Contains(It.Key()))
		{
			if (CanReport())
			{
				Send(Save.ProgressTelemetryRunId, It.Key().ToString().ToLower(), TEXT("quest"), It.Value());
			}
			It.RemoveCurrent();
		}
	}
}

bool UTunaSweeperProgressTelemetrySubsystem::Send(
	const FGuid& EventRunId, const FString& Checkpoint, const TCHAR* Category, double Seconds)
{
	const TSharedRef<FJsonObject> Event = MakeShared<FJsonObject>();
	constexpr EGuidFormats GuidFormat = EGuidFormats::DigitsWithHyphensLower;
	Event->SetStringField(TEXT("eventId"), FGuid::NewGuid().ToString(GuidFormat));
	Event->SetStringField(TEXT("playerId"), PlayerId.ToString(GuidFormat));
	Event->SetStringField(TEXT("runId"), EventRunId.ToString(GuidFormat));
	Event->SetStringField(TEXT("buildId"), BuildId);
	Event->SetStringField(TEXT("dataset"), Dataset);
	Event->SetStringField(TEXT("checkpointId"), Checkpoint);
	Event->SetStringField(TEXT("category"), Category);
	Event->SetNumberField(TEXT("playtimeSeconds"), FMath::Clamp(Seconds, 0.0, 315576000.0));

	FString Body;
	if (!FJsonSerializer::Serialize(Event, TJsonWriterFactory<>::Create(&Body)))
	{
		return false;
	}

	const TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(Endpoint);
	Request->SetVerb(TEXT("POST"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->SetContentAsString(Body);
	Request->SetTimeout(10.0f);
	Request->OnProcessRequestComplete().BindWeakLambda(this,
		[this](FHttpRequestPtr Finished, FHttpResponsePtr, bool) { Requests.Remove(Finished); });
	Requests.Add(Request);
	if (Request->ProcessRequest())
	{
		return true;
	}
	Request->OnProcessRequestComplete().Unbind();
	Requests.Remove(Request);
	return false;
}
