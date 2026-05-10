#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "TunaSweeperTempOpenLootWidget.generated.h"

class UButton;
class UTileView;
class UTunaSweeperTempOpenLootTileItemObject;

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API UTunaSweeperTempOpenLootWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Temp Open Loot", meta = (BindWidgetOptional))
	TObjectPtr<UTileView> TempLootTileView;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Temp Open Loot", meta = (BindWidgetOptional))
	TObjectPtr<UButton> TempCloseButton;

private:
	UFUNCTION()
	void HandleTempCloseClicked();

	void PopulateTempLootTiles();

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTunaSweeperTempOpenLootTileItemObject>> TempTileItemObjects;
};

