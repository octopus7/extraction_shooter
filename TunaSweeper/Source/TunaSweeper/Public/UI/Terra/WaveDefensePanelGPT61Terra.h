#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "WaveDefense/WaveDefenseTypes.h"
#include "WaveDefensePanelGPT61Terra.generated.h"
class AWaveDefenseDirector;
class UTextBlock;
class UButton;
// Model: GPT-6.1 Terra; reasoning effort: high.
UCLASS(Abstract)
class TUNASWEEPER_API UWaveDefensePanelGPT61Terra : public UUserWidget
{
    GENERATED_BODY()
public:
    void SetDirector(AWaveDefenseDirector* InDirector);
protected:
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& Geometry, float DeltaTime) override;
    UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> TitleText;
    UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> IntroText;
    UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> StatusText;
    UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> DetailText;
    UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> MessageText;
    UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton> StartButton;
    UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton> ReadyButton;
    UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton> AbortButton;
    UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton> ClaimButton;
    UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton> CloseButton;
    UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton> BarrierButton;
    UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton> TurretButton;
    UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton> RepairCoreButton;
    UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton> RepairFacilityButton;
    UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton> ReloadButton;
    UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton> NorthBarrierButton;
    UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton> WestBarrierButton;
    UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton> EastBarrierButton;
    UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton> NorthTurretButton;
    UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton> WestTurretButton;
    UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton> EastTurretButton;
private:
    UFUNCTION() void Start();
    UFUNCTION() void Ready();
    UFUNCTION() void Abort();
    UFUNCTION() void Claim();
    UFUNCTION() void Close();
    UFUNCTION() void Barrier();
    UFUNCTION() void Turret();
    UFUNCTION() void RepairCore();
    UFUNCTION() void RepairFacility();
    UFUNCTION() void Reload();
    UFUNCTION() void NorthBarrier();
    UFUNCTION() void WestBarrier();
    UFUNCTION() void EastBarrier();
    UFUNCTION() void NorthTurret();
    UFUNCTION() void WestTurret();
    UFUNCTION() void EastTurret();
    void RunAction(EWaveDefenseAction Action, FName SlotKey);
    FText Text(FName Key) const;
    void SetButtonText(UButton* Button, FName Key);
    TWeakObjectPtr<AWaveDefenseDirector> Director;
    FName SelectedSlot = TEXT("north.barrier");
    FName SelectedSlotTextKey = TEXT("ui.wave_defense.north_barrier");
    float ActionErrorSeconds = 0.0f;
};
