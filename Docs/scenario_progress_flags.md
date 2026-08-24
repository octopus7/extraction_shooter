# Scenario Progress Flags

This document defines how scenario/cutscene progress is stored and used.

## Goal

Scenario progress must be tracked per save slot, not globally. A player who completed a scenario in slot 1 must still see that scenario in slot 2 if slot 2 has no matching completion flag.

Scenario levels themselves are reusable presentation spaces. Do not block entry to a scenario map globally. Routing decisions should check flags only at the gameplay entry point that needs to skip an already completed scenario.

## Storage

- Save object field: `UTunaSweeperSaveGame::CompletedScenarioFlags`
- Runtime owner: `UTunaSweeperGameInstance`
- Runtime container: `TSet<FName> CompletedScenarioFlags`
- Save version containing this field: `3`

`CompletedScenarioFlags` is saved inside each slot's save file. It is not stored in `UTunaSweeperSaveSettings`; that settings save only tracks the last selected slot.

## Current Flags

| Flag | Meaning |
| --- | --- |
| `scenario.opening.awakening` | The first-start opening presentation has completed and the player successfully entered the bunker. |
| `dialogue.demo.toilet_intro` | The Demo-only two-line toilet-failure introduction has completed for the save slot. |
| `dialogue.main.bunker_intro` | The Main-only first bunker dialogue has completed for the save slot. |

## Current Routing

The intro menu asks `UTunaSweeperGameInstance::ResolveInitialGameplayLevelName()` for the first gameplay level.

- Demo routes directly to `BunkerMap` and does not use the opening flag as an entry gate.
- Main routes to the manifest's `OpeningScenarioMap` while `scenario.opening.awakening` is incomplete.
- Main routes directly to `BunkerMap` after `scenario.opening.awakening` is complete.

This applies to normal start and the Always New Start debug path after the selected save slot has been reset.

## Completion Timing

The opening scenario does not mark its flag immediately when the presentation starts or ends.

1. The scenario presentation requests bunker travel and calls `BeginScenarioBunkerEntry(ScenarioFlag)`.
2. When `BunkerMap` begins play, the player controller calls `CompletePendingScenarioBunkerEntryIfNeeded()`.
3. The game instance marks the pending flag complete and saves immediately.

This means the flag represents successful bunker entry after the presentation, not merely seeing part of the presentation.

Dialogue completion flags are selected from the active build flavor's `ScenarioDefinitions.json` and marked when the resolved dialogue sequence finishes. Demo uses `dialogue.demo.toilet_intro`; Main uses `dialogue.main.bunker_intro` after requiring `scenario.opening.awakening`. The legacy test flag `dialogue.mole.bunker_intro` is not reused.

See [scenario_data_system.md](scenario_data_system.md) for trigger, condition, localization, and flavor-pack rules.

## Reset Behavior

Starting a new save slot initializes an empty scenario flag set.

Deleting a selected slot through the Always New Start debug button removes that slot's save data and starts a new slot state, so scenario flags for that slot are cleared. Other slots are unaffected.

## Adding A New Scenario Flag

When adding a new scenario/cutscene gate:

1. Define a stable `FName` flag, preferably using the `scenario.<group>.<event>` pattern.
2. Store completion by calling `MarkScenarioProgressFlag()` or by using the pending-entry pattern if completion should happen after level travel.
3. Route only the relevant entry point based on the flag.
4. Do not prevent manual or scripted entry into the presentation map unless that is a separate explicit requirement.
5. Update this document with the new flag and its completion rule.
