#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "FileHelpers.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWaveDefensePlaceCoverOnce, "TunaSweeper.WaveDefense.PlaceCoverOnce", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWaveDefensePlaceCoverOnce::RunTest(const FString& Parameters)
{
    UWorld* World=UEditorLoadingAndSavingUtils::LoadMap(TEXT("/Game/MainRaid/RaidPlains"));
    if (!TestNotNull(TEXT("Plains map"),World)) return false;
    TMap<TWeakObjectPtr<AActor>,FTransform> Existing;
    for (TActorIterator<AActor> It(World); It; ++It) { if (It->GetActorLabel().StartsWith(TEXT("WD_Plains_Cover_"))) { AddError(TEXT("Cover already exists")); return false; } Existing.Add(*It,It->GetActorTransform()); }
    UStaticMesh* Mesh=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (!TestNotNull(TEXT("Cover mesh"),Mesh)) return false;
    const FVector2D Positions[]={{4800,-650},{4800,650},{5700,-650},{5700,650}};
    TArray<float> Heights;
    for (const FVector2D& P:Positions) { FHitResult Hit; FCollisionQueryParams Q(SCENE_QUERY_STAT(WaveDefenseCover),false); if (!World->LineTraceSingleByChannel(Hit,FVector(P.X,P.Y,5000),FVector(P.X,P.Y,-5000),ECC_Visibility,Q)) { AddError(TEXT("Cover ground missing")); return false; } Heights.Add(Hit.ImpactPoint.Z); }
    FActorSpawnParameters Params; Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    for (int32 I=0;I<4;++I)
    {
        auto* Cover=World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(),FVector(Positions[I].X,Positions[I].Y,Heights[I]+40),FRotator::ZeroRotator,Params);
        if (!Cover) { AddError(TEXT("Cover creation failed")); return false; }
        Cover->SetActorLabel(FString::Printf(TEXT("WD_Plains_Cover_%d"),I+1));
        Cover->GetStaticMeshComponent()->SetStaticMesh(Mesh); Cover->SetActorScale3D(FVector(1.5f,0.35f,0.8f)); Cover->GetStaticMeshComponent()->SetCollisionProfileName(TEXT("BlockAll"));
    }
    for (const auto& Pair:Existing) if (!Pair.Key.IsValid() || !Pair.Key->GetActorTransform().Equals(Pair.Value)) { AddError(TEXT("Existing actor changed")); return false; }
    World->MarkPackageDirty(); return TestTrue(TEXT("Cover saved"),UEditorLoadingAndSavingUtils::SaveMap(World,TEXT("/Game/MainRaid/RaidPlains")));
}
#endif
