#include "LoopRailStation.h"
#include "LoopRailTrack.h"
#include "LoopRailTrain.h"
#include "LoopRailArrivalWidget.h"
#include "Components/WidgetComponent.h"
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
    ArrivalDisplay = CreateDefaultSubobject<UWidgetComponent>(TEXT("ArrivalDisplay"));
    ArrivalDisplay->SetupAttachment(Root);
    ArrivalDisplay->SetMobility(EComponentMobility::Movable);
    ArrivalDisplay->SetWidgetClass(ULoopRailArrivalWidget::StaticClass());
    ArrivalDisplay->SetWidgetSpace(EWidgetSpace::World);
    ArrivalDisplay->SetDrawSize(FVector2D(460, 220));
    ArrivalDisplay->SetBlendMode(EWidgetBlendMode::Opaque);
    ArrivalDisplay->SetTwoSided(true);
    ArrivalDisplay->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ArrivalDisplay->SetGenerateOverlapEvents(false);
    ArrivalDisplay->SetCanEverAffectNavigation(false);
    ArrivalDisplay->SetWindowFocusable(false);
    ArrivalDisplay->SetTickWhenOffscreen(true);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    for (auto* Mesh : {Platform.Get(), AccessRamp.Get()})
    { Mesh->SetStaticMesh(Cube.Object); Mesh->SetMobility(EComponentMobility::Static); Mesh->SetCollisionProfileName(TEXT("BlockAll")); }
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
    if (GetNetMode() == NM_DedicatedServer) { ArrivalDisplay->SetVisibility(false); SetActorTickEnabled(false); return; }
    UpdateArrivalDisplayTransform();
    ArrivalDisplay->InitWidget();
    RefreshArrivalDisplay();
}

void ALoopRailStation::UpdateArrivalDisplayTransform()
{
    const float Side = bPlatformOnRight ? 1.f : -1.f;
    ArrivalDisplay->SetRelativeLocation(FVector(450-FMath::Max(200.f, PlatformLength)*.5,
        Side*(115+FMath::Max(100.f, PlatformWidth)*.5), ArrivalDisplayHeight));
    ArrivalDisplay->SetRelativeScale3D(FVector(FMath::Max(.1f, ArrivalDisplayScale)));
    ArrivalDisplay->SetVisibility(bShowArrivalDisplay && bShowPlatform);
}

void ALoopRailStation::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    ArrivalDisplay->SetVisibility(bShowArrivalDisplay && bShowPlatform);
    if (!bShowArrivalDisplay || !bShowPlatform) return;
    if (const APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(this, 0))
    {
        // Parallel to the view plane also keeps orthographic/top-down cameras upright.
        const FQuat View = Camera->GetCameraRotation().Quaternion();
        ArrivalDisplay->SetWorldRotation(FRotationMatrix::MakeFromXZ(-View.GetForwardVector(), View.GetUpVector()).Rotator());
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
