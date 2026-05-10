#pragma once

#include "CoreMinimal.h"
#include "Game/TunaSweeperGameInstance.h"
#include "UObject/Object.h"
#include "TunaSweeperTempOpenLootTileItemObject.generated.h"

UCLASS(BlueprintType)
class TUNASWEEPER_API UTunaSweeperTempOpenLootTileItemObject : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(const FTunaSweeperTempOpenLootItemData& InItemData);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Temp Open Loot")
	const FTunaSweeperTempOpenLootItemData& GetItemData() const { return ItemData; }

private:
	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Temp Open Loot", meta = (AllowPrivateAccess = "true"))
	FTunaSweeperTempOpenLootItemData ItemData;
};

