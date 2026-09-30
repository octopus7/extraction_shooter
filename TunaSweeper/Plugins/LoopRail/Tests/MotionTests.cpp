#include "../Source/LoopRail/Public/LoopRailMotion.h"
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
    std::printf("LoopRail motion: %d failure(s)\n", Failures);
    return Failures ? 1 : 0;
}
