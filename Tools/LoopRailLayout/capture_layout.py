"""Read-only overview and occupied-platform captures of the village railway."""

from pathlib import Path
import importlib.util
import math
import sys

import unreal


ROOT = Path(__file__).resolve().parents[2]
sys.dont_write_bytecode = True
spec = importlib.util.spec_from_file_location("main_raid_capture", ROOT / "Tools/MainRaidLevels/capture_maps.py")
capture = importlib.util.module_from_spec(spec)
spec.loader.exec_module(capture)
capture.OUTPUT = ROOT / "TunaSweeper/Saved/LoopRailLayout/Previews"
capture.REPORT = capture.OUTPUT / "capture.json"
capture.MAPS = [("Overview", "/Game/MainRaid/RaidVillage", "TS_Village_ReviewCamera")] + [
    (f"Station_{index:02}", "/Game/MainRaid/RaidVillage", f"TS_Village_Station_{index:02}")
    for index in range(1, 4)
]


class CaptureRailway(capture.CaptureMaps):
    def select_camera(self):
        actors = self.actors.get_all_level_actors()
        self.camera = self.actors.spawn_actor_from_class(unreal.CameraActor, unreal.Vector(0, 0, 30000))
        component = self.camera.get_component_by_class(unreal.CameraComponent)
        component.set_editor_property("aspect_ratio", capture.WIDTH / capture.HEIGHT)
        component.set_editor_property("projection_mode", unreal.CameraProjectionMode.ORTHOGRAPHIC)
        if self.index == 0:
            component.set_editor_property("ortho_width", 46000.0)
            self.camera.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(
                self.camera.get_actor_location(), unreal.Vector(0, 0, 0)), False)
        else:
            station = next(actor for actor in actors if actor.get_actor_label() == capture.MAPS[self.index][2])
            train = next(actor for actor in actors if isinstance(actor, unreal.LoopRailTrain))
            train.set_editor_property("start_distance", station.get_stop_distance())
            train.reset_train()
            location = station.get_actor_location()
            yaw = math.radians(station.get_actor_rotation().yaw)
            forward = unreal.Vector(math.cos(yaw), math.sin(yaw), 0)
            right = unreal.Vector(-math.sin(yaw), math.cos(yaw), 0)
            center = location - forward * 1450.0
            camera_location = center - right * 4500.0 + unreal.Vector(0, 0, 6000)
            self.camera.set_actor_location(camera_location, False, False)
            self.camera.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(camera_location, center), False)
            component.set_editor_property("ortho_width", 7000.0)
        self.levels.editor_set_game_view(True)
        self.levels.pilot_level_actor(self.camera)
        self.levels.set_exact_camera_view(True)
        unreal.AutomationLibrary.set_editor_active_viewport_view_mode(unreal.ViewModeIndex.VMI_LIT)
        unreal.AutomationLibrary.finish_loading_before_screenshot()
        self.transition("warmup")

    def load_map(self):
        if self.index == 0:
            super().load_map()
        else:
            # Reuse the unsaved preview world; the source package is never saved.
            self.levels.eject_pilot_level_actor()
            self.actors.destroy_actor(self.camera)
            self.transition("settle")


capture.CaptureMaps = CaptureRailway
capture.main()
