"""Read-only saved camera/cutaway and actual PIE distance-blend verification.

Run in a dedicated rendered editor with -ExecutePythonScript. Saves no assets.
"""
import json,math,time,traceback,hashlib
from pathlib import Path
import unreal as u
ROOT=Path(__file__).resolve().parents[2]
SOURCE=ROOT/'TunaSweeper/SourceArt/Environment/Basement'
MAP=ROOT/'TunaSweeper/Content/Environment/Basement/Maps/L_Basement.umap'
command=u.SystemLibrary.get_command_line().lower()
assert u.SystemLibrary.is_unattended() and '-executepythonscript' in command and '-nullrhi' not in command
baseline=hashlib.sha256(MAP.read_bytes()).hexdigest()
meta=json.loads((SOURCE/'range_camera_manifest.json').read_text())
levels=u.get_editor_subsystem(u.LevelEditorSubsystem);editor=u.get_editor_subsystem(u.UnrealEditorSubsystem)
world=u.EditorLoadingAndSavingUtils.load_map('/Game/Environment/Basement/Maps/L_Basement')
actors=u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors();labels={a.get_actor_label():a for a in actors}
rig=labels[meta['blend_actor']];camera=labels[meta['camera']]
report={'passed':False,'static_weights':[],'physical_cutaway_walls':[],'runtime_samples':[]}
def blocking(r):return isinstance(r,u.HitResult) and bool(r.to_tuple()[0])
def static_checks():
    assert rig.get_class()==u.load_asset(meta['blueprint']).generated_class()
    assert rig.get_target_camera_actor()==camera
    assert rig.get_attach_parent_actor()==camera.get_attach_parent_actor()==labels['ROOT_Range']
    assert (camera.get_actor_location()-u.Vector(*meta['camera_location_cm'])).length()<.01
    assert abs(camera.camera_component.field_of_view-meta['field_of_view'])<.01
    for p,expected in [((0,0,95),0),((340,0,95),0),((790,0,95),1),((1590,0,95),1)]:
        w=rig.get_blend_weight_at_location(u.Vector(*p));assert abs(w-expected)<.0001,(p,w)
        report['static_weights'].append({'location_cm':p,'weight':w})
    # The entire octagonal range footprint stays inside the complete-blend zone.
    original=json.loads((SOURCE/'Range/Scene/placement_manifest.json').read_text())
    cam=camera.camera_component;fwd=cam.get_forward_vector();right=cam.get_right_vector();up=cam.get_up_vector()
    tan=math.tan(math.radians(cam.field_of_view/2));screen=[]
    for x,y in original['boundary_xy_m']:
        p=u.Vector((15+y)*100,x*100,95)
        assert rig.get_blend_weight_at_location(p)>.999,(x,y)
        delta=p-cam.get_world_location();depth=delta.dot(fwd)
        nx=delta.dot(right)/(depth*tan);ny=delta.dot(up)/(depth*tan/(16/9))
        assert depth>0 and abs(nx)<.98 and abs(ny)<.98,('framing',x,y,nx,ny)
        screen.append([round(nx,4),round(ny,4)])
    report['range_floor_ndc_16_9']=screen
    for name in meta['additional_collision_only_walls']+meta['hidden_wall_fixtures']:
        a=labels[name];c=a.static_mesh_component
        assert not c.get_editor_property('visible') and c.get_editor_property('hidden_in_game'),name
        assert not c.get_editor_property('cast_shadow') and c.get_collision_enabled()==u.CollisionEnabled.QUERY_AND_PHYSICS
        assert str(c.get_collision_profile_name())=='BlockAll'
        if name not in meta['additional_collision_only_walls']:continue
        center,extent=a.get_actor_bounds(False);delta=u.Vector(extent.x+80,0,0) if extent.x<extent.y else u.Vector(0,extent.y+80,0)
        result=u.SystemLibrary.capsule_trace_single_by_profile(world,center-delta,center+delta,42,90,'Pawn',False,[o for o in actors if o!=a],u.DrawDebugTrace.NONE,True)
        assert blocking(result),name
        report['physical_cutaway_walls'].append(name)

samples=[('hub',(0,0,95)),('approach',(500,0,95)),('entrance',(930,0,95))]
samples += [(f'booth_{i}',(1095,y,95)) for i,y in enumerate([-315,-105,105,315],1)]
samples += [('left_target_side',(1780,-420,95)),('right_target_side',(1780,420,95)),('exit',(0,0,95))]
started=time.monotonic();state='settle';ticks=0;busy=False;index=0
def finish(error=None):
    report['passed']=error is None
    if error:report['error']=error;u.log_error(error)
    report['saved_map_unchanged']=hashlib.sha256(MAP.read_bytes()).hexdigest()==baseline
    report['passed']=report['passed'] and report['saved_map_unchanged']
    (SOURCE/'range_camera_validation.json').write_text(json.dumps(report,indent=2))
    u.log('RANGE_CAMERA_RUNTIME_'+('PASSED' if report['passed'] else 'FAILED'))
    u.unregister_slate_post_tick_callback(handle)
    if levels.is_in_play_in_editor():levels.editor_request_end_play()
    u.EditorPythonScripting.set_keep_python_script_alive(False);u.SystemLibrary.quit_editor()
def tick(dt):
    global state,ticks,busy,index,game,pc,pawn,live,live_camera,task
    if busy:return
    busy=True;ticks+=1
    try:
        assert time.monotonic()-started<240,'Camera runtime check timed out'
        if state=='settle' and ticks>=60:
            static_checks();levels.editor_request_begin_play();state='pie';ticks=0
        elif state=='pie' and ticks>=90:
            game=editor.get_game_world();assert game
            pc=u.GameplayStatics.get_player_controller(game,0);assert pc
            pawn=u.GameplayStatics.get_player_pawn(game,0);assert isinstance(pawn,u.TunaSweeperTopDownCharacter),str(pawn)
            pawn.character_movement.disable_movement()
            live=next(a for a in u.GameplayStatics.get_all_actors_of_class(game,u.TunaSweeperLocationBlendCameraActor) if a.get_actor_label()==meta['blend_actor'])
            live_camera=live.get_target_camera_actor();state='move';ticks=0
        elif state=='move':
            pawn.set_actor_location(u.Vector(*samples[index][1]),False,True);state='sample';ticks=0
        elif state=='sample' and ticks>=20:
            name,p=samples[index];weight=live.get_current_blend_weight();expected=live.get_blend_weight_at_location(pawn.get_actor_location())
            assert abs(weight-expected)<.001,(name,weight,expected)
            target=pc.get_view_target()
            if name in ('hub','exit'):assert weight==0 and target==pawn,(name,str(target))
            else:
                assert target==live,(name,str(target))
                if name=='approach':assert 0<weight<1
                else:
                    assert weight>.999,(name,weight)
                    mgr=u.GameplayStatics.get_player_camera_manager(game,0)
                    assert (mgr.get_camera_location()-live_camera.camera_component.get_world_location()).length()<1,(name,'POV position')
                    assert abs(mgr.get_fov_angle()-meta['field_of_view'])<.1,(name,'POV field of view')
                    actual=mgr.get_camera_rotation();goal=live_camera.camera_component.get_world_rotation()
                    assert abs(actual.yaw-goal.yaw)<.1 and abs(actual.pitch-goal.pitch)<.1,(name,'POV angle')
            report['runtime_samples'].append({'position':name,'weight':weight,'view_target':target.get_actor_label()})
            index+=1
            if index==len(samples):
                pawn.set_actor_location(u.Vector(1095,105,95),False,True);state='screenshot_wait';ticks=0
            else:state='move'
        elif state=='screenshot_wait' and ticks>=90:
            task=u.AutomationLibrary.take_high_res_screenshot(1600,900,str(SOURCE/'Previews/UE_Basement_RangeGameplay.png'),delay=0.0)
            state='screenshot';ticks=0
        elif state=='screenshot' and task.is_task_done():
            assert (SOURCE/'Previews/UE_Basement_RangeGameplay.png').is_file()
            levels.editor_request_end_play();state='end';ticks=0
        elif state=='end' and ticks>=15:finish()
    except Exception:finish(traceback.format_exc())
    finally:busy=False
u.EditorPythonScripting.set_keep_python_script_alive(True)
handle=u.register_slate_post_tick_callback(tick)
