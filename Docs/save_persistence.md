# Save/Load Persistence Contract

This file tracks gameplay/runtime state that must survive `UTunaSweeperSaveGame` save/load.
Update it whenever a new state field is expected to persist across save slots, level travel saves, death saves, or intro-menu reloads.

## Current Save Container

- Save object: `UTunaSweeperSaveGame`
- Current save version: `4`
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

Slot arrays store item UIDs. Any item UID referenced by these slots, including nested attachment UIDs, must also exist in `ItemInstances`.

### World Progress Objects

Stored through `UTunaSweeperSaveGame::WorldProgressStates`.

Each `FTunaSweeperWorldProgressSaveData` preserves:

- `ObjectId`: stable per-level object identifier from runtime spawn data.
- `InfoId`: reusable progress-object information id, such as a broken bridge or blocked entrance type.
- `State`: `InProgress` or `Completed`.
- `ProgressQuantity`: contributed material count for in-progress objects.

Raid world progress actors restore this state on spawn. Completed repair objects disable their blocking collision and spawn their configured completed replacement actor at the same transform.

Persistent door actors also use `WorldProgressStates`: once opened, they write `Completed` for their stable door `ObjectId`, then later restore by applying the open transform and disabling their blocking collision in the same save slot.

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
