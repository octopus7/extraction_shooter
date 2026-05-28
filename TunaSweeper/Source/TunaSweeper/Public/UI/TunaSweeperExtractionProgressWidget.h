#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "TunaSweeperExtractionProgressWidget.generated.h"

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API UTunaSweeperExtractionProgressWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Extraction")
	void SetExtractionProgress(float InCurrentSeconds, float InRequiredSeconds, bool bInVisible);

protected:
	virtual void NativeConstruct() override;
	virtual int32 NativePaint(
		const FPaintArgs& Args,
		const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FWidgetStyle& InWidgetStyle,
		bool bParentEnabled) const override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Extraction", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float BorderThickness = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Extraction")
	FLinearColor BorderColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Extraction")
	FLinearColor FillColor = FLinearColor(0.1f, 0.9f, 0.22f, 0.95f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Extraction")
	FLinearColor EmptyColor = FLinearColor(0.02f, 0.025f, 0.025f, 0.92f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Extraction")
	FLinearColor TextColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Extraction")
	FLinearColor TextShadowColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.85f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Extraction", meta = (ClampMin = "1", UIMin = "1"))
	int32 FontSize = 15;

private:
	float CurrentSeconds = 0.0f;
	float RequiredSeconds = 4.0f;
	bool bVisibleGauge = true;
};
