# Save/Load Persistence Contract

This file tracks gameplay/runtime state that must survive `UTunaSweeperSaveGame` save/load.
Update it whenever a new state field is expected to persist across save slots, level travel saves, death saves, or intro-menu reloads.

## Current Save Container

- Save object: `UTunaSweeperSaveGame`
- Current save version: `2`
- Runtime owner: `UTunaSweeperGameInstance`
- Save entry point: `UTunaSweeperGameInstance::SaveGameStateInternal()`
- Load entry point: `UTunaSweeperGameInstance::LoadGameState()`

## Persisted State

### Save Metadata

- `SaveVersion`
- `SaveSlotIndex`
- `TotalPlaySeconds`
- `LastSavedAtTicks`

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
