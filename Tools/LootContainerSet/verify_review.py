"""Read-only saved BP/map, native PIE animation, collision and screenshot audit.

All scene changes are transient. Never saves a game asset or review asset.
"""
from pathlib import Path
import unreal,json,traceback,math,time
ROOT=Path(__file__).resolve().parents[2];OUT=ROOT/'TunaSweeper/SourceArt/Environment/LootContainerSet'
DEST='/Game/Interaction/LootContainerSet';manifest=json.loads((OUT/'model_manifest.json').read_text())
report={'passed':False,'saved_map_reload':False,'animation':[],'captures':[]}
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem);actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
assert levels.load_level(DEST+'/Review/L_LootContainerSet')
bylabel={a.get_actor_label():a for a in actors.get_all_level_actors()}
testactors=[bylabel['LC_'+s['key']] for s in manifest['sets']]
def v(v):return [v.x,v.y,v.z]
for a,spec in zip(testactors,manifest['sets']):
    assert a.get_class().get_path_name()==DEST+'/Review/BP_LC_Review_'+spec['key']+'.BP_LC_Review_'+spec['key']+'_C'
    assert a.get_container_definition_id()==0
    body=a.get_body_mesh_component();lid=a.get_lid_mesh_component();pivot=a.get_lid_pivot_component()
    for comp,part in [(body,'Body'),(lid,'Lid')]:
        assert comp.static_mesh.get_path_name()==DEST+'/Meshes/SM_LC_'+spec['key']+'_'+part+'.SM_LC_'+spec['key']+'_'+part
        assert max(abs(c) for c in v(comp.get_editor_property('relative_location')))<.01
        assert max(abs(c-1) for c in v(comp.get_editor_property('relative_scale3d')))<.01
        assert comp.get_material(0).get_path_name()=='/Game/Environment/LabSupplyProps/Materials/M_LSP_Surface.M_LSP_Surface'
        assert comp.get_collision_enabled()==unreal.CollisionEnabled.QUERY_AND_PHYSICS
    assert max(abs(x-y*100) for x,y in zip(v(pivot.get_editor_property('relative_location')),spec['hinge_m']))<.01
    a.set_lid_open(True,True);assert abs(pivot.get_editor_property('relative_rotation').roll+105)<.01
    a.set_lid_open(False,True);assert abs(pivot.get_editor_property('relative_rotation').roll)<.01
report['saved_map_reload']=True
# SAT tests the actual persisted collision boxes against each other over the full arc.
def sat_box(lo,hi,matrix,otherlo,otherhi):
    center=unreal.MathLibrary.transform_location(matrix,unreal.Vector(*[(a+b)/2 for a,b in zip(lo,hi)]))
    basis=[unreal.MathLibrary.transform_direction(matrix,unreal.Vector(*b)) for b in [(1,0,0),(0,1,0),(0,0,1)]]
    axes=[unreal.Vector(1,0,0),unreal.Vector(0,1,0),unreal.Vector(0,0,1)]+basis
    axes +=[a.cross(b) for a in axes[:3] for b in basis]
    oc=unreal.Vector(*[(a+b)/2 for a,b in zip(otherlo,otherhi)]);ae=[(b-a)/2 for a,b in zip(lo,hi)];be=[(b-a)/2 for a,b in zip(otherlo,otherhi)]
    for axis in axes:
        if axis.length()<1e-7:continue
        axis=axis.normal();distance=abs((center-oc).dot(axis));ra=sum(e*abs(b.dot(axis)) for e,b in zip(ae,basis));rb=sum(e*abs(c) for e,c in zip(be,v(axis)))
        if distance>=ra+rb-1e-4:return False
    return True
report['collision_sweep']=[]
for spec in manifest['sets']:
    body=next(e for e in manifest['assets'] if e['name']=='SM_LC_'+spec['key']+'_Body')
    lid=next(e for e in manifest['assets'] if e['name']=='SM_LC_'+spec['key']+'_Lid')
    for angle in range(106):
        tm=unreal.Transform(location=unreal.Vector(*[x*100 for x in spec['hinge_m']]),rotation=unreal.Rotator(roll=-angle),scale=unreal.Vector(1,1,1))
        for llo,lhi in lid['collision_boxes']:
            for blo,bhi in body['collision_boxes']:
                assert not sat_box([x*100 for x in llo],[x*100 for x in lhi],tm,[x*100 for x in blo],[x*100 for x in bhi]),(spec['key'],angle)
    report['collision_sweep'].append({'kind':spec['key'],'sampled_angles':106,'penetrating_pairs':0,'note':'Persisted box geometry checked by asset audit; SAT on same boxes in centimeters.'})
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
def trace(a,start,end):
    offset=a.get_actor_location()
    result=unreal.SystemLibrary.line_trace_single(world,offset+unreal.Vector(*start),offset+unreal.Vector(*end),unreal.TraceTypeQuery.TRACE_TYPE_QUERY1,False,[],unreal.DrawDebugTrace.NONE,True)
    # UE Python exposes the bool/out-HitResult pair as HitResult or None.
    return result is not None
report['physics_rays']=[]
for a,spec in zip(testactors,manifest['sets']):
    w,d,h=[x*100 for x in spec['dimensions_m']];seam=spec['hinge_m'][2]*100
    a.set_lid_open(False,True)
    assert trace(a,[0,0,h+20],[0,0,seam-1]),spec['key']+' closed lid does not block'
    a.set_lid_open(True,True)
    assert not trace(a,[0,0,h+20],[0,0,seam-1]),spec['key']+' opened lid blocks mouth'
    assert trace(a,[0,0,seam-1],[0,0,-10]),spec['key']+' missing floor collision'
    assert not trace(a,[0,-d/4,seam/2],[0,d/4,seam/2]),spec['key']+' cavity is blocked'
    assert trace(a,[0,d,seam/2],[0,0,seam/2]),spec['key']+' front wall does not block'
    a.set_lid_open(False,True)
    report['physics_rays'].append({'kind':spec['key'],'closed_lid_blocks':True,'open_mouth_clear':True,'floor_blocks':True,'interior_clear':True,'front_wall_blocks':True})
floor=bylabel['LC_ReviewGround'];floor.static_mesh_component.set_material(0,unreal.load_asset('/Engine/BasicShapes/BasicShapeMaterial'))
for name in ['Oblique','Topdown','Rear']:
    cam=bylabel['LC_Camera_'+name].camera_component
    settings=cam.get_editor_property('post_process_settings');settings.set_editor_property('override_auto_exposure_method',False);settings.set_editor_property('auto_exposure_bias',1);cam.set_editor_property('post_process_settings',settings)
bylabel['LC_Camera_Topdown'].set_actor_rotation(unreal.Rotator(pitch=-90),False)
bylabel['LC_Camera_Topdown'].set_actor_location(unreal.Vector(0,0,1300),False,False)
for cmd in ['r.Shadow.Virtual.SMRT.RayCountLocal 16','r.Shadow.Virtual.SMRT.SamplesPerRayLocal 16']:
    unreal.SystemLibrary.execute_console_command(world,cmd)
state='warmup';ticks=0;busy=False;framecount=0;anim=[];simactors=[];task=None
captures=[('Closed','Oblique'),('Closed','Topdown'),('Open','Oblique'),('Open','Topdown'),('Open','Rear')]
def write(): (OUT/'unreal_review_validation.json').write_text(json.dumps(report,indent=2))
def tick(dt):
    global ticks,busy,state,framecount,anim,simactors,task
    if busy:return
    busy=True;ticks+=1
    try:
        assert ticks<2400,'Review timed out'
        if state=='warmup' and ticks>=80:
            levels.editor_play_simulate();state='pie_wait';framecount=0
        elif state=='pie_wait':
            framecount+=1
            if framecount<30:return
            game=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world();assert game
            parent=unreal.load_class(None,'/Script/TunaSweeper.TunaSweeperLootContainerActor')
            simactors=list(unreal.GameplayStatics.get_all_actors_of_class(game,parent));assert len(simactors)==3
            for a in simactors:a.set_lid_open(False,True);a.play_open_animation()
            state='opening';framecount=0;anim=[]
        elif state in ['opening','closing']:
            framecount+=1;values=[a.get_lid_pivot_component().get_editor_property('relative_rotation').roll for a in simactors];anim.append(values)
            target=-105 if state=='opening' else 0
            if not (all(abs(x-target)<.02 for x in values) and all(a.is_lid_open()==(state=='opening') for a in simactors)):
                assert framecount<500,'Native animation did not finish'
                return
            assert all(abs(x-target)<.02 for x in values),(state,values)
            assert any(any(-104<x<-1 for x in row) for row in anim),'No intermediate native animation frames'
            assert all(a.is_lid_open()==(state=='opening') for a in simactors)
            report['animation'].append({'phase':state,'frames':len(anim),'sampled_rolls':anim,'target_roll':target,'native_tick':True})
            if state=='opening':
                for a in simactors:a.play_close_animation()
                state='closing';framecount=0;anim=[]
            else:levels.editor_request_end_play();state='end_pie';framecount=0
        elif state=='end_pie':
            framecount+=1
            if framecount>30:
                # The project's PIE game instance applies TextureQuality=0.
                # Restore full source detail in this review process only.
                for cmd in ['sg.TextureQuality 3','r.Streaming.MipBias 0','r.Streaming.FullyLoadUsedTextures 1']:
                    unreal.SystemLibrary.execute_console_command(world,cmd)
                framecount=0;state='texture_wait'
        elif state=='texture_wait':
            framecount+=1
            atlas=unreal.load_asset('/Game/Environment/LabSupplyProps/Textures/T_LSP_Atlas')
            mask=unreal.load_asset('/Game/Environment/ModularInteriorPreview/Textures/T_MI_DirtMask')
            sizes=[[t.blueprint_get_size_x(),t.blueprint_get_size_y()] for t in [atlas,mask]]
            if framecount<60 or sizes!=[[2048,2048],[1024,1024]]:
                assert framecount<600,'Texture residency did not reach authored resolution: '+str(sizes)
                return
            report['capture_texture_residency']=sizes;report['capture_texture_quality']=3;state='capture'
        elif state=='capture':
            if task and not task.is_task_done():return
            if captures:
                status,view=captures.pop(0)
                for a in testactors:a.set_lid_open(status=='Open',True)
                filename='UE_Actual_'+status+'_'+view+'.png'
                task=unreal.AutomationLibrary.take_high_res_screenshot(1440,1000,str(OUT/'Previews'/filename),camera=bylabel['LC_Camera_'+view],delay=3)
                assert task;report['captures'].append(filename)
            else:
                assert all((OUT/'Previews'/p).is_file() for p in report['captures'])
                report['passed']=True;write();unreal.unregister_slate_post_tick_callback(handle);unreal.SystemLibrary.quit_editor()
    except Exception:
        report['error']=traceback.format_exc();unreal.log_error(report['error']);write();unreal.unregister_slate_post_tick_callback(handle);levels.editor_request_end_play();unreal.SystemLibrary.quit_editor()
    finally:busy=False
handle=unreal.register_slate_post_tick_callback(tick)
