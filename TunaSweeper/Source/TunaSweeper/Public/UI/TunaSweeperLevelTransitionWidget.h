#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "TunaSweeperLevelTransitionWidget.generated.h"

class UBorder;
class UImage;
class UMediaTexture;

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

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Level Transition", meta = (BindWidgetOptional))
	TObjectPtr<UImage> VideoImage;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Level Transition", meta = (BindWidgetOptional))
	TObjectPtr<UBorder> BlackFadePanel;
};
