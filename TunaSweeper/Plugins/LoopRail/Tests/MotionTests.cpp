#include "../Source/LoopRail/Public/LoopRailMotion.h"
#include "../Source/LoopRail/Public/LoopRailArrivalEstimate.h"
#include <cmath>
#include <cstdio>
int main()
{
    int Failures = 0;
    auto Check = [&Failures](bool Value, const char* Name)
    { if (!Value) { std::printf("FAIL: %s\n", Name); ++Failures; } };
    using namespace LoopRailMotion;
    Check(std::abs(Wrap(-20, 1000) - 980) < .001, "negative carriage distance wraps");
    Check(std::abs(Wrap(2020, 1000) - 20) < .001, "multiple laps wrap");
    Check(Wrap(30, 0) == 0, "zero length is safe");
    Check(std::abs(Forward(980, 20, 1000) - 40) < .001, "next stop crosses seam");
    Check(OccupiesCrossing(950, 10, 100, 20, 70, 5, 1000), "warning before seam");
    Check(OccupiesCrossing(80, 10, 100, 20, 0, 5, 1000), "rear carriage still occupies crossing");
    Check(!OccupiesCrossing(130, 10, 100, 20, 0, 5, 1000), "crossing clears after rear");
    Check(!OccupiesCrossing(400, 10, 100, 20, 70, 5, 1000), "unrelated section stays open");
    auto Step = Advance(0, 500, 100, 150, 10000, 1);
    Check(std::abs(Step.Speed - 100) < .001 && std::abs(Step.Distance - 50) < .001, "acceleration integrates distance");
    Step = Advance(500, 500, 100, 150, 10, 1);
    Check(Step.Arrived && Step.Speed == 0 && Step.Distance == 10, "large step cannot overshoot station");
    Step = Advance(100, 500, 100, 150, 1000, 0);
    Check(Step.Distance == 0 && Step.Speed == 100 && !Step.Arrived, "zero delta preserves speed");
    double Remaining = 3000, Speed = 0;
    bool Arrived = false;
    for (int i = 0; i < 2000 && !Arrived; ++i)
    {
        Step = Advance(Speed, 600, 120, 180, Remaining, .02);
        Check(Step.Distance >= 0 && Step.Distance <= Remaining, "bounded approach");
        Remaining -= Step.Distance; Speed = Step.Speed; Arrived = Step.Arrived;
    }
    Check(Arrived && Remaining < .001 && Speed == 0, "brake reaches exact stop");
    using namespace LoopRailArrivalEstimate;
    FState State;
    State.Length = 10000;
    State.CruiseSpeed = 600;
    State.Acceleration = 120;
    State.Brake = 180;
    State.Running = true;
    FStop Stops[] = {{2000, 7, true}, {6000, 11, true}, {9000, 3, true}};
    double Eta = -1;
    Check(TryEstimate(State, Stops, 3, 0, Eta) && Eta > 5 && Eta < 12,
        "arrival predicts acceleration and braking rather than distance divided by cruise");
    double FirstLeg = Eta;
    State.HeadDistance = 2000;
    State.CurrentStop = 0;
    State.SinceDeparture = 0;
    State.DwellRemaining = 4;
    Check(TryEstimate(State, Stops, 3, 0, Eta) && Eta == 0, "current dwelling station arrives now");
    Check(TryEstimate(State, Stops, 3, 1, Eta), "next station from dwell has an ETA");
    double WithDwell = Eta;
    State.DwellRemaining = 0;
    Check(TryEstimate(State, Stops, 3, 1, Eta) && std::abs(WithDwell-Eta-4) < .001,
        "remaining current dwell contributes exactly once");
    Check(TryEstimate(State, Stops, 3, 0, Eta) && Eta > 25,
        "departing current station predicts next lap instead of zero");
    State = FState{};
    State.Length = 10000; State.CruiseSpeed = 600; State.Acceleration = 120; State.Brake = 180;
    State.Running = true;
    Check(TryEstimate(State, Stops, 3, 2, Eta), "arrival through intermediate stations");
    double WithIntermediateDwell = Eta;
    Stops[0].DwellSeconds = Stops[1].DwellSeconds = 0;
    Check(TryEstimate(State, Stops, 3, 2, Eta) && std::abs(WithIntermediateDwell-Eta-18) < .001,
        "intermediate station dwell is counted, target dwell is not");
    State.HeadDistance = 9000;
    Check(TryEstimate(State, Stops, 3, 0, Eta) && Eta > FirstLeg,
        "seam crossing wraps forward to the target");
    State.Running = false;
    Check(!TryEstimate(State, Stops, 3, 0, Eta) && Eta == 0, "stopped service has no arrival estimate");
    State.Running = true;
    Stops[0].Enabled = false;
    Check(!TryEstimate(State, Stops, 3, 0, Eta), "disabled target has no ETA");
    Stops[0].Enabled = true;
    State.CruiseSpeed = 0;
    Check(!TryEstimate(State, Stops, 3, 0, Eta), "zero cruise cannot promise an arrival");
    State.CruiseSpeed = 600;
    State.Length = 0;
    Check(!TryEstimate(State, Stops, 3, 0, Eta), "invalid track cannot produce ETA");
    State.Length = 10000;
    State.HeadDistance = NAN;
    Check(!TryEstimate(State, Stops, 3, 0, Eta), "non-finite train state is rejected");
    State.HeadDistance = 0;
    State.CruiseSpeed = .0001;
    Check(!TryEstimate(State, Stops, 3, 0, Eta), "prediction work is bounded for unreachable distant stops");
    State.CruiseSpeed = 600;
    State.HeadDistance = 2000; State.CurrentStop = 0; State.SinceDeparture = 0;
    Check(TryEstimate(State, Stops, 1, 0, Eta) && Eta > 15,
        "a single-station loop departs and predicts the next full lap");
    State.DwellRemaining = 3; State.Running = false;
    Check(!TryEstimate(State, Stops, 1, 0, Eta), "paused service overrides currently dwelling status");
    State.Running = true;
    Check(!TryEstimate(State, Stops, 1, 5, Eta), "unknown target index is rejected");
    std::printf("LoopRail motion: %d failure(s)\n", Failures);
    return Failures ? 1 : 0;
}
