"""Read-only validation of station and mounted-vehicle arrival volumes.
Run with UE 5.7 -run=pythonscript -script=<this file>.
Does not modify assets, quest state, or player saves.
"""
from pathlib import Path
import json
import unreal

ROOT = Path(__file__).resolve().parents[2]
OUTPUT = ROOT / 'TunaSweeper/Saved/QuestStationArrival'
MAP = '/Game/MainRaid/RaidVillage'
BP = '/Game/Interaction/BP_QuestArrivalTrigger'

assert unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level(MAP)
cls = unreal.EditorAssetLibrary.load_blueprint_class(BP)
assert cls, 'Arrival Blueprint missing'
cdo = unreal.get_default_object(cls)
assert isinstance(cdo, unreal.TunaSweeperQuestArrivalTrigger)
assert cdo.get_editor_property('vehicle_condition') == unreal.TunaSweeperArrivalVehicleCondition.NONE
assert [v.name for v in unreal.TunaSweeperArrivalVehicleCondition] == ['NONE', 'ATV']
actors = list(unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors())
stations = [a for a in actors if isinstance(a, unreal.LoopRailStation)]
triggers = [a for a in actors if isinstance(a, unreal.TunaSweeperQuestArrivalTrigger)]
assert len(stations) == 3
assert len(triggers) == 4
north = max(stations, key=lambda a: a.get_actor_location().x)
others = [s for s in stations if s != north]
expected = {'north': north, 'east': max(others, key=lambda a: a.get_actor_location().y),
            'west': min(others, key=lambda a: a.get_actor_location().y)}
report = []
for direction, station in expected.items():
    location_id = 'village.station.' + direction
    matches = [t for t in triggers if str(t.get_editor_property('location_id')) == location_id]
    assert len(matches) == 1, location_id
    trigger = matches[0]
    assert trigger.get_class() == cls
    assert trigger.get_attach_parent_actor() == station
    assert trigger.get_editor_property('vehicle_condition') == unreal.TunaSweeperArrivalVehicleCondition.NONE
    assert not trigger.get_editor_property('required_completed_quest_ids')
    assert not trigger.get_editor_property('required_completed_progress_ids')
    area = trigger.get_editor_property('arrival_area')
    assert area.get_collision_enabled() == unreal.CollisionEnabled.NO_COLLISION
    extent = area.get_unscaled_box_extent()
    def inside(world):
        p = unreal.MathLibrary.inverse_transform_location(area.get_world_transform(), world)
        return all(abs(v) <= e + .01 for v, e in zip((p.x,p.y,p.z),(extent.x,extent.y,extent.z)))
    length = station.get_editor_property('platform_length')
    width = station.get_editor_property('platform_width')
    side = 1 if station.get_editor_property('platform_on_right') else -1
    # Player centers at both platform ends, platform edges and train seats must fit.
    for x in (450-length, 450-length*.5, 450):
        for y in (0, side*115, side*(115+width)):
            for z in (150, 350):
                assert inside(unreal.MathLibrary.transform_location(station.get_actor_transform(), unreal.Vector(x,y,z))), location_id
    for distant in (unreal.Vector(100000,100000,0), unreal.Vector(0,0,100000)):
        assert not inside(distant)
    report.append({'location_id': location_id, 'station_id': str(station.get_editor_property('station_id')),
                   'vehicle_condition': 'None', 'extent_cm': [extent.x,extent.y,extent.z]})
vehicle_matches = [t for t in triggers if str(t.get_editor_property('location_id')) == 'village.vehicle.north']
assert len(vehicle_matches) == 1
vehicle = vehicle_matches[0]
assert vehicle.get_class() == cls
assert vehicle.get_actor_label() == 'Village_ATVNorthArrival'
assert vehicle.get_editor_property('vehicle_condition') == unreal.TunaSweeperArrivalVehicleCondition.ATV
# Acceptance already enforces the fuel quest prerequisite. Do not depend on
# obsolete persistent refuel records: fuel now belongs to the current raid.
assert not vehicle.get_editor_property('required_completed_quest_ids')
assert not vehicle.get_editor_property('required_completed_progress_ids')
assert str(vehicle.get_editor_property('required_vehicle_tag')) == 'None'
vehicle_area = vehicle.get_editor_property('arrival_area')
assert vehicle_area.get_collision_enabled() == unreal.CollisionEnabled.NO_COLLISION
vehicle_extent = vehicle_area.get_unscaled_box_extent()
assert vehicle_extent.x >= 1000 and vehicle_extent.y >= 1500 and vehicle_extent.z >= 300
north_trigger = next(t for t in triggers if str(t.get_editor_property('location_id')) == 'village.station.north')
offset = vehicle.get_actor_location() - north_trigger.get_actor_location()
assert -2500 < offset.x < 0 and abs(offset.y) < 2500 and abs(offset.z) < 500
vehicle_report = {'label': vehicle.get_actor_label(), 'location_id': 'village.vehicle.north',
                  'vehicle_condition': 'ATV',
                  'position_cm': [vehicle.get_actor_location().x, vehicle.get_actor_location().y, vehicle.get_actor_location().z],
                  'extent_cm': [vehicle_extent.x, vehicle_extent.y, vehicle_extent.z]}
assert not unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages()
assert not unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages()
OUTPUT.mkdir(parents=True, exist_ok=True)
(OUTPUT/'assets_verified.json').write_text(json.dumps({'passed':True,'stations':report,'vehicle':vehicle_report},indent=2),encoding='utf-8')
print('STATION_ARRIVAL_ASSETS_PASS')
