#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateBrush.h"
#include "Blueprint/UserWidget.h"
#include "TunaSweeperSpeechBubbleWidget.generated.h"

class UBorder;
class UCanvasPanelSlot;
class UTextBlock;
class UTexture2D;

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API UTunaSweeperSpeechBubbleWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Speech Bubble")
	void SetBubbleText(const FText& InText);

	static bool IsAlertBubbleText(const FText& InText);

protected:
	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Speech Bubble", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> BubbleText;

private:
	void ResolvePresentationWidgets();
	void ApplyAlertPresentation(bool bAlertPresentation);

	UPROPERTY(Transient)
	TObjectPtr<UBorder> BubbleBackground;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> BubbleTail;

	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanelSlot> BubbleBackgroundSlot;

	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> AlertCloudTexture;

	FSlateBrush DefaultBackgroundBrush;
	FMargin DefaultBackgroundPadding;
	FVector2D DefaultBackgroundPosition = FVector2D::ZeroVector;
	FVector2D DefaultBackgroundSize = FVector2D::ZeroVector;
	ESlateVisibility DefaultTailVisibility = ESlateVisibility::Visible;
	int32 DefaultFontSize = 28;
	bool bDefaultPresentationCached = false;
};
