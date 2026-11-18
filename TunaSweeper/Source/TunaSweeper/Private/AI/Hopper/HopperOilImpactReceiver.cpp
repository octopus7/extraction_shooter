#include "AI/Hopper/HopperOilImpactReceiver.h"

#include "Components/DecalComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"

void UHopperOilImpactReceiver::Initialize(AActor* Source, UMaterialInterface* Material, bool bUseLargeSplash, const FVector& TravelDirection)
{
    Wreck = Source;
    StampMaterial = Material;
    SprayDirection = TravelDirection;
    bLargeSplash = bUseLargeSplash;
    NextVariant = FMath::RandHelper(4);
}

void UHopperOilImpactReceiver::ReceiveParticleData_Implementation(const TArray<FBasicParticleData>& Data, UNiagaraSystem* System,
    const FVector& SimulationPositionOffset)
{
    AActor* Source = Wreck.Get();
    UWorld* World = Source ? Source->GetWorld() : nullptr;
    if (!World || !StampMaterial || Data.IsEmpty()) return;

    FCollisionQueryParams Query(SCENE_QUERY_STAT(HopperOilContact), false, Source);
    TArray<AActor*> Attachments;
    Source->GetAttachedActors(Attachments);
    Query.AddIgnoredActors(Attachments);
    for (const FBasicParticleData& Drop : Data)
    {
        const FVector Position = Drop.Position + SimulationPositionOffset;
        FHitResult Floor;
        // Resolve the surface normal and moving receiver at the particle's reported contact.
        // A corridor on the floor below a catwalk must never receive the catwalk's oil.
        if (!World->LineTraceSingleByChannel(Floor, Position + FVector(0,0,11), Position - FVector(0,0,18),
            ECC_Visibility, Query) || Floor.ImpactNormal.Z < .50 || !Floor.GetComponent()) continue;

        FVector Travel = FVector::VectorPlaneProject(Drop.Velocity.GetSafeNormal2D(), Floor.ImpactNormal).GetSafeNormal();
        if (Travel.IsNearlyZero()) Travel = FVector::VectorPlaneProject(SprayDirection, Floor.ImpactNormal).GetSafeNormal();
        if (!bLargeSplash) Travel = Travel.RotateAngleAxis(FMath::FRandRange(-120.f,120.f), Floor.ImpactNormal);
        const float Radius = bLargeSplash ? FMath::FRandRange(22.f,33.f) : FMath::FRandRange(8.f,15.f);
        const FVector Size(1.6f, Radius, Radius);
        // Keep both axes equally scaled. Direction changes the pattern's rotation, never its center.
        const FRotator Orientation = FRotationMatrix::MakeFromXZ(Floor.ImpactNormal, Travel).Rotator();
        const FVector Center = Floor.ImpactPoint + Floor.ImpactNormal * .15f;
        // Register a single render proxy with its impact time and atlas tile already set.
        UMaterialInstanceDynamic* Film = UMaterialInstanceDynamic::Create(StampMaterial, Floor.GetComponent());
        if (!Film) continue;
        Film->SetScalarParameterValue(TEXT("OilVariant"), NextVariant);
        Film->SetScalarParameterValue(TEXT("ImpactTime"), World->GetTimeSeconds());
        UDecalComponent* Decal = UGameplayStatics::SpawnDecalAttached(Film, Size, Floor.GetComponent(), NAME_None,
            Center, Orientation, EAttachLocation::KeepWorldPosition, 35.f);
        if (!Decal) continue;
        NextVariant = (NextVariant + 1) % 4;
        Decal->ComponentTags.Add(bLargeSplash ? TEXT("HopperOilSplash") : TEXT("HopperOilPool"));
        Decal->SetFadeScreenSize(.00025f);
        Decal->SetFadeOut(30.f,5.f,false);
    }
}
