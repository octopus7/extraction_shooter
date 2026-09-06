"""One-off authoring entry point. Removed in the commit immediately after saved assets."""
from pathlib import Path
import unreal,json,math
ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'TunaSweeper/SourceArt/Environment/ModularInteriorExpansion'
DEST='/Game/Environment/ModularInteriorExpansion'
BASE='/Game/Environment/ModularInteriorPreview'
MAP=DEST+'/Maps/L_ModularInteriorExpansion'
m=json.loads((OUT/'model_manifest.json').read_text())
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem);levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
assert actors and levels
if unreal.EditorAssetLibrary.does_asset_exist(MAP):
    assert levels.load_level(MAP)
    old=[a for a in actors.get_all_level_actors() if isinstance(a,(unreal.StaticMeshActor,unreal.RectLight,unreal.CameraActor))]
    if old:assert actors.destroy_actors(old)
else:assert levels.new_level(MAP)
def spawn(cls,label,loc,rotation=None):
    a=actors.spawn_actor_from_class(cls,unreal.Vector(*loc),rotation or unreal.Rotator());assert a
    a.set_actor_label(label);return a
allentries=m['base_assets']+m['assets']
for p in m['placements']:
    e=next(e for e in allentries if e['key']==p['key'])
    folder=BASE if e in m['base_assets'] else DEST
    mesh=unreal.load_asset(folder+'/Meshes/'+e['name']);assert mesh
    a=spawn(unreal.StaticMeshActor,p['name'],[v*100 for v in p['location_m']],unreal.Rotator(pitch=0,yaw=p['yaw_deg'],roll=0))
    c=a.static_mesh_component;c.set_static_mesh(mesh);a.set_actor_scale3d(unreal.Vector(*p['scale']))
    c.set_mobility(unreal.ComponentMobility.STATIC)
    c.set_collision_profile_name('BlockAll' if e['collision_boxes'] else 'NoCollision')
    for i,v in enumerate([.65,1,*p['dirt_offset']]):c.set_default_custom_primitive_data_float(i,v)
    a.set_folder_path('Expansion/Modules' if folder==DEST else 'Base/Modules')
for i,f in enumerate(m['fixtures']):
    loc=unreal.Vector(*[v*100 for v in f['location_m']]);target=unreal.Vector(*[v*100 for v in f['target_m']])
    a=spawn(unreal.RectLight,'Fixture_'+str(i),[loc.x,loc.y,loc.z],unreal.MathLibrary.find_look_at_rotation(loc,target))
    c=a.get_component_by_class(unreal.RectLightComponent);c.set_mobility(unreal.ComponentMobility.MOVABLE);c.set_intensity(450);c.set_attenuation_radius(550);c.set_source_width(110);c.set_source_height(8)
# Preview-only broad overhead illumination, no ceiling geometry or ceiling fixture.
for i,loc in enumerate([[300,330,900],[1500,300,900]]):
    a=spawn(unreal.RectLight,'Preview_Softbox_'+str(i),loc,unreal.Rotator(pitch=-90,yaw=0,roll=0))
    c=a.get_component_by_class(unreal.RectLightComponent);c.set_mobility(unreal.ComponentMobility.MOVABLE);c.set_intensity(1800);c.set_attenuation_radius(1800);c.set_source_width(600);c.set_source_height(600)
for name,target in [('UE_BasePlayCamera',[3,3.3,.88]),('UE_ExpansionPlayCamera',[15,3,.88])]:
    a=spawn(unreal.CameraActor,name,[target[0]*100-1500*math.cos(math.radians(88)),target[1]*100,target[2]*100+1500*math.sin(math.radians(88))],unreal.Rotator(pitch=-88,yaw=0,roll=0));a.camera_component.set_field_of_view(70)
for name,loc,target,fov in [('UE_WindowServices',[1200,100,390],[1570,600,185],47),('UE_DoorFrame',[1600,350,250],[1380,0,140],54),('UE_Annex',[700,-700,900],[1500,300,100],60)]:
    a=spawn(unreal.CameraActor,name,loc,unreal.MathLibrary.find_look_at_rotation(unreal.Vector(*loc),unreal.Vector(*target)));a.camera_component.set_field_of_view(fov)
for a in actors.get_all_level_actors():
    if isinstance(a,unreal.CameraActor):
        c=a.camera_component;c.set_aspect_ratio(1.4)
        pp=c.get_editor_property('post_process_settings');pp.set_editor_property('override_auto_exposure_bias',True);pp.set_editor_property('auto_exposure_bias',-1)
        c.set_editor_property('post_process_settings',pp);c.set_editor_property('post_process_blend_weight',1)
assert levels.save_current_level()
exec(compile(Path(__file__).with_name('verify_unreal_map.py').read_text(),str(Path(__file__).with_name('verify_unreal_map.py')),'exec'),{'__file__':str(Path(__file__).with_name('verify_unreal_map.py')),'CAPTURE_ONCE':True})
