# Practice dummy armor implementation plan

> **For agentic workers:** Execute inline with superpowers:executing-plans.

**Goal:** Let BP instances independently equip body/head armor tiers 0–4 and configure the five completed shooting lanes as tiers 0–4.

**Architecture:** The native practice dummy resolves each selected tier to a real catalog item and uses `TunaSweeperArmor` for damage. Existing headshot, immortal minimum health and recovery rules remain. Actual level changes wait for the concurrent range expansion to finish.

**Tech Stack:** Unreal Engine 5.7 C++, Blueprint instance properties, editor Python verification.

**Spec:** User request received 2026-10-07 18:59:07; existing combat rules in `Docs/armor_penetration.md`.

## Global constraints

- Body/head armor retain the project's existing whole-character defense sum.
- Tier 0 removes the equipment slot; tiers 1–4 use catalog defense values.
- No new hardcoded gameplay UI strings or new save-game state.
- Keep concurrent range assets untouched until their owner finishes; commit implementation, verification and request log together.

## Review focus

Mixed body/head tiers; missing catalogs; out-of-range BP values; deliberate headshots; point versus generic damage must retain predictable behavior.

## Task 1: BP armor and combat

- [x] Add regression coverage in `TunaSweeperPracticeDummyDamageTests.cpp` for reflected editable tier properties; observe failure before implementation.
- [x] Add `BodyArmorTier`, `HeadArmorTier`, `ConfigurePracticeDummyArmor(int32,int32)`, item-ID getters and `GetEffectiveDefense(int32)` to the native actor header/cpp. Select matching catalog equipment deterministically; use shared item defense after headshot multiplier.
- [x] Verify naked and all 16 armor/ammunition combinations, mixed tiers, removal, clamping, health deltas, full absorption, headshots and generic damage. Run existing armor and dummy tests.

## Task 2: Saved five-lane setup

- [x] Monitor the other range conversation via a thread heartbeat, stopping it once placement is done.
- [x] When its work finishes, set existing `Target_01` through `Target_05` to paired body/head tiers 0–4 in `L_Basement`; update the placement manifest.
- [x] Extend read-only target runtime validation for persisted tiers, item IDs, actual damage and recovery; reload saved map and verify. Remove the one-off placement script and revalidate.
- [x] Document the BP controls and lane mapping, record actual elapsed time, review, commit only this task, and open the project editor.

## Execution rulings

- Work in the shared checkout because the user requests integration with assets being authored there; stage explicit paths only.
- The user has authorized implementation and follow-up placement; routine design choices do not require another approval.

## Verification evidence

- Before: ArmorEquipment failed because BP tier properties were absent.
- After: UE 5.7 Editor build succeeded; ArmorEquipment, HitDamage, and FourTiersAndActualDamage passed (3 tests, 0 errors).
- Saved map: target_armor_validation.json and target_runtime_validation.json passed after removing the one-off configuration script; independent code and placement review found no important issues.
- Follow-up heartbeat paused because actual five-lane placement completed in this turn.
