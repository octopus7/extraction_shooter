"""Read-only fresh-editor persisted sample audit; no saving or generation."""
from pathlib import Path
import unreal, json, math

ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'TunaSweeper/SourceArt/Environment/ExtractionMarkers'
DEST='/Game/Interaction/ExtractionMarkers'
REUSE='/Game/Environment/ModularInteriorExpansion'
MAP=DEST+'/Maps/L_ExtractionMarkers'
REPORT=OUT/'unreal_scene_reload_validation.json'


def prepare():
    assert unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level(MAP), 'Map failed to load'


def xyz(v):
    return [v.x,v.y,v.z]


def hit_boolean(result):
    # UE Python exposes native bool+OutHit as optional HitResult on this engine.
    if result is None:return False
    if isinstance(result,bool):return result
    if isinstance(result,unreal.HitResult):return True
    if isinstance(result,tuple):
        flags=[v for v in result if isinstance(v,bool)]
        if flags:return flags[0]
        return any(isinstance(v,unreal.HitResult) for v in result)
    raise AssertionError('Unexpected trace return type '+str(type(result)))


def validate():
    m=json.loads((OUT/'model_manifest.json').read_text())
    actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
    labels={a.get_actor_label():a for a in actors}
    report={'passed':False,'engine':unreal.SystemLibrary.get_engine_version(),'map':MAP,
            'mode':'fresh editor reload, read-only, no PIE','prop_instances':[],'physics_traces':[]}
    props=[]
    for p in m['placements']:
        a=labels[p['name']];props.append(a);c=a.static_mesh_component
        mesh_path=(REUSE if p['mesh']=='SM_MIE_EmergencyLight' else DEST)+'/Meshes/'+p['mesh']
        assert c.static_mesh.get_path_name().split('.')[0]==mesh_path
        loc=xyz(a.get_actor_location());scale=xyz(a.get_actor_scale3d());rot=a.get_actor_rotation()
        assert max(abs(a-b*100) for a,b in zip(loc,p['location_m']))<.1
        assert max(abs(v-1) for v in scale)<1e-5
        assert max(abs(v) for v in (rot.pitch,rot.yaw,rot.roll))<.01
        assert c.get_collision_enabled()==unreal.CollisionEnabled.NO_COLLISION
        assert not c.get_editor_property('generate_overlap_events')
        assert not c.get_editor_property('can_ever_affect_navigation')
        # Check actual signed render bounds against intended off-center placement.
        e=next(e for e in m['assets'] if e['name']==p['mesh']);b=e['bounds_m']
        lo=[loc[i]+b[i]*100 for i in range(3)];hi=[loc[i]+b[i+3]*100 for i in range(3)]
        nearest=[max(lo[i],min(0,hi[i])) for i in range(2)]
        clearance=math.hypot(*nearest)-m['sample_radius_cm']
        assert clearance>0,(p['name'],'prop enters extraction disk',clearance)
        report['prop_instances'].append({'name':p['name'],'mesh':mesh_path,'location_cm':loc,
            'scale':scale,'rotation_degrees':[rot.pitch,rot.yaw,rot.roll],'collision':'NoCollision',
            'generate_overlap_events':False,'can_ever_affect_navigation':False,
            'horizontal_bounds_clearance_from_extraction_disk_cm':clearance})
    assert len(props)==3
    static_meshes=[a for a in actors if isinstance(a,unreal.StaticMeshActor)]
    assert len(static_meshes)==4,'Unexpected additional static mesh actors'
    ex=labels['ExistingExtraction_R300_SampleOnly']
    assert ex.get_class().get_path_name()=='/Game/Interaction/BP_ExtractionPoint.BP_ExtractionPoint_C'
    assert abs(ex.get_editor_property('extraction_radius')-300)<.01
    assert str(ex.get_editor_property('target_level_name'))=='None','Review extraction could travel to a map'
    assert ex.get_actor_location().length()<.01
    rings=ex.get_components_by_class(unreal.ProceduralMeshComponent)
    assert len(rings)==1
    ring=rings[0]
    assert ring.get_collision_enabled()==unreal.CollisionEnabled.NO_COLLISION
    assert not ring.get_editor_property('generate_overlap_events')
    section=unreal.ProceduralMeshLibrary.get_section_from_procedural_mesh(ring,0)
    vertices,indices=section[0],section[1]
    assert len(indices)==576 and len(vertices)==194,(len(vertices),len(indices))
    assert abs(max(math.hypot(v.x,v.y) for v in vertices)-300)<.02
    for area in ex.get_components_by_class(unreal.SphereComponent):
        assert area.get_collision_enabled()==unreal.CollisionEnabled.NO_COLLISION
        assert not area.get_editor_property('generate_overlap_events')
    niagara=ex.get_components_by_class(unreal.NiagaraComponent)
    assert len(niagara)==1
    fx=niagara[0].get_asset();assert fx and fx.get_path_name().split('.')[0]=='/Game/FX/NS_ExtractionSmoke'
    report['extraction']={'actor_class':ex.get_class().get_path_name(),'radius_cm':300,
        'ring_vertices':len(vertices),'ring_triangles':len(indices)//3,'ring_collision':'NoCollision',
        'smoke_system':fx.get_path_name(),'actor_origin_cm':xyz(ex.get_actor_location())}
    bp=unreal.load_asset('/Game/Characters/Player/BP_TunaSweeperPlayerCharacter');assert bp
    settings=unreal.get_default_object(bp.generated_class()).get_editor_property('top_down_camera_mode_settings')
    arm=settings.get_editor_property('target_arm_length');pitch=settings.get_editor_property('boom_rotation').pitch
    yaw=settings.get_editor_property('boom_rotation').yaw;fov=settings.get_editor_property('default_fov')
    assert abs(arm-1500)<.1 and abs(pitch+88)<.1 and abs(yaw)<.1 and abs(fov-70)<.1
    cam=labels['UE_Extraction_PlayCamera'];rotation=cam.get_actor_rotation()
    assert abs(rotation.pitch-pitch)<.01 and abs(rotation.yaw-yaw)<.01
    assert abs(cam.camera_component.field_of_view-fov)<.01
    report['camera']={'verified_against_player_bp':True,'arm_cm':arm,'pitch':pitch,'yaw':yaw,'fov':fov}
    counts={kind.__name__:sum(isinstance(a,kind) for a in actors) for kind in
            (unreal.DirectionalLight,unreal.SkyLight,unreal.PointLight,unreal.SpotLight,unreal.RectLight)}
    assert counts['DirectionalLight']==1 and counts['SkyLight']==1
    assert counts['PointLight']==counts['SpotLight']==counts['RectLight']==0
    report['light_actor_counts']=counts
    world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    game_mode=world.get_world_settings().get_editor_property('default_game_mode')
    assert game_mode and game_mode.get_path_name()=='/Script/Engine.GameModeBase','Review map must use GameModeBase'
    report['sample_travel_prevention']={'target_level_name':str(ex.get_editor_property('target_level_name')),
                                       'default_game_mode':game_mode.get_path_name()}
    tests=[('center_north_south',(-450,0,90),(450,0,90),34,False),
           ('center_east_west',(0,-450,90),(0,450,90),34,False),
           ('center_floor',(0,0,100),(0,0,-50),0,True)]
    for p in m['placements']:
        x,y,z=[v*100 for v in p['location_m']]
        tests.append(('disabled_prop_'+p['name'],(x-100,y,z+20),(x+100,y,z+20),5,False))
    for name,start,end,radius,expected in tests:
        args=(world,unreal.Vector(*start),unreal.Vector(*end))
        tail=(unreal.TraceTypeQuery.TRACE_TYPE_QUERY1,False,[],unreal.DrawDebugTrace.NONE,True)
        result=unreal.SystemLibrary.sphere_trace_single(*args,radius,*tail) if radius else unreal.SystemLibrary.line_trace_single(*args,*tail)
        hit=hit_boolean(result);assert hit==expected,(name,hit,expected)
        report['physics_traces'].append({'name':name,'start_cm':list(start),'end_cm':list(end),
            'sphere_radius_cm':radius,'hit':hit,'expected_hit':expected})
    report['passed']=True
    REPORT.write_text(json.dumps(report,indent=2),encoding='utf-8')
    unreal.log('EM_UE_SCENE_RELOAD_PASSED')
    return report
