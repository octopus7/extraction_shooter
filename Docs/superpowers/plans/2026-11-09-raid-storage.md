# Persistent raid storage implementation plan

> Execution: implement in this chat using executing-plans; user authorized implementation and one final task commit after reviewing the concept and LED correction.

## Agreed design

One 10-slot secondary warehouse in the raid. All mistakenly duplicated actors address the same storage within the active gameplay save slot; opening a duplicate warns on screen. Contents survive travel, death, abandonment and restart, and changes save immediately. New games and different save slots remain separate, like the existing warehouse. Existing loot interaction/UI and lid animation are reused; no new widget classes or widget trees. A real editable Blueprint contains replaceable iron chest presentation and a 5-column, 2-row emissive indicator panel. Empty is off, item 3004 quantity 1–9 is dim green, quantity 10 is bright green, other items are orange. Dimming is editable for in-game exposure tuning; no cyan and no light components.

## Approach

Extend the existing local save's owned item graph with RaidStorageSlots rather than a separate file (avoids separately committed inventory/chest transfers). A loot-actor subclass opens this storage through the existing container UI without random generation or repeated loot rewards. Keep one shared runtime copy in GameInstance. Author presentation in a saved BP, with a temporary editor asset builder removed after validation. Do not place it in an unspecified gameplay map.

## Work

- [x] Add failing automation for save field and actor existence, then persistence/LED/asset tests.
- [x] Integrate shared 10-slot state, save/load/reset/death/combat-lab boundaries and immediate save failure reporting; suppress repeated acquisition credit on withdrawal.
- [x] Add actor subclass with editable meshes, emissive colors, index-tagged LED components, close handling and duplicate warning using localized keys.
- [x] Build UE 5.7; author BP and materials, validate asset components and runtime round-trip.
- [x] Remove one-off builder, rebuild/retest, review final changes, update save contract and user guide/log, commit only task files.

## Review focus

Death must preserve stored items and attached items; withdrawal must not award loot repeatedly; stale container state must not leak into another save slot; save failures must not be reported as success; old saves start with empty secondary storage. Combat tests must not overwrite user storage. Real asset must load with ten uniquely indexed LED meshes and no light component.

## Validation

Run TunaSweeper.RaidStorage automation in UE 5.7 command-line editor with isolated test save slots, plus existing inventory/save coverage as appropriate. Inspect generated Blueprint and material assets and a rendered preview. Existing unrelated questions and ProductionPayload work are excluded from the commit.

## Results

- Initial contract test failed on missing save field and actor; implementation resolved both.
- UE 5.7 final build passed after removal of the asset commandlet. RaidStorage + Save + Inventory: 18/18 passed under the isolated user directory.
- Independent review found no confirmed runtime defects. XP regression test was strengthened with an ordinary-loot positive control and split withdrawal. Removing both guards reproduced failures (8 expected versus 88/112); restoring them passed.
- Actual BP rendered successfully with a 5x2 panel and distinct full/dim green, orange and off states. Preview changes were transient, with no gameplay map saved.
- Minor coverage limits: attached-item death survival, use/remove disk writes, distinct saved-slot switching and combat-lab restore were reviewed statically but do not have dedicated new scenarios in this test group. No gameplay-map PIE acceptance was claimed.
