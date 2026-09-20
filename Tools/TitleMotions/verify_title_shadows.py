"""Read-only saved title lighting/PIE coverage audit and actual viewport capture.

Run with -ExecutePythonScript in a separate UE editor. Does not save assets.
"""
import json
import math
import time
import traceback
from pathlib import Path
import unreal

OUT = Path(__file__).resolve().parents[2] / 'TunaSweeper/Saved/Automation/TitleShadows'
OUT.mkdir(parents=True, exist_ok=True)
report = {'status': 'running', 'max_bone_angle_degrees': 0.0, 'sample_count': 0}
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
world = unreal.EditorLoadingAndSavingUtils.load_map('/Game/Maps/IntroMap')
started = time.monotonic()
frame = 0
busy = False
task = None

def tick(delta):
    global frame, busy, task
    if busy:
        return
    busy = True
    frame += 1
    try:
        assert time.monotonic() - started < 180, 'PIE lighting audit timed out'
        if frame == 10:
            levels.editor_request_begin_play()
        game = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
        if not game:
            return
        elapsed = unreal.GameplayStatics.get_time_seconds(game)
        studio = unreal.GameplayStatics.get_all_actors_of_class(game, unreal.TunaSweeperTitleStudioActor)[0]
        character = unreal.GameplayStatics.get_all_actors_of_class(game, unreal.TunaSweeperTitlePresentationActor)[0]
        key = next(c for c in studio.get_components_by_class(unreal.PointLightComponent) if c.get_name() == 'CharacterKeyLight')
        fill = next(c for c in studio.get_components_by_class(unreal.PointLightComponent) if c.get_name() == 'EmptyWallLight')
        assert isinstance(key, unreal.SpotLightComponent), 'Saved title must use the focused spotlight'
        assert key.get_editor_property('source_radius') > 0, 'Key needs a finite source for soft shadows'
        assert key.get_editor_property('cast_shadows'), 'Key must retain self-shadowing'
        assert not fill.get_editor_property('cast_shadows'), 'Fill must not add a second hard shadow'
        body = next(c for c in character.get_components_by_class(unreal.SkeletalMeshComponent) if c.get_name() == 'BodyMesh')
        if elapsed > 0.5:
            for bone in ('head', 'pelvis', 'hand_l', 'hand_r', 'foot_l', 'foot_r'):
                assert body.does_socket_exist(bone), 'Missing coverage bone: ' + bone
                delta_position = body.get_socket_location(bone) - key.get_world_location()
                distance = delta_position.length()
                dot = unreal.MathLibrary.dot_vector_vector(delta_position / distance, key.get_forward_vector())
                angle = math.degrees(math.acos(max(-1.0, min(1.0, dot))))
                report['max_bone_angle_degrees'] = max(report['max_bone_angle_degrees'], angle)
                assert angle < key.get_editor_property('inner_cone_angle'), (bone, 'outside fully lit cone', angle)
                assert distance < key.get_editor_property('attenuation_radius'), (bone, 'outside light range')
            report['sample_count'] += 1
        if elapsed >= 12.0 and task is None:
            report['source_radius_cm'] = key.get_editor_property('source_radius')
            report['inner_cone_degrees'] = key.get_editor_property('inner_cone_angle')
            report['outer_cone_degrees'] = key.get_editor_property('outer_cone_angle')
            unreal.GameplayStatics.set_game_paused(game, True)
            task = unreal.AutomationLibrary.take_high_res_screenshot(1600, 900, str(OUT / 'TitleShadows_Final.png'), delay=0.0)
        elif task and task.is_task_done():
            assert report['sample_count'] > 30, 'Insufficient animation coverage samples'
            assert (OUT / 'TitleShadows_Final.png').exists(), 'Missing viewport screenshot'
            report['status'] = 'passed'
    except Exception:
        report['status'] = 'failed'
        report['error'] = traceback.format_exc()
    finally:
        busy = False
        if report['status'] != 'running':
            (OUT / 'verification.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
            unreal.log('TITLE_SHADOW_VERIFY ' + json.dumps(report))
            unreal.unregister_slate_post_tick_callback(handle)
            levels.editor_request_end_play()
            unreal.SystemLibrary.quit_editor()

handle = unreal.register_slate_post_tick_callback(tick)
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
