#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "Styling/SlateBrush.h"
#include "TunaSweeperInteractionMarkerWidget.generated.h"

class UTextBlock;
class UWidget;
class UBorder;
class UImage;
class UTexture2D;

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API UTunaSweeperInteractionMarkerWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Interaction")
	void SetMarkerText(const FText& InText);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Interaction")
	void SetInteractionOptions(const TArray<FText>& InOptions, int32 InFocusedOptionIndex);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Interaction")
	void SetRequirementPreview(UTexture2D* InIconTexture, int32 InRequiredQuantity, bool bInShowRequirement);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Interaction")
	void SetMarkerPresentation(float InAlpha, float InRingScale, float InLabelAlpha);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Interaction")
	void SetMarkerOpened(bool bInOpened);

protected:
	virtual void NativeConstruct() override;
	virtual void NativePreConstruct() override;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Interaction", meta = (BindWidgetOptional))
	TObjectPtr<UWidget> MarkerRoot;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Interaction", meta = (BindWidgetOptional))
	TObjectPtr<UWidget> RingImage;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Interaction", meta = (BindWidgetOptional))
	TObjectPtr<UWidget> FilledImage;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Interaction", meta = (BindWidgetOptional))
	TObjectPtr<UBorder> LabelBackground;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Interaction", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DisplayNameText;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Interaction", meta = (BindWidgetOptional))
	TObjectPtr<UWidget> RequirementRoot;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Interaction", meta = (BindWidgetOptional))
	TObjectPtr<UImage> RequirementIconImage;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Interaction", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> RequirementQuantityText;

private:
	void CacheNamedWidgets();
	void EnsureRequirementWidgets();
	void ApplyState();
	FText BuildDisplayText() const;
	UTexture2D* ResolveOpenedCheckTexture();

	FText CachedDisplayText = FText::FromString(TEXT("Interact"));
	TArray<FText> CachedOptionTexts;
	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> CachedRequirementIconTexture;
	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> CachedOpenedCheckTexture;
	UPROPERTY(Transient)
	TObjectPtr<UImage> RingBrushImage;
	UPROPERTY(Transient)
	TObjectPtr<UImage> FilledBrushImage;
	int32 CachedRequiredQuantity = 0;
	int32 CachedFocusedOptionIndex = INDEX_NONE;
	bool bCachedShowRequirement = false;
	bool bCachedOpened = false;
	bool bHasCachedRingBrush = false;
	bool bHasCachedFilledBrush = false;
	FSlateBrush CachedRingBrush;
	FSlateBrush CachedFilledBrush;
	float CachedAlpha = 0.0f;
	float CachedRingScale = 3.0f;
	float CachedLabelAlpha = 0.0f;
};
