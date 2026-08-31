#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "UI/TunaSweeperSpeechBubbleTypes.h"

#include "TunaSweeperScreenSpaceSpeechBubbleWidget.generated.h"

class UImage;
class UOverlay;
class USizeBox;
class UTextBlock;
class UTexture2D;

/** Native, asset-backed speech bubble displayed by the screen-space bubble subsystem. */
UCLASS()
class TUNASWEEPER_API UTunaSweeperScreenSpaceSpeechBubbleWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void Configure(const FText& InText, ETunaSweeperSpeechBubbleTailDirection InTailDirection);
	FVector2D GetLocalAnchorPoint() const;
	ETunaSweeperSpeechBubbleTailDirection GetTailDirection() const { return TailDirection; }

	/** Exposed for deterministic anchor-alignment automation tests. */
	static FVector2D CalculateLocalAnchorPoint(
		ETunaSweeperSpeechBubbleTailDirection InTailDirection,
		const FVector2D& BodySize);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	void EnsureWidgetTree();
	void BuildNativeWidgetTree();
	void ApplyPresentation();
	void ApplyTailLayout();

	UPROPERTY(Transient)
	TObjectPtr<UOverlay> RootOverlay;

	UPROPERTY(Transient)
	TObjectPtr<USizeBox> BodyBox;

	UPROPERTY(Transient)
	TObjectPtr<UImage> BodyImage;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> BubbleText;

	UPROPERTY(Transient)
	TObjectPtr<USizeBox> TailBox;

	UPROPERTY(Transient)
	TObjectPtr<UImage> TailImage;

	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> BodyTexture;

	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> TailTexture;

	FText PendingText;
	ETunaSweeperSpeechBubbleTailDirection TailDirection = ETunaSweeperSpeechBubbleTailDirection::Down;
};
