#include "UI/Terra/WaveDefensePanelGPT61Terra.h"
#include "WaveDefense/WaveDefenseDirector.h"
#include "Player/TunaSweeperPlayerController.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Subsystem/TunaSweeperTextSubsystem.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Blueprint/WidgetTree.h"
void UWaveDefensePanelGPT61Terra::SetDirector(AWaveDefenseDirector* InDirector) { Director = InDirector; }
FText UWaveDefensePanelGPT61Terra::Text(FName Key) const
{
    const auto* Game = GetGameInstance<UTunaSweeperGameInstance>();
    auto* Strings = Game ? Game->GetSubsystem<UTunaSweeperTextSubsystem>() : nullptr;
    FText Value;
    return Strings && Strings->TryGetTextByKey(Key, Game->GetCurrentTextLanguage(), Value) ? Value : FText::GetEmpty();
}
void UWaveDefensePanelGPT61Terra::SetButtonText(UButton* Button, FName Key)
{
    if (Button && Button->GetChildrenCount() > 0) if (auto* Label = Cast<UTextBlock>(Button->GetChildAt(0))) Label->SetText(Text(Key));
}
void UWaveDefensePanelGPT61Terra::NativeConstruct()
{
    Super::NativeConstruct();
    TitleText->SetText(Text(TEXT("ui.wave_defense.title"))); IntroText->SetText(Text(TEXT("ui.wave_defense.intro")));
#define BIND(Button, Handler, Key) Button->OnClicked.AddDynamic(this, &UWaveDefensePanelGPT61Terra::Handler); SetButtonText(Button, TEXT(Key))
    BIND(StartButton, Start, "ui.wave_defense.start"); BIND(ReadyButton, Ready, "ui.wave_defense.ready");
    BIND(AbortButton, Abort, "ui.wave_defense.abort"); BIND(ClaimButton, Claim, "ui.wave_defense.claim"); BIND(CloseButton, Close, "ui.wave_defense.close");
    BIND(BarrierButton, Barrier, "ui.wave_defense.barrier"); BIND(TurretButton, Turret, "ui.wave_defense.turret");
    BIND(RepairCoreButton, RepairCore, "ui.wave_defense.repair_core"); BIND(RepairFacilityButton, RepairFacility, "ui.wave_defense.repair_facility"); BIND(ReloadButton, Reload, "ui.wave_defense.reload");
    BIND(NorthBarrierButton, NorthBarrier, "ui.wave_defense.north_barrier"); BIND(WestBarrierButton, WestBarrier, "ui.wave_defense.west_barrier"); BIND(EastBarrierButton, EastBarrier, "ui.wave_defense.east_barrier");
    BIND(NorthTurretButton, NorthTurret, "ui.wave_defense.north_turret"); BIND(WestTurretButton, WestTurret, "ui.wave_defense.west_turret"); BIND(EastTurretButton, EastTurret, "ui.wave_defense.east_turret");
#undef BIND
}
void UWaveDefensePanelGPT61Terra::NativeTick(const FGeometry& Geometry, float DeltaTime)
{
    Super::NativeTick(Geometry, DeltaTime);
    if (!Director.IsValid()) return;
    const auto S = Director->GetSnapshot();
    StatusText->SetText(FText::Format(Text(TEXT("ui.wave_defense.status")), FText::AsNumber(S.Wave), FText::AsNumber(S.TotalWaves), FText::AsNumber(S.CoreHealth), FText::AsNumber(S.CoreMaxHealth), FText::AsNumber(S.Alive), FText::AsNumber(S.Queued), FText::AsNumber(S.Supplies)));
    DetailText->SetText(FText::Format(Text(TEXT("ui.wave_defense.details")), FText::AsNumber(Director->GetDifficultyStage()), FText::AsNumber(Director->GetRewardCoins()), FText::AsNumber(FMath::CeilToInt(S.SecondsRemaining)), FText::AsNumber(FMath::CeilToInt(S.LeaveSecondsRemaining))));
}
void UWaveDefensePanelGPT61Terra::RunAction(EWaveDefenseAction Action, FName SlotKey) { if (!Director.IsValid() || !Director->TryFacilityAction(SlotKey, Action)) MessageText->SetText(Text(TEXT("ui.wave_defense.action_failed"))); else MessageText->SetText(FText::GetEmpty()); }
void UWaveDefensePanelGPT61Terra::Start() { if (!Director.IsValid() || !Director->TryStart(GetOwningPlayerPawn())) MessageText->SetText(Text(TEXT("ui.wave_defense.action_failed"))); }
void UWaveDefensePanelGPT61Terra::Ready() { if (!Director.IsValid() || !Director->SetReady()) MessageText->SetText(Text(TEXT("ui.wave_defense.action_failed"))); }
void UWaveDefensePanelGPT61Terra::Abort() { if (Director.IsValid()) Director->Abort(EWaveDefenseOutcome::Abandoned); }
void UWaveDefensePanelGPT61Terra::Claim() { if (!Director.IsValid() || !Director->TryClaimReward()) MessageText->SetText(Text(TEXT("ui.wave_defense.action_failed"))); }
void UWaveDefensePanelGPT61Terra::Close() { if (auto* Controller = Cast<ATunaSweeperPlayerController>(GetOwningPlayer())) Controller->CloseWaveDefensePanel(); }
void UWaveDefensePanelGPT61Terra::Barrier() { RunAction(EWaveDefenseAction::InstallBarrier, SelectedSlot); }
void UWaveDefensePanelGPT61Terra::Turret() { RunAction(EWaveDefenseAction::InstallTurret, SelectedSlot); }
void UWaveDefensePanelGPT61Terra::RepairCore() { RunAction(EWaveDefenseAction::RepairCore, TEXT("core")); }
void UWaveDefensePanelGPT61Terra::RepairFacility() { RunAction(EWaveDefenseAction::RepairFacility, SelectedSlot); }
void UWaveDefensePanelGPT61Terra::Reload() { RunAction(EWaveDefenseAction::ReloadTurret, SelectedSlot); }
void UWaveDefensePanelGPT61Terra::NorthBarrier() { SelectedSlot = TEXT("north.barrier"); }
void UWaveDefensePanelGPT61Terra::WestBarrier() { SelectedSlot = TEXT("west.barrier"); }
void UWaveDefensePanelGPT61Terra::EastBarrier() { SelectedSlot = TEXT("east.barrier"); }
void UWaveDefensePanelGPT61Terra::NorthTurret() { SelectedSlot = TEXT("north.turret"); }
void UWaveDefensePanelGPT61Terra::WestTurret() { SelectedSlot = TEXT("west.turret"); }
void UWaveDefensePanelGPT61Terra::EastTurret() { SelectedSlot = TEXT("east.turret"); }
