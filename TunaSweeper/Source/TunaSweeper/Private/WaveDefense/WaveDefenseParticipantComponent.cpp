#include "WaveDefense/WaveDefenseParticipantComponent.h"
#include "WaveDefense/WaveDefenseDirector.h"
void UWaveDefenseParticipantComponent::Initialize(AWaveDefenseDirector* InDirector, FGuid InRunId, FGuid InToken, EWaveDefenseEnemyRole InRole)
{
    Director = InDirector; RunId = InRunId; Token = InToken; Role = InRole; bDeathReported = false;
}
AActor* UWaveDefenseParticipantComponent::GetPreferredTarget() const
{
    return Director.IsValid() ? Director->GetPreferredTarget(GetOwner(), RunId, Token, Role) : nullptr;
}
void UWaveDefenseParticipantComponent::NotifyDeath()
{
    if (!bDeathReported && Director.IsValid()) { bDeathReported = true; Director->NotifyEnemyDied(RunId, Token); }
}
void UWaveDefenseParticipantComponent::NotifyDamageFrom(AActor* DamageSource)
{
    if (DamageSource && GetWorld()) { LastDamageSource = DamageSource; LastDamageTime = GetWorld()->GetTimeSeconds(); }
}
bool UWaveDefenseParticipantComponent::WasRecentlyHitBy(const AActor* DamageSource) const
{
    return DamageSource && LastDamageSource.Get() == DamageSource && GetWorld() && GetWorld()->GetTimeSeconds() - LastDamageTime <= 3.0;
}
