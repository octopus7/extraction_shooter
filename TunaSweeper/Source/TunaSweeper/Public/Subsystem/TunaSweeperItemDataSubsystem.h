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
	FName DescriptionStringKey;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Item")
	int32 ShopSellPrice = 0;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Item")
	FString IconFileName;
};

USTRUCT(BlueprintType)
struct TUNASWEEPER_API FTunaSweeperItemStack
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Item")
	int32 ItemId = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Item", meta = (ClampMin = "1", UIMin = "1"))
	int32 Quantity = 1;
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

USTRUCT(BlueprintType)
struct TUNASWEEPER_API FTunaSweeperLootContainerDefinition
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Loot Container")
	int32 Id = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Loot Container")
	FName NameStringKey;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Loot Container")
	int32 Capacity = 5;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Loot Container")
	FString StaticMeshPath;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Loot Container")
	FVector MeshScale = FVector::OneVector;
};

USTRUCT(BlueprintType)
struct TUNASWEEPER_API FTunaSweeperLootContainerContents
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Loot Container")
	int32 Id = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Loot Container")
	TArray<FTunaSweeperItemStack> Items;
};

USTRUCT(BlueprintType)
struct TUNASWEEPER_API FTunaSweeperLootContainerInstance
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Loot Container")
	int32 ContainerDefinitionId = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Loot Container")
	int32 ContentsId = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Loot Container")
	FText DisplayName;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Loot Container")
	int32 Capacity = 5;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Loot Container")
	TArray<FTunaSweeperItemStack> Items;
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
	bool TryGetItemString(FName StringKey, FTunaSweeperItemNameString& OutItemString);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Item Data")
	bool TryGetItemNameTextByKey(FName NameStringKey, ETunaSweeperItemTextLanguage Language, FText& OutText);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Item Data")
	bool TryGetItemTextByKey(FName StringKey, ETunaSweeperItemTextLanguage Language, FText& OutText);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Item Data")
	bool TryGetItemNameText(int32 ItemId, ETunaSweeperItemTextLanguage Language, FText& OutText);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Item Data")
	bool TryGetItemDescriptionText(int32 ItemId, ETunaSweeperItemTextLanguage Language, FText& OutText);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Item Data")
	bool GetAllItemDefinitions(TArray<FTunaSweeperItemDefinition>& OutItemDefinitions);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Item Data")
	bool TryGetLootContainerDefinition(int32 ContainerDefinitionId, FTunaSweeperLootContainerDefinition& OutDefinition);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Item Data")
	bool TryGetLootContainerContents(int32 ContentsId, FTunaSweeperLootContainerContents& OutContents);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Item Data")
	bool TryBuildLootContainerInstance(
		int32 ContainerDefinitionId,
		int32 ContentsId,
		ETunaSweeperItemTextLanguage Language,
		FTunaSweeperLootContainerInstance& OutInstance);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Item Data")
	bool GetAllLootContainerDefinitions(TArray<FTunaSweeperLootContainerDefinition>& OutDefinitions);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Item Data")
	bool GetAllLootContainerContents(TArray<FTunaSweeperLootContainerContents>& OutContents);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Item Data")
	FString BuildItemIconObjectPath(const FTunaSweeperItemDefinition& ItemDefinition) const;

private:
	bool EnsureItemDataLoaded();
	bool LoadItemTableJson();
	bool LoadItemNameStringsCsv();
	bool LoadLootContainerTableJson();
	bool LoadLootContainerContentsJson();
	void ResetLoadedItemData();
	FString GetItemTableJsonPath() const;
	FString GetItemNameStringsCsvPath() const;
	FString GetLootContainerTableJsonPath() const;
	FString GetLootContainerContentsJsonPath() const;

	UPROPERTY(Transient)
	TMap<int32, FTunaSweeperItemDefinition> ItemDefinitionsById;

	UPROPERTY(Transient)
	TMap<FName, FTunaSweeperItemNameString> ItemNameStringsByKey;

	UPROPERTY(Transient)
	TMap<int32, FTunaSweeperLootContainerDefinition> LootContainerDefinitionsById;

	UPROPERTY(Transient)
	TMap<int32, FTunaSweeperLootContainerContents> LootContainerContentsById;

	bool bItemDataLoaded = false;
};
