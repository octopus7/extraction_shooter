#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "Subsystem/TunaSweeperItemDataSubsystem.h"
#include "TunaSweeperItemRaritySlotAccentWidget.generated.h"

UCLASS(BlueprintType)
class TUNASWEEPER_API UTunaSweeperItemRaritySlotAccentWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetItemGrade(ETunaSweeperItemGrade InItemGrade, bool bInVisible);

protected:
	virtual int32 NativePaint(
		const FPaintArgs& Args,
		const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FWidgetStyle& InWidgetStyle,
		bool bParentEnabled) const override;

private:
	UPROPERTY(Transient)
	ETunaSweeperItemGrade ItemGrade = ETunaSweeperItemGrade::Common;

	UPROPERTY(Transient)
	bool bRarityVisible = false;
};
