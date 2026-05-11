#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "TunaSweeperPickupItemIconWidget.generated.h"

class UImage;
class UTexture2D;

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API UTunaSweeperPickupItemIconWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Pickup Item")
	void SetIconTexture(UTexture2D* InIconTexture);

protected:
	virtual void NativeConstruct() override;
	virtual void NativePreConstruct() override;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Pickup Item", meta = (BindWidgetOptional))
	TObjectPtr<UImage> ItemIconImage;

private:
	void CacheNamedWidgets();
	void ApplyIconTexture();

	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> CachedIconTexture;
};
