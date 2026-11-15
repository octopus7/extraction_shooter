#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "Game/TunaSweeperGameInstance.h"
#include "TunaSweeperHudBottomStatusWidget.generated.h"

class UProgressBar;
class UImage;
class UTextBlock;
class UWidget;
class UTunaSweeperHudStatusRingWidget;

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API UTunaSweeperHudBottomStatusWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|HUD")
	void SetHudState(const FTunaSweeperPlayerHudState& InHudState);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|HUD")
	void SetScratchState(float CurrentScratch, float MaxScratch);

protected:
	virtual void NativeConstruct() override;
	virtual void NativePreConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// The image and its health-relative layout are authored in the Widget Blueprint.
	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UImage> CheatActiveBadge;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> WeightText;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UWidget> WeightRow;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UWidget> GaugeOverlay;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> HealthText;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> HungerText;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> HydrationText;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> HealthGauge;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> ScratchGauge;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> HungerGauge;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> HydrationGauge;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UTunaSweeperHudStatusRingWidget> HealthRing;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UTunaSweeperHudStatusRingWidget> HungerRing;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UTunaSweeperHudStatusRingWidget> HydrationRing;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> CarryWeightGauge;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UWidget> WeightWarningIcon;

private:
	void ApplyHudState();
	void UpdateCheatActiveBadge();

	UPROPERTY(EditAnywhere, Category = "TunaSweeper|HUD")
	FTunaSweeperPlayerHudState PreviewHudState;

	float ScratchPercent = 0.0f;
};
