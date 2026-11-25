#include "Interaction/TunaSweeperRaidLiftActor.h"
#include "UI/TunaSweeperRaidLiftWidget.h"
#include "Engine/GameInstance.h"
#include "Subsystem/TunaSweeperAchievementSubsystem.h"
#include "Character/TunaSweeperTopDownCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/Image.h"
#include "Blueprint/GameViewportSubsystem.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Vehicle/TunaSweeperATVRiderAnimInstance.h"
#include "Brushes/SlateColorBrush.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "GameFramework/Pawn.h"
#include "Materials/MaterialInterface.h"
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
void ATunaSweeperRaidLiftActor::ApplyPassagePresentation(bool bUnlocked)
{
    if (!ReadyRailStatusMaterial || !LeftRail || !LeftRail->GetStaticMesh()) return;

    static const FName StatusLensSlot(TEXT("StatusLens"));
    const int32 Index = LeftRail->GetMaterialIndex(StatusLensSlot);
    if (Index != INDEX_NONE)
    {
        LeftRail->SetMaterial(Index, bUnlocked ? ReadyRailStatusMaterial.Get() : LeftRail->GetStaticMesh()->GetMaterial(Index));
    }
}

bool ATunaSweeperRaidLiftActor::IsPawnOnPlatform(const APawn* Pawn) const
{
    if (!IsValid(Pawn)) return false;
    if (const auto* Character = Cast<ATunaSweeperTopDownCharacter>(Pawn); Character && Character->IsDead()) return false;
    if (const auto* PC = Cast<ATunaSweeperPlayerController>(Pawn->GetController()); PC &&
        (PC->IsDialogueSequenceActive() || PC->IsPauseMenuOpen() || PC->IsInventoryUiOpen() || PC->IsHousingModeOpen())) return false;

    if (bUseChairBoardingArea)
    {
        const FVector Local = TravelInterior->GetComponentTransform().InverseTransformPosition(Pawn->GetActorLocation());
        const FVector Extent = TravelInterior->GetUnscaledBoxExtent();
        return FMath::Abs(Local.X) <= Extent.X && FMath::Abs(Local.Y) <= Extent.Y && FMath::Abs(Local.Z) <= Extent.Z;
    }
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
        || Picker || RideFade || PC->IsPauseMenuOpen() || PC->IsInventoryUiOpen() || PC->IsHousingModeOpen()
        || PC->IsDialogueSequenceActive() || PC->IsMoveInputIgnored() || PC->IsLookInputIgnored()) return false;
    Picker = CreateWidget<UTunaSweeperRaidLiftWidget>(PC, UTunaSweeperRaidLiftWidget::StaticClass());
    if (!Picker) return false;
    if (auto* Character = Cast<ATunaSweeperTopDownCharacter>(Pawn)) Character->CancelActiveGameplayActions();
    Passenger = Pawn;
    PassengerController = PC;
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
bool ATunaSweeperRaidLiftActor::HasVisitedDestination(ETunaSweeperRaidArrivalGroup Group) const
{
    const auto* History = UGameInstance::GetSubsystem<UTunaSweeperAchievementSubsystem>(GetGameInstance());
    if (!History) return false;
    switch (Group)
    {
    case ETunaSweeperRaidArrivalGroup::Forest: return History->HasReachedLocation(TEXT("region.forest"));
    case ETunaSweeperRaidArrivalGroup::VillageWest:
    case ETunaSweeperRaidArrivalGroup::VillageEast: return History->HasReachedLocation(TEXT("region.village"));
    case ETunaSweeperRaidArrivalGroup::Plains: return History->HasReachedLocation(TEXT("region.plains"));
    default: return false;
    }
}

bool ATunaSweeperRaidLiftActor::SelectArrival(ETunaSweeperRaidArrivalGroup Group)
{
    if (!Picker || !HasVisitedDestination(Group) || !IsPassageUnlocked() || !IsPawnOnPlatform(Passenger.Get())
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
    if (bUseChairBoardingArea) return StartChairRide(Cast<ATunaSweeperTopDownCharacter>(Pawn));
    ClosePicker();
    return Super::TravelToTargetLevel(Pawn);
}

bool ATunaSweeperRaidLiftActor::StartChairRide(ATunaSweeperTopDownCharacter* Character)
{
    auto* Viewport = UGameViewportSubsystem::Get(GetWorld());
    if (!Character || Character->GetAttachParentActor() || !Viewport) return false;
    auto* Mesh = Character->GetMesh();
    auto* Movement = Character->GetCharacterMovement();
    if (!Mesh || !Movement || Mesh->GetAnimationMode() != EAnimationMode::AnimationBlueprint) return false;

    // Above gameplay HUDs and below the existing map transition presentation.
    FGameViewportWidgetSlot Slot;
    Slot.ZOrder = 900;
    UImage* Fade = NewObject<UImage>(this);
    Fade->SetBrush(FSlateColorBrush(FLinearColor::Black));
    Fade->SetRenderOpacity(0.f);
    Fade->SetVisibility(ESlateVisibility::HitTestInvisible);
    if (!Viewport->AddWidget(Fade, Slot)) return false;

    BoardingTransform = Character->GetActorTransform();
    VisualMesh->SetMobility(EComponentMobility::Movable);
    if (!Character->AttachToComponent(VisualMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale))
    {
        Viewport->RemoveWidget(Fade);
        return false;
    }
    RideFade = Fade;
    BoardingMovementMode = Movement->MovementMode;
    BoardingCustomMode = Movement->CustomMovementMode;
    StandingAnimClass = Mesh->GetAnimClass();
    bBoardingCollision = Character->GetActorEnableCollision();
    ParkedChairLocation = VisualMesh->GetRelativeLocation();
    RideElapsed = 0.f;
    Movement->StopMovementImmediately();
    Movement->DisableMovement();
    Character->SetActorEnableCollision(false);
    Character->SetActorRelativeLocation(FVector(28, 0, 60));
    Mesh->SetAnimInstanceClass(UTunaSweeperLiftChairAnimInstance::StaticClass());
    Picker->RemoveFromParent();
    Picker = nullptr;
    PassengerController->bShowMouseCursor = false;

    if (auto* Spark = RailContactSpark.LoadSynchronous())
    {
        const FTransform& Rail = LeftRail->GetComponentTransform();
        const FVector Direction = Rail.GetUnitAxis(EAxis::Z) + .18f * Rail.GetUnitAxis(EAxis::X);
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), Spark,
            Rail.TransformPosition(FVector(12, 11, 106)), Direction.Rotation(), FVector(.85f), true, true, ENCPoolMethod::None, false);
    }
    SetActorTickEnabled(true);
    return true;
}

void ATunaSweeperRaidLiftActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!RideFade) return;
    auto* Character = Cast<ATunaSweeperTopDownCharacter>(Passenger.Get());
    if (!Character || !PassengerController.IsValid() || PassengerController->GetPawn() != Character
        || Character->GetAttachParentActor() != this || Character->IsDead() || !IsPassageUnlocked())
    {
        FinishChairRide(true);
        return;
    }
    constexpr float ContactLead = .22f;
    constexpr float RiseDuration = .46f;
    constexpr float RiseHeight = 40.f;
    RideElapsed += DeltaSeconds;
    const float Alpha = FMath::Clamp((RideElapsed - ContactLead) / RiseDuration, 0.f, 1.f);
    VisualMesh->SetRelativeLocation(ParkedChairLocation + FVector::UpVector * (RiseHeight * FMath::SmoothStep(0.f, 1.f, Alpha)));
    RideFade->SetRenderOpacity(Alpha);
    if (RideElapsed >= ContactLead + RiseDuration + .06f)
    {
        SetActorTickEnabled(false);
        if (!Super::TravelToTargetLevel(Character)) FinishChairRide(true);
    }
}

void ATunaSweeperRaidLiftActor::FinishChairRide(bool bRestoreBoardingState)
{
    if (!RideFade) return;
    RideFade->RemoveFromParent();
    RideFade = nullptr;
    if (bRestoreBoardingState)
    {
        VisualMesh->SetRelativeLocation(ParkedChairLocation);
        if (auto* Character = Cast<ATunaSweeperTopDownCharacter>(Passenger.Get()))
        {
            if (Character->GetAttachParentActor() == this)
            {
                Character->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
                Character->SetActorTransform(BoardingTransform, false, nullptr, ETeleportType::TeleportPhysics);
            }
            Character->SetActorEnableCollision(bBoardingCollision);
            if (!Character->IsDead())
            {
                Character->GetMesh()->SetAnimInstanceClass(StandingAnimClass);
                Character->GetCharacterMovement()->SetMovementMode(EMovementMode(BoardingMovementMode), BoardingCustomMode);
            }
        }
        SetActorTickEnabled(bUseSlidingDoors);
    }
    StandingAnimClass = nullptr;
    ReleasePassenger(bRestoreBoardingState);
}

void ATunaSweeperRaidLiftActor::ClosePicker()
{
    if (!Picker) return;
    Picker->RemoveFromParent();
    Picker = nullptr;
    ReleasePassenger(true);
}

void ATunaSweeperRaidLiftActor::ReleasePassenger(bool bRestoreInput)
{
    auto* PC = PassengerController.Get();
    const auto* Character = Cast<ATunaSweeperTopDownCharacter>(Passenger.Get());
    const bool bCanRestoreInput = bRestoreInput && PC && Passenger.IsValid() && PC->GetPawn() == Passenger.Get()
        && (!Character || !Character->IsDead()) && !PC->IsDialogueSequenceActive();
    Passenger.Reset();
    PassengerController.Reset();
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
    FinishChairRide(Reason == EEndPlayReason::Destroyed || Reason == EEndPlayReason::RemovedFromWorld);
    ClosePicker();
    Super::EndPlay(Reason);
}
