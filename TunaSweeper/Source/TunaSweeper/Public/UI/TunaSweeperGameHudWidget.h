#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "Subsystem/TunaSweeperItemDataSubsystem.h"
#include "UI/TunaSweeperHudTypes.h"
#include "TunaSweeperGameHudWidget.generated.h"

class UTunaSweeperHudBottomStatusWidget;
class UTunaSweeperHudExternalPanelWidget;
class UTunaSweeperHudInventoryAreaWidget;
class UTunaSweeperHudItemInfoPanelWidget;
class UTunaSweeperHudQuickSlotBarWidget;
class UTunaSweeperHudTopReserveWidget;
class UWidget;

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API UTunaSweeperGameHudWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|HUD")
	void SetCenterPanelsVisible(bool bVisible);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|HUD")
	void SetInventoryAreaVisible(bool bVisible);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|HUD")
	void SetItemInfoPanelVisible(bool bVisible);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|HUD")
	void ShowExternalPanel(ETunaSweeperHudExternalPanelMode PanelMode);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|HUD")
	void ShowInventoryOnlyPanel();

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|HUD")
	void ToggleInventoryOnlyPanel();

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|HUD")
	void ShowLootContainerPanel(const FTunaSweeperLootContainerInstance& ContainerInstance);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|HUD")
	UTunaSweeperHudQuickSlotBarWidget* GetQuickSlotBarWidget() const { return QuickSlotBarWidget; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|HUD")
	UTunaSweeperHudBottomStatusWidget* GetBottomStatusWidget() const { return BottomStatusWidget; }

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UTunaSweeperHudTopReserveWidget> TopStatusReserveWidget;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UTunaSweeperHudBottomStatusWidget> BottomStatusWidget;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UTunaSweeperHudQuickSlotBarWidget> QuickSlotBarWidget;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UWidget> CenterContentPanel;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UTunaSweeperHudInventoryAreaWidget> InventoryAreaWidget;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UTunaSweeperHudItemInfoPanelWidget> ItemInfoPanelWidget;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UTunaSweeperHudExternalPanelWidget> ExternalPanelWidget;

private:
	void RefreshBottomStatusFromGameInstance();
};
