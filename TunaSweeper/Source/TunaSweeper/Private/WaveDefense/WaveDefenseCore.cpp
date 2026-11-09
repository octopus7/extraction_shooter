#include "WaveDefense/WaveDefenseCore.h"
#include "WaveDefense/WaveDefenseDirector.h"
#include "WaveDefense/WaveDefenseParticipantComponent.h"
#include "Component/TunaSweeperFactionComponent.h"
#include "Subsystem/TunaSweeperFactionSubsystem.h"
#include "Subsystem/TunaSweeperDifficultySubsystem.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
float FWaveDefenseDamageRules::Resolve(float RawDamage, int32 Multiplier, FGuid TargetRun, FGuid SourceRun, bool bHostile, bool bActiveParticipant)
{
    if (!FMath::IsFinite(RawDamage) || RawDamage <= 0 || Multiplier <= 0 || !TargetRun.IsValid() || TargetRun != SourceRun || !bHostile || !bActiveParticipant) return 0.0f;
    return FMath::RoundToFloat(UTunaSweeperDifficultySubsystem::ScaleEnemyIncomingDamage(RawDamage, Multiplier));
}
AWaveDefenseCore::AWaveDefenseCore()
{
    VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CoreMesh")); SetRootComponent(VisualMesh);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (Cube.Succeeded()) VisualMesh->SetStaticMesh(Cube.Object);
    VisualMesh->SetRelativeScale3D(FVector(1.5f, 1.5f, 1.5f)); VisualMesh->SetCollisionProfileName(TEXT("BlockAll"));
    FactionComponent = CreateDefaultSubobject<UTunaSweeperFactionComponent>(TEXT("Faction"));
    FactionComponent->SetFactionId(TunaSweeperFactionIds::Player);
    FactionComponent->SetCanBeCombatTarget(true);
}
void AWaveDefenseCore::ResetForRun(FGuid InRunId, int32 InMaxHealth)
{
    RunId = InRunId; MaxHealth = FMath::Max(0, InMaxHealth); Health = MaxHealth;
}
float AWaveDefenseCore::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
    if (!Director || !GetWorld() || Health <= 0) return 0.0f;
    const AActor* Source = EventInstigator && EventInstigator->GetPawn() ? EventInstigator->GetPawn() : DamageCauser;
    const UWaveDefenseParticipantComponent* Participant = Source ? Source->FindComponentByClass<UWaveDefenseParticipantComponent>() : nullptr;
    if (!Participant && DamageCauser && DamageCauser->GetInstigator()) Participant = DamageCauser->GetInstigator()->FindComponentByClass<UWaveDefenseParticipantComponent>();
    const UTunaSweeperFactionSubsystem* Factions = GetWorld()->GetSubsystem<UTunaSweeperFactionSubsystem>();
    const bool bHostile = Factions && Source && Factions->AreActorsHostile(Source, this);
    const float Applied = Participant ? FWaveDefenseDamageRules::Resolve(DamageAmount, Director->GetDamageMultiplier(), RunId, Participant->GetRunId(), bHostile, Director->IsActiveParticipant(Participant->GetRunId(), Participant->GetToken(), Participant->GetOwner())) : 0.0f;
    const float Clamped = FMath::Min<float>(Health, Applied);
    Health -= FMath::RoundToInt(Clamped);
    return Clamped;
}
