#include "Subsystem/TunaSweeperItemDataSubsystem.h"

#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/Csv/CsvParser.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

DEFINE_LOG_CATEGORY_STATIC(LogTunaSweeperItemData, Log, All);

namespace TunaSweeperItemDataFiles
{
	const TCHAR* ItemTableJsonRelativePath = TEXT("Data/ItemTable.json");
	const TCHAR* ItemNameStringsCsvRelativePath = TEXT("Data/ItemNameStrings.csv");
	const TCHAR* LootContainerTableJsonRelativePath = TEXT("Data/LootContainerTable.json");
	const TCHAR* LootContainerContentsJsonRelativePath = TEXT("Data/LootContainerContents.json");

	FString GetCsvCell(const TArray<const TCHAR*>& Row, int32 CellIndex)
	{
		return Row.IsValidIndex(CellIndex)
			? FString(Row[CellIndex]).TrimStartAndEnd()
			: FString();
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
	const bool bLoadedNameStrings = LoadItemNameStringsCsv();
	const bool bLoadedLootContainerTable = LoadLootContainerTableJson();
	const bool bLoadedLootContainerContents = LoadLootContainerContentsJson();
	bItemDataLoaded = bLoadedItemTable && bLoadedNameStrings && bLoadedLootContainerTable && bLoadedLootContainerContents;

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

	if (Contents.Items.Num() > ContainerDefinition.Capacity)
	{
		UE_LOG(
			LogTunaSweeperItemData,
			Warning,
			TEXT("Loot contents %d has %d stacks, exceeding container %d capacity %d."),
			Contents.Id,
			Contents.Items.Num(),
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
	OutInstance.Items = Contents.Items;
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
		double NumericInventorySlotCapacity = 0.0;
		FString NameStringKey;
		FString DescriptionStringKey;
		FString IconFileName;
		FString CategoryTag;
		FString EquipmentSlotTag;
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
		if ((*JsonObject)->TryGetStringField(TEXT("category_tag"), CategoryTag))
		{
			ItemDefinition.CategoryTag = FName(*CategoryTag.TrimStartAndEnd());
		}
		if ((*JsonObject)->TryGetStringField(TEXT("equipment_slot_tag"), EquipmentSlotTag))
		{
			ItemDefinition.EquipmentSlotTag = FName(*EquipmentSlotTag.TrimStartAndEnd());
		}
		if ((*JsonObject)->TryGetNumberField(TEXT("inventory_slot_capacity"), NumericInventorySlotCapacity))
		{
			ItemDefinition.InventorySlotCapacity = FMath::Max(0, static_cast<int32>(NumericInventorySlotCapacity));
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

		const TArray<TSharedPtr<FJsonValue>>* MeshScaleArray = nullptr;
		if ((*JsonObject)->TryGetArrayField(TEXT("mesh_scale"), MeshScaleArray) && MeshScaleArray && MeshScaleArray->Num() >= 3)
		{
			Definition.MeshScale = FVector(
				static_cast<float>((*MeshScaleArray)[0]->AsNumber()),
				static_cast<float>((*MeshScaleArray)[1]->AsNumber()),
				static_cast<float>((*MeshScaleArray)[2]->AsNumber()));
		}

		const bool bCapacityIsSupported =
			Definition.Capacity == 5 ||
			Definition.Capacity == 10 ||
			Definition.Capacity == 15;
		if (Definition.Id == INDEX_NONE || Definition.NameStringKey.IsNone() || Definition.StaticMeshPath.IsEmpty() ||
			!bCapacityIsSupported)
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
			if (!(*ItemObject)->TryGetNumberField(TEXT("item_id"), NumericItemId) ||
				!(*ItemObject)->TryGetNumberField(TEXT("quantity"), NumericQuantity))
			{
				continue;
			}

			const int32 ItemId = static_cast<int32>(NumericItemId);
			if (!ItemDefinitionsById.Contains(ItemId))
			{
				UE_LOG(LogTunaSweeperItemData, Warning, TEXT("Skipping unknown item id %d in loot contents %d."), ItemId, Contents.Id);
				continue;
			}

			FTunaSweeperItemStack ItemStack;
			ItemStack.ItemId = ItemId;
			ItemStack.Quantity = FMath::Max(1, static_cast<int32>(NumericQuantity));
			Contents.Items.Add(ItemStack);
		}

		if (Contents.Id == INDEX_NONE || Contents.Items.Num() <= 0 || Contents.Items.Num() > 15)
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

void UTunaSweeperItemDataSubsystem::ResetLoadedItemData()
{
	ItemDefinitionsById.Reset();
	ItemNameStringsByKey.Reset();
	LootContainerDefinitionsById.Reset();
	LootContainerContentsById.Reset();
	bItemDataLoaded = false;
}

FString UTunaSweeperItemDataSubsystem::GetItemTableJsonPath() const
{
	return FPaths::Combine(FPaths::ProjectContentDir(), TunaSweeperItemDataFiles::ItemTableJsonRelativePath);
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
