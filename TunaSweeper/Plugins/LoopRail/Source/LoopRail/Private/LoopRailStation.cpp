#include "LoopRailStation.h"
#include "LoopRailTrack.h"
#include "LoopRailTrain.h"
#include "LoopRailArrivalWidget.h"
#include "Components/WidgetComponent.h"
#include "Components/CapsuleComponent.h"
#include "Materials/MaterialInterface.h"
#include "Camera/PlayerCameraManager.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "Internationalization/Internationalization.h"
#include "Components/SceneComponent.h"
#include "Components/ArrowComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

ALoopRailStation::ALoopRailStation()
{
    PrimaryActorTick.bCanEverTick = true;
    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root")); SetRootComponent(Root);
    Root->SetMobility(EComponentMobility::Static);
    StopMarker = CreateDefaultSubobject<UArrowComponent>(TEXT("LocomotiveStop")); StopMarker->SetupAttachment(Root);
    Platform = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Platform")); Platform->SetupAttachment(Root);
    AccessRamp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AccessRamp")); AccessRamp->SetupAttachment(Root);
    ArrivalMarkerPivot = CreateDefaultSubobject<USceneComponent>(TEXT("ArrivalMarkerPivot"));
    ArrivalMarkerPivot->SetupAttachment(Root);
    ArrivalMarkerPivot->SetMobility(EComponentMobility::Movable);
    ArrivalMarkerPivot->SetAbsolute(false, true, false);
    ArrivalMarkerBody = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ArrivalMarkerBody"));
    ArrivalMarkerBody->SetupAttachment(ArrivalMarkerPivot);
    ArrivalMarkerBody->SetMobility(EComponentMobility::Movable);
    ArrivalMarkerBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ArrivalMarkerBody->SetCanEverAffectNavigation(false);
    ArrivalMarkerCollision = CreateDefaultSubobject<UCapsuleComponent>(TEXT("ArrivalMarkerCollision"));
    ArrivalMarkerCollision->SetupAttachment(Root);
    ArrivalMarkerCollision->SetAbsolute(false, true, false);
    ArrivalMarkerCollision->SetCollisionProfileName(TEXT("BlockAll"));
    ArrivalMarkerCollision->SetGenerateOverlapEvents(false);
    ArrivalMarkerCollision->CanCharacterStepUpOn = ECB_No;
    ArrivalDisplay = CreateDefaultSubobject<UWidgetComponent>(TEXT("ArrivalDisplay"));
    ArrivalDisplay->SetupAttachment(ArrivalMarkerPivot);
    ArrivalDisplay->SetMobility(EComponentMobility::Movable);
    ArrivalDisplay->SetWidgetClass(ULoopRailArrivalWidget::StaticClass());
    ArrivalDisplay->SetWidgetSpace(EWidgetSpace::World);
    ArrivalDisplay->SetDrawSize(FVector2D(460, 220));
    ArrivalDisplay->SetBlendMode(EWidgetBlendMode::Opaque);
    ArrivalDisplay->SetTwoSided(false);
    ArrivalDisplay->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ArrivalDisplay->SetGenerateOverlapEvents(false);
    ArrivalDisplay->SetCanEverAffectNavigation(false);
    ArrivalDisplay->SetWindowFocusable(false);
    ArrivalDisplay->SetTickWhenOffscreen(true);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> MarkerMaterial(TEXT("/LoopRail/Materials/M_StationArrivalBody.M_StationArrivalBody"));
    ArrivalMarkerBody->SetStaticMesh(Cube.Object);
    ArrivalMarkerBody->SetMaterial(0, MarkerMaterial.Object);
    for (auto* Mesh : {Platform.Get(), AccessRamp.Get()})
    { Mesh->SetStaticMesh(Cube.Object); Mesh->SetMobility(EComponentMobility::Static); Mesh->SetCollisionProfileName(TEXT("BlockAll")); }
    UpdateArrivalDisplayTransform();
}
void ALoopRailStation::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    const float Side = bPlatformOnRight ? 1.f : -1.f;
    const float Width = FMath::Max(100.f, PlatformWidth), Length = FMath::Max(200.f, PlatformLength);
    Platform->SetRelativeLocation(FVector(450-Length*.5, Side*(115+Width*.5), 30));
    Platform->SetRelativeScale3D(FVector(Length/100, Width/100, .6));
    AccessRamp->SetRelativeLocation(FVector(250, Side*(115+Width+150), 10));
    AccessRamp->SetRelativeRotation(FRotator(0,0,Side*11.31f));
    AccessRamp->SetRelativeScale3D(FVector(2, 3.06, .4));
    UpdateArrivalDisplayTransform();
    for (auto* Mesh : {Platform.Get(), AccessRamp.Get()})
    { Mesh->SetVisibility(bShowPlatform); Mesh->SetCollisionEnabled(bShowPlatform ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision); }
}

void ALoopRailStation::BeginPlay()
{
    Super::BeginPlay();
    UpdateArrivalDisplayTransform();
    if (GetNetMode() == NM_DedicatedServer) { ArrivalDisplay->SetVisibility(false); ArrivalMarkerBody->SetVisibility(false); SetActorTickEnabled(false); return; }
    ArrivalDisplay->InitWidget();
    RefreshArrivalDisplay();
}

void ALoopRailStation::UpdateArrivalDisplayTransform()
{
    const float Side = bPlatformOnRight ? 1.f : -1.f;
    const float Height = FMath::Max(120.f, ArrivalMarkerHeight);
    const float Width = FMath::Max(40.f, ArrivalMarkerWidth);
    const float Depth = FMath::Max(10.f, ArrivalMarkerDepth);
    const FVector Base(450-FMath::Max(200.f, PlatformLength)*.5, Side*(115+FMath::Max(100.f, PlatformWidth)*.5), 60);
    ArrivalMarkerPivot->SetRelativeLocation(Base);
    ArrivalMarkerBody->SetRelativeLocation(FVector(0, 0, Height*.5f));
    ArrivalMarkerBody->SetRelativeScale3D(FVector(Depth, Width, Height)/100.f);
    // A fixed round footprint keeps collision identical for all viewing directions/clients.
    ArrivalMarkerCollision->SetRelativeLocation(Base+FVector(0, 0, Height*.5f));
    ArrivalMarkerCollision->SetCapsuleSize(FMath::Sqrt(Width*Width+Depth*Depth)*.5f, Height*.5f);
    const float FaceScale = (Width-8.f)/460.f;
    ArrivalDisplay->SetRelativeLocation(FVector(Depth*.5f+.3f, 0, Height-10.f-110.f*FaceScale));
    ArrivalDisplay->SetRelativeRotation(FRotator::ZeroRotator);
    ArrivalDisplay->SetRelativeScale3D(FVector(FaceScale));
    const bool bVisible = bShowArrivalDisplay && bShowPlatform;
    ArrivalDisplay->SetVisibility(bVisible);
    ArrivalMarkerBody->SetVisibility(bVisible);
    ArrivalMarkerCollision->SetCollisionEnabled(bVisible ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
}

void ALoopRailStation::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    ArrivalDisplay->SetVisibility(bShowArrivalDisplay && bShowPlatform);
    ArrivalMarkerBody->SetVisibility(bShowArrivalDisplay && bShowPlatform);
    ArrivalMarkerCollision->SetCollisionEnabled(bShowArrivalDisplay && bShowPlatform ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
    if (!bShowArrivalDisplay || !bShowPlatform) return;
    if (const APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(this, 0))
    {
        FVector Facing = Camera->GetCameraLocation()-ArrivalMarkerPivot->GetComponentLocation();
        Facing.Z = 0;
        // Keep the existing yaw directly overhead; never tip the grounded body toward the camera.
        if (!Facing.IsNearlyZero()) ArrivalMarkerPivot->SetWorldRotation(FRotator(0, Facing.Rotation().Yaw, 0));
    }
    DisplayRefreshRemaining -= DeltaSeconds;
    if (DisplayRefreshRemaining <= 0)
    {
        DisplayRefreshRemaining = .25f;
        RefreshArrivalDisplay();
    }
}

bool ALoopRailStation::TryGetNextArrivalSeconds(double& OutSeconds) const
{
    OutSeconds = 0;
    if (!bEnabled || !IsValid(Track) || !GetWorld()) return false;
    bool bFound = false;
    for (TActorIterator<ALoopRailTrain> It(GetWorld()); It; ++It)
    {
        double Seconds = 0;
        if (It->TryGetArrivalSeconds(this, Seconds) && (!bFound || Seconds < OutSeconds))
        { bFound = true; OutSeconds = Seconds; }
    }
    return bFound;
}

void ALoopRailStation::SetArrivalTextFormats(const FText& TimeFormat, const FText& UnavailableText)
{
    ArrivalTimeFormat = TimeFormat;
    ArrivalUnavailableText = UnavailableText;
    RefreshArrivalDisplay();
}

FText ALoopRailStation::GetArrivalDisplayText() const
{
    double Seconds = 0;
    if (!TryGetNextArrivalSeconds(Seconds)) return ArrivalUnavailableText;
    if (ArrivalTimeFormat.IsEmpty()) return FText::GetEmpty();
    const int32 Total = FMath::CeilToInt(FMath::Clamp(Seconds, 0.0, 359999.0));
    FNumberFormattingOptions MinutesOptions;
    MinutesOptions.SetUseGrouping(false);
    FNumberFormattingOptions SecondsOptions = MinutesOptions;
    SecondsOptions.SetMinimumIntegralDigits(2);
    FFormatNamedArguments Arguments;
    Arguments.Add(TEXT("Minutes"), FText::AsNumber(Total / 60, &MinutesOptions, FInternationalization::Get().GetInvariantCulture()));
    Arguments.Add(TEXT("Seconds"), FText::AsNumber(Total % 60, &SecondsOptions, FInternationalization::Get().GetInvariantCulture()));
    return FText::Format(ArrivalTimeFormat, Arguments);
}

void ALoopRailStation::RefreshArrivalDisplay()
{
    if (auto* Widget = Cast<ULoopRailArrivalWidget>(ArrivalDisplay->GetUserWidgetObject()))
        Widget->SetArrivalDisplayText(GetArrivalDisplayText());
}
double ALoopRailStation::GetStopDistance() const { return IsValid(Track) ? Track->FindDistance(GetActorLocation()) : 0; }
void ALoopRailStation::AlignToTrack()
{
    if (IsValid(Track)) { SetActorTransform(Track->Sample(GetStopDistance())); OnConstruction(GetActorTransform()); }
}
