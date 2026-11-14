# Movement cheat implementation plan

The user authorized implementation in this shared checkout and explicitly requested a commit after completion on 2026-11-15. Preserve unrelated pending changes.

## Final requirements

- External TunaBuildHelper process access controls a memory-only gate, default off each launch. F6 cycles 1, 3, 8; accelerated mode suppresses player-related enemy detection and combat.
- Helper uses Main, Cheat and Folder tabs. Entering Cheat searches automatically, selects a sole process, and displays its actual current gate state.
- In the M-key map, F9 over an ordinary existing marker opens an in-game Yes/No confirmation. There is no dedicated cheat bookmark, drawing, storage or help text. Ordinary markers retain normal persistence.
- Confirmation is a saved editable WBP, with localized text and an authored tree. Runtime code does not construct its tree. Confirm revalidates gate, map, marker and safe landing.
- Remove the retired local MovementCheats bookmark directory. Activation/speed never enter saves, cloud files or packaged defaults. Package/upload validation rejects enabled defaults and helper/legacy data contamination.

## Validation

- Helper real process read/write/identity/version/exit checks and WPF tab rendering.
- UE 5.7 Editor and Shipping builds; runtime gate/combat/teleport and map coordinate regression tests.
- Saved WBP reload, hierarchy, localization, Yes/No behavior and rendering checks. Remove one-off generator before final rebuild.
- Review final diff and commit only this movement-cheat/helper work and its request records. No store upload or live cloud sync.

## Implementation notes

Activation is standalone only. Teleport finds a walkable surface and checks capsule clearance. Existing ordinary patrol and unrelated faction combat continue. The earlier dedicated bookmark and OS-native dialog implementations are superseded by the final requirements above.
