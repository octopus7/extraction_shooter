"""Rendered PIE verification of live station countdowns and camera-facing panels."""
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
            eye = target + unreal.Vector(1400, -1400, 1400)
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
            self.assert_facing()
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
            eye = target + unreal.Vector(-1200, 800, 2000)
            self.camera.set_actor_location(eye, False, False)
            self.camera.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(eye, target), False)
            self.stage('arrived')
        elif self.state == 'arrived' and self.frames >= 20:
            assert self.train.is_dwelling()
            assert str(self.station.get_arrival_display_text()) == '0:00'
            self.assert_facing()
            self.report['arrived_text'] = '0:00'
            self.take_picture('Arrived', 'capture_arrived')
        elif self.state == 'capture_arrived' and self.task.is_task_done():
            assert self.pending.is_file()
            self.levels.editor_request_end_play()
            self.world = self.train = self.station = self.display = self.camera = self.controller = None
            self.stations = []
            self.stage('stop')
        elif self.state == 'stop' and self.frames >= 30:
            assert not self.editor.get_game_world()
            self.finish()

    def assert_facing(self):
        view = unreal.GameplayStatics.get_player_camera_manager(self.world, 0).get_camera_rotation().get_forward_vector()
        normal = self.display.get_forward_vector()
        dot = view.x*normal.x+view.y*normal.y+view.z*normal.z
        assert dot < -.999, dot
        self.report.setdefault('camera_facing_dot', []).append(dot)

    def take_picture(self, name, next_state='capture'):
        unreal.AutomationLibrary.finish_loading_before_screenshot()
        self.pending = OUT / (name+'.png')
        self.task = unreal.AutomationLibrary.take_high_res_screenshot(1600, 900, str(self.pending), camera=self.camera,
            mask_enabled=False, capture_hdr=False, delay=1.0, force_game_view=True)
        self.stage(next_state)

    def finish(self, error=None):
        unreal.unregister_slate_post_tick_callback(self.handle)
        if self.editor.get_game_world(): self.levels.editor_request_end_play()
        self.report['map_unchanged'] = hashlib.sha256(MAP.read_bytes()).hexdigest() == self.before
        self.report['passed'] = not error and self.report['map_unchanged']
        if error: self.report['error'] = error
        self.report['elapsed_seconds'] = time.monotonic()-self.started
        (OUT/'verification.json').write_text(json.dumps(self.report, indent=2), encoding='utf-8')
        unreal.EditorPythonScripting.set_keep_python_script_alive(False)
        unreal.SystemLibrary.quit_editor()


REVIEW = Review()
