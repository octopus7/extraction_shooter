#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "LoopRailTrack.h"
#include "LoopRailTrain.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SplineComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/WorldSettings.h"

namespace
{
struct FConnectionTestWorld
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    FConnectionTestWorld() { GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World); }
    ~FConnectionTestWorld() { GEngine->DestroyWorldContext(World); World->DestroyWorld(false); }
    void Tick() { ++GFrameCounter; World->Tick(LEVELTICK_All, 1.f / 60); }
};

TArray<UBoxComponent*> GetConnectionFloors(ALoopRailTrain* Train)
{
    TArray<UBoxComponent*> Result;
    TInlineComponentArray<UBoxComponent*> Boxes(Train);
    for (UBoxComponent* Box : Boxes)
        if (Box->ComponentHasTag(TEXT("LoopRailConnectionFloor"))) Result.Add(Box);
    return Result;
}

void ConfigureRoute(ALoopRailTrack* Track, bool bCurved)
{
    Track->Spline->ClearSplinePoints(false);
    if (bCurved)
    {
        for (int32 I = 0; I < 8; ++I)
        {
            const double Angle = I * UE_PI / 4;
            Track->Spline->AddSplinePoint(FVector(FMath::Cos(Angle) * 2500, FMath::Sin(Angle) * 2500,
                FMath::Sin(Angle) * 200), ESplineCoordinateSpace::Local, false);
        }
    }
    else
    {
        for (const FVector& Point : {FVector(-8000,-4000,0), FVector(8000,-4000,0), FVector(8000,4000,0), FVector(-8000,4000,0)})
            Track->Spline->AddSplinePoint(Point, ESplineCoordinateSpace::Local, false);
        for (int32 I = 0; I < 4; ++I) Track->Spline->SetSplinePointType(I, ESplinePointType::Linear, false);
    }
    Track->Spline->SetClosedLoop(true);
    Track->RebuildTrack();
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLoopRailConnectionLifecycleTest, "LoopRail.Runtime.Connections.Rebuild",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLoopRailConnectionLifecycleTest::RunTest(const FString& Parameters)
{
    FConnectionTestWorld T;
    auto* Train = T.World->SpawnActor<ALoopRailTrain>();
    Train->RebuildTrain();
    TestEqual(TEXT("Four cars have three walkable connections"), GetConnectionFloors(Train).Num(), 3);
    Train->RebuildTrain();
    TestEqual(TEXT("Rebuilding removes previous connection components"), GetConnectionFloors(Train).Num(), 3);
    Train->VehicleCount = 1;
    Train->RebuildTrain();
    TestEqual(TEXT("A single locomotive needs no connection"), GetConnectionFloors(Train).Num(), 0);
    Train->VehicleCount = 5;
    Train->RebuildTrain();
    TestEqual(TEXT("Changing the consist creates one connection per adjacent pair"), GetConnectionFloors(Train).Num(), 4);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLoopRailConnectionWalkTest, "LoopRail.Runtime.Connections.Walking",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLoopRailConnectionWalkTest::RunTest(const FString& Parameters)
{
    const struct { bool bMovingCurve; int32 LeadingCar; } Scenarios[] = {{false, 1}, {true, 1}, {false, 0}, {true, 0}};
    for (const auto& Scenario : Scenarios)
    {
        const bool bMovingCurve = Scenario.bMovingCurve;
        const int32 LeadingCar = Scenario.LeadingCar, FollowingCar = LeadingCar + 1;
        FConnectionTestWorld T;
        auto* Track = T.World->SpawnActor<ALoopRailTrack>();
        ConfigureRoute(Track, bMovingCurve);
        auto* Train = T.World->SpawnActor<ALoopRailTrain>();
        Train->Track = Track;
        Train->StartDistance = bMovingCurve ? 4000 : 8000;
        Train->CruiseSpeed = 600;
        Train->Acceleration = 1000;
        Train->bAutoRun = false;
        Train->RebuildTrain();
        Train->ResetTrain();
        if (!TestEqual(TEXT("Connection floors exist before walking"), GetConnectionFloors(Train).Num(), 3)) continue;

        FActorSpawnParameters Spawn;
        Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        auto* Rider = T.World->SpawnActor<ACharacter>(ACharacter::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, Spawn);
        Rider->GetCapsuleComponent()->SetCapsuleSize(34, 88);
        Rider->GetCharacterMovement()->bRunPhysicsWithNoController = true;
        Rider->GetCharacterMovement()->MaxWalkSpeed = 180;
        T.World->InitializeActorsForPlay(FURL());
        T.World->BeginPlay();
        T.World->GetWorldSettings()->NotifyBeginPlay();

        // Passenger aisles are fully traversable; the locomotive destination is its rear platform.
        Rider->SetActorLocation(Train->GetVehicleFloor(FollowingCar)->GetComponentTransform().TransformPosition(FVector(300, 0, 100)));
        for (int32 Frame = 0; Frame < 30; ++Frame) T.Tick();
        TestTrue(TEXT("Rider starts supported by the carriage"), Rider->GetMovementBase() && Rider->GetMovementBase()->GetOwner() == Train);
        Train->SetRunning(bMovingCurve);
        for (int32 Frame = 0; Frame < 60; ++Frame) T.Tick();
        if (bMovingCurve) TestTrue(TEXT("Connection walk starts at the normal 600cm/s operating speed"), Train->GetSpeed() > 599);
        const FVector StartingTrainForward = Train->GetVehicleFloor(0)->GetForwardVector();
        bool bEverFell = false;
        bool bUsedConnection = false;
        auto WalkTo = [&](int32 DestinationCar, bool bConnectionCentre)
        {
            bool bReached = false;
            for (int32 Frame = 0; Frame < 600; ++Frame)
            {
                const FVector RearEnd = Train->GetVehicleFloor(LeadingCar)->GetComponentTransform().TransformPosition(FVector(-450, 0, 100));
                const FVector FrontEnd = Train->GetVehicleFloor(FollowingCar)->GetComponentTransform().TransformPosition(FVector(450, 0, 100));
                const FVector Target = bConnectionCentre ? (RearEnd + FrontEnd) * .5 :
                    Train->GetVehicleFloor(DestinationCar)->GetComponentTransform().TransformPosition(
                        FVector(DestinationCar == LeadingCar ? (LeadingCar == 0 ? -420 : -300) : 300, 0, 100));
                FVector Direction = Target - Rider->GetActorLocation();
                Direction.Z = 0;
                if (Direction.Size() < 8) { bReached = true; break; }
                Rider->AddMovementInput(Direction.GetSafeNormal(), 1, true);
                T.Tick();
                bEverFell |= Rider->GetCharacterMovement()->IsFalling();
                if (const UPrimitiveComponent* Base = Rider->GetMovementBase())
                    bUsedConnection |= Base->ComponentHasTag(TEXT("LoopRailConnectionFloor"));
                if (bEverFell) break;
            }
            return bReached;
        };
        const bool bCrossedForward = WalkTo(LeadingCar, true) && WalkTo(LeadingCar, false);
        const bool bCrossedBack = bCrossedForward && WalkTo(FollowingCar, true) && WalkTo(FollowingCar, false);
        TestTrue(bMovingCurve ? TEXT("Walks both ways through an articulated moving connection") : TEXT("Walks both ways through a stationary straight connection"), bCrossedBack);
        TestFalse(TEXT("No gap causes a falling frame during the crossing"), bEverFell);
        TestTrue(TEXT("Character movement actually bases on the connection deck"), bUsedConnection);
        if (bMovingCurve)
            TestTrue(TEXT("Moving crossing includes changing car orientation"), FVector::DotProduct(StartingTrainForward, Train->GetVehicleFloor(0)->GetForwardVector()) < .999);
        AddInfo(FString::Printf(TEXT("Connection traversal: leading car=%d moving curve=%d speed=%.1f forward=%d return=%d fell=%d bridge base=%d"),
            LeadingCar, bMovingCurve, Train->GetSpeed(), bCrossedForward, bCrossedBack, bEverFell, bUsedConnection));
    }
    return true;
}
#endif
