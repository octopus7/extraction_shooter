#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TunaSweeperVehicleDismountWidget.generated.h"
class ATunaSweeperATVActor;
UCLASS()
class TUNASWEEPER_API UTunaSweeperVehicleDismountWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	void SetVehicle(ATunaSweeperATVActor* InVehicle);
	void SetDismountHintVisible(bool bVisible) { bDismountHintVisible = bVisible; }
protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
private:
	TWeakObjectPtr<ATunaSweeperATVActor> Vehicle;
	bool bDismountHintVisible = false;
};
