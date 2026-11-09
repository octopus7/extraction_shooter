#include "Player/TunaSweeperPlayerController.h"
#include "WaveDefense/WaveDefenseDirector.h"
#include "UI/Terra/WaveDefensePanelGPT61Terra.h"
#include "UI/Terra/WaveDefenseHUDGPT61Terra.h"
#include "Blueprint/UserWidget.h"
bool ATunaSweeperPlayerController::OpenWaveDefensePanel(AWaveDefenseDirector* Director)
{
    if (!IsLocalController() || !IsValid(Director) || !GetPawn()) return false;
    if (WaveDefensePanel) CloseWaveDefensePanel();
    UClass* PanelClass = LoadClass<UWaveDefensePanelGPT61Terra>(nullptr, TEXT("/Game/MainRaid/WaveDefense/UI/WBP_WaveDefensePanel_GPT61Terra.WBP_WaveDefensePanel_GPT61Terra_C"));
    if (!PanelClass) return false;
    WaveDefensePanel = CreateWidget<UWaveDefensePanelGPT61Terra>(this, PanelClass);
    if (!WaveDefensePanel) return false;
    WaveDefensePanel->SetDirector(Director); WaveDefensePanel->AddToViewport(100);
    if (!WaveDefenseHUD)
    {
        UClass* HUDClass = LoadClass<UWaveDefenseHUDGPT61Terra>(nullptr, TEXT("/Game/MainRaid/WaveDefense/UI/WBP_WaveDefenseHUD_GPT61Terra.WBP_WaveDefenseHUD_GPT61Terra_C"));
        if (HUDClass) { WaveDefenseHUD = CreateWidget<UWaveDefenseHUDGPT61Terra>(this, HUDClass); if (WaveDefenseHUD) WaveDefenseHUD->AddToViewport(10); }
    }
    if (WaveDefenseHUD) WaveDefenseHUD->SetDirector(Director);
    FInputModeGameAndUI Mode; Mode.SetWidgetToFocus(WaveDefensePanel->TakeWidget()); SetInputMode(Mode); bShowMouseCursor = true;
    return true;
}
void ATunaSweeperPlayerController::CloseWaveDefensePanel()
{
    if (WaveDefensePanel) { WaveDefensePanel->RemoveFromParent(); WaveDefensePanel = nullptr; }
    ApplyDefaultGameInputMode();
}
