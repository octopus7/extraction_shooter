#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"

#include "TunaSweeperSpeechBubbleLayerWidget.generated.h"

class UCanvasPanel;
class UTunaSweeperScreenSpaceSpeechBubbleWidget;

/** Full-viewport, hit-test-invisible host for transient speech bubbles. */
UCLASS()
class TUNASWEEPER_API UTunaSweeperSpeechBubbleLayerWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	bool AddBubble(UTunaSweeperScreenSpaceSpeechBubbleWidget* Bubble);
	void RemoveBubble(UTunaSweeperScreenSpaceSpeechBubbleWidget* Bubble);
	void RemoveAllBubbles();
	void SetBubbleAnchor(UTunaSweeperScreenSpaceSpeechBubbleWidget* Bubble, const FVector2D& LogicalAnchor);
	FVector2D GetLogicalLayerSize() const;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	void EnsureWidgetTree();

	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanel> BubbleCanvas;
};
