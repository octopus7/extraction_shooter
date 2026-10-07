# Armor and ammunition penetration

Armor and ammunition use gameplay tiers 1–4, independent of item rarity. `ItemTable.json` owns `armor_tier`, `defense_value`, and `penetration_tier`; the item CSV export and save-tool catalog expose them. Missing tier fields default to 0 for existing custom definitions. Authored tiers must be integers from 0 through 4.

## Equipment

| Tier | Body item | Body defense | Head item | Head defense |
| --- | --- | ---: | --- | ---: |
| 1 | 5010: tactical jacket | 2 | 5021: tier 1 ballistic helmet | 1 |
| 2 | 5001: body armor | 4 | 5006: ballistic helmet | 2 |
| 3 | 5022: tier 3 body armor | 6 | 5023: tier 3 ballistic helmet | 3 |
| 4 | 5024: tier 4 body armor | 8 | 5025: tier 4 ballistic helmet | 4 |

Existing item IDs remain valid. The old jacket and vest now use defense 2 and 4 respectively. Head and body defenses still protect the whole character and add together; this change does not introduce anatomical armor coverage, armor durability, or random penetration rolls.

## Ammunition

| Penetration tier | Ammunition | Pistol | Rifle | Shotgun |
| --- | --- | ---: | ---: | ---: |
| 1 | Rusty | 2011 | 2021 | 2031 |
| 2 | Standard | 2001 | 2002 | 2003 |
| 3 | AP | 2012 | 2022 | 2032 |
| 4 | Advanced AP | 2013 | 2024 | 2033 |

These are the complete ammunition catalog: four variants for each of three calibers, twelve items total. Utility ammunition such as incendiary rounds is not available. Retired item ID 2023 is reserved for migration to standard rifle ammunition 2002. Advanced AP uses the same base projectile damage as its AP counterpart and doubles its sell price; the upgrade is penetration. New items reuse the matching existing icons and appear automatically in the development armory.

## Damage rule

For each equipped item:

`effective defense = max(0, defense) × clamp(0.5 + 0.25 × (armor tier − penetration tier), 0, 1)`

| Armor tier compared with ammunition | Defense retained |
| --- | ---: |
| At least 2 tiers higher | 100% |
| 1 tier higher | 75% |
| Same tier | 50% |
| 1 tier lower | 25% |
| At least 2 tiers lower | 0% |

Resolve every equipped item's own tier before adding its effective defense. Ammo multiplication/bonus, headshots, enemy difficulty scaling and effective defense retain fractional precision. Only after all modifiers and defense subtraction, apply `round(max(0, damage))` once; exact positive halves round up. Do not round individual armor items or the projectile snapshot. The resulting whole-point damage drives health subtraction and damage-number feedback, capped to remaining health for mortal targets. Tier 0 ammunition and non-point damage retain full defense; tier 0 armor retains its legacy flat defense against every round. Small hits can be fully absorbed, and positive final damage below 0.5 rounds to zero.

Example: tier 4 body and head equipment have total base defense 12. An incoming 20-damage projectile deals 8 with rusty/standard ammunition, 11 with AP, or 14 with advanced AP. This example fixes raw damage at 20 to show penetration independently of ammunition damage bonuses.

`TunaSweeperArmor` is the common calculation used by player damage, enemy damage, and armor information text. The ammunition panel shows penetration tier alongside final rounded per-projectile damage for an unarmored, normal hit without difficulty scaling. The armor panel shows its exact pre-rounding damage reduction against all four ammunition types. Runtime penetration and unrounded ammo damage are copied into each projectile at firing time, including every shotgun pellet; an in-flight projectile never reads the owner's subsequently selected ammo.

## Enemy authoring

`ATunaSweeperEnemyCharacter` exposes `BodyArmorItemId` and `HeadArmorItemId`. `EnemySpawnProfiles.json` optionally accepts `body_armor_item_id` and `head_armor_item_id`. A positive ID must reference a defensive item for that equipment slot. Zero explicitly removes the slot; omission preserves the actor/BP default. Existing profiles remain unarmored unless armor is assigned. No placement IDs or spawn locations change.

Armor references provide combat stats, not an inventory instance or an automatic loot drop. Enemy ammunition penetration comes from the ammo definition resolved for its actual weapon loadout.

## Verification

### Practice dummy authoring

`ATunaSweeperShootingPracticeDummyActor` (including `BP_RangePracticeTarget`) exposes `BodyArmorTier` and `HeadArmorTier` under **Practice Dummy > Armor** in BP defaults and placed instances. Set each independently from 0 (empty slot) through 4. `ConfigurePracticeDummyArmor` also supports runtime changes; out-of-range values are clamped during configuration and resolution.

The item subsystem selects the lowest item ID matching that slot and tier with positive defense. `GetBodyArmorItemId` and `GetHeadArmorItemId` expose the actual equipped definitions during play; no catalog or no matching item returns `INDEX_NONE`. Damage reads the definitions' defense values through the same `TunaSweeperArmor::ItemDefense` path as enemies. Mixed tiers resolve independently and add together, retaining whole-character protection. Headshot damage is doubled before armor is subtracted; generic damage receives full defense with no penetration.

`GetEffectiveDefense(PenetrationTier)` exposes the same fractional defense used by `TakeDamage`. Current, maximum and minimum dummy health are integer-valued. Existing immortal minimum health and two-second recovery remain; fractional frame recovery accumulates separately and restores whole points. Returned damage is the rounded post-armor hit damage for comparison, even if minimum health prevents the full health loss on overkill. These settings are authored in BP/map assets; session health and runtime armor changes do not enter player saves.

`TunaSweeper.Combat.PracticeDummy.ArmorEquipment` covers tier properties, all 20 naked/armor–ammo combinations, real equipped item IDs, mixed tiers, health deltas, headshots, generic damage, zero-damage absorption, missing catalogs and runtime clamping/removal. `HitDamage` protects the original hit-zone rules.

In `/Game/Environment/Basement/Maps/L_Basement`, the five existing `BP_RangePracticeTarget` instances `Target_01` through `Target_05` use paired body/head tiers **0, 1, 2, 3, 4** respectively. Their meshes, positions and `ROOT_Range` attachment are preserved. `RangeTarget/placement.json` records both tier values; `Tools/Basement/verify_target_armor.py` reloads the saved map and verifies all presets, equipped IDs, the penetration defense table, actual generic damage, fractional-hit rounding, minimum health and integer-valued recovery in PIE. `target_armor_validation.json` records the latest result.

`TunaSweeper.Combat.Armor.FourTiersAndActualDamage` checks the complete 4×4 balance table against real player and enemy damage calls, mixed armor tiers, authored item tiers, and legacy/non-point damage behavior. `TunaSweeper.Inventory.EquipmentData.LaserAndStartingLoadout` verifies the four armor tiers, all three advanced AP items, information-panel damage, and projectile/pellet snapshots. Existing projectile/burn snapshot tests protect the unchanged status-effect path.
