#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "Subsystem/TunaSweeperItemDataSubsystem.h"
#include "TunaSweeperLootContainerWidget.generated.h"

class USizeBox;
class UTextBlock;
class UTileView;
class UDragDropOperation;

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API UTunaSweeperLootContainerWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Loot Container")
	void SetContainerInstance(const FTunaSweeperLootContainerInstance& InContainerInstance);

protected:
	virtual void NativeConstruct() override;
	virtual bool NativeOnDrop(
		const FGeometry& InGeometry,
		const FDragDropEvent& InDragDropEvent,
		UDragDropOperation* InOperation) override;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Loot Container", meta = (BindWidgetOptional))
	TObjectPtr<USizeBox> RootSizeBox;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Loot Container", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ContainerTitleText;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Loot Container", meta = (BindWidgetOptional))
	TObjectPtr<UTileView> ContainerTileView;

private:
	void PopulateContainerItems();

	UPROPERTY(Transient)
	FTunaSweeperLootContainerInstance ContainerInstance;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UObject>> TileObjects;
};
