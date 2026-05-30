#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "Inventory/TunaSweeperInventoryTypes.h"
#include "Subsystem/TunaSweeperItemDataSubsystem.h"
#include "UI/TunaSweeperHudTypes.h"
#include "TunaSweeperHudExternalPanelWidget.generated.h"

class UWidget;
class ULootContainerWidget;
class UShopContainerWidget;
class UStorageContainerWidget;
class UTunaSweeperWorkbenchPanelWidget;

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API UTunaSweeperHudExternalPanelWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|HUD")
	void SetExternalPanelMode(ETunaSweeperHudExternalPanelMode InPanelMode);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|HUD")
	ETunaSweeperHudExternalPanelMode GetExternalPanelMode() const { return PanelMode; }

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|HUD")
	void SetLootContainerInstance(const FTunaSweeperLootContainerInstance& InContainerInstance);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|HUD")
	void SetStorageContainer();

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|HUD")
	void SetShopContainer(int32 ShopId);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|HUD")
	void SetWorkbenchContainer(
		int32 WorkbenchId,
		ETunaSweeperWorkbenchMode WorkbenchMode = ETunaSweeperWorkbenchMode::Craft);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Workbench")
	bool AssignWorkbenchDismantleCandidateToTarget(const FTunaSweeperItemSlotReference& SlotReference);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Workbench")
	bool AssignFocusedWorkbenchDismantleCandidateToTarget();

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Workbench")
	bool AssignWorkbenchBlueprintItemToTarget(const FTunaSweeperItemSlotReference& SlotReference);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Workbench")
	bool AssignFocusedWorkbenchBlueprintItemToTarget();

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UWidget> LootingBoxPanel;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UWidget> ShopPanel;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UWidget> StoragePanel;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UWidget> WorkbenchPanel;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UTunaSweeperWorkbenchPanelWidget> WorkbenchPanelWidget;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional))
	TObjectPtr<ULootContainerWidget> LootContainerWidget;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UStorageContainerWidget> StorageContainerWidget;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UShopContainerWidget> ShopContainerWidget;

private:
	void ApplyPanelMode();
	void ApplyLootContainerPanelLayout();

	UPROPERTY(EditAnywhere, Category = "TunaSweeper|HUD")
	ETunaSweeperHudExternalPanelMode PanelMode = ETunaSweeperHudExternalPanelMode::None;
};
