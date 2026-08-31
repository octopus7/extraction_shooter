#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TunaThoughtIndicatorWidget.generated.h"

class UCanvasPanel;
class UImage;
class UOverlay;
class UTexture2D;

UCLASS(BlueprintType)
class TUNASWEEPER_API UTunaThoughtIndicatorWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void Configure(UTexture2D* InTexture, FVector2D InImageSizePixels, FVector2D InCanvasSizePixels);
	void SetBobOffsetPixels(float InOffsetPixels);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Tuna Thought Indicator")
	float GetBobOffsetPixels() const { return BobOffsetPixels; }

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativePreConstruct() override;

private:
	void EnsureWidgetTree();
	void BuildNativeWidgetTree();
	void ApplyPresentation();
	void ApplyLayout();

	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanel> RootCanvas;

	UPROPERTY(Transient)
	TObjectPtr<UOverlay> IconOverlay;

	UPROPERTY(Transient)
	TObjectPtr<UImage> ShadowImage;

	UPROPERTY(Transient)
	TObjectPtr<UImage> IndicatorImage;

	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> IndicatorTexture;

	FVector2D ImageSizePixels = FVector2D(176.0f, 176.0f);
	FVector2D CanvasSizePixels = FVector2D(192.0f, 208.0f);
	float BobOffsetPixels = 0.0f;
};
