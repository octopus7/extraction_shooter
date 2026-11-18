#include "Scenario/SSTOLaunchCinematic.h"

#include "Blueprint/UserWidget.h"
#include "Camera/CameraComponent.h"
#include "Camera/CameraModifier_CameraShake.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "HAL/PlatformTime.h"
#include "Interaction/TunaSweeperExtractionPointActor.h"
#include "Interaction/TunaSweeperLadderTransferActor.h"
#include "Interaction/TunaSweeperRaidStorageActor.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Math/RotationMatrix.h"
#include "Player/TunaSweeperPlayerController.h"
#include "Scenario/TunaSweeperEndingActor.h"

namespace SSTOLaunch
{
    constexpr float HoverCut = 5.6f;
    constexpr float CloudCut = 10.9f;
    constexpr float OrbitCut = 16.6f;
    constexpr float End = 21.0f;
    constexpr double CloudAltitude = 210000.0;
    constexpr double OrbitAltitude = 1400000.0;
    constexpr double Aspect = 16.0 / 9.0;
    constexpr int32 PlumeCount = 18;
    constexpr int32 OrbitalVaporCount = 9;
    constexpr float CabinLumens = 215.f;
    const FVector StairHinge(395, -353, 181);
    const FVector DoorHinge(395, -312, 410);
    const FName RevealParameter(TEXT("VerticalRevealActive"));
    const FName PlateParameter(TEXT("Plate"));
    const FName TintParameter(TEXT("Tint"));
    const FName DensityParameter(TEXT("Density"));
    const FName SeedParameter(TEXT("Seed"));
    const FName NormalCamera(TEXT("Default"));

    float Ease(float Time, float From, float To)
    {
        return FMath::SmoothStep(0.0f, 1.0f, FMath::Clamp((Time - From) / (To - From), 0.0f, 1.0f));
    }

    // Source vertices share the hull origin. Keep the authored hinge fixed while moving the mesh.
    FTransform AtHinge(const FTransform& Rest, const FVector& Pivot, const FQuat& Swing,
        const FVector& ScaleFactor = FVector::OneVector, const FVector& Slide = FVector::ZeroVector)
    {
        const FQuat Rotation = Swing * Rest.GetRotation();
        const FVector Scale = Rest.GetScale3D() * ScaleFactor;
        return FTransform(Rotation, Rest.TransformPosition(Pivot) - Rotation.RotateVector(Scale * Pivot) + Slide, Scale);
    }

    bool IsGear(FName Name, FVector& Mount)
    {
        static const FName Names[] = { TEXT("SM_SSTO_LandingGear0"), TEXT("SM_SSTO_LandingGear1"), TEXT("SM_SSTO_LandingGear2") };
        static const FVector Mounts[] = { FVector(917,0,119), FVector(-658,-265,119), FVector(-658,265,119) };
        for (int32 Index = 0; Index < UE_ARRAY_COUNT(Names); ++Index)
        {
            if (Name == Names[Index])
            {
                Mount = Mounts[Index];
                return true;
            }
        }
        return false;
    }

    void SetCard(UStaticMeshComponent* Card, const FVector& Location, const FQuat& Facing, double Width, double Height)
    {
        // The Engine plane is 100 cm square. Its UV axes are camera right and camera down.
        Card->SetWorldTransform(FTransform(Facing, Location, FVector(Width * .01, Height * .01, 1.0)),
            false, nullptr, ETeleportType::TeleportPhysics);
    }
}

ASSTOLaunchCinematic::ASSTOLaunchCinematic()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = false;
    PrimaryActorTick.bTickEvenWhenPaused = true;
    PrimaryActorTick.TickGroup = TG_PrePhysics;
    SetCanBeDamaged(false);

    auto* StageRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Stage"));
    SetRootComponent(StageRoot);
    StageRoot->SetMobility(EComponentMobility::Movable);
    Airframe = CreateDefaultSubobject<USceneComponent>(TEXT("Airframe"));
    Airframe->SetupAttachment(StageRoot);
    Airframe->SetMobility(EComponentMobility::Movable);
    Lens = CreateDefaultSubobject<UCameraComponent>(TEXT("LockedCamera"));
    Lens->SetupAttachment(StageRoot);
    Lens->SetMobility(EComponentMobility::Movable);
    Lens->bUsePawnControlRotation = false;
    Lens->SetConstraintAspectRatio(true);
    Lens->SetAspectRatio(SSTOLaunch::Aspect);
    Lens->SetPostProcessBlendWeight(1.0f);
    Lens->PostProcessSettings.bOverride_MotionBlurAmount = true;
    Lens->PostProcessSettings.MotionBlurAmount = 0.0f;
    Lens->PostProcessSettings.bOverride_LensFlareIntensity = true;
    Lens->PostProcessSettings.LensFlareIntensity = 0.0f;

    HatchAsset = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TEXT("/Game/Effects/SSTOEnding/SM_SSTO_CinematicHatch.SM_SSTO_CinematicHatch")));
    CloudAsset = TSoftObjectPtr<UTexture2D>(FSoftObjectPath(TEXT("/Game/Effects/SSTOEnding/T_SSTOCloudHorizon.T_SSTOCloudHorizon")));
    OrbitAsset = TSoftObjectPtr<UTexture2D>(FSoftObjectPath(TEXT("/Game/Effects/SSTOEnding/T_SSTOOrbitalLimb.T_SSTOOrbitalLimb")));
    BackdropAsset = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/Effects/SSTOEnding/M_SSTOEndingBackdrop.M_SSTOEndingBackdrop")));
    VaporAsset = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/Effects/SSTOEnding/M_SSTOEndingVapor.M_SSTOEndingVapor")));
}

UStaticMeshComponent* ASSTOLaunchCinematic::MakeMesh(UStaticMesh* Mesh, USceneComponent* Parent, bool bCastsShadow)
{
    auto* Part = NewObject<UStaticMeshComponent>(this, NAME_None, RF_Transient);
    Part->SetupAttachment(Parent);
    Part->SetMobility(EComponentMobility::Movable);
    Part->SetStaticMesh(Mesh);
    Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Part->SetGenerateOverlapEvents(false);
    Part->SetCanEverAffectNavigation(false);
    Part->SetCastShadow(bCastsShadow);
    Part->SetReceivesDecals(false);
    Part->SetOnlyOwnerSee(true); // The stage itself is this player's view actor.
    Part->SetHiddenInSceneCapture(true);
    Part->SetVisibleInRayTracing(false);
    Part->SetAffectDistanceFieldLighting(false);
    Part->bAllowCullDistanceVolume = false;
    Part->bNeverDistanceCull = true;
    Part->PrimaryComponentTick.bCanEverTick = false;
    Part->RegisterComponent();
    BuiltComponents.Add(Part);
    return Part;
}

bool ASSTOLaunchCinematic::BuildStage(AActor* Lander)
{
    UStaticMesh* DoorMesh = HatchAsset.LoadSynchronous();
    UStaticMesh* Plane = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane"));
    UMaterialInterface* PlateMaster = BackdropAsset.LoadSynchronous();
    UMaterialInterface* VaporMaster = VaporAsset.LoadSynchronous();
    CloudPlate = CloudAsset.LoadSynchronous();
    OrbitPlate = OrbitAsset.LoadSynchronous();
    if (!DoorMesh || !Plane || !PlateMaster || !VaporMaster || !CloudPlate || !OrbitPlate)
    {
        return false;
    }

    Airframe->SetWorldTransform(ParkedPose);
    TInlineComponentArray<UStaticMeshComponent*> Sources;
    Lander->GetComponents(Sources, true);
    TSet<FName> MeshNames;
    TMap<UMaterialInterface*, UMaterialInterface*> FixedMaterials;
    auto AssignHullMaterials = [&](UStaticMeshComponent* Copy, UStaticMesh* Mesh)
    {
        const TArray<FStaticMaterial>& Slots = Mesh->GetStaticMaterials();
        for (int32 Slot = 0; Slot < Slots.Num(); ++Slot)
        {
            UMaterialInterface* Source = Slots[Slot].MaterialInterface;
            if (!Source) continue;
            UMaterialInterface*& Fixed = FixedMaterials.FindOrAdd(Source);
            if (!Fixed)
            {
                Fixed = Source;
                float OldValue = 0.0f;
                if (Source->GetScalarParameterValue(FMaterialParameterInfo(SSTOLaunch::RevealParameter), OldValue))
                {
                    auto* MID = UMaterialInstanceDynamic::Create(Source, this);
                    MID->SetScalarParameterValue(SSTOLaunch::RevealParameter, 0.0f);
                    Fixed = MID;
                }
            }
            Copy->SetMaterial(Slot, Fixed);
        }
    };
    for (const UStaticMeshComponent* Source : Sources)
    {
        UStaticMesh* Mesh = Source->GetStaticMesh();
        if (!Mesh || !Mesh->GetPathName().StartsWith(TEXT("/Game/MainRaid/SSTO/Meshes/SM_SSTO_"))) continue;
        const FName Name = Mesh->GetFName();
        if (MeshNames.Contains(Name)) return false;
        MeshNames.Add(Name);
        const FTransform Rest = Source->GetComponentTransform().GetRelativeTransform(ParkedPose);
        UStaticMeshComponent* Copy = MakeMesh(Mesh, Airframe, Source->CastShadow);
        Copy->SetRelativeTransform(Rest);
        AssignHullMaterials(Copy, Mesh);
        FVector Mount;
        if (Name == TEXT("SM_SSTO_Ladder"))
        {
            Stair = Copy;
            StairRest = Rest;
        }
        else if (SSTOLaunch::IsGear(Name, Mount))
        {
            Gear.Add({Copy, Rest, Mount});
        }
    }
    if (MeshNames.Num() != 13 || !Stair || Gear.Num() != 3) return false;

    Hatch = MakeMesh(DoorMesh, Airframe, true);
    AssignHullMaterials(Hatch, DoorMesh);
    Backdrop = MakeMesh(Plane, GetRootComponent(), false);
    BackdropMaterial = UMaterialInstanceDynamic::Create(PlateMaster, this);
    Backdrop->SetMaterial(0, BackdropMaterial);
    Backdrop->SetVisibility(false);

    DockStrip = MakeMesh(Plane, Airframe, false);
    DockMaterial = UMaterialInstanceDynamic::Create(VaporMaster, this);
    DockMaterial->SetVectorParameterValue(SSTOLaunch::TintParameter, FLinearColor(2.8f, 1.25f, .38f));
    DockMaterial->SetScalarParameterValue(SSTOLaunch::SeedParameter, .42f);
    DockMaterial->SetScalarParameterValue(SSTOLaunch::DensityParameter, .7f);
    DockStrip->SetMaterial(0, DockMaterial);
    DockStrip->SetRelativeTransform(FTransform(FRotationMatrix::MakeFromXY(FVector::ForwardVector, -FVector::UpVector).ToQuat(),
        FVector(395,-311,398), FVector(.76,.055,1)));

    Plume.Reserve(SSTOLaunch::PlumeCount);
    for (int32 Index = 0; Index < SSTOLaunch::PlumeCount; ++Index)
    {
        UStaticMeshComponent* Card = MakeMesh(Plane, GetRootComponent(), false);
        UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(VaporMaster, this);
        const float Depth = static_cast<float>(Index) / (SSTOLaunch::PlumeCount - 1);
        MID->SetVectorParameterValue(SSTOLaunch::TintParameter, FMath::Lerp(
            FLinearColor(1.95f,1.48f,.97f), FLinearColor(1.08f,.72f,.59f), Depth));
        MID->SetScalarParameterValue(SSTOLaunch::DensityParameter, 0.0f);
        MID->SetScalarParameterValue(SSTOLaunch::SeedParameter, .37f + Index * 1.793f);
        Card->SetMaterial(0, MID);
        Card->SetTranslucentSortPriority(1 + SSTOLaunch::PlumeCount - Index);
        Card->SetVisibility(false);
        Plume.Add({Card, MID});
    }
    return true;
}

bool ASSTOLaunchCinematic::Start(ATunaSweeperPlayerController* Player, AActor* Lander)
{
    if (Controller.IsValid() || !IsValid(Player) || !Player->IsLocalController() || !Player->GetLocalPlayer() ||
        !Player->PlayerCameraManager || !IsValid(Lander) || !GetWorld() || Player->GetWorld() != GetWorld() ||
        Lander->GetWorld() != GetWorld() || Lander->IsActorBeingDestroyed()) return false;

    ParkedPose = Lander->GetActorTransform();
    LevelFrame = FTransform(FRotator(0, ParkedPose.Rotator().Yaw, 0), ParkedPose.GetLocation());
    SetActorTransform(LevelFrame);
    Controller = Player;
    SetOwner(Player);
    if (!BuildStage(Lander))
    {
        Stop();
        return false;
    }

    PreviousView = Player->GetViewTarget();
    bPreviousFullTick = Player->SetLaunchCameraFullTickWhenPaused(true);
    bPreviousControllerPauseTick = Player->GetTickableWhenPaused();
    bPreviousCameraMoveable = GetWorld()->bIsCameraMoveableWhenPaused;
    PreviousCameraStyle = Player->PlayerCameraManager->CameraStyle;
    bCameraOwned = true;
    Player->SetTickableWhenPaused(true);
    GetWorld()->bIsCameraMoveableWhenPaused = true;
    Player->PlayerCameraManager->CameraStyle = SSTOLaunch::NormalCamera;
    UCameraModifier* Shake = Player->PlayerCameraManager->FindCameraModifierByClass(UCameraModifier_CameraShake::StaticClass());
    if (Shake && !Shake->IsDisabled() && !Shake->IsPendingDisable())
    {
        SuspendedShake = Shake;
        Shake->DisableModifier(true);
    }
    const UGameInstance* Instance = GetWorld()->GetGameInstance();
    bSingleLocalWorld = GetNetMode() == NM_Standalone && Instance && Instance->GetNumLocalPlayers() == 1;
    HideGameplay(Lander);
    SetUpLighting();
    Elapsed = 0;
    bOrbitReported = false;
    StartedAt = FPlatformTime::Seconds();
    CutTo(0);
    SetFlightPose(0);
    Player->SetViewTargetWithBlend(this, 0.0f);
    Player->PlayerCameraManager->SetGameCameraCutThisFrame();
    SetActorTickEnabled(true);
    return true;
}

void ASSTOLaunchCinematic::HideGameplay(AActor* Lander)
{
    ATunaSweeperPlayerController* Player = Controller.Get();
    APawn* Pawn = Player->GetPawn();
    auto HideActor = [&](AActor* Actor)
    {
        if (Actor && !Player->HiddenActors.Contains(Actor))
        {
            Player->HiddenActors.Add(Actor);
            HiddenForView.Add(Actor);
        }
    };
    HideActor(Lander);
    HideActor(Pawn);
    TArray<AActor*> AttachedActors;
    Lander->GetAllChildActors(AttachedActors);
    for (AActor* Child : AttachedActors) HideActor(Child);
    if (Pawn)
    {
        Pawn->GetAttachedActors(AttachedActors, true, true);
        for (AActor* Child : AttachedActors) HideActor(Child);
    }
    for (TActorIterator<AActor> It(GetWorld()); It; ++It)
    {
        AActor* Actor = *It;
        // The Plains extraction beacon has a separate visible token at its foot.
        const auto* Prop = Cast<AStaticMeshActor>(Actor);
        const UStaticMesh* PropMesh = Prop ? Prop->GetStaticMeshComponent()->GetStaticMesh() : nullptr;
        static const FName ExtractionToken(TEXT("SM_SM_VoxelExtraction"));
        if (Actor->IsA<ATunaSweeperExtractionPointActor>() || (PropMesh && PropMesh->GetFName() == ExtractionToken) ||
            Actor->IsA<ATunaSweeperEndingActor>() || Actor->IsA<ATunaSweeperRaidStorageActor>() ||
            Actor->IsA<ATunaSweeperLadderTransferActor>() || (Pawn && Actor->IsOwnedBy(Pawn)))
        {
            HideActor(Actor);
        }
        // Interaction markers are screen-space widget components. HiddenActors only filters the 3D pass.
        TInlineComponentArray<UWidgetComponent*> Widgets(Actor);
        for (UWidgetComponent* Component : Widgets)
        {
            if (Component->GetWidgetSpace() != EWidgetSpace::Screen || Component->GetOwnerPlayer() != Player->GetLocalPlayer()) continue;
            UUserWidget* Widget = Component->GetUserWidgetObject();
            if (Widget && Widget->GetVisibility() != ESlateVisibility::Collapsed)
            {
                HiddenWidgets.Add({Widget, Widget->GetVisibility()});
                Widget->SetVisibility(ESlateVisibility::Collapsed);
            }
        }
    }
}

void ASSTOLaunchCinematic::SetUpLighting()
{
    // A directional light affects an entire world. Use the fixed camera grade alone for shared views.
    if (!bSingleLocalWorld) return;
    float OriginalIntensity = 8.0f;
    for (TActorIterator<ADirectionalLight> It(GetWorld()); It; ++It)
    {
        UDirectionalLightComponent* Light = It->FindComponentByClass<UDirectionalLightComponent>();
        if (Light && Light->GetVisibleFlag() && Light->bAtmosphereSunLight && Light->AtmosphereSunLightIndex == 0)
        {
            OriginalIntensity = Light->Intensity;
            HiddenSuns.Add(Light);
            Light->SetVisibility(false);
        }
    }
    for (TActorIterator<AExponentialHeightFog> It(GetWorld()); It; ++It)
    {
        UExponentialHeightFogComponent* Fog = It->FindComponentByClass<UExponentialHeightFogComponent>();
        if (Fog && Fog->GetVisibleFlag()) GroundFog.Add(Fog);
    }
    auto* Sunset = NewObject<UDirectionalLightComponent>(this, NAME_None, RF_Transient);
    Sunset->SetupAttachment(GetRootComponent());
    Sunset->SetMobility(EComponentMobility::Movable);
    Sunset->SetRelativeRotation(FRotator(-7.0, -104.0, 0));
    Sunset->SetAtmosphereSunLight(true);
    Sunset->SetAtmosphereSunLightIndex(0);
    Sunset->SetIntensity(FMath::Clamp(OriginalIntensity * .68f, 3.0f, 24000.0f));
    Sunset->SetLightColor(FLinearColor(1.0f,.61f,.35f));
    Sunset->RegisterComponent();
    BuiltComponents.Add(Sunset);

    CabinLight = NewObject<UPointLightComponent>(this, NAME_None, RF_Transient);
    CabinLight->SetupAttachment(Airframe);
    CabinLight->SetMobility(EComponentMobility::Movable);
    CabinLight->SetRelativeLocation(FVector(395,-294,350));
    CabinLight->SetLightColor(FLinearColor(1.0f,.52f,.19f));
    CabinLight->SetIntensityUnits(ELightUnits::Lumens);
    CabinLight->SetIntensity(SSTOLaunch::CabinLumens);
    CabinLight->SetAttenuationRadius(320.0f);
    CabinLight->SetCastShadows(false);
    CabinLight->RegisterComponent();
    BuiltComponents.Add(CabinLight);
}

FVector ASSTOLaunchCinematic::StagePosition(const FVector& LevelLocal) const
{
    return LevelFrame.TransformPosition(LevelLocal);
}

void ASSTOLaunchCinematic::CutTo(int32 NewShot)
{
    using namespace SSTOLaunch;
    ShotIndex = NewShot;
    FVector Position, Target;
    float Fov = 42.0f;
    switch (NewShot)
    {
    case 0:
        Position = ParkedPose.TransformPosition(FVector(995,-1470,245));
        Target = ParkedPose.TransformPosition(FVector(520,-387,206));
        Fov = 42.0f;
        break;
    case 1:
        Position = StagePosition(FVector(520,-4050,293));
        Target = StagePosition(FVector(0,0,555));
        Fov = 45.0f;
        break;
    case 2:
        Position = StagePosition(FVector(3800,-23400,CloudAltitude + 200));
        Target = StagePosition(FVector(0,0,CloudAltitude - 1225));
        break;
    default:
        Position = StagePosition(FVector(5400,-36500,OrbitAltitude - 2400));
        Target = StagePosition(FVector(0,0,OrbitAltitude - 2580));
        Fov = 40.0f;
        break;
    }
    Lens->SetFieldOfView(Fov);
    // Building from a direction keeps the horizon level even though the parked ship is tilted.
    Lens->SetWorldLocationAndRotation(Position, (Target - Position).Rotation());
    CardRotation = FRotationMatrix::MakeFromXY(Lens->GetRightVector(), -Lens->GetUpVector()).ToQuat();
    const bool bMatte = NewShot >= 2;
    FPostProcessSettings& Grade = Lens->PostProcessSettings;
    Grade.bOverride_ColorGain = !bMatte;
    Grade.ColorGain = FVector4(.98,.90,.86,1);
    Grade.bOverride_ColorGainShadows = !bMatte;
    Grade.ColorGainShadows = FVector4(.84,.87,1.04,1);
    // The high shots are exposed for emissive picture plates. A moving auto-exposure would pump at the cut.
    Grade.bOverride_AutoExposureMethod = bMatte;
    Grade.AutoExposureMethod = AEM_Manual;
    Grade.bOverride_AutoExposureApplyPhysicalCameraExposure = bMatte;
    Grade.AutoExposureApplyPhysicalCameraExposure = false;
    Grade.bOverride_AutoExposureBias = true;
    Grade.AutoExposureBias = bMatte ? 0.0f : -.5f;
    Grade.bOverride_AutoExposureBiasCurve = bMatte;
    Grade.AutoExposureBiasCurve = nullptr;
    Grade.bOverride_BloomIntensity = bMatte;
    Grade.BloomIntensity = .15f;

    Backdrop->SetVisibility(bMatte);
    if (bMatte)
    {
        UTexture2D* Plate = NewShot == 2 ? CloudPlate.Get() : OrbitPlate.Get();
        BackdropMaterial->SetTextureParameterValue(PlateParameter, Plate);
        const double Distance = NewShot == 2 ? 39000.0 : 58000.0;
        const double ViewWidth = 2.0 * Distance * FMath::Tan(FMath::DegreesToRadians(Fov * .5));
        const double ArtAspect = static_cast<double>(Plate->GetSizeX()) / FMath::Max(1, Plate->GetSizeY());
        const double Width = FMath::Max(ViewWidth, ViewWidth / Aspect * ArtAspect) * 1.025;
        SetCard(Backdrop, Position + Lens->GetForwardVector() * Distance, CardRotation, Width, Width / ArtAspect);
    }
    if (bMatte)
    {
        // Keep the low-altitude lighting in shots 0/1. These distant plates already include atmospheric haze.
        for (const auto& Weak : GroundFog)
            if (auto* Fog = Weak.Get()) Fog->SetVisibility(false);
    }
    for (int32 Index = 0; Index < Plume.Num(); ++Index)
    {
        const bool bVisible = NewShot == 2 || (NewShot == 3 && Index < OrbitalVaporCount);
        Plume[Index].Component->SetVisibility(bVisible);
    }
    if (Controller.IsValid() && Controller->PlayerCameraManager)
        Controller->PlayerCameraManager->SetGameCameraCutThisFrame();
}

void ASSTOLaunchCinematic::SetFlightPose(float Time)
{
    using namespace SSTOLaunch;
    FTransform ShipPose = ParkedPose;
    if (ShotIndex == 0)
    {
        const float Fold = Ease(Time, .58f, 2.78f);
        const float Closing = Ease(Time, 2.62f, 4.12f);
        const float Seating = Ease(Time, 4.12f, 4.63f);
        const FQuat FoldRotation(FVector::ForwardVector, FMath::DegreesToRadians(-152.0 * Fold));
        const FQuat DoorRotation(FVector::ForwardVector, FMath::DegreesToRadians(92.0 * (1.0 - Closing)));
        // The stair swings around the existing, terrain-adjusted hinge, then nests behind the pressure door.
        Stair->SetRelativeTransform(AtHinge(StairRest, StairHinge, FoldRotation, FVector::OneVector,
            FVector(0, 30.0 * Ease(Time, 2.0f, 2.78f), 0)));
        Stair->SetVisibility(Fold < .999f);
        // The panel pivots inside the cabin, then translates outwards into the pressure seal.
        Hatch->SetRelativeTransform(AtHinge(FTransform::Identity, DoorHinge, DoorRotation, FVector::OneVector,
            FVector(0,26.0 * (1.0 - Seating),0)));
        const float CabinGlow = 1.0f - Ease(Time, 3.64f, 4.63f);
        DockMaterial->SetScalarParameterValue(DensityParameter, .72f * CabinGlow);
        DockStrip->SetVisibility(CabinGlow > .002f);
        if (CabinLight) CabinLight->SetIntensity(CabinLumens * CabinGlow);
    }
    else
    {
        // A skipped tick may cross a whole shot. Set the sealed pose explicitly on every later cut.
        Hatch->SetRelativeTransform(FTransform::Identity);
        Stair->SetVisibility(false);
        DockStrip->SetVisibility(false);
        if (CabinLight) CabinLight->SetVisibility(false);
        const float Lift = Ease(Time, HoverCut + .28f, CloudCut - .72f);
        ShipPose.SetRotation(FQuat::Slerp(ParkedPose.GetRotation(), LevelFrame.GetRotation(), Lift));
        double Height = 378.0 * Lift;
        if (ShotIndex == 2)
            Height = CloudAltitude + 1230.0 * Ease(Time, CloudCut, OrbitCut);
        else if (ShotIndex == 3)
        {
            // A forced-perspective set keeps the distant point and plate within the same depth range.
            Height = OrbitAltitude + 225.0 * Ease(Time, OrbitCut, End);
            ShipPose.SetScale3D(ParkedPose.GetScale3D() * .12);
        }
        ShipPose.SetLocation(ParkedPose.GetLocation() + FVector::UpVector * Height);
        const float Retract = Ease(Time, HoverCut + 1.4f, HoverCut + 3.94f);
        for (const FGear& Leg : Gear)
        {
            if (auto* Mesh = Leg.Component.Get())
            {
                Mesh->SetRelativeTransform(AtHinge(Leg.Rest, Leg.Mount, FQuat::Identity,
                    FVector(1,1,1.0 - .90 * Retract)));
                Mesh->SetVisibility(Retract < .999f);
            }
        }
    }
    Airframe->SetWorldTransform(ShipPose, false, nullptr, ETeleportType::TeleportPhysics);
    if (ShotIndex == 2) DrawVapor(FMath::Clamp((Time - CloudCut) / (OrbitCut - CloudCut), 0.0f, 1.0f));
    else if (ShotIndex == 3) DrawVapor(FMath::Clamp((Time - OrbitCut) / (End - OrbitCut), 0.0f, 1.0f));
}

void ASSTOLaunchCinematic::DrawVapor(float ShotFraction)
{
    using namespace SSTOLaunch;
    const bool bOrbit = ShotIndex == 3;
    const int32 Count = bOrbit ? OrbitalVaporCount : Plume.Num();
    const FVector Origin = Airframe->GetComponentTransform().TransformPosition(FVector(-83,0,87));
    const FVector Right = Lens->GetRightVector();
    const FVector TowardCamera = -Lens->GetForwardVector();
    for (int32 Index = 0; Index < Count; ++Index)
    {
        const double S = static_cast<double>(Index) / (Count - 1);
        const double Phase = Index * 2.317 + ShotFraction * .94;
        FVector Center;
        double Width, Height;
        float Density;
        if (bOrbit)
        {
            if (Index == 0)
            {
                // The tiny actual spacecraft remains visible through this warm, soft navigation glint.
                Center = Origin + TowardCamera * 180.0;
                Width = 122; Height = 86; Density = .93f;
            }
            else
            {
                Center = Origin - FVector::UpVector * (26.0 + S * (1000.0 + 220.0 * ShotFraction));
                Center += Right * (FMath::Sin(Phase) * 9.0 * S);
                Width = 48.0 + 39.0 * S; Height = 360.0;
                Density = .20f * FMath::Pow(1.0 - S, 1.3) * (1.0f - .71f * ShotFraction);
            }
        }
        else
        {
            const double Spread = FMath::Pow(S, 1.45);
            Center = Origin + FVector::DownVector * (45.0 + S * (3800.0 + 1340.0 * ShotFraction));
            Center += Right * ((FMath::Sin(Phase) * 205.0 + FMath::Cos(Index * .67) * 109.0) * Spread);
            Center += TowardCamera * (24.0 + 89.0 * FMath::Cos(Phase));
            Width = (265.0 + Spread * 1700.0) * (1.0 + .12 * FMath::Sin(Phase));
            Height = 465.0 + 1290.0 * Spread;
            Density = (.57f - .26f * S) * (1.f - Ease(S, .65f, 1.0f));
        }
        SetCard(Plume[Index].Component.Get(), Center, CardRotation, Width, Height);
        Plume[Index].Material->SetScalarParameterValue(DensityParameter, Density);
    }
}

void ASSTOLaunchCinematic::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!Controller.IsValid() || !Controller->PlayerCameraManager)
    {
        FSimpleDelegate Interrupted = OnInterrupted;
        Stop();
        Interrupted.ExecuteIfBound();
        return;
    }
    using namespace SSTOLaunch;
    Elapsed = static_cast<float>(FMath::Clamp(FPlatformTime::Seconds() - StartedAt, 0.0, static_cast<double>(End)));
    const int32 CurrentShot = Elapsed < HoverCut ? 0 : Elapsed < CloudCut ? 1 : Elapsed < OrbitCut ? 2 : 3;
    if (CurrentShot != ShotIndex) CutTo(CurrentShot);
    SetFlightPose(Elapsed);
    if (Elapsed >= End && !bOrbitReported)
    {
        bOrbitReported = true;
        SetActorTickEnabled(false);
        // A listener may Stop or destroy the stage here. Do not touch it after the callback.
        OnOrbitFinished.ExecuteIfBound();
    }
}

void ASSTOLaunchCinematic::CalcCamera(float DeltaTime, FMinimalViewInfo& OutResult)
{
    Lens->GetCameraView(DeltaTime, OutResult);
}

void ASSTOLaunchCinematic::Stop()
{
    SetActorTickEnabled(false);
    ATunaSweeperPlayerController* Player = Controller.Get();
    if (Player && bCameraOwned)
    {
        if (Player->GetViewTarget() == this)
        {
            AActor* Restore = PreviousView.IsValid() ? PreviousView.Get() : Player->GetPawn();
            if (Restore) Player->SetViewTargetWithBlend(Restore, 0.0f);
        }
        Player->SetLaunchCameraFullTickWhenPaused(bPreviousFullTick);
        Player->SetTickableWhenPaused(bPreviousControllerPauseTick);
        if (Player->PlayerCameraManager)
        {
            Player->PlayerCameraManager->CameraStyle = PreviousCameraStyle;
            Player->PlayerCameraManager->SetGameCameraCutThisFrame();
        }
        if (auto* Shake = SuspendedShake.Get()) Shake->EnableModifier();
        for (const auto& Weak : HiddenForView)
            if (AActor* Actor = Weak.Get()) Player->HiddenActors.RemoveSingle(Actor);
    }
    for (const FScreenWidget& Entry : HiddenWidgets)
        if (auto* Widget = Entry.Widget.Get()) Widget->SetVisibility(Entry.Visibility);
    if (bCameraOwned && GetWorld()) GetWorld()->bIsCameraMoveableWhenPaused = bPreviousCameraMoveable;
    for (int32 Index = BuiltComponents.Num() - 1; Index >= 0; --Index)
        if (IsValid(BuiltComponents[Index])) BuiltComponents[Index]->DestroyComponent();
    for (const auto& Weak : HiddenSuns)
        if (auto* Sun = Weak.Get()) Sun->SetVisibility(true);
    for (const auto& Weak : GroundFog)
        if (auto* Fog = Weak.Get()) Fog->SetVisibility(true);
    BuiltComponents.Reset();
    HiddenForView.Reset();
    HiddenWidgets.Reset();
    HiddenSuns.Reset();
    GroundFog.Reset();
    Gear.Reset();
    Plume.Reset();
    Stair = nullptr; Hatch = nullptr; Backdrop = nullptr; BackdropMaterial = nullptr;
    DockStrip = nullptr; DockMaterial = nullptr; CabinLight = nullptr;
    CloudPlate = nullptr; OrbitPlate = nullptr;
    PreviousView.Reset();
    SuspendedShake.Reset();
    Controller.Reset();
    SetOwner(nullptr);
    bCameraOwned = false;
    bOrbitReported = false;
    ShotIndex = INDEX_NONE;
    Elapsed = 0.0f;
}

void ASSTOLaunchCinematic::EndPlay(const EEndPlayReason::Type Reason)
{
    Stop();
    Super::EndPlay(Reason);
}
