#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "TunaSweeperHudItemInfoPanelWidget.generated.h"

class UTextBlock;
class UWidget;

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API UTunaSweeperHudItemInfoPanelWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|HUD")
	void SetSelectedItemInfo(const FText& ItemName, const FText& ItemDescription, bool bShowModdingPanel);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|HUD")
	void ClearSelectedItemInfo();

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|HUD")
	void SetModdingPanelVisible(bool bVisible);

protected:
	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SelectedItemNameText;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SelectedItemDescriptionText;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UWidget> ModdingPanel;
};

