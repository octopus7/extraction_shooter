#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "TunaSweeperInteractionMarkerWidget.generated.h"

class UTextBlock;
class UWidget;

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API UTunaSweeperInteractionMarkerWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Interaction")
	void SetMarkerText(const FText& InText);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Interaction")
	void SetMarkerPresentation(float InAlpha, float InRingScale);

protected:
	virtual void NativeConstruct() override;
	virtual void NativePreConstruct() override;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Interaction", meta = (BindWidgetOptional))
	TObjectPtr<UWidget> MarkerRoot;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Interaction", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> RingText;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Interaction", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> FilledText;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Interaction", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DisplayNameText;

private:
	void CacheNamedWidgets();
	void ApplyState();

	FText CachedDisplayText = FText::FromString(TEXT("Interact"));
	float CachedAlpha = 0.0f;
	float CachedRingScale = 3.0f;
};
