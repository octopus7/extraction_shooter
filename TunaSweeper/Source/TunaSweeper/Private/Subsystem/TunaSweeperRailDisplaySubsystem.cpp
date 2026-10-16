#include "Subsystem/TunaSweeperRailDisplaySubsystem.h"
#include "Subsystem/TunaSweeperTextSubsystem.h"
#include "Game/TunaSweeperGameInstance.h"
#include "LoopRailStation.h"
#include "Interaction/TunaSweeperRailInteractionComponent.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "EngineUtils.h"

void UTunaSweeperRailDisplaySubsystem::OnWorldBeginPlay(UWorld& World)
{
    Super::OnWorldBeginPlay(World);
    if (World.GetNetMode() == NM_DedicatedServer) return;
    SpawnHandle = World.AddOnActorSpawnedHandler(FOnActorSpawned::FDelegate::CreateUObject(this, &ThisClass::ConfigureActor));
    for (TActorIterator<ALoopRailStation> It(&World); It; ++It) ConfigureActor(*It);
}

void UTunaSweeperRailDisplaySubsystem::ConfigureActor(AActor* Actor)
{
    auto* Station = Cast<ALoopRailStation>(Actor);
    UGameInstance* Game = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
    auto* Text = Game ? Game->GetSubsystem<UTunaSweeperTextSubsystem>() : nullptr;
    if (!Station) return;
    if (!Station->StationId.IsNone() && !Station->FindComponentByClass<UTunaSweeperRailInteractionComponent>())
    {
        for (bool bTravel : {false, true})
        {
            auto* Interaction = NewObject<UTunaSweeperRailInteractionComponent>(Station);
            Interaction->bTravelInteraction = bTravel;
            Station->AddInstanceComponent(Interaction);
            Interaction->SetupAttachment(Station->ArrivalMarkerPivot);
            Interaction->SetRelativeLocation(FVector(0, 0, 110));
            Interaction->RegisterComponent();
            Interaction->RefreshState();
        }
    }
    if (!Text) return;
    FText Format, Unavailable;
    const auto* TunaGame = Cast<UTunaSweeperGameInstance>(Game);
    const auto Language = TunaGame ? TunaGame->GetCurrentTextLanguage() : ETunaSweeperItemTextLanguage::English;
    // Numeric m:ss notation is deliberately identical in all supported languages.
    if (Text->TryGetTextByKey(TEXT("ui.rail.arrival_time_format"), Language, Format)
        && Text->TryGetTextByKey(TEXT("ui.rail.arrival_unavailable"), Language, Unavailable))
        Station->SetArrivalTextFormats(Format, Unavailable);
}

void UTunaSweeperRailDisplaySubsystem::Deinitialize()
{
    if (GetWorld() && SpawnHandle.IsValid()) GetWorld()->RemoveOnActorSpawnedHandler(SpawnHandle);
    Super::Deinitialize();
}
