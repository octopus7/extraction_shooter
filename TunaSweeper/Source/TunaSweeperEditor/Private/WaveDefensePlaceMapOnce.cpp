#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "FileHelpers.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "NavigationSystem.h"
#include "NavMesh/NavMeshBoundsVolume.h"
#include "ActorFactories/ActorFactory.h"
#include "Builders/CubeBuilder.h"
#include "WaveDefense/WaveDefenseDirector.h"
#include "WaveDefense/WaveDefenseDefinition.h"
#include "WaveDefense/WaveDefenseCore.h"
#include "WaveDefense/WaveDefenseFacility.h"
#include "WaveDefense/WaveDefenseSpawnPoint.h"
#include "WaveDefense/WaveDefenseTerminal.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWaveDefensePlaceMap, "TunaSweeper.WaveDefense.PlaceMapOnce", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWaveDefensePlaceMap::RunTest(const FString& Parameters)
{
    const FString MapPath(TEXT("/Game/MainRaid/RaidPlains"));
    UWorld* World = UEditorLoadingAndSavingUtils::LoadMap(MapPath);
    if (!TestNotNull(TEXT("Load Plains"), World)) return false;
    TMap<TWeakObjectPtr<AActor>, FTransform> Existing;
    for (TActorIterator<AActor> It(World); It; ++It)
    {
        if (It->GetActorLabel().StartsWith(TEXT("WD_Plains_"))) { AddError(TEXT("Wave placement already exists")); return false; }
        Existing.Add(*It, It->GetActorTransform());
    }
    const TCHAR* Prefix = TEXT("/Game/MainRaid/WaveDefense/");
    auto Class = [Prefix](const TCHAR* Name, UClass* Base)
    {
        return StaticLoadClass(Base, nullptr, *(FString::Printf(TEXT("%s%s.%s_C"), Prefix, Name, Name)));
    };
    UClass* CoreClass = Class(TEXT("BP_WaveDefenseCore"), AWaveDefenseCore::StaticClass());
    UClass* DirectorClass = Class(TEXT("BP_WaveDefenseDirector"), AWaveDefenseDirector::StaticClass());
    UClass* TerminalClass = Class(TEXT("BP_WaveDefenseTerminal"), AWaveDefenseTerminal::StaticClass());
    UClass* SpawnClass = Class(TEXT("BP_WaveDefenseSpawnPoint"), AWaveDefenseSpawnPoint::StaticClass());
    UClass* BarrierClass = Class(TEXT("BP_WaveDefenseBarrier"), AWaveDefenseFacility::StaticClass());
    UClass* TurretClass = Class(TEXT("BP_WaveDefenseTurret"), AWaveDefenseFacility::StaticClass());
    auto* Definition = LoadObject<UWaveDefenseDefinition>(nullptr, TEXT("/Game/MainRaid/WaveDefense/DA_PlainsWaveDefense.DA_PlainsWaveDefense"));
    if (!CoreClass || !DirectorClass || !TerminalClass || !SpawnClass || !BarrierClass || !TurretClass || !Definition) { AddError(TEXT("Missing authored assets")); return false; }
    auto Ground = [World](float X, float Y, float& Z)
    {
        FHitResult Hit; FCollisionQueryParams Query(SCENE_QUERY_STAT(WaveDefensePlacement), false);
        if (!World->LineTraceSingleByChannel(Hit, FVector(X,Y,5000), FVector(X,Y,-5000), ECC_Visibility, Query)) return false;
        Z = Hit.ImpactPoint.Z; return true;
    };
    const TArray<FVector2D> Samples = {{5000,0},{4400,0},{8200,-400},{8200,400},{4600,-3200},{5400,-3200},{4600,3200},{5400,3200},{6200,0},{5000,-1200},{5000,1200},{5850,400},{4600,-850},{4600,850}};
    TArray<float> Heights;
    for (const FVector2D& Point : Samples) { float Z = 0; if (!Ground(Point.X, Point.Y, Z)) { AddError(TEXT("Missing ground")); return false; } Heights.Add(Z); }
    FActorSpawnParameters Spawn; Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    auto* Core = World->SpawnActor<AWaveDefenseCore>(CoreClass, FVector(5000,0,Heights[0]+75), FRotator::ZeroRotator, Spawn);
    auto* Director = World->SpawnActor<AWaveDefenseDirector>(DirectorClass, FVector(5000,0,Heights[0]), FRotator::ZeroRotator, Spawn);
    auto* Terminal = World->SpawnActor<AWaveDefenseTerminal>(TerminalClass, FVector(4400,0,Heights[1]), FRotator::ZeroRotator, Spawn);
    if (!Core || !Director || !Terminal) { AddError(TEXT("Core/terminal/director spawn failed")); return false; }
    Core->SetActorLabel(TEXT("WD_Plains_Core")); Director->SetActorLabel(TEXT("WD_Plains_Director")); Terminal->SetActorLabel(TEXT("WD_Plains_Terminal"));
    TArray<AWaveDefenseSpawnPoint*> Points;
    const FName LaneNames[] = {TEXT("north"),TEXT("north"),TEXT("west"),TEXT("west"),TEXT("east"),TEXT("east")};
    for (int32 Index = 0; Index < 6; ++Index)
    {
        const FVector2D P = Samples[Index+2];
        auto* Point = World->SpawnActor<AWaveDefenseSpawnPoint>(SpawnClass, FVector(P.X,P.Y,Heights[Index+2]+20), FRotator::ZeroRotator, Spawn);
        if (!Point) { AddError(TEXT("Spawn point creation failed")); return false; }
        Point->LaneId = LaneNames[Index]; Point->SetActorLabel(FString::Printf(TEXT("WD_Plains_%s_%d"), *LaneNames[Index].ToString(), Index+1)); Points.Add(Point);
    }
    TArray<FWaveDefenseFacilitySlot> Slots;
    const FName Ids[] = {TEXT("north.barrier"),TEXT("west.barrier"),TEXT("east.barrier"),TEXT("north.turret"),TEXT("west.turret"),TEXT("east.turret")};
    for (int32 Index = 0; Index < 6; ++Index)
    {
        FWaveDefenseFacilitySlot Slot; Slot.SlotId = Ids[Index]; Slot.Kind = Index < 3 ? EWaveDefenseFacilityKind::Barrier : EWaveDefenseFacilityKind::Turret;
        const FVector2D P = Samples[Index+8];
        Slot.LocalTransform = FTransform(FRotator(0, Index == 1 || Index == 2 ? 90 : 0, 0), FVector(P.X-5000,P.Y,Heights[Index+8]+75-Heights[0]));
        Slots.Add(Slot);
    }
    Director->ConfigurePlacement(Definition, Core, Points, Slots, BarrierClass, TurretClass); Terminal->SetDirector(Director);
    auto* Nav = World->SpawnActor<ANavMeshBoundsVolume>(ANavMeshBoundsVolume::StaticClass(), FVector(5000,0,400), FRotator::ZeroRotator, Spawn);
    if (!Nav) { AddError(TEXT("Navigation volume spawn failed")); return false; }
    Nav->SetActorLabel(TEXT("WD_Plains_Navigation"));
    auto* Builder = NewObject<UCubeBuilder>(); Builder->X=9000; Builder->Y=9000; Builder->Z=2000;
    UActorFactory::CreateBrushForVolumeActor(Nav, Builder);
    if (auto* NavSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World)) { NavSystem->OnNavigationBoundsUpdated(Nav); NavSystem->Build(); }
    for (const auto& Pair : Existing) if (!Pair.Key.IsValid() || !Pair.Key->GetActorTransform().Equals(Pair.Value)) { AddError(TEXT("Existing actor changed")); return false; }
    World->MarkPackageDirty();
    return TestTrue(TEXT("Saved Plains map"), UEditorLoadingAndSavingUtils::SaveMap(World, MapPath));
}
#endif
