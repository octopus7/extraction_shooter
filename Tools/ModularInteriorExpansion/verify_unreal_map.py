"""Read-only fresh-process map/physics audit. No asset creation or saving."""
from pathlib import Path
import unreal,json
ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'TunaSweeper/SourceArt/Environment/ModularInteriorExpansion'
DEST='/Game/Environment/ModularInteriorExpansion';BASE='/Game/Environment/ModularInteriorPreview'
MAP=DEST+'/Maps/L_ModularInteriorExpansion'
m=json.loads((OUT/'model_manifest.json').read_text())
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem);levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
assert levels.load_level(MAP)
allactors=actors.get_all_level_actors();bylabel={a.get_actor_label():a for a in allactors}
assert not any(any(s in a.get_actor_label() for s in ('Ceiling','Beam','Pillar')) for a in allactors)
kit=[a for a in allactors if isinstance(a,unreal.StaticMeshActor)]
assert len(kit)==len(m['placements'])
for p in m['placements']:
    a=bylabel[p['name']];c=a.static_mesh_component
    e=next(e for e in m['base_assets']+m['assets'] if e['key']==p['key'])
    folder=BASE if e in m['base_assets'] else DEST
    assert c.static_mesh.get_path_name().split('.')[0]==folder+'/Meshes/'+e['name']
    loc=a.get_actor_location();scale=a.get_actor_scale3d();rot=a.get_actor_rotation()
    assert max(abs(v-w*100) for v,w in zip([loc.x,loc.y,loc.z],p['location_m']))<.1
    assert max(abs(v-w) for v,w in zip([scale.x,scale.y,scale.z],p['scale']))<1e-4
    assert abs((rot.yaw-p['yaw_deg']+180)%360-180)<.01
    data=c.get_editor_property('custom_primitive_data').get_editor_property('data');assert len(data)==4
    assert max(abs(a-b) for a,b in zip(data,[.65,1,*p['dirt_offset']]))<1e-4
bp=unreal.load_asset('/Game/Characters/Player/BP_TunaSweeperPlayerCharacter');cdo=unreal.get_default_object(bp.generated_class())
cam=cdo.get_editor_property('top_down_camera_mode_settings')
assert abs(cam.get_editor_property('target_arm_length')-1500)<.1
assert abs(cam.get_editor_property('boom_rotation').pitch+88)<.1
assert abs(cam.get_editor_property('default_fov')-70)<.1
for name in ('UE_BasePlayCamera','UE_ExpansionPlayCamera'):
    a=bylabel[name];assert abs(a.get_actor_rotation().pitch+88)<.01 and abs(a.get_actor_rotation().yaw)<.01
    assert abs(a.camera_component.field_of_view-70)<.01
report={'passed':False,'map':MAP,'kit_instances':len(kit),'kit_triangles':m['sample_triangles'],'placements_and_cpd_verified':True,'roofless':True,'camera_verified_against_player_bp':True,'fixture_lights':len(m['fixtures']),'preview_softboxes':2,'simple_collision_traces':[]}
def physics():
    world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    tests=[('base_wall',(250,-50,150),(250,50,150),True),('base_door',(300,580,120),(300,640,120),False),('floor',(1500,300,50),(1500,300,-50),True),('grating_solid',(1500,300,20),(1500,300,-20),True),('new_door',(1400,-50,120),(1400,40,120),False),('frame_jamb',(1295,-50,120),(1295,40,120),True),('window_pane',(1500,580,180),(1500,640,180),True),('window_sill',(1500,580,50),(1500,640,50),True),('low_partition',(1250,170,50),(1250,240,50),True),('above_partition',(1250,170,180),(1250,240,180),False)]
    for name,start,end,expected in tests:
        result=unreal.SystemLibrary.line_trace_single(world,unreal.Vector(*start),unreal.Vector(*end),unreal.TraceTypeQuery.TRACE_TYPE_QUERY1,False,[],unreal.DrawDebugTrace.NONE,True)
        hit=False if result is None else True if isinstance(result,unreal.HitResult) else next(v for v in result if isinstance(v,bool))
        assert hit==expected,(name,hit,expected)
        report['simple_collision_traces'].append({'name':name,'hit':hit,'expected':expected})
_ticks=0;_busy=False
def finish():
    unreal.unregister_slate_post_tick_callback(_handle)
    (OUT/('unreal_map_validation.json' if globals().get('CAPTURE_ONCE') else 'unreal_map_reload_validation.json')).write_text(json.dumps(report,indent=2))
    unreal.SystemLibrary.quit_editor()
def tick(dt):
    global _ticks,_busy
    if _busy:return
    _ticks+=1
    if _ticks<30:return
    _busy=True
    try:
        physics();report['passed']=True;report['editor_ticks_before_physics_query']=_ticks
        if globals().get('CAPTURE_ONCE'):
            unreal.unregister_slate_post_tick_callback(_handle)
            script=Path(__file__).with_name('capture_once.py')
            exec(compile(script.read_text(),str(script),'exec'),globals())
        else:finish()
    except Exception:
        import traceback
        report['error']=traceback.format_exc();unreal.log_error(report['error']);finish()
_handle=unreal.register_slate_post_tick_callback(tick)
