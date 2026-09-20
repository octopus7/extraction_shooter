#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/App.h"
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
#include "Particles/ParticleSystemComponent.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleEmitter.h"
#include "Particles/ParticleLODLevel.h"
#include "Particles/ParticleModuleRequired.h"
#include "PhysicsEngine/BodySetup.h"

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
    for (const TCHAR* Name : { TEXT("SM_BunkerPipe_LowPoly_Damaged"), TEXT("SM_BunkerPipe_LowPoly_Normal") })
    {
        auto* Mesh = LoadObject<UStaticMesh>(nullptr, *(FString(TEXT("/Game/Interaction/BunkerPipe/")) + Name));
        if (!TestNotNull(TEXT("Low-poly elbow pipe loads"), Mesh)) return false;
        const auto& LOD = Mesh->GetRenderData()->LODResources[0];
        TestTrue(TEXT("Low-poly triangle budget"), LOD.GetNumTriangles() > 100 && LOD.GetNumTriangles() < 1600);
        TestTrue(TEXT("UV coordinates exist for generated state texture"), LOD.VertexBuffers.StaticMeshVertexBuffer.GetNumTexCoords() > 0);
        TestTrue(TEXT("Waist-height pipe"), FMath::IsNearlyEqual(Mesh->GetBounds().BoxExtent.Z * 2.f, 107.1f, .1f));
        TestTrue(TEXT("Replacement retains solid collision"), Mesh->GetBodySetup() && Mesh->GetBodySetup()->AggGeom.GetElementCount() > 0);
        int32 InvalidNormals = 0;
        for (uint32 I = 0; I < LOD.VertexBuffers.PositionVertexBuffer.GetNumVertices(); ++I)
        {
            const FVector3f N = LOD.VertexBuffers.StaticMeshVertexBuffer.VertexTangentZ(I);
            if (N.ContainsNaN() || !FMath::IsNearlyEqual(N.SizeSquared(),1.f,.02f)) ++InvalidNormals;
        }
        TestEqual(FString(Name) + TEXT(" valid surface normals"), InvalidNormals, 0);
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
    auto* Visual = Pipe->FindComponentByClass<UStaticMeshComponent>();
    const FName ObjectId(TEXT("test.bunker_pipe"));
    Pipe->ConfigureWorldProgressDefaults(ObjectId, TEXT("bunker_pipe"), FText::FromString(TEXT("Pipe")),
        FText::FromString(TEXT("Repair")), 6005, 1, 0, FText::FromString(TEXT("Tape")),
        FVector(20,20,54), TSoftClassPtr<AActor>(RepairedClass));
    Pipe->DispatchBeginPlay();
    auto* Leak = Pipe->FindComponentByClass<UParticleSystemComponent>();
    if (!TestNotNull(TEXT("Leak emitter component exists"), Leak)) { World->DestroyWorld(false); return false; }
    TestNotNull(TEXT("Authored leak particle system assigned"), Leak->Template.Get());
    TestFalse(TEXT("No leak before cleaning"), Leak->IsVisible());
    TestFalse(TEXT("No active emission before cleaning"), Leak->IsActive());
    TestEqual(TEXT("Intact before crowbar cleaning"), Visual->GetStaticMesh()->GetName(), FString(TEXT("SM_BunkerPipe_LowPoly_Normal")));
    TestEqual(TEXT("Intact textured material before cleaning"), Visual->GetMaterial(0)->GetName(), FString(TEXT("M_BunkerPipe_Normal")));
    TestEqual(TEXT("No repair interaction before cleaning"), Pipe->GetInteractableComponent()->GetInteractionType(), ETunaSweeperInteractionType::None);
    TestTrue(TEXT("No quest event before cleaning"), Pipe->GetInteractableComponent()->GetObjectiveEventId().IsNone());
    GI->UpdateWorldProgressState(ObjectId, TEXT("bunker_pipe"), ETunaSweeperWorldProgressState::InProgress, 1, 1, false);
    TestFalse(TEXT("Contributed tape does not bypass cleaning readiness"), Pipe->IsRepairReady());
    TestFalse(TEXT("Direct repair does not bypass cleaning"), Pipe->Repair(false));
    TestFalse(TEXT("Repair cannot bypass cleaning"), Pipe->RepairUsingAvailableRequiredItems(false));
    TestEqual(TEXT("Cannot contribute before cleaning"), Pipe->UseAvailableRequiredItems(false), 0);
    GI->UpdateWorldProgressState(ObjectId, TEXT("bunker_pipe"), ETunaSweeperWorldProgressState::InProgress, 0, 1, false);
    GI->UpdateWorldProgressState(TEXT("demo.water_intake.blocked_screen"), NAME_None,
        ETunaSweeperWorldProgressState::Completed, 1, 1, false);
    TestEqual(TEXT("Cleaning enables repair immediately"), Pipe->GetInteractableComponent()->GetInteractionType(), ETunaSweeperInteractionType::WorldProgress);
    TestTrue(TEXT("Damage enables visible leak"), Leak->IsVisible());
    if (FApp::CanEverRender())
    {
        TestTrue(TEXT("Damage starts emitter"), Leak->IsActive());
    }
    if (Leak->Template && TestEqual(TEXT("Exactly one simple water emitter"), Leak->Template->Emitters.Num(), 1))
    {
        TestEqual(TEXT("Water tails follow particle velocity"), Leak->Template->Emitters[0]->LODLevels[0]->RequiredModule->ScreenAlignment.GetValue(), PSA_Velocity);
    }
    TestEqual(TEXT("Quest event"), Pipe->GetInteractableComponent()->GetObjectiveEventId(), FName(TEXT("demo.bunker_pipe.repair")));
    TestEqual(TEXT("No bridge override after construction"), Visual->GetStaticMesh()->GetName(), FString(TEXT("SM_BunkerPipe_LowPoly_Damaged")));
    TestEqual(TEXT("Damaged texture material"), Visual->GetMaterial(0)->GetName(), FString(TEXT("M_BunkerPipe_Damaged")));
    TestTrue(TEXT("Collision centered on waist-high pipe"), Pipe->FindComponentByClass<UBoxComponent>()->GetRelativeLocation().Equals(FVector(0, 0, 54)));
    TestFalse(TEXT("No tape cannot repair"), Pipe->RepairUsingAvailableRequiredItems(false));
    auto* ReloadedDamaged = World->SpawnActor<ATunaSweeperWorldProgressActor>(BrokenClass, Transform);
    ReloadedDamaged->DispatchBeginPlay();
    TestTrue(TEXT("Saved cleaning enables leak on spawn"), ReloadedDamaged->FindComponentByClass<UParticleSystemComponent>()->IsVisible());
    ReloadedDamaged->Destroy();
    // Use an already contributed item to exercise completion without inventory or disk side effects.
    Pipe->ConfigureWorldProgressDefaults(ObjectId, TEXT("bunker_pipe"), FText::FromString(TEXT("Pipe")),
        FText::FromString(TEXT("Repair")), 6005, 1, 0, FText::FromString(TEXT("Tape")),
        FVector(20,20,120), TSoftClassPtr<AActor>(RepairedClass));
    GI->UpdateWorldProgressState(ObjectId, TEXT("bunker_pipe"), ETunaSweeperWorldProgressState::InProgress, 1, 1, false);
    TestTrue(TEXT("Contributed tape permits repair"), Pipe->RepairUsingAvailableRequiredItems(false));
    TestFalse(TEXT("Repair immediately hides water tails"), Leak->IsVisible());
    FTunaSweeperWorldProgressSaveData Saved;
    TestTrue(TEXT("Saved progress exists"), GI->TryGetWorldProgressState(ObjectId, Saved));
    TestTrue(TEXT("Saved completion"), Saved.State == ETunaSweeperWorldProgressState::Completed);
    int32 Count = 0;
    for (TActorIterator<AStaticMeshActor> It(World, RepairedClass); It; ++It)
    {
        ++Count;
        TestTrue(TEXT("Replacement preserves transform and scale"), It->GetActorTransform().Equals(Transform));
        TestEqual(TEXT("Repaired mesh"), It->GetStaticMeshComponent()->GetStaticMesh()->GetName(), FString(TEXT("SM_BunkerPipe_LowPoly_Normal")));
        TestEqual(TEXT("Normal texture material"), It->GetStaticMeshComponent()->GetMaterial(0)->GetName(), FString(TEXT("M_BunkerPipe_Normal")));
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
