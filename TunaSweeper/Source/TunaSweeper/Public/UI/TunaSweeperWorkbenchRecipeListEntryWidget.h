#pragma once

#include "Blueprint/IUserObjectListEntry.h"
#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "UI/TunaSweeperItemStackTileItemObject.h"
#include "TunaSweeperWorkbenchRecipeListEntryWidget.generated.h"

class UBorder;
class UImage;
class UTextBlock;

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API UTunaSweeperWorkbenchRecipeListEntryWidget : public UUserWidget, public IUserObjectListEntry
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;
	virtual void NativeOnItemSelectionChanged(bool bIsSelected) override;
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Workbench", meta = (BindWidgetOptional))
	TObjectPtr<UBorder> RowBackground;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Workbench", meta = (BindWidgetOptional))
	TObjectPtr<UImage> RecipeIconImage;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Workbench", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> RecipeNameText;

private:
	void EnsureWidgetTree();
	void ApplyTileData();
	void ApplyRowState();

	UPROPERTY(Transient)
	FTunaSweeperItemStackTileData CachedTileData;

	bool bSelected = false;
	bool bHovered = false;
};
