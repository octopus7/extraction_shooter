#include "Progress/TunaSweeperProgressTrackerModel.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

namespace
{
    bool IsIdentifier(const FString& Value)
    {
        if (Value.IsEmpty() || Value.Len() > 128) return false;
        auto IsAlphanumeric = [](TCHAR C) { return (C >= 'a' && C <= 'z') || (C >= 'A' && C <= 'Z') || (C >= '0' && C <= '9'); };
        if (!IsAlphanumeric(Value[0])) return false;
        for (TCHAR C : Value)
            if (!IsAlphanumeric(C) && C != '_' && C != '.' && C != ':' && C != '-') return false;
        return true;
    }
}

void FTunaSweeperProgressTrackerState::StartNewRun()
{
    RunId = FGuid::NewGuid();
    AttemptedCheckpoints.Reset();
}

bool FTunaSweeperProgressTrackerState::TryRecordAttempt(FName CheckpointId)
{
    if (!RunId.IsValid() || CheckpointId.IsNone() || AttemptedCheckpoints.Contains(CheckpointId)) return false;
    AttemptedCheckpoints.Add(CheckpointId);
    return true;
}

FString TunaSweeperProgressTrackerModel::SerializeEvent(const FGuid& EventId, const FGuid& PlayerId, const FGuid& RunId,
    const FString& BuildId, const FString& CheckpointId, const FString& Category, double PlaytimeSeconds)
{
    if (!EventId.IsValid() || !PlayerId.IsValid() || !RunId.IsValid() || !IsIdentifier(BuildId) ||
        !IsIdentifier(CheckpointId) || !FMath::IsFinite(PlaytimeSeconds) ||
        PlaytimeSeconds < 0 || PlaytimeSeconds > 315576000 ||
        (Category != TEXT("start") && Category != TEXT("quest") && Category != TEXT("location") && Category != TEXT("complete")))
        return FString();
    const TSharedRef<FJsonObject> Json = MakeShared<FJsonObject>();
    Json->SetStringField(TEXT("eventId"), EventId.ToString(EGuidFormats::DigitsWithHyphensLower));
    Json->SetStringField(TEXT("playerId"), PlayerId.ToString(EGuidFormats::DigitsWithHyphensLower));
    Json->SetStringField(TEXT("runId"), RunId.ToString(EGuidFormats::DigitsWithHyphensLower));
    Json->SetStringField(TEXT("buildId"), BuildId);
    Json->SetStringField(TEXT("dataset"), TEXT("demo"));
    Json->SetStringField(TEXT("checkpointId"), CheckpointId);
    Json->SetStringField(TEXT("category"), Category);
    Json->SetNumberField(TEXT("playtimeSeconds"), PlaytimeSeconds);
    FString Body;
    FJsonSerializer::Serialize(Json, TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Body));
    return Body;
}

FString TunaSweeperProgressTrackerModel::LocationCheckpoint(FName LocationId)
{
    return LocationId.IsNone() ? FString() : TEXT("location.") + LocationId.ToString();
}

bool TunaSweeperProgressTrackerModel::IsTrackedQuest(FName QuestId)
{
    static const TSet<FName> Quests = { TEXT("demo_q1_water_intake_check"), TEXT("demo_q2_clear_water_screen"),
        TEXT("demo_q3a_repair_valve"), TEXT("demo_q3b_repair_bunker_pipe"), TEXT("demo_q4_todays_reward") };
    return Quests.Contains(QuestId);
}

FString TunaSweeperProgressTrackerModel::PrepareEvent(FTunaSweeperProgressTrackerState& State, const FGuid& PlayerId,
    const FString& BuildId, const FString& CheckpointId, const FString& Category, double PlaytimeSeconds)
{
    const FString Body = SerializeEvent(FGuid::NewGuid(), PlayerId, State.RunId, BuildId, CheckpointId, Category, PlaytimeSeconds);
    return !Body.IsEmpty() && State.TryRecordAttempt(FName(*CheckpointId)) ? Body : FString();
}
