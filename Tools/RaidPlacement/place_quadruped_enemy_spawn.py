import json
import os

import unreal


MAP_PACKAGE_PATH = "/Game/Maps/DemoRaidMap"
ANCHOR_BLUEPRINT_PATH = "/Game/Raid/Placement/BP_RaidPlacementAnchor"
ENEMY_BLUEPRINT_PATH = "/Game/Blueprints/BP_QuadrupedGunEnemy"
ANCHOR_LABEL = "TS_EnemySpawn_QuadrupedGun_01"
PLACEMENT_ID = 1
PROFILE_ID = "enemy.quadruped_gun_test"
LOCATION = unreal.Vector(5200.0, 1040.0, 90.0)
ROTATION = unreal.Rotator(0.0, 180.0, 0.0)


def _read_placement_id(actor):
    try:
        return int(actor.get_editor_property("placement_id"))
    except Exception:
        return None


def place_anchor():
    world = unreal.EditorLoadingAndSavingUtils.load_map(MAP_PACKAGE_PATH)
    if not world:
        raise RuntimeError(f"Could not load {MAP_PACKAGE_PATH}")

    anchor_class = unreal.EditorAssetLibrary.load_blueprint_class(ANCHOR_BLUEPRINT_PATH)
    if not anchor_class:
        raise RuntimeError(f"Could not load {ANCHOR_BLUEPRINT_PATH}")

    enemy_class = unreal.EditorAssetLibrary.load_blueprint_class(ENEMY_BLUEPRINT_PATH)
    if not enemy_class:
        raise RuntimeError(f"Could not load {ENEMY_BLUEPRINT_PATH}")

    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    existing = None
    for actor in actor_subsystem.get_all_level_actors():
        if actor.get_class() == enemy_class:
            raise RuntimeError(
                f"{ENEMY_BLUEPRINT_PATH} is directly placed as {actor.get_actor_label()}"
            )
        if actor.get_actor_label() == ANCHOR_LABEL:
            existing = actor
            continue
        if _read_placement_id(actor) == PLACEMENT_ID:
            raise RuntimeError(
                f"PlacementId {PLACEMENT_ID} is already used by {actor.get_actor_label()}"
            )

    anchor = existing or actor_subsystem.spawn_actor_from_class(
        anchor_class,
        LOCATION,
        ROTATION,
        transient=False,
    )
    if not anchor:
        raise RuntimeError("Could not spawn the raid placement anchor")

    actual_placement_id = _read_placement_id(anchor)
    anchor_kind = str(anchor.get_editor_property("anchor_kind"))
    if actual_placement_id != PLACEMENT_ID or "ENEMY" not in anchor_kind.upper():
        raise RuntimeError(
            f"Anchor defaults do not match enemy PlacementId {PLACEMENT_ID}: "
            f"placement_id={actual_placement_id}, anchor_kind={anchor_kind}"
        )

    anchor.set_actor_label(ANCHOR_LABEL)
    anchor.set_actor_location(LOCATION, sweep=False, teleport=True)
    anchor.set_actor_rotation(ROTATION, teleport_physics=True)
    anchor.set_folder_path("RaidPlacement/Enemy")

    if not unreal.EditorLoadingAndSavingUtils.save_map(world, MAP_PACKAGE_PATH):
        raise RuntimeError(f"Could not save {MAP_PACKAGE_PATH}")

    data_directory = os.path.join(unreal.Paths.project_content_dir(), "Data")
    with open(os.path.join(data_directory, "EnemySpawnProfiles.json"), encoding="utf-8") as source:
        profiles = json.load(source)
    with open(os.path.join(data_directory, "EnemySpawns.json"), encoding="utf-8") as source:
        placements = json.load(source)

    profile = next((row for row in profiles if row.get("profile_id") == PROFILE_ID), None)
    placement = next(
        (
            row
            for row in placements
            if row.get("level_name") == "DemoRaidMap"
            and row.get("placement_id") == PLACEMENT_ID
        ),
        None,
    )
    if not profile or profile.get("enemy_class") != f"{ENEMY_BLUEPRINT_PATH}.{ENEMY_BLUEPRINT_PATH.rsplit('/', 1)[-1]}_C":
        raise RuntimeError(f"Enemy profile {PROFILE_ID} is not linked to {ENEMY_BLUEPRINT_PATH}")
    if not placement or placement.get("profile_id") != PROFILE_ID:
        raise RuntimeError(f"PlacementId {PLACEMENT_ID} is not linked to profile {PROFILE_ID}")
    if any(field in placement for field in ("location", "rotation", "scale")):
        raise RuntimeError("Anchor-owned enemy placement data must not contain transform fields")

    unreal.log(
        f"Placed {ANCHOR_LABEL} (PlacementId={PLACEMENT_ID}) in {MAP_PACKAGE_PATH}; "
        f"profile {PROFILE_ID} resolves {ENEMY_BLUEPRINT_PATH}, which is not directly placed."
    )


place_anchor()
