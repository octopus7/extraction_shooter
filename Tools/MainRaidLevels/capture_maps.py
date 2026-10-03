"""Rendered, read-only overview capture for the three Main raid region maps."""

from pathlib import Path
import hashlib
import json
import struct
import time
import traceback

import unreal


ROOT = Path(__file__).resolve().parents[2]
OUTPUT = ROOT / "TunaSweeper/Saved/MainRaidLevels/Previews"
REPORT = OUTPUT / "capture.json"
WIDTH, HEIGHT = 1600, 900
MAPS = [
    ("RaidForest", "/Game/MainRaid/RaidForest", "TS_Forest_ReviewCamera"),
    ("RaidVillage", "/Game/MainRaid/RaidVillage", "TS_Village_ReviewCamera"),
    ("RaidPlains", "/Game/MainRaid/RaidPlains", "TS_Plains_ReviewCamera"),
]


def package_hashes():
    folder = ROOT / "TunaSweeper/Content/MainRaid"
    return {
        path.name: hashlib.sha256(path.read_bytes()).hexdigest()
        for path in sorted(folder.glob("Raid*.umap"))
    }


def png_info(path, previous_mtime):
    if not path.is_file() or (previous_mtime is not None and path.stat().st_mtime_ns <= previous_mtime):
        return None
    data = path.read_bytes()
    if len(data) < 33 or data[:8] != b"\x89PNG\r\n\x1a\n" or data[12:16] != b"IHDR":
        return None
    width, height = struct.unpack(">II", data[16:24])
    if (width, height) != (WIDTH, HEIGHT):
        raise RuntimeError(f"Wrong capture dimensions {width}x{height}: {path}")
    return {"path": str(path), "width": width, "height": height, "bytes": len(data), "sha256": hashlib.sha256(data).hexdigest()}


class CaptureMaps:
    def __init__(self):
        self.started = time.monotonic()
        self.levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
        self.actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
        self.baseline = package_hashes()
        self.index = 0
        self.state = "wait_viewport"
        self.frames = 0
        self.stage_started = self.started
        self.handle = None
        self.task = None
        self.pending_path = None
        self.previous_mtime = None
        self.report = {"passed": False, "engine_version": unreal.SystemLibrary.get_engine_version(), "captures": []}
        OUTPUT.mkdir(parents=True, exist_ok=True)

    def transition(self, state):
        self.state = state
        self.frames = 0
        self.stage_started = time.monotonic()
        unreal.log(f"MAIN_RAID_CAPTURE_STAGE {state}")

    def start(self):
        unreal.EditorPythonScripting.set_keep_python_script_alive(True)
        self.handle = unreal.register_slate_post_tick_callback(self.tick)

    def tick(self, _delta):
        if getattr(self, "busy", False):
            return
        self.busy = True
        try:
            self._tick()
        except Exception:
            self.finish(traceback.format_exc())
        finally:
            self.busy = False

    def _tick(self):
        if time.monotonic() - self.started > 900:
            raise TimeoutError(f"Capture timed out during {self.state}")
        self.frames += 1
        if self.state == "wait_viewport":
            if not self.levels.get_viewport_config_keys() or self.frames < 3:
                return
            self.load_map()
        elif self.state == "settle":
            if self.frames < 10:
                return
            self.select_camera()
        elif self.state == "warmup":
            self.levels.editor_invalidate_viewports()
            if self.frames >= 45 and time.monotonic() - self.stage_started >= 5.0:
                self.request_capture()
        elif self.state == "capture_pending":
            self.levels.editor_invalidate_viewports()
            if not self.task or not self.task.is_valid_task():
                raise RuntimeError("Unreal did not create a valid screenshot task")
            if self.task.is_task_done():
                self.transition("file_pending")
        elif self.state == "file_pending":
            evidence = png_info(self.pending_path, self.previous_mtime)
            if evidence is None:
                return
            evidence["map"] = MAPS[self.index][1]
            self.report["captures"].append(evidence)
            self.index += 1
            self.task = None
            if self.index >= len(MAPS):
                self.finish()
            else:
                self.load_map()

    def load_map(self):
        _name, path, _camera = MAPS[self.index]
        if not self.levels.load_level(path):
            raise RuntimeError(f"Unable to load {path}")
        self.transition("settle")

    def select_camera(self):
        _name, _path, label = MAPS[self.index]
        cameras = [actor for actor in self.actors.get_all_level_actors() if actor.get_actor_label() == label]
        if len(cameras) != 1 or not isinstance(cameras[0], unreal.CameraActor):
            raise RuntimeError(f"Expected one saved CameraActor labelled {label}; found {len(cameras)}")
        self.camera = cameras[0]
        self.levels.editor_set_game_view(True)
        self.levels.pilot_level_actor(self.camera)
        self.levels.set_exact_camera_view(True)
        unreal.AutomationLibrary.set_editor_active_viewport_view_mode(unreal.ViewModeIndex.VMI_LIT)
        unreal.AutomationLibrary.finish_loading_before_screenshot()
        self.transition("warmup")

    def request_capture(self):
        name, _path, label = MAPS[self.index]
        self.pending_path = OUTPUT / f"{name}.png"
        self.previous_mtime = self.pending_path.stat().st_mtime_ns if self.pending_path.exists() else None
        self.task = unreal.AutomationLibrary.take_high_res_screenshot(
            WIDTH, HEIGHT, str(self.pending_path), camera=self.camera,
            mask_enabled=False, capture_hdr=False, comparison_notes=label,
            delay=1.0, force_game_view=True,
        )
        self.transition("capture_pending")

    def finish(self, error=None):
        if self.handle is not None:
            unreal.unregister_slate_post_tick_callback(self.handle)
            self.handle = None
        after = package_hashes()
        changed = sorted(key for key in set(self.baseline) | set(after) if self.baseline.get(key) != after.get(key))
        self.report["asset_packages_unchanged"] = not changed
        self.report["changed_asset_packages"] = changed
        self.report["elapsed_seconds"] = round(time.monotonic() - self.started, 3)
        if error:
            self.report["error"] = error
        self.report["passed"] = error is None and not changed and len(self.report["captures"]) == len(MAPS)
        REPORT.write_text(json.dumps(self.report, indent=2), encoding="utf-8")
        (unreal.log if self.report["passed"] else unreal.log_error)(
            ("MAIN_RAID_CAPTURE_PASSED " if self.report["passed"] else "MAIN_RAID_CAPTURE_FAILED ") + str(REPORT)
        )
        if error:
            unreal.log_error(error)
        unreal.EditorPythonScripting.set_keep_python_script_alive(False)
        unreal.SystemLibrary.quit_editor()


def main():
    command_line = unreal.SystemLibrary.get_command_line().lower()
    if "-nullrhi" in command_line or "-run=" in command_line:
        raise RuntimeError("Capture requires a rendered full editor process")
    capture = CaptureMaps()
    capture.start()
    globals()["MAIN_RAID_CAPTURE"] = capture


if __name__ == "__main__":
    try:
        main()
    except Exception:
        OUTPUT.mkdir(parents=True, exist_ok=True)
        REPORT.write_text(json.dumps({"passed": False, "error": traceback.format_exc()}, indent=2), encoding="utf-8")
        unreal.log_error(traceback.format_exc())
        unreal.EditorPythonScripting.set_keep_python_script_alive(False)
        unreal.SystemLibrary.quit_editor()
        raise
