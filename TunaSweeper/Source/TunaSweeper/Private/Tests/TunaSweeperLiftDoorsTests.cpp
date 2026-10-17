#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "Interaction/TunaSweeperRegionLinkActor.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperLiftDoorsTest,
    "TunaSweeper.Lift.SlidingDoorsAndInteriorTravel",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaSweeperLiftDoorsTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Editor, false);
    auto* Lift = World->SpawnActor<ATunaSweeperRegionLinkActor>();
    auto* Pawn = World->SpawnActor<APawn>();
    auto* PawnRoot = NewObject<USceneComponent>(Pawn);
    Pawn->SetRootComponent(PawnRoot);
    PawnRoot->RegisterComponent();
    Lift->bUseSlidingDoors = true;
    Lift->DoorOpenSeconds = 1;
    Lift->RequiredProgressId = TEXT("test.unrepaired.lift");
    const FVector LeftAuthored = Lift->LeftDoorMesh->GetRelativeLocation();
    const FVector RightAuthored = Lift->RightDoorMesh->GetRelativeLocation();
    Pawn->SetActorLocation(FVector(0,0,100));
    Lift->RefreshPassage();
    Lift->Tick(2);
    TestEqual(TEXT("Unrepaired doors stay shut"), Lift->GetDoorOpenAlpha(), 0.f);
    TestFalse(TEXT("Unrepaired interior cannot travel"), Lift->CanUseLiftInterior(Pawn));
    TestFalse(TEXT("Travel entry point rejects unrepaired lift"), Lift->TravelToTargetLevel(Pawn));

    // None is the existing unlocked-passage policy; no player save is required for this fixture.
    Lift->RequiredProgressId = NAME_None;
    Lift->RefreshPassage();
    Lift->Tick(.5f);
    TestEqual(TEXT("Repair unlock animates instead of snapping"), Lift->GetDoorOpenAlpha(), .5f);
    TestFalse(TEXT("Interior is unavailable while doors are moving"), Lift->CanUseLiftInterior(Pawn));
    Lift->Tick(.5f);
    TestEqual(TEXT("Doors reach fully open"), Lift->GetDoorOpenAlpha(), 1.f);
    TestTrue(TEXT("Inside a fully open lift permits travel"), Lift->CanUseLiftInterior(Pawn));
    TestEqual(TEXT("Left leaf slides along configured offset"), Lift->LeftDoorSlide->GetRelativeLocation(), Lift->LeftDoorOpenOffset);
    TestEqual(TEXT("Right leaf slides along configured offset"), Lift->RightDoorSlide->GetRelativeLocation(), Lift->RightDoorOpenOffset);
    TestEqual(TEXT("Left replacement mesh alignment survives animation"), Lift->LeftDoorMesh->GetRelativeLocation(), LeftAuthored);
    TestEqual(TEXT("Right replacement mesh alignment survives animation"), Lift->RightDoorMesh->GetRelativeLocation(), RightAuthored);
    Pawn->SetActorLocation(FVector(300,0,100));
    TestFalse(TEXT("Outside the entrance cannot travel"), Lift->CanUseLiftInterior(Pawn));
    TestFalse(TEXT("Direct travel call cannot bypass the interior"), Lift->TravelToTargetLevel(Pawn));
    Pawn->SetActorLocation(FVector(0,0,400));
    TestFalse(TEXT("Above the cabin cannot travel"), Lift->CanUseLiftInterior(Pawn));
    Lift->SetActorTransform(FTransform(FRotator(0,90,0),FVector(800,900,100),FVector(1.2f)));
    Pawn->SetActorLocation(Lift->TravelInterior->GetComponentLocation());
    TestTrue(TEXT("Interior follows actor rotation and scale"), Lift->CanUseLiftInterior(Pawn));
    TestEqual(TEXT("No local player means no outside interaction marker"), Lift->GetInteractionType(), ETunaSweeperInteractionType::None);
    Lift->RequiredProgressId = TEXT("test.unrepaired.lift");
    Lift->RefreshPassage(); Lift->Tick(2);
    TestEqual(TEXT("Reset progress closes doors"), Lift->GetDoorOpenAlpha(), 0.f);
    TestFalse(TEXT("Reset progress revokes travel"), Lift->CanUseLiftInterior(Pawn));
    Lift->bUseSlidingDoors = false;
    Lift->RequiredProgressId = NAME_None;
    Lift->RefreshPassage();
    TestEqual(TEXT("Ordinary passages retain travel interaction"), Lift->GetInteractionType(), ETunaSweeperInteractionType::LevelTravel);
    World->DestroyWorld(false); World->RemoveFromRoot();
    return true;
}
#endif
