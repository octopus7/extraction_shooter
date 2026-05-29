#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "TunaSweeperShopSellPanelWidget.generated.h"

class UBorder;
class UButton;
class UImage;
class UTextBlock;
class UTexture2D;

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API UTunaSweeperShopSellPanelWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Shop")
	void RefreshSelectedItem();

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Shop")
	bool TrySellSelectedHoveredItem();

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativePreConstruct() override;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Shop", meta = (BindWidgetOptional))
	TObjectPtr<UBorder> PanelBackground;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Shop", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ItemNameText;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Shop", meta = (BindWidgetOptional))
	TObjectPtr<UImage> ItemIconImage;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Shop", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SalePriceText;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Shop", meta = (BindWidgetOptional))
	TObjectPtr<UButton> SellButton;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Shop", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SellButtonText;

private:
	void BuildNativeWidgetTree();
	void CacheNamedWidgets();
	void SetItemThumbnail(UTexture2D* IconTexture);
	void ClearItemThumbnail();

	UFUNCTION()
	void HandleSellButtonClicked();
};
