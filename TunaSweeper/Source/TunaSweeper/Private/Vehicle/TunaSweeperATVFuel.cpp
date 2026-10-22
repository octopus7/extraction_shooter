#include "Vehicle/TunaSweeperATVActor.h"
#include "Vehicle/TunaSweeperVehicleMountComponent.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Subsystem/TunaSweeperQuestSubsystem.h"
#include "Subsystem/TunaSweeperSpeechBubbleSubsystem.h"
#include "Character/TunaSweeperTopDownCharacter.h"
#include "Engine/Level.h"
#include "Engine/World.h"

void ATunaSweeperATVActor::InitializeSessionFuel()
{
    FuelSeconds = 0.f;
    FuelSessionKey = UWorld::RemovePIEPrefix(GetLevel()->GetOutermost()->GetName()) + TEXT(":") +
        (FuelSessionId.IsNone() ? GetName() : FuelSessionId.ToString());
    if (auto* Game = GetGameInstance<UTunaSweeperGameInstance>())
    {
        Game->GetInventorySlots(); // Save loading resets the session before any actor reads it.
        FuelSession = Game->GetVehicleFuelSession();
        FuelSeconds = Game->GetVehicleFuel(FuelSessionKey);
    }
}

void ATunaSweeperATVActor::StoreSessionFuel()
{
    if (auto* Game = GetGameInstance<UTunaSweeperGameInstance>())
        Game->StoreVehicleFuel(FuelSessionKey, FuelSeconds, FuelSession);
}

bool ATunaSweeperATVActor::Refuel(APawn* InstigatorPawn)
{
    auto* Game = GetGameInstance<UTunaSweeperGameInstance>();
    const auto* Character = Cast<ATunaSweeperTopDownCharacter>(InstigatorPawn);
    if (!Game || !IsValid(Character) || Character->IsDead() || Character->IsMountedInVehicle() ||
        bVehicleDestroyed || HasFuel() || bRefueling || MountComponent->GetRider() ||
        !MountComponent->IsWithinInteractionDistance(InstigatorPawn) ||
        FMath::Abs(Character->GetActorLocation().Z - MountComponent->GetComponentLocation().Z) > MountComponent->GetInteractionDistance()) return false;
    TGuardValue<bool> Guard(bRefueling, true);
    FTunaSweeperItemStack Fuel; Fuel.ItemId = 6009; Fuel.Quantity = 1;
    const bool bConsumed = Game->TryConsumeInventoryItems({Fuel}, [this, Game]()
    {
        FuelSeconds = 60.f;
        StoreSessionFuel();
        if (auto* Quests = Game->GetSubsystem<UTunaSweeperQuestSubsystem>(); Quests && !RefuelObjectiveEventId.IsNone())
            Quests->NotifyInteractionCompleted(RefuelObjectiveEventId, TEXT("world_progress"));
    });
    if (bConsumed) Game->SaveGameState();
    else if (auto* Speech = Game->GetSubsystem<UTunaSweeperSpeechBubbleSubsystem>())
        Speech->ShowForActor(Game->ResolveLocalizedText(TEXT("ui.vehicle.fuel_missing"), FText::GetEmpty()),
            InstigatorPawn, FVector(0,0,20), ETunaSweeperSpeechBubbleTailDirection::None, 2.5f, -28.f);
    RefreshFuelInteraction();
    return bConsumed;
}

void ATunaSweeperATVActor::ConsumeFuel(float DeltaSeconds, bool bEngineOn, bool bMoving)
{
    if (!bEngineOn || bVehicleDestroyed || !FMath::IsFinite(DeltaSeconds) || DeltaSeconds <= 0 || !HasFuel()) return;
    FuelSeconds = FMath::Max(0.f, FuelSeconds - DeltaSeconds * (bMoving ? 1.f : 0.1f));
    StoreSessionFuel();
    if (!HasFuel())
    {
        ClearDriveInput();
        MountComponent->StopEngineForFuel();
        RefreshFuelInteraction();
    }
}

void ATunaSweeperATVActor::RefreshFuelInteraction()
{
    const auto Type = bVehicleDestroyed || MountComponent->GetRider() ? ETunaSweeperInteractionType::None :
        (HasFuel() ? ETunaSweeperInteractionType::VehicleMount : ETunaSweeperInteractionType::WorldProgress);
    MountComponent->SetInteractionTypeDisplayNameAndStringKey(Type, FText::GetEmpty(),
        HasFuel() ? TEXT("ui.vehicle.mount") : TEXT("ui.vehicle.refuel"));
}
