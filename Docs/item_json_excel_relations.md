# Item JSON Excel Export Relations

Generated CSV tables live under:

- `TunaSweeper/Content/Data/ExcelExport`

The existing `TunaSweeper/Content/Data/ItemNameStrings.csv` remains the text table. Do not duplicate it into the export; join by `string_key` when display text is needed.

## Tables

| CSV | Source | Key | Notes |
| --- | --- | --- | --- |
| `item_definitions.csv` | `ItemTable.json` | `id` | One row per item. Array fields are moved to relation tables. `resolved_*` columns are derived from stack rules. |
| `item_attachment_slot_tags.csv` | `ItemTable.json` | `item_id`, `sort_order` | Weapon attachment slots accepted by an item. |
| `item_compatible_weapon_type_tags.csv` | `ItemTable.json` | `item_id`, `sort_order` | Attachment compatibility by weapon type. |
| `item_compatible_ammo_type_tags.csv` | `ItemTable.json` | `item_id`, `sort_order` | Gun compatibility by ammo type. |
| `item_clears_debuff_ids.csv` | `ItemTable.json` | `item_id`, `sort_order` | Consumable debuffs cleared on use. |
| `item_stack_definitions.csv` | `ItemStackDefinitions.json` | `stack_category_key` | Max stack size per stack category. |
| `loot_container_definitions.csv` | `LootContainerTable.json` | `id` | Container display and mesh data. `mesh_scale` is split into X/Y/Z columns. |
| `loot_container_contents.csv` | `LootContainerContents.json` | `contents_id` | One row per loot content set. |
| `loot_container_items.csv` | `LootContainerContents.json` | `contents_id`, `slot_index` | Items contained in each loot content set. |
| `shop_definitions.csv` | `ShopDefinitions.json` | `shop_id` | One row per shop. |
| `shop_items.csv` | `ShopDefinitions.json` | `shop_id`, `slot_index` | Items sold by each shop. `resolved_buy_price` uses override price first, then item sell price. |
| `workbench_recipes.csv` | `WorkbenchRecipes.json` | `recipe_id` | Crafting recipe header rows. |
| `workbench_recipe_ingredients.csv` | `WorkbenchRecipes.json` | `recipe_id`, `ingredient_index` | Ingredient rows per recipe. |
| `workbench_dismantle_recipes.csv` | `WorkbenchDismantleRecipes.json` | `source_item_id` | Dismantle recipe header rows. |
| `workbench_dismantle_results.csv` | `WorkbenchDismantleRecipes.json` | `source_item_id`, `result_index` | Dismantle output rows. |

## Relations

- `item_definitions.id` -> every `*_item_id` column.
- `item_definitions.name_string_key` and `description_string_key` -> `ItemNameStrings.csv.string_key`.
- `item_definitions.resolved_stack_category_key` -> `item_stack_definitions.stack_category_key`.
- `shop_items.shop_id` -> `shop_definitions.shop_id`; `shop_items.item_id` -> `item_definitions.id`.
- `loot_container_items.contents_id` -> `loot_container_contents.contents_id`; `loot_container_items.item_id` -> `item_definitions.id`.
- `workbench_recipe_ingredients.recipe_id` -> `workbench_recipes.recipe_id`; ingredient and output item ids both point to `item_definitions.id`.
- `workbench_dismantle_results.source_item_id` -> `workbench_dismantle_recipes.source_item_id`; source and result item ids both point to `item_definitions.id`.

## Viewer

Run `Tools/ItemDataCsvViewer/RunItemDataCsvViewer.bat`. The viewer can regenerate the CSV files and reload them immediately using the `Regenerate CSV` button.
The `Relation Graph` tab redraws a visual graph for the currently selected item row, showing tag, shop, loot, crafting, and dismantle relations from the generated CSV tables.
