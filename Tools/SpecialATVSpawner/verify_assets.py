"""Read-only saved BP and placement checks; run using UE 5.7 PythonScript commandlet."""
from pathlib import Path
import json, math
import unreal

ROOT=Path(__file__).resolve().parents[2]
MAP='/Game/MainRaid/RaidVillage'
BP='/Game/Blueprints/Spawners/BP_SpecialATVSpawner'
assert unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level(MAP)
bp=unreal.load_asset(BP)
cls=unreal.EditorAssetLibrary.load_blueprint_class(BP)
vehicle_cls=unreal.EditorAssetLibrary.load_blueprint_class('/Game/Blueprints/Vehicles/ATV/BP_ATV_TypeA')
assert bp and cls and vehicle_cls
cdo=unreal.get_default_object(cls)
assert isinstance(cdo,unreal.TunaSweeperSpecialVehicleSpawner)
assert cdo.get_editor_property('vehicle_class')==vehicle_cls
assert cdo.should_spawn_vehicle()
actors=list(unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors())
spawners=[a for a in actors if isinstance(a,unreal.TunaSweeperSpecialVehicleSpawner)]
assert len(spawners)==1
spawner=spawners[0]
assert spawner.get_class()==cls
assert spawner.get_editor_property('vehicle_class')==vehicle_cls
assert spawner.get_editor_property('spawned_vehicle') is None
assert spawner.spawn_vehicle() is None, 'Editor invocation must not spawn a vehicle'
assert not any(isinstance(a,unreal.TunaSweeperATVActor) for a in actors)
starts=[a for a in actors if a.get_actor_label()=='TS_Village_PlayerStart']
assert len(starts)==1
p=spawner.get_actor_location();s=starts[0].get_actor_location()
distance=math.hypot(p.x-s.x,p.y-s.y)
assert 300<=distance<=800, distance
icons=spawner.get_components_by_class(unreal.BillboardComponent)
assert len(icons)==1
icon=icons[0]
assert icon.get_editor_property('sprite').get_path_name()=='/Game/Editor/Icons/T_SpecialATVSpawner_Tire.T_SpecialATVSpawner_Tire'
assert icon.get_editor_property('is_editor_only') and icon.get_editor_property('hidden_in_game')
assert icon.get_editor_property('visible')
assert icon.get_collision_enabled()==unreal.CollisionEnabled.NO_COLLISION
assert not unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages()
# Some UE versions rebuild serialized nav data on map load; the verifier never saves it.
report={'passed':True,'blueprint':BP,'vehicle_blueprint':vehicle_cls.get_path_name(),
        'location_cm':[p.x,p.y,p.z],'distance_from_start_m':distance/100,
        'editor_only_tire_icon':True,'editor_spawned_vehicles':0}
out=ROOT/'TunaSweeper/Saved/SpecialATVSpawner/asset_verification.json'
out.parent.mkdir(parents=True,exist_ok=True)
out.write_text(json.dumps(report,indent=2),encoding='utf-8')
print('SPECIAL_ATV_ASSETS_PASS',json.dumps(report))
