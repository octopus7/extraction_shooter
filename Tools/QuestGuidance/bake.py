"""Rebake authored quest guidance coordinates using UE 5.7 -run=pythonscript -script=<this file>.

This only reads map packages. Raid anchors and loot contents are resolved against the
active gameplay dataset at runtime so the bake contains no quest definitions.
"""
from pathlib import Path
import unreal

ROOT = Path(__file__).resolve().parents[2]
MAPS = (
    "Maps/BunkerMap", "Maps/DemoRaidMap", "Maps/DemoBoxRaidMap",
    "MainRaid/RaidMap", "MainRaid/RaidForest", "MainRaid/RaidVillage",
    "MainRaid/RaidPlains", "Environment/Basement/Maps/L_Basement",
)
DESTINATIONS = dict(BUNKER="BunkerMap", RAID="RaidMap", BASEMENT="L_Basement",
                    FOREST="RaidForest", VILLAGE="RaidVillage", PLAINS="RaidPlains")


def prop(obj, name):
    return obj.get_editor_property(name)


def bake_actor(actor, rows, level):
    if prop(actor, "is_editor_only_actor"):
        return
    pos = actor.get_actor_location()
    def put(kind, key="", detail=""):
        row = [level, kind, str(key), str(detail), *(f"{v:.2f}" for v in (pos.x, pos.y, pos.z))]
        rows.add("\t".join(row))

    def is_a(short):
        return isinstance(actor, getattr(unreal, "TunaSweeper" + short))

    if is_a("RaidPlacementAnchor"):
        if prop(actor, "anchor_kind").name in ("ENEMY", "LOOT_CONTAINER"):
            put("anchor", prop(actor, "placement_id"), prop(actor, "anchor_kind").name)
    if is_a("EnemyCharacter"):
        put("enemy", prop(actor, "enemy_id"), actor.get_class().get_path_name())
        if prop(actor, "drop_container_definition_id") > 0 and prop(actor, "drop_contents_id") > 0:
            put("contents", prop(actor, "drop_contents_id"))
    if is_a("LootContainerActor"):
        put("contents", prop(actor, "contents_id"))
    if is_a("PickupItemActor"):
        put("item", prop(actor, "item_id"))
    if is_a("FoodWarehouseActor") and prop(actor, "food_quantity") > 0:
        if prop(actor, "require_quest"):
            if prop(actor, "required_quest_id") > 0:
                put("quest_item", prop(actor, "food_item_id"), prop(actor, "required_quest_id"))
        else:
            put("item", prop(actor, "food_item_id"))
    if is_a("CrowbarWallRackActor"):
        put("item", prop(actor, "crowbar_item_id"))
    if is_a("DeliveryDestinationActor") or is_a("ParcelContainerActor"):
        kind = "delivery" if is_a("DeliveryDestinationActor") else "parcel"
        put(kind, prop(actor, "quest_id"), prop(actor, "objective_id"))
    if is_a("QuestArrivalTrigger"):
        kind = "arrival" if prop(actor, "vehicle_condition").name == "NONE" else "vehicle_arrival"
        put(kind, prop(actor, "location_id"))
    if is_a("LevelTravelInteractableActor"):
        put("travel", DESTINATIONS[prop(actor, "destination").name])
    if is_a("ExtractionPointActor"):
        put("extract", prop(actor, "target_level_name"))
    if is_a("WarpPointActor"):
        put("warp", prop(actor, "warp_point_id"), prop(actor, "target_warp_point_id"))
    if is_a("WorkbenchActor"):
        put("workbench", prop(actor, "workbench_id"))
    if isinstance(actor, unreal.LoopRailStation):
        put("progress", "rail.lubrication." + str(prop(actor, "station_id")))
    if is_a("RegionLinkActor"):
        put("progress", prop(actor, "required_progress_id"))
    if any(is_a(t) for t in ("WorldProgressActor", "PowerFacilityActor", "RockfallBarrierActor", "BlockedIntakeScreenActor")):
        put("progress", prop(actor, "progress_object_id"))
    events = []
    if is_a("PowerFacilityActor"):
        events.append(("repair_objective_event_id", "world_progress"))
    if is_a("QuestFacilityActor"):
        events.append(("interaction_objective_event_id", "quest_facility_use"))
    if is_a("BlockedIntakeScreenActor"):
        put("progress", prop(actor, "valve_progress_object_id"))
        events.extend((field, "world_progress") for field in (
            "inspect_objective_event_id", "clear_debris_objective_event_id", "repair_valve_objective_event_id"))
    if is_a("WorldProgressActor"):
        events.append(("objective_event_id", "world_progress"))
    if is_a("ATVActor"):
        events.append(("refuel_objective_event_id", "world_progress"))
    if is_a("SpecialVehicleSpawner") and prop(actor, "vehicle_class"):
        vehicle = unreal.get_default_object(prop(actor, "vehicle_class"))
        event = str(prop(vehicle, "refuel_objective_event_id"))
        if event != "None":
            put("event", event, "world_progress")
    for field, kind in events:
        event = str(prop(actor, field))
        if event != "None":
            put("event", event, kind)
    for comp in actor.get_components_by_class(unreal.TunaSweeperInteractableComponent):
        kind = prop(comp, "interaction_type").name.lower()
        if kind != "none":
            put("event", prop(comp, "objective_event_id"), kind)
    for cls in ("FacilityNpcActor", "QuestVendingActor", "MoleCompanionActor"):
        if is_a(cls):
            put("provider", prop(actor, "quest_provider_id"))


def main():
    rows = set()
    editor = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    for path in MAPS:
        if not editor.load_level("/Game/" + path):
            raise RuntimeError("Could not load " + path)
        level = path.rsplit("/", 1)[-1]
        for actor in actors.get_all_level_actors():
            bake_actor(actor, rows, level)
    out = ROOT / "TunaSweeper/Content/Data/QuestGuidanceLocations.tsv"
    out.write_text("Map\tKind\tKey\tDetail\tX\tY\tZ\n" + "\n".join(sorted(rows)) + "\n", encoding="utf-8")
    unreal.log(f"Baked {len(rows)} quest guidance records")


main()
