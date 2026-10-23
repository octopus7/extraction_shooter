"""UE 5.7 D3D12 review capture. Changes transient objects only; never saves assets.

Run with -ExecutePythonScript, -RenderOffscreen, -dx12 and -sm6.
Outputs go to Saved/ShorelineReferenceWork/Review; fixed shader time is deliberate.
"""
import hashlib
import json
import math
from pathlib import Path
import sys
import time
import traceback
import uuid
import unreal as u
sys.path.insert(0, str(Path(__file__).resolve().parent))
from png_integrity import finalize_exported_png

if 'ExecutePythonScript=' not in u.SystemLibrary.get_command_line():
    raise RuntimeError('Run this tool in a dedicated -ExecutePythonScript process; it exits that process.')

ROOT = Path(u.Paths.project_dir()).resolve() / 'Plugins/ShorelineReference'
OUT = Path(u.Paths.project_saved_dir()).resolve() / 'ShorelineReferenceWork/Review'
OUT.mkdir(parents=True, exist_ok=True)
BASE = '/ShorelineReference'

def export_frame(world, render_target, name):
    # UE5.7 can export allocation padding after IEND, even to a fresh file.
    # Preserve all encoded pixel chunks, finalize the container, then replace.
    temporary = OUT / ('Capture_' + uuid.uuid4().hex + '.png')
    u.RenderingLibrary.export_render_target(world, render_target, str(OUT), temporary.name)
    assert temporary.is_file() and temporary.stat().st_size > 100, 'Frame export failed: ' + name
    finalize_exported_png(temporary)
    temporary.replace(OUT / name)

before = {str(p): hashlib.sha256(p.read_bytes()).hexdigest()
          for p in (ROOT / 'Content').rglob('*') if p.is_file()}
report = {'status': 'running', 'renderer': 'UE5.7 D3D12 SM6 SceneCapture2D',
          'sequence_seconds': 20, 'sequence_fps': 12, 'captures': []}
try:
    world = u.EditorLoadingAndSavingUtils.load_map(BASE + '/Maps/L_ShorelineReferenceIsland')
    assert world
    es = u.get_editor_subsystem(u.EditorActorSubsystem)
    actors = {a.get_actor_label(): a for a in es.get_all_level_actors()}
    water = actors['SR_Water'].static_mesh_component
    tail_strength = u.MaterialEditingLibrary.get_material_default_scalar_parameter_value(u.load_asset(BASE + '/Materials/M_ShorelineReference'), 'TailBreakupStrength')
    dynamic = water.create_dynamic_material_instance(0)
    assert dynamic
    dynamic.set_scalar_parameter_value('FreezeTime', 1)
    capture = es.spawn_actor_from_class(u.SceneCapture2D, u.Vector(0, -5800, 4400))
    camera = capture.get_component_by_class(u.SceneCaptureComponent2D)
    for name, value in {'fov_angle': 55.0, 'capture_source': u.SceneCaptureSource.SCS_FINAL_COLOR_LDR,
        'capture_every_frame': False, 'always_persist_rendering_state': True}.items():
        camera.set_editor_property(name, value)
    settings = camera.get_editor_property('post_process_settings')
    for name, value in {'override_auto_exposure_method': True, 'auto_exposure_method': u.AutoExposureMethod.AEM_MANUAL,
        'override_auto_exposure_bias': True, 'auto_exposure_bias': 2.5,
        'override_auto_exposure_apply_physical_camera_exposure': True, 'auto_exposure_apply_physical_camera_exposure': False,
        'override_motion_blur_amount': True, 'motion_blur_amount': 0.0,
        'override_bloom_intensity': True, 'bloom_intensity': 0.0}.items():
        settings.set_editor_property(name, value)
    camera.set_editor_property('post_process_settings', settings)
    target = u.RenderingLibrary.create_render_target2d(world, 1280, 800, u.TextureRenderTargetFormat.RTF_RGBA8)
    mask_target = u.RenderingLibrary.create_render_target2d(world, 1024, 1024, u.TextureRenderTargetFormat.RTF_RGBA8)
    camera.set_editor_property('texture_target', target)
    for command in ['r.Streaming.FullyLoadUsedTextures 1', 't.IdleWhenNotForeground 0', 'Slate.bAllowThrottling 0', 'r.AntiAliasingMethod 4']:
        u.SystemLibrary.execute_console_command(world, command)

    def view(location, aim):
        capture.set_actor_location(u.Vector(*location), False, True)
        capture.set_actor_rotation(u.MathLibrary.find_look_at_rotation(u.Vector(*location), u.Vector(*aim)), False)

    stages = [('Wide.png', 0.0, 'wide'), ('Close_0.png', 0.0, 'close'),
              ('Close_1.png', 1.0, 'close'), ('Close_2.png', 2.0, 'close'),
              ('Mask_0.png', 0.0, 'mask'), ('Mask_1.png', 1.0, 'mask'),
              ('Mask_1_repeat.png', 1.0, 'mask'), ('Mask_disabled.png', 1.0, 'disabled'),
              ('Mask_1_breakup_off.png', 1.0, 'baseline'), ('Close_1_breakup_off.png', 1.0, 'close_baseline')]
    started = time.monotonic()
    started_utc = time.time()
    ticks = 0
    stage = -1
    stage_ticks = 0
    sequence_frame = -1
    busy = False

    def configure(kind, shader_time):
        dynamic.set_scalar_parameter_value('PreviewTime', shader_time)
        dynamic.set_scalar_parameter_value('DebugMask', 1 if kind in ['mask', 'disabled', 'baseline'] else 0)
        dynamic.set_scalar_parameter_value('TailBreakupStrength', 0 if kind in ['baseline', 'close_baseline'] else tail_strength)
        dynamic.set_scalar_parameter_value('FoamStrength', 0 if kind == 'disabled' else 1)
        is_mask = kind in ['mask', 'disabled', 'baseline']
        camera.clear_hidden_components()
        if is_mask:
            camera.hide_actor_components(actors['SR_Island'])
        camera.set_editor_property('texture_target', mask_target if is_mask else target)
        camera.set_editor_property('projection_type', u.CameraProjectionMode.ORTHOGRAPHIC if is_mask else u.CameraProjectionMode.PERSPECTIVE)
        if is_mask:
            camera.set_editor_property('ortho_width', 8192.0)
            view((0, 0, 6000), (0, 0, 0))
        elif kind == 'wide':
            view((0, -5800, 4400), (0, 0, 0))
        else:
            view((-850, -2400, 1000), (-250, -1600, 0))

    def finish(error=None):
        required = [s[0] for s in stages] + ['Motion_%04d.png' % i for i in range(240)]
        report['required_captures_exist'] = all((OUT / name).is_file() and (OUT / name).stat().st_size > 100 and (OUT / name).stat().st_mtime >= started_utc for name in required)
        report['saved_assets_unchanged'] = all(hashlib.sha256(Path(p).read_bytes()).hexdigest() == h for p, h in before.items())
        report['status'] = 'failed' if error or not report['saved_assets_unchanged'] or not report['required_captures_exist'] else 'captured_pending_pixel_validation'
        report['wall_seconds'] = round(time.monotonic() - started, 3)
        if error:
            report['error'] = error
            u.log_error(error)
        (OUT / 'capture_validation.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
        u.log('SHORELINE_RENDER_' + report['status'].upper())
        u.unregister_slate_post_tick_callback(handle)
        u.EditorPythonScripting.set_keep_python_script_alive(False)
        u.SystemLibrary.quit_editor()

    def tick(dt):
        global ticks, stage, stage_ticks, sequence_frame, busy
        if busy:
            return
        busy = True
        try:
            ticks += 1
            assert time.monotonic() - started < 360, 'Render timeout'
            if ticks < 120:
                if ticks % 4 == 0:
                    camera.capture_scene()
                return
            if stage < 0:
                stage = 0
                configure(stages[0][2], stages[0][1])
            if stage < len(stages):
                stage_ticks += 1
                camera.capture_scene()
                if stage_ticks >= 24:
                    name, shader_time, kind = stages[stage]
                    export_frame(world, mask_target if kind in ['mask', 'disabled', 'baseline'] else target, name)
                    report['captures'].append({'name': name, 'shader_time': shader_time, 'view': kind})
                    stage += 1
                    stage_ticks = 0
                    if stage < len(stages):
                        configure(stages[stage][2], stages[stage][1])
                    else:
                        configure('close', 0)
                return
            sequence_frame += 1
            if sequence_frame >= 480:
                finish()
                return
            index = sequence_frame // 2
            shader_time = index / 12.0
            dynamic.set_scalar_parameter_value('PreviewTime', shader_time)
            if index >= 120:
                orbit = (index - 120) / 120.0
                view((-850 + 700 * orbit, -2400 - 250 * orbit, 1000 + 250 * orbit), (-250 + 300 * orbit, -1600, 0))
            camera.capture_scene()
            if sequence_frame % 2 == 1:
                export_frame(world, target, 'Motion_%04d.png' % index)
        except Exception:
            finish(traceback.format_exc())
        finally:
            busy = False

    u.EditorPythonScripting.set_keep_python_script_alive(True)
    handle = u.register_slate_post_tick_callback(tick)
except Exception:
    report.update(status='failed', error=traceback.format_exc())
    (OUT / 'capture_validation.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    u.log_error(report['error'])
    u.SystemLibrary.quit_editor()
