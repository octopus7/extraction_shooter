#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "Styling/SlateBrush.h"
#include "TunaSweeperVisionMaskWidget.generated.h"

class UTexture2D;

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API UTunaSweeperVisionMaskWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Vision")
	void SetMaskTexture(UTexture2D* InMaskTexture);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Vision")
	void SetMaskVisible(bool bVisible);

protected:
	virtual void NativeConstruct() override;
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
	TObjectPtr<UTexture2D> MaskTexture;

	mutable FSlateBrush MaskBrush;
};
