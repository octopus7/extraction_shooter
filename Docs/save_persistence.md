# Save/Load Persistence Contract

This file tracks gameplay/runtime state that must survive `UTunaSweeperSaveGame` save/load.
Update it whenever a new state field is expected to persist across save slots, level travel saves, death saves, or intro-menu reloads.

## Current Save Container

- Save object: `UTunaSweeperSaveGame`
- Current save version: `8`
- Runtime owner: `UTunaSweeperGameInstance`
- Save entry point: `UTunaSweeperGameInstance::SaveGameStateInternal()`
- Load entry point: `UTunaSweeperGameInstance::LoadGameState()`

## Persisted State

### Save Metadata

- `SaveVersion`
- `SaveSlotIndex`
- `TotalPlaySeconds`
- `LastSavedAtTicks`

### Scenario Progress Flags

Scenario progress is persisted per save slot through `UTunaSweeperSaveGame::CompletedScenarioFlags`.
See [Docs/scenario_progress_flags.md](scenario_progress_flags.md) for the flag contract, routing rules, and reuse constraints.

### Memo Unlocks

Collected memo ids are persisted per save slot through `UTunaSweeperSaveGame::AcquiredMemoIds`.

Memo interaction marks the id as acquired in `UTunaSweeperGameInstance` memory immediately and destroys the spawned memo actor. It does not force an immediate save by default; the acquired ids are written on the next normal save, level-travel save, death save, or any other call to `SaveGameStateInternal()`. Runtime memo spawns must query this in-memory set so a collected memo stays hidden after death/respawn even before the next disk write.

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

### Player Slot Layout

- `InventorySlots`
- `EquipmentSlots`
- `AuxiliaryBagSlots`
- `UsableQuickSlots`

Slot arrays store item UIDs. Any item UID referenced by these slots, including nested attachment UIDs, must also exist in `ItemInstances`.

`UsableQuickSlots` stores the 3-8 quick-slot layout shown in inventory mode and reflected in the gameplay quick-slot bar. Slots 1, 2, and melee are equipment slots and are not duplicated here. Only usable items can occupy these slots; currently that means item definitions tagged `item.category.consumable` or `item.category.throwable`.

Unlike memo unlocks and map markers, usable quick slots are item-possession state. Generic saves preserve the previously saved quick-slot payload without rewriting it, successful RaidMap-to-BunkerMap extraction saves write the current runtime quick slots, and death saves clear all usable quick slots.

### World Progress Objects

Stored through `UTunaSweeperSaveGame::WorldProgressStates`.

Each `FTunaSweeperWorldProgressSaveData` preserves:

- `ObjectId`: stable per-level object identifier from runtime spawn data.
- `InfoId`: reusable progress-object information id, such as a broken bridge or blocked entrance type.
- `State`: `InProgress` or `Completed`.
- `ProgressQuantity`: contributed material count for in-progress objects.

Raid world progress actors restore this state on spawn. Completed repair objects disable their blocking collision and spawn their configured completed replacement actor at the same transform.

Persistent door actors also use `WorldProgressStates`: once opened, they write `Completed` for their stable door `ObjectId`, then later restore by applying the open transform and disabling their blocking collision in the same save slot.

### Quest Progress

Stored through `UTunaSweeperSaveGame::QuestProgressStates`, `TrackedQuestId`, and `QuestCoinBalance`.

Each `FTunaSweeperQuestProgressSaveData` preserves:

- `QuestId`: stable quest identifier from `Content/Data/QuestDefinitions.json`.
- `State`: `Available`, `Accepted`, `RewardAvailable`, or `RewardCompleted`.
- `ObjectiveProgress`: per-objective `ObjectiveId` and `CurrentCount`.

`TrackedQuestId` stores the HUD-tracked quest for the active save slot. `QuestCoinBalance` stores quest reward currency separately from item instances. New save slots initialize with no quest progress, no tracked quest, and zero quest coins.

## Loaded Ammo Rules

- `LoadedAmmoItemId` is the source of truth for a weapon's selected/loaded ammo type.
- `LoadedAmmoCount` is the source of truth for the number of rounds currently loaded in that weapon.
- `SelectedAmmoItemId` is kept in sync for legacy/compatibility paths.
- Loading an older save that only has `SelectedAmmoItemId` migrates that value into `LoadedAmmoItemId`.
- If an item has no ammo item id, its loaded ammo count is normalized to `0`.

## Maintenance Rule

When adding a field that should survive save/load:

1. Add the field to a save-owned USTRUCT/UCLASS or explicitly copy it into one.
2. Restore it in the matching load path.
3. Add migration/default handling for older saves.
4. Update this document in the same change.
