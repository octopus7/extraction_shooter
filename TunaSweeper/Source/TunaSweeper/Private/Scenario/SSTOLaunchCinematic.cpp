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
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "Scenario/TunaSweeperEndingActor.h"
#include "UI/TunaSweeperScreenFadeWidget.h"

namespace SSTOLaunch
{
    enum EShot { StairDetail, HatchDetail, GroundLift, PitchUp, CloudMedium, Orbit };
    constexpr float HatchCut = 2.45f;
    constexpr float HatchSwingStart = 2.65f;
    constexpr float StairSeated = 3.72f;
    constexpr float HatchUpright = 4.33f;
    constexpr float HatchSeated = 4.82f;
    constexpr float HoverCut = 5.4f;
    constexpr float PitchCut = 8.75f;
    constexpr float UprightAt = 11.50f;
    constexpr float CloudCut = 14.2f;
    constexpr float OrbitCut = 18.45f;
    constexpr float End = 22.7f;
    constexpr double CloudAltitude = 210000.0;
    constexpr double OrbitAltitude = 1400000.0;
    constexpr double Aspect = 16.0 / 9.0;
    constexpr int32 OrbitalVaporCount = 9;
    constexpr int32 DustDirections = 6;
    constexpr float CabinLumens = 215.f;
    const FVector StairHinge(395, -353, 181);
    const FVector DoorHinge(395, -312, 410);
    const FVector BayHinge(395, -302.5, 109.5);
    const FVector FlightCenter(0, 0, 270);
    const FVector TailCenter(-1145, 0, 300);
    const FName RevealParameter(TEXT("VerticalRevealActive"));
    const FName PlateParameter(TEXT("Plate"));
    const FName TintParameter(TEXT("Tint"));
    const FName DensityParameter(TEXT("Density"));
    const FName SeedParameter(TEXT("Seed"));
    const FName PhaseParameter(TEXT("Phase"));
    const FName NormalCamera(TEXT("Default"));

    float Ease(float Time, float From, float To)
    {
        return FMath::SmoothStep(0.0f, 1.0f, FMath::Clamp((Time - From) / (To - From), 0.0f, 1.0f));
    }

    // Integrate each acceleration from the previous shot's exit velocity (centimeters and seconds).
    double LowFlightHeight(float Time)
    {
        const double Lift = FMath::Clamp(Time - HoverCut, 0.f, PitchCut - HoverCut);
        const double Turn = FMath::Clamp(Time - PitchCut, 0.f, UprightAt - PitchCut);
        const double Vertical = FMath::Max(Time - UprightAt, 0.f);
        constexpr double SpeedAtTurn = 85. + 34. * (PitchCut - HoverCut);
        constexpr double SpeedAtVertical = SpeedAtTurn + 90. * (UprightAt - PitchCut);
        return Lift * (85. + 17. * Lift) + Turn * (SpeedAtTurn + 45. * Turn) +
            Vertical * (SpeedAtVertical + 125. * Vertical);
    }

    double CloudHeight(double Time)
    {
        const double Pass = (Time - CloudCut) / (OrbitCut - CloudCut);
        return CloudAltitude - 5500. + 9410. * Pass * (.90 + .10 * Pass);
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

    UStaticMesh* LoadMechanism(const TCHAR* Name)
    {
        return LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Game/Effects/SSTOEnding/%s.%s"), Name, Name));
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
    FlameAsset = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/Effects/SSTOEnding/M_SSTOEndingFlame.M_SSTOEndingFlame")));
    DustAsset = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/Effects/SSTOEnding/M_SSTOEndingDust.M_SSTOEndingDust")));
    CloudExhaustAsset = TSoftObjectPtr<UParticleSystem>(FSoftObjectPath(TEXT("/Game/Effects/SSTOEnding/PS_SSTOCloudExhaust.PS_SSTOCloudExhaust")));
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

ASSTOLaunchCinematic::FEffectCard ASSTOLaunchCinematic::MakeEffectCard(UStaticMesh* Plane, UMaterialInterface* Master,
    float Seed, int32 Priority)
{
    auto* Card = MakeMesh(Plane, GetRootComponent(), false);
    auto* Material = UMaterialInstanceDynamic::Create(Master, this);
    Material->SetScalarParameterValue(SSTOLaunch::SeedParameter, Seed);
    Material->SetScalarParameterValue(SSTOLaunch::DensityParameter, 0.f);
    Card->SetMaterial(0, Material);
    Card->SetTranslucentSortPriority(Priority);
    Card->SetVisibility(false);
    return {Card, Material};
}

bool ASSTOLaunchCinematic::BuildStage(AActor* Lander)
{
    using namespace SSTOLaunch;
    UStaticMesh* DoorMesh = HatchAsset.LoadSynchronous();
    UStaticMesh* TrayMesh = LoadMechanism(TEXT("SM_SSTO_StairTray"));
    UStaticMesh* AftRailMesh = LoadMechanism(TEXT("SM_SSTO_StairAftRail"));
    UStaticMesh* ForeRailMesh = LoadMechanism(TEXT("SM_SSTO_StairForeRail"));
    UStaticMesh* HardwareMesh = LoadMechanism(TEXT("SM_SSTO_LiftHardware"));
    UStaticMesh* BayCoverMesh = LoadMechanism(TEXT("SM_SSTO_StairBayCover"));
    UStaticMesh* FlightHull = LoadMechanism(TEXT("SM_SSTO_CinematicLowHull"));
    UParticleSystem* CloudExhaust = CloudExhaustAsset.LoadSynchronous();
    UStaticMesh* Plane = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane"));
    UMaterialInterface* PlateMaster = BackdropAsset.LoadSynchronous();
    UMaterialInterface* VaporMaster = VaporAsset.LoadSynchronous();
    UMaterialInterface* FlameMaster = FlameAsset.LoadSynchronous();
    UMaterialInterface* DustMaster = DustAsset.LoadSynchronous();
    CloudPlate = CloudAsset.LoadSynchronous();
    OrbitPlate = OrbitAsset.LoadSynchronous();
    if (!DoorMesh || !TrayMesh || !AftRailMesh || !ForeRailMesh || !HardwareMesh ||
        !BayCoverMesh || !FlightHull || !CloudExhaust || !Plane || !PlateMaster ||
        !VaporMaster || !FlameMaster || !DustMaster || !CloudPlate || !OrbitPlate)
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
        const bool bStair = Name == TEXT("SM_SSTO_Ladder");
        if (bStair) Mesh = TrayMesh;
        else if (Name == TEXT("SM_SSTO_LowHull")) Mesh = FlightHull;
        UStaticMeshComponent* Copy = MakeMesh(Mesh, Airframe, Source->CastShadow);
        Copy->SetRelativeTransform(Rest);
        AssignHullMaterials(Copy, Mesh);
        FVector Mount;
        if (bStair)
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

    AftRail = MakeMesh(AftRailMesh, Airframe, true);
    ForeRail = MakeMesh(ForeRailMesh, Airframe, true);
    AssignHullMaterials(AftRail, AftRailMesh);
    AssignHullMaterials(ForeRail, ForeRailMesh);
    AssignHullMaterials(MakeMesh(HardwareMesh, Airframe, true), HardwareMesh);
    BayCover = MakeMesh(BayCoverMesh, Airframe, true);
    AssignHullMaterials(BayCover, BayCoverMesh);
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

    for (int32 Index = 0; Index < OrbitalVaporCount; ++Index)
    {
        const float Depth = .47f * Index / (OrbitalVaporCount - 1);
        auto& Card = OrbitalVapor.Add_GetRef(MakeEffectCard(Plane, VaporMaster, .37f + Index * 1.793f, 1 + OrbitalVaporCount - Index));
        Card.Material->SetVectorParameterValue(TintParameter,
            FMath::Lerp(FLinearColor(1.95f,1.48f,.97f), FLinearColor(1.08f,.72f,.59f), Depth));
    }

    // Centers are the open lips of the four ventral nozzles and the two existing tail bells.
    struct FOutlet { FVector Center; double Radius; FName Tag; };
    const FOutlet Outlets[] = {
        {FVector(-810,-204,94),61,TEXT("SSTO_MainLiftCore")}, {FVector(-810,204,94),61,TEXT("SSTO_MainLiftCore")},
        {FVector(752,-146,90),34,TEXT("SSTO_NoseLiftCore")}, {FVector(752,146,90),34,TEXT("SSTO_NoseLiftCore")},
        {TailCenter + FVector(0,-154,0),145,TEXT("SSTO_AxialCore")}, {TailCenter + FVector(0,154,0),145,TEXT("SSTO_AxialCore")}
    };
    FCollisionQueryParams Trace(SCENE_QUERY_STAT(SSTOGroundWash), false, Lander);
    Trace.AddIgnoredActor(this);
    if (Controller.IsValid()) Trace.AddIgnoredActor(Controller->GetPawn());
    TArray<AActor*> Attached;
    Lander->GetAllChildActors(Attached);
    Trace.AddIgnoredActors(Attached);
    for (const FOutlet& Outlet : Outlets)
    {
        const float Seed = .81f + Nozzles.Num() * 1.71f;
        const bool bAxial = Outlet.Tag == TEXT("SSTO_AxialCore");
        const FVector Start = ParkedPose.TransformPosition(Outlet.Center);
        FNozzle& Jet = Nozzles.Add_GetRef({Outlet.Center, Outlet.Radius, FVector(Start.X, Start.Y, ParkedPose.GetLocation().Z),
            FVector::UpVector, bAxial, Seed, {}, {}, {}});
        Jet.Core = MakeEffectCard(Plane, FlameMaster, Seed, 31);
        Jet.Core.Component->ComponentTags.Add(Outlet.Tag);
        if (bAxial)
        {
            auto* Particles = NewObject<UParticleSystemComponent>(this, NAME_None, RF_Transient);
            Particles->SetupAttachment(GetRootComponent());
            Particles->bAutoActivate = false;
            Particles->SetTemplate(CloudExhaust);
            Particles->SecondsBeforeInactive = 0.f;
            Particles->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            Particles->SetOnlyOwnerSee(true);
            Particles->SetHiddenInSceneCapture(true);
            Particles->SetVisibility(false);
            Particles->RegisterComponent();
            BuiltComponents.Add(Particles);
            Jet.CloudParticles = Particles;
        }
        else
        {
            FHitResult Hit;
            if (GetWorld()->LineTraceSingleByChannel(Hit, Start + FVector::UpVector * 80.,
                Start - FVector::UpVector * 4000., ECC_Visibility, Trace))
            {
                Jet.GroundPoint = Hit.ImpactPoint;
                Jet.GroundNormal = Hit.ImpactNormal;
            }
            for (int32 Puff = 0; Puff < DustDirections * 2; ++Puff)
            {
                auto& Card = Jet.Dust.Add_GetRef(MakeEffectCard(Plane, DustMaster, Seed+Puff*1.37f, 0));
                Card.Component->ComponentTags.Add(TEXT("SSTO_OutwashPuff"));
            }
        }
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
    AltitudeFade = CreateWidget<UTunaSweeperScreenFadeWidget>(Player);
    if (!AltitudeFade || !BuildStage(Lander))
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
    AltitudeFade->SetVisibility(ESlateVisibility::Collapsed);
    AltitudeFade->SetRenderOpacity(0.f);
    AltitudeFade->AddToPlayerScreen(1000);
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
    case StairDetail:
        Position = ParkedPose.TransformPosition(FVector(1050,-1340,155));
        Target = ParkedPose.TransformPosition(FVector(445,-419,155));
        Fov = 42.f;
        break;
    case HatchDetail:
        Position = ParkedPose.TransformPosition(FVector(155,-1040,277));
        Target = ParkedPose.TransformPosition(FVector(395,-321,267));
        Fov = 49.f;
        break;
    case GroundLift:
        Position = StagePosition(FVector(2340,-4210,760));
        Target = StagePosition(FVector(-60,0,407));
        Fov = 43.f;
        break;
    case PitchUp:
        Position = StagePosition(FVector(1740,-8660,1040));
        Target = StagePosition(FVector(0,0,1540));
        Fov = 53.f;
        break;
    case CloudMedium:
        Position = StagePosition(FVector(-8650,-12100,CloudAltitude + 650));
        Target = StagePosition(FVector(-260,0,CloudAltitude - 400));
        Fov = 42.f;
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
    const bool bMatte = NewShot >= CloudMedium;
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
        UTexture2D* Plate = NewShot == CloudMedium ? CloudPlate.Get() : OrbitPlate.Get();
        BackdropMaterial->SetTextureParameterValue(PlateParameter, Plate);
        const double Distance = NewShot == CloudMedium ? 39000.0 : 58000.0;
        const double ViewWidth = 2.0 * Distance * FMath::Tan(FMath::DegreesToRadians(Fov * .5));
        const double ArtAspect = static_cast<double>(Plate->GetSizeX()) / FMath::Max(1, Plate->GetSizeY());
        const double Width = FMath::Max(ViewWidth, ViewWidth / Aspect * ArtAspect) * 1.025;
        SetCard(Backdrop, Position + Lens->GetForwardVector() * Distance, CardRotation, Width, Width / ArtAspect);
    }
    if (bMatte)
    {
        // These distant plates already include atmospheric haze.
        for (const auto& Weak : GroundFog)
            if (auto* Fog = Weak.Get()) Fog->SetVisibility(false);
        for (const FNozzle& Jet : Nozzles)
            for (const auto& Puff : Jet.Dust) Puff.Component->SetVisibility(false);
    }
    for (const FEffectCard& Card : OrbitalVapor) Card.Component->SetVisibility(NewShot == Orbit);
    if (NewShot == Orbit)
    {
        for (const FNozzle& Jet : Nozzles)
            if (auto* FX = Jet.CloudParticles.Get())
            {
                FX->SetVisibility(false);
                FX->DeactivateSystem();
                FX->ResetParticles();
                FX->SetComponentTickEnabled(false);
            }
    }
    if (Controller.IsValid() && Controller->PlayerCameraManager)
        Controller->PlayerCameraManager->SetGameCameraCutThisFrame();
}

void ASSTOLaunchCinematic::SetBoardingPose(float Time)
{
    using namespace SSTOLaunch;
    // Omit the middle of the fold. The reverse angle still catches the last few rungs travelling into the sill.
    const bool bShowingFold = ShotIndex == StairDetail;
    const float RailFold = bShowingFold ? Ease(Time,.67f,2.77f) : 1.f;
    const float TrayFold = bShowingFold ? Ease(Time,.94f,3.48f) :
        FMath::Lerp(.89f, 1.f, Ease(Time, HatchCut-.03f, HatchCut+.70f));
    const float Inboard = bShowingFold ? 14.f * Ease(Time,1.72f,HatchCut) : FMath::Lerp(186.f,419.f,Ease(Time,HatchCut-.22f,StairSeated));
    const float Drop = bShowingFold ? 5.f * TrayFold : 46.f;
    const FVector DeployedVector = StairRest.TransformVector(FVector(0,-245,-181));
    const double LevelAngle = FMath::Atan2(DeployedVector.Z, -DeployedVector.Y);
    const FQuat TrayRotation(FVector::ForwardVector, LevelAngle * TrayFold);
    const FVector Hinge = StairRest.TransformPosition(StairHinge);
    const FTransform Carriage(TrayRotation, Hinge - TrayRotation.RotateVector(Hinge) + FVector(0,Inboard,-Drop));
    Stair->SetRelativeTransform(StairRest * Carriage);
    const FVector RailAxis = DeployedVector.GetSafeNormal();
    // The two rails stack at different stop angles to clear each other at the center of the tray.
    AftRail->SetRelativeTransform(AtHinge(StairRest, FVector(320,-356,176),
        FQuat(RailAxis, FMath::DegreesToRadians(-80.0 * RailFold))) * Carriage);
    ForeRail->SetRelativeTransform(AtHinge(StairRest, FVector(470,-356,176),
        FQuat(RailAxis, FMath::DegreesToRadians(87.0 * RailFold))) * Carriage);
    const double CoverAngle = 68. * (1. - Ease(Time, StairSeated, HatchSeated));
    BayCover->SetRelativeTransform(AtHinge(FTransform::Identity, BayHinge,
        FQuat(FVector::ForwardVector, FMath::DegreesToRadians(CoverAngle))));

    const float Swing = Ease(Time, HatchSwingStart, HatchUpright);
    const float Seat = Ease(Time, HatchUpright, HatchSeated);
    const FQuat DoorRotation(FVector::ForwardVector, FMath::DegreesToRadians(92.0 * (1.0 - Swing)));
    Hatch->SetRelativeTransform(AtHinge(FTransform::Identity, DoorHinge, DoorRotation, FVector::OneVector,
        FVector(0, 26.0 * (1.0 - Seat), 0)));
    if (ShotIndex <= HatchDetail)
    {
        const float CabinGlow = 1.0f - Ease(Time, HatchSwingStart, HatchSeated);
        DockMaterial->SetScalarParameterValue(DensityParameter, .72f * CabinGlow);
        DockStrip->SetVisibility(CabinGlow > .002f);
        if (CabinLight) CabinLight->SetIntensity(CabinLumens * CabinGlow);
    }
    else
    {
        DockStrip->SetVisibility(false);
        if (CabinLight) CabinLight->SetVisibility(false);
    }
}

void ASSTOLaunchCinematic::SetFlightPose(float Time)
{
    using namespace SSTOLaunch;
    SetBoardingPose(Time);
    FTransform ShipPose = ParkedPose;
    if (ShotIndex >= GroundLift)
    {
        const float Lift = Ease(Time, HoverCut, PitchCut);
        const float Rotation = Ease(Time, PitchCut, UprightAt);
        FQuat Attitude = FQuat::Slerp(ParkedPose.GetRotation(), LevelFrame.GetRotation(), Lift);
        if (ShotIndex >= PitchUp) Attitude = LevelFrame.GetRotation() * FRotator(90.0 * Rotation, 0, 0).Quaternion();
        ShipPose.SetRotation(Attitude);
        double Height = LowFlightHeight(Time);
        if (ShotIndex == CloudMedium) Height = CloudHeight(Time);
        if (ShotIndex == Orbit)
        {
            Height = OrbitAltitude + 100.0 * (Time - OrbitCut);
            ShipPose.SetScale3D(ParkedPose.GetScale3D() * .12);
        }
        const FVector Center = ParkedPose.TransformPosition(FlightCenter) + FVector::UpVector * Height;
        ShipPose.SetLocation(Center - ShipPose.TransformVector(FlightCenter));
        const float Retract = Ease(Time, HoverCut + .35f, PitchCut);
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
    DrawNozzles(Time);
    if (ShotIndex == CloudMedium) AdvanceCloudExhaust(Time);
    else if (ShotIndex == Orbit) DrawOrbitalVapor(FMath::Clamp((Time - OrbitCut) / (End - OrbitCut), 0.0f, 1.0f));
}

void ASSTOLaunchCinematic::DrawNozzles(float Time)
{
    using namespace SSTOLaunch;
    const float LiftBurn = Ease(Time, HoverCut-.20f, HoverCut+.12f) * (1.f - Ease(Time, PitchCut+.20f, UprightAt-.35f));
    const float AxialBurn = ShotIndex == PitchUp ? Ease(Time, PitchCut+.16f, UprightAt-.50f) : 0.f;
    const FVector TowardCamera = -Lens->GetForwardVector();
    for (FNozzle& Jet : Nozzles)
    {
        if (!Jet.bAxial && ShotIndex >= GroundLift && ShotIndex <= PitchUp) DrawGroundDust(Jet, Time);
        const float Power = Jet.bAxial ? AxialBurn : LiftBurn;
        const bool bLit = Power > .005f;
        Jet.Core.Component->SetVisibility(bLit);
        if (!bLit) continue;

        const FVector Mouth = Airframe->GetComponentTransform().TransformPosition(Jet.Socket);
        const FVector Axis = Jet.bAxial ? -Airframe->GetForwardVector() : FVector::DownVector;
        FVector Across = FVector::CrossProduct(TowardCamera, Axis).GetSafeNormal();
        if (Across.IsNearlyZero()) Across = Lens->GetRightVector();
        const FQuat Facing = FRotationMatrix::MakeFromYX(Axis, Across).ToQuat();
        const double Flicker = 1. + .045 * FMath::Sin(17.5 * Time + Jet.Seed);
        const double FreeLength = Jet.Radius * (Jet.bAxial ? 5.2 : 3.35) * (.40 + .60 * Power) * Flicker;
        const double Length = Jet.bAxial ? FreeLength : FMath::Min(FreeLength, FMath::Max(8., Mouth.Z - Jet.GroundPoint.Z));
        SetCard(Jet.Core.Component.Get(), Mouth + Axis * (.5 * Length), Facing, Jet.Radius * (1.18 + .98 * Power), Length);
        Jet.Core.Material->SetScalarParameterValue(DensityParameter, Power);
        Jet.Core.Material->SetScalarParameterValue(PhaseParameter, Time);
    }
}

void ASSTOLaunchCinematic::DrawGroundDust(FNozzle& Jet, float Time)
{
    using namespace SSTOLaunch;
    const double RestGap = ParkedPose.TransformPosition(Jet.Socket).Z - Jet.GroundPoint.Z;
    for (int32 Index = 0; Index < Jet.Dust.Num(); ++Index)
    {
        const int32 Lane = Index % DustDirections;
        const float Lifetime = 1.48f + .19f * FMath::Sin(Jet.Seed + Lane * 1.7f);
        const float Offset = FMath::Frac(.19f * Jet.Seed + .381966f * Lane + .5f * (Index / DustDirections));
        const float Cycle = (Time - HoverCut + .30f) / Lifetime + Offset;
        const float Age = FMath::Frac(Cycle);
        const float Born = Time - Age * Lifetime;
        const double GapAtEmission = RestGap + LowFlightHeight(Born);
        const float Emission = Ease(Born, HoverCut-.30f, HoverCut+.06f) * (1.f - Ease(Born, PitchCut, PitchCut+1.05f))
            * (1. - FMath::SmoothStep(400., 850., GapAtEmission));
        const float Density = Emission * Ease(Age, 0.f, .14f) * (1.f - Ease(Age, .51f, 1.f));
        auto& Puff = Jet.Dust[Index];
        Puff.Component->SetVisibility(Density > .012f);
        if (Density <= .012f) continue;

        const double Angle = 2. * PI * Lane / DustDirections + .20 * Jet.Seed + .13 * FMath::Sin(FMath::FloorToDouble(Cycle) + Lane);
        const FVector Radial = LevelFrame.TransformVectorNoScale(FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0));
        const double Reach = (Jet.Radius * 9. + 160.) * Age * (1.46 - .46 * Age);
        FVector OffsetOnGround = Radial * (Jet.Radius * .4 + Reach);
        OffsetOnGround.Z = -FVector::DotProduct(Jet.GroundNormal, OffsetOnGround) / FMath::Max(.25, Jet.GroundNormal.Z);
        const double Width = (2.0 + 6.6 * Age) * Jet.Radius + 81.;
        const double Height = (32. + Jet.Radius * .30 + 109. * Age) * (1. + .17 * FMath::Sin(Lane + Jet.Seed));
        SetCard(Puff.Component.Get(), Jet.GroundPoint + OffsetOnGround + FVector::UpVector * (Height * .39),
            CardRotation, Width, Height);
        Puff.Material->SetScalarParameterValue(PhaseParameter, Time * .65f);
        Puff.Material->SetScalarParameterValue(DensityParameter, Density * .82f);
    }
}

void ASSTOLaunchCinematic::AdvanceCloudExhaust(float Time)
{
    using namespace SSTOLaunch;
    // The raid is paused. Step these two CPU sprite systems on the film clock, at the historical
    // nozzle positions, so a low frame rate or the altitude cut cannot stretch the trail.
    constexpr double Step = 1. / 60.;
    constexpr double FirstEmission = CloudCut - .85;
    const int32 TargetStep = FMath::FloorToInt((Time - FirstEmission) / Step);
    for (const FNozzle& Jet : Nozzles)
    {
        auto* FX = Jet.CloudParticles.Get();
        if (!FX) continue;
        const FVector Mouth = Airframe->GetComponentTransform().TransformPosition(Jet.Socket);
        const double CurrentHeight = CloudHeight(Time);
        auto MouthAt = [&](int32 Index)
        {
            return Mouth + FVector::UpVector * (CloudHeight(FirstEmission + Index * Step) - CurrentHeight);
        };
        if (LastExhaustStep == INDEX_NONE)
        {
            FX->SetWorldLocation(MouthAt(0));
            FX->SetVisibility(true);
            FX->ActivateSystem(true);
            FX->SetComponentTickEnabled(false);
        }
        for (int32 Index = FMath::Max(1, LastExhaustStep + 1); Index <= TargetStep; ++Index)
        {
            FX->SetWorldLocation(MouthAt(Index));
            FX->TickComponent(static_cast<float>(Step), LEVELTICK_All, nullptr);
        }
    }
    LastExhaustStep = TargetStep;
}

void ASSTOLaunchCinematic::DrawOrbitalVapor(float ShotFraction)
{
    using namespace SSTOLaunch;
    const FVector Origin = Airframe->GetComponentTransform().TransformPosition(FlightCenter);
    const FVector Right = Lens->GetRightVector();
    const FVector TowardCamera = -Lens->GetForwardVector();
    for (int32 Index = 0; Index < OrbitalVapor.Num(); ++Index)
    {
        const double S = static_cast<double>(Index) / (OrbitalVapor.Num() - 1);
        const double Phase = Index * 2.317 + ShotFraction * .94;
        FVector Center;
        double Width, Height;
        float Density;
        if (Index == 0)
        {
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
        SetCard(OrbitalVapor[Index].Component.Get(), Center, CardRotation, Width, Height);
        OrbitalVapor[Index].Material->SetScalarParameterValue(DensityParameter, Density);
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
    const int32 CurrentShot = Elapsed < HatchCut ? StairDetail : Elapsed < HoverCut ? HatchDetail :
        Elapsed < PitchCut ? GroundLift : Elapsed < CloudCut ? PitchUp : Elapsed < OrbitCut ? CloudMedium : Orbit;
    if (CurrentShot != ShotIndex) CutTo(CurrentShot);
    SetFlightPose(Elapsed);
    const float Black = Ease(Elapsed, CloudCut-1.03f, CloudCut-.18f) * (1.f - Ease(Elapsed, CloudCut+.22f, CloudCut+.82f));
    AltitudeFade->SetVisibility(Black > 0.f ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
    AltitudeFade->SetRenderOpacity(Black);
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
    if (AltitudeFade)
    {
        AltitudeFade->RemoveFromParent();
        AltitudeFade = nullptr;
    }
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
    OrbitalVapor.Reset();
    LastExhaustStep = INDEX_NONE;
    Nozzles.Reset();
    Stair = nullptr; AftRail = nullptr; ForeRail = nullptr; BayCover = nullptr; Hatch = nullptr; Backdrop = nullptr; BackdropMaterial = nullptr;
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
