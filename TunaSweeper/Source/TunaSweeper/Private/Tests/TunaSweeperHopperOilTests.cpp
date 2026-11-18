#if WITH_DEV_AUTOMATION_TESTS
#include "AI/Hopper/HopperOilImpactReceiver.h"
#include "Components/BoxComponent.h"
#include "Components/DecalComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHopperWreckOilTest, "TunaSweeper.Hopper.Combat.WreckOilImpacts",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHopperWreckOilTest::RunTest(const FString& Parameters)
{
    const auto Init = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(true)
        .CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
    UWorld* World = UWorld::CreateWorld(EWorldType::Game,false,MakeUniqueObjectName(GetTransientPackage(),UWorld::StaticClass(),TEXT("OilContact")),
        GetTransientPackage(),true,ERHIFeatureLevel::Num,&Init);
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    ON_SCOPE_EXIT { World->DestroyWorld(false); GEngine->DestroyWorldContext(World); World->RemoveFromRoot(); };
    auto* Source = World->SpawnActor<AActor>();
    auto* Ground = World->SpawnActor<AActor>();
    auto* Floor = NewObject<UBoxComponent>(Ground);
    Ground->SetRootComponent(Floor);
    Floor->SetBoxExtent(FVector(190,150,5));
    Floor->SetCollisionProfileName(TEXT("BlockAll"));
    Floor->SetWorldLocationAndRotation(FVector(180,-230,65),FRotator(12,27,0));
    Floor->RegisterComponent();

    const FVector Point = Floor->GetComponentTransform().TransformPosition(FVector(0,0,5));
    const FVector Offset(32,-64,0);
    const FVector InFlight(330,85,-470);
    const FVector Direction = FVector::VectorPlaneProject(InFlight.GetSafeNormal2D(),Floor->GetUpVector()).GetSafeNormal();
    FBasicParticleData Drop;
    Drop.Position = Point - Offset;
    Drop.Velocity = InFlight;
    for (bool bFan : {false,true})
    {
        const TCHAR* Path = bFan ? TEXT("/Game/Characters/Hopper/Mk2/FX/Wreck/MI_HopperMk2OilSplash.MI_HopperMk2OilSplash") :
            TEXT("/Game/Characters/Hopper/Mk2/FX/Wreck/M_HopperMk2OilImpact.M_HopperMk2OilImpact");
        auto* Mat = LoadObject<UMaterialInterface>(nullptr,Path);
        if (!TestNotNull(TEXT("Ground film loads"),Mat)) return false;
        auto* Receiver = NewObject<UHopperOilImpactReceiver>(Source);
        Receiver->Initialize(Source,Mat,bFan,FVector::RightVector);
        for (int32 i=0;i<4;++i) Receiver->ReceiveParticleData_Implementation({Drop},nullptr,Offset);
        auto Miss = Drop;
        Miss.Position += FVector(0,0,150);
        Receiver->ReceiveParticleData_Implementation({Miss},nullptr,Offset);
        TArray<UDecalComponent*> Marks;
        Ground->GetComponents(Marks);
        Marks.RemoveAll([bFan](auto* Mark){ return !Mark->ComponentHasTag(bFan?TEXT("HopperOilSplash"):TEXT("HopperOilPool")); });
        TSet<int32> Variants;
        TestEqual(TEXT("Four floor contacts and an airborne miss produce four stamps"),Marks.Num(),4);
        for (auto* Mark : Marks)
        {
            auto* Surface = Cast<UMaterialInstanceDynamic>(Mark->GetDecalMaterial());
            if (!TestNotNull(TEXT("Each stain has an independent impact clock"),Surface)) continue;
            Variants.Add(FMath::RoundToInt(Surface->K2_GetScalarParameterValue(TEXT("OilVariant"))));
            TestTrue(TEXT("Decal projects into the sloped floor"),Mark->GetForwardVector().Equals(Floor->GetUpVector(),.01));
            if (bFan)
            {
                TestTrue(TEXT("Wide spray orients its splash to the projected arrival"),Mark->GetUpVector().Equals(Direction,.01));
                TestTrue(TEXT("Thick oil spreads radially"),Mark->DecalSize.Y==Mark->DecalSize.Z && Mark->DecalSize.Z>=22.f);
            }
            TestTrue(TEXT("Both sizes spread around the actual hit"),Mark->GetComponentLocation().Equals(Point,.5));
        }
        TestEqual(TEXT("Each group uses all four generated patterns"),Variants.Num(),4);
    }
    return true;
}
#endif
