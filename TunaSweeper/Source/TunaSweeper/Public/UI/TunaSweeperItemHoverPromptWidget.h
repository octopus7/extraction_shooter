#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "UI/TunaSweeperItemStackTileItemObject.h"
#include "TunaSweeperItemHoverPromptWidget.generated.h"

class UBorder;
class USizeBox;
class UTextBlock;
class UWidget;

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API UTunaSweeperItemHoverPromptWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Item Hover Prompt")
	void SetItemTileData(const FTunaSweeperItemStackTileData& InTileData);

	void SetPromptViewportPosition(const FVector2D& ViewportPosition);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativePreConstruct() override;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Item Hover Prompt", meta = (BindWidgetOptional))
	TObjectPtr<USizeBox> RootSizeBox;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Item Hover Prompt", meta = (BindWidgetOptional))
	TObjectPtr<UBorder> ItemInfoBackground;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Item Hover Prompt", meta = (BindWidgetOptional))
	TObjectPtr<UBorder> ActionHintsBackground;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Item Hover Prompt", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ItemNameText;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Item Hover Prompt", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ItemWeightText;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Item Hover Prompt", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ItemDescriptionText;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Item Hover Prompt", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ItemPriceText;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Item Hover Prompt", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TakeKeyText;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Item Hover Prompt", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TakeActionText;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Item Hover Prompt", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DropKeyText;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Item Hover Prompt", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DropActionText;

private:
	void BuildNativeWidgetTree();
	void CacheNamedWidgets();
	void ApplyTileData();
	FText BuildNameText() const;
	FText BuildWeightText() const;
	FText BuildPriceText() const;

	UPROPERTY(Transient)
	FTunaSweeperItemStackTileData CachedTileData;
};
