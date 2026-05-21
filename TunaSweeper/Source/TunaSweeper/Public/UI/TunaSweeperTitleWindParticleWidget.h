#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "Styling/SlateBrush.h"
#include "TunaSweeperTitleWindParticleWidget.generated.h"

class UTexture2D;

UCLASS()
class TUNASWEEPER_API UTunaSweeperTitleWindParticleWidget : public UUserWidget
{
	GENERATED_BODY()

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
	void EnsureParticleTexture();

	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> ParticleTexture;

	FSlateBrush ParticleBrush;
	float AnimationSeconds = 0.0f;
};
