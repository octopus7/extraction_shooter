#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
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
	void SetTransitionMessage(const FText& InMessage);

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Level Transition", meta = (BindWidgetOptional))
	TObjectPtr<UImage> VideoImage;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Level Transition", meta = (BindWidgetOptional))
	TObjectPtr<UBorder> BlackFadePanel;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Level Transition", meta = (BindWidgetOptional))
	TObjectPtr<UBorder> MessageBackground;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Level Transition", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TransitionMessageText;
};
