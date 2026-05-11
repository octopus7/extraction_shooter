#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "UI/TunaSweeperHudTypes.h"
#include "TunaSweeperHudExternalPanelWidget.generated.h"

class UWidget;

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API UTunaSweeperHudExternalPanelWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|HUD")
	void SetExternalPanelMode(ETunaSweeperHudExternalPanelMode InPanelMode);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|HUD")
	ETunaSweeperHudExternalPanelMode GetExternalPanelMode() const { return PanelMode; }

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UWidget> LootingBoxPanel;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UWidget> ShopPanel;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UWidget> StoragePanel;

private:
	void ApplyPanelMode();

	UPROPERTY(EditAnywhere, Category = "TunaSweeper|HUD")
	ETunaSweeperHudExternalPanelMode PanelMode = ETunaSweeperHudExternalPanelMode::None;
};

