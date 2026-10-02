#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "LoopRailTrack.h"
#include "LoopRailTrain.h"
#include "LoopRailStation.h"
#include "LoopRailCrossing.h"
#include "Components/SplineComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInterface.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/WorldSettings.h"
#include "StaticMeshResources.h"
#include "FileHelpers.h"
#include "Editor.h"
#include "UObject/UnrealType.h"
#include "AssetRegistry/AssetRegistryModule.h"

namespace
{
struct FTestWorld
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    FTestWorld()
    { GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World); }
    ~FTestWorld() { GEngine->DestroyWorldContext(World); World->DestroyWorld(false); }
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLoopRailActorTest,"LoopRail.Runtime.StationsAndCrossings",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLoopRailActorTest::RunTest(const FString& Parameters)
{
    FTestWorld T;
    auto* Track = T.World->SpawnActor<ALoopRailTrack>();
    auto* Train = T.World->SpawnActor<ALoopRailTrain>();
    Train->Track = Track; Train->StartDistance = 4500; Train->RebuildTrain(); Train->ResetTrain();
    TestNotNull(TEXT("Four vehicle floors"),Train->GetVehicleFloor(3));
    TestNull(TEXT("Only four vehicle floors"),Train->GetVehicleFloor(4));
    TestTrue(TEXT("Long closed route is usable"),Train->HasUsableTrack());
    auto* Station = T.World->SpawnActor<ALoopRailStation>();
    Station->Track = Track; Station->DwellSeconds = 1;
    Station->SetActorLocation(Track->Sample(4700).GetLocation());
    bool bStopped = false;
    for (int32 I=0; I<1000; ++I) { Train->Tick(.02f); if (Train->IsDwelling()) { bStopped = true; break; } }
    TestTrue(TEXT("Train reaches station"),bStopped);
    TestTrue(TEXT("Precise stop location"),FMath::Abs(Train->GetHeadDistance()-Station->GetStopDistance())<.1);
    TestEqual(TEXT("Station speed is zero"),Train->GetSpeed(),0.f);
    const double StopD = Train->GetHeadDistance();
    Train->Tick(.5f); TestEqual(TEXT("Dwell holds pose"),Train->GetHeadDistance(),StopD);
    for (int32 I=0; I<100; ++I) Train->Tick(.02f);
    TestTrue(TEXT("Departs without repeatedly stopping at same station"),Train->GetHeadDistance()>StopD+20);
    Train->SetRunning(false); const double Paused = Train->GetHeadDistance(); Train->Tick(1);
    TestEqual(TEXT("Disabled operation holds train"),Train->GetHeadDistance(),Paused);
    auto* Crossing = T.World->SpawnActor<ALoopRailCrossing>(); Crossing->Track = Track;
    Crossing->WarningDistance = 0; Crossing->RearClearance = 0; Crossing->WarningLeadSeconds = 0;
    Crossing->SetActorLocation(Track->Sample(4000).GetLocation());
    Train->StartDistance = 4500; Train->ResetTrain(); Crossing->Tick(2);
    TestTrue(TEXT("Crossing stays closed under rear carriages"),Crossing->IsWarningActive());
    TestEqual(TEXT("Barrier fully lowered"),Crossing->GetBarrierOpenAlpha(),0.f);
    Train->StartDistance = 8500; Train->ResetTrain(); Crossing->Tick(2);
    TestFalse(TEXT("Crossing clears after last carriage"),Crossing->IsWarningActive());
    TestEqual(TEXT("Barrier reopens"),Crossing->GetBarrierOpenAlpha(),1.f);
    Train->VehicleCount = 2; Train->RebuildTrain();
    TestNotNull(TEXT("Two car configuration has a passenger car"),Train->GetVehicleFloor(1));
    TestNull(TEXT("Rebuild removes old cars"),Train->GetVehicleFloor(2));
    Track->Spline->SetClosedLoop(false); const double Before = Train->GetHeadDistance(); Train->Tick(1);
    TestFalse(TEXT("Open track rejected"),Train->HasUsableTrack());
    TestEqual(TEXT("Invalid route does not move train"),Train->GetHeadDistance(),Before);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLoopRailMeshTest,"LoopRail.Assets.VehicleBudgetAndAisle",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLoopRailMeshTest::RunTest(const FString& Parameters)
{
    for (const TCHAR* Name : {TEXT("Locomotive"),TEXT("Carriage")})
    {
        UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr,*FString::Printf(TEXT("/LoopRail/Meshes/SM_%s.SM_%s"),Name,Name));
        if (!TestNotNull(FString(Name)+TEXT(" saved mesh"),Mesh)) continue;
        if (!TestNotNull(TEXT("Render data"),Mesh->GetRenderData())) continue;
        const int32 TriangleBudget = FString(Name)==TEXT("Locomotive") ? 12000 : 16000;
        TestTrue(FString(Name)+TEXT(" within detail triangle budget"),Mesh->GetRenderData()->LODResources[0].GetNumTriangles()<=TriangleBudget);
        TestTrue(TEXT("9m body scale"),FMath::Abs(Mesh->GetBoundingBox().GetSize().X-900)<1);
        if (FString(Name)==TEXT("Carriage"))
        {
            auto* Roof=LoadObject<UStaticMesh>(nullptr,TEXT("/LoopRail/Meshes/SM_CarriageRoof.SM_CarriageRoof"));
            if (TestNotNull(TEXT("Optional roof"),Roof))
                TestTrue(TEXT("Carriage including roof stays within 16000 triangles"),Mesh->GetRenderData()->LODResources[0].GetNumTriangles()+Roof->GetRenderData()->LODResources[0].GetNumTriangles()<=16000);
        }
    }
    FTestWorld T;
    auto* Train = T.World->SpawnActor<ALoopRailTrain>();
    Train->VehicleCount=2; Train->RebuildTrain();
    const UBoxComponent* Floor=Train->GetVehicleFloor(1);
    if (!TestNotNull(TEXT("Passenger floor"),Floor)) return false;
    // Character-sized capsule travels through the entire central aisle and each end entrance.
    const FTransform Frame=Floor->GetComponentTransform();
    FHitResult Hit;
    const FCollisionShape Capsule=FCollisionShape::MakeCapsule(34,88);
    const FVector A=Frame.TransformPosition(FVector(-400,0,100)), B=Frame.TransformPosition(FVector(400,0,100));
    TestFalse(TEXT("68cm capsule clears central aisle"),T.World->SweepSingleByChannel(Hit,A,B,Frame.GetRotation(),ECC_Pawn,Capsule));
    for (double X : {-390.0,390.0})
        TestFalse(TEXT("Entrance clears capsule"),T.World->SweepSingleByChannel(Hit,Frame.TransformPosition(FVector(X,-170,100)),
            Frame.TransformPosition(FVector(X,170,100)),Frame.GetRotation(),ECC_Pawn,Capsule));
    int32 SeatHits=0;
    for (int32 Row=0;Row<8;++Row) for (double Side : {-1.0,1.0})
    {
        const FVector P=Frame.TransformPosition(FVector(-297.5+85*Row,Side*70,0));
        if (T.World->LineTraceSingleByChannel(Hit,P+FVector(0,0,200),P+FVector(0,0,20),ECC_Visibility)
            && Hit.ImpactPoint.Z>Floor->GetComponentLocation().Z+40) ++SeatHits;
    }
    TestEqual(TEXT("Sixteen solid seat positions"),SeatHits,16);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLoopRailTextureTest,"LoopRail.Assets.DetailedSurfaceTextures",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLoopRailTextureTest::RunTest(const FString& Parameters)
{
    for (const TCHAR* Name : {TEXT("Locomotive"),TEXT("Carriage"),TEXT("CarriageRoof"),TEXT("CarriageConnection")})
    {
        const FString MeshPath=FString::Printf(TEXT("/LoopRail/Meshes/SM_%s.SM_%s"),Name,Name);
        auto* Mesh=LoadObject<UStaticMesh>(nullptr,*MeshPath);
        if (!TestNotNull(TEXT("Detailed mesh exists"),Mesh) || !TestNotNull(TEXT("Detailed mesh render data"),Mesh->GetRenderData())) continue;
        const int32 ExportedTriangles=FString(Name)==TEXT("Locomotive") ? 4916 : FString(Name)==TEXT("Carriage") ? 9012 : FString(Name)==TEXT("CarriageRoof") ? 188 : 396;
        TestEqual(FString(Name)+TEXT(" matches the detailed FBX export"),Mesh->GetNumTriangles(0),ExportedTriangles);
        auto* Material=Mesh->GetMaterial(0);
        const FString MaterialPath=FString::Printf(TEXT("/LoopRail/Materials/M_%sDetail.M_%sDetail"),Name,Name);
        TestTrue(FString(Name)+TEXT(" uses its dedicated texture material"),Material && Material->GetPathName()==MaterialPath);
        const auto& Vertices=Mesh->GetRenderData()->LODResources[0].VertexBuffers.StaticMeshVertexBuffer;
        if (TestTrue(TEXT("UV channel exists"),Vertices.GetNumTexCoords()>0))
        {
            bool bFiniteAndInAtlas=true;
            FVector2f Minimum(1,1),Maximum(0,0);
            for (uint32 I=0;I<Vertices.GetNumVertices();++I)
            {
                const FVector2f UV=Vertices.GetVertexUV(I,0);
                bFiniteAndInAtlas &= FMath::IsFinite(UV.X) && FMath::IsFinite(UV.Y) && UV.X>=0 && UV.X<=1 && UV.Y>=0 && UV.Y<=1;
                Minimum.X=FMath::Min(Minimum.X,UV.X); Minimum.Y=FMath::Min(Minimum.Y,UV.Y);
                Maximum.X=FMath::Max(Maximum.X,UV.X); Maximum.Y=FMath::Max(Maximum.Y,UV.Y);
            }
            TestTrue(TEXT("Surface UVs stay inside the atlas"),bFiniteAndInAtlas);
            TestTrue(TEXT("UVs span the painted atlas"),Maximum.X-Minimum.X>.5f && Maximum.Y-Minimum.Y>.5f);
        }
        for (const TCHAR* Channel : {TEXT("BaseColor"),TEXT("Roughness"),TEXT("Metallic")})
        {
            const FString TexturePath=FString::Printf(TEXT("/LoopRail/Textures/T_%s_%s.T_%s_%s"),Name,Channel,Name,Channel);
            auto* Texture=LoadObject<UTexture2D>(nullptr,*TexturePath);
            if (TestNotNull(TEXT("Dedicated surface texture exists"),Texture))
            {
                TestTrue(TEXT("Imported texture source retains 2K surface detail"),Texture->Source.GetSizeX()>=2048 && Texture->Source.GetSizeY()>=2048);
                TestEqual(TEXT("Colour data is sRGB; physical masks are linear"),bool(Texture->SRGB),FString(Channel)==TEXT("BaseColor"));
            }
        }
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLoopRailRidingTest,"LoopRail.Runtime.MovingPassenger",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLoopRailRidingTest::RunTest(const FString& Parameters)
{
    FTestWorld T;
    auto* Track=T.World->SpawnActor<ALoopRailTrack>();
    auto* Train=T.World->SpawnActor<ALoopRailTrain>();
    Train->Track=Track; Train->StartDistance=4500; Train->CruiseSpeed=300; Train->bAutoRun=false;
    Train->RebuildTrain(); Train->ResetTrain();
    FActorSpawnParameters Spawn; Spawn.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    auto* Rider=T.World->SpawnActor<ACharacter>(ACharacter::StaticClass(),FVector::ZeroVector,FRotator::ZeroRotator,Spawn);
    Rider->GetCapsuleComponent()->SetCapsuleSize(34,88);
    Rider->GetCharacterMovement()->bRunPhysicsWithNoController=true;
    Rider->SetActorLocation(Train->GetVehicleFloor(1)->GetComponentLocation()+FVector(0,0,100));
    T.World->InitializeActorsForPlay(FURL()); T.World->BeginPlay();
    // This isolated world has no GameMode, so explicitly dispatch the world-settings start notification.
    T.World->GetWorldSettings()->NotifyBeginPlay();
    for(int32 I=0;I<20;++I) { ++GFrameCounter; T.World->Tick(LEVELTICK_All,.02f); }
    TestTrue(TEXT("Rider finds passenger floor"),Rider->GetMovementBase()==Train->GetVehicleFloor(1));
    const FVector InitialLocal=Train->GetVehicleFloor(1)->GetComponentTransform().InverseTransformPosition(Rider->GetActorLocation());
    const FVector InitialWorld=Rider->GetActorLocation();
    Train->SetRunning(true);
    for(int32 I=0;I<200;++I) { ++GFrameCounter; T.World->Tick(LEVELTICK_All,.02f); }
    const FVector FinalLocal=Train->GetVehicleFloor(1)->GetComponentTransform().InverseTransformPosition(Rider->GetActorLocation());
    AddInfo(FString::Printf(TEXT("Ride: head %.1f speed %.1f world delta %.1f local drift %.2f begun=%d trainBegun=%d tick=%d"),Train->GetHeadDistance(),Train->GetSpeed(),FVector::Dist(InitialWorld,Rider->GetActorLocation()),FVector::Dist(InitialLocal,FinalLocal),T.World->HasBegunPlay(),Train->HasActorBegunPlay(),Train->IsActorTickEnabled()));
    TestTrue(TEXT("Passenger travels with train"),FVector::Dist(InitialWorld,Rider->GetActorLocation())>500);
    TestTrue(TEXT("Passenger holds location relative to moving floor"),FVector::Dist(InitialLocal,FinalLocal)<10);
    TestTrue(TEXT("Passenger remains based"),Rider->GetMovementBase()==Train->GetVehicleFloor(1));
    bool bWrapped=false; double Previous=Train->GetHeadDistance(); double MaxDrift=0;
    const int32 LapFrames=FMath::CeilToInt(Track->GetLength()/300/.02)+100;
    for(int32 I=0;I<LapFrames;++I)
    {
        ++GFrameCounter; T.World->Tick(LEVELTICK_All,.02f);
        bWrapped|=Train->GetHeadDistance()<Previous; Previous=Train->GetHeadDistance();
        const FVector Local=Train->GetVehicleFloor(1)->GetComponentTransform().InverseTransformPosition(Rider->GetActorLocation());
        MaxDrift=FMath::Max(MaxDrift,FVector::Dist(InitialLocal,Local));
    }
    TestTrue(TEXT("Passenger completed route seam"),bWrapped);
    TestTrue(TEXT("Passenger stays on floor through curves and full lap"),MaxDrift<10 && Rider->GetMovementBase()==Train->GetVehicleFloor(1));
    AddInfo(FString::Printf(TEXT("Full lap maximum passenger drift %.3f cm"),MaxDrift));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLoopRailRepStateTest,"LoopRail.Runtime.ReplicatedDwell",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLoopRailRepStateTest::RunTest(const FString& Parameters)
{
    FTestWorld T;
    auto* Train=T.World->SpawnActor<ALoopRailTrain>();
    T.World->InitializeActorsForPlay(FURL());
    FStructProperty* Property=FindFProperty<FStructProperty>(ALoopRailTrain::StaticClass(),TEXT("RepState"));
    UFunction* Notify=Train->FindFunction(TEXT("OnRep_State"));
    if(!TestNotNull(TEXT("Replicated state property"),Property)||!TestNotNull(TEXT("State notification"),Notify)) return false;
    auto* State=Property->ContainerPtrToValuePtr<FLoopRailRepState>(Train);
    State->DwellSeconds=8; State->Distance=5000; State->Speed=0;
    Train->ProcessEvent(Notify,nullptr);
    TestTrue(TEXT("Receiving a station stop updates public dwell query"),Train->IsDwelling());
    State->DwellSeconds=0; State->Speed=120;
    Train->ProcessEvent(Notify,nullptr);
    TestFalse(TEXT("Receiving departure clears dwell query"),Train->IsDwelling());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLoopRailPreviewMapTest,"LoopRail.Assets.SavedPreviewMap",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLoopRailPreviewMapTest::RunTest(const FString& Parameters)
{
    if(!TestTrue(TEXT("Preview map loads from plugin"),FEditorFileUtils::LoadMap(TEXT("/LoopRail/Maps/LoopRailPreview"),false,true))) return false;
    UWorld* World=GEditor->GetEditorWorldContext().World();
    int32 Trains=0,Stations=0,Crossings=0;
    for(TActorIterator<ALoopRailTrain> It(World);It;++It)
    {
        ++Trains; TestTrue(TEXT("Saved train has usable track reference"),It->HasUsableTrack());
        It->RebuildTrain(); It->RebuildTrain();
        for (int32 Car=0;Car<4;++Car) TestNotNull(TEXT("Saved vehicle floor survives rebuilding"),It->GetVehicleFloor(Car));
        TestNull(TEXT("Loading and rebuilding keeps exactly four vehicle floors"),It->GetVehicleFloor(4));
        TInlineComponentArray<UBoxComponent*> Boxes(*It);
        int32 VehicleFloors=0,ConnectionFloors=0;
        for (const UBoxComponent* Box:Boxes)
        {
            VehicleFloors+=Box->GetName().StartsWith(TEXT("VehicleFloor_")) ? 1 : 0;
            ConnectionFloors+=Box->ComponentHasTag(TEXT("LoopRailConnectionFloor")) ? 1 : 0;
        }
        TestEqual(TEXT("Rebuilding does not leave duplicate vehicle floors"),VehicleFloors,4);
        TestEqual(TEXT("Saved four-car train has exactly three connection floors"),ConnectionFloors,3);
    }
    for(TActorIterator<ALoopRailStation> It(World);It;++It) { ++Stations; TestNotNull(TEXT("Saved station track reference"),It->Track.Get()); }
    for(TActorIterator<ALoopRailCrossing> It(World);It;++It) { ++Crossings; TestNotNull(TEXT("Saved crossing track reference"),It->Track.Get()); }
    TestEqual(TEXT("One saved train"),Trains,1); TestEqual(TEXT("Two saved stations"),Stations,2); TestEqual(TEXT("One saved crossing"),Crossings,1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLoopRailDependencyTest,"LoopRail.Assets.NoProjectDependencies",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLoopRailDependencyTest::RunTest(const FString& Parameters)
{
    auto& Registry=FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
    Registry.ScanPathsSynchronous({TEXT("/LoopRail")},true);
    TArray<FAssetData> Assets; Registry.GetAssetsByPath(TEXT("/LoopRail"),Assets,true);
    TestTrue(TEXT("Plugin assets discovered"),Assets.Num()>=8);
    for(const auto& Asset:Assets)
    {
        TArray<FName> Dependencies; Registry.GetDependencies(Asset.PackageName,Dependencies);
        for(FName Dependency:Dependencies)
        {
            const FString Path=Dependency.ToString();
            TestTrue(FString::Printf(TEXT("%s has portable dependency %s"),*Asset.PackageName.ToString(),*Path),
                Path.StartsWith(TEXT("/LoopRail/"))||Path.StartsWith(TEXT("/Engine/"))||Path==TEXT("/Script/Engine")
                ||Path==TEXT("/Script/CoreUObject")||Path==TEXT("/Script/LoopRail")
                ||Path==TEXT("/Script/NavigationSystem")||Path==TEXT("/Script/StaticMeshDescription")
                ||Path==TEXT("/Script/InterchangeEngine")); // Engine-owned editor import metadata.
        }
    }
    return true;
}
#endif
