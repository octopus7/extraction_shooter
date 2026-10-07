"""Read-only saved firing-station geometry and registered-world collision checks.

Call validate(world) in a loaded, ticked editor world. Never saves assets.
"""
import json,time,traceback
from pathlib import Path
import unreal as u

ROOT=Path(__file__).resolve().parents[2]
SOURCE=ROOT/'TunaSweeper/SourceArt/Environment/Basement'
DEST='/Game/Environment/Basement/Range'

def pos(x,y,z):return u.Vector((15+y)*100,x*100,z*100)
def hit(result):return isinstance(result,u.HitResult) and bool(result.to_tuple()[0])

def validate(world):
    actors=u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors()
    labels={a.get_actor_label():a for a in actors}
    meta=json.loads((SOURCE/'RangeStations/mesh_manifest.json').read_text())
    layout=json.loads((SOURCE/'RangeStations/placement_manifest.json').read_text())
    sm=u.get_editor_subsystem(u.StaticMeshEditorSubsystem)
    for name,data in meta['assets'].items():
        mesh=u.load_asset(DEST+'/Meshes/'+name)
        assert mesh.get_num_triangles(0)==data['triangles'],(name,mesh.get_num_triangles(0),data['triangles'])
        assert sm.get_num_uv_channels(mesh,0)==2,name
        assert mesh.get_material(0).get_name()=='MI_Range_Props',name
        assert not sm.get_nanite_settings(mesh).enabled,name
        assert mesh.get_editor_property('body_setup').get_editor_property('collision_trace_flag')==u.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE
        assert Path(mesh.get_editor_property('asset_import_data').get_first_filename()).resolve()==(SOURCE/'RangeStations/Models'/f'{name}.glb').resolve()
    render_only=[]
    for p in layout['placements']:
        a=labels[p['label']]
        assert (a.get_actor_location()-pos(*p['author_location_m'])).length()<.1,p['label']
        assert a.static_mesh_component.static_mesh.get_name()==p['mesh']
        assert a.get_attach_parent_actor().get_actor_label()==p['parent']
        c=a.static_mesh_component
        profile=p.get('collision_profile','BlockAll')
        assert str(c.get_collision_profile_name())==profile
        if profile=='NoCollision':
            assert c.get_collision_enabled()==u.CollisionEnabled.NO_COLLISION
            assert c.get_editor_property('visible') and not c.get_editor_property('hidden_in_game')
            assert not c.get_editor_property('generate_overlap_events')
            center,extent=a.get_actor_bounds(False)
            delta=u.Vector(extent.x+30,0,0)
            trace=u.SystemLibrary.line_trace_single(world,center-delta,center+delta,u.TraceTypeQuery.TRACE_TYPE_QUERY1,True,[o for o in actors if o!=a],u.DrawDebugTrace.NONE,True)
            assert not hit(trace),p['label']+' still blocks shots'
            render_only.append(p['label'])
    assert sorted(render_only)==['AmmoBox_001','AmmoBox_002','AmmoBox_003']
    routes=[]
    def clear(name,start,end):
        result=u.SystemLibrary.capsule_trace_single_by_profile(world,pos(*start),pos(*end),42,90,'Pawn',False,[],u.DrawDebugTrace.NONE,True)
        assert not hit(result),(name,str(result.to_tuple()) if result else '')
        routes.append(name)
    for i,x in enumerate(layout['lane_centers_m'],1):
        bench=labels[f'FiringBench_{i:03}'];target=labels[f'Target_{i:02}']
        assert abs(bench.get_actor_location().y-target.get_actor_location().y)<.1
        clear(f'lane_{i}_rear_walkway',(0,-5.2,.95),(x,-5.2,.95))
        clear(f'lane_{i}_entry',(x,-5.2,.95),(x,-4.05,.95))
        shot=u.SystemLibrary.line_trace_single(world,pos(x,-4.05,1.25),pos(x,2.5,1.25),u.TraceTypeQuery.TRACE_TYPE_QUERY1,True,[],u.DrawDebugTrace.NONE,True)
        assert not hit(shot),f'lane_{i} firing line obstructed'
    barriers=[]
    for i,x in enumerate([-2.1,0,2.1],1):
        a=labels[f'LanePartition_{i:03}'];ignore=[other for other in actors if other!=a]
        for z in [.7,1.4]:
            result=u.SystemLibrary.line_trace_single(world,pos(x-.6,-3.9,z),pos(x+.6,-3.9,z),u.TraceTypeQuery.TRACE_TYPE_QUERY1,True,ignore,u.DrawDebugTrace.NONE,True)
            assert hit(result),(i,z)
        result=u.SystemLibrary.capsule_trace_single_by_profile(world,pos(x-.8,-3.9,.95),pos(x+.8,-3.9,.95),42,90,'Pawn',False,ignore,u.DrawDebugTrace.NONE,True)
        assert hit(result),i
        barriers.append(a.get_actor_label())
    # The screen's base must sit exactly on its separate desk, outside all lanes.
    desk=labels['RangeControlDesk_001'];screen=labels['ControlConsole_001']
    assert abs(screen.get_actor_location().z-93.5)<.1
    assert screen.get_actor_location().y < labels['FiringBench_001'].get_actor_location().y-100
    floor=u.SystemLibrary.line_trace_single(world,pos(-4.45,-3.9,1.0),pos(-4.45,-3.9,.8),u.TraceTypeQuery.TRACE_TYPE_QUERY1,True,[screen],u.DrawDebugTrace.NONE,True)
    assert hit(floor) and abs(floor.to_tuple()[5].z-93.5)<.2,'Desk does not support monitor'
    report={'passed':True,'lanes':4,'bench_target_alignment':True,'partition_collision_checks':barriers,'capsule_radius_cm':42,'capsule_half_height_cm':90,'clear_routes':routes,'clear_firing_lines':4,'screen_on_separate_desk':True,'asset_triangles':{k:v['triangles'] for k,v in meta['assets'].items()}}
    report['render_only_ammo_boxes']=render_only
    (SOURCE/'RangeStations/unreal_validation.json').write_text(json.dumps(report,indent=2))
    u.log('RANGE_STATIONS_VERIFIED')
    return report

if __name__=='__main__':
    world=u.EditorLoadingAndSavingUtils.load_map('/Game/Environment/Basement/Maps/L_Basement')
    u.AutomationLibrary.finish_loading_before_screenshot()
    started=time.monotonic();ticks=0;busy=False
    u.EditorPythonScripting.set_keep_python_script_alive(True)
    def tick(dt):
        global ticks,busy
        if busy:return
        ticks+=1
        if ticks<60 or time.monotonic()-started<3:return
        busy=True
        try:validate(world)
        except Exception:
            error=traceback.format_exc();u.log_error(error)
            (SOURCE/'RangeStations/unreal_validation.json').write_text(json.dumps({'passed':False,'error':error},indent=2))
        finally:
            u.unregister_slate_post_tick_callback(handle)
            u.EditorPythonScripting.set_keep_python_script_alive(False)
            u.SystemLibrary.quit_editor()
    handle=u.register_slate_post_tick_callback(tick)
