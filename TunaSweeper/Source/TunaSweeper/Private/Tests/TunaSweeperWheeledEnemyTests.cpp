#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "AI/TunaSweeperEnemyCharacter.h"
#include "AI/TunaSweeperEnemyAIController.h"
#include "Component/TunaSweeperFactionComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DamageEvents.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/ScopeExit.h"
#include "Subsystem/TunaSweeperFactionSubsystem.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperWheeledEnemyPresentationTest,
    "TunaSweeper.Enemy.Wheeled.PresentationMovementAndFaction",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTunaSweeperWheeledEnemyPresentationTest::RunTest(const FString& Parameters)
{
    // The test also runs before the native presentation class is implemented.
    UClass* Class = FindObject<UClass>(nullptr, TEXT("/Script/TunaSweeper.TunaSweeperWheeledEnemyCharacter"));
    if (!TestNotNull(TEXT("Wheeled robot presentation class exists"), Class)) return false;
    auto* TricycleProperty = FindFProperty<FBoolProperty>(Class, TEXT("bTricycle"));
    if (!TestNotNull(TEXT("Tricycle layout is configurable on the BP"), TricycleProperty)) return false;
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    ON_SCOPE_EXIT {
        World->DestroyWorld(false);
        GEngine->DestroyWorldContext(World);
        World->RemoveFromRoot();
    };
    auto Spawn = [&](bool bTricycle, uint8 Faction, FVector Location)
    {
        auto* Enemy = World->SpawnActorDeferred<ATunaSweeperEnemyCharacter>(Class, FTransform(Location),
            nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
        TricycleProperty->SetPropertyValue_InContainer(Enemy, bTricycle);
        Enemy->GetFactionComponent()->SetFactionId(Faction);
        Enemy->FinishSpawning(FTransform(Location));
        Enemy->DispatchBeginPlay();
        return Enemy;
    };
    auto Part = [](AActor* Actor, const TCHAR* Name) -> UStaticMeshComponent*
    {
        TArray<UStaticMeshComponent*> Parts;
        Actor->GetComponents(Parts);
        for (auto* Mesh : Parts) if (Mesh->GetFName() == FName(Name)) return Mesh;
        return nullptr;
    };
    auto* Marine = Spawn(false, 20, FVector(0, 0, 60));
    auto* Pirate = Spawn(false, 21, FVector(500, 0, 60));
    auto* Rabbit = Spawn(true, 22, FVector(1000, 0, 78));
    auto* Left = Part(Marine, TEXT("LeftWheel"));
    auto* Right = Part(Marine, TEXT("RightWheel"));
    auto* Front = Part(Marine, TEXT("FrontWheel"));
    auto* RabbitFront = Part(Rabbit, TEXT("FrontWheel"));
    auto* Body = Part(Marine, TEXT("RobotBody"));
    auto* LegacyBody = Part(Marine, TEXT("VisualMesh"));
    auto* LegacyMarker = Part(Marine, TEXT("ForwardMarkerMesh"));
    if (!TestNotNull(TEXT("Left wheel"), Left) || !TestNotNull(TEXT("Right wheel"), Right) ||
        !TestNotNull(TEXT("Front wheel component"), Front) || !TestNotNull(TEXT("Rabbit front wheel"), RabbitFront) ||
        !TestNotNull(TEXT("Robot body"), Body) || !TestNotNull(TEXT("Legacy body"), LegacyBody) ||
        !TestNotNull(TEXT("Legacy marker"), LegacyMarker)) return false;
    TestFalse(TEXT("BeginPlay does not restore visible voxel body"), LegacyBody->IsVisible());
    TestFalse(TEXT("BeginPlay does not restore visible voxel marker"), LegacyMarker->IsVisible());
    TestFalse(TEXT("Delivery robot has no visible third wheel"), Front->IsVisible());
    TestTrue(TEXT("Rabbit uses its third wheel"), RabbitFront->IsVisible());
    TestTrue(TEXT("Body origin follows the capsule bottom"), FMath::IsNearlyEqual(Body->GetRelativeLocation().Z,
        -Marine->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight()));
    TestTrue(TEXT("Direct delivery BP uses melee"), Marine->UsesMeleeAttack());
    TestFalse(TEXT("Direct rabbit BP uses ranged attacks"), Rabbit->UsesMeleeAttack());
    auto* Factions = World->GetSubsystem<UTunaSweeperFactionSubsystem>();
    TestTrue(TEXT("Distinct robot factions can target each other"), Factions->CanTargetActor(Marine, Pirate));
    auto* Friendly = Spawn(false, 20, FVector(1500, 0, 60));
    TestFalse(TEXT("Same robot faction does not target itself"), Factions->CanTargetActor(Marine, Friendly));
    for (auto* Mesh : { Body, Left, Right, Front })
    {
        TestEqual(TEXT("Visual meshes do not block movement"), Mesh->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
        TestFalse(TEXT("Visual meshes cannot cut navmesh"), Mesh->CanEverAffectNavigation());
    }
    // A quarter circumference moves the top of an axle-Y wheel to the front.
    Marine->AddActorWorldOffset(FVector(50.265482, 0, 0));
    static_cast<AActor*>(Marine)->Tick(0.2f);
    TestTrue(TEXT("Forward travel spins wheel by the distance travelled"),
        Left->GetRelativeRotation().Quaternion().RotateVector(FVector::UpVector).Equals(FVector::ForwardVector, 0.001));
    const FQuat Stopped = Left->GetRelativeRotation().Quaternion();
    static_cast<AActor*>(Marine)->Tick(0.2f);
    TestTrue(TEXT("Stationary wheel stops rotating"), Stopped.Equals(Left->GetRelativeRotation().Quaternion(), 0.001));
    Marine->AddActorWorldOffset(FVector(-50.265482, 0, 0));
    static_cast<AActor*>(Marine)->Tick(0.2f);
    TestTrue(TEXT("Reversing reverses wheel rotation"), Left->GetRelativeRotation().Quaternion().Equals(FQuat::Identity, 0.001));
    Marine->SetActorRotation(FRotator(0, 90, 0));
    static_cast<AActor*>(Marine)->Tick(0.2f);
    TestTrue(TEXT("Turning in place rolls left wheel forward"), Left->GetRelativeRotation().Quaternion().RotateVector(FVector::UpVector).X > 0.1);
    TestTrue(TEXT("Turning in place rolls right wheel backward"), Right->GetRelativeRotation().Quaternion().RotateVector(FVector::UpVector).X < -0.1);
    // A closing melee robot must stop before its capsule meets the wider trike capsule.
    auto* Attacker = Spawn(false, 20, FVector(3000, 0, 60));
    const float ContactDistance = Attacker->GetCapsuleComponent()->GetScaledCapsuleRadius() +
        Rabbit->GetCapsuleComponent()->GetScaledCapsuleRadius();
    TestTrue(TEXT("Delivery melee stopping distance clears both robot capsules"),
        Attacker->GetMeleeApproachStopRange() >= ContactDistance);
    TestTrue(TEXT("Delivery melee hysteresis stays within attack range"),
        Attacker->GetMeleeApproachStopRange() < Attacker->GetMeleeApproachStartRange() &&
        Attacker->GetMeleeApproachStartRange() <= Attacker->GetMeleeAttackRange());
    auto Profile = Attacker->GetCombatProfile();
    Profile.AlertSeconds = 0;
    Attacker->ConfigureCombatProfile(Profile, 20, NAME_None, 0);
    Rabbit->SetActorLocation(FVector(3200, 0, 78));
    auto* Controller = World->SpawnActor<ATunaSweeperEnemyAIController>();
    Controller->Possess(Attacker);
    Controller->NotifyDamageTaken(Rabbit);
    static_cast<AActor*>(Controller)->Tick(0.1f);
    const float BeforeContactHealth = Rabbit->GetHealth();
    Rabbit->SetActorLocation(FVector(3000 + ContactDistance, 0, 78));
    static_cast<AActor*>(Controller)->Tick(0.1f);
    TestTrue(TEXT("Closing delivery AI attacks a hostile trike at capsule contact"),
        Rabbit->GetHealth() < BeforeContactHealth);
    Marine->TakeDamage(100000, FDamageEvent(), nullptr, nullptr);
    TestTrue(TEXT("Damage still kills wheeled robot"), Marine->IsDead());
    TestFalse(TEXT("Death immediately hides new body"), Body->IsVisible());
    TestFalse(TEXT("Death immediately hides new wheels"), Left->IsVisible());
    return true;
}
#endif
