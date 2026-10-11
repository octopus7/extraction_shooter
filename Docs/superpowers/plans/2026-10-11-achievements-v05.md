# TunaSweeper achievements v0.5 implementation plan

> Execution: inline using executing-plans; final independent code review. All source, verification and task documentation belong in one task commit.

Goal: extend the existing UE 5.7 achievement subsystem for the supplied 20-slot v0.5 specification, preserving exact ko/en/ja names and actual gameplay semantics.

Architecture: retain the existing model, GameInstance subsystem, independent safe-save container and platform publisher. Add narrowly typed gameplay facts and transient raid state. Subscribe to existing inventory/world-state delegates. Unconfirmed target IDs, region mappings, facility membership and platform mappings remain empty and disabled.

Specification: user-provided TunaSweeper 도전과제 기획 목록 v0.5 (2026-10-11) and implementation requirements in this task. Fixed IDs 01–20; 14 can never unlock, including remote synchronization. The 17 quantity is item 3004 in carried inventory plus base storage, confirmed by user.

## Constraints and review focus

- Do not edit quests, prerequisites, gameplay rules, authored maps or private narrative data.
- Localization uses existing UITextStrings.csv and text subsystem, never literal UI fallbacks.
- Extraction samples health and time when the hold finishes, then commits only on arrival at the intended destination. Requests/cancellation/death do not unlock.
- Raid stats are transient; account totals, typed gameplay facts and unlocks persist using the existing safe-save envelope. Version 1 saves migrate.
- No achievement is inferred from quest reward receipt. No unconfirmed identifier is invented.
- No platform setup is performed; guide only. Existing online achievement publisher remains behind empty platform mappings.
- Verify reserved/disabled conditions through local evaluation, save restore and remote merge; non-finite numbers and empty facility lists must fail closed.

## Tasks

- [x] 1. Add and run regression for reserved ID and empty production catalog. Extend model with disabled/reserved definitions, typed facts, item/facility snapshots and extraction predicates. Test 10% and 295/305 boundaries, same-raid count/reset and duplicate unlocks.
- [x] 2. Extend JSON loading and public query/report APIs, safe-save v2 migration and inventory/world-state subscriptions. Register exact 19 names in the current string table. Test runtime loading in three languages and safe-save round trip.
- [x] 3. Connect actual player kills, successful weapon shots, player-world readiness, death and extraction completion. Add vehicle collision damage classification without introducing collision damage rules. Leave unavailable gameplay producers explicitly pending.
- [x] 4. Build TunaSweeperEditor Win64 Development, run achievement automation, review changes, document per-ID status, platform setup and remaining manual work. Record actual verification limits. Open project editor after build. Update save_persistence and requests; commit only task files.

## Initial code findings

Existing AchievementDefinitions.json is empty. EnemyCharacter::HandleDeath already checks player attribution and guards duplicate deaths. LocationTrigger reports actual overlaps. Quest subsystem reports reward claims (retained for compatibility but unused by the new catalog). Inventory and world progress expose multicast delegates. ExtractionPointActor finishes its hold before ExtractPawn; generic level travel persistence executes on request and cannot establish achievement success. LoopRail currently implements physical trains/stations, not a confirmed fast-travel completion producer. No confirmed RPG item/launcher or ATV collision-damage producer was found. BabyBoar and lift sequence assets alone do not establish joined/used events. The existing ending actor is demo-specific and is not a source for the full ending achievement.

## Execution notes

Work starts 2026-10-11 20:33:20. Existing unrelated changes: Docs/questions.md and Content/MainRaid/RaidMap.umap; preserve and exclude from task commit. Work in current checkout to retain project-local build/runtime dependencies; no asset generation is needed.

Final verification: UE 5.7 Editor Development build exit 0; achievement automation 10/10 and independent process restart 1/1 passed. Review fixes cover v1 replacement validation and stale extraction evidence after travel failure. PIE, packaging and platform server tests were not run. Full report: Docs/achievements_system.md.
