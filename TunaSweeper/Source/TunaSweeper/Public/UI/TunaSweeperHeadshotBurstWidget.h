#pragma once

#include "Blueprint/UserWidget.h"
#include "TunaSweeperHeadshotBurstWidget.generated.h"

/** Resolution-independent comic impact backing for a headshot damage number. */
UCLASS()
class TUNASWEEPER_API UTunaSweeperHeadshotBurstWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
		int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
};
