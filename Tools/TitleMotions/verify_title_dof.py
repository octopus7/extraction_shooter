"""Read-only saved material and main/submenu PIE focus check with screenshots."""
import json
import time
import traceback
from pathlib import Path
import unreal

OUT = Path(__file__).resolve().parents[2] / 'TunaSweeper/Saved/Automation/TitleDOF'
OUT.mkdir(parents=True, exist_ok=True)
report = {'status': 'running', 'samples': 0, 'max_focus_error_cm': 0.0}
mat = unreal.load_asset('/Game/UI/Title/M_TitleMatteLake')
assert mat.get_editor_property('translucency_pass') == unreal.MaterialTranslucencyPass.MTP_BEFORE_DOF
assert mat.get_editor_property('output_translucent_velocity')
world = unreal.EditorLoadingAndSavingUtils.load_map('/Game/Maps/IntroMap')
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
started = time.monotonic()
frame = 0
phase = 'main'
phase_start = 0
task = None
busy = False

def tick(delta):
    global frame, phase, phase_start, task, busy
    if busy:
        return
    busy = True
    frame += 1
    try:
        assert time.monotonic() - started < 180, 'Title DOF audit timed out'
        if frame == 10:
            levels.editor_request_begin_play()
        game = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
        if not game:
            return
        elapsed = unreal.GameplayStatics.get_time_seconds(game)
        char = unreal.GameplayStatics.get_all_actors_of_class(game, unreal.TunaSweeperTitlePresentationActor)[0]
        camera = char.get_component_by_class(unreal.CameraComponent)
        body = next(c for c in char.get_components_by_class(unreal.SkeletalMeshComponent) if c.get_name() == 'BodyMesh')
        settings = camera.get_editor_property('post_process_settings')
        if elapsed > 1:
            expected = max(1.0, unreal.MathLibrary.dot_vector_vector(body.get_socket_location('head') - camera.get_world_location(), camera.get_forward_vector()))
            error = abs(settings.get_editor_property('depth_of_field_focal_distance') - expected)
            report['max_focus_error_cm'] = max(report['max_focus_error_cm'], error)
            assert error < 5.0, ('Face focus failed to follow camera', error)
            assert settings.get_editor_property('override_depth_of_field_focal_distance')
            assert settings.get_editor_property('override_depth_of_field_fstop')
            assert abs(settings.get_editor_property('depth_of_field_fstop') - 2.8) < 0.01
            report['samples'] += 1
        if phase == 'main' and elapsed >= 8:
            report['main_focus_cm'] = settings.get_editor_property('depth_of_field_focal_distance')
            task = unreal.AutomationLibrary.take_high_res_screenshot(1600, 900, str(OUT / 'TitleDOF_Final.png'), delay=0.0)
            phase = 'main_capture'
        elif phase == 'main_capture' and task.is_task_done():
            char.set_main_menu_presentation_active(False)
            phase_start = elapsed
            phase = 'submenu'
        elif phase == 'submenu' and elapsed - phase_start >= 3:
            report['submenu_focus_cm'] = settings.get_editor_property('depth_of_field_focal_distance')
            task = unreal.AutomationLibrary.take_high_res_screenshot(1600, 900, str(OUT / 'TitleDOF_Submenu.png'), delay=0.0)
            phase = 'submenu_capture'
        elif phase == 'submenu_capture' and task.is_task_done():
            assert report['samples'] > 30
            assert (OUT / 'TitleDOF_Final.png').exists() and (OUT / 'TitleDOF_Submenu.png').exists()
            report['status'] = 'passed'
    except Exception:
        report['status'] = 'failed'
        report['error'] = traceback.format_exc()
    finally:
        busy = False
        if report['status'] != 'running':
            (OUT / 'verification.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
            unreal.log('TITLE_DOF_VERIFY ' + json.dumps(report))
            unreal.unregister_slate_post_tick_callback(handle)
            levels.editor_request_end_play()
            unreal.SystemLibrary.quit_editor()

handle = unreal.register_slate_post_tick_callback(tick)
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
