#include "TunaRocketConfig.h"
#include "TunaRocketFlight.h"
void FTunaRocketSettings::Normalize()
{
    using TunaRocketFlight::FiniteClamp;
    Speed = FiniteClamp(Speed, 1200, 0, 50000);
    TurnRate = FiniteClamp(TurnRate, 30, 0, 720);
    Lifetime = FiniteClamp(Lifetime, 3, .05f, 30);
    GuidanceDelay = FiniteClamp(GuidanceDelay, .15f, 0, Lifetime);
    GuidanceDuration = FiniteClamp(GuidanceDuration, 1.1f, 0, Lifetime);
    GuidanceConeHalfAngle = FiniteClamp(GuidanceConeHalfAngle, 65, 0, 89);
    CollisionRadius = FiniteClamp(CollisionRadius, 6, .1f, 100);
    Damage = FiniteClamp(Damage, 0, 0, 100000);
    DamageRadius = FiniteClamp(DamageRadius, 100, 0, 10000);
    TrailInterval = FiniteClamp(TrailInterval, .08f, .03f, 10);
    TrailLifetime = FiniteClamp(TrailLifetime, .45f, .01f, 5);
    ExplosionLifetime = FiniteClamp(ExplosionLifetime, .55f, .01f, 5);
    TrailSize = FiniteClamp(TrailSize, 9, .1f, 1000);
    ExplosionVisualRadius = FiniteClamp(ExplosionVisualRadius, 100, .1f, 10000);
}
