#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TunaSweeperVehicleDismountWidget.generated.h"
UCLASS()
class TUNASWEEPER_API UTunaSweeperVehicleDismountWidget : public UUserWidget
{
	GENERATED_BODY()
protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
};
