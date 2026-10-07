"""Read-only saved Blueprint collision and PIE damage/recovery verification.

Run in a dedicated unattended rendered editor with -ExecutePythonScript.
Never saves a map or modifies asset defaults.
"""
import json,time,traceback,sys
from pathlib import Path
import unreal as u
command_line=u.SystemLibrary.get_command_line().lower()
if (not u.SystemLibrary.is_unattended() or '-executepythonscript' not in command_line
        or Path(__file__).name.lower() not in command_line or '-nullrhi' in command_line):
    raise RuntimeError('Run this verifier in a dedicated unattended rendered editor')
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(Path(__file__).parent))
from verify_target import verify_asset,BP_PATH,SOURCE
report=verify_asset();report['passed']=False
levels=u.get_editor_subsystem(u.LevelEditorSubsystem)
assert levels.load_level('/Game/Environment/Basement/Maps/L_Basement')
actors=u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors()
targets=[a for a in actors if 'TS_RangeTarget' in [str(t) for t in a.tags]]
assert len(targets)==5
cls=u.load_asset(BP_PATH).generated_class()
assert all(a.get_class()==cls for a in targets)
assert all(a.get_attach_parent_actor().get_actor_label()=='ROOT_Range' for a in targets)
world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
state='settle';ticks=0;busy=False;started=time.monotonic()
u.EditorPythonScripting.set_keep_python_script_alive(True)

def finish(error=None):
    report['passed']=error is None
    if error:report['error']=error;u.log_error(error)
    (SOURCE/'target_runtime_validation.json').write_text(json.dumps(report,indent=2))
    u.unregister_slate_post_tick_callback(handle)
    if levels.is_in_play_in_editor():levels.editor_request_end_play()
    u.EditorPythonScripting.set_keep_python_script_alive(False)
    u.log('RANGE_TARGET_RUNTIME_'+('FAILED' if error else 'PASSED'))
    u.SystemLibrary.quit_editor()

def tick(dt):
    global ticks,busy,state,game,live,recovery_started,lowest
    if busy:return
    busy=True;ticks+=1
    try:
        assert time.monotonic()-started<180,'Runtime verification timeout'
        if state=='settle' and ticks>=60:
            report['collision_rays']=[]
            for a in targets:
                tm=a.get_actor_transform()
                # Sensor, chassis, both half-height ears and empty old tall tips.
                for name,start,end,expected in [
                    ('sensor_lens',(0,200,108),(0,0,108),'HeadMesh'),
                    ('sensor_housing',(29,200,108),(29,0,108),'HeadshotPlateMesh'),
                    ('body',(0,200,80),(0,0,80),'BodyMesh'),
                    ('short_ear_left',(-18,200,140),(-18,-100,140),'HeadMesh'),
                    ('short_ear_right',(18,200,140),(18,-100,140),'HeadMesh'),
                    ('former_tall_tips',(18,200,170),(18,-100,170),None)]:
                    p=u.MathLibrary.transform_location(tm,u.Vector(*start));q=u.MathLibrary.transform_location(tm,u.Vector(*end))
                    hit=u.SystemLibrary.line_trace_single(world,p,q,u.TraceTypeQuery.TRACE_TYPE_QUERY1,False,[o for o in actors if o!=a],u.DrawDebugTrace.NONE,True)
                    if expected:
                        assert hit and hit.to_tuple()[0],(a.get_actor_label(),name,'no collision')
                        component=hit.to_tuple()[10];assert component.get_name()==expected,(name,component.get_name())
                    else:assert hit is None or not hit.to_tuple()[0],'Original tall ear tips still collide'
                    report['collision_rays'].append({'target':a.get_actor_label(),'zone':name,'component':expected})
            levels.editor_play_simulate();state='pie_wait';ticks=0
        elif state=='pie_wait' and ticks>=40:
            game=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_game_world();assert game
            live=sorted(u.GameplayStatics.get_all_actors_of_class(game,cls),key=lambda a:a.get_editor_property('body_armor_tier'));assert len(live)==5
            for a in live:
                assert abs(a.get_health_fraction()-1)<.0001
                widget=a.get_editor_property('health_bar_widget_component').get_user_widget_object();assert widget
            a=live[0]
            damage=u.GameplayStatics.apply_damage(a,25,None,None,u.DamageType)
            assert abs(damage-25)<.001 and abs(a.get_health_fraction()-.75)<.001
            u.GameplayStatics.apply_damage(a,10000,None,None,u.DamageType)
            lowest=a.get_health_fraction();assert abs(lowest-.01)<.001
            assert all(abs(o.get_health_fraction()-1)<.001 for o in live[1:])
            report['damage']={'input':25,'returned_damage':damage,'health_after':75,'minimum_health_after_overkill':lowest*100,'health_widgets':len(live),'other_targets_unchanged':True}
            recovery_started=u.GameplayStatics.get_time_seconds(game);state='recovery'
        elif state=='recovery':
            elapsed=u.GameplayStatics.get_time_seconds(game)-recovery_started
            health=live[0].get_health_fraction()
            assert health>=lowest-.001;lowest=health
            if elapsed>=2.1:
                assert abs(health-1)<.001,health
                report['recovery']={'native_tick':True,'game_seconds':elapsed,'final_health':health*100}
                levels.editor_request_end_play();state='end';ticks=0
        elif state=='end' and ticks>=15:finish()
    except Exception:finish(traceback.format_exc())
    finally:busy=False

handle=u.register_slate_post_tick_callback(tick)
