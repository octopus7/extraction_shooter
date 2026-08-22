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

UENUM(BlueprintType)
enum class ETunaSweeperWorkbenchMode : uint8
{
	Craft UMETA(DisplayName = "Craft"),
	Dismantle UMETA(DisplayName = "Dismantle"),
	BlueprintRegister UMETA(DisplayName = "Blueprint Register")
};

UENUM(BlueprintType)
enum class ETunaSweeperItemGrade : uint8
{
	Common UMETA(DisplayName = "Common"),
	Uncommon UMETA(DisplayName = "Uncommon"),
	Rare UMETA(DisplayName = "Rare"),
	Epic UMETA(DisplayName = "Epic"),
	Legendary UMETA(DisplayName = "Legendary")
};

UENUM(BlueprintType)
enum class ETunaSweeperWeaponFireMode : uint8
{
	NotApplicable UMETA(DisplayName = "Not Applicable"),
	SemiAutomatic UMETA(DisplayName = "Semi Automatic"),
	Automatic UMETA(DisplayName = "Automatic")
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
	int32 ExperienceValue = 0;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Item")
	float WeightKg = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Item")
	ETunaSweeperItemGrade ItemGrade = ETunaSweeperItemGrade::Common;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Item")
	FString IconFileName;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Item")
	FName CategoryTag;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Item")
	FName MaxStackCategoryKey;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Item")
	FName BlueprintRecipeId;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Item")
	FName EquipmentSlotTag;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Item")
	int32 DefenseValue = 0;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Item")
	FName WeaponTypeTag;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Item")
	ETunaSweeperWeaponFireMode FireMode = ETunaSweeperWeaponFireMode::NotApplicable;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Item")
	FName AttachmentSlotTag;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Item")
	FName AmmoTypeTag;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Item")
	FName ImpactProfileId;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Item")
	FName ProjectileHitEffectId;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Item", meta = (ClampMin = "0", UIMin = "0"))
	int32 ProjectileDamageMultiplier = 10000;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Item")
	int32 ProjectileDamageBonus = 0;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Item")
	TArray<FName> AttachmentSlotTags;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Item")
	TArray<FName> CompatibleWeaponTypeTags;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Item")
	TArray<FName> CompatibleAmmoTypeTags;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Item")
	int32 MagazineCapacity = 0;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Item")
	int32 MagazineCapacityBonus = 0;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Item")
	float ReloadSeconds = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Item")
	int32 InventorySlotCapacity = 0;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Item")
	float CarryStrengthBonus = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Item")
	float HeadphoneHearingRange = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Item")
	float HeadphoneSensitivity = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Item")
	float HeadphoneMinStrength = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Item")
	float UseHealthDelta = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Item")
	float UseFoodDelta = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Item")
	float UseHydrationDelta = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Item", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float UseSeconds = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Item")
	TArray<FName> ClearsDebuffIds;
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

struct TUNASWEEPER_API FTunaSweeperLootContainerItemQuantity
{
	int32 ItemId = INDEX_NONE;

	int32 QuantityMin = 1;

	int32 QuantityMax = 1;

	float DropChance = 1.0f;
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
	FString MaterialPath;

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

	TArray<FTunaSweeperLootContainerItemQuantity> ItemQuantities;
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

USTRUCT(BlueprintType)
struct TUNASWEEPER_API FTunaSweeperShopItemDefinition
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Shop")
	int32 ItemId = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Shop", meta = (ClampMin = "0", UIMin = "0"))
	int32 StockQuantity = 0;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Shop")
	int32 PriceOverride = INDEX_NONE;
};

USTRUCT(BlueprintType)
struct TUNASWEEPER_API FTunaSweeperShopDefinition
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Shop")
	int32 ShopId = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Shop")
	FName NameStringKey;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Shop")
	TArray<FTunaSweeperShopItemDefinition> Items;
};

USTRUCT(BlueprintType)
struct TUNASWEEPER_API FTunaSweeperShopItemView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Shop")
	int32 ShopId = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Shop")
	int32 SlotIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Shop")
	int32 ItemId = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Shop", meta = (ClampMin = "0", UIMin = "0"))
	int32 StockQuantity = 0;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Shop", meta = (ClampMin = "0", UIMin = "0"))
	int32 TotalStockQuantity = 0;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Shop", meta = (ClampMin = "0", UIMin = "0"))
	int32 Price = 0;
};

USTRUCT(BlueprintType)
struct TUNASWEEPER_API FTunaSweeperWorkbenchIngredient
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Workbench")
	int32 ItemId = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Workbench", meta = (ClampMin = "1", UIMin = "1"))
	int32 Quantity = 1;
};

USTRUCT(BlueprintType)
struct TUNASWEEPER_API FTunaSweeperWorkbenchRecipeDefinition
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Workbench")
	FName RecipeId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Workbench", meta = (ClampMin = "1", UIMin = "1"))
	int32 WorkbenchId = 1;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Workbench")
	FName NameStringKey = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Workbench")
	int32 OutputItemId = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Workbench", meta = (ClampMin = "1", UIMin = "1"))
	int32 OutputQuantity = 1;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Workbench")
	TArray<FTunaSweeperWorkbenchIngredient> Ingredients;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Workbench")
	bool bAutoUnlocked = true;
};

USTRUCT(BlueprintType)
struct TUNASWEEPER_API FTunaSweeperWorkbenchIngredientView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Workbench")
	int32 ItemId = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Workbench", meta = (ClampMin = "1", UIMin = "1"))
	int32 RequiredQuantity = 1;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Workbench", meta = (ClampMin = "0", UIMin = "0"))
	int32 AvailableQuantity = 0;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Workbench", meta = (ClampMin = "0", UIMin = "0"))
	int32 MissingQuantity = 0;
};

USTRUCT(BlueprintType)
struct TUNASWEEPER_API FTunaSweeperWorkbenchRecipeView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Workbench")
	FName RecipeId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Workbench", meta = (ClampMin = "1", UIMin = "1"))
	int32 WorkbenchId = 1;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Workbench")
	int32 SlotIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Workbench")
	int32 OutputItemId = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Workbench", meta = (ClampMin = "1", UIMin = "1"))
	int32 OutputQuantity = 1;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Workbench")
	TArray<FTunaSweeperWorkbenchIngredientView> Ingredients;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Workbench", meta = (ClampMin = "0", UIMin = "0"))
	int32 MissingIngredientCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Workbench")
	bool bCanCraft = false;
};

USTRUCT(BlueprintType)
struct TUNASWEEPER_API FTunaSweeperWorkbenchDismantleDefinition
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Workbench")
	int32 SourceItemId = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Workbench")
	TArray<FTunaSweeperItemStack> Results;
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

	bool TryGetWeaponActorClassPath(int32 ItemId, FSoftObjectPath& OutWeaponClassPath);

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

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Shop")
	bool TryGetShopDefinition(int32 ShopId, FTunaSweeperShopDefinition& OutDefinition);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Shop")
	bool TryGetShopItemDefinition(
		int32 ShopId,
		int32 SlotIndex,
		FTunaSweeperShopItemDefinition& OutItemDefinition);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Shop")
	bool GetAllShopDefinitions(TArray<FTunaSweeperShopDefinition>& OutDefinitions);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Shop")
	int32 ResolveShopItemBuyPrice(const FTunaSweeperShopItemDefinition& ShopItemDefinition) const;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Item Data")
	int32 ResolveItemMaxStackQuantity(const FTunaSweeperItemDefinition& ItemDefinition) const;

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Workbench")
	bool TryGetWorkbenchRecipeDefinition(FName RecipeId, FTunaSweeperWorkbenchRecipeDefinition& OutDefinition);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Workbench")
	bool GetWorkbenchRecipeDefinitions(int32 WorkbenchId, TArray<FTunaSweeperWorkbenchRecipeDefinition>& OutDefinitions);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Workbench")
	bool GetAllWorkbenchRecipeDefinitions(TArray<FTunaSweeperWorkbenchRecipeDefinition>& OutDefinitions);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Workbench")
	bool TryGetWorkbenchDismantleDefinition(
		int32 SourceItemId,
		FTunaSweeperWorkbenchDismantleDefinition& OutDefinition);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Workbench")
	bool GetAllWorkbenchDismantleDefinitions(TArray<FTunaSweeperWorkbenchDismantleDefinition>& OutDefinitions);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Item Data")
	FString BuildItemIconObjectPath(const FTunaSweeperItemDefinition& ItemDefinition) const;

private:
	bool EnsureItemDataLoaded();
	bool LoadItemTableJson();
	bool LoadWeaponActorClassMappingsJson();
	bool LoadItemStackDefinitionsJson();
	bool LoadItemNameStringsCsv();
	bool LoadLootContainerTableJson();
	bool LoadLootContainerContentsJson();
	bool LoadShopDefinitionsJson();
	bool LoadWorkbenchRecipesJson();
	bool LoadWorkbenchDismantleRecipesJson();
	void ResetLoadedItemData();
	FString GetItemTableJsonPath() const;
	FString GetWeaponActorClassMappingsJsonPath() const;
	FString GetItemStackDefinitionsJsonPath() const;
	FString GetItemNameStringsCsvPath() const;
	FString GetLootContainerTableJsonPath() const;
	FString GetLootContainerContentsJsonPath() const;
	FString GetShopDefinitionsJsonPath() const;
	FString GetWorkbenchRecipesJsonPath() const;
	FString GetWorkbenchDismantleRecipesJsonPath() const;

	UPROPERTY(Transient)
	TMap<int32, FTunaSweeperItemDefinition> ItemDefinitionsById;

	UPROPERTY(Transient)
	TMap<int32, FSoftObjectPath> WeaponActorClassPathsByItemId;

	UPROPERTY(Transient)
	TMap<FName, int32> MaxStackQuantitiesByCategoryKey;

	UPROPERTY(Transient)
	TMap<FName, FTunaSweeperItemNameString> ItemNameStringsByKey;

	UPROPERTY(Transient)
	TMap<int32, FTunaSweeperLootContainerDefinition> LootContainerDefinitionsById;

	UPROPERTY(Transient)
	TMap<int32, FTunaSweeperLootContainerContents> LootContainerContentsById;

	UPROPERTY(Transient)
	TMap<int32, FTunaSweeperShopDefinition> ShopDefinitionsById;

	UPROPERTY(Transient)
	TMap<FName, FTunaSweeperWorkbenchRecipeDefinition> WorkbenchRecipeDefinitionsById;

	UPROPERTY(Transient)
	TArray<FName> WorkbenchRecipeIdsInLoadOrder;

	UPROPERTY(Transient)
	TMap<int32, FTunaSweeperWorkbenchDismantleDefinition> WorkbenchDismantleDefinitionsByItemId;

	UPROPERTY(Transient)
	TArray<int32> WorkbenchDismantleItemIdsInLoadOrder;

	bool bItemDataLoaded = false;
};
