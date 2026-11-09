#pragma once
#include "WaveDefense/WaveDefenseTypes.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Subsystem/TunaSweeperTextSubsystem.h"
namespace WaveDefenseText
{
    inline FText Get(const UTunaSweeperGameInstance* Game, FName Key)
    {
        FText Text;
        auto* Strings = Game ? Game->GetSubsystem<UTunaSweeperTextSubsystem>() : nullptr;
        if (Strings) Strings->TryGetTextByKey(Key, Game->GetCurrentTextLanguage(), Text);
        return Text;
    }
    inline FText Direction(const UTunaSweeperGameInstance* Game, const FWaveDefenseSnapshot& Snapshot)
    {
        auto Lane = [Game](FName Id)
        {
            if (Id == TEXT("north")) return Get(Game, TEXT("ui.wave_defense.direction_north"));
            if (Id == TEXT("west")) return Get(Game, TEXT("ui.wave_defense.direction_west"));
            if (Id == TEXT("east")) return Get(Game, TEXT("ui.wave_defense.direction_east"));
            return FText::GetEmpty();
        };
        if (Snapshot.AnnouncedLanes.IsEmpty()) return FText::GetEmpty();
        return FText::Format(Get(Game, TEXT("ui.wave_defense.direction_line")), Lane(Snapshot.AnnouncedLanes[0]),
            Snapshot.AnnouncedLanes.Num() > 1 ? Lane(Snapshot.AnnouncedLanes[1]) : FText::GetEmpty(), FText::AsNumber(FMath::CeilToInt(Snapshot.DirectionWarningSeconds)));
    }
    inline FText Result(const UTunaSweeperGameInstance* Game, const FWaveDefenseSnapshot& Snapshot)
    {
        if (Snapshot.bResultSavePending) return Get(Game, TEXT("ui.wave_defense.result_pending"));
        if (Snapshot.bRewardPending) return Get(Game, TEXT("ui.wave_defense.reward_pending"));
        if (Snapshot.Phase == EWaveDefensePhase::Resolving && Snapshot.Outcome == EWaveDefenseOutcome::Succeeded) return FText::GetEmpty();
        switch (Snapshot.Outcome)
        {
        case EWaveDefenseOutcome::Succeeded: return Get(Game, TEXT("ui.wave_defense.result_success"));
        case EWaveDefenseOutcome::CoreDestroyed: return Get(Game, TEXT("ui.wave_defense.result_core"));
        case EWaveDefenseOutcome::PlayerDied: return Get(Game, TEXT("ui.wave_defense.result_player"));
        case EWaveDefenseOutcome::Abandoned: return Get(Game, TEXT("ui.wave_defense.result_abandoned"));
        case EWaveDefenseOutcome::Interrupted: return Get(Game, TEXT("ui.wave_defense.result_interrupted"));
        case EWaveDefenseOutcome::Faulted: return Get(Game, TEXT("ui.wave_defense.result_faulted"));
        default: return FText::GetEmpty();
        }
    }
    inline FText Warnings(const UTunaSweeperGameInstance* Game, const FWaveDefenseSnapshot& Snapshot)
    {
        if (Snapshot.LeaveSecondsRemaining > 0.0f)
            return FText::Format(Get(Game, TEXT("ui.wave_defense.leave_warning")), FText::AsNumber(FMath::CeilToInt(Snapshot.LeaveSecondsRemaining)));
        if (Snapshot.CoreMaxHealth > 0 && Snapshot.CoreHealth > 0 && int64(Snapshot.CoreHealth) * 10 <= int64(Snapshot.CoreMaxHealth) * 3)
            return Get(Game, TEXT("ui.wave_defense.core_warning"));
        return FText::GetEmpty();
    }
    inline FText Message(const UTunaSweeperGameInstance* Game, const FWaveDefenseSnapshot& Snapshot, bool bActionError, FName SelectionKey)
    {
        if (bActionError) return Get(Game, TEXT("ui.wave_defense.action_failed"));
        const FText Outcome = Result(Game, Snapshot);
        if (!Outcome.IsEmpty()) return Outcome;
        return FText::Format(Get(Game, TEXT("ui.wave_defense.selected")), Get(Game, SelectionKey));
    }
}
