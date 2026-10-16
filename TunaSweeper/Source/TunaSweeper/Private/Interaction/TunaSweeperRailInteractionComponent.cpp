#include "Interaction/TunaSweeperRailInteractionComponent.h"

#include "Camera/PlayerCameraManager.h"
#include "Character/TunaSweeperTopDownCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Game/TunaSweeperGameInstance.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "LoopRailMotion.h"
#include "LoopRailStation.h"
#include "LoopRailTrack.h"
#include "LoopRailTrain.h"
#include "Subsystem/TunaSweeperItemDataSubsystem.h"
#include "Subsystem/TunaSweeperSpeechBubbleSubsystem.h"
#include "UI/TunaSweeperScreenFadeWidget.h"

UTunaSweeperRailInteractionComponent::UTunaSweeperRailInteractionComponent()
{
    ConfigureInteractionDefaults(ETunaSweeperInteractionType::RailStation, FText::GetEmpty(),
        TSoftClassPtr<UTunaSweeperInteractionMarkerWidget>(FSoftObjectPath(TEXT("/Game/UI/WBP_InteractionMarker.WBP_InteractionMarker_C"))),
        TEXT("ui.rail.lubricate"));
}

FName UTunaSweeperRailInteractionComponent::ProgressId(const ALoopRailStation* Station)
{
    return IsValid(Station) && !Station->StationId.IsNone()
        ? FName(*(TEXT("rail.lubrication.") + Station->StationId.ToString())) : NAME_None;
}

void UTunaSweeperRailInteractionComponent::BeginPlay()
{
    Super::BeginPlay();
    if (auto* Game = Cast<UTunaSweeperGameInstance>(GetWorld()->GetGameInstance()))
    {
        Game->GetInventorySlots();
        Game->OnWorldProgressChanged.AddUObject(this, &ThisClass::RefreshState);
        Game->OnInventoryStateChanged.AddUObject(this, &ThisClass::RefreshState);
        Game->OnLanguageChanged.AddUObject(this, &ThisClass::RefreshState);
    }
    RefreshState();
}

void UTunaSweeperRailInteractionComponent::EndPlay(const EEndPlayReason::Type Reason)
{
    FinishTravel();
    if (auto* Game = GetWorld() ? Cast<UTunaSweeperGameInstance>(GetWorld()->GetGameInstance()) : nullptr)
    {
        Game->OnWorldProgressChanged.RemoveAll(this);
        Game->OnInventoryStateChanged.RemoveAll(this);
        Game->OnLanguageChanged.RemoveAll(this);
    }
    Super::EndPlay(Reason);
}

bool UTunaSweeperRailInteractionComponent::IsLubricated() const
{
    const auto* Game = GetWorld() ? Cast<UTunaSweeperGameInstance>(GetWorld()->GetGameInstance()) : nullptr;
    FTunaSweeperWorldProgressSaveData State;
    return Game && Game->TryGetWorldProgressState(ProgressId(Cast<ALoopRailStation>(GetOwner())), State) &&
        State.State == ETunaSweeperWorldProgressState::Completed;
}

bool UTunaSweeperRailInteractionComponent::IsTravelUnlocked() const
{
    const auto* Source = Cast<ALoopRailStation>(GetOwner());
    const auto* Game = GetWorld() ? Cast<UTunaSweeperGameInstance>(GetWorld()->GetGameInstance()) : nullptr;
    if (!Source || !IsValid(Source->Track) || !Game) return false;
    for (TActorIterator<ALoopRailStation> It(GetWorld()); It; ++It)
    {
        FTunaSweeperWorldProgressSaveData State;
        if (It->Track == Source->Track && Game->TryGetWorldProgressState(ProgressId(*It), State) &&
            State.State == ETunaSweeperWorldProgressState::Completed) return true;
    }
    return false;
}

ALoopRailStation* UTunaSweeperRailInteractionComponent::GetNextStation() const
{
    const auto* Source = Cast<ALoopRailStation>(GetOwner());
    if (!Source || !IsValid(Source->Track) || !Source->Track->IsUsableLoop()) return nullptr;
    ALoopRailStation* Next = nullptr;
    double Best = TNumericLimits<double>::Max();
    for (TActorIterator<ALoopRailStation> It(GetWorld()); It; ++It)
    {
        // Resolve the physical next station first. A disabled next stop must not silently skip a station.
        if (*It == Source || It->Track != Source->Track || It->IsActorBeingDestroyed()) continue;
        const double Distance = LoopRailMotion::Forward(Source->GetStopDistance(), It->GetStopDistance(), Source->Track->GetLength());
        if (Distance > 1.0 && Distance < Best) { Best = Distance; Next = *It; }
    }
    return Next;
}

bool UTunaSweeperRailInteractionComponent::CanOffer() const
{
    const auto* Source = Cast<ALoopRailStation>(GetOwner());
    if (!Source || !Source->bEnabled || !Source->bShowPlatform || !Source->bShowArrivalDisplay ||
        ProgressId(Source).IsNone() || bTravelling) return false;
    if (!bTravelInteraction) return !IsLubricated();
    const auto* Next = GetNextStation();
    return IsTravelUnlocked() && Next && Next->bEnabled && Next->bShowPlatform;
}

void UTunaSweeperRailInteractionComponent::RefreshState()
{
    const FName Key = bTravelInteraction ? TEXT("ui.rail.travel_next") : TEXT("ui.rail.lubricate");
    SetInteractionTypeDisplayNameAndStringKey(ETunaSweeperInteractionType::RailStation, FText::GetEmpty(), Key);
    SetInteractionOrder(bTravelInteraction ? 0 : 1);
    UTexture2D* Icon = nullptr;
    auto* Game = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
    if (!bTravelInteraction)
    {
        FTunaSweeperItemDefinition Item;
        if (auto* Items = Game ? Game->GetSubsystem<UTunaSweeperItemDataSubsystem>() : nullptr;
            Items && Items->TryGetItemDefinition(OilItemId, Item))
            Icon = LoadObject<UTexture2D>(nullptr, *Items->BuildItemIconObjectPath(Item));
    }
    SetInteractionRequirementPreview(Icon, 1, !bTravelInteraction && !IsLubricated());
}

bool UTunaSweeperRailInteractionComponent::Lubricate(bool bSaveImmediately)
{
    auto* Game = GetWorld() ? Cast<UTunaSweeperGameInstance>(GetWorld()->GetGameInstance()) : nullptr;
    if (!Game || bTravelInteraction || bLubricating || !CanOffer() || OilItemId <= 0) return false;
    TGuardValue<bool> Guard(bLubricating, true);
    // Existing transaction rolls payment and world completion back together if saving fails.
    return Game->CommitPowerFacilityRepair(ProgressId(Cast<ALoopRailStation>(GetOwner())), {{OilItemId, 1}}, true, bSaveImmediately);
}

bool UTunaSweeperRailInteractionComponent::Interact(APawn* Pawn)
{
    const auto* Character = Cast<ATunaSweeperTopDownCharacter>(Pawn);
    const auto* Controller = Character ? Cast<APlayerController>(Character->GetController()) : nullptr;
    if (!Character || Character->IsDead() || Character->IsMountedInVehicle() || !Controller ||
        Controller->IsMoveInputIgnored() || !IsWithinInteractionDistance(Pawn) || !CanOffer()) return false;
    if (bTravelInteraction) return StartTravel(Pawn);
    auto* Game = GetWorld()->GetGameInstance<UTunaSweeperGameInstance>();
    if (Game && Game->CountInventoryItemById(OilItemId) < 1)
    {
        if (auto* Speech = Game->GetSubsystem<UTunaSweeperSpeechBubbleSubsystem>())
            Speech->ShowForActor(Game->ResolveLocalizedText(TEXT("ui.rail.need_oil"), FText::GetEmpty()),
                Pawn, FVector(0, 0, 110), ETunaSweeperSpeechBubbleTailDirection::Down, 2.f);
        return false;
    }
    return Lubricate();
}

bool UTunaSweeperRailInteractionComponent::ResolveTravel(APawn* Pawn, ALoopRailStation*& Station,
    ALoopRailTrain*& Train, FVector& Location) const
{
    Station = nullptr; Train = nullptr;
    auto* Character = Cast<ATunaSweeperTopDownCharacter>(Pawn);
    const auto* Source = Cast<ALoopRailStation>(GetOwner());
    if (!bTravelInteraction || !IsTravelUnlocked() || !Source || !Source->bEnabled ||
        !Source->bShowPlatform || !Source->bShowArrivalDisplay || !Character || Character->GetWorld() != GetWorld() ||
        Character->IsDead() || Character->IsMountedInVehicle() || !IsWithinInteractionDistance(Pawn)) return false;
    Station = GetNextStation();
    if (!Station || !Station->bEnabled || !Station->bShowPlatform || !Station->ArrivalMarkerPivot ||
        !FMath::IsFinite(Station->DwellSeconds)) return false;
    for (TActorIterator<ALoopRailTrain> It(GetWorld()); It; ++It)
        if (It->Track == Source->Track && It->HasAuthority() && It->HasUsableTrack() && !It->IsActorBeingDestroyed())
        { Train = *It; break; }
    if (!Train) return false;
    const auto* Capsule = Character->GetCapsuleComponent();
    const auto* Movement = Character->GetCharacterMovement();
    if (!Capsule || !Movement || !Capsule->IsQueryCollisionEnabled()) return false;
    float Radius, HalfHeight;
    Capsule->GetScaledCapsuleSize(Radius, HalfHeight);
    // Land beside the monument on the platform, clear of its rotating body and train sweep.
    const FVector Floor = Station->ArrivalMarkerPivot->GetComponentLocation() - Station->GetActorForwardVector() * 180.f;
    FCollisionQueryParams Query(SCENE_QUERY_STAT(RailArrival), false, Character);
    const FCollisionResponseParams Responses(Capsule->GetCollisionResponseToChannels());
    FHitResult Ground;
    if (!GetWorld()->LineTraceSingleByChannel(Ground, Floor + FVector(0, 0, 20), Floor - FVector(0, 0, 50),
        Capsule->GetCollisionObjectType(), Query, Responses) || !Movement->IsWalkable(Ground)) return false;
    Location = Ground.ImpactPoint + FVector(0, 0, HalfHeight + 3.f);
    return !Location.ContainsNaN() && !GetWorld()->OverlapBlockingTestByChannel(Location, FQuat::Identity,
        Capsule->GetCollisionObjectType(), FCollisionShape::MakeCapsule(Radius, HalfHeight), Query, Responses);
}

bool UTunaSweeperRailInteractionComponent::StartTravel(APawn* Pawn)
{
    ALoopRailStation* Station; ALoopRailTrain* Train; FVector Location;
    auto* Controller = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
    if (bTravelling || !Controller || Controller->IsMoveInputIgnored() || !Controller->IsLocalController() ||
        !Controller->GetLocalPlayer() || !ResolveTravel(Pawn, Station, Train, Location)) return false;
    FadeWidget = CreateWidget<UTunaSweeperScreenFadeWidget>(Controller, UTunaSweeperScreenFadeWidget::StaticClass());
    if (!FadeWidget) return false;
    PendingPawn = Pawn; PendingController = Controller; bTravelling = true;
    auto* Character = CastChecked<ATunaSweeperTopDownCharacter>(Pawn);
    Character->CancelActiveGameplayActions();
    Character->GetCharacterMovement()->StopMovementImmediately();
    Pawn->ConsumeMovementInputVector();
    Controller->StopMovement();
    Controller->SetIgnoreMoveInput(true);
    Controller->SetIgnoreLookInput(true);
    FadeWidget->AddToViewport(10000);
    FadeWidget->StartFadeToBlack(.2f, FSimpleDelegate::CreateUObject(this, &ThisClass::CompleteFadeOut));
    return true;
}

bool UTunaSweeperRailInteractionComponent::ExecuteTravel(APawn* Pawn)
{
    ALoopRailStation* Station; ALoopRailTrain* Train; FVector Location;
    if (!ResolveTravel(Pawn, Station, Train, Location)) return false;
    const FVector OldLocation = Pawn->GetActorLocation();
    const FRotator OldRotation = Pawn->GetActorRotation();
    if (!Pawn->TeleportTo(Location, FRotator(0, Station->GetActorRotation().Yaw, 0), false, false)) return false;
    if (!Train->ArriveAtStation(Station, 1.f))
    {
        Pawn->TeleportTo(OldLocation, OldRotation, false, true);
        return false;
    }
    auto* Character = CastChecked<ATunaSweeperTopDownCharacter>(Pawn);
    Character->SetBase(nullptr);
    Character->GetCharacterMovement()->StopMovementImmediately();
    Character->ConsumeMovementInputVector();
    return true;
}

void UTunaSweeperRailInteractionComponent::CompleteFadeOut()
{
    if (PendingController.IsValid() && PendingController->GetPawn() == PendingPawn.Get() && ExecuteTravel(PendingPawn.Get()))
        if (PendingController->PlayerCameraManager) PendingController->PlayerCameraManager->SetGameCameraCutThisFrame();
    if (FadeWidget) FadeWidget->StartFadeFromBlack(.2f, FSimpleDelegate::CreateUObject(this, &ThisClass::FinishTravel));
    else FinishTravel();
}

void UTunaSweeperRailInteractionComponent::FinishTravel()
{
    if (bTravelling && PendingController.IsValid())
    {
        PendingController->SetIgnoreMoveInput(false);
        PendingController->SetIgnoreLookInput(false);
    }
    if (FadeWidget) FadeWidget->RemoveFromParent();
    FadeWidget = nullptr;
    PendingPawn.Reset(); PendingController.Reset(); bTravelling = false;
}
