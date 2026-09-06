"""Full UE editor only: create or fresh-process verify the isolated review map."""
from pathlib import Path
import unreal,json,os,math
ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'TunaSweeper/SourceArt/Environment/ModularInteriorPreview'
DEST='/Game/Environment/ModularInteriorPreview'
MAP=DEST+'/Maps/L_ModularInteriorPreview'
m=json.loads((OUT/'model_manifest.json').read_text())
verify=os.environ.get('MI_VERIFY_ONLY')=='1'
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
assert actors and levels,'Run this script in the full editor'
if verify:assert levels.load_level(MAP)
else:
    if unreal.EditorAssetLibrary.does_asset_exist(MAP):
        assert levels.load_level(MAP)
        old=[a for a in actors.get_all_level_actors() if isinstance(a,(unreal.StaticMeshActor,unreal.RectLight,unreal.CameraActor))]
        if old:assert actors.destroy_actors(old)
    else:assert levels.new_level(MAP)
    def spawn(cls,label,loc,rotation=None):
        a=actors.spawn_actor_from_class(cls,unreal.Vector(*loc),rotation or unreal.Rotator())
        assert a;a.set_actor_label(label);return a
    for p in m['placements']:
        e=next(e for e in m['assets'] if e['key']==p['key'])
        mesh=unreal.load_asset(f"{DEST}/Meshes/{e['name']}");assert mesh
        a=spawn(unreal.StaticMeshActor,p['name'],[v*100 for v in p['location_m']],unreal.Rotator(pitch=0,yaw=p['yaw_deg'],roll=0))
        c=a.static_mesh_component;c.set_static_mesh(mesh)
        a.set_actor_scale3d(unreal.Vector(*p['scale']))
        c.set_mobility(unreal.ComponentMobility.STATIC)
        c.set_collision_profile_name('NoCollision' if p['key']=='LightBar' else 'BlockAll')
        for index,value in enumerate([.65,1,*p['dirt_offset']]):c.set_default_custom_primitive_data_float(index,value)
        a.set_folder_path('MI/Modules')
    # Engine capsule-sized reference, excluded from the six kit meshes.
    a=spawn(unreal.StaticMeshActor,'PreviewOnly_176cm_ScaleReference',[300,330,88])
    a.static_mesh_component.set_static_mesh(unreal.load_asset('/Engine/BasicShapes/Cylinder'))
    a.set_actor_scale3d(unreal.Vector(.68,.68,1.76));a.static_mesh_component.set_collision_profile_name('NoCollision')
    a.set_folder_path('MI/Reference')
    for i,f in enumerate(m['fixtures']):
        loc=unreal.Vector(*[v*100 for v in f['location_m']]);target=unreal.Vector(*[v*100 for v in f['target_m']])
        a=spawn(unreal.RectLight,f'MI_FixtureLight_{i}',[loc.x,loc.y,loc.z],unreal.MathLibrary.find_look_at_rotation(loc,target))
        c=a.get_component_by_class(unreal.RectLightComponent);c.set_mobility(unreal.ComponentMobility.MOVABLE)
        c.set_intensity(450);c.set_attenuation_radius(550)
        c.set_source_width(110);c.set_source_height(8);a.set_folder_path('MI/Lights')
    pitch=math.radians(88)
    a=spawn(unreal.CameraActor,'MI_PlayCamera',[300-1500*math.cos(pitch),330,88+1500*math.sin(pitch)],unreal.Rotator(pitch=-88,yaw=0,roll=0))
    a.camera_component.set_field_of_view(70)
    a.camera_component.set_aspect_ratio(1.4)
    a=spawn(unreal.CameraActor,'MI_CorridorPlayCamera',[500-1500*math.cos(pitch),1020,88+1500*math.sin(pitch)],unreal.Rotator(pitch=-88,yaw=0,roll=0))
    a.camera_component.set_field_of_view(70);a.camera_component.set_aspect_ratio(1.4)
    a=spawn(unreal.CameraActor,'MI_Eye160cm',[100,90,160],unreal.MathLibrary.find_look_at_rotation(unreal.Vector(100,90,160),unreal.Vector(340,640,150)))
    a.camera_component.set_field_of_view(84);a.camera_component.set_aspect_ratio(1.4)
    a=spawn(unreal.CameraActor,'MI_Overview',[-1400,-1600,2000],unreal.MathLibrary.find_look_at_rotation(unreal.Vector(-1400,-1600,2000),unreal.Vector(400,700,40)))
    a.camera_component.set_projection_mode(unreal.CameraProjectionMode.ORTHOGRAPHIC)
    a.camera_component.set_ortho_width(2300);a.camera_component.set_aspect_ratio(1.4)
    # Preview-only exposure prevents emissive wall fixtures washing out the shared atlas.
    for camera_actor in actors.get_all_level_actors():
        if isinstance(camera_actor,unreal.CameraActor):
            camera=camera_actor.camera_component
            settings=camera.get_editor_property('post_process_settings')
            settings.set_editor_property('override_auto_exposure_bias',True)
            settings.set_editor_property('auto_exposure_bias',-1.5)
            camera.set_editor_property('post_process_settings',settings)
            camera.set_editor_property('post_process_blend_weight',1.0)
    assert levels.save_current_level()
    for retired in ('Ceiling','Beam'):
        path=f'{DEST}/Meshes/SM_MI_{retired}'
        if unreal.EditorAssetLibrary.does_asset_exist(path):assert unreal.EditorAssetLibrary.delete_asset(path)

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
report={'passed':True,'mode':'reload' if verify else 'create','map':MAP,'kit_instances':len(m['placements']),
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

# Loading a map queues Chaos registration. Let the editor tick before querying its broadphase.
# No PIE/game instance is started, and no gameplay save can be touched by this audit.
_ticks=0
_screens=[]
_capture=None
_busy=False

def _finish():
    unreal.unregister_slate_post_tick_callback(_tick_handle)
    report['editor_ticks_before_physics_query']=30
    (OUT/('unreal_map_reload_validation.json' if verify else 'unreal_map_validation.json')).write_text(json.dumps(report,indent=2))
    unreal.SystemLibrary.quit_editor()
def _after_map_tick(delta):
    global _ticks,_capture,_busy
    if _busy:return
    _ticks+=1
    if _ticks<30:return
    _busy=True
    try:
        if _ticks==30:
            finish_verification()
            if verify:_finish();return
            _screens.extend([('UE_PlayCamera','MI_PlayCamera'),('UE_CorridorPlayCamera','MI_CorridorPlayCamera')])
        if _capture and not _capture.is_task_done():
            assert _ticks<1200,'Screenshot timeout'
            return
        if _screens:
            name,camera=_screens.pop(0)
            _capture=unreal.AutomationLibrary.take_high_res_screenshot(1400,1000,str(OUT/'Previews'/f'{name}.png'),camera=by_label[camera],delay=5.0)
        else:
            assert all((OUT/'Previews'/f'{n}.png').is_file() for n in ('UE_PlayCamera','UE_CorridorPlayCamera'))
            _finish()
    except Exception:
        import traceback
        report['passed']=False;report['error']=traceback.format_exc();unreal.log_error(report['error'])
        _finish()
    finally:_busy=False
_tick_handle=unreal.register_slate_post_tick_callback(_after_map_tick)
