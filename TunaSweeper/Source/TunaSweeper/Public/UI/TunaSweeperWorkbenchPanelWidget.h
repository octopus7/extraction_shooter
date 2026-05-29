#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "Game/TunaSweeperGameInstance.h"
#include "TunaSweeperWorkbenchPanelWidget.generated.h"

class UButton;
class UImage;
class UTextBlock;
class UTileView;
class UVerticalBox;

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API UTunaSweeperWorkbenchPanelWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Workbench")
	void SetWorkbenchContext(
		int32 WorkbenchId,
		ETunaSweeperWorkbenchMode WorkbenchMode = ETunaSweeperWorkbenchMode::Craft);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Workbench")
	void RefreshWorkbenchView();

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Workbench")
	void SelectCraftRecipe(int32 RecipeSlotIndex);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Workbench")
	void SelectDismantleCandidate(const FTunaSweeperItemSlotReference& SlotReference);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Workbench")
	void SelectBlueprintItem(const FTunaSweeperItemSlotReference& SlotReference);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Workbench")
	bool ExecuteSelectedWorkbenchAction();

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Workbench")
	ETunaSweeperWorkbenchMode GetWorkbenchMode() const { return ActiveWorkbenchMode; }

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Workbench", meta = (BindWidgetOptional))
	TObjectPtr<UTileView> CraftRecipeTileView;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Workbench", meta = (BindWidgetOptional))
	TObjectPtr<UTileView> DismantleInventoryTileView;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Workbench", meta = (BindWidgetOptional))
	TObjectPtr<UTileView> DismantleStorageTileView;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Workbench", meta = (BindWidgetOptional))
	TObjectPtr<UTileView> BlueprintItemTileView;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Workbench", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> WorkbenchTitleText;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Workbench", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DismantleInventoryHeaderText;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Workbench", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DismantleStorageHeaderText;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Workbench", meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> CraftIngredientList;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Workbench", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CraftArrowText;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Workbench", meta = (BindWidgetOptional))
	TObjectPtr<UImage> CraftOutputImage;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Workbench", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CraftOutputText;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Workbench", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DismantleResultText;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Workbench", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> BlueprintRegisterText;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Workbench", meta = (BindWidgetOptional))
	TObjectPtr<UButton> CraftButton;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Workbench", meta = (BindWidgetOptional))
	TObjectPtr<UButton> DismantleButton;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Workbench", meta = (BindWidgetOptional))
	TObjectPtr<UButton> BlueprintRegisterButton;

private:
	UFUNCTION()
	void HandleCraftButtonClicked();

	UFUNCTION()
	void HandleDismantleButtonClicked();

	UFUNCTION()
	void HandleBlueprintRegisterButtonClicked();

	void HandleCraftTileClicked(UObject* ItemObject);
	void HandleDismantleTileClicked(UObject* ItemObject);
	void HandleBlueprintTileClicked(UObject* ItemObject);
	void PopulateCraftRecipes();
	void PopulateDismantleItems();
	void PopulateBlueprintItems();
	void RefreshCraftDetails();
	void RefreshDismantleDetails();
	void RefreshBlueprintDetails();
	void SetActionButtonState(UButton* Button, bool bEnabled) const;
	void SetPanelModeVisibility() const;

	UPROPERTY(Transient)
	int32 ActiveWorkbenchId = INDEX_NONE;

	UPROPERTY(Transient)
	ETunaSweeperWorkbenchMode ActiveWorkbenchMode = ETunaSweeperWorkbenchMode::Craft;

	UPROPERTY(Transient)
	int32 SelectedCraftRecipeSlotIndex = INDEX_NONE;

	UPROPERTY(Transient)
	FTunaSweeperItemSlotReference SelectedDismantleSlot;

	UPROPERTY(Transient)
	FTunaSweeperItemSlotReference SelectedBlueprintSlot;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UObject>> TileObjects;
};
