#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TunaSweeperSpeechBubbleWidget.generated.h"

class UTextBlock;

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API UTunaSweeperSpeechBubbleWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Speech Bubble")
	void SetBubbleText(const FText& InText);

protected:
	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Speech Bubble", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> BubbleText;
};
