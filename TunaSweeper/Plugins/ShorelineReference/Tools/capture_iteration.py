"""Read-only visual iteration capture. Run in a dedicated UE5.7 D3D12 process.

-ShorelineRun=<label> selects a label, never an output path. Every run creates
a new directory and each view preserves an unprocessed yyyymmdd-hhmm.png.
"""
from datetime import datetime
import hashlib
import json
from pathlib import Path
import re
import sys
import time
import traceback
import unreal as u
sys.path.insert(0, str(Path(__file__).resolve().parent))
from png_integrity import finalize_exported_png

command = u.SystemLibrary.get_command_line()
if 'ExecutePythonScript=' not in command:
    raise RuntimeError('This capture exits its dedicated process; do not run in an interactive editor.')
match = re.search(r'-ShorelineRun=([A-Za-z0-9_-]+)', command)
label = match.group(1) if match else 'review'
ROOT = Path(u.Paths.project_dir()).resolve() / 'Plugins/ShorelineReference'
OUT = ROOT / 'Resources/Iterations' / (datetime.now().strftime('%Y%m%d-%H%M%S') + '-' + label)
OUT.mkdir(parents=True, exist_ok=False)
SAVED = Path(u.Paths.project_saved_dir()).resolve() / 'ShorelineVisualIteration'
SAVED.mkdir(parents=True, exist_ok=True)
before = {str(p): hashlib.sha256(p.read_bytes()).hexdigest()
          for p in (ROOT / 'Content').rglob('*') if p.is_file()}
report = {'schema_version': 2, 'status': 'running', 'label': label, 'started': datetime.now().isoformat(),
          'renderer': 'UE5.7 D3D12 SM6 SceneCapture2D', 'captures': [],
          'shader_hash': hashlib.sha256((ROOT / 'Resources/Shaders/CalmShoreline.hlsl').read_bytes()).hexdigest()}
log_match = re.search(r'-abslog=(?:"([^"]+)"|(\S+))', command, re.IGNORECASE)
if log_match:
    report['log_file'] = log_match.group(1) or log_match.group(2)
(OUT / 'shader_source.hlsl').write_bytes((ROOT / 'Resources/Shaders/CalmShoreline.hlsl').read_bytes())
BASE = '/ShorelineReference'
try:
    world = u.EditorLoadingAndSavingUtils.load_map(BASE + '/Maps/L_ShorelineReferenceIsland')
    assert world
    es = u.get_editor_subsystem(u.EditorActorSubsystem)
    actors = {a.get_actor_label(): a for a in es.get_all_level_actors()}
    water = actors['SR_Water'].static_mesh_component
    water_scale = actors['SR_Water'].get_actor_scale3d()
    report['water_actor_scale'] = [water_scale.x, water_scale.y, water_scale.z]
    material = u.load_asset(BASE + '/Materials/M_ShorelineReference')
    lib = u.MaterialEditingLibrary
    rgb = lib.get_material_property_input_node(material, u.MaterialProperty.MP_EMISSIVE_COLOR)
    custom = lib.get_inputs_for_material_expression(material, rgb)[0]
    assert custom.get_editor_property('code') == (ROOT / 'Resources/Shaders/CalmShoreline.hlsl').read_text(encoding='utf-8')
    parameter_nodes = lib.get_inputs_for_material_expression(material, custom)
    report['parameters'] = {str(n.get_editor_property('parameter_name')): n.get_editor_property('default_value')
        for n in parameter_nodes if isinstance(n, u.MaterialExpressionScalarParameter)}
    report['colors'] = {}
    for node in parameter_nodes:
        if not isinstance(node, u.MaterialExpressionVectorParameter):
            continue
        name = node.get_editor_property('parameter_name')
        value = node.get_editor_property('default_value')
        report['colors'][str(name)] = [value.r, value.g, value.b, value.a]
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
    for console in ['r.Streaming.FullyLoadUsedTextures 1', 't.IdleWhenNotForeground 0', 'Slate.bAllowThrottling 0',
                    'r.AntiAliasingMethod 4', 'Editor.AsyncAssetCompilationFinishAll']:
        u.SystemLibrary.execute_console_command(world, console)
    # Each view has its own directory, including repeated fixed times and A/Bs.
    stages = [
        ('wide', 1.0, 'wide', {}), ('close', 1.0, 'close', {}),
        ('close-repeat', 1.0, 'close', {}), ('close-t3', 3.0, 'close', {}),
        ('close-detail-off', 1.0, 'close', {'SurfaceDetailStrength': 0}),
        ('close-caustics-off', 1.0, 'close', {'CausticsStrength': 0}),
        ('close-glint-off', 1.0, 'close', {'GlintStrength': 0}),
        ('low', 1.0, 'low', {}), ('surface', 1.0, 'surface', {}),
        ('surface-off', 1.0, 'surface', {'SurfaceDetailStrength': 0}),
        ('mask', 1.0, 'mask', {}),
    ]
    defaults = {name: report['parameters'].get(name, 0.0)
                for name in ['SurfaceDetailStrength', 'CausticsStrength', 'GlintStrength']}
    started = time.monotonic()
    ticks = 0
    stage = -1
    stage_ticks = 0
    busy = False

    def configure(item):
        name, shader_time, kind, overrides = item
        for parameter, value in defaults.items():
            dynamic.set_scalar_parameter_value(parameter, value)
        for parameter, value in overrides.items():
            dynamic.set_scalar_parameter_value(parameter, value)
        dynamic.set_scalar_parameter_value('PreviewTime', shader_time)
        dynamic.set_scalar_parameter_value('DebugMask', 1 if kind == 'mask' else 0)
        camera.clear_hidden_components()
        if kind == 'mask':
            camera.hide_actor_components(actors['SR_Island'])
        camera.set_editor_property('texture_target', mask_target if kind == 'mask' else target)
        camera.set_editor_property('projection_type', u.CameraProjectionMode.ORTHOGRAPHIC if kind == 'mask' else u.CameraProjectionMode.PERSPECTIVE)
        location, aim = {
            'wide': ((0, -5800, 4400), (0, 0, 0)),
            'close': ((-850, -2400, 1000), (-250, -1600, 0)),
            'low': ((-700, -3300, 550), (0, -1300, 0)),
            'surface': ((-650, -3500, 1100), (-150, -3100, 0)),
            'mask': ((0, 0, 6000), (0, 0, 0)),
        }[kind]
        camera.set_editor_property('ortho_width', 8192.0)
        capture.set_actor_location(u.Vector(*location), False, True)
        capture.set_actor_rotation(u.MathLibrary.find_look_at_rotation(u.Vector(*location), u.Vector(*aim)), False)
        return location, aim

    def finish(error=None):
        report['saved_assets_unchanged'] = all(hashlib.sha256(Path(p).read_bytes()).hexdigest() == h for p,h in before.items())
        report['status'] = 'failed' if error or not report['saved_assets_unchanged'] else 'captured'
        report['wall_seconds'] = round(time.monotonic() - started, 3)
        if error:
            report['error'] = error
            u.log_error(error)
        (OUT / 'manifest.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
        (SAVED / 'latest_iteration.json').write_text(json.dumps({'directory': str(OUT), 'status': report['status']}), encoding='utf-8')
        u.log('SHORELINE_ITERATION_' + report['status'].upper() + ' ' + str(OUT))
        u.unregister_slate_post_tick_callback(handle)
        u.EditorPythonScripting.set_keep_python_script_alive(False)
        u.SystemLibrary.quit_editor()

    def tick(dt):
        global ticks, stage, stage_ticks, busy
        if busy:
            return
        busy = True
        try:
            ticks += 1
            assert time.monotonic() - started < 300, 'Capture timeout'
            if ticks < 100:
                if ticks % 4 == 0:
                    configure(stages[0])
                    camera.capture_scene()
                return
            if stage < 0:
                stage = 0
                configure(stages[stage])
            stage_ticks += 1
            camera.capture_scene()
            if stage_ticks < 32:
                return
            name, shader_time, kind, overrides = stages[stage]
            folder = OUT / name
            folder.mkdir()
            filename = datetime.now().strftime('%Y%m%d-%H%M') + '.png'
            u.RenderingLibrary.export_render_target(world, mask_target if kind == 'mask' else target, str(folder), filename)
            path = folder / filename
            assert path.is_file() and path.stat().st_size > 100
            padding = finalize_exported_png(path)
            location, rotation = capture.get_actor_location(), capture.get_actor_rotation()
            report['captures'].append({'view': name, 'shader_time': shader_time, 'overrides': overrides,
                'camera_location': [location.x, location.y, location.z],
                'camera_rotation': [rotation.pitch, rotation.yaw, rotation.roll],
                'file': path.relative_to(OUT).as_posix(), 'sha256': hashlib.sha256(path.read_bytes()).hexdigest(),
                'export_padding_removed_bytes': padding})
            (OUT / 'manifest.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
            stage += 1
            stage_ticks = 0
            if stage == len(stages):
                finish()
            else:
                configure(stages[stage])
        except Exception:
            finish(traceback.format_exc())
        finally:
            busy = False

    u.EditorPythonScripting.set_keep_python_script_alive(True)
    handle = u.register_slate_post_tick_callback(tick)
except Exception:
    report.update(status='failed', error=traceback.format_exc())
    (OUT / 'manifest.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    u.log_error(report['error'])
    u.SystemLibrary.quit_editor()
