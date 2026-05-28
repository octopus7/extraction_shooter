#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "Rendering/RenderingCommon.h"
#include "TunaSweeperLevelTransitionWidget.generated.h"

class UBorder;
class UImage;
class UMediaTexture;
class UTextBlock;

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API UTunaSweeperLevelTransitionWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Level Transition")
	void SetVideoTexture(UMediaTexture* InMediaTexture);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Level Transition")
	void SetVideoVisible(bool bVisible);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Level Transition")
	void SetBlackOpacity(float InOpacity);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Level Transition")
	void SetCircularRevealMask(float HoleRadiusPixels, bool bVisible);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Level Transition")
	void SetLetterboxEnabled(bool bEnabled);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Level Transition")
	void SetTransitionMessage(const FText& InMessage);

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

	void EnsureLetterboxPanels();
	void UpdateLetterboxVisibility();
	void DrawCircularRevealMask(
		const FGeometry& AllottedGeometry,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId) const;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Level Transition", meta = (BindWidgetOptional))
	TObjectPtr<UImage> VideoImage;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Level Transition", meta = (BindWidgetOptional))
	TObjectPtr<UBorder> LetterboxTopPanel;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Level Transition", meta = (BindWidgetOptional))
	TObjectPtr<UBorder> LetterboxBottomPanel;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Level Transition", meta = (BindWidgetOptional))
	TObjectPtr<UBorder> BlackFadePanel;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Level Transition", meta = (BindWidgetOptional))
	TObjectPtr<UBorder> MessageBackground;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Level Transition", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TransitionMessageText;

	bool bVideoVisible = false;
	bool bLetterboxEnabled = false;
	bool bCircularRevealMaskVisible = false;
	float CircularRevealHoleRadiusPixels = 0.0f;
	mutable TArray<FSlateVertex> CircularRevealVertices;
	mutable TArray<SlateIndex> CircularRevealIndices;
};
