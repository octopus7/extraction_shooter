#include "Interaction/TunaSweeperRegionLinkActor.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Subsystem/TunaSweeperItemDataSubsystem.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"

ATunaSweeperRegionLinkActor::ATunaSweeperRegionLinkActor()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = false;
    LeftDoorSlide = CreateDefaultSubobject<USceneComponent>(TEXT("LeftDoorSlide"));
    LeftDoorSlide->SetupAttachment(RootComponent);
    RightDoorSlide = CreateDefaultSubobject<USceneComponent>(TEXT("RightDoorSlide"));
    RightDoorSlide->SetupAttachment(RootComponent);
    LeftDoorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftDoorMesh"));
    LeftDoorMesh->SetupAttachment(LeftDoorSlide);
    RightDoorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightDoorMesh"));
    RightDoorMesh->SetupAttachment(RightDoorSlide);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    for (auto* Door : {LeftDoorMesh.Get(), RightDoorMesh.Get()})
    {
        Door->SetStaticMesh(Cube.Object);
        Door->SetRelativeScale3D(FVector(.14f,2.1f,2.8f));
        Door->SetMobility(EComponentMobility::Movable);
        Door->SetCollisionProfileName(TEXT("BlockAllDynamic"));
        Door->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Door->SetVisibility(false);
    }
    LeftDoorMesh->SetRelativeLocation(FVector(230,-105,140));
    RightDoorMesh->SetRelativeLocation(FVector(230,105,140));
    TravelInterior = CreateDefaultSubobject<UBoxComponent>(TEXT("TravelInterior"));
    TravelInterior->SetupAttachment(RootComponent);
    TravelInterior->SetRelativeLocation(FVector(-20,0,150));
    TravelInterior->SetBoxExtent(FVector(175,180,150));
    TravelInterior->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    TravelInterior->SetGenerateOverlapEvents(false);
    RepairInteractionPoint = CreateDefaultSubobject<USceneComponent>(TEXT("RepairInteractionPoint"));
    RepairInteractionPoint->SetupAttachment(RootComponent);
    RepairInteractionPoint->SetRelativeLocation(FVector(340,0,100));
    InteriorInteractionPoint = CreateDefaultSubobject<USceneComponent>(TEXT("InteriorInteractionPoint"));
    InteriorInteractionPoint->SetupAttachment(RootComponent);
    InteriorInteractionPoint->SetRelativeLocation(FVector(0,0,110));
}
void ATunaSweeperRegionLinkActor::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    DoorOpenAlpha = bPreviewDoorsOpen ? 1.f : 0.f;
    ApplyDoorPresentation();
    RefreshPassage();
}
void ATunaSweeperRegionLinkActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!bUseSlidingDoors) return;
    DoorOpenAlpha = FMath::FInterpConstantTo(DoorOpenAlpha, bDoorTargetOpen ? 1.f : 0.f,
        DeltaSeconds, 1.f / FMath::Max(.01f, DoorOpenSeconds));
    ApplyDoorPresentation();
    if (bInteriorTravelAvailable != IsLocalPlayerInside()) RefreshPassage();
}
void ATunaSweeperRegionLinkActor::ApplyDoorPresentation()
{
    LeftDoorSlide->SetRelativeLocation(LeftDoorOpenOffset * DoorOpenAlpha);
    RightDoorSlide->SetRelativeLocation(RightDoorOpenOffset * DoorOpenAlpha);
    for (auto* Door : {LeftDoorMesh.Get(), RightDoorMesh.Get()})
    {
        Door->SetVisibility(bUseSlidingDoors);
        Door->SetCollisionEnabled(bUseSlidingDoors ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
    }
}
bool ATunaSweeperRegionLinkActor::CanUseLiftInterior(const APawn* Pawn) const
{
    if (!bUseSlidingDoors || !Pawn || DoorOpenAlpha < 1.f || !IsPassageUnlocked()) return false;
    const FVector Local = TravelInterior->GetComponentTransform().InverseTransformPosition(Pawn->GetActorLocation());
    const FVector Extent = TravelInterior->GetUnscaledBoxExtent();
    return FMath::Abs(Local.X) <= Extent.X && FMath::Abs(Local.Y) <= Extent.Y && FMath::Abs(Local.Z) <= Extent.Z;
}
bool ATunaSweeperRegionLinkActor::IsLocalPlayerInside() const
{
    const auto* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
    return PC && CanUseLiftInterior(PC->GetPawn());
}
void ATunaSweeperRegionLinkActor::BeginPlay()
{
    Super::BeginPlay();
    if (auto* Game = GetGameInstance<UTunaSweeperGameInstance>())
    {
        Game->OnWorldProgressChanged.AddUObject(this, &ThisClass::RefreshPassage);
        Game->OnInventoryStateChanged.AddUObject(this, &ThisClass::RefreshPassage);
        Game->OnLanguageChanged.AddUObject(this, &ThisClass::RefreshPassage);
    }
    DoorOpenAlpha = IsPassageUnlocked() ? 1.f : 0.f;
    ApplyDoorPresentation();
    SetActorTickEnabled(bUseSlidingDoors);
    RefreshPassage();
}
void ATunaSweeperRegionLinkActor::EndPlay(const EEndPlayReason::Type Reason)
{
    if (auto* Game = GetGameInstance<UTunaSweeperGameInstance>())
    {
        Game->OnWorldProgressChanged.RemoveAll(this);
        Game->OnInventoryStateChanged.RemoveAll(this);
        Game->OnLanguageChanged.RemoveAll(this);
    }
    Super::EndPlay(Reason);
}
bool ATunaSweeperRegionLinkActor::IsPassageUnlocked() const
{
    if (RequiredProgressId.IsNone()) return true;
    const auto* Game = GetGameInstance<UTunaSweeperGameInstance>();
    FTunaSweeperWorldProgressSaveData State;
    return Game && Game->TryGetWorldProgressState(RequiredProgressId, State) && State.State == ETunaSweeperWorldProgressState::Completed;
}
bool ATunaSweeperRegionLinkActor::RepairPassage(bool bSaveImmediately)
{
    auto* Game = GetGameInstance<UTunaSweeperGameInstance>();
    if (!Game || bRepairing || RequiredProgressId.IsNone() || RepairParts.IsEmpty() || IsPassageUnlocked()) return false;
    TGuardValue<bool> Guard(bRepairing, true);
    const bool bRepaired = Game->TryConsumeInventoryItems(RepairParts, [this, Game]()
    {
        Game->UpdateWorldProgressState(RequiredProgressId, TEXT("world_progress.region_passage"), ETunaSweeperWorldProgressState::Completed, 1, 1, false);
    });
    if (bRepaired && bSaveImmediately) Game->SaveGameState();
    RefreshPassage();
    return bRepaired;
}
void ATunaSweeperRegionLinkActor::RefreshPassage()
{
    if (!InteractableComponent) return;
    const bool bUnlocked = IsPassageUnlocked();
    bDoorTargetOpen = bUnlocked;
    bInteriorTravelAvailable = IsLocalPlayerInside();
    if (bUseSlidingDoors)
        InteractableComponent->SetWorldLocation((bUnlocked ? InteriorInteractionPoint : RepairInteractionPoint)->GetComponentLocation());
    auto* Game = GetGameInstance<UTunaSweeperGameInstance>();
    FText Label;
    FName Key = TravelStringKey;
    auto Type = ETunaSweeperInteractionType::LevelTravel;
    if (!bUnlocked)
    {
        Type = RepairParts.IsEmpty() ? ETunaSweeperInteractionType::None : ETunaSweeperInteractionType::WorldProgress;
        Key = TEXT("ui.interaction.repair");
        if (Game && !RepairParts.IsEmpty())
        {
            TArray<FText> PartLabels;
            auto* Items = Game->GetSubsystem<UTunaSweeperItemDataSubsystem>();
            for (const auto& Part : RepairParts)
            {
                FText Name;
                if (Items) Items->TryGetItemNameText(Part.ItemId, Game->GetCurrentTextLanguage(), Name);
                PartLabels.Add(FText::Format(Game->ResolveLocalizedText(TEXT("ui.passage.part"), FText::GetEmpty()), Name, FText::AsNumber(Part.Quantity)));
            }
            Label = FText::Format(Game->ResolveLocalizedText(TEXT("ui.passage.repair"), FText::GetEmpty()), FText::Join(Game->ResolveLocalizedText(TEXT("ui.passage.separator"), FText::GetEmpty()), PartLabels));
            Key = NAME_None; // Already localized, with item names and quantities formatted above.
        }
    }
    if (Game && Label.IsEmpty()) Label = Game->ResolveLocalizedText(Key, FText::GetEmpty());
    if (bUseSlidingDoors && bUnlocked && !bInteriorTravelAvailable) Type = ETunaSweeperInteractionType::None;
    InteractableComponent->SetInteractionTypeDisplayNameAndStringKey(Type, Label, Key);
    InteractableComponent->SetMarkerCompleted(false);
}
bool ATunaSweeperRegionLinkActor::TravelToTargetLevel(APawn* Pawn)
{
    if (bTravelStarted || !IsPassageUnlocked() || (bUseSlidingDoors && !CanUseLiftInterior(Pawn))) return false;
    bTravelStarted = Super::TravelToTargetLevel(Pawn);
    return bTravelStarted;
}
