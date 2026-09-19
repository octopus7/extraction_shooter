import json
from pathlib import Path


PROJECT_ROOT = Path(__file__).resolve().parents[2]
DATA_ROOT = PROJECT_ROOT / "TunaSweeper" / "Content" / "Data"
CONTENT_ROOT = PROJECT_ROOT / "TunaSweeper" / "Content"

EXPECTED_ENEMIES = {
    2: ("enemy.quadruped_gun_blue", "Blue"),
    3: ("enemy.quadruped_gun_bright_gray", "BrightGray"),
    4: ("enemy.quadruped_gun_dark_gray", "DarkGray"),
    5: ("enemy.quadruped_gun_gold", "Gold"),
    6: ("enemy.quadruped_gun_red", "Red"),
}


def load_rows(file_name):
    with (DATA_ROOT / file_name).open(encoding="utf-8-sig") as source:
        return json.load(source)


def main():
    enemy_spawns = load_rows("EnemySpawns.json")
    enemy_profiles = load_rows("EnemySpawnProfiles.json")
    loot_spawns = load_rows("LootContainerSpawns.json")

    profiles_by_id = {row["profile_id"]: row for row in enemy_profiles}
    enemy_rows_by_id = {
        row["placement_id"]: row
        for row in enemy_spawns
        if row["level_name"] == "DemoRaidMap"
    }

    for placement_id, (profile_id, suffix) in EXPECTED_ENEMIES.items():
        placement = enemy_rows_by_id.get(placement_id)
        assert placement is not None, f"Missing enemy placement ID {placement_id}"
        assert placement["profile_id"] == profile_id
        assert placement["spawn_chance"] == 10000
        assert placement["condition_id"] == "always"

        profile = profiles_by_id.get(profile_id)
        assert profile is not None, f"Missing enemy profile {profile_id}"
        asset_name = f"BP_QuadrupedGunEnemy_{suffix}"
        expected_class = (
            "/Game/Blueprints/Enemies/QuadrupedVariants/"
            f"{asset_name}.{asset_name}_C"
        )
        assert profile["enemy_class"] == expected_class
        assert (CONTENT_ROOT / "Blueprints" / "Enemies" / "QuadrupedVariants" / f"{asset_name}.uasset").is_file()

    occupied = set()
    for kind, rows in (("enemy", enemy_spawns), ("loot", loot_spawns)):
        for row in rows:
            key = (row["level_name"], row["placement_id"])
            assert key not in occupied, f"Duplicate placement ID across kinds: {key}"
            occupied.add(key)
            if kind == "loot":
                assert row["placement_id"] >= 1000, (
                    f"Loot placement ID must start at 1000: {key}"
                )

    print("COLOR_ENEMY_PLACEMENT_VALIDATION_SUCCESS")


if __name__ == "__main__":
    main()
