#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "Styling/SlateBrush.h"
#include "TunaSweeperVisionMaskWidget.generated.h"

class UTexture2D;
class UImage;

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API UTunaSweeperVisionMaskWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Vision")
	void SetMaskTexture(UTexture2D* InMaskTexture);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Vision")
	void SetMaskVisible(bool bVisible);

protected:
	virtual void NativeConstruct() override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> MaskTexture;

	UPROPERTY(Transient)
	TObjectPtr<UImage> MaskImage;

	mutable FSlateBrush MaskBrush;
};
