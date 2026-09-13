#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "StaticMeshResources.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Interaction/TunaSweeperInteractableComponent.h"
#include "Interaction/TunaSweeperWorldProgressActor.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionConstant3Vector.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperBunkerPipeTest,
    "TunaSweeper.WorldProgress.BunkerPipe",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaSweeperBunkerPipeTest::RunTest(const FString& Parameters)
{
    UClass* BrokenClass = LoadClass<ATunaSweeperWorldProgressActor>(nullptr,
        TEXT("/Game/Interaction/BunkerPipe/BP_BunkerPipe_Broken.BP_BunkerPipe_Broken_C"));
    UClass* RepairedClass = LoadClass<AStaticMeshActor>(nullptr,
        TEXT("/Game/Interaction/BunkerPipe/BP_BunkerPipe_Repaired.BP_BunkerPipe_Repaired_C"));
    if (!TestNotNull(TEXT("Broken BP loads"), BrokenClass) ||
        !TestNotNull(TEXT("Repaired BP loads"), RepairedClass)) return false;
    for (const TCHAR* Name : { TEXT("SM_BunkerPipe_Broken"), TEXT("SM_BunkerPipe_Repaired") })
    {
        auto* Mesh = LoadObject<UStaticMesh>(nullptr, *(FString(TEXT("/Game/Interaction/BunkerPipe/")) + Name));
        if (!TestNotNull(TEXT("Cylinder loads"), Mesh)) return false;
        const auto& LOD = Mesh->GetRenderData()->LODResources[0];
        int32 InwardNormals = 0;
        for (uint32 I = 0; I < LOD.VertexBuffers.PositionVertexBuffer.GetNumVertices(); ++I)
        {
            const FVector3f P = LOD.VertexBuffers.PositionVertexBuffer.VertexPosition(I) - FVector3f(0, 0, 120);
            const FVector3f N = LOD.VertexBuffers.StaticMeshVertexBuffer.VertexTangentZ(I);
            if (FVector3f::DotProduct(P, N) <= 0.f) ++InwardNormals;
        }
        TestEqual(FString(Name) + TEXT(" side and cap normals face outward"), InwardNormals, 0);
    }
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    UTunaSweeperGameInstance* GI = NewObject<UTunaSweeperGameInstance>(World);
    World->SetGameInstance(GI);
    GI->ConsumeInventoryItemById(6005, GI->CountInventoryItemById(6005));
    const FTransform Transform(FRotator(0, 35, 0), FVector(100, 200, 0), FVector(1.5, 1.5, 1.5));
    auto* Pipe = World->SpawnActor<ATunaSweeperWorldProgressActor>(BrokenClass, Transform);
    if (!TestNotNull(TEXT("Pipe spawns"), Pipe)) { World->DestroyWorld(false); return false; }
    TestEqual(TEXT("Waterproof tape item"), Pipe->GetRequiredItemId(), 6005);
    TestEqual(TEXT("One tape"), Pipe->GetRequiredQuantity(), 1);
    TestEqual(TEXT("Quest event"), Pipe->GetInteractableComponent()->GetObjectiveEventId(), FName(TEXT("demo.bunker_pipe.repair")));
    auto* Visual = Pipe->FindComponentByClass<UStaticMeshComponent>();
    TestEqual(TEXT("No bridge override after construction"), Visual->GetStaticMesh()->GetName(), FString(TEXT("SM_BunkerPipe_Broken")));
    TestEqual(TEXT("Red material"), Visual->GetMaterial(0)->GetName(), FString(TEXT("M_BunkerPipe_Broken_Red")));
    TestTrue(TEXT("Vertical 240cm mesh"), FMath::IsNearlyEqual(Visual->GetStaticMesh()->GetBounds().BoxExtent.Z, 120.f));
    TestTrue(TEXT("Collision centered on pipe"), Pipe->FindComponentByClass<UBoxComponent>()->GetRelativeLocation().Equals(FVector(0, 0, 120)));
    TestFalse(TEXT("No tape cannot repair"), Pipe->RepairUsingAvailableRequiredItems(false));
    // Use an already contributed item to exercise completion without inventory or disk side effects.
    const FName ObjectId(TEXT("test.bunker_pipe"));
    Pipe->ConfigureWorldProgressDefaults(ObjectId, TEXT("bunker_pipe"), FText::FromString(TEXT("Pipe")),
        FText::FromString(TEXT("Repair")), 6005, 1, 0, FText::FromString(TEXT("Tape")),
        FVector(20,20,120), TSoftClassPtr<AActor>(RepairedClass));
    GI->UpdateWorldProgressState(ObjectId, TEXT("bunker_pipe"), ETunaSweeperWorldProgressState::InProgress, 1, 1, false);
    TestTrue(TEXT("Contributed tape permits repair"), Pipe->RepairUsingAvailableRequiredItems(false));
    FTunaSweeperWorldProgressSaveData Saved;
    TestTrue(TEXT("Saved progress exists"), GI->TryGetWorldProgressState(ObjectId, Saved));
    TestTrue(TEXT("Saved completion"), Saved.State == ETunaSweeperWorldProgressState::Completed);
    int32 Count = 0;
    for (TActorIterator<AStaticMeshActor> It(World, RepairedClass); It; ++It)
    {
        ++Count;
        TestTrue(TEXT("Replacement preserves transform and scale"), It->GetActorTransform().Equals(Transform));
        TestEqual(TEXT("Repaired mesh"), It->GetStaticMeshComponent()->GetStaticMesh()->GetName(), FString(TEXT("SM_BunkerPipe_Repaired")));
        TestEqual(TEXT("Gray material"), It->GetStaticMeshComponent()->GetMaterial(0)->GetName(), FString(TEXT("M_BunkerPipe_Repaired_Gray")));
        It->Destroy();
    }
    TestEqual(TEXT("One replacement"), Count, 1);
    auto* Restored = World->SpawnActor<ATunaSweeperWorldProgressActor>(BrokenClass, Transform);
    Restored->ConfigureWorldProgressDefaults(ObjectId, TEXT("bunker_pipe"), FText::FromString(TEXT("Pipe")),
        FText::FromString(TEXT("Repair")), 6005, 1, 0, FText::FromString(TEXT("Tape")),
        FVector(20,20,120), TSoftClassPtr<AActor>(RepairedClass));
    Restored->DispatchBeginPlay();
    TestTrue(TEXT("Saved completion replaces damaged pipe on begin play"), Restored->IsActorBeingDestroyed());
    World->DestroyWorld(false);
    return true;
}
#endif
