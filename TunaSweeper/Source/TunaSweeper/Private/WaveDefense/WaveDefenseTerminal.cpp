#include "WaveDefense/WaveDefenseTerminal.h"
#include "WaveDefense/WaveDefenseDirector.h"
#include "Player/TunaSweeperPlayerController.h"
#include "GameFramework/Pawn.h"
AWaveDefenseTerminal::AWaveDefenseTerminal()
{
    ConfigureInteractionDefaults(ETunaSweeperInteractionType::WaveDefense, FText::GetEmpty(), TSoftClassPtr<UTunaSweeperInteractionMarkerWidget>(FSoftObjectPath(TEXT("/Game/UI/WBP_InteractionMarker.WBP_InteractionMarker_C"))), TEXT("ui.wave_defense.title"));
}
bool AWaveDefenseTerminal::OpenFor(APawn* Player)
{
    auto* Controller = Player ? Cast<ATunaSweeperPlayerController>(Player->GetController()) : nullptr;
    return Controller && Director && IsWithinInteractionDistance(Player) && Controller->OpenWaveDefensePanel(Director);
}
void AWaveDefenseTerminal::SetDirector(AWaveDefenseDirector* InDirector) { Director = InDirector; }
