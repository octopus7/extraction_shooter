#pragma once

#include "Blueprint/IUserObjectListEntry.h"
#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "TunaSweeperTempOpenLootTileEntryWidget.generated.h"

class UImage;
class UTextBlock;

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API UTunaSweeperTempOpenLootTileEntryWidget : public UUserWidget, public IUserObjectListEntry
{
	GENERATED_BODY()

protected:
	virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Temp Open Loot", meta = (BindWidgetOptional))
	TObjectPtr<UImage> TempItemIconImage;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Temp Open Loot", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TempItemQuantityText;
};

