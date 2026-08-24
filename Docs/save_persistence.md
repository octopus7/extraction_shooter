# Save/Load Persistence Contract

This file tracks gameplay/runtime state that must survive `UTunaSweeperSaveGame` save/load.
Update it whenever a new state field is expected to persist across save slots, level travel saves, death saves, or intro-menu reloads.

## Current Save Container

- Save object: `UTunaSweeperSaveGame`
- Current save version: `20`
- Runtime owner: `UTunaSweeperGameInstance`
- Save entry point: `UTunaSweeperGameInstance::SaveGameStateInternal()`
- Load entry point: `UTunaSweeperGameInstance::LoadGameState()`

## Fail-Closed File Commit

Gameplay slots and the last-selected-slot settings file use the same fail-closed file commit path. The existing active file is never written in place.

1. Serialize the new save into a CRC-protected envelope and write it to `<active>.candidate`.
2. Fully flush the candidate to disk, reload it, verify the envelope length/CRC, deserialize it, and validate its save class. Gameplay slots additionally validate save version, build flavor, and slot index.
3. If candidate validation fails, delete only the candidate and report save failure. The active file remains untouched.
4. Before promotion, copy the current valid active file into `<active>.previous` through its own flushed and verified candidate.
5. Promote the validated candidate over the active path, then reload and validate the promoted active file. Any promotion/validation failure reports save failure and restores from the previous verified generation when possible.

An uncommitted candidate left by forced termination is deleted on the next load and is never promoted. Load order is active file, previous verified generation, then newest valid timestamped backup. Recovery candidates must pass the same CRC/class/version/flavor/slot validation before they can repair the active file. If no valid generation exists, the slot remains occupied but unloadable so ordinary save calls cannot silently overwrite it; only an explicit slot deletion can discard its artifacts.

Legacy raw Unreal `.sav` files remain readable. Their next successful save rewrites the active generation using the CRC envelope. Explicit slot deletion removes the active, candidate, previous, and timestamped backup files for that slot so deleted data cannot be resurrected by recovery.

## Save Timing Rules

### Bunker Item Mutation Deferral

Bunker item ownership/layout mutations must be saved after the player leaves the blocking UI and returns to a gameplay-capable mode. This includes purchases, sales, storage moves, inventory/equipment/quick-slot layout edits, weapon attachment changes, and future bunker-only item transactions that modify persisted item instances or slot arrays.

While the responsible UI is alive, the mutation should only mark a pending bunker item save in runtime memory. Multiple mutations during the same UI session coalesce into one pending save. The pending save must be flushed immediately when the HUD/input state becomes gameplay-capable again: no inventory-only panel, external storage/shop/workbench panel, attachment edit UI, modal confirmation, or other blocking item UI remains open, and the player can resume normal bunker gameplay controls.

The gameplay-capable flush point is a player-facing UX rule, not a purely theoretical engine-state rule. If a player would reasonably feel that control has returned or that delaying the save is uncomfortable, prefer the earlier/comfortable flush point. Minor differences between UI flows are acceptable when they match player perception; do not delay a save just to make every flow satisfy an abstractly identical state checklist.

The flush uses the normal active-slot save path through `UTunaSweeperGameInstance::SaveGameStateInternal()` with runtime quick slots persisted. If another guaranteed save writes the same active slot before gameplay becomes available, it may clear the pending bunker item save only after the in-memory item state has been included in that write.

Raid item changes keep their existing extraction/death/level-travel save rules and are not covered by this bunker UI deferral rule.

## Persisted State

### Save Metadata

- `SaveVersion`
- `SaveSlotIndex`
- `BuildFlavor`: build target identity (`Demo` or `Main`). A save whose value does not match the running target is rejected.
- `TotalPlaySeconds`
- `DifficultyStage`: save-slot difficulty stage, clamped to `1..3`; `1` is Farming, `2` is Normal, and `3` is Hard. Main slots keep the default `1` until the player confirms a difficulty. Demo slots atomically initialize to fixed stage `2` when the Demo notice is confirmed.
- `bDifficultySelected`: whether the slot completed its required start gate. For Main this is difficulty confirmation. For Demo this is the notice confirmation written together with the first slot save; no Demo save is created before confirmation.
- `LastSavedAtTicks`

Demo and Main use the same logical slot-name format but separate physical roots: `Saved/SaveGames/Demo/` and `Saved/SaveGames/Main/`. Demo exposes and accepts only `TunaSweeperSave_Slot01`; Main exposes slots `TunaSweeperSave_Slot01` through `03`. Each root independently owns last-selected-slot settings, backups, and deletion logs. The directory boundary and `BuildFlavor` field together prevent progress, rewards, inventory, currency, unlocks, and world state from crossing targets.

### Obsolete Save Deletion

Save version 20 is the minimum supported version. During `UTunaSweeperGameInstance::Init()`, every loadable `UTunaSweeperSaveGame` file below version 20 under the active `Demo` or `Main` root, including backups, is deleted. Slot lookup repeats the same check so an obsolete file introduced after initialization cannot block a new save. Save settings and unreadable files inside the active root are not auto-deleted.

Every successful obsolete-version deletion appends a local-time timestamp, detected version, and path relative to the active root to that root's `AutoDeletedSaveLog.txt`. The game prepares the audit log before deleting; if the log cannot be prepared, it leaves the versioned save file in place and reports an error through the Unreal log.

Legacy `.sav` files directly under `Saved/SaveGames/` are not migrated. They are deleted at startup regardless of version and their file names are appended to the active target's deletion log. The `Demo/` and `Main/` subdirectories are never included in this flat-file cleanup.

### Scenario Progress Flags

Scenario progress is persisted per save slot through `UTunaSweeperSaveGame::CompletedScenarioFlags`.
See [Docs/scenario_progress_flags.md](scenario_progress_flags.md) for the flag contract, routing rules, and reuse constraints.
The active scenario data pack supplies dialogue completion flags. Demo uses `dialogue.demo.toilet_intro`, while Main uses `dialogue.main.bunker_intro` after `scenario.opening.awakening`; the retired test-dialogue flag `dialogue.mole.bunker_intro` is intentionally left as inert legacy save data.

### Memo Unlocks

Collected memo ids are persisted per save slot through `UTunaSweeperSaveGame::AcquiredMemoIds`.

Memo interaction marks the id as acquired in `UTunaSweeperGameInstance` memory immediately and destroys the spawned memo actor. It does not force an immediate save by default; the acquired ids are written on the next normal save, level-travel save, death save, or any other call to `SaveGameStateInternal()`. Runtime memo spawns must query this in-memory set so a collected memo stays hidden after death/respawn even before the next disk write.

### Item Acquisition History

Item ids that the player has acquired at least once are persisted per save slot through `UTunaSweeperSaveGame::EverAcquiredItemIds`.

This history is separate from current item ownership. It is updated when items enter player inventory through pickups, rewards, purchases, crafting, and similar acquisition paths, and older saves are backfilled from currently saved item instances on load. Housing facility definitions can use this history for one-time unlock conditions, such as unlocking the piggy bank facility after the player has ever owned an ancient coin or ancient banknote.

### Map Markers

Player-created map markers are persisted per save slot through `UTunaSweeperSaveGame::MapMarkers`.

Each `FTunaSweeperMapMarkerSaveData` preserves:

- `MarkerId`: stable runtime id used for delete/update targeting.
- `MapPosition`: normalized map-space position where `(0, 0)` is the top-left of the current map image and `(1, 1)` is the bottom-right.
- `MarkerIconIndex`: fixed marker icon palette index.
- `MarkerColorIndex`: fixed marker color palette index.

Marker add/delete actions update `UTunaSweeperGameInstance` memory immediately and broadcast `OnMapMarkersChanged`. They do not force an immediate save by default; the marker array is written on the next normal save, level-travel save, death save, or any other call to `SaveGameStateInternal()`.

### Player-Owned Item Instances

Stored through `UTunaSweeperSaveGame::ItemInstances`.

Confirming the Demo notice creates the single Demo save slot with a starting rifle (`1002`) in equipment weapon slot 1 and 60 total rifle rounds (`2002`): 30 rounds loaded in the rifle and 30 rounds in the first inventory slot. The rifle stores `2002` as both its loaded and selected ammo item id, and weapon slot 1 is selected for the immediate Bunker entry. This loadout is initialized only while creating a new Demo slot; Main saves and already-existing Demo saves are not modified.

Each `FTunaSweeperItemInstance` must preserve:

- `Uid`
- `ItemId`
- `Quantity`
- `AttachmentSlots`
- `LoadedAmmoItemId`
- `LoadedAmmoCount`
- `SelectedAmmoItemId` as compatibility mirror of `LoadedAmmoItemId`

Weapon loaded ammo state is part of the weapon item instance, not player-global state.
When a weapon is loaded from a save or equipped later, ammo type and loaded count must be read from that weapon instance.
Weapon attachment slots are keyed by attachment slot tags. Rifle instances may persist `attachment.slot.magazine`, `attachment.slot.optic`, and `attachment.slot.tactical`; the tactical slot currently stores the laser sight item when equipped.

### Player Slot Layout

- `InventorySlots`
- `EquipmentSlots`
- `AuxiliaryBagSlots`
- `UsableQuickSlots`

Slot arrays store item UIDs. Any item UID referenced by these slots, including nested attachment UIDs, must also exist in `ItemInstances`. `FTunaSweeperInventorySlot::bSortLocked` persists the player's inventory sort-lock state; the inventory compact/sort button leaves locked inventory slots in place and only trims unlocked inventory slots around them.

`UsableQuickSlots` stores the 3-8 quick-slot layout shown in inventory mode and reflected in the gameplay quick-slot bar. Slots 1, 2, and melee are equipment slots and are not duplicated here. Only usable items can occupy these slots; currently that means item definitions tagged `item.category.consumable` or `item.category.throwable`.

Unlike memo unlocks and map markers, usable quick slots are item-possession state. Generic saves preserve the previously saved quick-slot payload without rewriting it, successful RaidMap-to-BunkerMap extraction saves write the current runtime quick slots, and death saves clear all usable quick slots.

### Storage Slots

The bunker-only storage/warehouse is persisted per save slot through:

- `StorageSlotCapacity`
- `StorageSlots`

New save slots initialize storage with `100` slots. `StorageSlotCapacity` is saved separately from the slot array so future upgrades can expand capacity without changing the storage item layout. Storage slot arrays store item UIDs just like inventory slots, and any item UID referenced by storage, including nested attachment UIDs, must exist in `ItemInstances`. The storage compact/sort button rewrites `StorageSlots` into first-available unlocked slots while preserving any `bSortLocked` slot positions.

Storage is accessible only in `BunkerMap`: entering inventory mode in the bunker opens the storage panel at the same HUD location used by loot containers, and `ATunaSweeperStorageActor` opens the same panel through `StorageOpen` interaction. Raid maps do not offer or open storage. Death persistence clears carried inventory, equipment, auxiliary bag, and usable quick slots, but preserves storage slots and their item instances.

### Shop Stock

Bunker shop stock is persisted per save slot through `UTunaSweeperSaveGame::ShopStockStates`.

Each `FTunaSweeperShopStockSaveData` preserves:

- `ShopId`: stable shop identifier assigned on `ATunaSweeperShopActor`.
- `SlotIndex`: index of the item entry in `Content/Data/ShopDefinitions.json`.
- `ItemId`: item id from the shop entry, included to guard against reordered or changed definitions.
- `StockQuantity`: remaining stock for that shop entry.

Shop definitions are static data in `Content/Data/ShopDefinitions.json`. New saves start from each entry's defined `stock_quantity`; purchases decrease the runtime stock and immediately save the remaining stock with the inventory/currency transaction. Shop access is bunker-only through `ShopOpen` interaction. Item sale does not affect shop stock; it removes the player-owned item and grants half of the item table `shop_sell_price`.

### Piggy Bank Deposits

Piggy bank deposits are persisted per save slot through `UTunaSweeperSaveGame::PiggyBankStates`.

Each `FTunaSweeperPiggyBankSaveData` preserves:

- `PiggyBankId`: stable id from the gameplay interaction spawn id or housing placed facility instance id.
- `StoredAncientCoinValue`: accumulated value from deposited ancient coin and ancient banknote items.

Ancient coin and ancient banknote are item instances, not player currency. Depositing removes those item instances from carried inventory and adds to the piggy bank state; one ancient banknote contributes ten ancient-coin value. Withdraw interaction is currently exposed as a placeholder only and does not mutate save data.

### Workbench Recipe Unlocks

Workbench recipe unlocks are persisted per save slot through `UTunaSweeperSaveGame::UnlockedWorkbenchRecipeIds`.

Each entry is a stable recipe id from `Content/Data/WorkbenchRecipes.json`. Recipes with `auto_unlocked` remain available without being written to the save. Non-default recipes can be added by quest rewards or by registering a blueprint item whose item definition points at a `BlueprintRecipeId`; older saves also derive missing unlocks from already completed quest rewards during load.

### World Progress Objects

Stored through `UTunaSweeperSaveGame::WorldProgressStates`.

Each `FTunaSweeperWorldProgressSaveData` preserves:

- `ObjectId`: stable per-level object identifier from runtime spawn data or a directly placed level actor.
- `InfoId`: reusable progress-object information id, such as a broken bridge or blocked entrance type.
- `State`: `InProgress` or `Completed`.
- `ProgressQuantity`: contributed material count for in-progress objects.

Raid world progress actors restore this state on spawn. Completed repair objects disable their blocking collision and spawn their configured completed replacement actor at the same transform.

Persistent door actors also use `WorldProgressStates`: once opened, they write `Completed` for their stable door `ObjectId`, then later restore by applying the open transform and disabling their blocking collision in the same save slot.

Directly placed `ATunaSweeperBlockedIntakeScreenActor` instances also use `WorldProgressStates`. Clearing the intake screen writes `Completed` immediately, then swaps `BlockedScreenMesh` to `ClearedScreenMesh` on the same component so the placed transform is preserved. Load restores the cleared mesh, and death saves preserve this long-term world state while clearing only carried raid possessions. Every placed instance must keep a stable, unique `ProgressObjectId`.

### Bunker Housing Facilities

Placed/stored facilities are saved through `UTunaSweeperSaveGame::HousingFacilities`.

Each `FTunaSweeperHousingPlacedFacilitySaveData` preserves:

- `InstanceId`: stable owned facility instance id used for placement, respawn, and storage targeting.
- `FacilityId`: stable facility definition id from `Content/Data/HousingFacilityDefinitions.json`.
- `AnchorCell`: top-left grid cell in the active bunker housing area.
- `RotationQuarterTurns`: Q/E rotation in 90 degree steps.
- `bStored`: whether the owned facility is in storage instead of placed in the bunker grid.

Housing placement state is owned at runtime by `UTunaSweeperHousingSubsystem`. Changes are copied into `UTunaSweeperGameInstance` and written on the next normal save; placement and storage actions currently request an immediate save.

Unlocked housing facility functions are saved through `UTunaSweeperSaveGame::UnlockedHousingFacilityIds`.

Each entry is a stable facility definition id from `Content/Data/HousingFacilityDefinitions.json`. Quest rewards can add ids to this set, and older saves also derive missing unlocks from already completed quest rewards during load. As a temporary progression rule, closing housing mode after a placed `housing_workbench` exists also unlocks `housing_signal_control` and `housing_supply` into this same set.

### Quest Progress

Stored through `UTunaSweeperSaveGame::QuestProgressStates`, `TrackedQuestId`, and `QuestCoinBalance`.

Each `FTunaSweeperQuestProgressSaveData` preserves:

- `QuestId`: stable quest identifier from `Content/Data/QuestDefinitions.json`.
- `State`: `Available`, `Accepted`, `RewardAvailable`, or `RewardCompleted`.
- `ObjectiveProgress`: per-objective `ObjectiveId` and `CurrentCount`.

`TrackedQuestId` stores the HUD-tracked quest for the active save slot. `QuestCoinBalance` stores the player coin currency balance separately from item instances; the field name is legacy and is kept for save compatibility. New save slots initialize with no quest progress, no tracked quest, and zero coins.

### Experience Progress

Stored through `UTunaSweeperSaveGame::TotalExperiencePoints`.

`TotalExperiencePoints` is the source of truth for the bunker/player experience level. The level thresholds and maximum level are derived from `Content/Data/ExperienceLevelTable.json`; the last level row is the current level cap. Raid item and enemy experience is accumulated only in runtime pending state while the player is in `RaidMap`; it is copied into `TotalExperiencePoints` only during successful `RaidMap` to `BunkerMap` survival persistence, before the save is written. The pending raid gain and raid-start baseline are not saved.

Death persistence calls clear the pending raid gain before saving, so failed raids keep the previously saved `TotalExperiencePoints` unchanged.

Level reward stat bonuses are derived from the current level and `Content/Data/ExperienceLevelRewards.json`. The reward table only drives max health, max food/fullness, max hydration, and max stamina increases; it does not persist separate stat-bonus fields and does not apply level-based attack or defense scaling.

Raid-to-bunker travel keeps the entering vitals ratios in runtime memory only so the bunker character can preserve health and restore food/fullness or hydration up to at least 50% if they entered below that threshold. This bunker-entry adjustment is not a save-owned field.

## Loaded Ammo Rules

- `LoadedAmmoItemId` is the source of truth for a weapon's selected/loaded ammo type.
- `LoadedAmmoCount` is the source of truth for the number of rounds currently loaded in that weapon.
- `SelectedAmmoItemId` is kept in sync for legacy/compatibility paths.
- Loading an older save that only has `SelectedAmmoItemId` migrates that value into `LoadedAmmoItemId`.
- If an item has no ammo item id, its loaded ammo count is normalized to `0`.
- `LootLoadedAmmoSourceCount`, `LootLoadedAmmoDeductedCount`, `LootLoadedAmmoDeductionRatio`, and `LootLoadedAmmoFlatDeduction` record enemy-loot conversion metadata when a dropped weapon keeps only part of the enemy's loaded magazine. These fields are informational rule traces; `LoadedAmmoCount` remains the playable loaded-ammo value.

## Maintenance Rule

When adding a field that should survive save/load:

1. Add the field to a save-owned USTRUCT/UCLASS or explicitly copy it into one.
2. Restore it in the matching load path.
3. Add migration/default handling for older saves.
4. Update this document in the same change.
