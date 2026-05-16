#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "TunaSweeperHudInventoryAreaWidget.generated.h"

class UWidget;
class UTileView;
class UDragDropOperation;
class UButton;

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API UTunaSweeperHudInventoryAreaWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|HUD")
	void SetInventoryVisible(bool bVisible);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|HUD")
	void SetAuxiliaryBagVisible(bool bVisible);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|HUD")
	void RefreshInventoryItems();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual bool NativeOnDrop(
		const FGeometry& InGeometry,
		const FDragDropEvent& InDragDropEvent,
		UDragDropOperation* InOperation) override;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UWidget> InventoryPanel;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UWidget> AuxiliaryBagPanel;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UTileView> EquipmentReserveTileView;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UTileView> AuxiliaryBagTileView;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UTileView> InventoryTileView;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UButton> SortInventoryButton;

private:
	UFUNCTION()
	void HandleSortInventoryClicked();

	UPROPERTY(Transient)
	TArray<TObjectPtr<UObject>> TileObjects;
};
