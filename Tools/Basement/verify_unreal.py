"""Read-only verification of saved Basement assets and traversable connections.

Run in UE 5.7 with -ExecutePythonScript (a rendered editor is required for
physics registration). This verifier never imports, generates, or saves assets.
"""
import json,time,traceback,math,sys
from pathlib import Path
import unreal as u

ROOT=Path(__file__).resolve().parents[2]
SOURCE=ROOT/'TunaSweeper/SourceArt/Environment/Basement'
DEST='/Game/Environment/Basement'
MAP=DEST+'/Maps/L_Basement'

def blocking(result):
    if isinstance(result,u.HitResult):return bool(result.to_tuple()[0]),result
    if isinstance(result,tuple):
        detail=next((v for v in result if isinstance(v,u.HitResult)),None)
        return detail is not None and bool(detail.to_tuple()[0]),detail
    return False,None

def world_pos(author):
    x,y,z=author
    return u.Vector(y*100,x*100,z*100)

def validate(world=None):
    world=world or u.EditorLoadingAndSavingUtils.load_map(MAP)
    assert world
    actors=u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors()
    labels={a.get_actor_label():a for a in actors}
    assert len(labels)==len(actors),'Duplicate actor labels'
    layout=json.loads((SOURCE/'layout_manifest.json').read_text(encoding='utf8'))
    mesh_editor=u.get_editor_subsystem(u.StaticMeshEditorSubsystem)
    assets=[]
    material_report=[]
    source_refs=json.loads((SOURCE/'source_references.json').read_text(encoding='utf8'))
    assert source_refs['passed'] and source_refs['temporary_sources']==0
    for entry in source_refs['references']:
        obj=u.load_asset(entry['asset'])
        actual=Path(obj.get_editor_property('asset_import_data').get_first_filename()).resolve()
        assert actual==(ROOT/entry['source']).resolve() and actual.is_file(),entry['asset']
    assert not u.EditorAssetLibrary.does_asset_exist(DEST+'/Range/Calibration/SM_AxisCalibration')
    for atlas in ('Architecture','Props','Robot'):
        mi=u.load_asset(DEST+'/Range/Materials/MI_Range_'+atlas)
        assert isinstance(mi,u.MaterialInstanceConstant)
        channels={}
        for channel in ('BaseColor','Normal','ORM','Emissive'):
            tex=u.MaterialEditingLibrary.get_material_instance_texture_parameter_value(mi,channel)
            assert tex and tex.get_name()==f'T_{atlas}_{channel}',(atlas,channel,tex)
            channels[channel]=tex.get_name()
        material_report.append({'atlas':atlas,'textures':channels})
    for name,meta in layout['assets'].items():
        mesh=u.load_asset(DEST+'/Meshes/'+name)
        assert isinstance(mesh,u.StaticMesh),name
        assert mesh_editor.get_num_uv_channels(mesh,0)>=2,name
        assert mesh.get_editor_property('body_setup').get_editor_property('collision_trace_flag')==u.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE,name
        assert mesh.get_num_triangles(0)==meta['triangles'],name
        assert mesh.get_material(0) is not None,name
        assets.append({'name':name,'triangles':mesh.get_num_triangles(0),'uv_channels':mesh_editor.get_num_uv_channels(mesh,0)})
    hidden=[]
    for inst in layout['instances']:
        a=labels[inst['id']];c=a.static_mesh_component
        assert c.get_editor_property('static_mesh').get_name()==inst['asset'],inst['id']
        assert (a.get_actor_location()-world_pos(inst['location_m'])).length()<.1,inst['id']
        assert a.get_attach_parent_actor().get_actor_label()=='ROOT_'+inst['zone'],inst['id']
        assert str(c.get_collision_profile_name())=='BlockAll',inst['id']
        if inst['collision_only']:
            assert not c.get_editor_property('visible') and c.get_editor_property('hidden_in_game'),inst['id']
            assert not c.get_editor_property('cast_shadow'),inst['id']
    for a in actors:
        if 'BasementCollisionOnly' in [str(t) for t in a.tags]:
            c=a.static_mesh_component
            assert not c.get_editor_property('visible') and c.get_editor_property('hidden_in_game'),a.get_actor_label()
            assert c.get_collision_enabled()==u.CollisionEnabled.QUERY_AND_PHYSICS,a.get_actor_label()
            hidden.append(a.get_actor_label())
    assert len(hidden)>20
    ranges=[a for a in actors if 'TS_RangeTarget' in [str(t) for t in a.tags]]
    assert len(ranges)==4
    target_bp=u.load_asset(DEST+'/Range/Blueprints/BP_RangePracticeTarget')
    assert all(a.get_class()==target_bp.generated_class() for a in ranges)
    assert len({a.get_editor_property('body_mesh').static_mesh for a in ranges})==1
    connectors=[a for a in actors if 'BasementExpansionConnector' in [str(t) for t in a.tags]]
    assert len(connectors)==3
    for side in ('East','West','South'):
        parent='ROOT_'+side+'_Expansion'
        for child in ('ROOT_'+side+'_Expansion_RemovableCap','ROOT_Connector_'+side):
            assert labels[child].get_attach_parent_actor()==labels[parent],child
    for i,parent in enumerate(('ROOT_LadderHub','ROOT_North_RangeLink','ROOT_West_Expansion','ROOT_East_Expansion','ROOT_South_Expansion')):
        assert labels['Basement_Fill_'+str(i)].get_attach_parent_actor()==labels[parent]
    assert 'Hub_Ladder300' in labels and 'Hub_HatchLanding200' in labels
    assert len([a for a in actors if isinstance(a,u.PlayerStart)])==1
    sys.path.insert(0,str(ROOT/'Tools/Basement'))
    from verify_stations import validate as validate_stations
    station_report=validate_stations(world)
    sweeps=[]
    # Capsule dimensions are deliberately larger than the 2.4 m entry needs.
    for name,start,end in [('hub_to_range',(0,0,.95),(0,10,.95)),('east',(0,0,.95),(9,0,.95)),('west',(0,0,.95),(-9,0,.95)),('south',(0,0,.95),(0,-7.4,.95))]:
        result=u.SystemLibrary.capsule_trace_single_by_profile(world,world_pos(start),world_pos(end),42,90,'Pawn',False,[],u.DrawDebugTrace.NONE,True)
        hit,detail=blocking(result)
        assert not hit,(name,detail.to_tuple() if detail else None)
        sweeps.append({'route':name,'capsule_radius_cm':42,'capsule_half_height_cm':90,'clear':True})
    # Test every collision-only modular wall across its centre; ignore all other
    # actors so a floor, neighbouring wall, or prop cannot make a false positive.
    wall_hits=[]
    for inst in layout['instances']:
        if not inst['collision_only'] or 'Corner' in inst['asset']:continue
        a=labels[inst['id']];center,extent=a.get_actor_bounds(False)
        narrow_x=extent.x<extent.y
        delta=u.Vector(extent.x+80,0,0) if narrow_x else u.Vector(0,extent.y+80,0)
        ignore=[other for other in actors if other!=a]
        hit,detail=blocking(u.SystemLibrary.capsule_trace_single_by_profile(world,center-delta,center+delta,42,90,'Pawn',False,ignore,u.DrawDebugTrace.NONE,True))
        assert hit,inst['id']+' has no physical barrier'
        wall_hits.append(inst['id'])
    for label in hidden:
        if not label.startswith('Wall_'):continue
        a=labels[label];center,extent=a.get_actor_bounds(False)
        delta=u.Vector(extent.x+80,0,0) if extent.x<extent.y else u.Vector(0,extent.y+80,0)
        hit,detail=blocking(u.SystemLibrary.capsule_trace_single_by_profile(world,center-delta,center+delta,42,90,'Pawn',False,[other for other in actors if other!=a],u.DrawDebugTrace.NONE,True))
        assert hit,label+' range cutaway wall has no collision'
        wall_hits.append(label)
    for inst in layout['instances']:
        if not inst['collision_only'] or not inst['asset'].startswith('SM_Basement_Corner'):continue
        a=labels[inst['id']];sign=1 if inst['asset'].endswith('Convex') else -1
        angle=math.radians(inst['rotation_xyz_deg'][2]);co=math.cos(angle);si=math.sin(angle)
        for segment in range(3):
            theta=-sign*math.pi/2+sign*(segment+.5)*math.pi/6
            nx,ny=math.cos(theta),math.sin(theta)
            radius=(.4+sign*.13)*math.cos(math.pi/12)
            x,y=-.4+nx*radius,sign*.4+ny*radius
            p=inst['location_m'];center=world_pos((p[0]+x*co-y*si,p[1]+x*si+y*co,1.4))
            delta=world_pos((.8*(nx*co-ny*si),.8*(nx*si+ny*co),0))
            hit,detail=blocking(u.SystemLibrary.capsule_trace_single_by_profile(world,center-delta,center+delta,42,90,'Pawn',False,[other for other in actors if other!=a],u.DrawDebugTrace.NONE,True))
            assert hit,(inst['id'],'corner segment',segment)
            wall_hits.append(inst['id']+'_segment'+str(segment))
    for p in [(0,0,0),(0,3.2,0),(0,9.19,0),(0,9.21,0),(0,10,0),(-8,0,0),(8,0,0),(0,-6,0)]:
        top=list(p);top[2]=.5;bottom=list(p);bottom[2]=-.5
        hit,detail=blocking(u.SystemLibrary.line_trace_single(world,world_pos(top),world_pos(bottom),u.TraceTypeQuery.TRACE_TYPE_QUERY1,False,[],u.DrawDebugTrace.NONE,True))
        assert hit,('floor missing',p)
        assert abs(detail.to_tuple()[5].z)<.2,('floor step',p,detail.to_tuple()[5])
    # Verify the complete recursive hierarchy, including nested caps and anchors.
    moved_groups=[]
    def descendants(parent):
        out=[]
        for child in parent.get_attached_actors():out.append(child);out.extend(descendants(child))
        return out
    for name in ('LadderHub','Range','North_RangeLink','East_Expansion','West_Expansion','South_Expansion'):
        root=labels['ROOT_'+name];children=descendants(root);assert children,name
        before={a:a.get_actor_location() for a in children};origin=root.get_actor_location();delta=u.Vector(120,80,0)
        root.static_mesh_component.set_mobility(u.ComponentMobility.MOVABLE)
        root.set_actor_location(origin+delta,False,True)
        assert all((a.get_actor_location()-before[a]-delta).length()<.1 for a in children),name
        root.set_actor_location(origin,False,True)
        rotation=root.get_actor_rotation();root.set_actor_rotation(u.Rotator(pitch=rotation.pitch,yaw=rotation.yaw+90,roll=rotation.roll),True)
        for a in children:
            offset=before[a]-origin;expected=origin+u.Vector(-offset.y,offset.x,offset.z)
            assert (a.get_actor_location()-expected).length()<.2,(name,a.get_actor_label(),'rotation',str(rotation),str(root.get_actor_rotation()),str(before[a]),str(a.get_actor_location()),str(expected))
        root.set_actor_rotation(rotation,True);root.static_mesh_component.set_mobility(u.ComponentMobility.STATIC)
        moved_groups.append({'root':name,'descendants':len(children),'translation_verified':True,'rotation_90_degrees_verified':True})
    report={'passed':True,'map':MAP,'engine':u.SystemLibrary.get_engine_version(),'assets':assets,'range_materials':material_report,'range_targets':4,'module_placements':len(layout['instances']),'hidden_collision_actors':hidden,'physical_invisible_wall_tests':wall_hits,'capsule_routes':sweeps,'floor_join_gap_cm':0,'expansion_connectors':3}
    report['module_group_movement']=moved_groups
    report['range_stations']=station_report
    (SOURCE/'unreal_validation.json').write_text(json.dumps(report,indent=2),encoding='utf8')
    u.log('BASEMENT_VERIFICATION_PASSED')
    return report

if __name__=='__main__':
    # A loaded level needs editor ticks for async mesh/Chaos registration.
    # Querying it synchronously can report a false missing floor.
    _world=u.EditorLoadingAndSavingUtils.load_map(MAP)
    u.AutomationLibrary.finish_loading_before_screenshot()
    _started=time.monotonic();_ticks=0;_busy=False
    u.EditorPythonScripting.set_keep_python_script_alive(True)
    def tick(dt):
        global _ticks,_busy
        if _busy:return
        _ticks+=1
        if _ticks<60 or time.monotonic()-_started<3:return
        _busy=True
        try:validate(_world)
        except Exception:
            error=traceback.format_exc();u.log_error(error)
            (SOURCE/'unreal_validation.json').write_text(json.dumps({'passed':False,'error':error},indent=2))
        finally:
            u.unregister_slate_post_tick_callback(_handle)
            u.EditorPythonScripting.set_keep_python_script_alive(False)
            u.SystemLibrary.quit_editor()
    _handle=u.register_slate_post_tick_callback(tick)
