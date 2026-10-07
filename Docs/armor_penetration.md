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

Resolve every equipped item's own tier before adding its effective defense. Subtract this sum from incoming damage and clamp the result to zero. Keep fractional defense and health damage; do not round each equipment item. For enemy attacks on the player, the existing difficulty scaling and whole-point rounding happen before armor. Tier 0 ammunition and non-point damage retain full defense; tier 0 armor retains its legacy flat defense against every round. Small hits can be fully absorbed.

Example: tier 4 body and head equipment have total base defense 12. An incoming 20-damage projectile deals 8 with rusty/standard ammunition, 11 with AP, or 14 with advanced AP. This example fixes raw damage at 20 to show penetration independently of ammunition damage bonuses.

`TunaSweeperArmor` is the common calculation used by player damage, enemy damage, and armor information text. The ammunition panel shows penetration tier alongside its shared pre-defense projectile damage. The armor panel shows its exact damage reduction against all four ammunition types. Runtime penetration is copied into each projectile at firing time, including every shotgun pellet; an in-flight projectile never reads the owner's subsequently selected ammo.

## Enemy authoring

`ATunaSweeperEnemyCharacter` exposes `BodyArmorItemId` and `HeadArmorItemId`. `EnemySpawnProfiles.json` optionally accepts `body_armor_item_id` and `head_armor_item_id`. A positive ID must reference a defensive item for that equipment slot. Zero explicitly removes the slot; omission preserves the actor/BP default. Existing profiles remain unarmored unless armor is assigned. No placement IDs or spawn locations change.

Armor references provide combat stats, not an inventory instance or an automatic loot drop. Enemy ammunition penetration comes from the ammo definition resolved for its actual weapon loadout.

## Verification

`TunaSweeper.Combat.Armor.FourTiersAndActualDamage` checks the complete 4×4 balance table against real player and enemy damage calls, mixed armor tiers, authored item tiers, and legacy/non-point damage behavior. `TunaSweeper.Inventory.EquipmentData.LaserAndStartingLoadout` verifies the four armor tiers, all three advanced AP items, information-panel damage, and projectile/pellet snapshots. Existing projectile/burn snapshot tests protect the unchanged status-effect path.
