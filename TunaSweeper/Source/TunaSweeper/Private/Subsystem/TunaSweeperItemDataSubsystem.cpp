#include "Subsystem/TunaSweeperItemDataSubsystem.h"

#include "Dom/JsonObject.h"
#include "Game/TunaSweeperDataValueTypes.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/Csv/CsvParser.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

DEFINE_LOG_CATEGORY_STATIC(LogTunaSweeperItemData, Log, All);

namespace TunaSweeperItemDataFiles
{
	const TCHAR* ItemTableJsonRelativePath = TEXT("Data/ItemTable.json");
	const TCHAR* ItemStackDefinitionsJsonRelativePath = TEXT("Data/ItemStackDefinitions.json");
	const TCHAR* ItemNameStringsCsvRelativePath = TEXT("Data/ItemNameStrings.csv");
	const TCHAR* LootContainerTableJsonRelativePath = TEXT("Data/LootContainerTable.json");
	const TCHAR* LootContainerContentsJsonRelativePath = TEXT("Data/LootContainerContents.json");
	const TCHAR* ShopDefinitionsJsonRelativePath = TEXT("Data/ShopDefinitions.json");
	const TCHAR* WorkbenchRecipesJsonRelativePath = TEXT("Data/WorkbenchRecipes.json");
	const TCHAR* WorkbenchDismantleRecipesJsonRelativePath = TEXT("Data/WorkbenchDismantleRecipes.json");

	FString GetCsvCell(const TArray<const TCHAR*>& Row, int32 CellIndex)
	{
		return Row.IsValidIndex(CellIndex)
			? FString(Row[CellIndex]).TrimStartAndEnd()
			: FString();
	}

	void BuildLootItemQuantityEntry(
		int32 ItemId,
		int32 QuantityMin,
		int32 QuantityMax,
		float DropChance,
		FTunaSweeperLootContainerItemQuantity& OutEntry)
	{
		OutEntry.ItemId = ItemId;
		OutEntry.QuantityMin = FMath::Max(1, FMath::Min(QuantityMin, QuantityMax));
		OutEntry.QuantityMax = FMath::Max(OutEntry.QuantityMin, FMath::Max(QuantityMin, QuantityMax));
		OutEntry.DropChance = FMath::Clamp(DropChance, 0.0f, 1.0f);
	}

	float NormalizeDropChanceValue(float RawChance)
	{
		if (RawChance > 1.0f)
		{
			return TunaSweeperDataValues::ToRatioFloat(FMath::RoundToInt(RawChance));
		}

		return FMath::Clamp(RawChance, 0.0f, 1.0f);
	}

	ETunaSweeperItemGrade ResolveItemGradeFromString(const FString& GradeString)
	{
		FString NormalizedGrade = GradeString.TrimStartAndEnd().ToLower();
		NormalizedGrade.ReplaceInline(TEXT("-"), TEXT("_"));
		NormalizedGrade.ReplaceInline(TEXT(" "), TEXT("_"));

		if (NormalizedGrade == TEXT("uncommon") || NormalizedGrade == TEXT("green"))
		{
			return ETunaSweeperItemGrade::Uncommon;
		}
		if (NormalizedGrade == TEXT("rare") || NormalizedGrade == TEXT("blue"))
		{
			return ETunaSweeperItemGrade::Rare;
		}
		if (NormalizedGrade == TEXT("epic") || NormalizedGrade == TEXT("purple"))
		{
			return ETunaSweeperItemGrade::Epic;
		}
		if (NormalizedGrade == TEXT("legendary") || NormalizedGrade == TEXT("orange") || NormalizedGrade == TEXT("gold"))
		{
			return ETunaSweeperItemGrade::Legendary;
		}

		return ETunaSweeperItemGrade::Common;
	}

	ETunaSweeperWeaponFireMode ResolveWeaponFireModeFromString(const FString& FireModeString)
	{
		FString NormalizedFireMode = FireModeString.TrimStartAndEnd().ToLower();
		NormalizedFireMode.ReplaceInline(TEXT("-"), TEXT("_"));
		NormalizedFireMode.ReplaceInline(TEXT(" "), TEXT("_"));

		if (NormalizedFireMode == TEXT("semi_automatic") || NormalizedFireMode == TEXT("semi_auto"))
		{
			return ETunaSweeperWeaponFireMode::SemiAutomatic;
		}
		if (NormalizedFireMode == TEXT("automatic") || NormalizedFireMode == TEXT("auto"))
		{
			return ETunaSweeperWeaponFireMode::Automatic;
		}

		return ETunaSweeperWeaponFireMode::NotApplicable;
	}

	FName ResolveDefaultMaxStackCategoryKey(FName ItemCategoryTag)
	{
		static const TMap<FName, FName> DefaultStackCategoryKeysByItemCategory =
		{
			{ FName(TEXT("item.category.weapon.gun")), FName(TEXT("stack.default.weapon")) },
			{ FName(TEXT("item.category.weapon.melee")), FName(TEXT("stack.default.weapon")) },
			{ FName(TEXT("item.category.ammo")), FName(TEXT("stack.default.ammo")) },
			{ FName(TEXT("item.category.attachment")), FName(TEXT("stack.default.attachment")) },
			{ FName(TEXT("item.category.consumable")), FName(TEXT("stack.default.consumable")) },
			{ FName(TEXT("item.category.body")), FName(TEXT("stack.default.equipment")) },
			{ FName(TEXT("item.category.bag")), FName(TEXT("stack.default.equipment")) },
			{ FName(TEXT("item.category.head")), FName(TEXT("stack.default.equipment")) },
			{ FName(TEXT("item.category.face")), FName(TEXT("stack.default.equipment")) },
			{ FName(TEXT("item.category.ear")), FName(TEXT("stack.default.equipment")) },
			{ FName(TEXT("item.category.material")), FName(TEXT("stack.default.material")) },
			{ FName(TEXT("item.category.blueprint")), FName(TEXT("stack.default.blueprint")) },
			{ FName(TEXT("item.category.currency")), FName(TEXT("stack.default.currency")) }
		};

		if (const FName* StackCategoryKey = DefaultStackCategoryKeysByItemCategory.Find(ItemCategoryTag))
		{
			return *StackCategoryKey;
		}

		return NAME_None;
	}

	TArray<FTunaSweeperItemStack> ResolveLootContainerItems(const FTunaSweeperLootContainerContents& Contents)
	{
		TArray<FTunaSweeperItemStack> ResolvedItems;
		if (Contents.ItemQuantities.Num() > 0)
		{
			ResolvedItems.Reserve(Contents.ItemQuantities.Num());
			for (const FTunaSweeperLootContainerItemQuantity& ItemQuantity : Contents.ItemQuantities)
			{
				if (ItemQuantity.ItemId == INDEX_NONE)
				{
					continue;
				}
				if (ItemQuantity.DropChance <= 0.0f ||
					(ItemQuantity.DropChance < 1.0f && FMath::FRand() > ItemQuantity.DropChance))
				{
					continue;
				}

				FTunaSweeperItemStack ItemStack;
				ItemStack.ItemId = ItemQuantity.ItemId;
				ItemStack.Quantity = ItemQuantity.QuantityMin == ItemQuantity.QuantityMax
					? ItemQuantity.QuantityMin
					: FMath::RandRange(ItemQuantity.QuantityMin, ItemQuantity.QuantityMax);
				ResolvedItems.Add(ItemStack);
			}

			return ResolvedItems;
		}

		return Contents.Items;
	}
}

bool UTunaSweeperItemDataSubsystem::LoadItemData(bool bForceReload)
{
	if (bItemDataLoaded && !bForceReload)
	{
		return true;
	}

	ResetLoadedItemData();

	const bool bLoadedItemTable = LoadItemTableJson();
	const bool bLoadedItemStackDefinitions = LoadItemStackDefinitionsJson();
	const bool bLoadedNameStrings = LoadItemNameStringsCsv();
	const bool bLoadedLootContainerTable = LoadLootContainerTableJson();
	const bool bLoadedLootContainerContents = LoadLootContainerContentsJson();
	const bool bLoadedShopDefinitions = LoadShopDefinitionsJson();
	const bool bLoadedWorkbenchRecipes = LoadWorkbenchRecipesJson();
	const bool bLoadedWorkbenchDismantleRecipes = LoadWorkbenchDismantleRecipesJson();
	bItemDataLoaded =
		bLoadedItemTable &&
		bLoadedItemStackDefinitions &&
		bLoadedNameStrings &&
		bLoadedLootContainerTable &&
		bLoadedLootContainerContents &&
		bLoadedShopDefinitions &&
		bLoadedWorkbenchRecipes &&
		bLoadedWorkbenchDismantleRecipes;

	if (!bItemDataLoaded)
	{
		ResetLoadedItemData();
	}

	return bItemDataLoaded;
}

bool UTunaSweeperItemDataSubsystem::TryGetItemDefinition(int32 ItemId, FTunaSweeperItemDefinition& OutItemDefinition)
{
	if (!EnsureItemDataLoaded())
	{
		OutItemDefinition = FTunaSweeperItemDefinition();
		return false;
	}

	if (const FTunaSweeperItemDefinition* FoundItemDefinition = ItemDefinitionsById.Find(ItemId))
	{
		OutItemDefinition = *FoundItemDefinition;
		return true;
	}

	OutItemDefinition = FTunaSweeperItemDefinition();
	return false;
}

bool UTunaSweeperItemDataSubsystem::TryGetItemNameString(FName NameStringKey, FTunaSweeperItemNameString& OutNameString)
{
	return TryGetItemString(NameStringKey, OutNameString);
}

bool UTunaSweeperItemDataSubsystem::TryGetItemString(FName StringKey, FTunaSweeperItemNameString& OutItemString)
{
	if (!EnsureItemDataLoaded())
	{
		OutItemString = FTunaSweeperItemNameString();
		return false;
	}

	if (const FTunaSweeperItemNameString* FoundNameString = ItemNameStringsByKey.Find(StringKey))
	{
		OutItemString = *FoundNameString;
		return true;
	}

	OutItemString = FTunaSweeperItemNameString();
	return false;
}

bool UTunaSweeperItemDataSubsystem::TryGetItemNameTextByKey(
	FName NameStringKey,
	ETunaSweeperItemTextLanguage Language,
	FText& OutText)
{
	return TryGetItemTextByKey(NameStringKey, Language, OutText);
}

bool UTunaSweeperItemDataSubsystem::TryGetItemTextByKey(
	FName StringKey,
	ETunaSweeperItemTextLanguage Language,
	FText& OutText)
{
	FTunaSweeperItemNameString NameString;
	if (!TryGetItemString(StringKey, NameString))
	{
		OutText = FText::GetEmpty();
		return false;
	}

	switch (Language)
	{
	case ETunaSweeperItemTextLanguage::Korean:
		OutText = NameString.Korean;
		break;
	case ETunaSweeperItemTextLanguage::English:
		OutText = NameString.English;
		break;
	case ETunaSweeperItemTextLanguage::Japanese:
		OutText = NameString.Japanese;
		break;
	default:
		OutText = FText::GetEmpty();
		break;
	}

	return !OutText.IsEmpty();
}

bool UTunaSweeperItemDataSubsystem::TryGetItemNameText(
	int32 ItemId,
	ETunaSweeperItemTextLanguage Language,
	FText& OutText)
{
	FTunaSweeperItemDefinition ItemDefinition;
	if (!TryGetItemDefinition(ItemId, ItemDefinition))
	{
		OutText = FText::GetEmpty();
		return false;
	}

	return TryGetItemNameTextByKey(ItemDefinition.NameStringKey, Language, OutText);
}

bool UTunaSweeperItemDataSubsystem::TryGetItemDescriptionText(
	int32 ItemId,
	ETunaSweeperItemTextLanguage Language,
	FText& OutText)
{
	FTunaSweeperItemDefinition ItemDefinition;
	if (!TryGetItemDefinition(ItemId, ItemDefinition))
	{
		OutText = FText::GetEmpty();
		return false;
	}

	return TryGetItemTextByKey(ItemDefinition.DescriptionStringKey, Language, OutText);
}

bool UTunaSweeperItemDataSubsystem::GetAllItemDefinitions(TArray<FTunaSweeperItemDefinition>& OutItemDefinitions)
{
	if (!EnsureItemDataLoaded())
	{
		OutItemDefinitions.Reset();
		return false;
	}

	ItemDefinitionsById.GenerateValueArray(OutItemDefinitions);
	OutItemDefinitions.Sort(
		[](const FTunaSweeperItemDefinition& Left, const FTunaSweeperItemDefinition& Right)
		{
			return Left.Id < Right.Id;
		});
	return true;
}

bool UTunaSweeperItemDataSubsystem::TryGetLootContainerDefinition(
	int32 ContainerDefinitionId,
	FTunaSweeperLootContainerDefinition& OutDefinition)
{
	if (!EnsureItemDataLoaded())
	{
		OutDefinition = FTunaSweeperLootContainerDefinition();
		return false;
	}

	if (const FTunaSweeperLootContainerDefinition* FoundDefinition = LootContainerDefinitionsById.Find(ContainerDefinitionId))
	{
		OutDefinition = *FoundDefinition;
		return true;
	}

	OutDefinition = FTunaSweeperLootContainerDefinition();
	return false;
}

bool UTunaSweeperItemDataSubsystem::TryGetLootContainerContents(
	int32 ContentsId,
	FTunaSweeperLootContainerContents& OutContents)
{
	if (!EnsureItemDataLoaded())
	{
		OutContents = FTunaSweeperLootContainerContents();
		return false;
	}

	if (const FTunaSweeperLootContainerContents* FoundContents = LootContainerContentsById.Find(ContentsId))
	{
		OutContents = *FoundContents;
		return true;
	}

	OutContents = FTunaSweeperLootContainerContents();
	return false;
}

bool UTunaSweeperItemDataSubsystem::TryBuildLootContainerInstance(
	int32 ContainerDefinitionId,
	int32 ContentsId,
	ETunaSweeperItemTextLanguage Language,
	FTunaSweeperLootContainerInstance& OutInstance)
{
	FTunaSweeperLootContainerDefinition ContainerDefinition;
	FTunaSweeperLootContainerContents Contents;
	if (!TryGetLootContainerDefinition(ContainerDefinitionId, ContainerDefinition) ||
		!TryGetLootContainerContents(ContentsId, Contents))
	{
		OutInstance = FTunaSweeperLootContainerInstance();
		return false;
	}

	TArray<FTunaSweeperItemStack> ResolvedItems = TunaSweeperItemDataFiles::ResolveLootContainerItems(Contents);
	if (ResolvedItems.Num() > ContainerDefinition.Capacity)
	{
		UE_LOG(
			LogTunaSweeperItemData,
			Warning,
			TEXT("Loot contents %d has %d stacks, exceeding container %d capacity %d."),
			Contents.Id,
			ResolvedItems.Num(),
			ContainerDefinition.Id,
			ContainerDefinition.Capacity);
		OutInstance = FTunaSweeperLootContainerInstance();
		return false;
	}

	FText DisplayName;
	if (!TryGetItemTextByKey(ContainerDefinition.NameStringKey, Language, DisplayName))
	{
		DisplayName = FText::FromString(FString::Printf(TEXT("Container %d"), ContainerDefinition.Id));
	}

	OutInstance.ContainerDefinitionId = ContainerDefinition.Id;
	OutInstance.ContentsId = Contents.Id;
	OutInstance.DisplayName = DisplayName;
	OutInstance.Capacity = ContainerDefinition.Capacity;
	OutInstance.Items = MoveTemp(ResolvedItems);
	return true;
}

bool UTunaSweeperItemDataSubsystem::GetAllLootContainerDefinitions(TArray<FTunaSweeperLootContainerDefinition>& OutDefinitions)
{
	if (!EnsureItemDataLoaded())
	{
		OutDefinitions.Reset();
		return false;
	}

	LootContainerDefinitionsById.GenerateValueArray(OutDefinitions);
	OutDefinitions.Sort(
		[](const FTunaSweeperLootContainerDefinition& Left, const FTunaSweeperLootContainerDefinition& Right)
		{
			return Left.Id < Right.Id;
		});
	return true;
}

bool UTunaSweeperItemDataSubsystem::GetAllLootContainerContents(TArray<FTunaSweeperLootContainerContents>& OutContents)
{
	if (!EnsureItemDataLoaded())
	{
		OutContents.Reset();
		return false;
	}

	LootContainerContentsById.GenerateValueArray(OutContents);
	OutContents.Sort(
		[](const FTunaSweeperLootContainerContents& Left, const FTunaSweeperLootContainerContents& Right)
		{
			return Left.Id < Right.Id;
	});
	return true;
}

bool UTunaSweeperItemDataSubsystem::TryGetShopDefinition(int32 ShopId, FTunaSweeperShopDefinition& OutDefinition)
{
	if (!EnsureItemDataLoaded())
	{
		OutDefinition = FTunaSweeperShopDefinition();
		return false;
	}

	if (const FTunaSweeperShopDefinition* FoundDefinition = ShopDefinitionsById.Find(ShopId))
	{
		OutDefinition = *FoundDefinition;
		return true;
	}

	OutDefinition = FTunaSweeperShopDefinition();
	return false;
}

bool UTunaSweeperItemDataSubsystem::TryGetShopItemDefinition(
	int32 ShopId,
	int32 SlotIndex,
	FTunaSweeperShopItemDefinition& OutItemDefinition)
{
	FTunaSweeperShopDefinition ShopDefinition;
	if (!TryGetShopDefinition(ShopId, ShopDefinition) || !ShopDefinition.Items.IsValidIndex(SlotIndex))
	{
		OutItemDefinition = FTunaSweeperShopItemDefinition();
		return false;
	}

	OutItemDefinition = ShopDefinition.Items[SlotIndex];
	return true;
}

bool UTunaSweeperItemDataSubsystem::GetAllShopDefinitions(TArray<FTunaSweeperShopDefinition>& OutDefinitions)
{
	if (!EnsureItemDataLoaded())
	{
		OutDefinitions.Reset();
		return false;
	}

	ShopDefinitionsById.GenerateValueArray(OutDefinitions);
	OutDefinitions.Sort(
		[](const FTunaSweeperShopDefinition& Left, const FTunaSweeperShopDefinition& Right)
		{
			return Left.ShopId < Right.ShopId;
		});
	return true;
}

int32 UTunaSweeperItemDataSubsystem::ResolveShopItemBuyPrice(
	const FTunaSweeperShopItemDefinition& ShopItemDefinition) const
{
	if (ShopItemDefinition.PriceOverride >= 0)
	{
		return ShopItemDefinition.PriceOverride;
	}

	if (const FTunaSweeperItemDefinition* ItemDefinition = ItemDefinitionsById.Find(ShopItemDefinition.ItemId))
	{
		return FMath::Max(0, ItemDefinition->ShopSellPrice);
	}

	return 0;
}

int32 UTunaSweeperItemDataSubsystem::ResolveItemMaxStackQuantity(
	const FTunaSweeperItemDefinition& ItemDefinition) const
{
	FName StackCategoryKey = ItemDefinition.MaxStackCategoryKey;
	if (StackCategoryKey.IsNone())
	{
		StackCategoryKey = TunaSweeperItemDataFiles::ResolveDefaultMaxStackCategoryKey(ItemDefinition.CategoryTag);
	}

	if (StackCategoryKey.IsNone())
	{
		return 1;
	}

	if (const int32* MaxStackQuantity = MaxStackQuantitiesByCategoryKey.Find(StackCategoryKey))
	{
		return FMath::Max(1, *MaxStackQuantity);
	}

	return 1;
}

bool UTunaSweeperItemDataSubsystem::TryGetWorkbenchRecipeDefinition(
	FName RecipeId,
	FTunaSweeperWorkbenchRecipeDefinition& OutDefinition)
{
	if (!EnsureItemDataLoaded())
	{
		OutDefinition = FTunaSweeperWorkbenchRecipeDefinition();
		return false;
	}

	if (const FTunaSweeperWorkbenchRecipeDefinition* FoundDefinition = WorkbenchRecipeDefinitionsById.Find(RecipeId))
	{
		OutDefinition = *FoundDefinition;
		return true;
	}

	OutDefinition = FTunaSweeperWorkbenchRecipeDefinition();
	return false;
}

bool UTunaSweeperItemDataSubsystem::GetWorkbenchRecipeDefinitions(
	int32 WorkbenchId,
	TArray<FTunaSweeperWorkbenchRecipeDefinition>& OutDefinitions)
{
	if (!EnsureItemDataLoaded())
	{
		OutDefinitions.Reset();
		return false;
	}

	OutDefinitions.Reset();
	const int32 SanitizedWorkbenchId = FMath::Max(1, WorkbenchId);
	for (const FName& RecipeId : WorkbenchRecipeIdsInLoadOrder)
	{
		const FTunaSweeperWorkbenchRecipeDefinition* Definition = WorkbenchRecipeDefinitionsById.Find(RecipeId);
		if (Definition && Definition->WorkbenchId == SanitizedWorkbenchId)
		{
			OutDefinitions.Add(*Definition);
		}
	}

	return OutDefinitions.Num() > 0;
}

bool UTunaSweeperItemDataSubsystem::GetAllWorkbenchRecipeDefinitions(
	TArray<FTunaSweeperWorkbenchRecipeDefinition>& OutDefinitions)
{
	if (!EnsureItemDataLoaded())
	{
		OutDefinitions.Reset();
		return false;
	}

	OutDefinitions.Reset();
	for (const FName& RecipeId : WorkbenchRecipeIdsInLoadOrder)
	{
		if (const FTunaSweeperWorkbenchRecipeDefinition* Definition = WorkbenchRecipeDefinitionsById.Find(RecipeId))
		{
			OutDefinitions.Add(*Definition);
		}
	}

	return OutDefinitions.Num() > 0;
}

bool UTunaSweeperItemDataSubsystem::TryGetWorkbenchDismantleDefinition(
	int32 SourceItemId,
	FTunaSweeperWorkbenchDismantleDefinition& OutDefinition)
{
	if (!EnsureItemDataLoaded())
	{
		OutDefinition = FTunaSweeperWorkbenchDismantleDefinition();
		return false;
	}

	if (const FTunaSweeperWorkbenchDismantleDefinition* FoundDefinition = WorkbenchDismantleDefinitionsByItemId.Find(SourceItemId))
	{
		OutDefinition = *FoundDefinition;
		return true;
	}

	OutDefinition = FTunaSweeperWorkbenchDismantleDefinition();
	return false;
}

bool UTunaSweeperItemDataSubsystem::GetAllWorkbenchDismantleDefinitions(
	TArray<FTunaSweeperWorkbenchDismantleDefinition>& OutDefinitions)
{
	if (!EnsureItemDataLoaded())
	{
		OutDefinitions.Reset();
		return false;
	}

	OutDefinitions.Reset();
	for (const int32 SourceItemId : WorkbenchDismantleItemIdsInLoadOrder)
	{
		if (const FTunaSweeperWorkbenchDismantleDefinition* Definition = WorkbenchDismantleDefinitionsByItemId.Find(SourceItemId))
		{
			OutDefinitions.Add(*Definition);
		}
	}

	return OutDefinitions.Num() > 0;
}

FString UTunaSweeperItemDataSubsystem::BuildItemIconObjectPath(const FTunaSweeperItemDefinition& ItemDefinition) const
{
	const FString IconAssetName = FPaths::GetBaseFilename(ItemDefinition.IconFileName);
	if (IconAssetName.IsEmpty())
	{
		return FString();
	}

	return FString::Printf(
		TEXT("/Game/UI/Icons/%s.%s"),
		*IconAssetName,
		*IconAssetName);
}

bool UTunaSweeperItemDataSubsystem::EnsureItemDataLoaded()
{
	return bItemDataLoaded || LoadItemData(false);
}

bool UTunaSweeperItemDataSubsystem::LoadItemTableJson()
{
	FString JsonContent;
	const FString ItemTableJsonPath = GetItemTableJsonPath();
	if (!FFileHelper::LoadFileToString(JsonContent, *ItemTableJsonPath))
	{
		UE_LOG(LogTunaSweeperItemData, Error, TEXT("Failed to read item table JSON: %s"), *ItemTableJsonPath);
		return false;
	}

	TArray<TSharedPtr<FJsonValue>> JsonRows;
	const TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(JsonContent);
	if (!FJsonSerializer::Deserialize(JsonReader, JsonRows))
	{
		UE_LOG(LogTunaSweeperItemData, Error, TEXT("Failed to parse item table JSON: %s"), *ItemTableJsonPath);
		return false;
	}

	bool bHasValidRows = false;
	for (int32 RowIndex = 0; RowIndex < JsonRows.Num(); ++RowIndex)
	{
		const TSharedPtr<FJsonObject>* JsonObject = nullptr;
		if (!JsonRows[RowIndex].IsValid() || !JsonRows[RowIndex]->TryGetObject(JsonObject) || !JsonObject || !JsonObject->IsValid())
		{
			UE_LOG(LogTunaSweeperItemData, Warning, TEXT("Skipping item table row %d: row is not an object."), RowIndex);
			continue;
		}

		double NumericId = 0.0;
		double NumericShopSellPrice = 0.0;
		double NumericExperienceValue = 0.0;
		double NumericWeightKg = 0.0;
		double NumericInventorySlotCapacity = 0.0;
		double NumericCarryStrengthBonus = 0.0;
		double NumericHeadphoneHearingRange = 0.0;
		double NumericHeadphoneSensitivity = 0.0;
		double NumericHeadphoneMinStrength = 0.0;
		double NumericMagazineCapacity = 0.0;
		double NumericMagazineCapacityBonus = 0.0;
		double NumericReloadSeconds = 0.0;
		double NumericProjectileDamageMultiplier = TunaSweeperDataValues::RatioIdentity;
		double NumericProjectileDamageBonus = 0.0;
		double NumericDefenseValue = 0.0;
		double NumericUseHealthDelta = 0.0;
		double NumericUseFoodDelta = 0.0;
		double NumericUseHydrationDelta = 0.0;
		double NumericUseSeconds = 0.0;
		FString NameStringKey;
		FString DescriptionStringKey;
		FString IconFileName;
		FString ItemGradeString;
		FString CategoryTag;
		FString MaxStackCategoryKey;
		FString BlueprintRecipeId;
		FString EquipmentSlotTag;
		FString WeaponTypeTag;
		FString FireModeString;
		FString AttachmentSlotTag;
		FString AmmoTypeTag;
		FString ImpactProfileId;
		FString ProjectileHitEffectId;
		if (!(*JsonObject)->TryGetNumberField(TEXT("id"), NumericId) ||
			!(*JsonObject)->TryGetStringField(TEXT("name_string_key"), NameStringKey) ||
			!(*JsonObject)->TryGetStringField(TEXT("description_string_key"), DescriptionStringKey) ||
			!(*JsonObject)->TryGetNumberField(TEXT("shop_sell_price"), NumericShopSellPrice) ||
			!(*JsonObject)->TryGetStringField(TEXT("icon_file_name"), IconFileName))
		{
			UE_LOG(LogTunaSweeperItemData, Warning, TEXT("Skipping item table row %d: required field is missing."), RowIndex);
			continue;
		}

		FTunaSweeperItemDefinition ItemDefinition;
		ItemDefinition.Id = static_cast<int32>(NumericId);
		ItemDefinition.NameStringKey = FName(*NameStringKey.TrimStartAndEnd());
		ItemDefinition.DescriptionStringKey = FName(*DescriptionStringKey.TrimStartAndEnd());
		ItemDefinition.ShopSellPrice = FMath::Max(0, static_cast<int32>(NumericShopSellPrice));
		ItemDefinition.IconFileName = IconFileName.TrimStartAndEnd();
		if ((*JsonObject)->TryGetStringField(TEXT("item_grade"), ItemGradeString) ||
			(*JsonObject)->TryGetStringField(TEXT("rarity"), ItemGradeString) ||
			(*JsonObject)->TryGetStringField(TEXT("grade"), ItemGradeString))
		{
			ItemDefinition.ItemGrade = TunaSweeperItemDataFiles::ResolveItemGradeFromString(ItemGradeString);
		}
		if ((*JsonObject)->TryGetNumberField(TEXT("experience_value"), NumericExperienceValue) ||
			(*JsonObject)->TryGetNumberField(TEXT("experience_points"), NumericExperienceValue) ||
			(*JsonObject)->TryGetNumberField(TEXT("xp_value"), NumericExperienceValue))
		{
			ItemDefinition.ExperienceValue = FMath::Max(0, static_cast<int32>(NumericExperienceValue));
		}
		if ((*JsonObject)->TryGetNumberField(TEXT("weight_kg"), NumericWeightKg) ||
			(*JsonObject)->TryGetNumberField(TEXT("weight"), NumericWeightKg))
		{
			ItemDefinition.WeightKg = FMath::Max(0.0f, static_cast<float>(NumericWeightKg));
		}
		if ((*JsonObject)->TryGetStringField(TEXT("category_tag"), CategoryTag))
		{
			ItemDefinition.CategoryTag = FName(*CategoryTag.TrimStartAndEnd());
		}
		if ((*JsonObject)->TryGetStringField(TEXT("max_stack_category_key"), MaxStackCategoryKey) ||
			(*JsonObject)->TryGetStringField(TEXT("stack_category_key"), MaxStackCategoryKey) ||
			(*JsonObject)->TryGetStringField(TEXT("max_stack_key"), MaxStackCategoryKey) ||
			(*JsonObject)->TryGetStringField(TEXT("stack_class_key"), MaxStackCategoryKey))
		{
			ItemDefinition.MaxStackCategoryKey = FName(*MaxStackCategoryKey.TrimStartAndEnd());
		}
		if ((*JsonObject)->TryGetStringField(TEXT("blueprint_recipe_id"), BlueprintRecipeId) ||
			(*JsonObject)->TryGetStringField(TEXT("workbench_recipe_id"), BlueprintRecipeId) ||
			(*JsonObject)->TryGetStringField(TEXT("unlock_recipe_id"), BlueprintRecipeId))
		{
			ItemDefinition.BlueprintRecipeId = FName(*BlueprintRecipeId.TrimStartAndEnd());
		}
		if ((*JsonObject)->TryGetStringField(TEXT("equipment_slot_tag"), EquipmentSlotTag))
		{
			ItemDefinition.EquipmentSlotTag = FName(*EquipmentSlotTag.TrimStartAndEnd());
		}
		if ((*JsonObject)->TryGetNumberField(TEXT("defense_value"), NumericDefenseValue) ||
			(*JsonObject)->TryGetNumberField(TEXT("armor_defense"), NumericDefenseValue) ||
			(*JsonObject)->TryGetNumberField(TEXT("defense"), NumericDefenseValue))
		{
			ItemDefinition.DefenseValue = FMath::Max(0, FMath::RoundToInt(NumericDefenseValue));
		}
		if ((*JsonObject)->TryGetStringField(TEXT("weapon_type_tag"), WeaponTypeTag))
		{
			ItemDefinition.WeaponTypeTag = FName(*WeaponTypeTag.TrimStartAndEnd());
		}
		if ((*JsonObject)->TryGetStringField(TEXT("fire_mode"), FireModeString))
		{
			ItemDefinition.FireMode = TunaSweeperItemDataFiles::ResolveWeaponFireModeFromString(FireModeString);
		}
		if ((*JsonObject)->TryGetStringField(TEXT("attachment_slot_tag"), AttachmentSlotTag))
		{
			ItemDefinition.AttachmentSlotTag = FName(*AttachmentSlotTag.TrimStartAndEnd());
		}
		if ((*JsonObject)->TryGetStringField(TEXT("ammo_type_tag"), AmmoTypeTag))
		{
			ItemDefinition.AmmoTypeTag = FName(*AmmoTypeTag.TrimStartAndEnd());
		}
		if ((*JsonObject)->TryGetStringField(TEXT("impact_profile_id"), ImpactProfileId))
		{
			ItemDefinition.ImpactProfileId = FName(*ImpactProfileId.TrimStartAndEnd());
		}
		if ((*JsonObject)->TryGetStringField(TEXT("projectile_hit_effect_id"), ProjectileHitEffectId) ||
			(*JsonObject)->TryGetStringField(TEXT("hit_effect_id"), ProjectileHitEffectId))
		{
			ItemDefinition.ProjectileHitEffectId = FName(*ProjectileHitEffectId.TrimStartAndEnd());
		}
		if ((*JsonObject)->TryGetNumberField(TEXT("projectile_damage_multiplier"), NumericProjectileDamageMultiplier) ||
			(*JsonObject)->TryGetNumberField(TEXT("ammo_damage_multiplier"), NumericProjectileDamageMultiplier) ||
			(*JsonObject)->TryGetNumberField(TEXT("damage_multiplier"), NumericProjectileDamageMultiplier))
		{
			ItemDefinition.ProjectileDamageMultiplier =
				TunaSweeperDataValues::ClampRatioValue(FMath::RoundToInt(NumericProjectileDamageMultiplier));
		}
		if ((*JsonObject)->TryGetNumberField(TEXT("projectile_damage_bonus"), NumericProjectileDamageBonus) ||
			(*JsonObject)->TryGetNumberField(TEXT("ammo_damage_bonus"), NumericProjectileDamageBonus) ||
			(*JsonObject)->TryGetNumberField(TEXT("damage_bonus"), NumericProjectileDamageBonus))
		{
			ItemDefinition.ProjectileDamageBonus = FMath::RoundToInt(NumericProjectileDamageBonus);
		}
		const TArray<TSharedPtr<FJsonValue>>* AttachmentSlotTagsArray = nullptr;
		if ((*JsonObject)->TryGetArrayField(TEXT("attachment_slot_tags"), AttachmentSlotTagsArray) && AttachmentSlotTagsArray)
		{
			for (const TSharedPtr<FJsonValue>& AttachmentSlotTagValue : *AttachmentSlotTagsArray)
			{
				const FString AttachmentSlotTagString = AttachmentSlotTagValue.IsValid()
					? AttachmentSlotTagValue->AsString().TrimStartAndEnd()
					: FString();
				if (!AttachmentSlotTagString.IsEmpty())
				{
					ItemDefinition.AttachmentSlotTags.Add(FName(*AttachmentSlotTagString));
				}
			}
		}
		const TArray<TSharedPtr<FJsonValue>>* CompatibleWeaponTypeTagsArray = nullptr;
		if ((*JsonObject)->TryGetArrayField(TEXT("compatible_weapon_type_tags"), CompatibleWeaponTypeTagsArray) && CompatibleWeaponTypeTagsArray)
		{
			for (const TSharedPtr<FJsonValue>& WeaponTypeTagValue : *CompatibleWeaponTypeTagsArray)
			{
				const FString WeaponTypeTagString = WeaponTypeTagValue.IsValid()
					? WeaponTypeTagValue->AsString().TrimStartAndEnd()
					: FString();
				if (!WeaponTypeTagString.IsEmpty())
				{
					ItemDefinition.CompatibleWeaponTypeTags.Add(FName(*WeaponTypeTagString));
				}
			}
		}
		const TArray<TSharedPtr<FJsonValue>>* CompatibleAmmoTypeTagsArray = nullptr;
		if ((*JsonObject)->TryGetArrayField(TEXT("compatible_ammo_type_tags"), CompatibleAmmoTypeTagsArray) && CompatibleAmmoTypeTagsArray)
		{
			for (const TSharedPtr<FJsonValue>& AmmoTypeTagValue : *CompatibleAmmoTypeTagsArray)
			{
				const FString AmmoTypeTagString = AmmoTypeTagValue.IsValid()
					? AmmoTypeTagValue->AsString().TrimStartAndEnd()
					: FString();
				if (!AmmoTypeTagString.IsEmpty())
				{
					ItemDefinition.CompatibleAmmoTypeTags.Add(FName(*AmmoTypeTagString));
				}
			}
		}
		if ((*JsonObject)->TryGetNumberField(TEXT("magazine_capacity"), NumericMagazineCapacity))
		{
			ItemDefinition.MagazineCapacity = FMath::Max(0, static_cast<int32>(NumericMagazineCapacity));
		}
		if ((*JsonObject)->TryGetNumberField(TEXT("magazine_capacity_bonus"), NumericMagazineCapacityBonus))
		{
			ItemDefinition.MagazineCapacityBonus = FMath::Max(0, static_cast<int32>(NumericMagazineCapacityBonus));
		}
		if ((*JsonObject)->TryGetNumberField(TEXT("reload_seconds"), NumericReloadSeconds))
		{
			ItemDefinition.ReloadSeconds = FMath::Max(0.0f, static_cast<float>(NumericReloadSeconds));
		}
		if ((*JsonObject)->TryGetNumberField(TEXT("inventory_slot_capacity"), NumericInventorySlotCapacity))
		{
			ItemDefinition.InventorySlotCapacity = FMath::Max(0, static_cast<int32>(NumericInventorySlotCapacity));
		}
		if ((*JsonObject)->TryGetNumberField(TEXT("carry_strength_bonus"), NumericCarryStrengthBonus) ||
			(*JsonObject)->TryGetNumberField(TEXT("strength_bonus"), NumericCarryStrengthBonus))
		{
			ItemDefinition.CarryStrengthBonus = FMath::Max(0.0f, static_cast<float>(NumericCarryStrengthBonus));
		}
		if ((*JsonObject)->TryGetNumberField(TEXT("headphone_hearing_range"), NumericHeadphoneHearingRange) ||
			(*JsonObject)->TryGetNumberField(TEXT("hearing_range"), NumericHeadphoneHearingRange) ||
			(*JsonObject)->TryGetNumberField(TEXT("noise_hearing_range"), NumericHeadphoneHearingRange))
		{
			ItemDefinition.HeadphoneHearingRange = FMath::Max(0.0f, static_cast<float>(NumericHeadphoneHearingRange));
		}
		if ((*JsonObject)->TryGetNumberField(TEXT("headphone_sensitivity"), NumericHeadphoneSensitivity) ||
			(*JsonObject)->TryGetNumberField(TEXT("hearing_sensitivity"), NumericHeadphoneSensitivity) ||
			(*JsonObject)->TryGetNumberField(TEXT("noise_sensitivity"), NumericHeadphoneSensitivity))
		{
			ItemDefinition.HeadphoneSensitivity = FMath::Max(0.0f, static_cast<float>(NumericHeadphoneSensitivity));
		}
		if ((*JsonObject)->TryGetNumberField(TEXT("headphone_min_strength"), NumericHeadphoneMinStrength) ||
			(*JsonObject)->TryGetNumberField(TEXT("hearing_min_strength"), NumericHeadphoneMinStrength) ||
			(*JsonObject)->TryGetNumberField(TEXT("noise_min_strength"), NumericHeadphoneMinStrength))
		{
			ItemDefinition.HeadphoneMinStrength = FMath::Max(0.0f, static_cast<float>(NumericHeadphoneMinStrength));
		}
		if ((*JsonObject)->TryGetNumberField(TEXT("use_health_delta"), NumericUseHealthDelta))
		{
			ItemDefinition.UseHealthDelta = static_cast<float>(NumericUseHealthDelta);
		}
		if ((*JsonObject)->TryGetNumberField(TEXT("use_food_delta"), NumericUseFoodDelta))
		{
			ItemDefinition.UseFoodDelta = static_cast<float>(NumericUseFoodDelta);
		}
		if ((*JsonObject)->TryGetNumberField(TEXT("use_hydration_delta"), NumericUseHydrationDelta))
		{
			ItemDefinition.UseHydrationDelta = static_cast<float>(NumericUseHydrationDelta);
		}
		if ((*JsonObject)->TryGetNumberField(TEXT("use_seconds"), NumericUseSeconds) ||
			(*JsonObject)->TryGetNumberField(TEXT("use_duration_seconds"), NumericUseSeconds) ||
			(*JsonObject)->TryGetNumberField(TEXT("consume_seconds"), NumericUseSeconds))
		{
			ItemDefinition.UseSeconds = FMath::Max(0.0f, static_cast<float>(NumericUseSeconds));
		}
		const TArray<TSharedPtr<FJsonValue>>* ClearsDebuffIdsArray = nullptr;
		if (((*JsonObject)->TryGetArrayField(TEXT("clears_debuff_ids"), ClearsDebuffIdsArray) ||
			 (*JsonObject)->TryGetArrayField(TEXT("clear_debuff_ids"), ClearsDebuffIdsArray) ||
			 (*JsonObject)->TryGetArrayField(TEXT("remove_debuff_ids"), ClearsDebuffIdsArray)) &&
			ClearsDebuffIdsArray)
		{
			for (const TSharedPtr<FJsonValue>& DebuffIdValue : *ClearsDebuffIdsArray)
			{
				const FString DebuffIdString = DebuffIdValue.IsValid()
					? DebuffIdValue->AsString().TrimStartAndEnd()
					: FString();
				if (!DebuffIdString.IsEmpty())
				{
					ItemDefinition.ClearsDebuffIds.AddUnique(FName(*DebuffIdString));
				}
			}
		}

		if (ItemDefinition.Id == INDEX_NONE || ItemDefinition.NameStringKey.IsNone() ||
			ItemDefinition.DescriptionStringKey.IsNone() || ItemDefinition.IconFileName.IsEmpty())
		{
			UE_LOG(LogTunaSweeperItemData, Warning, TEXT("Skipping item table row %d: field value is invalid."), RowIndex);
			continue;
		}

		if (ItemDefinitionsById.Contains(ItemDefinition.Id))
		{
			UE_LOG(LogTunaSweeperItemData, Warning, TEXT("Duplicate item id %d found. The later row will replace the earlier row."), ItemDefinition.Id);
		}

		ItemDefinitionsById.Add(ItemDefinition.Id, ItemDefinition);
		bHasValidRows = true;
	}

	if (!bHasValidRows)
	{
		UE_LOG(LogTunaSweeperItemData, Error, TEXT("Item table JSON has no valid rows: %s"), *ItemTableJsonPath);
	}

	return bHasValidRows;
}

bool UTunaSweeperItemDataSubsystem::LoadItemStackDefinitionsJson()
{
	FString JsonContent;
	const FString ItemStackDefinitionsJsonPath = GetItemStackDefinitionsJsonPath();
	if (!FFileHelper::LoadFileToString(JsonContent, *ItemStackDefinitionsJsonPath))
	{
		UE_LOG(LogTunaSweeperItemData, Error, TEXT("Failed to read item stack definitions JSON: %s"), *ItemStackDefinitionsJsonPath);
		return false;
	}

	TSharedPtr<FJsonObject> JsonObject;
	const TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(JsonContent);
	if (!FJsonSerializer::Deserialize(JsonReader, JsonObject) || !JsonObject.IsValid())
	{
		UE_LOG(LogTunaSweeperItemData, Error, TEXT("Failed to parse item stack definitions JSON: %s"), *ItemStackDefinitionsJsonPath);
		return false;
	}

	bool bHasValidRows = false;
	for (const TPair<FString, TSharedPtr<FJsonValue>>& StackDefinitionPair : JsonObject->Values)
	{
		const FString StackCategoryKeyString = StackDefinitionPair.Key.TrimStartAndEnd();
		if (StackCategoryKeyString.IsEmpty() || !StackDefinitionPair.Value.IsValid())
		{
			continue;
		}

		double NumericMaxStackQuantity = 0.0;
		if (StackDefinitionPair.Value->Type == EJson::Number)
		{
			NumericMaxStackQuantity = StackDefinitionPair.Value->AsNumber();
		}
		else if (StackDefinitionPair.Value->Type == EJson::Object)
		{
			const TSharedPtr<FJsonObject> StackObject = StackDefinitionPair.Value->AsObject();
			if (StackObject.IsValid())
			{
				StackObject->TryGetNumberField(TEXT("quantity"), NumericMaxStackQuantity) ||
					StackObject->TryGetNumberField(TEXT("max_stack"), NumericMaxStackQuantity) ||
					StackObject->TryGetNumberField(TEXT("max_stack_quantity"), NumericMaxStackQuantity);
			}
		}

		const int32 MaxStackQuantity = FMath::Max(1, FMath::RoundToInt(NumericMaxStackQuantity));
		MaxStackQuantitiesByCategoryKey.Add(FName(*StackCategoryKeyString), MaxStackQuantity);
		bHasValidRows = true;
	}

	if (!bHasValidRows)
	{
		UE_LOG(LogTunaSweeperItemData, Error, TEXT("Item stack definitions JSON has no valid rows: %s"), *ItemStackDefinitionsJsonPath);
	}

	return bHasValidRows;
}

bool UTunaSweeperItemDataSubsystem::LoadItemNameStringsCsv()
{
	FString CsvContent;
	const FString ItemNameStringsCsvPath = GetItemNameStringsCsvPath();
	if (!FFileHelper::LoadFileToString(CsvContent, *ItemNameStringsCsvPath))
	{
		UE_LOG(LogTunaSweeperItemData, Error, TEXT("Failed to read item name strings CSV: %s"), *ItemNameStringsCsvPath);
		return false;
	}

	FCsvParser CsvParser(CsvContent);
	const FCsvParser::FRows& Rows = CsvParser.GetRows();
	if (Rows.Num() < 2)
	{
		UE_LOG(LogTunaSweeperItemData, Error, TEXT("Item name strings CSV has no data rows: %s"), *ItemNameStringsCsvPath);
		return false;
	}

	const TArray<const TCHAR*>& HeaderRow = Rows[0];
	const bool bHeaderIsValid =
		TunaSweeperItemDataFiles::GetCsvCell(HeaderRow, 0).Equals(TEXT("string_key"), ESearchCase::IgnoreCase) &&
		TunaSweeperItemDataFiles::GetCsvCell(HeaderRow, 1).Equals(TEXT("ko"), ESearchCase::IgnoreCase) &&
		TunaSweeperItemDataFiles::GetCsvCell(HeaderRow, 2).Equals(TEXT("en"), ESearchCase::IgnoreCase) &&
		TunaSweeperItemDataFiles::GetCsvCell(HeaderRow, 3).Equals(TEXT("ja"), ESearchCase::IgnoreCase);
	if (!bHeaderIsValid)
	{
		UE_LOG(LogTunaSweeperItemData, Error, TEXT("Item name strings CSV header must be string_key,ko,en,ja: %s"), *ItemNameStringsCsvPath);
		return false;
	}

	bool bHasValidRows = false;
	for (int32 RowIndex = 1; RowIndex < Rows.Num(); ++RowIndex)
	{
		const TArray<const TCHAR*>& Row = Rows[RowIndex];
		if (Row.Num() < 4)
		{
			UE_LOG(LogTunaSweeperItemData, Warning, TEXT("Skipping item name row %d: expected 4 columns."), RowIndex);
			continue;
		}

		const FString StringKey = TunaSweeperItemDataFiles::GetCsvCell(Row, 0);
		const FString Korean = TunaSweeperItemDataFiles::GetCsvCell(Row, 1);
		const FString English = TunaSweeperItemDataFiles::GetCsvCell(Row, 2);
		const FString Japanese = TunaSweeperItemDataFiles::GetCsvCell(Row, 3);

		if (StringKey.IsEmpty() || Korean.IsEmpty() || English.IsEmpty() || Japanese.IsEmpty())
		{
			UE_LOG(LogTunaSweeperItemData, Warning, TEXT("Skipping item name row %d: required cell is empty."), RowIndex);
			continue;
		}

		FTunaSweeperItemNameString NameString;
		NameString.StringKey = FName(*StringKey);
		NameString.Korean = FText::FromString(Korean);
		NameString.English = FText::FromString(English);
		NameString.Japanese = FText::FromString(Japanese);

		if (ItemNameStringsByKey.Contains(NameString.StringKey))
		{
			UE_LOG(LogTunaSweeperItemData, Warning, TEXT("Duplicate item name string key %s found. The later row will replace the earlier row."), *StringKey);
		}

		ItemNameStringsByKey.Add(NameString.StringKey, NameString);
		bHasValidRows = true;
	}

	if (!bHasValidRows)
	{
		UE_LOG(LogTunaSweeperItemData, Error, TEXT("Item name strings CSV has no valid rows: %s"), *ItemNameStringsCsvPath);
	}

	return bHasValidRows;
}

bool UTunaSweeperItemDataSubsystem::LoadLootContainerTableJson()
{
	FString JsonContent;
	const FString LootContainerTableJsonPath = GetLootContainerTableJsonPath();
	if (!FFileHelper::LoadFileToString(JsonContent, *LootContainerTableJsonPath))
	{
		UE_LOG(LogTunaSweeperItemData, Error, TEXT("Failed to read loot container table JSON: %s"), *LootContainerTableJsonPath);
		return false;
	}

	TArray<TSharedPtr<FJsonValue>> JsonRows;
	const TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(JsonContent);
	if (!FJsonSerializer::Deserialize(JsonReader, JsonRows))
	{
		UE_LOG(LogTunaSweeperItemData, Error, TEXT("Failed to parse loot container table JSON: %s"), *LootContainerTableJsonPath);
		return false;
	}

	bool bHasValidRows = false;
	for (int32 RowIndex = 0; RowIndex < JsonRows.Num(); ++RowIndex)
	{
		const TSharedPtr<FJsonObject>* JsonObject = nullptr;
		if (!JsonRows[RowIndex].IsValid() || !JsonRows[RowIndex]->TryGetObject(JsonObject) || !JsonObject || !JsonObject->IsValid())
		{
			UE_LOG(LogTunaSweeperItemData, Warning, TEXT("Skipping loot container row %d: row is not an object."), RowIndex);
			continue;
		}

		double NumericId = 0.0;
		double NumericCapacity = 0.0;
		FString NameStringKey;
		FString StaticMeshPath;
		FString MaterialPath;
		if (!(*JsonObject)->TryGetNumberField(TEXT("id"), NumericId) ||
			!(*JsonObject)->TryGetStringField(TEXT("name_string_key"), NameStringKey) ||
			!(*JsonObject)->TryGetNumberField(TEXT("capacity"), NumericCapacity) ||
			!(*JsonObject)->TryGetStringField(TEXT("static_mesh_path"), StaticMeshPath))
		{
			UE_LOG(LogTunaSweeperItemData, Warning, TEXT("Skipping loot container row %d: required field is missing."), RowIndex);
			continue;
		}

		FTunaSweeperLootContainerDefinition Definition;
		Definition.Id = static_cast<int32>(NumericId);
		Definition.NameStringKey = FName(*NameStringKey.TrimStartAndEnd());
		Definition.Capacity = static_cast<int32>(NumericCapacity);
		Definition.StaticMeshPath = StaticMeshPath.TrimStartAndEnd();
		if ((*JsonObject)->TryGetStringField(TEXT("material_path"), MaterialPath))
		{
			Definition.MaterialPath = MaterialPath.TrimStartAndEnd();
		}

		const TArray<TSharedPtr<FJsonValue>>* MeshScaleArray = nullptr;
		if ((*JsonObject)->TryGetArrayField(TEXT("mesh_scale"), MeshScaleArray) && MeshScaleArray && MeshScaleArray->Num() >= 3)
		{
			Definition.MeshScale = FVector(
				static_cast<float>((*MeshScaleArray)[0]->AsNumber()),
				static_cast<float>((*MeshScaleArray)[1]->AsNumber()),
				static_cast<float>((*MeshScaleArray)[2]->AsNumber()));
		}

		if (Definition.Id == INDEX_NONE || Definition.NameStringKey.IsNone() || Definition.StaticMeshPath.IsEmpty() ||
			Definition.Capacity <= 0)
		{
			UE_LOG(LogTunaSweeperItemData, Warning, TEXT("Skipping loot container row %d: field value is invalid."), RowIndex);
			continue;
		}

		LootContainerDefinitionsById.Add(Definition.Id, Definition);
		bHasValidRows = true;
	}

	if (!bHasValidRows)
	{
		UE_LOG(LogTunaSweeperItemData, Error, TEXT("Loot container table JSON has no valid rows: %s"), *LootContainerTableJsonPath);
	}

	return bHasValidRows;
}

bool UTunaSweeperItemDataSubsystem::LoadLootContainerContentsJson()
{
	FString JsonContent;
	const FString LootContainerContentsJsonPath = GetLootContainerContentsJsonPath();
	if (!FFileHelper::LoadFileToString(JsonContent, *LootContainerContentsJsonPath))
	{
		UE_LOG(LogTunaSweeperItemData, Error, TEXT("Failed to read loot container contents JSON: %s"), *LootContainerContentsJsonPath);
		return false;
	}

	TArray<TSharedPtr<FJsonValue>> JsonRows;
	const TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(JsonContent);
	if (!FJsonSerializer::Deserialize(JsonReader, JsonRows))
	{
		UE_LOG(LogTunaSweeperItemData, Error, TEXT("Failed to parse loot container contents JSON: %s"), *LootContainerContentsJsonPath);
		return false;
	}

	bool bHasValidRows = false;
	for (int32 RowIndex = 0; RowIndex < JsonRows.Num(); ++RowIndex)
	{
		const TSharedPtr<FJsonObject>* JsonObject = nullptr;
		if (!JsonRows[RowIndex].IsValid() || !JsonRows[RowIndex]->TryGetObject(JsonObject) || !JsonObject || !JsonObject->IsValid())
		{
			UE_LOG(LogTunaSweeperItemData, Warning, TEXT("Skipping loot contents row %d: row is not an object."), RowIndex);
			continue;
		}

		double NumericId = 0.0;
		const TArray<TSharedPtr<FJsonValue>>* ItemsArray = nullptr;
		if (!(*JsonObject)->TryGetNumberField(TEXT("id"), NumericId) ||
			!(*JsonObject)->TryGetArrayField(TEXT("items"), ItemsArray) ||
			!ItemsArray)
		{
			UE_LOG(LogTunaSweeperItemData, Warning, TEXT("Skipping loot contents row %d: required field is missing."), RowIndex);
			continue;
		}

		FTunaSweeperLootContainerContents Contents;
		Contents.Id = static_cast<int32>(NumericId);

		for (const TSharedPtr<FJsonValue>& ItemValue : *ItemsArray)
		{
			const TSharedPtr<FJsonObject>* ItemObject = nullptr;
			if (!ItemValue.IsValid() || !ItemValue->TryGetObject(ItemObject) || !ItemObject || !ItemObject->IsValid())
			{
				continue;
			}

			double NumericItemId = 0.0;
			double NumericQuantity = 0.0;
			double NumericQuantityMin = 0.0;
			double NumericQuantityMax = 0.0;
			double NumericDropChance = 1.0;
			const bool bHasFixedQuantity = (*ItemObject)->TryGetNumberField(TEXT("quantity"), NumericQuantity);
			const bool bHasQuantityMin =
				(*ItemObject)->TryGetNumberField(TEXT("quantity_min"), NumericQuantityMin) ||
				(*ItemObject)->TryGetNumberField(TEXT("min_quantity"), NumericQuantityMin);
			const bool bHasQuantityMax =
				(*ItemObject)->TryGetNumberField(TEXT("quantity_max"), NumericQuantityMax) ||
				(*ItemObject)->TryGetNumberField(TEXT("max_quantity"), NumericQuantityMax);
			(*ItemObject)->TryGetNumberField(TEXT("drop_chance"), NumericDropChance) ||
				(*ItemObject)->TryGetNumberField(TEXT("chance"), NumericDropChance) ||
				(*ItemObject)->TryGetNumberField(TEXT("probability"), NumericDropChance) ||
				(*ItemObject)->TryGetNumberField(TEXT("drop_chance_ratio"), NumericDropChance);
			if (!(*ItemObject)->TryGetNumberField(TEXT("item_id"), NumericItemId) ||
				(!bHasFixedQuantity && (!bHasQuantityMin || !bHasQuantityMax)))
			{
				continue;
			}

			const int32 ItemId = static_cast<int32>(NumericItemId);
			if (!ItemDefinitionsById.Contains(ItemId))
			{
				UE_LOG(LogTunaSweeperItemData, Warning, TEXT("Skipping unknown item id %d in loot contents %d."), ItemId, Contents.Id);
				continue;
			}

			const int32 QuantityMin = bHasFixedQuantity
				? static_cast<int32>(NumericQuantity)
				: static_cast<int32>(NumericQuantityMin);
			const int32 QuantityMax = bHasFixedQuantity
				? static_cast<int32>(NumericQuantity)
				: static_cast<int32>(NumericQuantityMax);

			FTunaSweeperLootContainerItemQuantity QuantityEntry;
			TunaSweeperItemDataFiles::BuildLootItemQuantityEntry(
				ItemId,
				QuantityMin,
				QuantityMax,
				TunaSweeperItemDataFiles::NormalizeDropChanceValue(static_cast<float>(NumericDropChance)),
				QuantityEntry);
			Contents.ItemQuantities.Add(QuantityEntry);

			FTunaSweeperItemStack ItemStack;
			ItemStack.ItemId = ItemId;
			ItemStack.Quantity = QuantityEntry.QuantityMin;
			Contents.Items.Add(ItemStack);
		}

		if (Contents.Id == INDEX_NONE || Contents.Items.Num() <= 0)
		{
			UE_LOG(LogTunaSweeperItemData, Warning, TEXT("Skipping loot contents row %d: field value is invalid."), RowIndex);
			continue;
		}

		LootContainerContentsById.Add(Contents.Id, Contents);
		bHasValidRows = true;
	}

	if (!bHasValidRows)
	{
		UE_LOG(LogTunaSweeperItemData, Error, TEXT("Loot container contents JSON has no valid rows: %s"), *LootContainerContentsJsonPath);
	}

	return bHasValidRows;
}

bool UTunaSweeperItemDataSubsystem::LoadShopDefinitionsJson()
{
	FString JsonContent;
	const FString ShopDefinitionsJsonPath = GetShopDefinitionsJsonPath();
	if (!FFileHelper::LoadFileToString(JsonContent, *ShopDefinitionsJsonPath))
	{
		UE_LOG(LogTunaSweeperItemData, Error, TEXT("Failed to read shop definitions JSON: %s"), *ShopDefinitionsJsonPath);
		return false;
	}

	TArray<TSharedPtr<FJsonValue>> JsonRows;
	const TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(JsonContent);
	if (!FJsonSerializer::Deserialize(JsonReader, JsonRows))
	{
		UE_LOG(LogTunaSweeperItemData, Error, TEXT("Failed to parse shop definitions JSON: %s"), *ShopDefinitionsJsonPath);
		return false;
	}

	bool bHasValidRows = false;
	for (int32 RowIndex = 0; RowIndex < JsonRows.Num(); ++RowIndex)
	{
		const TSharedPtr<FJsonObject>* JsonObject = nullptr;
		if (!JsonRows[RowIndex].IsValid() || !JsonRows[RowIndex]->TryGetObject(JsonObject) || !JsonObject || !JsonObject->IsValid())
		{
			UE_LOG(LogTunaSweeperItemData, Warning, TEXT("Skipping shop definition row %d: row is not an object."), RowIndex);
			continue;
		}

		double NumericShopId = 0.0;
		FString NameStringKey;
		const TArray<TSharedPtr<FJsonValue>>* ItemsArray = nullptr;
		if (!(*JsonObject)->TryGetNumberField(TEXT("shop_id"), NumericShopId) ||
			!(*JsonObject)->TryGetArrayField(TEXT("items"), ItemsArray) ||
			!ItemsArray)
		{
			UE_LOG(LogTunaSweeperItemData, Warning, TEXT("Skipping shop definition row %d: required field is missing."), RowIndex);
			continue;
		}

		FTunaSweeperShopDefinition Definition;
		Definition.ShopId = static_cast<int32>(NumericShopId);
		if ((*JsonObject)->TryGetStringField(TEXT("name_string_key"), NameStringKey))
		{
			Definition.NameStringKey = FName(*NameStringKey.TrimStartAndEnd());
		}

		for (const TSharedPtr<FJsonValue>& ItemValue : *ItemsArray)
		{
			const TSharedPtr<FJsonObject>* ItemObject = nullptr;
			if (!ItemValue.IsValid() || !ItemValue->TryGetObject(ItemObject) || !ItemObject || !ItemObject->IsValid())
			{
				continue;
			}

			double NumericItemId = 0.0;
			double NumericStockQuantity = 0.0;
			double NumericPrice = INDEX_NONE;
			const bool bHasStockQuantity =
				(*ItemObject)->TryGetNumberField(TEXT("stock_quantity"), NumericStockQuantity) ||
				(*ItemObject)->TryGetNumberField(TEXT("total_stock_quantity"), NumericStockQuantity) ||
				(*ItemObject)->TryGetNumberField(TEXT("stock"), NumericStockQuantity) ||
				(*ItemObject)->TryGetNumberField(TEXT("quantity"), NumericStockQuantity);
			if (!(*ItemObject)->TryGetNumberField(TEXT("item_id"), NumericItemId) || !bHasStockQuantity)
			{
				continue;
			}

			const int32 ItemId = static_cast<int32>(NumericItemId);
			if (!ItemDefinitionsById.Contains(ItemId))
			{
				UE_LOG(LogTunaSweeperItemData, Warning, TEXT("Skipping unknown item id %d in shop %d."), ItemId, Definition.ShopId);
				continue;
			}

			FTunaSweeperShopItemDefinition ShopItem;
			ShopItem.ItemId = ItemId;
			ShopItem.StockQuantity = FMath::Max(0, static_cast<int32>(NumericStockQuantity));
			if ((*ItemObject)->TryGetNumberField(TEXT("price"), NumericPrice) ||
				(*ItemObject)->TryGetNumberField(TEXT("buy_price"), NumericPrice) ||
				(*ItemObject)->TryGetNumberField(TEXT("shop_price"), NumericPrice))
			{
				ShopItem.PriceOverride = FMath::Max(0, static_cast<int32>(NumericPrice));
			}

			Definition.Items.Add(ShopItem);
		}

		if (Definition.ShopId == INDEX_NONE || Definition.ShopId <= 0 || Definition.Items.Num() <= 0)
		{
			UE_LOG(LogTunaSweeperItemData, Warning, TEXT("Skipping shop definition row %d: field value is invalid."), RowIndex);
			continue;
		}

		if (ShopDefinitionsById.Contains(Definition.ShopId))
		{
			UE_LOG(LogTunaSweeperItemData, Warning, TEXT("Duplicate shop id %d found. The later row will replace the earlier row."), Definition.ShopId);
		}

		ShopDefinitionsById.Add(Definition.ShopId, Definition);
		bHasValidRows = true;
	}

	if (!bHasValidRows)
	{
		UE_LOG(LogTunaSweeperItemData, Error, TEXT("Shop definitions JSON has no valid rows: %s"), *ShopDefinitionsJsonPath);
	}

	return bHasValidRows;
}

bool UTunaSweeperItemDataSubsystem::LoadWorkbenchRecipesJson()
{
	FString JsonContent;
	const FString WorkbenchRecipesJsonPath = GetWorkbenchRecipesJsonPath();
	if (!FPaths::FileExists(WorkbenchRecipesJsonPath))
	{
		return true;
	}

	if (!FFileHelper::LoadFileToString(JsonContent, *WorkbenchRecipesJsonPath))
	{
		UE_LOG(LogTunaSweeperItemData, Error, TEXT("Failed to read workbench recipes JSON: %s"), *WorkbenchRecipesJsonPath);
		return false;
	}

	TArray<TSharedPtr<FJsonValue>> JsonRows;
	const TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(JsonContent);
	if (!FJsonSerializer::Deserialize(JsonReader, JsonRows))
	{
		UE_LOG(LogTunaSweeperItemData, Error, TEXT("Failed to parse workbench recipes JSON: %s"), *WorkbenchRecipesJsonPath);
		return false;
	}

	bool bHasValidRows = false;
	for (int32 RowIndex = 0; RowIndex < JsonRows.Num(); ++RowIndex)
	{
		const TSharedPtr<FJsonObject>* JsonObject = nullptr;
		if (!JsonRows[RowIndex].IsValid() || !JsonRows[RowIndex]->TryGetObject(JsonObject) || !JsonObject || !JsonObject->IsValid())
		{
			UE_LOG(LogTunaSweeperItemData, Warning, TEXT("Skipping workbench recipe row %d: row is not an object."), RowIndex);
			continue;
		}

		FString RecipeIdString;
		double NumericRecipeId = 0.0;
		if (!(*JsonObject)->TryGetStringField(TEXT("recipe_id"), RecipeIdString) &&
			!(*JsonObject)->TryGetStringField(TEXT("id"), RecipeIdString))
		{
			if ((*JsonObject)->TryGetNumberField(TEXT("id"), NumericRecipeId))
			{
				RecipeIdString = FString::Printf(TEXT("%d"), static_cast<int32>(NumericRecipeId));
			}
		}

		double NumericWorkbenchId = 1.0;
		double NumericOutputItemId = INDEX_NONE;
		double NumericOutputQuantity = 1.0;
		FString NameStringKey;
		const TArray<TSharedPtr<FJsonValue>>* IngredientsArray = nullptr;
		JsonObject->Get()->TryGetNumberField(TEXT("workbench_id"), NumericWorkbenchId);
		JsonObject->Get()->TryGetStringField(TEXT("name_string_key"), NameStringKey);
		JsonObject->Get()->TryGetNumberField(TEXT("output_quantity"), NumericOutputQuantity) ||
			JsonObject->Get()->TryGetNumberField(TEXT("result_quantity"), NumericOutputQuantity) ||
			JsonObject->Get()->TryGetNumberField(TEXT("quantity"), NumericOutputQuantity);
		if (!JsonObject->Get()->TryGetNumberField(TEXT("output_item_id"), NumericOutputItemId) &&
			!JsonObject->Get()->TryGetNumberField(TEXT("result_item_id"), NumericOutputItemId) &&
			!JsonObject->Get()->TryGetNumberField(TEXT("item_id"), NumericOutputItemId))
		{
			UE_LOG(LogTunaSweeperItemData, Warning, TEXT("Skipping workbench recipe row %d: output item id is missing."), RowIndex);
			continue;
		}

		if (!JsonObject->Get()->TryGetArrayField(TEXT("ingredients"), IngredientsArray) || !IngredientsArray)
		{
			UE_LOG(LogTunaSweeperItemData, Warning, TEXT("Skipping workbench recipe row %d: ingredients are missing."), RowIndex);
			continue;
		}

		FTunaSweeperWorkbenchRecipeDefinition Definition;
		Definition.RecipeId = FName(*RecipeIdString.TrimStartAndEnd());
		Definition.WorkbenchId = FMath::Max(1, static_cast<int32>(NumericWorkbenchId));
		Definition.NameStringKey = FName(*NameStringKey.TrimStartAndEnd());
		Definition.OutputItemId = static_cast<int32>(NumericOutputItemId);
		Definition.OutputQuantity = FMath::Max(1, static_cast<int32>(NumericOutputQuantity));
		JsonObject->Get()->TryGetBoolField(TEXT("auto_unlocked"), Definition.bAutoUnlocked) ||
			JsonObject->Get()->TryGetBoolField(TEXT("unlocked"), Definition.bAutoUnlocked);

		if (Definition.RecipeId.IsNone())
		{
			Definition.RecipeId = FName(*FString::Printf(TEXT("workbench_%d_item_%d"), Definition.WorkbenchId, Definition.OutputItemId));
		}

		if (Definition.OutputItemId == INDEX_NONE || !ItemDefinitionsById.Contains(Definition.OutputItemId))
		{
			UE_LOG(LogTunaSweeperItemData, Warning, TEXT("Skipping workbench recipe %s: output item id %d is unknown."), *Definition.RecipeId.ToString(), Definition.OutputItemId);
			continue;
		}

		for (const TSharedPtr<FJsonValue>& IngredientValue : *IngredientsArray)
		{
			const TSharedPtr<FJsonObject>* IngredientObject = nullptr;
			if (!IngredientValue.IsValid() || !IngredientValue->TryGetObject(IngredientObject) || !IngredientObject || !IngredientObject->IsValid())
			{
				continue;
			}

			double NumericItemId = INDEX_NONE;
			double NumericQuantity = 1.0;
			if (!(*IngredientObject)->TryGetNumberField(TEXT("item_id"), NumericItemId))
			{
				continue;
			}
			(*IngredientObject)->TryGetNumberField(TEXT("quantity"), NumericQuantity) ||
				(*IngredientObject)->TryGetNumberField(TEXT("count"), NumericQuantity) ||
				(*IngredientObject)->TryGetNumberField(TEXT("required_quantity"), NumericQuantity);

			FTunaSweeperWorkbenchIngredient Ingredient;
			Ingredient.ItemId = static_cast<int32>(NumericItemId);
			Ingredient.Quantity = FMath::Max(1, static_cast<int32>(NumericQuantity));
			if (Ingredient.ItemId == INDEX_NONE || !ItemDefinitionsById.Contains(Ingredient.ItemId))
			{
				UE_LOG(LogTunaSweeperItemData, Warning, TEXT("Skipping unknown ingredient item id %d in workbench recipe %s."), Ingredient.ItemId, *Definition.RecipeId.ToString());
				continue;
			}

			FTunaSweeperWorkbenchIngredient* ExistingIngredient = Definition.Ingredients.FindByPredicate(
				[&Ingredient](const FTunaSweeperWorkbenchIngredient& Candidate)
				{
					return Candidate.ItemId == Ingredient.ItemId;
				});
			if (ExistingIngredient)
			{
				ExistingIngredient->Quantity += Ingredient.Quantity;
			}
			else
			{
				Definition.Ingredients.Add(Ingredient);
			}
		}

		if (Definition.Ingredients.Num() <= 0)
		{
			UE_LOG(LogTunaSweeperItemData, Warning, TEXT("Skipping workbench recipe %s: no valid ingredients."), *Definition.RecipeId.ToString());
			continue;
		}

		if (WorkbenchRecipeDefinitionsById.Contains(Definition.RecipeId))
		{
			UE_LOG(LogTunaSweeperItemData, Warning, TEXT("Duplicate workbench recipe id %s found. The later row will replace the earlier row."), *Definition.RecipeId.ToString());
			WorkbenchRecipeIdsInLoadOrder.Remove(Definition.RecipeId);
		}

		WorkbenchRecipeDefinitionsById.Add(Definition.RecipeId, Definition);
		WorkbenchRecipeIdsInLoadOrder.Add(Definition.RecipeId);
		bHasValidRows = true;
	}

	if (!bHasValidRows)
	{
		UE_LOG(LogTunaSweeperItemData, Error, TEXT("Workbench recipes JSON has no valid rows: %s"), *WorkbenchRecipesJsonPath);
	}

	return bHasValidRows;
}

bool UTunaSweeperItemDataSubsystem::LoadWorkbenchDismantleRecipesJson()
{
	FString JsonContent;
	const FString WorkbenchDismantleRecipesJsonPath = GetWorkbenchDismantleRecipesJsonPath();
	if (!FPaths::FileExists(WorkbenchDismantleRecipesJsonPath))
	{
		return true;
	}

	if (!FFileHelper::LoadFileToString(JsonContent, *WorkbenchDismantleRecipesJsonPath))
	{
		UE_LOG(LogTunaSweeperItemData, Error, TEXT("Failed to read workbench dismantle recipes JSON: %s"), *WorkbenchDismantleRecipesJsonPath);
		return false;
	}

	TArray<TSharedPtr<FJsonValue>> JsonRows;
	const TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(JsonContent);
	if (!FJsonSerializer::Deserialize(JsonReader, JsonRows))
	{
		UE_LOG(LogTunaSweeperItemData, Error, TEXT("Failed to parse workbench dismantle recipes JSON: %s"), *WorkbenchDismantleRecipesJsonPath);
		return false;
	}

	bool bHasValidRows = false;
	for (int32 RowIndex = 0; RowIndex < JsonRows.Num(); ++RowIndex)
	{
		const TSharedPtr<FJsonObject>* JsonObject = nullptr;
		if (!JsonRows[RowIndex].IsValid() || !JsonRows[RowIndex]->TryGetObject(JsonObject) || !JsonObject || !JsonObject->IsValid())
		{
			UE_LOG(LogTunaSweeperItemData, Warning, TEXT("Skipping workbench dismantle row %d: row is not an object."), RowIndex);
			continue;
		}

		double NumericSourceItemId = INDEX_NONE;
		if (!(*JsonObject)->TryGetNumberField(TEXT("source_item_id"), NumericSourceItemId) &&
			!(*JsonObject)->TryGetNumberField(TEXT("input_item_id"), NumericSourceItemId) &&
			!(*JsonObject)->TryGetNumberField(TEXT("item_id"), NumericSourceItemId))
		{
			UE_LOG(LogTunaSweeperItemData, Warning, TEXT("Skipping workbench dismantle row %d: source item id is missing."), RowIndex);
			continue;
		}

		FTunaSweeperWorkbenchDismantleDefinition Definition;
		Definition.SourceItemId = static_cast<int32>(NumericSourceItemId);
		if (Definition.SourceItemId == INDEX_NONE || !ItemDefinitionsById.Contains(Definition.SourceItemId))
		{
			UE_LOG(LogTunaSweeperItemData, Warning, TEXT("Skipping workbench dismantle row %d: source item id %d is unknown."), RowIndex, Definition.SourceItemId);
			continue;
		}

		const TArray<TSharedPtr<FJsonValue>>* ResultArray = nullptr;
		if (!(*JsonObject)->TryGetArrayField(TEXT("results"), ResultArray) &&
			!(*JsonObject)->TryGetArrayField(TEXT("outputs"), ResultArray))
		{
			UE_LOG(LogTunaSweeperItemData, Warning, TEXT("Skipping workbench dismantle row %d: results are missing."), RowIndex);
			continue;
		}
		if (!ResultArray)
		{
			continue;
		}

		for (const TSharedPtr<FJsonValue>& ResultValue : *ResultArray)
		{
			const TSharedPtr<FJsonObject>* ResultObject = nullptr;
			if (!ResultValue.IsValid() || !ResultValue->TryGetObject(ResultObject) || !ResultObject || !ResultObject->IsValid())
			{
				continue;
			}

			double NumericItemId = INDEX_NONE;
			double NumericQuantity = 1.0;
			if (!(*ResultObject)->TryGetNumberField(TEXT("item_id"), NumericItemId))
			{
				continue;
			}
			(*ResultObject)->TryGetNumberField(TEXT("quantity"), NumericQuantity) ||
				(*ResultObject)->TryGetNumberField(TEXT("count"), NumericQuantity);

			FTunaSweeperItemStack ResultStack;
			ResultStack.ItemId = static_cast<int32>(NumericItemId);
			ResultStack.Quantity = FMath::Max(1, static_cast<int32>(NumericQuantity));
			if (ResultStack.ItemId == INDEX_NONE || !ItemDefinitionsById.Contains(ResultStack.ItemId))
			{
				UE_LOG(LogTunaSweeperItemData, Warning, TEXT("Skipping unknown dismantle result item id %d for source item %d."), ResultStack.ItemId, Definition.SourceItemId);
				continue;
			}

			FTunaSweeperItemStack* ExistingResult = Definition.Results.FindByPredicate(
				[&ResultStack](const FTunaSweeperItemStack& Candidate)
				{
					return Candidate.ItemId == ResultStack.ItemId;
				});
			if (ExistingResult)
			{
				ExistingResult->Quantity += ResultStack.Quantity;
			}
			else
			{
				Definition.Results.Add(ResultStack);
			}
		}

		if (Definition.Results.Num() <= 0)
		{
			UE_LOG(LogTunaSweeperItemData, Warning, TEXT("Skipping workbench dismantle source item %d: no valid results."), Definition.SourceItemId);
			continue;
		}

		if (WorkbenchDismantleDefinitionsByItemId.Contains(Definition.SourceItemId))
		{
			UE_LOG(LogTunaSweeperItemData, Warning, TEXT("Duplicate workbench dismantle source item id %d found. The later row will replace the earlier row."), Definition.SourceItemId);
			WorkbenchDismantleItemIdsInLoadOrder.Remove(Definition.SourceItemId);
		}

		WorkbenchDismantleDefinitionsByItemId.Add(Definition.SourceItemId, Definition);
		WorkbenchDismantleItemIdsInLoadOrder.Add(Definition.SourceItemId);
		bHasValidRows = true;
	}

	if (!bHasValidRows)
	{
		UE_LOG(LogTunaSweeperItemData, Error, TEXT("Workbench dismantle recipes JSON has no valid rows: %s"), *WorkbenchDismantleRecipesJsonPath);
	}

	return bHasValidRows;
}

void UTunaSweeperItemDataSubsystem::ResetLoadedItemData()
{
	ItemDefinitionsById.Reset();
	MaxStackQuantitiesByCategoryKey.Reset();
	ItemNameStringsByKey.Reset();
	LootContainerDefinitionsById.Reset();
	LootContainerContentsById.Reset();
	ShopDefinitionsById.Reset();
	WorkbenchRecipeDefinitionsById.Reset();
	WorkbenchRecipeIdsInLoadOrder.Reset();
	WorkbenchDismantleDefinitionsByItemId.Reset();
	WorkbenchDismantleItemIdsInLoadOrder.Reset();
	bItemDataLoaded = false;
}

FString UTunaSweeperItemDataSubsystem::GetItemTableJsonPath() const
{
	return FPaths::Combine(FPaths::ProjectContentDir(), TunaSweeperItemDataFiles::ItemTableJsonRelativePath);
}

FString UTunaSweeperItemDataSubsystem::GetItemStackDefinitionsJsonPath() const
{
	return FPaths::Combine(FPaths::ProjectContentDir(), TunaSweeperItemDataFiles::ItemStackDefinitionsJsonRelativePath);
}

FString UTunaSweeperItemDataSubsystem::GetItemNameStringsCsvPath() const
{
	return FPaths::Combine(FPaths::ProjectContentDir(), TunaSweeperItemDataFiles::ItemNameStringsCsvRelativePath);
}

FString UTunaSweeperItemDataSubsystem::GetLootContainerTableJsonPath() const
{
	return FPaths::Combine(FPaths::ProjectContentDir(), TunaSweeperItemDataFiles::LootContainerTableJsonRelativePath);
}

FString UTunaSweeperItemDataSubsystem::GetLootContainerContentsJsonPath() const
{
	return FPaths::Combine(FPaths::ProjectContentDir(), TunaSweeperItemDataFiles::LootContainerContentsJsonRelativePath);
}

FString UTunaSweeperItemDataSubsystem::GetShopDefinitionsJsonPath() const
{
	return FPaths::Combine(FPaths::ProjectContentDir(), TunaSweeperItemDataFiles::ShopDefinitionsJsonRelativePath);
}

FString UTunaSweeperItemDataSubsystem::GetWorkbenchRecipesJsonPath() const
{
	return FPaths::Combine(FPaths::ProjectContentDir(), TunaSweeperItemDataFiles::WorkbenchRecipesJsonRelativePath);
}

FString UTunaSweeperItemDataSubsystem::GetWorkbenchDismantleRecipesJsonPath() const
{
	return FPaths::Combine(FPaths::ProjectContentDir(), TunaSweeperItemDataFiles::WorkbenchDismantleRecipesJsonRelativePath);
}
