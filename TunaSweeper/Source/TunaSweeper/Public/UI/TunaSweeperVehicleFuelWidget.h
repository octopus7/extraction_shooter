#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TunaSweeperVehicleFuelWidget.generated.h"

/** A texture-free fuel capsule; its owning vehicle HUD supplies position and visibility. */
UCLASS()
class TUNASWEEPER_API UTunaSweeperVehicleFuelWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetFuelRatio(float InFuelRatio);

protected:
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId,
		const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

private:
	float FuelRatio = 0.0f;
};
