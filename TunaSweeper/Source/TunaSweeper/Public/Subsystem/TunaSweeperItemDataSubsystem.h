#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "TunaSweeperItemDataSubsystem.generated.h"

UENUM(BlueprintType)
enum class ETunaSweeperItemTextLanguage : uint8
{
	Korean UMETA(DisplayName = "Korean"),
	English UMETA(DisplayName = "English"),
	Japanese UMETA(DisplayName = "Japanese")
};

USTRUCT(BlueprintType)
struct TUNASWEEPER_API FTunaSweeperItemDefinition
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Item")
	int32 Id = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Item")
	FName NameStringKey;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Item")
	int32 ShopSellPrice = 0;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Item")
	FString IconFileName;
};

USTRUCT(BlueprintType)
struct TUNASWEEPER_API FTunaSweeperItemNameString
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Item")
	FName StringKey;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Item")
	FText Korean;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Item")
	FText English;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Item")
	FText Japanese;
};

UCLASS()
class TUNASWEEPER_API UTunaSweeperItemDataSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Item Data")
	bool LoadItemData(bool bForceReload = false);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Item Data")
	bool IsItemDataLoaded() const { return bItemDataLoaded; }

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Item Data")
	bool TryGetItemDefinition(int32 ItemId, FTunaSweeperItemDefinition& OutItemDefinition);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Item Data")
	bool TryGetItemNameString(FName NameStringKey, FTunaSweeperItemNameString& OutNameString);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Item Data")
	bool TryGetItemNameTextByKey(FName NameStringKey, ETunaSweeperItemTextLanguage Language, FText& OutText);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Item Data")
	bool TryGetItemNameText(int32 ItemId, ETunaSweeperItemTextLanguage Language, FText& OutText);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Item Data")
	bool GetAllItemDefinitions(TArray<FTunaSweeperItemDefinition>& OutItemDefinitions);

private:
	bool EnsureItemDataLoaded();
	bool LoadItemTableJson();
	bool LoadItemNameStringsCsv();
	void ResetLoadedItemData();
	FString GetItemTableJsonPath() const;
	FString GetItemNameStringsCsvPath() const;

	UPROPERTY(Transient)
	TMap<int32, FTunaSweeperItemDefinition> ItemDefinitionsById;

	UPROPERTY(Transient)
	TMap<FName, FTunaSweeperItemNameString> ItemNameStringsByKey;

	bool bItemDataLoaded = false;
};
