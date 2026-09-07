"""Read-only full-editor audit: load the saved preview map and verify geometry/physics."""
from pathlib import Path
import unreal,json
ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'TunaSweeper/SourceArt/Environment/ModularInteriorPreview'
DEST='/Game/Environment/ModularInteriorPreview'
MAP=DEST+'/Maps/L_ModularInteriorPreview'
m=json.loads((OUT/'model_manifest.json').read_text())
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
assert actors and levels,'Run this script in the full editor'
assert levels.load_level(MAP)

all_actors=actors.get_all_level_actors()
assert not any(unreal.EditorAssetLibrary.does_asset_exist(f'{DEST}/Meshes/SM_MI_{key}') for key in ('Ceiling','Beam'))
assert not any('Ceiling' in a.get_actor_label() or 'Beam' in a.get_actor_label() for a in all_actors)
assert len([a for a in all_actors if isinstance(a,unreal.StaticMeshActor) and a.static_mesh_component.static_mesh and a.static_mesh_component.static_mesh.get_path_name().startswith(DEST+'/Meshes/')])==55
by_label={a.get_actor_label():a for a in all_actors}
assert len([a for a in all_actors if isinstance(a,unreal.RectLight)])==len(m['fixtures'])==5
for i,f in enumerate(m['fixtures']):
    light=by_label[f'MI_FixtureLight_{i}'];loc=light.get_actor_location()
    assert max(abs(v-w*100) for v,w in zip([loc.x,loc.y,loc.z],f['location_m']))<.1
for name in ('MI_PlayCamera','MI_CorridorPlayCamera'):
    camera=by_label[name];rotation=camera.get_actor_rotation()
    assert abs(rotation.pitch+88)<.01 and abs(rotation.yaw)<.01
    assert abs(camera.camera_component.field_of_view-70)<.01
for p in m['placements']:
    a=by_label[p['name']];c=a.static_mesh_component
    expected=f"{DEST}/Meshes/SM_MI_{p['key']}"
    assert c.static_mesh.get_path_name().split('.')[0]==expected
    loc=a.get_actor_location();scale=a.get_actor_scale3d();rot=a.get_actor_rotation()
    assert max(abs(v-w*100) for v,w in zip([loc.x,loc.y,loc.z],p['location_m']))<.1
    assert max(abs(v-w) for v,w in zip([scale.x,scale.y,scale.z],p['scale']))<1e-4
    assert abs((rot.yaw-p['yaw_deg']+180)%360-180)<.01,(p['name'],str(rot),p['yaw_deg'])
    cpd=c.get_editor_property('custom_primitive_data').get_editor_property('data')
    assert max(abs(a-b) for a,b in zip(cpd,[.65,1,*p['dirt_offset']]))<1e-4
    assert len(cpd)>=4
report={'passed':True,'mode':'reload','map':MAP,'kit_instances':len(m['placements']),
        'kit_triangles':m['sample_triangles'],'rect_lights':5,'reference_objects':1,
        'placement_transforms_and_dirt_cpd_verified':True,'roofless':True,'ceiling_present':False,'beam_present':False,'wall_mounted_lights':5}
def finish_verification():
    world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    report['simple_collision_traces']=[]
    for label,start,end,expected in [
        ('south_wall',(250,-50,150),(250,50,150),True),
        ('door_opening',(300,580,120),(300,640,120),False),
        ('door_jamb',(150,580,120),(150,640,120),True),
        ('floor',(300,300,50),(300,300,-50),True)]:
        result=unreal.SystemLibrary.line_trace_single(world,unreal.Vector(*start),unreal.Vector(*end),
            unreal.TraceTypeQuery.TRACE_TYPE_QUERY1,False,[],unreal.DrawDebugTrace.NONE,True)
        if result is None:hit=False
        elif isinstance(result,unreal.HitResult):hit=True
        else:hit=next(v for v in result if isinstance(v,bool))
        assert hit==expected,(label,hit,expected)
        report['simple_collision_traces'].append({'name':label,'blocking_hit':hit,'expected':expected})
    bp=unreal.load_asset('/Game/Characters/Player/BP_TunaSweeperPlayerCharacter')
    cdo=unreal.get_default_object(bp.generated_class())
    cam=cdo.get_editor_property('top_down_camera_mode_settings')
    report['project_camera']={
     'arm_cm':cam.get_editor_property('target_arm_length'),
     'pitch_deg':cam.get_editor_property('boom_rotation').pitch,
     'yaw_deg':cam.get_editor_property('boom_rotation').yaw,
     'fov_deg':cam.get_editor_property('default_fov')}
    assert abs(report['project_camera']['arm_cm']-1500)<.1
    assert abs(report['project_camera']['pitch_deg']+88)<.1
    assert abs(report['project_camera']['fov_deg']-70)<.1
    unreal.log('MI_MAP_VALIDATION_PASSED')

# Wait for Chaos registration without starting PIE or a gameplay instance.
_ticks=0
_busy=False
def _after_map_tick(delta):
    global _ticks,_busy
    if _busy:return
    _ticks+=1
    if _ticks<30:return
    _busy=True
    unreal.unregister_slate_post_tick_callback(_tick_handle)
    try:finish_verification()
    except Exception:
        import traceback
        report['passed']=False;report['error']=traceback.format_exc();unreal.log_error(report['error'])
    report['editor_ticks_before_physics_query']=_ticks
    (OUT/'unreal_map_reload_validation.json').write_text(json.dumps(report,indent=2))
    unreal.SystemLibrary.quit_editor()
_tick_handle=unreal.register_slate_post_tick_callback(_after_map_tick)
