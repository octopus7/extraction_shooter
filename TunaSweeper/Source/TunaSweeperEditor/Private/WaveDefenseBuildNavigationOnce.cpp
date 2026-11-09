#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "FileHelpers.h"
#include "Editor.h"
#include "EngineUtils.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "NavMesh/NavMeshBoundsVolume.h"
#include "WaveDefense/WaveDefenseCore.h"
#include "WaveDefense/WaveDefenseSpawnPoint.h"

DEFINE_LATENT_AUTOMATION_COMMAND_ONE_PARAMETER(FWaveDefenseBuildNavStep, FAutomationTestBase*, Test);
bool FWaveDefenseBuildNavStep::Update()
{
    UWorld* World = GEditor->GetEditorWorldContext().World();
    auto* Nav = World ? FNavigationSystem::GetCurrent<UNavigationSystemV1>(World) : nullptr;
    if (!Test->TestNotNull(TEXT("Navigation system"), Nav)) return true;
    Nav->Build();
    return true;
}
DEFINE_LATENT_AUTOMATION_COMMAND_ONE_PARAMETER(FWaveDefenseVerifyAndSaveNav, FAutomationTestBase*, Test);
bool FWaveDefenseVerifyAndSaveNav::Update()
{
    UWorld* World = GEditor->GetEditorWorldContext().World();
    auto* Nav = World ? FNavigationSystem::GetCurrent<UNavigationSystemV1>(World) : nullptr;
    if (!World || !Nav) { Test->AddError(TEXT("Navigation unavailable")); return true; }
    AWaveDefenseCore* Core = nullptr;
    TArray<AWaveDefenseSpawnPoint*> Points;
    for (TActorIterator<AWaveDefenseCore> It(World); It; ++It) Core = *It;
    for (TActorIterator<AWaveDefenseSpawnPoint> It(World); It; ++It) Points.Add(*It);
    if (!Core || Points.Num() != 6) { Test->AddError(TEXT("Missing wave placement")); return true; }
    FNavLocation End;
    if (!Nav->ProjectPointToNavigation(Core->GetActorLocation(), End, FVector(500,500,500))) { Test->AddError(TEXT("Core cannot be projected")); return true; }
    bool bAll = true;
    for (AWaveDefenseSpawnPoint* Point : Points)
    {
        FNavLocation Start;
        const bool bProjected = Nav->ProjectPointToNavigation(Point->GetActorLocation(), Start, FVector(150,150,300));
        const UNavigationPath* Path = bProjected ? UNavigationSystemV1::FindPathToLocationSynchronously(World, Start.Location, End.Location) : nullptr;
        const bool bValid = Path && Path->IsValid() && !Path->IsPartial();
        bAll &= bValid;
        Test->TestTrue(*FString::Printf(TEXT("Reachable: %s"), *Point->GetActorLabel()), bValid);
    }
    if (bAll) Test->TestTrue(TEXT("Save built navigation"), UEditorLoadingAndSavingUtils::SaveMap(World, TEXT("/Game/MainRaid/RaidPlains")));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWaveDefenseBuildNavigationOnce, "TunaSweeper.WaveDefense.BuildNavigationOnce", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWaveDefenseBuildNavigationOnce::RunTest(const FString& Parameters)
{
    UWorld* World = UEditorLoadingAndSavingUtils::LoadMap(TEXT("/Game/MainRaid/RaidPlains"));
    if (!TestNotNull(TEXT("Load Plains"), World)) return false;
    auto* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
    if (!TestNotNull(TEXT("Navigation system"), Nav)) return false;
    int32 Count = 0;
    for (TActorIterator<ANavMeshBoundsVolume> It(World); It; ++It) if (It->GetActorLabel() == TEXT("WD_Plains_Navigation")) { Nav->OnNavigationBoundsUpdated(*It); ++Count; }
    if (!TestEqual(TEXT("Wave navigation volume"), Count, 1)) return false;
    ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(2.0f));
    ADD_LATENT_AUTOMATION_COMMAND(FWaveDefenseBuildNavStep(this));
    ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(3.0f));
    ADD_LATENT_AUTOMATION_COMMAND(FWaveDefenseVerifyAndSaveNav(this));
    return true;
}
#endif
