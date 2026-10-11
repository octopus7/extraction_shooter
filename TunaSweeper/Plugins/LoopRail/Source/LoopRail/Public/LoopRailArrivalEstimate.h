#pragma once
#include "LoopRailMotion.h"
#include <cstddef>
#include <limits>

// Uses the train's actual 20ms motion integrator. No engine/UObject dependency.
namespace LoopRailArrivalEstimate
{
inline constexpr std::size_t NoStop = std::numeric_limits<std::size_t>::max();
struct FStop
{
    double Distance = 0;
    double DwellSeconds = 0;
    bool Enabled = true;
};
struct FState
{
    double Length = 0;
    double HeadDistance = 0;
    double Speed = 0;
    double CruiseSpeed = 0;
    double Acceleration = 1;
    double Brake = 1;
    double DwellRemaining = 0;
    double SinceDeparture = 100;
    std::size_t CurrentStop = NoStop;
    bool Running = false;
};

inline bool TryEstimate(FState State, const FStop* Stops, std::size_t Count,
    std::size_t Target, double& OutSeconds)
{
    OutSeconds = 0;
    if (!State.Running || !Stops || Count == 0 || Count > 1024 || Target >= Count || !Stops[Target].Enabled
        || !std::isfinite(State.Length) || State.Length <= 0
        || !std::isfinite(State.HeadDistance) || !std::isfinite(State.Speed)
        || !std::isfinite(State.CruiseSpeed) || !std::isfinite(State.Acceleration)
        || !std::isfinite(State.Brake) || !std::isfinite(State.DwellRemaining)
        || !std::isfinite(State.SinceDeparture)) return false;
    for (std::size_t I = 0; I < Count; ++I)
        if (Stops[I].Enabled && (!std::isfinite(Stops[I].Distance) || !std::isfinite(Stops[I].DwellSeconds))) return false;
    if (State.DwellRemaining > 0 && State.CurrentStop == Target) return true;
    if (State.CruiseSpeed <= 0) return false;

    double Seconds = std::max(0.0, State.DwellRemaining);
    if (State.DwellRemaining > 0) State.SinceDeparture = 0;
    State.HeadDistance = LoopRailMotion::Wrap(State.HeadDistance, State.Length);
    // At most 2,000 simulated seconds and one million station comparisons per query.
    // An impractically slow service returns unavailable instead of blocking editor/UI.
    const std::size_t StepBudget = std::min<std::size_t>(100000, 1000000 / Count);
    constexpr double Dt = .02;
    for (std::size_t Iteration = 0; Iteration < StepBudget; ++Iteration)
    {
        double Remaining = std::numeric_limits<double>::max();
        std::size_t Next = NoStop;
        for (std::size_t I = 0; I < Count; ++I)
        {
            if (!Stops[I].Enabled || (I == State.CurrentStop && State.SinceDeparture < 5)) continue;
            const double Distance = LoopRailMotion::Forward(State.HeadDistance, Stops[I].Distance, State.Length);
            if (Distance < Remaining) { Remaining = Distance; Next = I; }
        }
        const LoopRailMotion::FStep Step = LoopRailMotion::Advance(State.Speed, State.CruiseSpeed,
            State.Acceleration, State.Brake, Remaining, Dt);
        State.HeadDistance = LoopRailMotion::Wrap(State.HeadDistance + Step.Distance, State.Length);
        State.SinceDeparture += Step.Distance;
        State.Speed = Step.Speed;
        // An exact current stop is already reached before advancing a frame.
        Seconds += (Step.Arrived && Remaining <= .01) ? 0.0 : Dt;
        if (Next != NoStop && Step.Arrived)
        {
            if (!std::isfinite(Seconds)) return false;
            if (Next == Target) { OutSeconds = Seconds; return true; }
            Seconds += std::max(0.0, Stops[Next].DwellSeconds);
            if (!std::isfinite(Seconds)) return false;
            State.CurrentStop = Next;
            State.SinceDeparture = 0;
        }
    }
    return false;
}
}
