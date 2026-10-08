"""Read-only rendered editor capture; no saved cameras or generation entry points.

Dedicated UnrealEditor.exe -unattended -ExecutePythonScript=<this file>.
"""
import hashlib,json,time,traceback
from pathlib import Path
import unreal as u

ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'TunaSweeper/SourceArt/Environment/BunkerControl/Previews'
assert u.SystemLibrary.is_unattended() and '-executepythonscript' in u.SystemLibrary.get_command_line().lower()
map_file=ROOT/'TunaSweeper/Content/Maps/BunkerMap.umap'
baseline=hashlib.sha256(map_file.read_bytes()).hexdigest()
levels=u.get_editor_subsystem(u.LevelEditorSubsystem);es=u.get_editor_subsystem(u.EditorActorSubsystem)
shots=[('Room',[-3430,-2200,1091],u.Rotator(pitch=-60,yaw=0,roll=0),50.),('Entrances',[-1565,0,1700],u.Rotator(pitch=-60,yaw=0,roll=0),60.)]
report={'passed':False,'captures':[]};start=time.monotonic();stage='viewport';ticks=0;index=0;task=None;camera=None;busy=False;stage_start=start
def transition(value):
    global stage,ticks,stage_start
    stage=value;ticks=0;stage_start=time.monotonic()
def finish(error=None):
    report['saved_map_unchanged']=hashlib.sha256(map_file.read_bytes()).hexdigest()==baseline
    report['passed']=not error and report['saved_map_unchanged'] and len(report['captures'])==len(shots)
    if error:report['error']=error;u.log_error(error)
    (OUT/'capture_validation.json').write_text(json.dumps(report,indent=2),encoding='utf8')
    u.log('BUNKER_CONTROL_CAPTURE_'+('PASSED' if report['passed'] else 'FAILED'))
    u.unregister_slate_post_tick_callback(handle);u.EditorPythonScripting.set_keep_python_script_alive(False);u.SystemLibrary.quit_editor()
def tick(dt):
    global ticks,index,task,camera,busy
    if busy:return
    busy=True;ticks+=1
    try:
        assert time.monotonic()-start<240,'Capture timeout: '+stage
        if stage=='viewport' and levels.get_viewport_config_keys() and ticks>=5:
            assert levels.load_level('/Game/Maps/BunkerMap');transition('camera')
        elif stage=='camera' and ticks>=10:
            name,loc,rot,fov=shots[index]
            camera=es.spawn_actor_from_class(u.CameraActor,u.Vector(*loc),rot,transient=True)
            camera.camera_component.set_editor_property('field_of_view',fov)
            u.log('CAPTURE_CAMERA '+str(camera.get_actor_location())+' '+str(camera.get_actor_rotation())+' '+str(camera.camera_component.get_world_rotation()))
            levels.editor_set_viewport_realtime(True);levels.editor_set_game_view(True);levels.pilot_level_actor(camera);levels.set_exact_camera_view(True)
            u.AutomationLibrary.set_editor_active_viewport_view_mode(u.ViewModeIndex.VMI_LIT)
            u.AutomationLibrary.finish_loading_before_screenshot();transition('warmup')
        elif stage=='warmup':
            levels.editor_invalidate_viewports()
            if ticks>=90 and time.monotonic()-stage_start>=8:
                task=u.AutomationLibrary.take_high_res_screenshot(1600,900,str(OUT/('UE_'+shots[index][0]+'.png')),camera=camera,mask_enabled=False,capture_hdr=False,delay=1.,force_game_view=True);transition('pending')
        elif stage=='pending':
            levels.editor_invalidate_viewports()
            if task and task.is_task_done():
                path=OUT/('UE_'+shots[index][0]+'.png')
                if not path.is_file():return
                report['captures'].append({'view':shots[index][0],'bytes':path.stat().st_size})
                levels.eject_pilot_level_actor();es.destroy_actor(camera);index+=1
                if index==len(shots):finish()
                else:transition('camera')
    except Exception:finish(traceback.format_exc())
    finally:busy=False
u.EditorPythonScripting.set_keep_python_script_alive(True)
handle=u.register_slate_post_tick_callback(tick)
