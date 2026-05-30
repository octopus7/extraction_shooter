#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "Rendering/RenderingCommon.h"
#include "TunaSweeperItemHoverBorderEffectWidget.generated.h"

UCLASS(BlueprintType)
class TUNASWEEPER_API UTunaSweeperItemHoverBorderEffectWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetHoverBorderEffectActive(bool bInActive);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual int32 NativePaint(
		const FPaintArgs& Args,
		const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FWidgetStyle& InWidgetStyle,
		bool bParentEnabled) const override;

private:
	bool bEffectActive = false;
	float AnimationSeconds = 0.0f;
	float EffectOpacity = 0.0f;
};
