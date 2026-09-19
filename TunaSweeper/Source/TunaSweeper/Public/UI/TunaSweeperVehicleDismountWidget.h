#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TunaSweeperVehicleDismountWidget.generated.h"
class ATunaSweeperATVActor;
class UProgressBar;
class UBorder;
class UTextBlock;
UCLASS()
class TUNASWEEPER_API UTunaSweeperVehicleDismountWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	void SetVehicle(ATunaSweeperATVActor* InVehicle);
	void SetDismountHintVisible(bool bVisible);
protected:
	virtual void NativeConstruct() override;
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
	TWeakObjectPtr<ATunaSweeperATVActor> Vehicle;
	bool bDismountHintVisible = false;
};
