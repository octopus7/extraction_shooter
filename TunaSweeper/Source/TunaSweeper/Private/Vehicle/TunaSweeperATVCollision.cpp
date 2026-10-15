#include "Vehicle/TunaSweeperATVActor.h"

#include "AI/TunaSweeperEnemyCharacter.h"
#include "Character/TunaSweeperTopDownCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "Kismet/GameplayStatics.h"
#include "Subsystem/TunaSweeperFactionSubsystem.h"
#include "Vehicle/TunaSweeperVehicleCollisionDamageType.h"
#include "Vehicle/TunaSweeperVehicleMountComponent.h"

void ATunaSweeperATVActor::UpdateCollisionDamage(float DeltaSeconds)
{
    const FVector End = ChassisCollision->GetComponentLocation();
    const FVector Start = bHasCollisionLocation ? PreviousCollisionLocation : End;
    PreviousCollisionLocation = End;
    bHasCollisionLocation = true;
    auto* Driver = MountComponent->GetRider();
    AController* DriverController = Driver ? Driver->GetController() : nullptr;
    if (bVehicleDestroyed || !Driver || Driver->IsDead() || !DriverController ||
        !DriverController->IsPlayerController() || !FMath::IsFinite(DeltaSeconds) || DeltaSeconds <= 0) return;

    const float Speed = VehicleMesh->GetPhysicsLinearVelocity().Size2D();
    if (!FMath::IsFinite(Speed) || Speed < FMath::Max(1.0f, CollisionDamageMinSpeed)) return;
    // Discontinuous teleports are not a driven collision path.
    if (FVector::DistSquared(Start, End) > FMath::Square(FMath::Max(100.0f, Speed * DeltaSeconds * 3.0f + 50.0f))) return;
    const float Damage = FMath::Clamp(Speed * CollisionDamagePerSpeed, 0.0f, FMath::Max(0.0f, CollisionDamageMaximum));
    if (!FMath::IsFinite(Damage) || Damage <= 0) return;

    UWorld* World = GetWorld();
    auto* Factions = World ? World->GetSubsystem<UTunaSweeperFactionSubsystem>() : nullptr;
    if (!World || !Factions) return;
    const double Now = World->GetTimeSeconds();
    for (auto It = NextCollisionDamageTime.CreateIterator(); It; ++It)
        if (!It.Key().IsValid() || It.Value() <= Now) It.RemoveCurrent();

    FCollisionQueryParams Query(SCENE_QUERY_STAT(ATVEnemyCollision), false, this);
    Query.AddIgnoredActor(Driver);
    TArray<FHitResult> Hits;
    World->SweepMultiByObjectType(Hits, Start, End, ChassisCollision->GetComponentQuat(),
        FCollisionObjectQueryParams(ECC_Pawn), FCollisionShape::MakeBox(ChassisCollision->GetScaledBoxExtent()), Query);
    TSet<ATunaSweeperEnemyCharacter*> Visited;
    for (const FHitResult& Hit : Hits)
    {
        auto* Enemy = Cast<ATunaSweeperEnemyCharacter>(Hit.GetActor());
        if (!IsValid(Enemy) || Enemy->IsDead() || Visited.Contains(Enemy) ||
            NextCollisionDamageTime.Contains(Enemy) || !Factions->CanTargetActor(Driver, Enemy)) continue;
        Visited.Add(Enemy);
        NextCollisionDamageTime.Add(Enemy, Now + FMath::Max(0.05f, CollisionDamageCooldown));
        UGameplayStatics::ApplyDamage(Enemy, Damage, DriverController, this, UTunaSweeperVehicleCollisionDamageType::StaticClass());
    }
}
