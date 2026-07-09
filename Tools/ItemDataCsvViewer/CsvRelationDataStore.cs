using System.Data;
using System.Globalization;

namespace ItemDataCsvViewer;

internal sealed class CsvRelationDataStore
{
	private readonly Dictionary<string, CsvTable> tables = new(StringComparer.OrdinalIgnoreCase);
	private readonly Dictionary<string, Dictionary<string, string>> textStringsByKey = new(StringComparer.OrdinalIgnoreCase);
	private readonly Dictionary<int, Dictionary<string, string>> itemsById = [];

	public IReadOnlyList<Dictionary<string, string>> ItemRows { get; private set; } = [];

	public IReadOnlyList<Dictionary<string, string>> ShopRows { get; private set; } = [];

	public IReadOnlyList<Dictionary<string, string>> LootRows { get; private set; } = [];

	public IReadOnlyList<Dictionary<string, string>> RecipeRows { get; private set; } = [];

	public IReadOnlyList<Dictionary<string, string>> DismantleRows { get; private set; } = [];

	public RelationGraph BuildItemGraph(string itemId)
	{
		if (string.IsNullOrWhiteSpace(itemId) ||
			!int.TryParse(itemId, NumberStyles.Integer, CultureInfo.InvariantCulture, out int parsedItemId) ||
			!itemsById.TryGetValue(parsedItemId, out Dictionary<string, string>? item))
		{
			return RelationGraph.Empty;
		}

		string centerId = $"item:{itemId}";
		List<RelationGraphNode> nodes =
		[
			new(centerId, ItemLabel(itemId), $"{Get(item, "category_tag")} {Get(item, "item_grade")}".Trim(), "Item")
		];
		List<RelationGraphEdge> edges = [];

		AddTagNodes(nodes, edges, centerId, "item_attachment_slot_tags", "item_id", itemId, "attachment_slot_tag", "accepts slot");
		AddTagNodes(nodes, edges, centerId, "item_compatible_weapon_type_tags", "item_id", itemId, "weapon_type_tag", "weapon type");
		AddTagNodes(nodes, edges, centerId, "item_compatible_ammo_type_tags", "item_id", itemId, "ammo_type_tag", "ammo type");
		AddTagNodes(nodes, edges, centerId, "item_clears_debuff_ids", "item_id", itemId, "debuff_id", "clears");

		foreach (Dictionary<string, string> shopItem in Rows("shop_items").Where(row => Same(row, "item_id", itemId)))
		{
			string nodeId = $"shop:{Get(shopItem, "shop_id")}:{Get(shopItem, "slot_index")}";
			nodes.Add(new RelationGraphNode(
				nodeId,
				$"Shop {Get(shopItem, "shop_id")} slot {Get(shopItem, "slot_index")}",
				$"stock {Get(shopItem, "stock_quantity")} price {Get(shopItem, "resolved_buy_price")}",
				"Shop"));
			edges.Add(new RelationGraphEdge(centerId, nodeId, "sold in"));
		}

		foreach (Dictionary<string, string> lootItem in Rows("loot_container_items").Where(row => Same(row, "item_id", itemId)))
		{
			string nodeId = $"loot:{Get(lootItem, "contents_id")}:{Get(lootItem, "slot_index")}";
			nodes.Add(new RelationGraphNode(
				nodeId,
				$"Loot {Get(lootItem, "contents_id")}",
				$"x{QuantitySummary(lootItem)} chance {Get(lootItem, "drop_chance")}",
				"Loot"));
			edges.Add(new RelationGraphEdge(nodeId, centerId, "contains"));
		}

		foreach (Dictionary<string, string> recipe in Rows("workbench_recipes").Where(row => Same(row, "output_item_id", itemId)))
		{
			string nodeId = $"recipe-output:{Get(recipe, "recipe_id")}";
			nodes.Add(new RelationGraphNode(
				nodeId,
				Get(recipe, "recipe_id"),
				$"outputs x{Get(recipe, "output_quantity")}",
				"Craft"));
			edges.Add(new RelationGraphEdge(nodeId, centerId, "crafts"));
		}

		foreach (Dictionary<string, string> ingredient in Rows("workbench_recipe_ingredients").Where(row => Same(row, "item_id", itemId)))
		{
			string recipeId = Get(ingredient, "recipe_id");
			Dictionary<string, string>? recipe = Rows("workbench_recipes").FirstOrDefault(row => Same(row, "recipe_id", recipeId));
			string nodeId = $"recipe-ingredient:{recipeId}:{Get(ingredient, "ingredient_index")}";
			string outputItem = recipe is null ? string.Empty : ItemLabel(Get(recipe, "output_item_id"));
			nodes.Add(new RelationGraphNode(
				nodeId,
				recipeId,
				string.IsNullOrWhiteSpace(outputItem) ? $"uses x{Get(ingredient, "quantity")}" : $"uses x{Get(ingredient, "quantity")} for {outputItem}",
				"Craft"));
			edges.Add(new RelationGraphEdge(centerId, nodeId, "ingredient"));
		}

		foreach (Dictionary<string, string> result in Rows("workbench_dismantle_results").Where(row => Same(row, "source_item_id", itemId)))
		{
			string resultItemId = Get(result, "item_id");
			string nodeId = $"dismantle-result:{itemId}:{Get(result, "result_index")}";
			nodes.Add(new RelationGraphNode(
				nodeId,
				ItemLabel(resultItemId),
				$"result x{Get(result, "quantity")}",
				"Dismantle"));
			edges.Add(new RelationGraphEdge(centerId, nodeId, "dismantles"));
		}

		foreach (Dictionary<string, string> result in Rows("workbench_dismantle_results").Where(row => Same(row, "item_id", itemId)))
		{
			string sourceItemId = Get(result, "source_item_id");
			string nodeId = $"dismantle-source:{sourceItemId}:{Get(result, "result_index")}";
			nodes.Add(new RelationGraphNode(
				nodeId,
				ItemLabel(sourceItemId),
				$"source gives x{Get(result, "quantity")}",
				"Dismantle"));
			edges.Add(new RelationGraphEdge(nodeId, centerId, "dismantle result"));
		}

		return new RelationGraph(centerId, nodes, edges);
	}

	public static CsvRelationDataStore Load(string projectRoot)
	{
		CsvRelationDataStore store = new();
		store.LoadTables(ProjectPaths.ExportDirectory(projectRoot));
		store.LoadTextStrings(Path.Combine(ProjectPaths.DataDirectory(projectRoot), "ItemNameStrings.csv"));
		store.BuildIndexes();
		store.BuildViews();
		return store;
	}

	public static DataTable ToDataTable(IEnumerable<Dictionary<string, string>> rows)
	{
		List<Dictionary<string, string>> rowList = rows.ToList();
		DataTable table = new();
		foreach (string column in rowList.SelectMany(row => row.Keys).Distinct(StringComparer.OrdinalIgnoreCase))
		{
			table.Columns.Add(column);
		}

		foreach (Dictionary<string, string> row in rowList)
		{
			DataRow dataRow = table.NewRow();
			foreach (DataColumn column in table.Columns)
			{
				dataRow[column.ColumnName] = row.TryGetValue(column.ColumnName, out string? value) ? value : string.Empty;
			}

			table.Rows.Add(dataRow);
		}

		return table;
	}

	private void LoadTables(string exportDirectory)
	{
		string[] fileNames =
		[
			"item_definitions.csv",
			"item_attachment_slot_tags.csv",
			"item_compatible_weapon_type_tags.csv",
			"item_compatible_ammo_type_tags.csv",
			"item_clears_debuff_ids.csv",
			"item_stack_definitions.csv",
			"loot_container_definitions.csv",
			"loot_container_contents.csv",
			"loot_container_items.csv",
			"shop_definitions.csv",
			"shop_items.csv",
			"workbench_recipes.csv",
			"workbench_recipe_ingredients.csv",
			"workbench_dismantle_recipes.csv",
			"workbench_dismantle_results.csv"
		];

		foreach (string fileName in fileNames)
		{
			string tableName = Path.GetFileNameWithoutExtension(fileName);
			tables[tableName] = Csv.Read(Path.Combine(exportDirectory, fileName), tableName);
		}
	}

	private void LoadTextStrings(string itemNameStringsPath)
	{
		CsvTable textTable = Csv.Read(itemNameStringsPath, "ItemNameStrings");
		foreach (Dictionary<string, string> row in textTable.Rows)
		{
			string key = Get(row, "string_key");
			if (!string.IsNullOrWhiteSpace(key))
			{
				textStringsByKey[key] = row;
			}
		}
	}

	private void BuildIndexes()
	{
		itemsById.Clear();
		foreach (Dictionary<string, string> item in Rows("item_definitions"))
		{
			if (TryGetInt(item, "id", out int itemId))
			{
				itemsById[itemId] = item;
			}
		}
	}

	private void BuildViews()
	{
		ItemRows = BuildItemRows();
		ShopRows = BuildShopRows();
		LootRows = BuildLootRows();
		RecipeRows = BuildRecipeRows();
		DismantleRows = BuildDismantleRows();
	}

	private IReadOnlyList<Dictionary<string, string>> BuildItemRows()
	{
		List<Dictionary<string, string>> rows = [];
		foreach (Dictionary<string, string> item in Rows("item_definitions"))
		{
			string itemId = Get(item, "id");
			Dictionary<string, string> row = CopySelected(
				item,
				"id",
				"name_string_key",
				"description_string_key",
				"category_tag",
				"item_grade",
				"shop_sell_price",
				"resolved_max_stack_quantity",
				"weight_kg",
				"equipment_slot_tag",
				"weapon_type_tag",
				"ammo_type_tag",
				"attachment_slot_tag",
				"blueprint_recipe_id",
				"icon_file_name");
			row["name_ko"] = Text(Get(item, "name_string_key"), "ko");
			row["name_en"] = Text(Get(item, "name_string_key"), "en");
			row["description_ko"] = Text(Get(item, "description_string_key"), "ko");
			row["description_en"] = Text(Get(item, "description_string_key"), "en");
			row["attachment_slots"] = JoinValues("item_attachment_slot_tags", "item_id", itemId, "attachment_slot_tag");
			row["compatible_weapon_types"] = JoinValues("item_compatible_weapon_type_tags", "item_id", itemId, "weapon_type_tag");
			row["compatible_ammo_types"] = JoinValues("item_compatible_ammo_type_tags", "item_id", itemId, "ammo_type_tag");
			row["clears_debuffs"] = JoinValues("item_clears_debuff_ids", "item_id", itemId, "debuff_id");
			row["shop_entries"] = string.Join("; ", Rows("shop_items")
				.Where(shopItem => Same(shopItem, "item_id", itemId))
				.Select(shopItem => $"shop {Get(shopItem, "shop_id")} slot {Get(shopItem, "slot_index")} stock {Get(shopItem, "stock_quantity")} price {Get(shopItem, "resolved_buy_price")}"));
			row["loot_entries"] = string.Join("; ", Rows("loot_container_items")
				.Where(lootItem => Same(lootItem, "item_id", itemId))
				.Select(lootItem => $"contents {Get(lootItem, "contents_id")} x{QuantitySummary(lootItem)} chance {Get(lootItem, "drop_chance")}"));
			row["crafted_by"] = string.Join("; ", Rows("workbench_recipes")
				.Where(recipe => Same(recipe, "output_item_id", itemId))
				.Select(recipe => $"{Get(recipe, "recipe_id")} x{Get(recipe, "output_quantity")}"));
			row["used_as_ingredient"] = string.Join("; ", Rows("workbench_recipe_ingredients")
				.Where(ingredient => Same(ingredient, "item_id", itemId))
				.Select(ingredient => $"{Get(ingredient, "recipe_id")} x{Get(ingredient, "quantity")}"));
			row["dismantles_into"] = string.Join("; ", Rows("workbench_dismantle_results")
				.Where(result => Same(result, "source_item_id", itemId))
				.Select(result => $"{ItemLabel(Get(result, "item_id"))} x{Get(result, "quantity")}"));
			row["dismantle_result_of"] = string.Join("; ", Rows("workbench_dismantle_results")
				.Where(result => Same(result, "item_id", itemId))
				.Select(result => $"{ItemLabel(Get(result, "source_item_id"))} x{Get(result, "quantity")}"));
			rows.Add(row);
		}

		return rows;
	}

	private IReadOnlyList<Dictionary<string, string>> BuildShopRows()
	{
		List<Dictionary<string, string>> rows = [];
		foreach (Dictionary<string, string> shopItem in Rows("shop_items"))
		{
			string shopId = Get(shopItem, "shop_id");
			Dictionary<string, string>? shopDefinition = Rows("shop_definitions").FirstOrDefault(row => Same(row, "shop_id", shopId));
			Dictionary<string, string> row = CopySelected(shopItem, "shop_id", "slot_index", "item_id", "stock_quantity", "price_override", "resolved_buy_price");
			row["shop_name_string_key"] = shopDefinition is null ? string.Empty : Get(shopDefinition, "name_string_key");
			row["shop_name_en"] = Text(row["shop_name_string_key"], "en");
			row["item"] = ItemLabel(Get(shopItem, "item_id"));
			row["item_name_en"] = ItemText(Get(shopItem, "item_id"), "name_string_key", "en");
			row["item_category"] = ItemField(Get(shopItem, "item_id"), "category_tag");
			rows.Add(row);
		}

		return rows;
	}

	private IReadOnlyList<Dictionary<string, string>> BuildLootRows()
	{
		List<Dictionary<string, string>> rows = [];
		foreach (Dictionary<string, string> lootItem in Rows("loot_container_items"))
		{
			string contentsId = Get(lootItem, "contents_id");
			Dictionary<string, string>? contents = Rows("loot_container_contents").FirstOrDefault(row => Same(row, "contents_id", contentsId));
			Dictionary<string, string> row = CopySelected(lootItem, "contents_id", "slot_index", "item_id", "quantity", "quantity_min", "quantity_max", "drop_chance");
			row["item"] = ItemLabel(Get(lootItem, "item_id"));
			row["item_name_en"] = ItemText(Get(lootItem, "item_id"), "name_string_key", "en");
			row["item_category"] = ItemField(Get(lootItem, "item_id"), "category_tag");
			row["memo_ko"] = contents is null ? string.Empty : Get(contents, "memo_ko");
			rows.Add(row);
		}

		return rows;
	}

	private IReadOnlyList<Dictionary<string, string>> BuildRecipeRows()
	{
		List<Dictionary<string, string>> rows = [];
		foreach (Dictionary<string, string> recipe in Rows("workbench_recipes"))
		{
			string recipeId = Get(recipe, "recipe_id");
			Dictionary<string, string> row = CopySelected(recipe, "recipe_id", "workbench_id", "name_string_key", "output_item_id", "output_quantity", "auto_unlocked");
			row["output_item"] = ItemLabel(Get(recipe, "output_item_id"));
			row["recipe_name_en"] = Text(Get(recipe, "name_string_key"), "en");
			row["ingredients"] = string.Join("; ", Rows("workbench_recipe_ingredients")
				.Where(ingredient => Same(ingredient, "recipe_id", recipeId))
				.Select(ingredient => $"{ItemLabel(Get(ingredient, "item_id"))} x{Get(ingredient, "quantity")}"));
			rows.Add(row);
		}

		return rows;
	}

	private IReadOnlyList<Dictionary<string, string>> BuildDismantleRows()
	{
		List<Dictionary<string, string>> rows = [];
		foreach (Dictionary<string, string> recipe in Rows("workbench_dismantle_recipes"))
		{
			string sourceItemId = Get(recipe, "source_item_id");
			Dictionary<string, string> row = CopySelected(recipe, "source_item_id");
			row["source_item"] = ItemLabel(sourceItemId);
			row["results"] = string.Join("; ", Rows("workbench_dismantle_results")
				.Where(result => Same(result, "source_item_id", sourceItemId))
				.Select(result => $"{ItemLabel(Get(result, "item_id"))} x{Get(result, "quantity")}"));
			rows.Add(row);
		}

		return rows;
	}

	private IEnumerable<Dictionary<string, string>> Rows(string tableName)
	{
		return tables.TryGetValue(tableName, out CsvTable? table) ? table.Rows : [];
	}

	private string JoinValues(string tableName, string keyColumn, string keyValue, string valueColumn)
	{
		return string.Join("; ", Rows(tableName)
			.Where(row => Same(row, keyColumn, keyValue))
			.OrderBy(row => TryGetInt(row, "sort_order", out int sortOrder) ? sortOrder : 0)
			.Select(row => Get(row, valueColumn)));
	}

	private void AddTagNodes(
		List<RelationGraphNode> nodes,
		List<RelationGraphEdge> edges,
		string centerId,
		string tableName,
		string keyColumn,
		string keyValue,
		string valueColumn,
		string edgeLabel)
	{
		foreach (Dictionary<string, string> row in Rows(tableName).Where(candidate => Same(candidate, keyColumn, keyValue)))
		{
			string value = Get(row, valueColumn);
			if (string.IsNullOrWhiteSpace(value))
			{
				continue;
			}

			string nodeId = $"tag:{tableName}:{value}";
			nodes.Add(new RelationGraphNode(nodeId, value, tableName.Replace("item_", string.Empty, StringComparison.OrdinalIgnoreCase), "Tag"));
			edges.Add(new RelationGraphEdge(centerId, nodeId, edgeLabel));
		}
	}

	private string Text(string key, string languageColumn)
	{
		return textStringsByKey.TryGetValue(key, out Dictionary<string, string>? row) ? Get(row, languageColumn) : string.Empty;
	}

	private string ItemLabel(string itemId)
	{
		string name = ItemText(itemId, "name_string_key", "en");
		return string.IsNullOrWhiteSpace(name) ? itemId : $"{itemId} {name}";
	}

	private string ItemText(string itemId, string keyColumn, string languageColumn)
	{
		return int.TryParse(itemId, NumberStyles.Integer, CultureInfo.InvariantCulture, out int parsedItemId) &&
			itemsById.TryGetValue(parsedItemId, out Dictionary<string, string>? item)
				? Text(Get(item, keyColumn), languageColumn)
				: string.Empty;
	}

	private string ItemField(string itemId, string fieldName)
	{
		return int.TryParse(itemId, NumberStyles.Integer, CultureInfo.InvariantCulture, out int parsedItemId) &&
			itemsById.TryGetValue(parsedItemId, out Dictionary<string, string>? item)
				? Get(item, fieldName)
				: string.Empty;
	}

	private static string QuantitySummary(IReadOnlyDictionary<string, string> row)
	{
		string quantity = Get(row, "quantity");
		if (!string.IsNullOrWhiteSpace(quantity))
		{
			return quantity;
		}

		string min = Get(row, "quantity_min");
		string max = Get(row, "quantity_max");
		return string.Equals(min, max, StringComparison.OrdinalIgnoreCase) ? min : $"{min}-{max}";
	}

	private static Dictionary<string, string> CopySelected(IReadOnlyDictionary<string, string> source, params string[] columns)
	{
		Dictionary<string, string> row = new(StringComparer.OrdinalIgnoreCase);
		foreach (string column in columns)
		{
			row[column] = Get(source, column);
		}

		return row;
	}

	private static bool Same(IReadOnlyDictionary<string, string> row, string column, string expected)
	{
		return string.Equals(Get(row, column), expected, StringComparison.OrdinalIgnoreCase);
	}

	private static string Get(IReadOnlyDictionary<string, string> row, string column)
	{
		return row.TryGetValue(column, out string? value) ? value : string.Empty;
	}

	private static bool TryGetInt(IReadOnlyDictionary<string, string> row, string column, out int value)
	{
		return int.TryParse(Get(row, column), NumberStyles.Integer, CultureInfo.InvariantCulture, out value);
	}
}
