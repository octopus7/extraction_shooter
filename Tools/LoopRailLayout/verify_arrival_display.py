"""Rendered PIE checks for grounded, yaw-only station steles and live countdowns."""
from pathlib import Path
import hashlib
import json
import math
import re
import time
import traceback
import unreal

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / 'TunaSweeper/Saved/LoopRailLayout/ArrivalDisplay'
MAP = ROOT / 'TunaSweeper/Content/MainRaid/RaidVillage.umap'


def xyz(value):
    return [float(value.x), float(value.y), float(value.z)]


def near(actual, expected, label, tolerance=.05):
    assert abs(actual-expected) <= tolerance, (label, actual, expected)


def transform_record(component):
    transform = component.get_world_transform()
    rotation = transform.rotation.rotator()
    return xyz(transform.translation) + [rotation.pitch, rotation.yaw, rotation.roll] + xyz(transform.scale3d)


class Review:
    def __init__(self):
        OUT.mkdir(parents=True, exist_ok=True)
        self.started = time.monotonic()
        self.before = hashlib.sha256(MAP.read_bytes()).hexdigest()
        self.levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
        self.editor = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
        self.actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
        self.report = {'passed': False}
        self.state, self.frames = 'load', 0
        self.busy = False
        self.handle = unreal.register_slate_post_tick_callback(self.tick)
        unreal.EditorPythonScripting.set_keep_python_script_alive(True)

    def stage(self, name):
        self.state, self.frames = name, 0
        self.stage_time = time.monotonic()

    def tick(self, dt):
        if self.busy: return
        self.busy = True
        self.frames += 1
        try:
            assert time.monotonic() - self.started < 180, self.state
            self.update()
        except Exception:
            self.finish(traceback.format_exc())
        finally:
            self.busy = False

    def update(self):
        if self.state == 'load':
            assert self.levels.load_level('/Game/MainRaid/RaidVillage')
            self.stage('settle')
        elif self.state == 'settle' and self.frames >= 30:
            self.camera = self.actors.spawn_actor_from_class(unreal.CameraActor, unreal.Vector(0, 0, 5000))
            self.camera.set_actor_label('RailArrivalReviewCamera')
            self.camera.get_component_by_class(unreal.CameraComponent).set_editor_property('field_of_view', 40.0)
            self.levels.editor_play_simulate()
            self.stage('pie')
        elif self.state == 'pie' and self.frames >= 45:
            self.world = self.editor.get_game_world()
            assert self.world
            self.stations = sorted(unreal.GameplayStatics.get_all_actors_of_class(self.world, unreal.LoopRailStation),
                                   key=lambda a: str(a.get_editor_property('station_id')))
            assert len(self.stations) == 3, len(self.stations)
            self.assert_geometry()
            self.train = unreal.GameplayStatics.get_all_actors_of_class(self.world, unreal.LoopRailTrain)[0]
            self.camera = next(a for a in unreal.GameplayStatics.get_all_actors_of_class(self.world, unreal.CameraActor)
                               if a.get_actor_label() == 'RailArrivalReviewCamera')
            self.controller = unreal.GameplayStatics.get_player_controller(self.world, 0)
            assert self.controller
            self.train.set_editor_property('start_distance', self.stations[0].get_stop_distance()-8000)
            self.train.reset_train()
            self.station = self.stations[0]
            self.display = self.station.get_editor_property('arrival_display')
            target = self.display.get_world_location()
            eye = target + unreal.Vector(600, -600, 350)
            self.camera.set_actor_location(eye, False, False)
            self.camera.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(eye, target), False)
            self.controller.set_view_target_with_blend(self.camera, 0)
            self.levels.pilot_level_actor(self.camera)
            self.levels.set_exact_camera_view(True)
            self.levels.editor_set_game_view(True)
            self.stage('countdown')
        elif self.state == 'countdown' and self.frames >= 30:
            rows = []
            for station in self.stations:
                text = str(station.get_arrival_display_text())
                assert re.fullmatch(r'\d+:\d{2}', text), text
                widget = station.get_editor_property('arrival_display').get_user_widget_object()
                assert str(widget.get_arrival_display_text()) == text or re.fullmatch(r'\d+:\d{2}', str(widget.get_arrival_display_text()))
                rows.append({'station': str(station.get_editor_property('station_id')), 'text': text})
            self.report['stations'] = rows
            self.initial_text = rows[0]['text']
            self.stage('countdown_wait')
        elif self.state == 'countdown_wait' and time.monotonic()-self.stage_time >= 2:
            now = str(self.station.get_arrival_display_text())
            def seconds(text):
                m, s = map(int, text.split(':'))
                return m*60+s
            assert seconds(now) < seconds(self.initial_text), (self.initial_text, now)
            self.report['countdown'] = [self.initial_text, now]
            self.assert_facing('diagonal_countdown')
            self.take_picture('Countdown')
        elif self.state == 'capture' and self.task.is_task_done():
            assert self.pending.is_file()
            self.train.set_running(False)
            self.station.refresh_arrival_display()
            assert str(self.station.get_arrival_display_text()) == '--:--'
            self.report['stopped_text'] = '--:--'
            self.train.set_editor_property('start_distance', self.station.get_stop_distance())
            self.train.reset_train()
            self.train.set_running(True)
            target = self.display.get_world_location()
            eye = target + unreal.Vector(-500, 400, 650)
            self.camera.set_actor_location(eye, False, False)
            self.camera.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(eye, target), False)
            self.stage('arrived')
        elif self.state == 'arrived' and self.frames >= 20:
            assert self.train.is_dwelling()
            assert str(self.station.get_arrival_display_text()) == '0:00'
            self.assert_facing('steep_arrived')
            self.report['arrived_text'] = '0:00'
            self.take_picture('Arrived', 'capture_arrived')
        elif self.state == 'capture_arrived' and self.task.is_task_done():
            assert self.pending.is_file()
            pivot = self.station.get_editor_property('arrival_marker_pivot')
            self.overhead_yaw = pivot.get_world_rotation().yaw
            target = pivot.get_world_location()
            eye = target + unreal.Vector(0, 0, 2200)
            self.camera.set_actor_location(eye, False, False)
            self.camera.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(eye, target), False)
            self.stage('overhead')
        elif self.state == 'overhead' and self.frames >= 20:
            self.assert_facing('vertical_overhead')
            yaw = self.station.get_editor_property('arrival_marker_pivot').get_world_rotation().yaw
            near((yaw-self.overhead_yaw+180) % 360-180, 0, 'vertical view retains last yaw')
            self.take_picture('Overhead', 'capture_overhead')
        elif self.state == 'capture_overhead' and self.task.is_task_done():
            assert self.pending.is_file()
            self.begin_stop()
        elif self.state == 'release_capture' and self.frames >= 2:
            self.levels.editor_request_end_play()
            self.stage('stop')
        elif self.state == 'stop' and self.frames >= 30:
            assert not self.editor.get_game_world()
            self.finish(self.pending_error)

    def begin_stop(self, error=None):
        # Release the viewport's PIE camera and screenshot UObject while their
        # world and editor modules still exist, then allow a viewport tick.
        self.pending_error = error
        self.levels.eject_pilot_level_actor()
        self.levels.set_exact_camera_view(False)
        self.task = None
        self.world = self.train = self.station = self.display = self.camera = self.controller = None
        self.stations = []
        self.stage('release_capture')

    def assert_geometry(self):
        self.collision_baseline = {}
        rows = []
        for station in self.stations:
            identity = str(station.get_editor_property('station_id'))
            height = station.get_editor_property('arrival_marker_height')
            width = station.get_editor_property('arrival_marker_width')
            depth = station.get_editor_property('arrival_marker_depth')
            for actual, expected, label in [(height, 220, 'height'), (width, 100, 'width'), (depth, 25, 'depth')]:
                near(actual, expected, identity+' '+label)
            pivot = station.get_editor_property('arrival_marker_pivot')
            body = station.get_editor_property('arrival_marker_body')
            collider = station.get_editor_property('arrival_marker_collision')
            display = station.get_editor_property('arrival_display')
            platform = station.get_editor_property('platform')
            assert body.get_attach_parent() == pivot and display.get_attach_parent() == pivot
            assert collider.get_attach_parent() != pivot, 'collision must not rotate with visuals'
            bounds = body.static_mesh.get_bounds()
            scale = body.get_world_transform().scale3d
            dimensions = [2*abs(bounds.box_extent.x*scale.x), 2*abs(bounds.box_extent.y*scale.y), 2*abs(bounds.box_extent.z*scale.z)]
            for actual, expected in zip(dimensions, [depth, width, height]):
                near(actual, expected, identity+' physical mesh dimensions')
            body_bottom = body.get_world_location().z-dimensions[2]/2
            platform_bounds = platform.static_mesh.get_bounds()
            platform_top = unreal.MathLibrary.transform_location(platform.get_world_transform(),
                unreal.Vector(platform_bounds.origin.x, platform_bounds.origin.y, platform_bounds.origin.z+platform_bounds.box_extent.z)).z
            near(body_bottom, platform_top, identity+' ground contact')
            near(pivot.get_world_location().z, platform_top, identity+' pivot base')
            body_local = body.get_editor_property('relative_location')
            for actual, expected in zip(xyz(body_local), [0, 0, height/2]):
                near(actual, expected, identity+' body center')
            face_scale = (width-8)/460
            face_local = display.get_editor_property('relative_location')
            for actual, expected in zip(xyz(face_local), [depth/2+.3, 0, height-10-110*face_scale]):
                near(actual, expected, identity+' front upper LED')
            for actual in xyz(display.get_editor_property('relative_scale3d')):
                near(actual, face_scale, identity+' LED scale', .0001)
            assert collider.get_collision_enabled() == unreal.CollisionEnabled.QUERY_AND_PHYSICS
            assert collider.get_collision_response_to_channel(unreal.CollisionChannel.ECC_PAWN) == unreal.CollisionResponseType.ECR_BLOCK
            near(collider.get_world_location().z-collider.get_scaled_capsule_half_height(), platform_top, identity+' collision ground contact')
            baseline = transform_record(collider)
            self.collision_baseline[identity] = baseline
            rows.append({'station': identity, 'dimensions_depth_width_height_cm': dimensions,
                         'body_bottom_z': body_bottom, 'platform_top_z': platform_top,
                         'pivot_world_cm': xyz(pivot.get_world_location()), 'led_relative_cm': xyz(face_local),
                         'collision_world_transform': baseline, 'collision_radius_cm': collider.get_scaled_capsule_radius()})
        self.report['marker_geometry'] = rows

    def assert_facing(self, phase):
        eye = unreal.GameplayStatics.get_player_camera_manager(self.world, 0).get_camera_location()
        rows = []
        for station in self.stations:
            identity = str(station.get_editor_property('station_id'))
            pivot = station.get_editor_property('arrival_marker_pivot')
            for prop in ['arrival_marker_pivot', 'arrival_marker_body', 'arrival_display']:
                rotation = station.get_editor_property(prop).get_world_rotation()
                assert all(math.isfinite(v) for v in [rotation.pitch, rotation.yaw, rotation.roll])
                near(rotation.pitch, 0, identity+' '+prop+' pitch', .001)
                near(rotation.roll, 0, identity+' '+prop+' roll', .001)
            offset = eye-pivot.get_world_location()
            horizontal = math.hypot(offset.x, offset.y)
            normal = pivot.get_forward_vector()
            dot = (offset.x*normal.x+offset.y*normal.y)/horizontal if horizontal > .01 else None
            if dot is not None: assert dot > .999, (identity, phase, dot)
            for actual, expected in zip(transform_record(station.get_editor_property('arrival_marker_collision')), self.collision_baseline[identity]):
                near(actual, expected, identity+' camera-independent collision', .001)
            rows.append({'station': identity, 'horizontal_facing_dot': dot, 'yaw': pivot.get_world_rotation().yaw,
                         'pitch': pivot.get_world_rotation().pitch, 'roll': pivot.get_world_rotation().roll,
                         'collision_unchanged': True})
        self.report.setdefault('camera_views', []).append({'phase': phase, 'camera_world_cm': xyz(eye), 'stations': rows})

    def take_picture(self, name, next_state='capture'):
        unreal.AutomationLibrary.finish_loading_before_screenshot()
        self.pending = OUT / (name+'.png')
        self.task = unreal.AutomationLibrary.take_high_res_screenshot(1600, 900, str(self.pending), camera=self.camera,
            mask_enabled=False, capture_hdr=False, delay=1.0, force_game_view=True)
        self.stage(next_state)

    def finish(self, error=None):
        # End PIE on a later editor tick before requesting process shutdown.
        if self.editor.get_game_world():
            self.begin_stop(error)
            return
        unreal.unregister_slate_post_tick_callback(self.handle)
        self.handle = None
        self.report['map_unchanged'] = hashlib.sha256(MAP.read_bytes()).hexdigest() == self.before
        self.report['passed'] = not error and self.report['map_unchanged']
        if error: self.report['error'] = error
        self.report['elapsed_seconds'] = time.monotonic()-self.started
        (OUT/'verification.json').write_text(json.dumps(self.report, indent=2), encoding='utf-8')
        self.task = self.levels = self.editor = self.actors = None
        unreal.EditorPythonScripting.set_keep_python_script_alive(False)
        unreal.SystemLibrary.quit_editor()


REVIEW = Review()
