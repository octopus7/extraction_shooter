import unreal


MAP_PATH = "/Game/Maps/DemoBoxRaidMap"
OLD_PLACEMENT_ID = 2
NEW_PLACEMENT_ID = 1000


world = unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH)
if world is None:
    raise RuntimeError(f"Could not load {MAP_PATH}")

actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
target = None
for actor in actor_subsystem.get_all_level_actors():
    try:
        placement_id = int(actor.get_editor_property("placement_id"))
        anchor_kind = str(actor.get_editor_property("anchor_kind"))
    except Exception:
        continue

    if placement_id == NEW_PLACEMENT_ID:
        raise RuntimeError(
            f"PlacementId {NEW_PLACEMENT_ID} is already used by {actor.get_actor_label()}"
        )
    if placement_id == OLD_PLACEMENT_ID and "LOOT_CONTAINER" in anchor_kind.upper():
        target = actor

if target is None:
    raise RuntimeError(
        f"Could not find loot-container anchor PlacementId {OLD_PLACEMENT_ID}"
    )

target.set_editor_property("placement_id", NEW_PLACEMENT_ID)
target.set_actor_label("TS_LootContainer_North_1000")

if not unreal.EditorLoadingAndSavingUtils.save_map(world, MAP_PATH):
    raise RuntimeError(f"Could not save {MAP_PATH}")

unreal.log(
    f"RAID_ANCHOR_MIGRATED;Map={MAP_PATH};"
    f"OldId={OLD_PLACEMENT_ID};NewId={NEW_PLACEMENT_ID}"
)
