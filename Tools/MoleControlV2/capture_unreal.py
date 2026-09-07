"""Read-only UE 5.7 rendered-viewport QA in a dedicated editor process.

Launch only after importing the MoleControlV2 assets. Example arguments for a
new UnrealEditor.exe process (not a commandlet and never with -NullRHI):

    <uproject> -ExecutePythonScript=<absolute path to this file>
    -unattended -NoSourceControl -nosplash -NoSound -NoLoadingScreen
    -windowed -ResX=1600 -ResY=900
    -ExecCmds="t.IdleWhenNotForeground 0,Slate.bAllowThrottling 0"

The script only loads a saved map, changes this process's transient viewport,
and writes PNG/JSON evidence. It never spawns, saves, or edits project assets.
It exits only the dedicated process in which this script is running. An
interactive console invocation is rejected before changing the viewport.

Optional environment variables:
  MOLE_V2_CAPTURE_LEVEL       Saved /Game/Environment/MoleControlV2/Maps/... map
  MOLE_V2_CAPTURE_CAMERAS     Comma-separated actor labels; default both views
  MOLE_V2_CAPTURE_OUTPUT      Output directory; default SourceArt/.../Previews
  MOLE_V2_CAPTURE_WARMUP      Minimum rendered warmup seconds per camera (15)
  MOLE_V2_CAPTURE_FRAMES      Minimum Slate warmup ticks per camera (90)
  MOLE_V2_CAPTURE_TIMEOUT     Overall callback timeout seconds (600)

API provenance, checked against the installed UE 5.7 source:
  Engine/Source/Developer/FunctionalTesting/Public/AutomationBlueprintFunctionLibrary.h
  Engine/Source/Developer/FunctionalTesting/Private/AutomationBlueprintFunctionLibrary.cpp
    TakeHighResScreenshot:1223ff (camera pilot, shader/streaming flush, async task)
    FinishLoadingBeforeScreenshot:702 (FinishAllCompilation, mip streaming)
  Engine/Source/Editor/LevelEditor/Public/LevelEditorSubsystem.h
  Engine/Plugins/Experimental/PythonScriptPlugin/Source/PythonScriptPlugin/Private/
    EditorUtilities/EditorPythonScriptingLibrary.h and PySlate.cpp
  https://dev.epicgames.com/documentation/en-us/unreal-engine/python-api/class/AutomationLibrary?application_version=5.7
  https://dev.epicgames.com/documentation/en-us/unreal-engine/python-api/class/LevelEditorSubsystem?application_version=5.7
  https://dev.epicgames.com/documentation/en-us/unreal-engine/python-api/class/EditorPythonScripting?application_version=5.7
"""
import hashlib
import json
import os
from pathlib import Path
import struct
import time
import traceback
from datetime import datetime, timezone

import unreal


ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / 'TunaSweeper/SourceArt/Environment/MoleControlV2'
MAP_ROOT = '/Game/Environment/MoleControlV2/Maps/'
DEFAULT_LEVEL = MAP_ROOT + 'L_MoleControlV2_StoneVault'
DEFAULT_CAMERAS = 'MoleV2_ReviewCamera,MoleV2_InteriorCamera'
WIDTH, HEIGHT = 1600, 900


def package_hashes():
    """Read only this collection's content files to detect accidental saves."""
    folder = Path(unreal.Paths.project_content_dir()).resolve() / 'Environment/MoleControlV2'
    return {
        p.relative_to(folder).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest()
        for p in sorted(folder.rglob('*'))
        if p.is_file() and p.suffix.lower() in {'.umap', '.uasset', '.uexp', '.ubulk'}
    }


def png_evidence(path, previous_mtime):
    """Reject a stale, partial, or wrong-size PNG without any imaging dependency."""
    if not path.is_file():
        return None
    stat = path.stat()
    if previous_mtime is not None and stat.st_mtime_ns <= previous_mtime:
        return None
    data = path.read_bytes()
    if len(data) < 33 or data[:8] != b'\x89PNG\r\n\x1a\n':
        return None
    if data[12:16] != b'IHDR' or data[-8:] != b'IEND\xaeB`\x82':
        return None
    width, height = struct.unpack('>II', data[16:24])
    if (width, height) != (WIDTH, HEIGHT):
        raise RuntimeError(f'Expected {WIDTH}x{HEIGHT} PNG; got {width}x{height}: {path}')
    return {
        'path': str(path), 'width': width, 'height': height,
        'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest(),
    }


class FacilityCapture:
    def __init__(self):
        self.started = time.monotonic()
        self.level_path = os.environ.get('MOLE_V2_CAPTURE_LEVEL', DEFAULT_LEVEL)
        if not self.level_path.startswith(MAP_ROOT) or '..' in self.level_path:
            raise ValueError(f'Capture map must be inside {MAP_ROOT}')
        self.labels = [s.strip() for s in os.environ.get(
            'MOLE_V2_CAPTURE_CAMERAS', DEFAULT_CAMERAS).split(',') if s.strip()]
        allowed = {'MoleV2_ReviewCamera', 'MoleV2_InteriorCamera'}
        if not self.labels or len(set(self.labels)) != len(self.labels) or not set(self.labels) <= allowed:
            raise ValueError('Select one or both supported Facility camera labels, without duplicates')
        self.output = Path(os.environ.get('MOLE_V2_CAPTURE_OUTPUT', str(SOURCE / 'Previews'))).resolve()
        self.output.mkdir(parents=True, exist_ok=True)
        self.warmup_seconds = max(5.0, float(os.environ.get('MOLE_V2_CAPTURE_WARMUP', '15')))
        self.warmup_frames = max(30, int(os.environ.get('MOLE_V2_CAPTURE_FRAMES', '90')))
        self.timeout = max(60.0, float(os.environ.get('MOLE_V2_CAPTURE_TIMEOUT', '600')))
        self.level_editor = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
        self.actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
        self.handle = None
        self.task = None
        self.camera = None
        self.camera_index = 0
        self.state = 'wait_viewport'
        self.stage_started = self.started
        self.frames = 0
        self.previous_mtime = None
        self.pending_path = None
        self.finished = False
        self.baseline = package_hashes()
        if not self.baseline:
            raise RuntimeError('MoleControlV2 content does not exist in this project')
        self.report = {
            'passed': False,
            'started_at_utc': datetime.now(timezone.utc).isoformat(),
            'engine_version': unreal.SystemLibrary.get_engine_version(),
            'level': self.level_path,
            'requested_cameras': self.labels,
            'resolution': [WIDTH, HEIGHT],
            'warmup_seconds': self.warmup_seconds,
            'warmup_frames': self.warmup_frames,
            'captures': [],
            'validation': 'rendered editor viewport PNG; visual acceptance is separate',
        }

    def transition(self, state):
        self.state = state
        self.stage_started = time.monotonic()
        self.frames = 0
        unreal.log(f'MOLE_V2_CAPTURE_STAGE {state}')

    def start(self):
        unreal.EditorPythonScripting.set_keep_python_script_alive(True)
        self.handle = unreal.register_slate_post_tick_callback(self.tick)
        unreal.log(f'MOLE_V2_CAPTURE_STARTED {self.level_path}')

    def tick(self, delta_seconds):
        # UE asset flushes pump Slate; nested ticks must never re-pilot a camera.
        if getattr(self,'in_tick',False):
            return
        self.in_tick=True
        try:
            self._tick(delta_seconds)
        finally:
            self.in_tick=False

    def _tick(self, delta_seconds):
        if self.finished:
            return
        try:
            if time.monotonic() - self.started > self.timeout:
                raise TimeoutError(f'Capture timed out during {self.state}')
            self.frames += 1
            if self.state == 'wait_viewport':
                # The native screenshot function dereferences the first active
                # level viewport. Do not call it before a viewport exists.
                if not self.level_editor.get_viewport_config_keys():
                    return
                if str(self.level_editor.get_active_viewport_config_key()) in {'', 'None'}:
                    return
                if self.frames < 3:
                    return
                if not self.level_editor.load_level(self.level_path):
                    raise RuntimeError(f'Unable to load {self.level_path}')
                self.transition('map_settle')
            elif self.state == 'map_settle':
                if self.frames < 5:
                    return
                self.select_camera()
            elif self.state == 'warmup':
                self.level_editor.editor_invalidate_viewports()
                if self.frames >= self.warmup_frames and time.monotonic() - self.stage_started >= self.warmup_seconds:
                    self.request_screenshot()
            elif self.state == 'capture_pending':
                self.level_editor.editor_invalidate_viewports()
                if self.task is None or not self.task.is_valid_task():
                    raise RuntimeError('UE did not create a valid viewport screenshot task')
                if self.task.is_task_done():
                    self.transition('file_pending')
            elif self.state == 'file_pending':
                evidence = png_evidence(self.pending_path, self.previous_mtime)
                if evidence is None:
                    return
                evidence['camera_label'] = self.labels[self.camera_index]
                self.report['captures'].append(evidence)
                unreal.log('MOLE_V2_CAPTURE_IMAGE_READY ' + json.dumps(evidence))
                self.task = None
                self.camera_index += 1
                if self.camera_index < len(self.labels):
                    self.select_camera()
                else:
                    self.finish()
        except Exception:
            self.finish(traceback.format_exc())

    def select_camera(self):
        label = self.labels[self.camera_index]
        found = [a for a in self.actors.get_all_level_actors() if a.get_actor_label() == label]
        if len(found) != 1 or not isinstance(found[0], unreal.CameraActor):
            raise RuntimeError(f'Expected one saved CameraActor labelled {label}; found {len(found)}')
        self.camera = found[0]
        def vector(value):return [value.x,value.y,value.z]
        def rotation(value):return [value.pitch,value.yaw,value.roll]
        self.report.setdefault('camera_transforms',[]).append({
            'label':label,'actor_location':vector(self.camera.get_actor_location()),
            'actor_rotation':rotation(self.camera.get_actor_rotation()),
            'component_location':vector(self.camera.camera_component.get_world_location()),
            'component_rotation':rotation(self.camera.camera_component.get_world_rotation()),
            'fov':self.camera.camera_component.get_editor_property('field_of_view')})
        self.level_editor.editor_set_viewport_realtime(True)
        self.level_editor.editor_set_game_view(True)
        self.level_editor.pilot_level_actor(self.camera)
        self.level_editor.set_exact_camera_view(True)
        unreal.AutomationLibrary.set_editor_active_viewport_view_mode(unreal.ViewModeIndex.VMI_LIT)
        # This official blocking flush completes pending shader/asset compilation
        # and requests all texture mips; temporal lighting then settles by ticks.
        unreal.AutomationLibrary.finish_loading_before_screenshot()
        unreal.log(f'MOLE_V2_CAPTURE_CAMERA {label}')
        self.transition('warmup')

    def request_screenshot(self):
        label = self.labels[self.camera_index]
        suffix = label.removeprefix('MoleV2_').removesuffix('Camera')
        map_name = self.level_path.rsplit('/', 1)[-1].removeprefix('L_')
        self.pending_path = self.output / f'UE_{map_name}_{suffix}.png'
        self.previous_mtime = self.pending_path.stat().st_mtime_ns if self.pending_path.exists() else None
        # Keep the returned UObject referenced until it signals completion.
        self.task = unreal.AutomationLibrary.take_high_res_screenshot(
            WIDTH, HEIGHT, str(self.pending_path), camera=self.camera,
            mask_enabled=False, capture_hdr=False, comparison_notes=label,
            delay=1.0, force_game_view=True)
        self.transition('capture_pending')

    def finish(self, error=None):
        if self.finished:
            return
        self.finished = True
        if self.handle is not None:
            unreal.unregister_slate_post_tick_callback(self.handle)
            self.handle = None
        try:
            after = package_hashes()
            changed = sorted(set(self.baseline) ^ set(after) | {
                key for key in self.baseline.keys() & after.keys()
                if self.baseline[key] != after[key]})
            self.report['asset_packages_checked'] = len(self.baseline)
            self.report['asset_packages_unchanged'] = not changed
            self.report['changed_asset_packages'] = changed
            self.report['elapsed_seconds'] = round(time.monotonic() - self.started, 3)
            self.report['passed'] = error is None and not changed and len(self.report['captures']) == len(self.labels)
            if error:
                self.report['error'] = error
            report_path = self.output / 'unreal_viewport_capture.json'
            report_path.write_text(json.dumps(self.report, indent=2), encoding='utf-8')
            (unreal.log if self.report['passed'] else unreal.log_error)(
                ('MOLE_V2_VIEWPORT_CAPTURE_PASSED ' if self.report['passed'] else 'MOLE_V2_VIEWPORT_CAPTURE_FAILED ')
                + str(report_path))
            if error:
                unreal.log_error(error)
        except Exception:
            unreal.log_error('MOLE_V2_VIEWPORT_CAPTURE_REPORT_FAILED ' + traceback.format_exc())
        finally:
            unreal.EditorPythonScripting.set_keep_python_script_alive(False)
            unreal.SystemLibrary.quit_editor()


def main():
    command_line = unreal.SystemLibrary.get_command_line().lower()
    if (not unreal.SystemLibrary.is_unattended()
            or '-executepythonscript' not in command_line
            or Path(__file__).name.lower() not in command_line):
        raise RuntimeError('Capture requires a new unattended -ExecutePythonScript=capture_unreal.py editor process')
    if '-nullrhi' in command_line or '-run=' in command_line:
        raise RuntimeError('Capture requires a rendered Slate editor, not NullRHI or a commandlet')
    global CAPTURE
    try:
        CAPTURE = FacilityCapture()
        CAPTURE.start()
    except Exception:
        unreal.log_error('MOLE_V2_VIEWPORT_CAPTURE_START_FAILED ' + traceback.format_exc())
        unreal.EditorPythonScripting.set_keep_python_script_alive(False)
        unreal.SystemLibrary.quit_editor()
        raise


if __name__ == '__main__':
    main()
