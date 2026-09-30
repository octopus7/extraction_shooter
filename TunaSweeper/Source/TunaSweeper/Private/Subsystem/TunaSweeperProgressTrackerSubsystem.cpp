#include "Subsystem/TunaSweeperProgressTrackerSubsystem.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Progress/TunaSweeperProgressTrackerModel.h"
#include "Settings/TunaSweeperBuildFlavor.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Misc/ConfigCacheIni.h"
#include "Engine/World.h"

void UTunaSweeperProgressTrackerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    SessionState.StartNewRun();
    if (!GConfig) return;
    const TCHAR* Section = TEXT("TunaSweeper.ProgressTracker");
    GConfig->GetBool(Section, TEXT("Enabled"), bEnabled, GGameIni);
    GConfig->GetBool(Section, TEXT("AllowDevelopment"), bAllowDevelopment, GGameIni);
    GConfig->GetString(Section, TEXT("Endpoint"), Endpoint, GGameIni);
    // Match the project version displayed by the title screen.
    GConfig->GetString(TEXT("/Script/EngineSettings.GeneralProjectSettings"), TEXT("ProjectVersion"), BuildId, GGameIni);
    Endpoint.TrimStartAndEndInline();
    BuildId.TrimStartAndEndInline();
    if (!bEnabled || !TunaSweeperBuildFlavor::IsDemo() || !Endpoint.StartsWith(TEXT("https://")) ||
        Endpoint.Len() <= 8 || BuildId.IsEmpty() || BuildId.Len() > 128) return;
#if !UE_BUILD_SHIPPING
    if (!bAllowDevelopment) return;
#endif
    FString StoredId;
    GConfig->GetString(Section, TEXT("PlayerId"), StoredId, GGameUserSettingsIni);
    if (!FGuid::Parse(StoredId, PlayerId) || !PlayerId.IsValid())
    {
        PlayerId = FGuid::NewGuid();
        GConfig->SetString(Section, TEXT("PlayerId"), *PlayerId.ToString(EGuidFormats::DigitsWithHyphensLower), GGameUserSettingsIni);
        GConfig->Flush(false, GGameUserSettingsIni);
    }
}

void UTunaSweeperProgressTrackerSubsystem::Deinitialize()
{
    for (const FHttpRequestPtr& Request : Requests)
    {
        Request->OnProcessRequestComplete().Unbind();
        Request->CancelRequest();
    }
    Requests.Reset();
    Super::Deinitialize();
}

void UTunaSweeperProgressTrackerSubsystem::ReportTitleScreenEntered() { Report(TEXT("location.title_screen"), TEXT("location"), true); }
void UTunaSweeperProgressTrackerSubsystem::ReportEndingScreenEntered() { Report(TEXT("location.ending_screen"), TEXT("location")); }
void UTunaSweeperProgressTrackerSubsystem::ReportNewGame() { Report(TEXT("game.start"), TEXT("start")); }
void UTunaSweeperProgressTrackerSubsystem::ReportDemoComplete() { Report(TEXT("demo.complete"), TEXT("complete")); }
void UTunaSweeperProgressTrackerSubsystem::ReportQuestRewardClaimed(FName QuestId)
{
    if (TunaSweeperProgressTrackerModel::IsTrackedQuest(QuestId)) Report(TEXT("quest.") + QuestId.ToString(), TEXT("quest"));
}
void UTunaSweeperProgressTrackerSubsystem::ReportLocationReached(FName LocationId)
{
    if (!LocationId.IsNone()) Report(TunaSweeperProgressTrackerModel::LocationCheckpoint(LocationId), TEXT("location"));
}

void UTunaSweeperProgressTrackerSubsystem::Report(const FString& CheckpointId, const FString& Category, bool bSessionObservation)
{
    UTunaSweeperGameInstance* GI = Cast<UTunaSweeperGameInstance>(GetGameInstance());
    if (!bEnabled || !PlayerId.IsValid() || !TunaSweeperBuildFlavor::IsDemo() || !GI || GI->IsCombatTestSession() ||
        (!bSessionObservation && !GI->ProgressTrackerState.RunId.IsValid())) return;
#if !UE_BUILD_SHIPPING
    if (!bAllowDevelopment) return;
#endif
    if (GetWorld() && GetWorld()->WorldType == EWorldType::PIE && !bAllowDevelopment) return;
    FTunaSweeperProgressTrackerState& State = bSessionObservation ? SessionState : GI->ProgressTrackerState;
    const FString Body = TunaSweeperProgressTrackerModel::PrepareEvent(State, PlayerId, BuildId, CheckpointId, Category,
        bSessionObservation ? 0.0 : GI->GetCurrentActiveSlotTotalPlaySeconds());
    if (Body.IsEmpty()) return;
    // Persist run observations before dispatch. Title visits have no save or gameplay run.
    if (!bSessionObservation) GI->SaveGameStateInternal();
    const TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(Endpoint);
    Request->SetVerb(TEXT("POST"));
    Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    Request->SetContentAsString(Body);
    Request->SetTimeout(10.0f);
    Request->OnProcessRequestComplete().BindWeakLambda(this,
        [this](FHttpRequestPtr Finished, FHttpResponsePtr, bool) { Requests.Remove(Finished); });
    Requests.Add(Request);
    if (!Request->ProcessRequest())
    {
        Request->OnProcessRequestComplete().Unbind();
        Requests.Remove(Request);
    }
}
