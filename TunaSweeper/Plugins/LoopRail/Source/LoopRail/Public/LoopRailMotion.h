#pragma once
#include <algorithm>
#include <cmath>

// Centimetres and seconds; no engine dependency so these are exercised directly.
namespace LoopRailMotion
{
inline double Wrap(double Distance, double Length)
{
    if (!std::isfinite(Distance) || !std::isfinite(Length) || Length <= 0) return 0;
    const double R = std::fmod(Distance, Length);
    return R < 0 ? R + Length : R;
}
inline double Forward(double From, double To, double Length) { return Wrap(To - From, Length); }
inline bool OccupiesCrossing(double Head, double HalfCar, double ConsistLength,
    double Crossing, double Approach, double Clearance, double Length)
{
    if (Length <= 0 || ConsistLength <= 0) return false;
    const double Rear = Head + HalfCar - ConsistLength - std::max(0.0, Clearance);
    const double Span = ConsistLength + std::max(0.0, Approach) + std::max(0.0, Clearance);
    return Span >= Length || Forward(Rear, Crossing, Length) <= Span;
}
struct FStep { double Distance; double Speed; bool Arrived; };
inline FStep Advance(double Speed, double MaxSpeed, double Acceleration, double Brake,
    double DistanceToStop, double DeltaSeconds)
{
    Speed = std::max(0.0, Speed);
    if (DeltaSeconds <= 0) return {0, Speed, false};
    const double D = std::max(0.0, DistanceToStop);
    if (D <= .01) return {D, 0, true};
    const double Target = std::min(std::max(0.0, MaxSpeed), std::sqrt(2 * std::max(1.0, Brake) * D));
    const double Next = Speed < Target ? std::min(Target, Speed + std::max(1.0, Acceleration) * DeltaSeconds)
                                     : std::max(Target, Speed - std::max(1.0, Brake) * DeltaSeconds);
    const double Travel = (Speed + Next) * .5 * DeltaSeconds;
    return Travel >= D ? FStep{D, 0, true} : FStep{Travel, Next, false};
}
}
