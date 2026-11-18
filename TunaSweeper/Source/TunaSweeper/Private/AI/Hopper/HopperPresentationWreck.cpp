#include "AI/Hopper/HopperPresentationComponent.h"

#include "AI/Hopper/HopperArmModule.h"
#include "AI/Hopper/HopperEnemyCharacter.h"
#include "AI/Hopper/HopperOilImpactReceiver.h"
#include "AI/Hopper/HopperVisualData.h"
#include "Components/MaterialBillboardComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/AssetManager.h"
#include "Engine/StaticMesh.h"
#include "Engine/StreamableManager.h"
#include "Engine/World.h"
#include "LocalVertexFactory.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "NiagaraComponent.h"
#include "NiagaraEmitterHandle.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraRendererProperties.h"
#include "NiagaraSystem.h"
#include "PSOPrecache.h"
#include "PSOPrecacheMaterial.h"

namespace
{
    constexpr float BlastFlashSeconds = .075f;
}

void UHopperPresentationComponent::InitializeBlastFlash()
{
    BlastFlash = nullptr;
    BlastCoreMaterial = nullptr;
    BlastFlashRemaining = 0.f;
    if (GetNetMode() == NM_DedicatedServer) return;
    const FHopperWreckEffects& Effects = VisualData->WreckEffects;
    UMaterialInterface* Material = Effects.BlastFlashMaterial.LoadSynchronous();
    if (!Material) return;

    // Register the transparent core ahead of the break. Niagara takes over as the outer flame expands.
    BlastFlash = NewObject<UMaterialBillboardComponent>(GetOwner(), NAME_None, RF_Transient);
    BlastFlash->CreationMethod = HasBegunPlay() ? EComponentCreationMethod::Instance : EComponentCreationMethod::UserConstructionScript;
    BlastFlash->ComponentTags.Append({TEXT("HopperGenerated"), TEXT("HopperBlastFlash")});
    BlastFlash->SetCanEverAffectNavigation(false);
    BlastFlash->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    BlastFlash->SetGenerateOverlapEvents(false);
    BlastFlash->SetCastShadow(false);
    BlastFlash->SetReceivesDecals(false);
    BlastFlash->SetupAttachment(BodyRoot);
    // The front seam is exposed even with the hatch closed.
    BlastFlash->SetRelativeLocation(Effects.Origin + FVector(62.f, 0.f, 8.f));
    BlastCoreMaterial = UMaterialInstanceDynamic::Create(Material, BlastFlash);
    BlastCoreMaterial->SetScalarParameterValue(TEXT("FlashGain"), 0.f);
    BlastFlash->AddElement(BlastCoreMaterial, nullptr, false, 67.f, 60.f, nullptr);
    BlastFlash->RegisterComponent();
}

void UHopperPresentationComponent::FadeBlastFlash(float DeltaSeconds)
{
    BlastFlashRemaining = FMath::Max(0.f, BlastFlashRemaining - DeltaSeconds);
    if (BlastFlashRemaining == 0.f)
    {
        BlastFlash->DestroyComponent();
        BlastFlash = nullptr;
        BlastCoreMaterial = nullptr;
        return;
    }
    BlastCoreMaterial->SetScalarParameterValue(TEXT("FlashGain"), BlastFlashRemaining / BlastFlashSeconds);
}

void UHopperPresentationComponent::PreloadWreckEffects()
{
    WreckAssetHandle.Reset();
    if (!VisualData || !VisualData->WreckEffects.IsConfigured() || GetNetMode() == NM_DedicatedServer) return;
    const FHopperWreckEffects& Effects = VisualData->WreckEffects;
    TArray<FSoftObjectPath> Paths = {Effects.RadialBlast.ToSoftObjectPath(), Effects.Fire.ToSoftObjectPath(),
        Effects.MetalDebris.ToSoftObjectPath(), Effects.OilSpray.ToSoftObjectPath(),
        Effects.Scorch.ToSoftObjectPath(), Effects.OilPool.ToSoftObjectPath(), Effects.OilSplash.ToSoftObjectPath()};
    Paths.RemoveAll([](const FSoftObjectPath& Path){ return !Path.IsValid(); });
    WreckAssetHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(MoveTemp(Paths),
        FStreamableDelegate::CreateUObject(this, &UHopperPresentationComponent::PrepareWreckMaterials));
}

void UHopperPresentationComponent::PrepareWreckMaterials()
{
    if (!VisualData) return;
    UNiagaraSystem* Burst = VisualData->WreckEffects.RadialBlast.Get();
    if (Burst)
    {
#if WITH_EDITOR
        // A cold editor can finish compiling the sprite shader after this short burst has already ended.
        TArray<UMaterialInterface*> BurstMaterials;
        for (const FNiagaraEmitterHandle& Emitter : Burst->GetEmitterHandles())
            Emitter.ForEachEnabledRendererWithIndex([&](const UNiagaraRendererProperties* Renderer, int32)
            {
                Renderer->GetUsedMaterials(nullptr, BurstMaterials);
            });
        for (UMaterialInterface* Material : TSet<UMaterialInterface*>(BurstMaterials))
            if (Material) Material->EnsureIsComplete();
#endif
#if UE_WITH_PSO_PRECACHING
        Burst->PrecachePSOs();
        BoostPSOPriority(EPSOPrecachePriority::High, Burst->GetMaterialPSOPrecacheRequestIDs());
#endif
    }
    UMaterialInterface* Materials[] = {VisualData->WreckEffects.OilPool.Get(), VisualData->WreckEffects.OilSplash.Get()};
#if WITH_EDITOR
    if (UMaterialInterface* Scorch = VisualData->WreckEffects.Scorch.Get()) Scorch->EnsureIsComplete();
    if (UMaterialInterface* Core = VisualData->WreckEffects.BlastFlashMaterial.Get()) Core->EnsureIsComplete();
    // Editor sessions can compile deferred decal permutations on demand on their first draw.
    for (UMaterialInterface* Material : Materials)
    {
        if (!Material) continue;
        UMaterial* Parent = Material->GetMaterial();
        if (Parent && Parent != Material) Parent->EnsureIsComplete();
        Material->EnsureIsComplete();
    }
#endif
#if UE_WITH_PSO_PRECACHING
    if (!IsComponentPSOPrecachingEnabled()) return;
    // Same vertex factory and precache parameters used by UDecalComponent at registration.
    FPSOPrecacheVertexFactoryDataList Factories;
    Factories.Add(FPSOPrecacheVertexFactoryData(&FLocalVertexFactory::StaticType));
    for (UMaterialInterface* Material : Materials)
    {
        TArray<FMaterialPSOPrecacheRequestID> Requests;
        if (Material) Material->PrecachePSOs(Factories,FPSOPrecacheParams{},EPSOPrecachePriority::High,Requests);
    }
#endif
}

void UHopperPresentationComponent::EmitWreckEffects()
{
    UWorld* World = GetWorld();
    if (bWreckBurstPlayed || !World || !BodyRoot || World->GetNetMode() == NM_DedicatedServer) return;
    bWreckBurstPlayed = true;
    const FHopperWreckEffects& Effects = VisualData->WreckEffects;
    const FTransform Chassis = BodyRoot->GetComponentTransform();
    const FVector Origin = Chassis.TransformPosition(Effects.Origin);
    ApplyHullScorch(Chassis);
    if (BlastCoreMaterial)
    {
        BlastCoreMaterial->SetScalarParameterValue(TEXT("FlashGain"), 1.f);
        BlastFlashRemaining = BlastFlashSeconds;
    }
    for (const TSoftObjectPtr<UNiagaraSystem>& System : {Effects.RadialBlast, Effects.MetalDebris})
    {
        if (UNiagaraSystem* Burst = System.LoadSynchronous())
            UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, Burst, Origin, Chassis.Rotator(), FVector::OneVector, true, true, ENCPoolMethod::None, false);
    }
    UNiagaraSystem* Oil = Effects.OilSpray.LoadSynchronous();
    if (!Oil) return;
    if (WreckSprayDirection.IsNearlyZero())
        WreckSprayDirection = Chassis.TransformVectorNoScale(FVector(1,.65,0)).GetSafeNormal2D();
    UNiagaraComponent* Spray = UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, Oil, Origin, WreckSprayDirection.Rotation(),
        FVector::OneVector, true, false, ENCPoolMethod::None, false);
    if (!Spray) return;
    auto BindImpact = [&](FName Parameter, const TSoftObjectPtr<UMaterialInterface>& Material, bool bLargeSplash)
    {
        if (UMaterialInterface* Decal = Material.LoadSynchronous())
        {
            UHopperOilImpactReceiver* Receiver = NewObject<UHopperOilImpactReceiver>(this);
            Receiver->Initialize(GetOwner(), Decal, bLargeSplash, WreckSprayDirection);
            OilReceivers.Add(Receiver);
            Spray->SetVariableObject(Parameter, Receiver);
        }
    };
    BindImpact(TEXT("User.NearOilHit"), Effects.OilPool, false);
    BindImpact(TEXT("User.FanOilHit"), Effects.OilSplash, true);
    Spray->Activate(true);
}

void UHopperPresentationComponent::ApplyHullScorch(const FTransform& Chassis)
{
    UMaterialInterface* Scorch = VisualData->WreckEffects.Scorch.LoadSynchronous();
    if (!Scorch) return;
    auto ScorchPart = [&](UStaticMeshComponent* Part)
    {
        if (!Part || !Part->GetStaticMesh() || Part->bHiddenInGame) return;
        const FTransform Relative = Part->GetComponentTransform().GetRelativeTransform(Chassis);
        const FVector Origin = Relative.GetTranslation() - VisualData->WreckEffects.Origin;
        const FVector X = Relative.TransformVector(FVector::ForwardVector);
        const FVector Y = Relative.TransformVector(FVector::RightVector);
        const FVector Z = Relative.TransformVector(FVector::UpVector);
        // Freeze the blast coordinates into this part. Its soot follows the panel as the hatch opens.
        UMaterialInstanceDynamic* Layer = UMaterialInstanceDynamic::Create(Scorch, Part);
        Layer->SetVectorParameterValue(TEXT("ScorchRowX"), FLinearColor(X.X,Y.X,Z.X,Origin.X));
        Layer->SetVectorParameterValue(TEXT("ScorchRowY"), FLinearColor(X.Y,Y.Y,Z.Y,Origin.Y));
        Layer->SetVectorParameterValue(TEXT("ScorchRowZ"), FLinearColor(X.Z,Y.Z,Z.Z,Origin.Z));
        Layer->SetScalarParameterValue(TEXT("PaintFade"),0.f);
        Part->SetOverlayMaterial(Layer);
        ScorchedHullMaterials.Add(Layer);
    };
    for (const auto& Pair : Parts)
    {
        const FString Name = Pair.Key.ToString();
        if (Name.StartsWith(TEXT("Body_")) || Name.StartsWith(TEXT("Armor_")) || Name.StartsWith(TEXT("Guard")) ||
            Name.EndsWith(TEXT("_06_Fairing")) || Pair.Key == TEXT("Hatch_Pivot") || Pair.Key == TEXT("RampExtension"))
            ScorchPart(Pair.Value);
    }
    if (auto* Hopper = Cast<ATunaSweeperHopperEnemyCharacter>(GetOwner()))
    {
        for (bool bLeft : {false,true})
        {
            if (auto* Arm = Hopper->GetArmModule(bLeft))
            {
                TInlineComponentArray<UStaticMeshComponent*> ArmParts(Arm);
                for (auto* Part : ArmParts)
                    if (Part->GetStaticMesh() && Part->GetStaticMesh()->GetName().Contains(TEXT("Shoulder_Armor"))) ScorchPart(Part);
            }
        }
    }
}

void UHopperPresentationComponent::UpdateWreckBurn(float FlightTime)
{
    constexpr float SeatClearanceSeconds = .16f;
    constexpr float BurnSeconds = 8.f;
    const float BurnAge = FlightTime - SeatClearanceSeconds;
    if (BurnAge < 0.f) return;

    if (!bWreckFireStarted)
    {
        bWreckFireStarted = true;
        const FHopperWreckEffects& Effects = VisualData->WreckEffects;
        if (BurnAge < BurnSeconds)
        {
            if (UNiagaraSystem* Flame = Effects.Fire.LoadSynchronous())
            {
                UNiagaraComponent* Fire = UNiagaraFunctionLibrary::SpawnSystemAttached(Flame, BodyRoot, NAME_None,
                    Effects.Origin, FRotator::ZeroRotator, EAttachLocation::KeepRelativeOffset, true, true, ENCPoolMethod::None, false);
                // A missed launch frame must not restart the fire's full lifetime.
                if (Fire && BurnAge > .1f) Fire->AdvanceSimulationByTime(BurnAge, 1.f/30.f);
            }
        }
    }
    const float Fade = FMath::SmoothStep(0.f, BurnSeconds, BurnAge);
    for (UMaterialInstanceDynamic* Layer : ScorchedHullMaterials)
        Layer->SetScalarParameterValue(TEXT("PaintFade"), Fade);
    if (BurnAge >= BurnSeconds) ScorchedHullMaterials.Reset();
}
