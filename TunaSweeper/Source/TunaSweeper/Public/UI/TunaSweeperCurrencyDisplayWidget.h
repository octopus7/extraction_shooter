#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "Rendering/RenderingCommon.h"
#include "TunaSweeperCurrencyDisplayWidget.generated.h"

class UHorizontalBox;
class UImage;
class USizeBox;
class UTextBlock;
class UTexture2D;

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API UTunaSweeperCurrencyDisplayWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Currency")
	void RefreshCurrencyBalance();

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Currency")
	void SetCurrencyAmount(int32 InCurrencyAmount);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Currency")
	int32 GetCurrencyAmount() const { return CurrencyAmount; }

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Currency")
	void EnsureCurrencyContent();

	static UTexture2D* LoadCurrencyCoinIconTexture();

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativePreConstruct() override;
	virtual int32 NativePaint(
		const FPaintArgs& Args,
		const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FWidgetStyle& InWidgetStyle,
		bool bParentEnabled) const override;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Currency", meta = (BindWidgetOptional))
	TObjectPtr<UHorizontalBox> RootBox;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Currency", meta = (BindWidgetOptional))
	TObjectPtr<UImage> CoinImage;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Currency", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> BalanceText;

private:
	void BuildNativeWidgetTree();
	void CacheNamedWidgets();
	void ApplyCurrencyPresentation();
	void HandleQuestProgressChanged();

	UPROPERTY(Transient)
	int32 CurrencyAmount = 0;
};
