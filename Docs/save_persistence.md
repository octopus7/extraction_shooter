# Save/Load Persistence Contract

## ATV repair and raid fuel

Repairable ATV scenery consumes one working vehicle spark plug through the existing atomic facility-repair transaction. Repair progress uses the configured stable world-progress object ID and existing gameplay save fields. A repaired actor remains noninteractive scenery during that map visit, including after reward notifications. Only a later BeginPlay with both saved repair completion and the configured quest reward claimed spawns its configured drivable ATV Blueprint. The scenery and replacement classes remain separate.

Fuel is session memory only: one carried ethanol fills 60 driving seconds; a seated, idle engine consumes 10% of the driving rate, and an unoccupied vehicle consumes none. The GameInstance cache is keyed by level package and a stable vehicle placement ID. Placed vehicles default to their actor name; spawners and repair replacements assign their stable source ID before BeginPlay. Returning to a vehicle during the same raid restores its remaining fuel. Travel involving a non-raid map, gameplay-slot loading, or new inventory/session initialization clears the cache. A generation token rejects late writes from actors belonging to the preceding session. Fuel is not serialized and does not change the save version.

Successful refueling reports the configured interaction objective event after inventory consumption and fuel assignment. Quest objective progress is saved normally; fuel itself is not. An empty tank stops driving input and engine audio but keeps the rider seated so the normal dismount action remains available.

## Workbench crafting objective progress

`workbench_recipe_crafted` counts successful crafting operations for one exact `recipe_id`, after ingredients are consumed and all output items fit. Acquisition, possession, a different recipe, failed crafting, and crafting before quest acceptance do not advance it. A multi-item output counts as one operation. The count uses the existing numeric quest objective progress and is included in the normal crafting save; no new save fields or version migration are needed. The new objective enum value is appended so existing ordinals remain unchanged.

Recipe unlock rewards and returned material rewards use the existing claimed-quest and unlocked-recipe persistence. `recipe.rocket_launcher` remains a stable recipe ID; its unlock state is restored through the existing reward reconciliation. No extra crafting-history save or retroactive credit is introduced.

## Integer quest identity (2026-10-10)

Runtime quest identities, prerequisite references, tracking, objective submission, scenario quest conditions, and claimed-quest achievement facts now use positive `int32` IDs; `0` means unset. Objective, provider, achievement, and localization keys retain their existing name types. Display formatting may pad an ID to at least three digits without changing the saved integer.

Gameplay save version remains 22 and achievement version remains 1 because this is an additive tagged-property migration. The original `QuestProgressStates` array keeps its original `FTunaSweeperQuestProgressSaveData` structure and `FName QuestId`; the original `TrackedQuestId` and achievement `ClaimedQuestIds` properties also retain their original name types. New writes use `NumericQuestProgressStates` (`FTunaSweeperNumericQuestProgressSaveData`), `NumericTrackedQuestId`, and achievement `NumericClaimedQuestIds`. No existing name property is reinterpreted as an integer.

On gameplay load, digit-only legacy IDs within the positive int32 range migrate in memory, retaining objective counts and quest state; padded `003` becomes integer `3`. Numeric entries take precedence if both formats exist. Retired nonnumeric IDs, zero, signed strings, and overflow are discarded. The active catalog still filters unknown progress and tracking. Legacy fields are cleared after migration and new gameplay saves write integer data. Account achievement loading merges valid numeric legacy claims with its new integer facts. Currency and unrelated saved state remain unchanged; loading does not itself rewrite a gameplay slot.


## Retired demo quest catalog (2026-10-10)

The five former demo quests are removed from the public quest catalog. Existing save files are not rewritten by this content change. The existing quest loader discards saved quest progress and tracking whose IDs are absent from the active catalog; subsequent saves therefore omit that retired progress. Quest coins and unrelated inventory, scenario flags, and world progress remain intact. Completing the intro dialogue no longer accepts a fixed demo quest, and the removed final quest no longer queues or resumes the old ending. Quest-gated environment interactions remain gated until explicitly authored again.

QuestStudio `authoring_tags` and graph coordinates are authoring metadata only. They do not add gameplay save fields or change prerequisite evaluation. QuestStudio snapshots are separate from UE gameplay save slots.

## Armor and ammunition penetration

Combat health and state gauges (health, food, hydration, stamina and scratch) now normalize current/max values to whole points while retaining existing float storage. Imported/restored vitals and ratio-based level transitions round at the normal state setters; no save format/version change or direct save-file rewrite is required. Continuous vitals depletion, stamina changes and practice-dummy recovery keep sub-point remainders only in transient runtime memory. Setting/restoring vitals or resetting the corresponding state clears its remainder; unchanged max-stat refreshes preserve it. Excess changes beyond full/empty are discarded, while opposite-direction pending changes remain. Weight, timers and progress ratios retain their existing fractional units.

Armor tier, base defense, and ammunition penetration tier are item-definition data resolved from the existing saved item IDs. Existing IDs, equipment slots, loaded-ammo IDs/counts, and save version remain unchanged; old tactical jackets and body armor receive the updated definition values on load. New tiered armor and advanced AP ammunition use the normal item-instance persistence. Each projectile snapshots penetration tier when fired; projectiles and enemy profile armor references are transient and are not new save fields. Enemy armor references provide stats only and do not add inventory or loot instances.

## Debug armory

The development-only `BP_DebugArmory` uses the existing atomic inventory-add transaction for free weapon, ammunition, attachment, and protective-equipment supplies. Granted items follow normal inventory save/load, acquisition notifications, and inventory-capacity rules; insufficient space rolls the entire request back. Opening the panel, its selected category/quantity, and the active rack reference are transient and are never saved. There is no armory currency, stock, unlock state, or new save format. Shipping removes the interaction component and rejects panel opening, catalog access, and supply calls; the rack remains scenery.

This file tracks gameplay/runtime state that must survive gameplay-slot or account-global save/load.
Update it whenever a new state field is expected to persist across save slots, level travel saves, death saves, or intro-menu reloads.

## Cloud Save Roots

All gameplay slots, selected-slot settings, achievements, previous generations, backups, and cleanup logs resolve their root through `TunaSweeperBuildFlavor::GetSaveGameDirectory()`.

- Editor/local saves: `Saved/SaveGames/<Demo|FullGame>/`.
- Packaged Steam channel: `Saved/SaveGames/Steam/<64BitSteamID>/<Demo|FullGame>/`, fixed for the process lifetime. Steam Auto-Cloud synchronizes committed `.sav` and `.sav.previous` files, including `Backups/`, when configured in Steamworks. An unavailable identity selects `Steam/LocalOnly/<Demo|FullGame>/`, outside the registered account paths.
- Packaged Stove channel: `<Base_GetCloudSavingPath result>/<Demo|FullGame>/`. The SDK root/account are cached during authenticated module startup, before GameInstance subsystems load saves. Studio must register an account-specific root; see [Stove cloud setup](stove_cloud_saves.md). An unavailable/invalid SDK root uses `Saved/SaveGames/Stove/LocalOnly/<MemberNumber|UnknownAccount>/<Demo|FullGame>/`. The selected root stays fixed until restart.
- Steam paths are never selected by Stove/NoStore or the editor. Existing shared local saves are preserved and are not automatically claimed by an arbitrary store account.

Machine-specific settings, candidate files, corrupt archives, and cleanup audit logs are not Steam Cloud targets. Steam Cloud being disabled or the client being offline does not disable local saves. See [Steam cloud setup and verification](Steam/cloud_saves.md) for the required per-app portal configuration; local code tests do not prove live synchronization.

## Offline Save Tool

The editor-only `TunaSweeperEditor.TunaSweeperSaveTool` commandlet and .NET 10 wrapper inspect gameplay saves and add weapons/ammunition/armor or replace equipment from a versioned JSON preset. See [agent commandlet protocol](save_tool_commandlet.md) and [human CLI guide](../Tools/SaveTool/README.md).

Reads never trigger recovery or migration. Mutations require current save version 22, explicit path/flavor/slot, and explicit commit; previews leave the file unchanged. The tool uses native serialization and the existing fail-closed writer after validating the complete mutation. A unique byte-for-byte `.tool-backup-*` original is retained in addition to `.previous`. Stop game/PIE before editing; the tool mutex does not coordinate gameplay or cloud writers.

Added/replaced items use existing instance, ownership, loaded-ammo, attachment and acquisition-history fields. Replaced equipment moves to inventory; insufficient capacity aborts the entire edit. Quest/achievement events are not synthesized. Unrelated saved progress is preserved, and `LastSavedAtTicks` follows the existing local-time convention. No new gameplay save fields, versions, or bundled equipment presets are introduced.

## Current Save Container

### Extraction return quest objectives

The `extraction_return` objective advances only when the destination game world has finished loading into the base after a successful extraction-point hold. The quest subsystem snapshots eligible accepted objectives at departure, then consumes that transient record on its own GameInstance's `PostLoadMapWithWorld` notification. Ordinary level travel, death rescue, travel failure, another destination, save loading, new game, and definition reload clear pending credit. A duplicate arrival notification cannot increment twice. The record is not serialized, so restarting during travel cannot award an unconfirmed return.

Confirmed objective progress and `RewardAvailable` state use the existing numeric quest progress save fields and are saved after arrival. Claiming rewards remains a separate NPC action. No save version changes. Existing `level_travel` objectives retain their request-time behavior.

### Developer startup logo (Main only)

The Main startup logo is attempted once per `UTunaSweeperGameInstance` session; returning to the title does not replay it. This guard is transient and independent of gameplay save slots. Demo builds retain their existing direct title entry.

Main/Full is the edition, independent of the distribution channel: `NoStoreFull`, `SteamFull`, and `StoveFull` all use the Main logo policy; only `NoStoreDemo`, `SteamDemo`, and `StoveDemo` use the Demo policy. Packaging targets are respectively `TunaSweeperNoStore`, `TunaSweeper`, `TunaSweeperStove`, and their `Demo`-suffixed variants. `NoStore` is not a synonym for Demo. The saved editor preview and packaging defaults are aligned to NoStore Full. Temporary editor command-line preview overrides apply in memory only; explicit build-menu selections persist both targets together.

`bDeveloperLogoOncePerDay=False` in `[/Script/TunaSweeper.TunaSweeperGameInstance]` is intentionally dormant. While false, startup neither reads nor writes a viewing date, and every fresh Main session plays the sequence. Enable only on an explicit request. When enabled, `TunaSweeper.StartupLogo/MainLastCompletedLocalDate` in `GGameUserSettingsIni` stores the local calendar date (`YYYY-MM-DD`) after successful video playback and completion of the still/fade sequence. Failed or timed-out playback does not record a date. A matching local date skips startup presentation; it is a calendar-day check, not a rolling 24-hour timer. This machine-local preference is shared across accounts/stores and does not participate in gameplay-slot saves or require a save-version change.

Presentation timings live in `[/Script/TunaSweeper.TunaSweeperStartupLogoWidget]`: `FadeSeconds=0.25`, `StillSeconds=1.0`, `BlackSeconds=0.2`. The flow opens `/Game/Movies/MS_DevTunaCut`, retains its last frame for the crossfade to `/Game/UI/Title/T_DevTunaLogoWhite`, fades back to black, then releases the existing title fade and BGM. The white still source is `chatgpt/devTuna_logo_white.png`, generated from the devTuna wordmark in `chatgpt/logo_pv.png`. Opening (10 seconds) and playback (30 seconds after fade-in) watchdogs fall back to the still instead of trapping startup. Both the video and the unmodified logo texture are centered at 50% width and height relative to the fitted 16:9 presentation frame. A solid white viewport background and opaque still sit below the video; only video opacity changes during the crossfade. A fixed white feathered edge mask above both images hides the rectangular boundary without double-applying group opacity or allocating a composite texture. Any key press/release, mouse button/movement/wheel, intentional controller axis input (dead zone 0.2), or touch press during the one-second still hold starts its 0.25-second fade-out immediately. Input during the video/crossfade is consumed without queuing a skip, and repeated input cannot restart the fade.

Interface language persists independently of gameplay slots in `GGameUserSettingsIni`, under `TunaSweeper.InterfaceSettings/Language`. Supported saved codes are `ko`, `en`, `ja`, `zh-Hans`, `zh-Hant`, `ru`, and `pt-BR`. Existing enum values 0–5 retain their meaning; Brazilian Portuguese uses 6. Startup restores a valid saved language. Chinese OS locales distinguish explicit Hans/Hant scripts, then TW/HK/MO (Traditional) from other regions (Simplified); region-qualified Russian locales resolve to Russian. `pt-BR` and bare `pt` resolve to Brazilian Portuguese (case-insensitive, with hyphens or underscores); `pt-PT` remains unsupported and follows the default-language fallback. If the value is missing or invalid, Steam selects and saves English (`en`); other distribution channels detect the OS language/locale, falling back to English when unsupported. Packaged builds read `TunaSweeper.Distribution/DistributionChannel` from `GGameIni`; editor runs use the selected build target, matching title-menu channel selection. Manual language selection continues to use the existing global setting. This setting remains shared across stores, so a valid language saved by another store also takes precedence over the first-launch default. No gameplay save-version change is required.

- Save object: `UTunaSweeperSaveGame`
- Current save version: `22` (versions `20` and `21` remain readable)
- Runtime owner: `UTunaSweeperGameInstance`
- Save entry point: `UTunaSweeperGameInstance::SaveGameStateInternal()`
- Load entry point: `UTunaSweeperGameInstance::LoadGameState()`

## Outfit Selection and Global Unlocks

`UTunaSweeperSaveGame::SelectedOutfitId` stores the equipped appearance per gameplay slot. The supported IDs are `Maid`, `SchoolUniform`, `MechanicOutfit`, `Sportswear`, `BunnyPajamas`, `AdventurerOutfit`, `Raincoat`, and `SciFiSuit`. New slots and older saves default to `Maid`; missing or unknown saved IDs resolve to `Maid`. New-game/slot initialization resets the selection, while death inventory loss, bunker/raid travel, and normal saves preserve it. A locally controlled player's new pawn restores the selection at startup/possession. The original face component, expressions, animation class, and ragdoll physics remain in place. Adding `SciFiSuit` does not change the gameplay save version or global unlock format.

Cosmetic ownership is separate: `UTunaSweeperCosmeticUnlockSaveGame` version `1` stores `UnlockedOutfitIds` in `CosmeticUnlocks_<DistributionNamespace>.sav` under the same active account/build root used by achievements. The distribution namespace uses the existing `TunaSweeperAchievements/DistributionNamespace` setting. This file is independent of gameplay slots and is never reset by new game, slot selection, death, or gameplay-slot deletion. Demo/Main and account-root boundaries continue to apply. `Maid` is always unlocked. Unknown IDs are rejected and ignored when loading the unlock list.

`UTunaSweeperGameInstance::bUnlockAllOutfitsOverride` is a config property, default `true`. Set it to `False` under `[/Script/TunaSweeper.TunaSweeperGameInstance]` in `DefaultGame.ini` to require actual unlocks. The override only controls access; it does not write all outfit IDs into the unlock file. `TryUnlockOutfit` still records a real unlock while the override is enabled, is idempotent, and rolls back the runtime addition if saving fails. A fresh process with the override disabled restores only the committed unlocks plus `Maid`.

Unlock writes use the existing CRC envelope and candidate/previous fail-closed commit path. If neither the active file nor previous generation is valid, they are retained and new unlock writes are blocked for the session to avoid silently replacing ownership data. Appearance changes validate the complete body/clothing pair before applying it. `TryEquipOutfit` rejects locked/unknown/unloadable outfits; failure leaves the previous selection and appearance intact. A gameplay save failure restores both after an attempted change and does not broadcast a successful selection. If a saved outfit becomes locked after the override is disabled, or its assets cannot be restored, restoration selects and displays `Maid` in memory and updates the UI. It does not grant an outfit or immediately overwrite the save; the next normal gameplay save includes the fallback selection. The previously selected outfit can be equipped normally after it is unlocked or repaired.

## Account-Global Achievement Container

Achievements use `UTunaSweeperAchievementSaveGame` version `2` (with version `1` read/upgrade support) instead of `UTunaSweeperSaveGame`. The file is stored as `Achievements_<DistributionNamespace>.sav` in the active save root described above, so Demo/Main and Steam/Stove/NoStore progress do not cross. It uses the same CRC envelope and fail-closed active/candidate/previous commit path as gameplay saves. If both active and previous generations are invalid, they are retained with a timestamped `.corrupt-*` suffix before a fresh state is allowed.

The achievement container persists independently of gameplay slots:

- total player-attributed enemy kills;
- enemy ids killed at least once;
- reached achievement location ids;
- quest ids whose rewards were claimed;
- confirmed gameplay facts (event kind plus target id, such as actual facility use);
- internally unlocked achievement ids;
- platform unlock confirmations used to avoid duplicate writes.

Named raid bosses use an optional spawn-profile `enemy_id` as the same stable kill identity for quest objectives and the account-global first-kill achievement. Missing IDs retain the existing per-raid instance ID; actor instance tags remain seed-specific. Existing quest counters and killed-enemy IDs persist this progress without a save schema change.

Facts are saved immediately when first observed, regardless of whether a current JSON definition consumes them. Loading a new definition can therefore unlock an achievement retroactively. New game, save-slot selection, and slot deletion must not reset this container. The platform publisher may merge remote unlocks into it, while failed or unavailable platform writes remain derivable as pending from locally unlocked ids without a matching platform confirmation.

Version 1 files load and upgrade on the next successful write. The candidate/previous replacement validator accepts both supported versions so the existing v1 primary can be preserved during migration. Reserved achievement `14` is excluded from restored/captured unlocks and platform synchronization. New unlock notifications and platform writes wait for a successful local save; failed writes retain dirty progress for retry without duplicate notifications.

Raid start time and kills, pending extraction evidence, current inventory quantities and current facility states are transient, not account facts. Raid state starts at actual raid-world readiness and resets after arrival or death. Completed extraction samples health/time/kills and commits only upon arrival at its intended destination; travel failure or non-extraction travel discards the pending evidence. Item `3004` quantity counts currently owned carried/equipped/quick-slot/auxiliary-bag and base-storage items, deduplicated by UID; loot containers and cumulative acquisition do not count. Facility completion checks the configured full set in the current gameplay slot.

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

The pause menu's confirmed **Return to title** and **Quit game** actions share `SaveForGameplayExit`. In the bunker this persists current item/quick-slot state before leaving. In a raid it uses the existing death-loss rule: carried inventory, equipment, auxiliary bags, quick slots, and pending raid experience are forfeited; storage and long-term quest/world progress remain. This path never calls successful extraction persistence. Both actions first show a localized confirmation explaining the applicable outcome. If saving fails, the game stays paused and raid item/experience state is restored without broadcasting the temporary loss, allowing safe cancellation or retry. Pausing itself is transient and adds no save field.

Bunker item ownership/layout mutations must be saved after the player leaves the blocking UI and returns to a gameplay-capable mode. This includes purchases, sales, storage moves, inventory/equipment/quick-slot layout edits, weapon attachment changes, and future bunker-only item transactions that modify persisted item instances or slot arrays.

While the responsible UI is alive, the mutation should only mark a pending bunker item save in runtime memory. Multiple mutations during the same UI session coalesce into one pending save. The pending save must be flushed immediately when the HUD/input state becomes gameplay-capable again: no inventory-only panel, external storage/shop/workbench panel, attachment edit UI, modal confirmation, or other blocking item UI remains open, and the player can resume normal bunker gameplay controls.

The gameplay-capable flush point is a player-facing UX rule, not a purely theoretical engine-state rule. If a player would reasonably feel that control has returned or that delaying the save is uncomfortable, prefer the earlier/comfortable flush point. Minor differences between UI flows are acceptable when they match player perception; do not delay a save just to make every flow satisfy an abstractly identical state checklist.

The flush uses the normal active-slot save path through `UTunaSweeperGameInstance::SaveGameStateInternal()` with runtime quick slots persisted. If another guaranteed save writes the same active slot before gameplay becomes available, it may clear the pending bunker item save only after the in-memory item state has been included in that write.

Raid item changes keep their existing extraction/death/level-travel save rules and are not covered by this bunker UI deferral rule.

## Persisted State

### Melee Item Replacements

- Item `1004` retains its identity and becomes the wooden club.
- Item `1005` retains its identity and becomes the spiked club. Existing instance UIDs, quantities, equipment/loot references, acquisition history, and shop references remain unchanged. It is not remapped to `6003`.
- Item `6003` now occupies the melee equipment slot and remains the reusable water-intake tool. Existing stacked quantities are preserved; new weapon stacks use the weapon stack limit. World-progress keys and the reusable-tool ID do not change. No new save fields or save-version change are required.

### Save Metadata

- `SaveVersion`
- `SaveSlotIndex`
- `BuildFlavor`: build target identity (`Demo` or `Main`). A save whose value does not match the running target is rejected.
- `TotalPlaySeconds`
- `DifficultyStage`: save-slot difficulty stage, clamped to `1..3`; `1` is Farming, `2` is Normal, and `3` is Hard. Main slots keep the default `1` until the player confirms a difficulty. Demo slots atomically initialize to fixed stage `2` when the Demo notice is confirmed. Runtime resolves the stage through `DifficultyDefinitions.json`; its `enemy_incoming_damage_multiplier` scales only enemy-attributed damage received by the player (`5000/10000/20000` for stages `1/2/3`) before integer defense is subtracted.
- `bDifficultySelected`: whether the slot completed its required start gate. For Main this is difficulty confirmation. For Demo this is the notice confirmation written together with the first slot save; no Demo save is created before confirmation.
- `LastSavedAtTicks`

Demo and Main use the same logical slot-name format but separate physical roots: `Saved/SaveGames/Demo/` and `Saved/SaveGames/FullGame/` for editor/local saves, or the per-store cloud roots described above. Demo exposes and accepts only `TunaSweeperSave_Slot01`; Main exposes slots `TunaSweeperSave_Slot01` through `03`. Each root independently owns last-selected-slot settings, backups, and deletion logs. The directory boundary and `BuildFlavor` field together prevent progress, rewards, inventory, currency, unlocks, and world state from crossing targets.

The full-game directory name is `FullGame`; serialized `BuildFlavor=Main` and quest dataset identifiers remain unchanged. The local path is resolved directly without inspecting, moving, or falling back to a `Main/` directory.

### Obsolete Save Deletion

Save version 20 is the minimum supported version. During `UTunaSweeperGameInstance::Init()`, every loadable `UTunaSweeperSaveGame` file below version 20 under the active `Demo` or `FullGame` root, including backups, is deleted. Slot lookup repeats the same check so an obsolete file introduced after initialization cannot block a new save. Save settings and unreadable files inside the active root are not auto-deleted.

Every successful obsolete-version deletion appends a local-time timestamp, detected version, and path relative to the active root to that root's `AutoDeletedSaveLog.txt`. The game prepares the audit log before deleting; if the log cannot be prepared, it leaves the versioned save file in place and reports an error through the Unreal log.

Legacy `.sav` files directly under `Saved/SaveGames/` are not migrated. They are deleted at startup regardless of version and their file names are appended to the active target's deletion log. Subdirectories are never included in this flat-file cleanup.

### Scenario Progress Flags

Scenario progress is persisted per save slot through `UTunaSweeperSaveGame::CompletedScenarioFlags`.

The first BunkerMap controls tutorial records `tutorial.bunker.basics_seen` when Continue is pressed. It uses the existing scenario flag save/load/reset path (no save schema/version change). Completion triggers an immediate save attempt; if saving fails, the in-memory flag still prevents repetition during this session and is included in subsequent normal saves. A new slot/reset clears it. Popup visibility, pause and input focus are transient.

The second (combat) tutorial records `tutorial.raid.combat_seen` through the same slot-local path when Continue is pressed. Bunker-to-raid departure queues it only while this flag is absent; it waits for the transition subsystem to become idle and gameplay input to be available. Repeat departures/reloads preserve completion; a new game resets it. The pending arrival flag is transient and is cleared on save load and runtime/new-slot reset, preventing a pending popup from leaking across slots.

The lounge controls interaction opens a repeatable three-page review with cyclic navigation. Review mode, current page and visibility are transient; opening, paging and closing this mode neither writes a save nor changes either automatic tutorial completion flag. The third page has no automatic trigger or completion flag.

Demo dinner ending writes `demo.ending.farewell_seen` when the farewell card is shown, then immediately deletes only the active Demo save slot, including its backups/recovery artifacts, and resets its runtime state. A transient slot retirement barrier blocks late autosaves until explicit slot activation so title return cannot recreate the deleted save. Interrupted dinner dialogue can still resume because deletion has not happened yet. Main is unlimited continued play: it never enters this automatic demo ending or deletes its save, even if a legacy demo flag exists. Temporary camera/input state and dinner actor transforms are not persisted.

The food warehouse grants canned tuna (3004) through the existing inventory save path. Its collected state is transient per world visit, deliberately replenishing on a later raid visit so death or consuming the food cannot permanently block delivery. Final delivery consumes one can and advances the existing `demo.canned_tuna.deliver` objective; reward retries use RewardAvailable without consuming another can.
See [Docs/scenario_progress_flags.md](scenario_progress_flags.md) for the flag contract, routing rules, and reuse constraints.
The active scenario data pack supplies dialogue completion flags. Demo uses `dialogue.demo.toilet_intro`, while Main uses `dialogue.main.bunker_intro` after `scenario.opening.awakening`; the retired test-dialogue flag `dialogue.mole.bunker_intro` is intentionally left as inert legacy save data.

### Memo Unlocks

Collected memo ids are persisted per save slot through `UTunaSweeperSaveGame::AcquiredMemoIds`.

The directly placed `BP_MemoAnchor` / `ATunaSweeperMemoAnchor` uses only `MemoId`; there is no raid PlacementId or MemoSpawns.json linkage. Memo interaction calls `MarkMemoAcquired(MemoId, true)` and immediately persists the existing save schema. If saving fails, it removes the newly acquired ID and leaves the memo collectible without publishing a state change. After success, all live matching memo actors hide their mesh, collision, and interaction. They remain as hidden level instances so activating a different save slot can restore an uncollected memo. Save initialization broadcasts `OnMemoStateChanged` after restoration is complete. On map re-entry or application restart, already collected IDs remain hidden. IDs are permanent per save slot until that slot is reset; collection does not depend on raid extraction or death. No new serialized fields or save-version change are required.

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

Confirming the Demo notice creates the single Demo save slot using `Content/Data/DemoStartingLoadout.json`. Its default configuration supplies a rifle (`1002`) in equipment weapon slot 1, a laser sight (`2006`) attached to that rifle's `attachment.slot.tactical` slot, and 60 total rifle rounds (`2002`): 30 rounds loaded in the rifle and 30 rounds in the first inventory slot. The rifle stores `2002` as both its loaded and selected ammo item id, and weapon slot 1 is selected for the immediate Bunker entry. The laser sight exists as a nested item instance referenced by the rifle rather than occupying an inventory slot. This loadout is initialized only while creating a new Demo slot; Main saves and already-existing Demo saves are not modified. The combat laboratory also reads this configuration for its temporary equipment.

Starting equipment is validated against item definitions, attachment/ammunition compatibility, slot capacity, and stack limits. Invalid configuration rolls back all newly created instances and slot changes without recording acquisition history. Configuration edits affect future initialization only; existing saves retain their instance IDs, slot layout, attachments, and ammunition. `provides_laser_sight` is a static item-definition property read when checking an equipped attachment, not new per-instance save data. No save-version change is required. See [starting equipment authoring](starting_equipment.md).

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
Carried weight is derived from item and attachment weights plus `LoadedAmmoCount` times the `weight_kg` of `LoadedAmmoItemId`, including guns carried unequipped in inventory. Reloading transfers weight from loose ammunition to the gun; firing removes the consumed round's weight. Storage contents remain excluded. Weight is recalculated from these existing saved fields and current item definitions, so no additional save field or version change is required.
Weapon attachment slots are keyed by attachment slot tags. Rifle instances may persist `attachment.slot.magazine`, `attachment.slot.optic`, and `attachment.slot.tactical`; the tactical slot currently stores the laser sight item when equipped.

### Player Slot Layout

- `InventorySlots`
- `EquipmentSlots`
- `AuxiliaryBagSlots`
- `UsableQuickSlots`

Slot arrays store item UIDs. Any item UID referenced by these slots, including nested attachment UIDs, must also exist in `ItemInstances`. `FTunaSweeperInventorySlot::bSortLocked` persists the player's inventory sort-lock state; the inventory compact/sort button leaves locked inventory slots in place and only trims unlocked inventory slots around them.

Backpack tiers 1–5 provide 50/60/80/100/120 inventory slots. Tier 5 is item `5011`; existing backpack IDs `5002`–`5005` remain stable after the visual/name replacement. The required maximum and default maximum are 120, including when a legacy Blueprint still specifies a 100-slot maximum. Both equipped-capacity calculation and the load-time occupied-slot preservation limit use this required maximum, so slots 100–119 must not be truncated on load. The existing slot arrays and item-instance payload handle this extension without a save-version change. The equipment regression test serializes and deserializes the item UID and quantity in slot 119 entirely in memory.

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

`recipe.vehicle_spark_plug` uses this same per-slot unlock mechanism and starts locked. Its addition does not change the save schema; existing completed quest rewards are reconciled through the same load path.

### World Progress Objects

Bunker pipe repair unlocks from the existing `demo.water_intake.blocked_screen` completed state. Before crowbar debris clearing, the pipe uses its intact mesh and has no repair interaction; after clearing it uses the damaged mesh and allows repair. This presentation is derived again on load without a new save field. An already completed pipe repair still restores the repaired replacement, including older saves.

Stored through `UTunaSweeperSaveGame::WorldProgressStates`.

The bunker pipe BP (`/Game/Interaction/BunkerPipe/BP_BunkerPipe_Broken`) reuses this state: waterproof tape item 6005, quantity 1, completed replacement `BP_BunkerPipe_Repaired`. Assign a stable unique `ProgressObjectId` to each placed pipe; the BP leaves it unset rather than sharing one ID across all placements. Completion is restored through the existing replacement flow, including the placed actor's scale. No save schema change is required.

Each `FTunaSweeperWorldProgressSaveData` preserves:

- `ObjectId`: stable per-level object identifier from runtime spawn data or a directly placed level actor.
- `InfoId`: reusable progress-object information id, such as a broken bridge or blocked entrance type.
- `State`: `InProgress` or `Completed`.
- `ProgressQuantity`: contributed material count for in-progress objects.

Raid world progress actors restore this state on spawn. Completed repair objects disable their blocking collision and spawn their configured completed replacement actor at the same transform.

Persistent door actors also use `WorldProgressStates`: once opened, they write `Completed` for their stable door `ObjectId`, then later restore by applying the open transform and disabling their blocking collision in the same save slot.

Game-owned raid actor catalogs preserve each converted source actor's exact `FName` through `ActorName`. Persistent doors and world-progress actors fall back to that name when their explicit object ID is empty; blocked intake screens also derive a valve key from the actor name. Runtime spawning rejects name collisions rather than suffixing a name, and restores instance settings before BeginPlay. Anchor `PlacementId` does not replace these existing save keys. This changes no save fields or save version.

Directly placed `ATunaSweeperBlockedIntakeScreenActor` instances also use `WorldProgressStates`. The `BP_WaterIntake` facility keeps both its pump/control-panel mesh and screen mesh visible, while the separate `DebrisMesh` using `/Game/Meshes/Props/WaterIntake/SM_ScreenDebris` is the only visual toggled by clearing. The native component has no hard-coded debris asset so the BP owns that presentation assignment. Clearing debris writes `Completed` under `ProgressObjectId`; repairing the valve writes a second `Completed` entry under `ValveProgressObjectId`. Load restores both states, and death saves preserve this long-term world progress while clearing only carried raid possessions. Every placed facility must keep stable, unique screen and valve object ids.

### Power facility repair

`ATunaSweeperPowerFacilityActor` (`/Game/Interaction/BP_PowerFacility`) uses the existing `WorldProgressStates` container. Each future placement must receive an explicit, stable, unique `ProgressObjectId`; an unset ID blocks repair and does not create a save entry. The reusable BP intentionally leaves that ID unset. `InfoId` is `world_progress.power_facility`, with quantity 0/1 while broken and `Completed`, quantity 1/1 after repair. No schema or save-version change is required.

The original facility actor, mesh and collision remain after completion. Its looping fault sparks stop immediately, and BeginPlay restores that presentation before activating the effect. World-progress notifications refresh existing instances as well. Quest eligibility and item requirements control repair availability, independently of whether a broken facility emits sparks. Editor repaired-state preview is transient and never writes save state. This asset creation task adds no map placement or quest dataset entry; see [power facility configuration](power_facility.md).

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

Item submission objectives (`item_submitted`) use the existing objective id/count and quest state fields. Inventory consumption and all matching objective counts are committed in memory before inventory observers or quest save requests run. Requirements for the same item are aggregated; insufficient batches do not change inventory or progress. A saved `RewardAvailable` submission retries rewards without consuming items again. `RewardCompleted` submissions never consume again. The demo retains `deliver_canned_tuna` as its objective id, so progress recorded by the former interaction event survives the change. No save version or serialized field was added.

Completing `dialogue.demo.toilet_intro` automatically accepts `demo_q1_water_intake_check` through the normal quest acceptance path. The dialogue completion flag is set before acceptance saves, so the same save includes the flag, accepted quest state, and tracked quest. Existing accepted/completed quest states prevent duplicate acceptance and notifications; no new save field or version is required. The acceptance toast is transient and is not replayed on load.

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

## Ability Stat Research

Ability-stat research is persisted per save slot through `AppliedResearchNodeIds`, `ActiveResearchStates`, and `ResearchLastObservedUtcTicks`.

- `AppliedResearchNodeIds` contains only nodes whose timer finished and whose `Complete` button was pressed. Only these nodes contribute stat effects.
- Each `FTunaSweeperActiveResearchSaveData` stores an independent `NodeId`, UTC start/finish ticks, and whether timer completion was detected. Multiple entries may run concurrently without a slot limit.
- `ResearchLastObservedUtcTicks` prevents a local wall-clock rollback from reducing already-observed progress. Offline progress uses UTC finish times.
- Starting research, first detecting timer completion, and claiming completion each request an immediate save.
- New games clear applied and active research. Save version 21 adds these fields while version 20 remains load-compatible and initializes them empty.
- New games also reset the session UTC/monotonic clock anchors so a future timestamp loaded from a previous slot cannot carry into the new game.
- Applied and active records whose node IDs are absent from the current definitions are retained across save/load. They do not contribute effects or unlock counts until their definitions are restored. A failed JSON load never deletes progress; a failed reload retains the last valid definitions.
- The bathroom research station's scan-bar phase is cosmetic runtime state and is not saved. No save schema/version change is needed for the station or these safeguards.
- Node layout, unlock counts, durations, localization string keys, and stat effects are static editable data in `Content/Data/StatResearchNodes.json`; the corresponding Korean, English, and Japanese text lives in `Content/Data/UITextStrings.csv`. Neither is duplicated in save data.

## Loaded Ammo Rules

- `LoadedAmmoItemId` is the source of truth for a weapon's selected/loaded ammo type.
- `LoadedAmmoCount` is the source of truth for the number of rounds currently loaded in that weapon.
- `SelectedAmmoItemId` is kept in sync for legacy/compatibility paths.
- Loading an older save that only has `SelectedAmmoItemId` migrates that value into `LoadedAmmoItemId`.
- If an item has no ammo item id, its loaded ammo count is normalized to `0`.
- `LootLoadedAmmoSourceCount`, `LootLoadedAmmoDeductedCount`, `LootLoadedAmmoDeductionRatio`, and `LootLoadedAmmoFlatDeduction` record enemy-loot conversion metadata when a dropped weapon keeps only part of the enemy's loaded magazine. These fields are informational rule traces; `LoadedAmmoCount` remains the playable loaded-ammo value.
- `BP_DebugRifleSupply` changes the same save-owned equipment item, loaded-ammo fields, and carried ammo item stacks used by normal inventory actions. It adds no debug-only save field: a supplied rifle, full magazine, and reserve ammunition therefore follow the existing bunker/raid save and death-persistence rules.

## Maintenance Rule

ATV durability (`MaxDurability` configuration / transient `CurrentDurability`), destruction latch, smoke timers, delayed explosion timer/trigger latch and detached debris are session-local vehicle state. They follow the existing non-persistent ATV placement/movement policy and reset on a fresh level load. No save-owned fields or migration are added by the vehicle damage implementation.

`BP_SpecialATVSpawner` creates its configured ATV Blueprint once on game BeginPlay. The spawned vehicle reference and successful-spawn latch are transient: repeated spawn calls cannot duplicate a vehicle or replace a destroyed one in the same level visit; entering a fresh level creates a new ATV. Editor worlds never spawn a gameplay ATV. The current `ShouldSpawnVehicle` Blueprint policy always returns true; future quest-dependent behavior can override it and call `SpawnVehicle` when eligibility changes. No quest condition or persistent spawner state is currently introduced. The tire Billboard is editor-only and hidden in game.

When adding a field that should survive save/load:

1. Add the field to a save-owned USTRUCT/UCLASS or explicitly copy it into one.
2. Restore it in the matching load path.
3. Add migration/default handling for older saves.
4. Update this document in the same change.

## Story-triggered quest acceptance

- Quest definitions may specify `auto_accept_after_scenario_flag`. Completing a scenario flag reconciles eligible quests through the existing acceptance path, including tracking and saving.
- After inventory/save restoration is fully complete, reconciliation runs again using restored `CompletedScenarioFlags`. This repairs a saved story completion whose quest acceptance was interrupted.
- Existing accepted, reward-available, and reward-completed quest states are preserved. Missing definitions or unmet prerequisites are not forced through; a later load can retry.
- Uses the existing scenario flags, numeric quest progress, and tracked quest fields; no save version or new serialized field is required.

## Practice dummy armor

Practice dummy body/head armor tiers are authored in Blueprint defaults and map instances. Current health, regeneration and any armor changes made during play are transient; loading a level restores the authored tiers and full health. No player save fields or version change are introduced.

## Enemy Burn

- Active enemy burn timers, tick phase, stack count, shot/application IDs, damage snapshots, damage-source references, and flame effects are transient combat state. They end with enemy death, EndPlay, or level travel and are not restored by save/load.
- Weapon/ammunition burn research uses the existing AppliedResearchNodeIds and ActiveResearchStates fields. Only claimed nodes contribute; no new save field or version is required.
- Utility ammunition is retired. Item ID 2023 migrates to standard rifle ammunition 2002 for inventory/storage instances, loaded/selected rounds, restored item templates, and acquisition history. UID, quantity, loaded-round count, and attachments are retained. Save normalization applies the same conversion, so subsequent saves no longer write the retired item ID in item instances. ID 2023 must not be reused. The save version remains 22. Common burn effect and research state remain independent of the removed ammo item.
- The retired shop offer's stock is discarded. Remaining offers retain their saved quantity after row removal by resolving the current slot for their item ID; ambiguous duplicate offers are not reassigned.

## Boss Combat Lab

- Encounter occupancy, boss ownership, ready/active/cleared state, and reset latches are transient; no save fields or migration are added.
- `TunaSweeperBossTestGameMode` temporarily backs up the player's inventory/equipment/ammunition, acquired-item set, weapon selection, and experience state in memory. The original state is restored when the test game mode ends. Test supplies and repeated deaths never write gameplay saves.
- Combat-test sessions suppress quest objective/reward progression and achievement event reporting so lab kills and portal use do not become persistent progression.

## Modular Boss Laboratory

- Boss definitions persist independently of gameplay save slots in `Saved/BossLab/<Demo|Main>/Slot01.boss.json` through `Slot12.boss.json`. The library enumerates these fixed filenames; no separate index is required for externally copied files.
- Version 1 stores catalog version, boss GUID/name, part instance IDs/module IDs/parent IDs/socket/yaw, tactic preset, attack interval, phase threshold, and alternating-weapons setting. Only built-in catalog IDs and bounded values are accepted. Unsupported versions and corrupt files remain on disk and never replace the current draft.
- Slot saves validate the definition, write and read back a `.candidate`, preserve an existing file as `.previous`, and promote the candidate. A failed validation does not overwrite the slot. Explicit deletion removes the slot and its auxiliary files. A duplicated boss gets a new GUID.
- Editor drafts, dirty flags, selected slot, preview selection and camera rotation are session state, not automatic disk saves. Switching to solo play preserves the prior editing draft in memory. Exiting the application discards unsaved edits.
- Part health/destruction, targets, warnings, projectiles, phase and battle results are transient. Each battle uses a definition snapshot and starts with fresh runtime state. The boss lab game mode keeps the existing combat-test save guard active until EndPlay and restores original inventory/experience on exit.
- No network session data, downloads, matchmaking state or replication is stored. See `Docs/boss_laboratory.md` for the current single-player workflow.

## Quest NPC presence

- Quest-gated raid loot placements use `required_active_quest_id` from placement data and derive presence from the existing quest state (`Accepted` only). Quest-change and save-load notifications update visibility/collision immediately. Per-world actor references and the spawned-once guard are transient; no new save fields or version changes are introduced. Hidden containers retain their existing runtime contents within that world and are not regenerated on repeated quest events.

- Quest-driven NPC visibility and collision are derived from existing quest progress on BeginPlay and OnQuestProgressChanged, including save-load broadcasts. FieldOffer appears only while CanAcceptQuest is true; BaseResident appears only after RewardCompleted. No actor visibility flags, new save fields, or save-version changes are needed.
- Optional completion_provider_id belongs to quest definitions, not saved progress. Missing or empty values retain provider_id as the completion NPC.

### 수락 즉시 완료 퀘스트

명시적 `complete_on_accept` 정의는 수락 시 기존 퀘스트 상태 `RewardAvailable`을 저장한다. 보상 수령 전에는 후속 선행 완료로 간주하지 않으며, 기존 보상 수령 경로가 `RewardCompleted`를 저장한다. 신규 저장 필드나 스키마 변경은 없다.

### 기본 전력 시설 프리셋의 저장 ID

`BP_PowerFacility_Primary`는 기존 월드 진행 저장의 `power.facility.primary` ID를 사용한다. 수리 성공 시 완료 상태가 저장되고 다시 로드한 액터는 스파크가 꺼진 상태로 복원된다. 같은 시설을 대표하는 단일 배치용 ID이며 독립 설비에 중복 사용하지 않는다. 저장 스키마 변경은 없다.

## 휘발성 배달 아이템 — 기지 도착 시 제거

- 기지로 귀환할 때 휘발성 배달 아이템을 제거한다.
- 레이드 레벨 사이를 이동할 때는 해당 아이템을 유지한다. 일반적인 레벨 전환이나 액터 EndPlay 자체를 제거 조건으로 사용하지 않는다.
- `RaidArrivalSubsystem`의 실제 맵 로드 완료 콜백에서 정확히 `BunkerMap`에 도착했을 때 휴대 중인 `item.category.volatile` 아이템을 제거한다. 일반 인벤토리·장비·보조 가방·퀵 슬롯 및 그 하위 참조를 정리하며 창고 아이템은 보존한다.
- 이동 요청·실패 시에는 제거하지 않는다. 기존 기지 세이브를 불러오는 경우에도 정리한다. 제거 후 인벤토리 변경을 알리고 기존 저장 경로로 즉시 저장하며, 쓰기 실패는 저장 대기 상태로 남긴다. 새 저장 필드는 추가하지 않는다.

### 택배 상자 획득 상태

- 택배 상자는 `AActor` 기반 전용 픽업 액터이며 루팅 상자·슬롯·루팅 창을 사용하지 않는다. 진행 중인 퀘스트의 미완료 배달 목표를 참조해 휘발성 아이템 1개를 인벤토리에 직접 지급하고 기존 저장 경로로 저장한다.
- 지급 성공 시 해당 상자를 숨기고 충돌·상호작용을 끈다. 인벤토리 변경 콜백과 연속 입력에서도 중복 지급하지 않는다. 공간 부족으로 지급이 실패하면 아이템을 만들지 않고 상자를 그대로 남겨 재시도할 수 있다.
- 상자의 획득 여부는 동일 월드 수명 동안만 유지하는 상태이며 새 저장 필드는 없다. 새로 진입한 레이드에서는 미완료 배달 상자를 다시 획득할 수 있으므로 기지 귀환으로 사라진 배송 물품도 재시도할 수 있다. 휘발성 제거는 상자 코드가 아닌 기지 도착 처리에서 담당한다.

### 배달 목적지 제출 상태

- 목적지 제출은 기존 `item_submitted` 목표의 진행값과 인벤토리 저장을 사용한다. 필요한 수량의 소모와 해당 목표 진행값 변경을 마친 뒤 인벤토리 관찰자·저장 경로에 알린다.
- 저장 복원 후 이미 달성한 목표에는 재제출하지 않는다. 목적지 액터는 별도 완료 상태를 저장하지 않고 퀘스트·인벤토리 이벤트에서 상호작용 가능 여부를 재계산한다. 새 저장 필드나 버전 변경은 없다.

### 역·차량 도착 목표

- `location_reached` / `vehicle_location_reached`는 기존 `NumericQuestProgressStates`의 퀘스트 ID·목표 ID·`CurrentCount`로 저장한다. 목적지별 목표는 0 또는 1이며 도착 액터에 별도의 방문 완료 플래그를 저장하지 않는다.
- 맵 재진입·트리거 재생성·저장 복원 후에도 목표 진행값이 중복 방문을 막는다. 수락 전 방문은 기억하지 않으며 수락 시점에 여전히 영역 안이면 다시 판정한다.
- 배치의 `LocationId`와 목표의 `location_id`, 안정적인 `objective_id`를 유지한다. 지역 번호를 런타임 퀘스트 ID에 인코딩하지 않는다.
- 선택 선행 조건은 기존 퀘스트 보상 완료 상태와 `WorldProgressStates`의 완료 상태를 읽는다. 새 차량 복구/급유 저장 필드나 실시간 연료 상태는 추가하지 않는다. 저장 스키마 버전 변경 없음.

### 본편 지역 통로

- 숲→마을 장애물의 폭파 완료는 `WorldProgressStates[region.forest.village.obstacle]`, 숲→평원 승강기의 수리 완료는 `[region.forest.plains.lift]`에 저장한다. 모두 기존 `Completed` 상태를 사용하며 저장 스키마 변경은 없다.
- 두 수리 재료를 모두 확보했을 때만 인벤토리 소모와 완료 상태 변경을 함께 처리하고 저장한다. 일부 재료만 가진 실패 시도와 이미 완료된 승강기의 재시도는 소모하지 않는다.
- 2026-10-15 기존 폭파 장애물 대신 낙석 BP를 연결했다. `region.forest.village.obstacle`의 기존 완료 저장은 그대로 인정한다. 승강기는 재입장 시 이동 가능 상태로 복원한다. 독립 통로에는 새 저장 ID가 필요하다.
- 숲·마을·평원 간 이동은 같은 레이드 세션이다. 체력·허기·수분 비율을 목적지가 일치하는 새 플레이어에게 한 번 전달한다. 전달 대기값은 `GameInstance` 런타임에만 두며 저장 파일에는 추가하지 않는다. 기지 귀환의 허기·수분 최소 회복 규칙을 지역 이동에 적용하지 않는다.
- 지역 이동 중 레이드 경험치를 정산하지 않는다. 업적용 처치 수와 누적 플레이 시간을 이어가되 로딩·일시정지 시간은 합산하지 않는다. 기지 귀환·사망 등 기존 종료 경로는 유지한다. 자세한 배치는 `Docs/region_travel.md`를 따른다.

### 퀘스트 조건 시설과 레벨 조명

- 시설 상호작용 해금 여부와 조명 그룹 상태는 기존 `NumericQuestProgressStates`의 퀘스트 상태에서 계산한다. 액터 자체에 해금/점등 플래그를 저장하지 않으며 저장 스키마 변경은 없다.
- BeginPlay에서 저장된 퀘스트 상태를 즉시 적용하고, 퀘스트 변경·저장 불러오기·새 게임 초기화 알림으로 다시 계산한다. 기본 해금 및 조명 완료 기준은 보상 수령까지 끝난 `RewardCompleted`다.
- BunkerMap의 BaseFacilityQuestLights는 작업실(010)·욕실(011)·침실(012)·식당(013)의 보상 수령 완료 상태를 각 조명에 적용한다. 맵 저장 기본 상태는 이 네 조명 꺼짐이며, Center·LaundryRoom·BedroomWall·Out1·Out2는 컨트롤러 대상에서 제외해 상시 켜짐을 유지한다. 에디터 눈 아이콘 대신 라이트 컴포넌트 가시성을 설정한다.
- 냄비의 수증기와 뚜껑 흔들림은 RequiredCompletedQuestId(기본 013)의 RewardCompleted에서 계산한다. BeginPlay·퀘스트 변경·로드·새 게임 초기화 시 재평가하며, 잠기면 Niagara를 즉시 종료하고 숨긴다. 별도 점등/끓음 저장 필드는 추가하지 않는다. 값 0은 장식용 무조건 동작 설정이다.
- 시설의 `UnlockConditions` 및 조명 컨트롤러의 `LightGroups`는 BP/레벨 저작 데이터다. 시설 서비스에서 별도 영속 데이터를 추가할 경우 해당 데이터의 저장 규칙을 별도로 정의한다. 기본 시설 템플릿의 사용 이벤트 자체는 영속 상태를 만들지 않는다.

## Editor-only quest completion cheat

The quest cheat edits the last selected existing save slot in the active dataset only while gameplay is stopped. It validates the complete prerequisite graph before changing `NumericQuestProgressStates`. The inclusive action completes the target and every ancestor; the prerequisites-only action preserves the target's existing state. Edited rows use `RewardCompleted` and objective counts at their required values. Tracking is cleared only when the tracked quest is among the completed IDs. Legacy numeric IDs follow the existing migration helper.

No items, coins, experience, dialogue, or world-progress rewards are executed by this editor operation. Unrelated slot data remains intact. Subsequent gameplay load keeps its existing quest-derived reconciliation behavior. The offline save tool validates identity/version, compares the source hash, creates a byte-identical `.tool-backup-*` backup, and writes through the existing fail-closed save path. PIE/SIE, pending play, editor standalone, and Launcher sessions lock the UI; the mutation callback rechecks the lock and the save backend rejects live gameplay worlds. No new save fields or schema version are introduced. See [quest cheat usage](quest_cheats.md).

## 상점 상품 퀘스트 해금

- 상품 해금은 `required_completed_quest_ids`와 기존 `NumericQuestProgressStates`의 `RewardCompleted` 상태를 조회해 계산한다. 별도 상품 해금 저장 플래그나 저장 버전 변경은 없다.
- 잠긴 상품을 목록에서 숨겨도 재고의 원래 `ShopId`·`SlotIndex`·`ItemId` 키를 유지한다. 기존 재고 저장값을 재사용하며 해금·재잠금·저장 불러오기·상점 재개방으로 재고를 초기화하지 않는다.
- 퀘스트 상태 복원 또는 새 게임 초기화 후 열린 상점 목록도 다시 계산하고 구매 경로는 현재 상태를 재검증한다. 기존 저장 재고가 있으므로 상품 배열 순서와 아이템 정체성을 임의로 바꾸지 않는다.

### 저장된 월드 완료 상태를 사용하는 퀘스트 목표

- `world_progress_completed`는 정의의 `world_progress_id`로 기존 `WorldProgressStates`의 `Completed` 상태를 읽고 기존 `NumericQuestProgressStates`에 목표 진행값 0/1과 보상 대기 상태를 저장한다. 새 저장 필드·버전 변경은 없다.
- 수락한 목표는 월드 진행 변경 시 판정한다. 수락 전에 완료된 오브젝트는 수락 시 인정하며, 미수락 퀘스트나 선행 조건을 자동으로 건너뛰지 않는다. 완료한 목표는 되돌리지 않고 보상 수령은 기존 경로로 처리한다.
- 실제 세이브 로더는 월드 오브젝트를 먼저 복원하고 퀘스트 진행을 복원한다. 이 시점에 누락된 목표 진행값을 조용히 보정하되 토스트·자동 저장·보상 지급은 하지 않는다. 해당 액터가 현재 맵에 없어도 복원 가능하다.
- 월드 변경 알림의 목표 동기화는 자체 저장하지 않는다. 폭파·수리 등 생산자의 저장 정책을 따르므로 재료 소모·월드 개방·퀘스트 진행이 함께 저장되며 지연 저장 요청을 무시하지 않는다. 반복 알림·재입장으로 진행값이나 보상을 누적하지 않는다. 전투 테스트 세션에서는 판정하지 않는다.

### 숲·마을·평원 진입 도전과제

- 실제 게임 월드 도착 시 숲은 `region.forest`, 마을은 `region.village`, 평원은 `region.plains`를 기존 도전과제 저장의 `ReachedLocationIds`에 기록한다. 도전과제 01·07·19의 해금은 기존 저장 경로를 사용하며 새 필드나 스키마 변경은 없다.
- 이동 요청만으로 진입 기록을 만들지 않는다. 개발용 `RaidMap`은 이 세 지역 기록을 만들지 않으며 같은 월드의 준비 완료 알림 반복은 중복 처리하지 않는다. 이전 버전에서 기록되지 않은 방문은 다음 실제 입장 때 판정한다.


### 퀘스트 소유 조건과 미수령 보상 보관함

- NPC 완료 담당자와 제출 대상이 같은 `item_submitted` 목표는 로드 시 `item_possessed`로 해석한다. 별도 월드 목적지 제출은 유지한다. NPC 상호작용은 UI만 열고, 명시적인 보상 수령 버튼에서 아이템 소비·보상·완료 저장을 처리한다.
- 숫자 퀘스트 저장에 `bOwnershipReadinessRecorded`, 목표 저장에 `SubmittedItemCount`를 추가한다. 기존 저장에서 전자가 없는 경우 위 변환 대상의 이전 진행 수량을 이미 제출한 수량으로 보존한다. 이후에는 현재 소유분과 제출분을 구분하여 저장하고, 소유 판정과 보상 수령 차감에서 이미 제출한 수량을 제외한다. 새 진행은 생성 시 구분 플래그를 설정하므로 과거 소유량을 제출분으로 오인하지 않는다. 저장 버전 초기화는 하지 않는다.

- `item_possessed`의 진행도와 보상 대기 상태는 실제 인벤토리·보조 가방·창고가 모두 복원된 뒤 현재 수량으로 조용히 재계산한다. 이전 획득 누적 진행값만으로 제출할 수 없으며 이미 `RewardCompleted`인 퀘스트는 보존한다. 재계산은 로드 중 토스트나 저장을 발생시키지 않는다.
- 퀘스트 제출 시 세 컨테이너 수량을 함께 검증한 후 인벤토리·보조 가방·창고 순으로 일괄 차감한다. 보상 정의 검증은 차감 전에 끝내며, 제출·보상·퀘스트 상태 변경을 모두 마친 후 알림과 기존 퀘스트 저장 요청을 실행한다.
- `UTunaSweeperSaveGame::PendingQuestRewards`에 미수령 보상의 아이템 ID·수량 배열을 추가했다. 기존 슬롯은 빈 배열로 로드되며 저장 버전을 올려 초기화하지 않는다. 데이터셋/저장 슬롯별로 분리되고 새 게임·슬롯 초기화 때 비우며 레이드 사망으로 소실되지 않는다.
- 보상은 인벤토리에 수용 가능한 수량부터 넣고 나머지를 최대 스택 크기 이하 항목으로 보관한다. 받기는 선택 항목 전체를 수용할 수 있을 때만 인벤토리 추가와 보관 항목 삭제를 함께 처리하고 저장한다. 실패 시 양쪽 모두 유지한다. 재진입 수령·전투 테스트 세션의 지급/수령은 차단한다.
- 미수령 보관함은 일반 창고 슬롯이 아니며 직접 입고·아이템 이동·퀘스트 제출에 사용할 수 없다. UI와 액터가 없어져도 보관 데이터는 게임 저장에 남는다.

### 레이드 리프트 복구와 도착 지점

- 고용량 콘덴서 1개 소모와 `world.passage.raid_lift` 완료는 기존 재료 소비 트랜잭션 안에서 함께 갱신하고 저장한다. 같은 진행 ID의 리프트는 복구 상태를 공유하며 중복 수리는 차감하지 않는다. 저장 스키마를 추가하지 않는다.
- 목적지 선택의 도착 그룹(Forest, VillageEast, VillageWest, Plains)은 게임 인스턴스의 일회성 메모리 상태다. 해당 맵의 최초 시작 지점 선택에서 소비하고, 다른 목적지·일반 이동·로드 완료·이동 실패 시 남은 요청을 지운다. 저장 슬롯에 직렬화하지 않는다.
- 일반 이동은 전용 리프트 시작 지점을 후보에서 제외한다. 리프트 이동은 선택한 그룹만 사용하며 없는 그룹을 다른 지역이나 원점으로 대체하지 않는다. 레이드 간 인벤토리와 생존 수치 보존은 기존 이동 경로를 따른다.


## Editor quest cheat inspection and target reset

- `quest-inspect` reads the selected slot and active quest catalog without writing the save. It reports quest IDs, title string keys/resolved titles and persisted states, distinguishing unmet prerequisites from available quests.
- `quest-reset` uses the existing versioned offline save transaction, exact backup and expected-hash checks. After ancestor validation/completion it removes only the target numeric quest progress (including objective counts) and clears tracking if it points to that target. Legacy quest IDs are migrated in memory before editing.
- Descendants, unrelated quests, world progress, scenario flags and already granted rewards remain unchanged. No items, coins or XP are granted. Existing save schema is unchanged. Runtime auto-accept rules still apply after loading.
- The editor panel disables operations during play, polls selected-slot/file changes and refreshes immediately after a cheat. It never writes when listing quests.

## Editor quest prerequisite audit and repair

- `quest-audit` reads the selected slot without writing it and checks ancestor chains of all started numeric quests. Available/unaccepted branches are not errors. Reports include incomplete prerequisite edges and structural failures (cycles or missing definitions).
- `quest-fix` requires the audited `expectedHash`, validates the complete batch before mutating it, and uses existing backup and atomic-save checks. Only unmet prerequisites become RewardCompleted with filled objective counts; tracking of those completed quests is cleared. No reward delivery or runtime events occur. Other progress and existing rewards are preserved.
- Cycles/missing definitions block the entire repair. The editor shows Fix only for repairable audit results, invalidates the result after save/slot changes or other cheats, and audits again after successful repair. Existing play locks remain in effect. No save schema changes.

### 지하 창고 접근 범위

- 메인 데이터셋에서 지하 창고 개방 전에는 앞 20칸만 사용할 수 있다. 창고 자체는 처음부터 열 수 있고, 개방 상태는 기존 퀘스트 보상 수령 완료 상태에서 계산한다. 데모는 기존 용량 정책을 유지한다.
- StorageSlots와 StorageSlotCapacity의 원본 저장 형식·수량은 유지한다. 기존 저장의 제한 범위 밖 물품은 삭제하거나 이동하지 않고 그대로 보존하며, 개방 후 다시 접근할 수 있다. 별도 개방 bool을 중복 저장하지 않는다.
- 제한은 UI뿐 아니라 슬롯 이동·정렬·제작 재료·퀘스트 제출에도 적용한다. 개방 보상 수령 알림으로 목록과 접근 범위를 즉시 갱신하고, 재접속 시 저장된 퀘스트 상태에서 복원한다.

## Editor development slot snapshots (2026-10-14)

The editor-only TunaSweeper > 세이브 스냅샷 tool reads the active dataset and last selected slot using the existing selected-slot settings. It copies the last committed gameplay slot, not unsaved PIE memory. Snapshot creation and restore are disabled during queued/running PIE and editor-launched standalone sessions. Runtime builds expose no snapshot menu or API.

Snapshots are independent, exact-byte copies under `Saved/DeveloperSaveSnapshots/<Main|Demo>/SlotNN/<source-path-hash>/`. Local creation time and a GUID identify each copy; source-root isolation prevents mixing accounts/workspaces that use the same slot number. The list includes manual snapshots and automatic pre-restore backups. No gameplay schema/version changes are introduced.

The editor snapshot creation field requires a nonblank Unicode memo (up to 2048 UTF-16 code units); the creation button is disabled for empty or whitespace-only input. A nonempty memo is stored as UTF-8 JSON in `<snapshot>.sav.json`, separately from the unchanged gameplay bytes. Snapshot list rows show creation time and memo on one line, without a snapshot-type label or memo prefix; copies without metadata remain supported and display the localized no-memo label. Malformed metadata does not prevent listing or restoring the gameplay copy. The memo input clears only after a successful creation. The underlying snapshot API and automatic pre-restore backups still support an omitted memo.

The selected snapshot can be deleted after a confirmation showing its timestamp and memo. This removes only that snapshot and its sidecar in the selected dataset/slot/source-root directory; active gameplay saves and other snapshots are untouched. Deletion shares the tool lock and play-session restrictions with creation/restore and rechecks slot selection after confirmation. A corrupt or missing current gameplay save does not prevent cleanup of developer snapshot files. A failed sidecar deletion reports failure; retrying the same snapshot ID can remove an orphaned sidecar. No new gameplay save fields are added.

Restore requires a readable, valid current slot and a CRC-valid snapshot whose exact save class, dataset, slot and supported save version match. It rejects path traversal and rechecks current bytes before writing, retains the current slot as a separate automatic backup, and uses the existing candidate/previous fail-closed save writer. Restored gameplay fields and original saved time are retained. Load the selected slot in the next play session to use the restored state; there is no live in-memory replacement. A pre-restore backup can be selected to undo a restore.

The snapshot includes all gameplay-slot fields (inventory, storage, pending rewards, quests and world progress). Account-global achievements, outfit unlocks, language/settings and other slots are not captured or rolled back. These developer snapshots are outside the active SaveGames/cloud-save root and are not packaged or tracked in Git.

## Quest-gated NPC food exchange and item reward notices

- Optional resident NPC feeding is unlocked from the existing prerequisite quest RewardCompleted state; no separate unlock flag is saved.
- A feeding exchange consumes exactly one carried item (inventory or auxiliary bag, never storage) and grants its configured reward before inventory notifications and the normal item-state save boundary. Invalid or unavailable exchanges leave inventory unchanged. Full inventory uses the existing persisted PendingQuestRewards receiving storage.
- Item reward notifications use an independent transient FIFO channel. Each reward entry displays for one second; the queue and delayed speech timers are not serialized. World cleanup discards reward notices, and NPC EndPlay cancels delayed speech. Save version and slot schema are unchanged.

### Extraction completion protection

Once the extraction hold completes, the source world is paused and the extracting pawn's damage flag is disabled before persistence and return presentation begin. This freezes enemy movement, animation-driven attacks and attack timers; projectile and direct damage checks also respect the disabled damage flag. Slate experience presentation and the level-transition subsystem continue while the world is paused. Travel failure or extraction actor removal cancels the presentation and restores only the pause/damage state owned by this extraction. These guards are transient, are not written into a save, and do not grant invulnerability to the new pawn in the destination world.

### 아이템 장전형 터널 낙석

- 기존 `WorldProgressStates[region.forest.village.obstacle]`에 `InProgress` + `ProgressQuantity=1`이면 장전, `Completed`이면 개방이다. 저장 스키마·버전 변경은 없다.
- 장전 트랜잭션은 소지품 1개 소모와 월드 상태를 먼저 저장하고 성공 후 변경 알림을 발행한다. 저장 실패 시 아이템 인스턴스·인벤토리·보조 가방·월드 진행을 원복하며 타이머를 시작하지 않는다.
- 장전 상태로 로드하면 추가 아이템 없이 5초 카운트다운을 재개한다. 남은 시간·경고 머티리얼·피해는 저장하지 않는다. 종료 시 이전 월드의 타이머를 해제한다.
- 폭발 완료 저장이 실패하면 길을 막은 채 1초마다 재시도한다. 성공 이후에만 한 번 피해·이펙트를 적용한다. 이미 완료된 저장 복원에는 피해·이펙트가 없다.
- 월드 상태를 저장한 후 퀘스트 알림을 보내므로 저장의 퀘스트 목표가 아직 갱신 전일 수 있다. 기존 로더의 월드 목표 재조정으로 복구하며 보상은 자동 지급하지 않는다.


### 퀘스트 자판기 이전과 시설 조건 차량 생성

- `ATunaSweeperQuestVendingActor`의 기지 역할은 이전 퀘스트 보상 수령 완료 시 즉시 출현한다. 현장 역할은 이번 방문에서 보였던 경우 보상 수령 후에도 메시·충돌을 유지하고 인사 대화만 제공한다. 현장 액터가 언로드된 뒤 다시 생성되면 저장된 완료 상태에 따라 숨는다. 방문 유지 bool은 액터 메모리에만 두며 저장하지 않는다. 수락·소유 목표 달성만으로 이전하지 않는다.
- `ATunaSweeperSpecialVehicleSpawner.RequiredWorldProgressObjectId`가 지정되면 기존 월드 진행 상태의 `Completed`에서만 생성한다. 복구 알림으로 즉시 생성하고 재방문 시 저장된 시설 상태를 다시 확인한다. 빈 ID는 기존 동작이다. 한 방문에서 한 번만 생성하는 제한은 유지한다.
- 자판기 상품 해금은 기존 퀘스트 완료 조건을 재사용하며 재고·코인은 기존 상점 저장 경로를 사용한다. 신규 저장 필드나 버전 변경은 없다.

### 다중 재료 시설 수리와 반복 생산

- 시설 수리는 소지품의 요구 재료 전체와 기존 월드 진행 `Completed` 상태를 한 번에 저장한다. 재료 부족·잘못된 수량·저장 실패 시 일부 재료만 소모하지 않는다. 저장 성공 후 인벤토리와 월드 진행 알림을 보낸다.
- 생산 시설은 저장된 수리 완료 상태에서만 1회분 재료를 결과물로 교환한다. 창고 물품은 사용하지 않으며, 결과물을 넣을 공간 부족 또는 저장 실패 시 교환 전체를 원복한다. 생산 가능 여부를 별도로 저장하지 않는다.
- 기존 시설 퀘스트의 보상을 이미 수령한 저장에 시설 기록 자체가 없으면, 시설 첫 로드에서 무상으로 수리 완료 기록을 저장한다. 명시적인 미완료 시설 기록은 덮어쓰지 않으며 저장 실패 시 다음 로드에 재시도한다.
- 정의 변경으로 새 목표가 생긴 미수령 퀘스트는 저장된 보상 가능 상태를 다시 검사한다. 새 목표가 미완료이면 진행 중으로 되돌리고 소유·월드 상태에서 재평가한다. 보상 수령 완료 기록은 유지한다. 저장 스키마·버전 변경은 없다.


### 순환철도 비석 윤활과 즉시 이동

- 같은 순환선은 어느 역에서든 오일 1개를 사용하면 전체 역의 기존 `WorldProgressStates[rail.lubrication.<StationId>]`를 `Completed`, `ProgressQuantity=1`로 함께 저장한다. 오일 1개 소비와 모든 역의 완료 기록은 하나의 시설 수리 배치 트랜잭션이며, 저장 실패 시 재료와 전체 기록을 원복한다. 첫 사용 직후 모든 비석에서 오일 사용 항목이 사라지고 다시 소비할 수 없다. 다른 철로에는 전파하지 않는다.
- 동일 철로의 역 중 하나라도 윤활 완료 기록이 있으면 모든 역의 다음 역 이동을 즉시 허용한다. 퀘스트 수락·목표 전체 완료·보상 수령과 독립적이며, 재방문과 세이브 로드 시 같은 기록에서 복원한다. 기존 저장에 기록이 없으면 잠금 상태다. 새 저장 필드나 버전 변경은 없다.
- 예전 저장에 한 역의 윤활 기록만 있어도 오일 항목은 즉시 숨긴다. 비석 상호작용의 BeginPlay에서 이미 지불한 기록을 기준으로 나머지 역의 완료 기록을 추가 소비 없이 일괄 저장하여 기존 역별 퀘스트 목표도 인정한다. 저장 실패 시 원래 기록을 유지하고 다음 초기화 때 재시도한다. 퀘스트 보상만으로 무상 윤활을 생성하지 않는다.
- 열차 위치·정차 잔여 시간·진행 중 페이드·입력 잠금은 저장하지 않는다. 이동은 기존 철로의 정방향 다음 역으로만 진행하고, 검은 화면에서 플레이어를 다음 승강장에 배치한 뒤 열차를 그 역에 정차시킨다. 정상 운행은 해당 역의 정차 시간이 지나면 재개한다.
- 퀘스트 목표는 저장된 월드 완료 기록을 기존 `world_progress_completed` 재조정 경로로 읽는다. 윤활을 먼저 했더라도 나중에 수락하면 인정되며, 완료 퀘스트만으로 윤활 또는 이동을 무상 해금하지 않는다.

### 에디터 월드 진행 치트

- `world-inspect` / `world-set`은 기존 `WorldProgressStates`의 개별 ObjectId 상태·수량만 조회/변경한다. 고정 배치 카탈로그와 기존 저장의 ID를 합쳐 표시하며 데모 취수구 이물질 제거·밸브 수리는 제외한다. 저장 필드나 버전은 추가하지 않는다.
- 초기화는 기록 삭제 대신 명시적인 `InProgress`, 수량 0을 저장한다. 시설의 과거 퀘스트 보상 기반 누락 기록 보정이 초기화를 무효화하지 않는다. 철도 공유 보정 등 기존 런타임 규칙은 유지한다.
- 기존 슬롯 검증·원본 해시·정확한 백업·fail-closed 저장을 재사용한다. 퀘스트·인벤토리·보상은 직접 수정하지 않고 PIE/독립 실행 중 변경을 차단한다. 상세 사용법은 [월드 진행 치트](world_progress_cheats.md)를 참조한다.

### 퀘스트 보상 후 NPC 반복 교환

- NPC의 반복 교환 해금은 지정 퀘스트의 기존 `RewardCompleted` 상태에서 계산한다. 별도 해금 플래그·횟수·저장 버전을 추가하지 않으며 보상 수령 직후와 재로드 때 같은 조건을 적용한다.
- 교환은 소지품의 입력 수량 소모와 결과물 지급을 한 번에 저장한다. 창고 물품은 사용하지 않으며, 재료 부족·가방 공간 부족·저장 실패 시 부분 교환을 남기지 않는다. 인벤토리와 획득 알림은 저장 성공 후 발행한다.
- 기존 시설 생산도 같은 배치 교환 함수를 사용하되 시설의 저장된 수리 완료 조건을 먼저 확인한다. NPC 교환은 퀘스트를 재수락하거나 추가 보상 수령 상태로 바꾸지 않는다.


### 수리 승강기 문과 실내 이동

- 문 개방 여부는 기존 `WorldProgressStates[region.forest.plains.lift]`의 `Completed` 상태에서 계산한다. 새 저장 필드·스키마 버전·재료 소모 경로는 추가하지 않는다.
- 수리 직후에는 좌우 문을 애니메이션으로 열고, 수리 완료 상태를 로드하거나 지역에 재입장하면 처음부터 완전히 열린 상태로 복원한다. 진행 상태를 초기화하면 문을 닫고 실내 이동 권한을 회수한다.
- 개폐 보간값, 플레이어의 실내 여부, 이동 중 중복 요청 방지는 세션 상태이며 저장하지 않는다. 에디터의 문 열림 미리보기는 실제 수리 기록을 변경하지 않는다.


### Hopper 메카 파괴 시 전투 종료

- 기본 Hopper는 메카 내구도 0에서 기존 적 처치 알림을 한 번 발행한다. 수락된 처치 목표의 기존 진행값 저장 경로를 사용하며 보상은 자동 지급하지 않는다.
- 열린 조종석 잔해와 메카·조종사 전투 상태는 현재 월드의 임시 상태다. 좌석 슬라이드·로켓 사출 진행 시간과 화염·연기 역시 저장하지 않는다. 지역 재로드는 기존 적 스폰 규칙을 따르며 잔해 보존용 저장 필드나 버전은 추가하지 않는다.
