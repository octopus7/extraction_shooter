#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TunaSweeperVehicleDismountWidget.generated.h"
class ATunaSweeperATVActor;
class UProgressBar;
class UBorder;
class UTextBlock;
class UTunaSweeperVehicleFuelWidget;
UCLASS()
class TUNASWEEPER_API UTunaSweeperVehicleDismountWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	void SetVehicle(ATunaSweeperATVActor* InVehicle);
	void SetDismountHintVisible(bool bVisible);
	virtual void SetVisibility(ESlateVisibility InVisibility) override;
protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UProgressBar> DurabilityBar;
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UBorder> DismountPanel;
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> DismountText;
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> DismountKeyText;
private:
	void RefreshState();
	void RefreshFuelGauge();
	UPROPERTY(Transient)
	TObjectPtr<UTunaSweeperVehicleFuelWidget> FuelGauge;
	TWeakObjectPtr<ATunaSweeperATVActor> Vehicle;
	bool bDismountHintVisible = false;
};
