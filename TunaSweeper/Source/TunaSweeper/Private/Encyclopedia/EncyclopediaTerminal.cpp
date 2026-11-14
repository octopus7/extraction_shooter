#include "Encyclopedia/EncyclopediaTerminal.h"
#include "GameFramework/Pawn.h"
#include "Player/TunaSweeperPlayerController.h"
#include "UI/Astra/EncyclopediaGPT6Astra.h"

AEncyclopediaTerminal::AEncyclopediaTerminal()
{
    ConfigureInteractionDefaults(ETunaSweeperInteractionType::Encyclopedia, FText::GetEmpty(),
        TSoftClassPtr<UTunaSweeperInteractionMarkerWidget>(FSoftObjectPath(
            TEXT("/Game/UI/WBP_InteractionMarker.WBP_InteractionMarker_C"))), TEXT("codex.title"));
    PanelWidgetClass = TSoftClassPtr<UEncyclopediaPanelGPT6Astra>(FSoftObjectPath(
        TEXT("/Game/UI/Encyclopedia/WBP_EncyclopediaPanel_GPT6Astra.WBP_EncyclopediaPanel_GPT6Astra_C")));
}

bool AEncyclopediaTerminal::OpenFor(APawn* Player)
{
    auto* Controller = Player ? Cast<ATunaSweeperPlayerController>(Player->GetController()) : nullptr;
    return Controller && IsWithinInteractionDistance(Player) && Controller->OpenEncyclopedia(this);
}

TSubclassOf<UEncyclopediaPanelGPT6Astra> AEncyclopediaTerminal::ResolvePanelWidgetClass() const
{
    return PanelWidgetClass.LoadSynchronous();
}
