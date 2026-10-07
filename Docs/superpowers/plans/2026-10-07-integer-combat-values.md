# Integer combat values implementation plan

**Goal:** Round damage once after all modifiers and keep health, food, hydration, stamina and scratch gauges integer-valued regardless of storage type. Weight and time/progress remain fractional, as confirmed by the user.

**Design:** Preserve intermediate ammo/difficulty/armor precision. A shared numeric helper finalizes damage and normalizes gauge values. Continuous depletion/recovery retains a private fractional remainder and applies whole points; discrete costs/effects round once. Clamped boundaries discard outward remainder, while loading/resetting state clears transient remainder. Existing float storage and save schema remain compatible.

**Constraints:** UE 5.7; existing localization for UI; preserve concurrent movement/level edits; commit implementation, tests and logs together. Dummy overkill keeps its existing comparison-hit return convention.

- [x] Observe failing final-rounding and gauge normalization regression tests.
- [x] Apply final rounding at player/enemy/dummy and other damage receivers, preserving integer health/durability limits. Keep info-panel previews consistent with an unarmored hit.
- [x] Normalize state gauges and add remainder accumulation for continuous vitals/stamina/dummy recovery; round discrete scratch and action costs. Preserve existing frame-rate independent rates and clamping.
- [x] Verify final rounding boundaries, actual health loss, mixed armor, projectile previews/snapshots, difficulty order, depletion/recovery at different frame rates, full/empty gauge bounds and reset/load behavior.
- [x] Update combat/save documentation, perform review, build/test and prepare the task log and single commit; reopen the project editor.

Validation: UE 5.7 Development Editor build succeeded. Initial regression failed 11 assertions; dedicated remainder boundary regression failed two assertions before the fix. Final combat/gauge/preview/difficulty/stamina/boss/Hopper/burn coverage passed all 19 tests (one existing duplicate `ui.common.confirm` warning). The real range BP verification passed for five tiers, rounded 20.5-input hits and 1,110 integer-health recovery samples. Review's two findings (boundary remainder loss and tomato component health normalization) were fixed and re-reviewed.

Additional vehicle integration coverage: corrected the test's stale smoke count to exclude its independent explosion component. Fractional maximum durability and hit rounding, damage and destruction assertions passed; the separate `Blocked exit releases at current seat` assertion still failed. Vehicle dismount code was not modified. Reports: `Saved/Automation/IntegerCombatValues/{Before,BoundaryBefore,After2,VehicleAfter}` and `Saved/Logs/IntegerCombatValues-Range.log`.
