#include "Interaction/TunaSweeperRaidLiftActor.h"
#include "UI/TunaSweeperRaidLiftWidget.h"
#include "Character/TunaSweeperTopDownCharacter.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "GameFramework/Pawn.h"
#include "Player/TunaSweeperPlayerController.h"
#include "UObject/ConstructorHelpers.h"

ATunaSweeperRaidLiftActor::ATunaSweeperRaidLiftActor()
{
    RequiredProgressId = TEXT("world.passage.raid_lift");
    TravelStringKey = TEXT("ui.lift.choose");
    FTunaSweeperItemStack Part;
    Part.ItemId = 6025;
    Part.Quantity = 1;
    RepairParts.Add(Part);
    Part.ItemId = 6026;
    Part.Quantity = 2;
    RepairParts.Add(Part);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    VisualMesh->SetStaticMesh(Cube.Object);
    VisualMesh->SetRelativeLocation(FVector(0, 0, 10));
    VisualMesh->SetRelativeScale3D(FVector(2.4, 2.4, 0.2));
    VisualMesh->SetCollisionProfileName(TEXT("BlockAll"));
    LeftRail = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftRail"));
    RightRail = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightRail"));
    for (UStaticMeshComponent* Rail : {LeftRail.Get(), RightRail.Get()})
    {
        Rail->SetupAttachment(SceneRoot);
        Rail->SetStaticMesh(Cube.Object);
        Rail->SetRelativeScale3D(FVector(0.16, 0.16, 5));
        Rail->SetCollisionProfileName(TEXT("BlockAll"));
    }
    LeftRail->SetRelativeLocation(FVector(-110, -110, 250));
    RightRail->SetRelativeLocation(FVector(-110, 110, 250));
    for (const TCHAR* Name : {TEXT("Forest"), TEXT("VillageEast"), TEXT("VillageWest"), TEXT("Plains")})
    {
        DestinationImages.Add(TSoftObjectPtr<UTexture2D>(FSoftObjectPath(FString::Printf(TEXT("/Game/UI/RaidLift/T_Lift_%s.T_Lift_%s"), Name, Name))));
    }
}
bool ATunaSweeperRaidLiftActor::IsPawnOnPlatform(const APawn* Pawn) const
{
    if (!IsValid(Pawn)) return false;
    if (const auto* Character = Cast<ATunaSweeperTopDownCharacter>(Pawn); Character && Character->IsDead()) return false;
    if (const auto* PC = Cast<ATunaSweeperPlayerController>(Pawn->GetController()); PC &&
        (PC->IsDialogueSequenceActive() || PC->IsPauseMenuOpen() || PC->IsInventoryUiOpen() || PC->IsHousingModeOpen())) return false;
    const FVector Local = GetActorTransform().InverseTransformPosition(Pawn->GetActorLocation());
    return FMath::Abs(Local.X) <= 115 && FMath::Abs(Local.Y) <= 115 && Local.Z >= 20 && Local.Z <= 180;
}
UTexture2D* ATunaSweeperRaidLiftActor::GetDestinationImage(int32 Index) const
{
    return DestinationImages.IsValidIndex(Index) ? DestinationImages[Index].LoadSynchronous() : nullptr;
}
bool ATunaSweeperRaidLiftActor::TravelToTargetLevel(APawn* Pawn)
{
    auto* PC = Pawn ? Cast<ATunaSweeperPlayerController>(Pawn->GetController()) : nullptr;
    if (!PC || !PC->IsLocalController() || !IsPassageUnlocked() || !IsPawnOnPlatform(Pawn)
        || Picker || PC->IsPauseMenuOpen() || PC->IsInventoryUiOpen() || PC->IsHousingModeOpen()
        || PC->IsDialogueSequenceActive() || PC->IsMoveInputIgnored() || PC->IsLookInputIgnored()) return false;
    Picker = CreateWidget<UTunaSweeperRaidLiftWidget>(PC, UTunaSweeperRaidLiftWidget::StaticClass());
    if (!Picker) return false;
    if (auto* Character = Cast<ATunaSweeperTopDownCharacter>(Pawn)) Character->CancelActiveGameplayActions();
    Passenger = Pawn;
    Picker->InitializeLift(this);
    Picker->AddToViewport(100);
    PC->SetIgnoreMoveInput(true);
    PC->SetIgnoreLookInput(true);
    FInputModeUIOnly Input;
    Input.SetWidgetToFocus(Picker->TakeWidget());
    Input.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    PC->SetInputMode(Input);
    PC->bShowMouseCursor = true;
    Picker->SetKeyboardFocus();
    return true;
}
bool ATunaSweeperRaidLiftActor::SelectArrival(ETunaSweeperRaidArrivalGroup Group)
{
    if (!Picker || !IsPassageUnlocked() || !IsPawnOnPlatform(Passenger.Get())
        || Passenger->GetController() != Picker->GetOwningPlayer()) return false;
    switch (Group)
    {
    case ETunaSweeperRaidArrivalGroup::Forest: SetDestination(ETunaSweeperLevelTravelDestination::Forest); break;
    case ETunaSweeperRaidArrivalGroup::VillageEast:
    case ETunaSweeperRaidArrivalGroup::VillageWest: SetDestination(ETunaSweeperLevelTravelDestination::Village); break;
    case ETunaSweeperRaidArrivalGroup::Plains: SetDestination(ETunaSweeperLevelTravelDestination::Plains); break;
    default: return false;
    }
    SelectedArrival = Group;
    APawn* Pawn = Passenger.Get();
    ClosePicker();
    return Super::TravelToTargetLevel(Pawn);
}
void ATunaSweeperRaidLiftActor::ClosePicker()
{
    if (!Picker) return;
    auto* PC = Cast<ATunaSweeperPlayerController>(Picker->GetOwningPlayer());
    const auto* Character = Cast<ATunaSweeperTopDownCharacter>(Passenger.Get());
    const bool bCanRestoreInput = PC && Passenger.IsValid() && PC->GetPawn() == Passenger.Get()
        && (!Character || !Character->IsDead()) && !PC->IsDialogueSequenceActive();
    Picker->RemoveFromParent();
    Picker = nullptr;
    Passenger.Reset();
    if (PC)
    {
        // Balance only this picker’s input locks; do not reset locks owned by other systems.
        PC->SetIgnoreMoveInput(false);
        PC->SetIgnoreLookInput(false);
        if (!bCanRestoreInput) return;
        if (PC->IsPauseMenuOpen()) PC->ApplyDefaultGameInputMode();
        else
        {
            FInputModeGameAndUI Input;
            Input.SetHideCursorDuringCapture(false);
            Input.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
            PC->SetInputMode(Input);
            PC->bShowMouseCursor = true;
        }
    }
}
void ATunaSweeperRaidLiftActor::EndPlay(const EEndPlayReason::Type Reason)
{
    ClosePicker();
    Super::EndPlay(Reason);
}
