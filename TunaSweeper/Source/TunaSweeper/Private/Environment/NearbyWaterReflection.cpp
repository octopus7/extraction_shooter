#include "Environment/NearbyWaterReflection.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/MeshComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "ConvexVolume.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Engine/TextureRenderTarget2D.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "SceneView.h"
#include "UnrealClient.h"

ANearbyWaterReflection::ANearbyWaterReflection()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickGroup = TG_PostUpdateWork;
    Capture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("ReflectionCapture"));
    SetRootComponent(Capture);
    Capture->bCaptureEveryFrame = false;
    Capture->bCaptureOnMovement = false;
    Capture->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
    Capture->CaptureSource = SCS_SceneColorHDR;
    Capture->bConsiderUnrenderedOpaquePixelAsFullyTranslucent = true;
    Capture->ShowFlags.SetAtmosphere(false);
    Capture->ShowFlags.SetFog(false);
    Capture->ShowFlags.SetVolumetricFog(false);
    Capture->ShowFlags.SetCloud(false);
    Capture->ShowFlags.SetScreenSpaceReflections(false);
    Capture->ShowFlags.SetTemporalAA(false);
    Capture->ShowFlags.SetDynamicShadows(false);
    Capture->bUseCustomProjectionMatrix = true;
}

void ANearbyWaterReflection::BeginPlay()
{
    Super::BeginPlay();
    if (WaterActor && GetNetMode() != NM_DedicatedServer)
    {
        TInlineComponentArray<UMeshComponent*> Meshes(WaterActor);
        for (UMeshComponent* Mesh : Meshes)
        {
            if (Mesh->GetFName() == SurfaceName)
            {
                Surface = Mesh;
                OriginalMaterial = Mesh->GetMaterial(0);
                Material = Mesh->CreateDynamicMaterialInstance(0);
                break;
            }
        }
    }
    SetActorTickEnabled(Material != nullptr);
}

void ANearbyWaterReflection::RefreshObjects(const FBox& Bounds)
{
    Capture->ClearShowOnlyComponents();
    const double Z = Bounds.GetCenter().Z;
    const FBox Area(FVector(Bounds.Min.X-ShoreRange, Bounds.Min.Y-ShoreRange, Z),
        FVector(Bounds.Max.X+ShoreRange, Bounds.Max.Y+ShoreRange, Z+MaxHeightAboveWater));
    for (TActorIterator<AActor> It(GetWorld()); It; ++It)
    {
        AActor* Actor = *It;
        if (Actor == WaterActor || Actor->IsHidden() || Actor == this) continue;
        TInlineComponentArray<UMeshComponent*> Meshes(Actor);
        for (UMeshComponent* Mesh : Meshes)
        {
            if ((!Mesh->IsA<UStaticMeshComponent>() && !Mesh->IsA<USkeletalMeshComponent>()) ||
                Mesh->IsA<UInstancedStaticMeshComponent>() || !Mesh->IsRegistered() || !Mesh->IsVisible() ||
                !Mesh->bRenderInMainPass || !Area.Intersect(Mesh->Bounds.GetBox())) continue;
            const FVector Extent = Mesh->Bounds.BoxExtent;
            if (FMath::Max(Extent.X, Extent.Y) > MaxObjectHalfWidth) continue;
            Capture->ShowOnlyComponent(Mesh);
        }
    }
}

void ANearbyWaterReflection::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    ULocalPlayer* Player = PC ? PC->GetLocalPlayer() : nullptr;
    if (!IsValid(Surface) || !Player || !Player->ViewportClient || !Player->ViewportClient->Viewport) return;
    FSceneViewProjectionData View;
    if (!Player->GetProjectionData(Player->ViewportClient->Viewport, View)) return;
    const FBox Bounds = Surface->Bounds.GetBox();
    FConvexVolume Frustum;
    GetViewFrustumBounds(Frustum, View.ComputeViewProjectionMatrix(), true);
    if (!Frustum.IntersectBox(Bounds.GetCenter(), Bounds.GetExtent()) ||
        Bounds.ComputeSquaredDistanceToPoint(View.ViewOrigin) > FMath::Square(ViewRange) ||
        View.ViewOrigin.Z <= Bounds.GetCenter().Z)
    {
        Material->SetScalarParameterValue(TEXT("NearbyReflectionEnabled"), 0.f);
        UntilRefresh = 0.f;
        return;
    }
    const FIntRect Rect = View.GetConstrainedViewRect();
    if (Rect.Width() <= 0 || Rect.Height() <= 0) return;
    const int32 W = FMath::Clamp(Resolution, 64, 1024);
    const int32 H = FMath::Clamp(FMath::RoundToInt(double(W)*Rect.Height()/Rect.Width()), 32, 2048);
    if (!Target || Target->SizeX != W || Target->SizeY != H)
    {
        Target = NewObject<UTextureRenderTarget2D>(this);
        Target->ClearColor = FLinearColor(0, 0, 0, 1);
        Target->InitCustomFormat(W, H, PF_FloatRGBA, true);
        Capture->TextureTarget = Target;
        Material->SetTextureParameterValue(TEXT("NearbyReflection"), Target);
    }
    UntilRefresh -= DeltaSeconds;
    if (UntilRefresh <= 0.f)
    {
        RefreshObjects(Bounds);
        UntilRefresh = .35f;
    }
    if (Capture->ShowOnlyComponents.IsEmpty())
    {
        Material->SetScalarParameterValue(TEXT("NearbyReflectionEnabled"), 0.f);
        return;
    }
    FVector Location = View.ViewOrigin;
    Location.Z = 2*Bounds.GetCenter().Z - Location.Z;
    const FRotator Camera = PC->PlayerCameraManager->GetCameraRotation();
    Capture->SetWorldLocationAndRotation(Location, FRotator(-Camera.Pitch, Camera.Yaw, -Camera.Roll));
    Capture->CustomProjectionMatrix = View.ProjectionMatrix;
    Capture->CaptureSceneDeferred();
    Material->SetScalarParameterValue(TEXT("NearbyReflectionEnabled"), 1.f);
}

void ANearbyWaterReflection::EndPlay(const EEndPlayReason::Type Reason)
{
    if (IsValid(Surface) && Surface->GetMaterial(0) == Material) Surface->SetMaterial(0, OriginalMaterial);
    Super::EndPlay(Reason);
}
