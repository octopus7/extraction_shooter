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
    side_layout=json.loads((SOURCE/'side_furniture_placement.json').read_text())
    swapped=[]
    for p in side_layout['placements']:
        a=labels[p['label']];c=a.get_component_by_class(u.StaticMeshComponent)
        assert (a.get_actor_location()-u.Vector(*p['location_cm'])).length()<.1,p['label']
        assert abs((a.get_actor_rotation().yaw-p['yaw_deg']+180)%360-180)<.1,p['label']
        assert abs(abs((p['yaw_deg']-p['before']['yaw_deg']+180)%360-180)-180)<.1,p['label']
        assert (p['location_cm'][1]-175)*(p['before']['location_cm'][1]-175)<0,p['label']
        assert c.static_mesh.get_path_name()==p['mesh'] and str(c.get_collision_profile_name())==p['collision_profile']
        assert a.get_attach_parent_actor().get_actor_label()==p['parent']
        swapped.append(p['label'])
    barriers=[];table_clearances=[]
    for i,x in enumerate(layout['partition_centers_m'],1):
        a=labels[f'LanePartition_{i:03}'];ignore=[other for other in actors if other!=a]
        center,extent=a.get_actor_bounds(False)
        bench_center,bench_extent=labels[f'FiringBench_{i:03}'].get_actor_bounds(False)
        gap=(bench_center.x-bench_extent.x)-(center.x+extent.x)
        assert gap>=5,('Partition overlaps table depth',i,gap)
        assert abs(extent.x*2-layout['partition_length_m']*100)<.1,(i,extent.x)
        # The entire tabletop depth stays open between adjacent benches.
        for y in [-3.4,-3.07,-2.7]:
            for z in [.7,1.4]:
                result=u.SystemLibrary.line_trace_single(world,pos(x-.6,y,z),pos(x+.6,y,z),u.TraceTypeQuery.TRACE_TYPE_QUERY1,True,ignore,u.DrawDebugTrace.NONE,True)
                assert not hit(result),('Partition blocks table gap',i,y,z)
        table_clearances.append(round(gap,2))
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
    # Walk around the last booth, down the dedicated side aisle and up to each target.
    expansion=json.loads((SOURCE/'RangeExpansion/layout_manifest.json').read_text())
    aisle=expansion['aisle_center_author_x_m']
    clear('rear_to_side_aisle',(0,-5.2,.95),(aisle,-5.2,.95))
    clear('side_aisle_to_targets',(aisle,-5.2,.95),(aisle,2.1,.95))
    for i,x in enumerate(layout['lane_centers_m'],1):clear(f'walk_to_target_{i}',(aisle,2.1,.95),(x,2.1,.95))
    # A 140cm diameter sweep establishes usable aisle width, beyond pawn clearance.
    wide=u.SystemLibrary.capsule_trace_single_by_profile(world,pos(aisle,-4.5,.95),pos(aisle,4.3,.95),70,90,'Pawn',False,[],u.DrawDebugTrace.NONE,True)
    assert not hit(wide),('140cm side aisle obstructed',str(wide.to_tuple()) if wide else '')
    clear('side_aisle_to_armory',(aisle,.28,.95),(7.45,.28,.95))
    armory=labels['WeaponRack_001']
    assert armory.get_interaction_type()==u.TunaSweeperInteractionType.DEBUG_ARMORY_OPEN
    assert (armory.get_actor_location()-pos(7.45,.28,0)).length()<armory.get_interactable_component().get_editor_property('interaction_distance')
    for p in expansion['changed_actors']:
        assert (labels[p['label']].get_actor_location()-u.Vector(*p['location_cm'])).length()<.1,p['label']
    for x in [5.5,6.95,8.2]:
        for y in [-3,0,3]:
            ground=u.SystemLibrary.line_trace_single(world,pos(x,y,.2),pos(x,y,-.2),u.TraceTypeQuery.TRACE_TYPE_QUERY1,True,[],u.DrawDebugTrace.NONE,True)
            assert hit(ground) and abs(ground.to_tuple()[5].z)<.2,('Expansion floor gap',x,y)
    report={'passed':True,'lanes':len(layout['lane_centers_m']),'bench_target_alignment':True,'partition_collision_checks':barriers,'capsule_radius_cm':42,'capsule_half_height_cm':90,'clear_routes':routes,'clear_firing_lines':len(layout['lane_centers_m']),'side_aisle_clear_width_cm':140,'screen_on_separate_desk':True,'asset_triangles':{k:v['triangles'] for k,v in meta['assets'].items()}}
    report['render_only_ammo_boxes']=render_only
    report['partition_table_clearance_cm']=table_clearances
    report['partition_length_cm']=layout['partition_length_m']*100
    report['swapped_side_furniture']=swapped
    report['armory_approach_in_interaction_range']=True
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
