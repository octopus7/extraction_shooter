#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LoopRailArrivalWidget.generated.h"

/** A station display face. The owning presentation code supplies localized text. */
UCLASS(BlueprintType, Blueprintable)
class LOOPRAIL_API ULoopRailArrivalWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "LoopRail|Arrival Display")
	void SetArrivalDisplayText(const FText& InText);

	UFUNCTION(BlueprintPure, Category = "LoopRail|Arrival Display")
	FText GetArrivalDisplayText() const { return ArrivalDisplayText; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LoopRail|Arrival Display")
	FLinearColor DotColor = FLinearColor(0.015f, 0.48f, 1.0f, 1.0f);

protected:
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
		int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

private:
	UPROPERTY(Transient)
	FText ArrivalDisplayText;
};
