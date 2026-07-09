using System.Globalization;
using System.Text;
using System.Text.Json;
using System.Text.Json.Nodes;
using System.Text.RegularExpressions;

namespace ItemDataCsvViewer;

internal sealed class ItemDataCsvExportResult
{
	public ItemDataCsvExportResult(string exportDirectory, string relationDocumentPath, IReadOnlyList<string> writtenFiles, IReadOnlyList<string> warnings)
	{
		ExportDirectory = exportDirectory;
		RelationDocumentPath = relationDocumentPath;
		WrittenFiles = writtenFiles;
		Warnings = warnings;
	}

	public string ExportDirectory { get; }

	public string RelationDocumentPath { get; }

	public IReadOnlyList<string> WrittenFiles { get; }

	public IReadOnlyList<string> Warnings { get; }
}

internal static class ItemDataCsvExporter
{
	private static readonly JsonDocumentOptions DocumentOptions = new()
	{
		AllowTrailingCommas = true,
		CommentHandling = JsonCommentHandling.Skip
	};

	private static readonly Dictionary<string, string> DefaultStackCategoryByItemCategory = new(StringComparer.OrdinalIgnoreCase)
	{
		["item.category.weapon.gun"] = "stack.default.weapon",
		["item.category.weapon.melee"] = "stack.default.weapon",
		["item.category.ammo"] = "stack.default.ammo",
		["item.category.attachment"] = "stack.default.attachment",
		["item.category.consumable"] = "stack.default.consumable",
		["item.category.body"] = "stack.default.equipment",
		["item.category.bag"] = "stack.default.equipment",
		["item.category.head"] = "stack.default.equipment",
		["item.category.face"] = "stack.default.equipment",
		["item.category.ear"] = "stack.default.equipment",
		["item.category.material"] = "stack.default.material",
		["item.category.blueprint"] = "stack.default.blueprint",
		["item.category.currency"] = "stack.default.currency"
	};

	private static readonly string[] ItemDefinitionColumns =
	[
		"source_row_index",
		"id",
		"name_string_key",
		"description_string_key",
		"shop_sell_price",
		"experience_value",
		"weight_kg",
		"item_grade",
		"icon_file_name",
		"icon_object_path",
		"category_tag",
		"max_stack_category_key",
		"resolved_stack_category_key",
		"resolved_max_stack_quantity",
		"blueprint_recipe_id",
		"equipment_slot_tag",
		"defense_value",
		"weapon_type_tag",
		"attachment_slot_tag",
		"ammo_type_tag",
		"projectile_hit_effect_id",
		"projectile_damage_multiplier",
		"projectile_damage_bonus",
		"magazine_capacity",
		"magazine_capacity_bonus",
		"reload_seconds",
		"inventory_slot_capacity",
		"carry_strength_bonus",
		"headphone_hearing_range",
		"headphone_sensitivity",
		"headphone_min_strength",
		"use_health_delta",
		"use_food_delta",
		"use_hydration_delta",
		"use_seconds"
	];

	public static ItemDataCsvExportResult Export(string projectRoot)
	{
		string dataDirectory = ProjectPaths.DataDirectory(projectRoot);
		string exportDirectory = ProjectPaths.ExportDirectory(projectRoot);
		string relationDocumentPath = ProjectPaths.RelationDocumentPath(projectRoot);
		Directory.CreateDirectory(exportDirectory);

		List<string> writtenFiles = [];
		List<string> warnings = [];

		JsonArray itemRows = ReadArray(Path.Combine(dataDirectory, "ItemTable.json"));
		JsonObject stackDefinitions = ReadObject(Path.Combine(dataDirectory, "ItemStackDefinitions.json"));
		JsonArray lootContainerRows = ReadArray(Path.Combine(dataDirectory, "LootContainerTable.json"));
		JsonArray lootContentRows = ReadArray(Path.Combine(dataDirectory, "LootContainerContents.json"), repairLootContents: true, warnings);
		JsonArray shopRows = ReadArray(Path.Combine(dataDirectory, "ShopDefinitions.json"));
		JsonArray recipeRows = ReadArray(Path.Combine(dataDirectory, "WorkbenchRecipes.json"));
		JsonArray dismantleRows = ReadArray(Path.Combine(dataDirectory, "WorkbenchDismantleRecipes.json"));

		Dictionary<int, Dictionary<string, string>> itemsById = [];
		Dictionary<string, int> stackQuantityByKey = BuildStackDefinitionRows(stackDefinitions, exportDirectory, writtenFiles);

		ExportItemDefinitions(itemRows, stackQuantityByKey, exportDirectory, writtenFiles, itemsById);
		ExportItemArrayRelations(itemRows, exportDirectory, writtenFiles);
		ExportLootContainers(lootContainerRows, exportDirectory, writtenFiles);
		ExportLootContents(lootContentRows, exportDirectory, writtenFiles);
		ExportShops(shopRows, itemsById, exportDirectory, writtenFiles);
		ExportWorkbenchRecipes(recipeRows, exportDirectory, writtenFiles);
		ExportWorkbenchDismantleRecipes(dismantleRows, exportDirectory, writtenFiles);
		WriteRelationDocument(projectRoot, relationDocumentPath, warnings);

		return new ItemDataCsvExportResult(exportDirectory, relationDocumentPath, writtenFiles, warnings);
	}

	private static Dictionary<string, int> BuildStackDefinitionRows(JsonObject stackDefinitions, string exportDirectory, List<string> writtenFiles)
	{
		List<Dictionary<string, string>> rows = [];
		Dictionary<string, int> quantitiesByKey = new(StringComparer.OrdinalIgnoreCase);
		foreach (KeyValuePair<string, JsonNode?> pair in stackDefinitions)
		{
			string stackKey = pair.Key.Trim();
			if (string.IsNullOrEmpty(stackKey))
			{
				continue;
			}

			int quantity = Math.Max(1, ReadInt(pair.Value, "quantity", "max_stack", "max_stack_quantity") ?? ReadInt(pair.Value) ?? 1);
			quantitiesByKey[stackKey] = quantity;
			rows.Add(new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase)
			{
				["stack_category_key"] = stackKey,
				["max_stack_quantity"] = Format(quantity)
			});
		}

		WriteTable(exportDirectory, "item_stack_definitions.csv", ["stack_category_key", "max_stack_quantity"], rows, writtenFiles);
		return quantitiesByKey;
	}

	private static void ExportItemDefinitions(
		JsonArray itemRows,
		IReadOnlyDictionary<string, int> stackQuantityByKey,
		string exportDirectory,
		List<string> writtenFiles,
		Dictionary<int, Dictionary<string, string>> itemsById)
	{
		List<Dictionary<string, string>> rows = [];
		for (int index = 0; index < itemRows.Count; ++index)
		{
			if (itemRows[index] is not JsonObject itemObject)
			{
				continue;
			}

			Dictionary<string, string> row = new(StringComparer.OrdinalIgnoreCase);
			foreach (string column in ItemDefinitionColumns)
			{
				row[column] = string.Empty;
			}

			row["source_row_index"] = Format(index);
			foreach (string column in ItemDefinitionColumns)
			{
				if (column is "source_row_index" or "icon_object_path" or "resolved_stack_category_key" or "resolved_max_stack_quantity")
				{
					continue;
				}

				row[column] = ReadString(itemObject, column);
			}

			string iconFileName = row["icon_file_name"];
			if (!string.IsNullOrWhiteSpace(iconFileName))
			{
				string iconAssetName = Path.GetFileNameWithoutExtension(iconFileName);
				row["icon_object_path"] = string.IsNullOrWhiteSpace(iconAssetName)
					? string.Empty
					: $"/Game/UI/Icons/{iconAssetName}.{iconAssetName}";
			}

			string resolvedStackCategoryKey = row["max_stack_category_key"];
			if (string.IsNullOrWhiteSpace(resolvedStackCategoryKey) &&
				DefaultStackCategoryByItemCategory.TryGetValue(row["category_tag"], out string? defaultStackCategoryKey))
			{
				resolvedStackCategoryKey = defaultStackCategoryKey;
			}

			row["resolved_stack_category_key"] = resolvedStackCategoryKey;
			row["resolved_max_stack_quantity"] = !string.IsNullOrWhiteSpace(resolvedStackCategoryKey) &&
				stackQuantityByKey.TryGetValue(resolvedStackCategoryKey, out int maxStackQuantity)
					? Format(maxStackQuantity)
					: "1";

			if (int.TryParse(row["id"], NumberStyles.Integer, CultureInfo.InvariantCulture, out int itemId))
			{
				itemsById[itemId] = row;
			}

			rows.Add(row);
		}

		WriteTable(exportDirectory, "item_definitions.csv", ItemDefinitionColumns, rows, writtenFiles);
	}

	private static void ExportItemArrayRelations(JsonArray itemRows, string exportDirectory, List<string> writtenFiles)
	{
		List<Dictionary<string, string>> attachmentSlotRows = [];
		List<Dictionary<string, string>> compatibleWeaponRows = [];
		List<Dictionary<string, string>> compatibleAmmoRows = [];
		List<Dictionary<string, string>> clearsDebuffRows = [];

		for (int rowIndex = 0; rowIndex < itemRows.Count; ++rowIndex)
		{
			if (itemRows[rowIndex] is not JsonObject itemObject)
			{
				continue;
			}

			string itemId = ReadString(itemObject, "id");
			AddStringArrayRows(itemObject, "attachment_slot_tags", itemId, "attachment_slot_tag", attachmentSlotRows);
			AddStringArrayRows(itemObject, "compatible_weapon_type_tags", itemId, "weapon_type_tag", compatibleWeaponRows);
			AddStringArrayRows(itemObject, "compatible_ammo_type_tags", itemId, "ammo_type_tag", compatibleAmmoRows);
			AddStringArrayRows(itemObject, "clears_debuff_ids", itemId, "debuff_id", clearsDebuffRows);
		}

		WriteTable(exportDirectory, "item_attachment_slot_tags.csv", ["item_id", "sort_order", "attachment_slot_tag"], attachmentSlotRows, writtenFiles);
		WriteTable(exportDirectory, "item_compatible_weapon_type_tags.csv", ["item_id", "sort_order", "weapon_type_tag"], compatibleWeaponRows, writtenFiles);
		WriteTable(exportDirectory, "item_compatible_ammo_type_tags.csv", ["item_id", "sort_order", "ammo_type_tag"], compatibleAmmoRows, writtenFiles);
		WriteTable(exportDirectory, "item_clears_debuff_ids.csv", ["item_id", "sort_order", "debuff_id"], clearsDebuffRows, writtenFiles);
	}

	private static void ExportLootContainers(JsonArray lootContainerRows, string exportDirectory, List<string> writtenFiles)
	{
		string[] columns = ["source_row_index", "id", "name_string_key", "capacity", "static_mesh_path", "material_path", "mesh_scale_x", "mesh_scale_y", "mesh_scale_z"];
		List<Dictionary<string, string>> rows = [];
		for (int index = 0; index < lootContainerRows.Count; ++index)
		{
			if (lootContainerRows[index] is not JsonObject containerObject)
			{
				continue;
			}

			JsonArray? meshScale = ReadArrayProperty(containerObject, "mesh_scale");
			rows.Add(new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase)
			{
				["source_row_index"] = Format(index),
				["id"] = ReadString(containerObject, "id"),
				["name_string_key"] = ReadString(containerObject, "name_string_key"),
				["capacity"] = ReadString(containerObject, "capacity"),
				["static_mesh_path"] = ReadString(containerObject, "static_mesh_path"),
				["material_path"] = ReadString(containerObject, "material_path"),
				["mesh_scale_x"] = ReadArrayScalar(meshScale, 0),
				["mesh_scale_y"] = ReadArrayScalar(meshScale, 1),
				["mesh_scale_z"] = ReadArrayScalar(meshScale, 2)
			});
		}

		WriteTable(exportDirectory, "loot_container_definitions.csv", columns, rows, writtenFiles);
	}

	private static void ExportLootContents(JsonArray lootContentRows, string exportDirectory, List<string> writtenFiles)
	{
		List<Dictionary<string, string>> contentRows = [];
		List<Dictionary<string, string>> itemRows = [];
		for (int index = 0; index < lootContentRows.Count; ++index)
		{
			if (lootContentRows[index] is not JsonObject contentObject)
			{
				continue;
			}

			string contentsId = ReadString(contentObject, "id");
			contentRows.Add(new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase)
			{
				["source_row_index"] = Format(index),
				["contents_id"] = contentsId,
				["memo_ko"] = ReadString(contentObject, "memo_ko")
			});

			JsonArray? items = ReadArrayProperty(contentObject, "items");
			if (items is null)
			{
				continue;
			}

			for (int itemIndex = 0; itemIndex < items.Count; ++itemIndex)
			{
				if (items[itemIndex] is not JsonObject itemObject)
				{
					continue;
				}

				string quantity = FirstNonEmpty(ReadString(itemObject, "quantity"), ReadString(itemObject, "count"));
				string quantityMin = FirstNonEmpty(ReadString(itemObject, "quantity_min"), ReadString(itemObject, "min_quantity"), quantity, "1");
				string quantityMax = FirstNonEmpty(ReadString(itemObject, "quantity_max"), ReadString(itemObject, "max_quantity"), quantityMin);
				itemRows.Add(new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase)
				{
					["contents_id"] = contentsId,
					["slot_index"] = Format(itemIndex),
					["item_id"] = ReadString(itemObject, "item_id"),
					["quantity"] = quantity,
					["quantity_min"] = quantityMin,
					["quantity_max"] = quantityMax,
					["drop_chance"] = FirstNonEmpty(ReadString(itemObject, "drop_chance"), ReadString(itemObject, "chance"), ReadString(itemObject, "probability"), ReadString(itemObject, "drop_chance_ratio"), "1")
				});
			}
		}

		WriteTable(exportDirectory, "loot_container_contents.csv", ["source_row_index", "contents_id", "memo_ko"], contentRows, writtenFiles);
		WriteTable(exportDirectory, "loot_container_items.csv", ["contents_id", "slot_index", "item_id", "quantity", "quantity_min", "quantity_max", "drop_chance"], itemRows, writtenFiles);
	}

	private static void ExportShops(JsonArray shopRows, IReadOnlyDictionary<int, Dictionary<string, string>> itemsById, string exportDirectory, List<string> writtenFiles)
	{
		List<Dictionary<string, string>> definitionRows = [];
		List<Dictionary<string, string>> itemRows = [];
		for (int index = 0; index < shopRows.Count; ++index)
		{
			if (shopRows[index] is not JsonObject shopObject)
			{
				continue;
			}

			string shopId = ReadString(shopObject, "shop_id");
			definitionRows.Add(new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase)
			{
				["source_row_index"] = Format(index),
				["shop_id"] = shopId,
				["name_string_key"] = ReadString(shopObject, "name_string_key")
			});

			JsonArray? items = ReadArrayProperty(shopObject, "items");
			if (items is null)
			{
				continue;
			}

			for (int itemIndex = 0; itemIndex < items.Count; ++itemIndex)
			{
				if (items[itemIndex] is not JsonObject itemObject)
				{
					continue;
				}

				string itemId = ReadString(itemObject, "item_id");
				string priceOverride = FirstNonEmpty(ReadString(itemObject, "price"), ReadString(itemObject, "buy_price"), ReadString(itemObject, "shop_price"));
				string resolvedBuyPrice = priceOverride;
				if (string.IsNullOrWhiteSpace(resolvedBuyPrice) &&
					int.TryParse(itemId, NumberStyles.Integer, CultureInfo.InvariantCulture, out int parsedItemId) &&
					itemsById.TryGetValue(parsedItemId, out Dictionary<string, string>? itemDefinition) &&
					itemDefinition.TryGetValue("shop_sell_price", out string? shopSellPrice))
				{
					resolvedBuyPrice = shopSellPrice;
				}

				itemRows.Add(new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase)
				{
					["shop_id"] = shopId,
					["slot_index"] = Format(itemIndex),
					["item_id"] = itemId,
					["stock_quantity"] = FirstNonEmpty(ReadString(itemObject, "stock_quantity"), ReadString(itemObject, "total_stock_quantity"), ReadString(itemObject, "stock"), ReadString(itemObject, "quantity")),
					["price_override"] = priceOverride,
					["resolved_buy_price"] = resolvedBuyPrice
				});
			}
		}

		WriteTable(exportDirectory, "shop_definitions.csv", ["source_row_index", "shop_id", "name_string_key"], definitionRows, writtenFiles);
		WriteTable(exportDirectory, "shop_items.csv", ["shop_id", "slot_index", "item_id", "stock_quantity", "price_override", "resolved_buy_price"], itemRows, writtenFiles);
	}

	private static void ExportWorkbenchRecipes(JsonArray recipeRows, string exportDirectory, List<string> writtenFiles)
	{
		List<Dictionary<string, string>> recipes = [];
		List<Dictionary<string, string>> ingredients = [];
		for (int index = 0; index < recipeRows.Count; ++index)
		{
			if (recipeRows[index] is not JsonObject recipeObject)
			{
				continue;
			}

			string recipeId = FirstNonEmpty(ReadString(recipeObject, "recipe_id"), ReadString(recipeObject, "id"));
			recipes.Add(new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase)
			{
				["source_row_index"] = Format(index),
				["recipe_id"] = recipeId,
				["workbench_id"] = FirstNonEmpty(ReadString(recipeObject, "workbench_id"), "1"),
				["name_string_key"] = ReadString(recipeObject, "name_string_key"),
				["output_item_id"] = FirstNonEmpty(ReadString(recipeObject, "output_item_id"), ReadString(recipeObject, "result_item_id"), ReadString(recipeObject, "item_id")),
				["output_quantity"] = FirstNonEmpty(ReadString(recipeObject, "output_quantity"), ReadString(recipeObject, "result_quantity"), ReadString(recipeObject, "quantity"), "1"),
				["auto_unlocked"] = FirstNonEmpty(ReadString(recipeObject, "auto_unlocked"), ReadString(recipeObject, "unlocked"), "true")
			});

			JsonArray? ingredientArray = ReadArrayProperty(recipeObject, "ingredients");
			if (ingredientArray is null)
			{
				continue;
			}

			for (int ingredientIndex = 0; ingredientIndex < ingredientArray.Count; ++ingredientIndex)
			{
				if (ingredientArray[ingredientIndex] is not JsonObject ingredientObject)
				{
					continue;
				}

				ingredients.Add(new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase)
				{
					["recipe_id"] = recipeId,
					["ingredient_index"] = Format(ingredientIndex),
					["item_id"] = ReadString(ingredientObject, "item_id"),
					["quantity"] = FirstNonEmpty(ReadString(ingredientObject, "quantity"), ReadString(ingredientObject, "count"), ReadString(ingredientObject, "required_quantity"), "1")
				});
			}
		}

		WriteTable(exportDirectory, "workbench_recipes.csv", ["source_row_index", "recipe_id", "workbench_id", "name_string_key", "output_item_id", "output_quantity", "auto_unlocked"], recipes, writtenFiles);
		WriteTable(exportDirectory, "workbench_recipe_ingredients.csv", ["recipe_id", "ingredient_index", "item_id", "quantity"], ingredients, writtenFiles);
	}

	private static void ExportWorkbenchDismantleRecipes(JsonArray dismantleRows, string exportDirectory, List<string> writtenFiles)
	{
		List<Dictionary<string, string>> recipes = [];
		List<Dictionary<string, string>> results = [];
		for (int index = 0; index < dismantleRows.Count; ++index)
		{
			if (dismantleRows[index] is not JsonObject dismantleObject)
			{
				continue;
			}

			string sourceItemId = FirstNonEmpty(ReadString(dismantleObject, "source_item_id"), ReadString(dismantleObject, "input_item_id"), ReadString(dismantleObject, "item_id"));
			recipes.Add(new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase)
			{
				["source_row_index"] = Format(index),
				["source_item_id"] = sourceItemId
			});

			JsonArray? resultArray = ReadArrayProperty(dismantleObject, "results") ?? ReadArrayProperty(dismantleObject, "outputs");
			if (resultArray is null)
			{
				continue;
			}

			for (int resultIndex = 0; resultIndex < resultArray.Count; ++resultIndex)
			{
				if (resultArray[resultIndex] is not JsonObject resultObject)
				{
					continue;
				}

				results.Add(new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase)
				{
					["source_item_id"] = sourceItemId,
					["result_index"] = Format(resultIndex),
					["item_id"] = ReadString(resultObject, "item_id"),
					["quantity"] = FirstNonEmpty(ReadString(resultObject, "quantity"), ReadString(resultObject, "count"), "1")
				});
			}
		}

		WriteTable(exportDirectory, "workbench_dismantle_recipes.csv", ["source_row_index", "source_item_id"], recipes, writtenFiles);
		WriteTable(exportDirectory, "workbench_dismantle_results.csv", ["source_item_id", "result_index", "item_id", "quantity"], results, writtenFiles);
	}

	private static void WriteRelationDocument(string projectRoot, string relationDocumentPath, IReadOnlyList<string> warnings)
	{
		string exportDirectory = Path.GetRelativePath(projectRoot, ProjectPaths.ExportDirectory(projectRoot)).Replace('\\', '/');
		StringBuilder builder = new();
		builder.AppendLine("# Item JSON Excel Export Relations");
		builder.AppendLine();
		builder.AppendLine("Generated CSV tables live under:");
		builder.AppendLine();
		builder.AppendLine($"- `{exportDirectory}`");
		builder.AppendLine();
		builder.AppendLine("The existing `TunaSweeper/Content/Data/ItemNameStrings.csv` remains the text table. Do not duplicate it into the export; join by `string_key` when display text is needed.");
		builder.AppendLine();
		builder.AppendLine("## Tables");
		builder.AppendLine();
		builder.AppendLine("| CSV | Source | Key | Notes |");
		builder.AppendLine("| --- | --- | --- | --- |");
		builder.AppendLine("| `item_definitions.csv` | `ItemTable.json` | `id` | One row per item. Array fields are moved to relation tables. `resolved_*` columns are derived from stack rules. |");
		builder.AppendLine("| `item_attachment_slot_tags.csv` | `ItemTable.json` | `item_id`, `sort_order` | Weapon attachment slots accepted by an item. |");
		builder.AppendLine("| `item_compatible_weapon_type_tags.csv` | `ItemTable.json` | `item_id`, `sort_order` | Attachment compatibility by weapon type. |");
		builder.AppendLine("| `item_compatible_ammo_type_tags.csv` | `ItemTable.json` | `item_id`, `sort_order` | Gun compatibility by ammo type. |");
		builder.AppendLine("| `item_clears_debuff_ids.csv` | `ItemTable.json` | `item_id`, `sort_order` | Consumable debuffs cleared on use. |");
		builder.AppendLine("| `item_stack_definitions.csv` | `ItemStackDefinitions.json` | `stack_category_key` | Max stack size per stack category. |");
		builder.AppendLine("| `loot_container_definitions.csv` | `LootContainerTable.json` | `id` | Container display and mesh data. `mesh_scale` is split into X/Y/Z columns. |");
		builder.AppendLine("| `loot_container_contents.csv` | `LootContainerContents.json` | `contents_id` | One row per loot content set. |");
		builder.AppendLine("| `loot_container_items.csv` | `LootContainerContents.json` | `contents_id`, `slot_index` | Items contained in each loot content set. |");
		builder.AppendLine("| `shop_definitions.csv` | `ShopDefinitions.json` | `shop_id` | One row per shop. |");
		builder.AppendLine("| `shop_items.csv` | `ShopDefinitions.json` | `shop_id`, `slot_index` | Items sold by each shop. `resolved_buy_price` uses override price first, then item sell price. |");
		builder.AppendLine("| `workbench_recipes.csv` | `WorkbenchRecipes.json` | `recipe_id` | Crafting recipe header rows. |");
		builder.AppendLine("| `workbench_recipe_ingredients.csv` | `WorkbenchRecipes.json` | `recipe_id`, `ingredient_index` | Ingredient rows per recipe. |");
		builder.AppendLine("| `workbench_dismantle_recipes.csv` | `WorkbenchDismantleRecipes.json` | `source_item_id` | Dismantle recipe header rows. |");
		builder.AppendLine("| `workbench_dismantle_results.csv` | `WorkbenchDismantleRecipes.json` | `source_item_id`, `result_index` | Dismantle output rows. |");
		builder.AppendLine();
		builder.AppendLine("## Relations");
		builder.AppendLine();
		builder.AppendLine("- `item_definitions.id` -> every `*_item_id` column.");
		builder.AppendLine("- `item_definitions.name_string_key` and `description_string_key` -> `ItemNameStrings.csv.string_key`.");
		builder.AppendLine("- `item_definitions.resolved_stack_category_key` -> `item_stack_definitions.stack_category_key`.");
		builder.AppendLine("- `shop_items.shop_id` -> `shop_definitions.shop_id`; `shop_items.item_id` -> `item_definitions.id`.");
		builder.AppendLine("- `loot_container_items.contents_id` -> `loot_container_contents.contents_id`; `loot_container_items.item_id` -> `item_definitions.id`.");
		builder.AppendLine("- `workbench_recipe_ingredients.recipe_id` -> `workbench_recipes.recipe_id`; ingredient and output item ids both point to `item_definitions.id`.");
		builder.AppendLine("- `workbench_dismantle_results.source_item_id` -> `workbench_dismantle_recipes.source_item_id`; source and result item ids both point to `item_definitions.id`.");
		builder.AppendLine();
		builder.AppendLine("## Viewer");
		builder.AppendLine();
		builder.AppendLine("Run `Tools/ItemDataCsvViewer/RunItemDataCsvViewer.bat`. The viewer can regenerate the CSV files and reload them immediately using the `Regenerate CSV` button.");
		builder.AppendLine("The `Relation Graph` tab redraws a visual graph for the currently selected item row, showing tag, shop, loot, crafting, and dismantle relations from the generated CSV tables.");
		if (warnings.Count > 0)
		{
			builder.AppendLine();
			builder.AppendLine("## Export Notes");
			builder.AppendLine();
			foreach (string warning in warnings)
			{
				builder.AppendLine($"- {warning}");
			}
		}

		Directory.CreateDirectory(Path.GetDirectoryName(relationDocumentPath) ?? ".");
		File.WriteAllText(relationDocumentPath, builder.ToString(), new UTF8Encoding(encoderShouldEmitUTF8Identifier: false));
	}

	private static void AddStringArrayRows(JsonObject itemObject, string arrayName, string itemId, string valueColumn, List<Dictionary<string, string>> rows)
	{
		JsonArray? values = ReadArrayProperty(itemObject, arrayName);
		if (values is null)
		{
			return;
		}

		for (int index = 0; index < values.Count; ++index)
		{
			string value = ReadScalar(values[index]);
			if (string.IsNullOrWhiteSpace(value))
			{
				continue;
			}

			rows.Add(new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase)
			{
				["item_id"] = itemId,
				["sort_order"] = Format(index),
				[valueColumn] = value
			});
		}
	}

	private static void WriteTable(string exportDirectory, string fileName, IReadOnlyList<string> columns, IEnumerable<IReadOnlyDictionary<string, string>> rows, List<string> writtenFiles)
	{
		string path = Path.Combine(exportDirectory, fileName);
		Csv.Write(path, columns, rows);
		writtenFiles.Add(path);
	}

	private static JsonArray ReadArray(string path, bool repairLootContents = false, List<string>? warnings = null)
	{
		JsonNode? node = ReadNode(path, repairLootContents, warnings);
		return node as JsonArray ?? throw new FormatException($"Expected JSON array: {path}");
	}

	private static JsonObject ReadObject(string path)
	{
		JsonNode? node = ReadNode(path, repairLootContents: false, warnings: null);
		return node as JsonObject ?? throw new FormatException($"Expected JSON object: {path}");
	}

	private static JsonNode? ReadNode(string path, bool repairLootContents, List<string>? warnings)
	{
		string json = File.ReadAllText(path, Encoding.UTF8);
		try
		{
			return JsonNode.Parse(json, documentOptions: DocumentOptions);
		}
		catch (JsonException) when (repairLootContents)
		{
			string repairedJson = RepairLootContentMemoStrings(json);
			JsonNode? repairedNode = JsonNode.Parse(repairedJson, documentOptions: DocumentOptions);
			warnings?.Add("`LootContainerContents.json` currently has unclosed `memo_ko` strings. The exporter repaired those memo fields in memory for CSV export; the source JSON was not modified.");
			return repairedNode;
		}
	}

	private static string RepairLootContentMemoStrings(string json)
	{
		return Regex.Replace(
			json,
			"(\"memo_ko\"\\s*:\\s*\")([^\"\\r\\n]*),\\s*(\\r?\\n\\s*\"items\"\\s*:)",
			"$1$2\",$3",
			RegexOptions.CultureInvariant);
	}

	private static JsonArray? ReadArrayProperty(JsonObject jsonObject, string propertyName)
	{
		return jsonObject.TryGetPropertyValue(propertyName, out JsonNode? node) ? node as JsonArray : null;
	}

	private static int? ReadInt(JsonNode? node, params string[] propertyNames)
	{
		if (propertyNames.Length > 0 && node is JsonObject jsonObject)
		{
			foreach (string propertyName in propertyNames)
			{
				if (jsonObject.TryGetPropertyValue(propertyName, out JsonNode? propertyNode))
				{
					int? parsed = ReadInt(propertyNode);
					if (parsed.HasValue)
					{
						return parsed;
					}
				}
			}

			return null;
		}

		string value = ReadScalar(node);
		return int.TryParse(value, NumberStyles.Integer, CultureInfo.InvariantCulture, out int parsedInt)
			? parsedInt
			: double.TryParse(value, NumberStyles.Float, CultureInfo.InvariantCulture, out double parsedDouble)
				? (int)Math.Round(parsedDouble)
				: null;
	}

	private static string ReadString(JsonObject jsonObject, string propertyName)
	{
		return jsonObject.TryGetPropertyValue(propertyName, out JsonNode? node) ? ReadScalar(node) : string.Empty;
	}

	private static string ReadArrayScalar(JsonArray? array, int index)
	{
		return array is not null && index >= 0 && index < array.Count ? ReadScalar(array[index]) : string.Empty;
	}

	private static string ReadScalar(JsonNode? node)
	{
		if (node is null)
		{
			return string.Empty;
		}

		if (node is JsonValue value)
		{
			if (value.TryGetValue(out string? stringValue))
			{
				return stringValue;
			}
			if (value.TryGetValue(out int intValue))
			{
				return Format(intValue);
			}
			if (value.TryGetValue(out long longValue))
			{
				return longValue.ToString(CultureInfo.InvariantCulture);
			}
			if (value.TryGetValue(out double doubleValue))
			{
				return Format(doubleValue);
			}
			if (value.TryGetValue(out bool boolValue))
			{
				return boolValue ? "true" : "false";
			}
		}

		return node.ToJsonString();
	}

	private static string FirstNonEmpty(params string[] values)
	{
		foreach (string value in values)
		{
			if (!string.IsNullOrWhiteSpace(value))
			{
				return value;
			}
		}

		return string.Empty;
	}

	private static string Format(int value)
	{
		return value.ToString(CultureInfo.InvariantCulture);
	}

	private static string Format(double value)
	{
		return value.ToString("0.########", CultureInfo.InvariantCulture);
	}
}
