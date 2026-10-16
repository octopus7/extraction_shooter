#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "UI/TunaSweeperSpeechBubbleTypes.h"

#include "TunaSweeperScreenSpaceSpeechBubbleWidget.generated.h"

class USizeBox;
class UTextBlock;
class UImage;

/** Behavior for WBP_CharacterSpeechBubble; its presentation is authored in the Designer. */
UCLASS()
class TUNASWEEPER_API UTunaSweeperScreenSpaceSpeechBubbleWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Speech Bubble")
	void Configure(const FText& InText, ETunaSweeperSpeechBubbleTailDirection InTailDirection);
	FVector2D GetLocalAnchorPoint() const;
	ETunaSweeperSpeechBubbleTailDirection GetTailDirection() const { return TailDirection; }

	/** Exposed for deterministic anchor-alignment automation tests. */
	static FVector2D CalculateLocalAnchorPoint(
		ETunaSweeperSpeechBubbleTailDirection InTailDirection,
		const FVector2D& BodySize,
		const FVector2D& TailSize = FVector2D(34.0f, 34.0f),
		float TailReserve = 24.0f,
		float TailTipInset = 1.0f);

	/** Space reserved between the body and the outer edge on the active tail side. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Speech Bubble", meta = (ClampMin = "0"))
	float TailReserve = 24.0f;

	/** Inset between the tail sizing guide edge and its visible tip. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Speech Bubble", meta = (ClampMin = "0"))
	float TailTipInset = 1.0f;

	/** Existing localized string used only by the Designer preview. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Speech Bubble|Preview")
	FName PreviewTextStringKey = TEXT("ui.vehicle.no_fuel");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Speech Bubble|Preview")
	ETunaSweeperSpeechBubbleTailDirection PreviewTailDirection = ETunaSweeperSpeechBubbleTailDirection::Down;

protected:
	virtual void NativePreConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	void ApplyPresentation();
	void ApplyTailLayout();
	void UpdateShapeMaterial() const;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> BodyImage;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USizeBox> BodyBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> BubbleText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USizeBox> TailBox;

	FText PendingText;
	bool bHasPendingText = false;
	ETunaSweeperSpeechBubbleTailDirection TailDirection = ETunaSweeperSpeechBubbleTailDirection::Down;
};
