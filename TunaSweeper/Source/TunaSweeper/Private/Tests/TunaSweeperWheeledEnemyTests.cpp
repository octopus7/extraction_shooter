#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "AI/TunaSweeperEnemyCharacter.h"
#include "AI/TunaSweeperEnemyAIController.h"
#include "AI/TunaSweeperWheeledEnemyCharacter.h"
#include "AI/TunaSweeperWheeledMovementComponent.h"
#include "Component/TunaSweeperFactionComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DamageEvents.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/StaticMeshActor.h"
#include "GameFramework/CharacterMovementComponent.h"
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperTrikeDrivingTest,
    "TunaSweeper.Enemy.Wheeled.TrikeSteeringAndStationaryFire",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTunaSweeperTrikeDrivingTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    ON_SCOPE_EXIT {
        World->DestroyWorld(false);
        GEngine->DestroyWorldContext(World);
        World->RemoveFromRoot();
    };
    auto* Rabbit = World->SpawnActorDeferred<ATunaSweeperWheeledEnemyCharacter>(
        ATunaSweeperWheeledEnemyCharacter::StaticClass(), FTransform::Identity,
        nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
    Rabbit->bTricycle = true;
    FTunaSweeperEnemyCombatProfile Profile;
    Profile.ProfileId = TEXT("test.trike");
    Profile.AttackMode = ETunaSweeperEnemyAttackMode::Ranged;
    Profile.MovementSpeed = 430;
    Profile.TurnSpeedDegreesPerSecond = 180;
    Profile.AttackFacingToleranceDegrees = 8;
    Rabbit->ConfigureCombatProfile(Profile, 22, NAME_None, 0);
    Rabbit->FinishSpawning(FTransform::Identity);
    Rabbit->DispatchBeginPlay();
    auto* Movement = Rabbit->GetCharacterMovement();
    Movement->SetMovementMode(MOVE_Walking);
    Movement->RequestDirectMove(FVector(0, 430, 0), true);
    Movement->CalcVelocity(0.1f, 8, false, 2048);
    TestTrue(TEXT("Lateral goal turns the trike within its 180 deg/s limit"),
        Rabbit->GetActorRotation().Yaw > 0 && Rabbit->GetActorRotation().Yaw <= 18.01);
    TestTrue(TEXT("A sharp turn slows before driving"), Movement->Velocity.Size2D() < 100);
    TestTrue(TEXT("Navigation cannot drive sideways through the wheels"),
        FMath::Abs(FVector::DotProduct(Movement->Velocity, Rabbit->GetActorRightVector())) < 0.1);
    for (int32 Step = 0; Step < 20; ++Step)
    {
        Movement->RequestDirectMove(FVector(0, 430, 0), true);
        Movement->CalcVelocity(0.05f, 8, false, 2048);
    }
    TestTrue(TEXT("Steering reaches the lateral heading without getting stuck"),
        FMath::Abs(FMath::FindDeltaAngleDegrees(Rabbit->GetActorRotation().Yaw, 90)) < 0.1);
    TestTrue(TEXT("Aligned travel recovers forward speed"), Movement->Velocity.Size2D() > 300);

    auto* Controller = World->SpawnActor<ATunaSweeperEnemyAIController>();
    Controller->Possess(Rabbit);
    // Possession initializes the AI in idle and stops prior movement.
    Movement->Velocity = Rabbit->GetActorForwardVector() * 200;
    Controller->bMoveRequestActive = true;
    const float DrivingYaw = Rabbit->GetActorRotation().Yaw;
    Controller->RotateTowardLocation(Rabbit->GetActorLocation() + FVector(1000, 0, 0), 0.1f);
    TestTrue(TEXT("AI target facing does not fight active steering"),
        FMath::IsNearlyEqual(Rabbit->GetActorRotation().Yaw, DrivingYaw));

    auto* Target = World->SpawnActor<ATunaSweeperEnemyCharacter>();
    Target->GetFactionComponent()->SetFactionId(20);
    Target->SetActorLocation(Rabbit->GetActorLocation() + Rabbit->GetActorForwardVector() * 1000);
    TestEqual(TEXT("Moving trike cannot fire through a direct call"),
        Rabbit->TryFireProjectileAt(Target), ETunaSweeperEnemyFireResult::Blocked);
    Movement->StopMovementImmediately();
    Controller->bMoveRequestActive = false;
    Controller->RotateTowardLocation(Rabbit->GetActorLocation() + FVector(1000, 0, 0), 0.1f);
    TestTrue(TEXT("Stopped trike resumes bounded aiming"), Rabbit->GetActorRotation().Yaw < DrivingYaw);
    Target->SetActorLocation(Rabbit->GetActorLocation() - Rabbit->GetActorForwardVector() * 1000);
    TestEqual(TEXT("Stopped trike must align its fixed gun before firing"),
        Rabbit->TryFireProjectileAt(Target), ETunaSweeperEnemyFireResult::Blocked);
    Target->SetActorLocation(Rabbit->GetActorLocation() + Rabbit->GetActorForwardVector() * 1000);
    TestTrue(TEXT("Stopped aligned trike releases the driving fire gate"),
        Rabbit->TryFireProjectileAt(Target) != ETunaSweeperEnemyFireResult::Blocked);
    Rabbit->AddMovementInput(Rabbit->GetActorForwardVector(), 1, true);
    TestEqual(TEXT("Pending movement input blocks firing before motion starts"),
        Rabbit->TryFireProjectileAt(Target), ETunaSweeperEnemyFireResult::Blocked);
    Rabbit->ConsumeMovementInputVector();

    // A wheel's axle is invariant under roll, so its X component isolates steering yaw.
    Rabbit->SetActorRotation(FRotator::ZeroRotator);
    for (int32 Step = 0; Step < 20; ++Step) static_cast<AActor*>(Rabbit)->Tick(0.1f);
    Rabbit->AddActorWorldOffset(FVector(20, 0, 0));
    Rabbit->SetActorRotation(FRotator(0, 10, 0));
    static_cast<AActor*>(Rabbit)->Tick(0.1f);
    TestTrue(TEXT("Front wheel visibly steers into a turn while rolling"),
        Rabbit->FrontWheel->GetRelativeRotation().Quaternion().RotateVector(FVector::RightVector).X < -0.01);
    for (int32 Step = 0; Step < 20; ++Step) static_cast<AActor*>(Rabbit)->Tick(0.1f);
    TestTrue(TEXT("Front steering centers after stopping"),
        FMath::Abs(Rabbit->FrontWheel->GetRelativeRotation().Quaternion().RotateVector(FVector::RightVector).X) < 0.01);

    Controller->NonCombatState = ETunaSweeperNonCombatState::Wander;
    Controller->AwarenessState = ETunaSweeperEnemyAwarenessState::Unaware;
    Rabbit->SetActorRotation(FRotator(0, 90, 0));
    const FVector NoiseLocation = Rabbit->GetActorLocation() + FVector(1000, 0, 0);
    Controller->StartSuspicion(NoiseLocation, TEXT("Automation: heard noise while driving"));
    Controller->RotateTowardLocation(NoiseLocation, 0.1f);
    TestTrue(TEXT("Stopped wandering trike can turn to investigate noise"),
        FMath::FindDeltaAngleDegrees(90.0f, Rabbit->GetActorRotation().Yaw) < -1.0f);
    Rabbit->SetActorRotation(FRotator(0, 90, 0));
    Target->SetActorLocation(NoiseLocation);
    Controller->StartAlerted(Target, TEXT("Automation: alerted while wandering"));
    Controller->RotateTowardLocation(NoiseLocation, 0.1f);
    TestTrue(TEXT("Stopped wandering trike can face its alerted target"),
        FMath::FindDeltaAngleDegrees(90.0f, Rabbit->GetActorRotation().Yaw) < -1.0f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperTrikeGroundTravelTest,
    "TunaSweeper.Enemy.Wheeled.TrikeBlueprintGroundTravel",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTunaSweeperTrikeGroundTravelTest::RunTest(const FString& Parameters)
{
    UClass* RabbitClass = LoadClass<ATunaSweeperWheeledEnemyCharacter>(nullptr,
        TEXT("/Game/Characters/Region2Robots/BP_RabbitTrike.BP_RabbitTrike_C"));
    UClass* DeliveryClass = LoadClass<ATunaSweeperWheeledEnemyCharacter>(nullptr,
        TEXT("/Game/Characters/Region2Robots/BP_DeliveryRobot_Marine.BP_DeliveryRobot_Marine_C"));
    if (!TestNotNull(TEXT("Saved rabbit Blueprint"), RabbitClass) ||
        !TestNotNull(TEXT("Saved delivery Blueprint"), DeliveryClass)) return false;
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    ON_SCOPE_EXIT {
        World->DestroyWorld(false);
        GEngine->DestroyWorldContext(World);
        World->RemoveFromRoot();
    };
    auto MakeBox = [World](const FVector& Location, const FVector& Scale)
    {
        auto* Box = World->SpawnActor<AStaticMeshActor>(Location, FRotator::ZeroRotator);
        auto* Mesh = Box->GetStaticMeshComponent();
        Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
        Mesh->SetWorldScale3D(Scale);
        Mesh->SetCollisionProfileName(TEXT("BlockAll"));
        return Box;
    };
    MakeBox(FVector(0, 0, -50), FVector(50, 50, 1));
    auto SpawnRobot = [World](UClass* RobotClass, FVector Location)
    {
        auto* Robot = World->SpawnActorDeferred<ATunaSweeperWheeledEnemyCharacter>(RobotClass,
            FTransform(Location), nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
        Robot->AutoPossessAI = EAutoPossessAI::Disabled;
        Robot->FinishSpawning(FTransform(Location));
        Robot->DispatchBeginPlay();
        Robot->GetCharacterMovement()->bRunPhysicsWithNoController = true;
        Robot->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
        return Robot;
    };
    auto* Rabbit = SpawnRobot(RabbitClass, FVector(0, 0, 80));
    auto* Movement = Rabbit->GetCharacterMovement();
    if (!TestNotNull(TEXT("Saved Blueprint receives the steering movement subobject"),
        Cast<UTunaSweeperWheeledMovementComponent>(Movement))) return false;
    const float OriginalMaxSpeed = Movement->MaxWalkSpeed;
    auto DriveTo = [&](FVector Goal, bool bAccelerationInput)
    {
        for (int32 Step = 0; Step < 480; ++Step)
        {
            const FVector ToGoal = (Goal - Rabbit->GetActorLocation()).GetSafeNormal2D();
            if (FVector::Dist2D(Goal, Rabbit->GetActorLocation()) < 35) return true;
            if (bAccelerationInput) Movement->RequestPathMove(ToGoal);
            else Movement->RequestDirectMove(ToGoal * OriginalMaxSpeed, true);
            Movement->TickComponent(1.0f / 60.0f, LEVELTICK_All, nullptr);
            if (FMath::Abs(FVector::DotProduct(Movement->Velocity, Rabbit->GetActorRightVector())) > 0.1)
            {
                AddError(TEXT("Ground integration introduced sideways driving"));
                return false;
            }
        }
        return false;
    };
    TestTrue(TEXT("Acceleration-based path input reaches a lateral ground destination"),
        DriveTo(FVector(0, 400, 80), true));
    Movement->StopMovementImmediately();
    TestTrue(TEXT("Direct nav velocity reaches a destination behind the trike"),
        DriveTo(Rabbit->GetActorLocation() - Rabbit->GetActorForwardVector() * 400, false));
    TestEqual(TEXT("Steering preserves the configured maximum speed"), Movement->MaxWalkSpeed, OriginalMaxSpeed);
    TestTrue(TEXT("Character movement retains ground contact"), Movement->IsMovingOnGround());

    Movement->StopMovementImmediately();
    Rabbit->SetActorLocation(FVector(0, 0, 80));
    Rabbit->SetActorRotation(FRotator::ZeroRotator);
    auto* Wall = MakeBox(FVector(220, 0, 100), FVector(0.2, 20, 2));
    for (int32 Step = 0; Step < 120; ++Step)
    {
        Movement->RequestDirectMove(FVector(OriginalMaxSpeed, 0, 0), true);
        Movement->TickComponent(1.0f / 60.0f, LEVELTICK_All, nullptr);
    }
    TestTrue(TEXT("Steering retains swept capsule collision against a wall"),
        Rabbit->GetActorLocation().X > 50 && Rabbit->GetActorLocation().X < 155);
    Movement->StopMovementImmediately();
    TestFalse(TEXT("Stopping clears steering intent and velocity"), Rabbit->IsDriving());

    // Standard character wall sliding must not turn a wheel drive into a side-step.
    Wall->GetStaticMeshComponent()->SetMobility(EComponentMobility::Movable);
    Wall->SetActorRotation(FRotator(0, 45, 0));
    Rabbit->SetActorLocation(FVector(0, 0, 80));
    Rabbit->SetActorRotation(FRotator::ZeroRotator);
    for (int32 Step = 0; Step < 120; ++Step)
    {
        Movement->RequestDirectMove(FVector(OriginalMaxSpeed, 0, 0), true);
        Movement->TickComponent(1.0f / 60.0f, LEVELTICK_All, nullptr);
    }
    TestTrue(TEXT("Oblique wall contact cannot slide the trike sideways"),
        FMath::Abs(Rabbit->GetActorLocation().Y) < 1 && FMath::Abs(Movement->Velocity.Y) < 0.1);
    Movement->StopMovementImmediately();

    auto* Delivery = SpawnRobot(DeliveryClass, FVector(-500, 0, 62));
    Delivery->GetCharacterMovement()->RequestDirectMove(FVector(0, 200, 0), true);
    Delivery->GetCharacterMovement()->CalcVelocity(0.1f, 8, false, 2048);
    TestTrue(TEXT("Delivery robot retains its original unrestricted movement"),
        FMath::IsNearlyZero(Delivery->GetActorRotation().Yaw) && Delivery->GetVelocity().Y > 0);
    return true;
}
#endif
