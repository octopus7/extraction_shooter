"""Read-only saved RaidVillage rail geometry, clearance and optional PIE loop verification."""
from pathlib import Path
import hashlib
import json
import math
import time
import traceback
import unreal

ROOT = Path(__file__).resolve().parents[2]
OUTPUT = ROOT / 'TunaSweeper/Saved/LoopRailLayout'
SNAPSHOT = ROOT / 'Docs/level_layout_snapshots/RaidVillage_20261012.json'
MAP = '/Game/MainRaid/RaidVillage'
RUNTIME = '-villagerailruntime' in unreal.SystemLibrary.get_command_line().lower()
REPORT = OUTPUT / ('runtime.json' if RUNTIME else 'verification.json')
WORLD = unreal.SplineCoordinateSpace.WORLD
HEADS = [(3000, -11000, 0), (12000, 3000, 90), (-3000, 11000, 180)]
MAX_GRADE_DEGREES = 6.0


def vector(v):
    return [v.x, v.y, v.z]


def package_hash():
    return hashlib.sha256((ROOT / 'TunaSweeper/Content/MainRaid/RaidVillage.umap').read_bytes()).hexdigest()


def hit_tuple(result):
    if isinstance(result, tuple):
        result = next((item for item in result if isinstance(item, unreal.HitResult)), None)
    return result.to_tuple() if result and result.to_tuple()[0] else None


def angle(a, b):
    divisor = math.sqrt(sum(v*v for v in a) * sum(v*v for v in b))
    assert divisor > 0, 'Zero spline direction'
    return math.degrees(math.acos(max(-1, min(1, sum(x*y for x, y in zip(a, b))/divisor))))


class Verifier:
    def __init__(self):
        self.started = time.monotonic()
        self.before = package_hash()
        self.levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
        self.editor = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
        self.state = 'load'
        self.frames = 0
        self.busy = False
        self.report = {'passed': False, 'map': MAP, 'runtime_requested': RUNTIME}
        self.runtime_world = None
        self.runtime_train = None
        self.runtime_stations = []
        self.arrival_callback = None
        unreal.EditorPythonScripting.set_keep_python_script_alive(True)
        self.handle = unreal.register_slate_post_tick_callback(self.tick)

    def transition(self, state):
        self.state, self.frames = state, 0

    def tick(self, _delta):
        if self.busy:
            return
        self.busy = True
        self.frames += 1
        try:
            assert time.monotonic()-self.started < 300, 'Rail verification timed out: '+self.state
            if self.state == 'load':
                assert self.levels.load_level(MAP), 'Map failed to load'
                self.transition('settle')
            elif self.state == 'settle' and self.frames >= 30:
                self.validate_saved_map()
                if RUNTIME:
                    self.levels.editor_play_simulate()
                    self.transition('pie_start')
                else:
                    self.finish()
            elif self.state == 'pie_start' and self.frames >= 30:
                self.begin_runtime_check()
                self.transition('running')
            elif self.state == 'running':
                self.check_runtime_tick()
            elif self.state == 'pie_stop' and self.frames >= 30:
                assert not self.editor.get_game_world(), 'PIE world did not stop'
                self.finish()
        except Exception:
            self.finish(traceback.format_exc())
        finally:
            self.busy = False

    def ground(self, x, y):
        # A zero-width ray exactly on a fractional Landscape vertex can miss.
        # Retry only within one millimeter; broad collision gaps still fail.
        for dx, dy in ((0, 0), (0, .1), (.1, 0), (-.1, -.1)):
            result = unreal.SystemLibrary.line_trace_single(self.world,
                unreal.Vector(x+dx, y+dy, 10000), unreal.Vector(x+dx, y+dy, -10000),
                unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, False, self.ignore_terrain,
                unreal.DrawDebugTrace.NONE, True)
            hit = hit_tuple(result)
            if hit:
                assert hit[9] == self.landscape
                return hit[5].z
        raise AssertionError(f'No Landscape support at {(x, y)}')

    def validate_saved_map(self):
        self.world = self.editor.get_editor_world()
        self.actors = list(unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors())
        assert len(self.actors) == 171, f'Actor count changed: {len(self.actors)}'
        by_name = {a.get_name(): a for a in self.actors}
        by_label = {a.get_actor_label(): a for a in self.actors}
        self.landscape = by_label['TS_Village_Landscape']
        self.ignore_terrain = [a for a in self.actors if a != self.landscape]
        self.track = by_label['TS_Village_LoopRail']
        self.train = next(a for a in self.actors if isinstance(a, unreal.LoopRailTrain))
        stations = sorted((a for a in self.actors if isinstance(a, unreal.LoopRailStation)),
                          key=lambda a: str(a.get_editor_property('station_id')))
        assert len(stations) == 3
        assert [str(a.get_editor_property('station_id')) for a in stations] == ['village_01', 'village_02', 'village_03']
        assert self.track.is_usable_loop() and self.train.has_usable_track()
        assert self.train.get_editor_property('track') == self.track
        assert self.train.get_editor_property('vehicle_count') == 4
        assert abs(self.train.get_editor_property('vehicle_gap')-80) < .01
        assert abs(self.train.get_consist_length()-3840) < .01
        spline = self.track.get_editor_property('spline')
        length = self.track.get_length()
        expected_length = 60000 + 2*math.pi*4000
        assert abs(length-expected_length) < 600, f'Unexpected rounded loop length {length}'

        snapshot = json.loads(SNAPSHOT.read_text(encoding='utf-8'))
        central = [r for r in snapshot['actors'] if r['category'] == 'central_test_actor']
        assert len(central) == 156
        for record in central:
            actor = by_name[record['name']]
            after = record['after']
            rotation = actor.get_actor_rotation()
            for actual, expected in ((vector(actor.get_actor_location()), after['location_cm']),
                                     (vector(actor.get_actor_scale3d()), after['scale'])):
                assert all(abs(a-b) < .01 for a, b in zip(actual, expected)), f"Central transform changed: {record['name']}"
            actual = [rotation.pitch, rotation.yaw, rotation.roll]
            assert all(abs((a-b+180)%360-180) < .01 for a, b in zip(actual, after['rotation_degrees'])), f"Central rotation changed: {record['name']}"

        station_reports = []
        for station, (x, y, yaw) in zip(stations, HEADS):
            assert station.get_editor_property('track') == self.track
            assert station.get_editor_property('enabled')
            assert abs(station.get_editor_property('platform_length')-3800) < .01
            p = station.get_actor_location()
            assert math.hypot(p.x-x, p.y-y) < 2, f'Wrong station head: {station.get_actor_label()}'
            stop = station.get_stop_distance()
            direction = [math.cos(math.radians(yaw)), math.sin(math.radians(yaw)), 0]
            max_lateral = max_height_delta = max_direction_error = 0
            for offset in range(-3840, 501, 20):
                pose = self.track.sample(stop+offset)
                point = pose.translation
                difference = [point.x-p.x, point.y-p.y, point.z-p.z]
                lateral = abs(difference[0]*direction[1]-difference[1]*direction[0])
                heading_error = angle(vector(pose.rotation.vector_forward()), direction)
                max_lateral = max(max_lateral, lateral)
                max_height_delta = max(max_height_delta, abs(difference[2]))
                max_direction_error = max(max_direction_error, heading_error)
            assert max_lateral < 2 and max_height_delta < 2 and max_direction_error < .2, (
                station.get_actor_label(), max_lateral, max_height_delta, max_direction_error)
            platform = station.get_editor_property('platform')
            ramp = station.get_editor_property('access_ramp')
            side = 1 if station.get_editor_property('platform_on_right') else -1
            platform_pose = platform.get_world_transform()
            platform_edge = unreal.MathLibrary.transform_location(platform_pose, unreal.Vector(0, -side*50, 50))
            platform_local = unreal.MathLibrary.inverse_transform_location(station.get_actor_transform(), platform_edge)
            assert abs(platform_local.z-60) < .1, 'Platform must meet the vehicle floor at +60cm'
            boarding_gap = abs(platform_local.y)-110
            assert 0 <= boarding_gap <= 10, f'Platform boarding gap {boarding_gap}'
            platform_ground_gaps = []
            for longitudinal in (-50, 0, 50):
                for lateral in (-50, 50):
                    point = unreal.MathLibrary.transform_location(platform_pose, unreal.Vector(longitudinal, lateral, -50))
                    platform_ground_gaps.append(point.z-self.ground(point.x, point.y))
            assert min(platform_ground_gaps) >= -10 and max(platform_ground_gaps) <= 30, (
                'Platform foundation terrain clearance', station.get_actor_label(), platform_ground_gaps)
            ramp_ground_gaps = []
            for longitudinal in (-45, 0, 45):
                tip = unreal.MathLibrary.transform_location(ramp.get_world_transform(), unreal.Vector(longitudinal, side*50, 50))
                ramp_ground_gaps.append(tip.z-self.ground(tip.x, tip.y))
            assert all(-10 <= gap <= 40 for gap in ramp_ground_gaps), (
                'Station ramp outer edge must meet terrain within step height', station.get_actor_label(), ramp_ground_gaps)
            station_reports.append({'station_id': str(station.get_editor_property('station_id')),
                'head_cm': vector(p), 'stop_distance_cm': stop, 'straight_sample_count': 218,
                'max_lateral_cm': max_lateral, 'max_height_delta_cm': max_height_delta,
                'max_heading_error_degrees': max_direction_error, 'boarding_gap_cm': boarding_gap,
                'platform_foundation_ground_gap_cm': platform_ground_gaps,
                'ramp_outer_edge_ground_gap_cm': ramp_ground_gaps})

        sample_count = math.ceil(length/100)
        support_deltas, grades, heading_changes, shape_errors = [], [], [], []
        previous_direction = None
        clearance_ignores = [self.track, self.train] + stations
        extents_xy = []
        for index in range(sample_count):
            distance = index*length/sample_count
            pose = self.track.sample(distance)
            p = pose.translation
            direction = pose.rotation.vector_forward()
            right = pose.rotation.vector_right()
            extents_xy.append([p.x, p.y])
            # Signed distance to the requested 240m x 220m rounded rectangle,
            # with 40m corner radii. Cubic quarter-circle approximation is allowed.
            qx, qy = abs(p.x)-8000, abs(p.y)-7000
            shape_errors.append(abs(math.hypot(max(qx, 0), max(qy, 0))+min(max(qx, qy), 0)-4000))
            grades.append(abs(math.degrees(math.atan2(direction.z, math.hypot(direction.x, direction.y)))))
            if previous_direction is not None:
                heading_changes.append(angle(previous_direction, vector(direction)))
            previous_direction = vector(direction)
            for offset in (-75, 0, 75):
                q = p+right*offset
                support_deltas.append(q.z-self.ground(q.x, q.y))
            # The rail and own station/train meshes are intentionally excluded;
            # terrain, central props and map boundaries must clear the car body.
            center = p+unreal.Vector(0, 0, 225)
            hit = hit_tuple(unreal.SystemLibrary.box_trace_single_by_profile(self.world,
                center, center+unreal.Vector(0, 0, .1), unreal.Vector(450, 115, 150),
                pose.rotation.rotator(), 'Pawn', False, clearance_ignores,
                unreal.DrawDebugTrace.NONE, True))
            assert not hit, f'Car body clearance blocked at {distance}: {hit[9].get_actor_label() if hit else ""}'
        assert max(grades) <= MAX_GRADE_DEGREES, f'Rail grade {max(grades)}'
        assert max(shape_errors) <= 15, f'Rail deviates from rounded rectangle {max(shape_errors)}cm'
        assert max(heading_changes) <= 3, f'Rail kink per meter {max(heading_changes)}'
        assert min(support_deltas) >= 5 and max(support_deltas) <= 25, (
            'Sleepers unsupported or buried', min(support_deltas), max(support_deltas))
        assert abs(max(abs(p[0]) for p in extents_xy)-12000) < 2
        assert abs(max(abs(p[1]) for p in extents_xy)-11000) < 2
        joins = []
        for index in range(spline.get_number_of_spline_points()):
            distance = spline.get_distance_along_spline_at_spline_point(index)
            before = self.track.sample(distance-1).rotation.vector_forward()
            after = self.track.sample(distance+1).rotation.vector_forward()
            joins.append(angle(vector(before), vector(after)))
        assert max(joins) < .5, f'Spline tangent break: {max(joins)}'
        for actor in [self.track, self.train] + stations:
            center, extent = actor.get_actor_bounds(False)
            assert abs(center.x)+extent.x < 14900 and abs(center.y)+extent.y < 14900, (
                'Rail infrastructure outside 300m boundary', actor.get_actor_label(), vector(center), vector(extent))
        self.report['structure'] = {'actor_count': len(self.actors), 'central_transforms_unchanged': len(central),
            'length_cm': length, 'rail_samples': sample_count, 'ground_samples': len(support_deltas),
            'rail_ground_clearance_cm': [min(support_deltas), max(support_deltas)],
            'max_rounded_rectangle_error_cm': max(shape_errors),
            'max_grade_degrees': max(grades), 'max_heading_change_per_meter_degrees': max(heading_changes),
            'max_join_angle_degrees': max(joins), 'stations': station_reports}
        unreal.SystemLibrary.execute_console_command(self.world, 'MAP CHECK')

    def begin_runtime_check(self):
        self.runtime_world = self.editor.get_game_world()
        assert self.runtime_world, 'PIE world missing'
        self.runtime_train = unreal.GameplayStatics.get_all_actors_of_class(self.runtime_world, unreal.LoopRailTrain)[0]
        self.runtime_stations = list(unreal.GameplayStatics.get_all_actors_of_class(self.runtime_world, unreal.LoopRailStation))
        self.runtime_track = self.runtime_train.get_editor_property('track')
        self.runtime_length = self.runtime_track.get_length()
        self.runtime_arrivals = []
        self.arrival_callback = self.station_arrived
        self.runtime_train.on_station_arrived.add_callable(self.arrival_callback)
        for station in self.runtime_stations:
            station.set_editor_property('dwell_seconds', .5)
        first = min(self.runtime_stations, key=lambda a: str(a.get_editor_property('station_id')))
        self.runtime_train.set_editor_property('start_distance', first.get_stop_distance()-5000)
        self.runtime_train.set_editor_property('cruise_speed', 1600)
        self.runtime_train.set_editor_property('acceleration', 2400)
        self.runtime_train.set_editor_property('braking_deceleration', 2400)
        self.runtime_train.reset_train()
        self.runtime_train.set_running(True)
        unreal.GameplayStatics.set_global_time_dilation(self.runtime_world, 10)
        self.last_distance = self.runtime_train.get_head_distance()
        self.travelled = 0

    def station_arrived(self, station):
        stop = station.get_stop_distance()
        head = self.runtime_train.get_head_distance()
        distance_error = min((head-stop)%self.runtime_length, (stop-head)%self.runtime_length)
        self.runtime_arrivals.append({'station_id': str(station.get_editor_property('station_id')),
            'distance_error_cm': distance_error, 'speed_cm_per_second': self.runtime_train.get_speed()})

    def check_runtime_tick(self):
        distance = self.runtime_train.get_head_distance()
        delta = (distance-self.last_distance)%self.runtime_length
        assert delta < self.runtime_length*.25, 'Train moved backwards or jumped'
        self.travelled += delta
        self.last_distance = distance
        for index in range(4):
            floor = self.runtime_train.get_vehicle_floor(index)
            assert floor, f'Runtime vehicle floor {index} missing'
            pose = floor.get_world_transform()
            center_distance = distance-index*980
            front = self.runtime_track.sample(center_distance+300).translation
            back = self.runtime_track.sample(center_distance-300).translation
            expected = (front+back)*.5+pose.rotation.vector_up()*50
            assert math.dist(vector(pose.translation), vector(expected)) < 1, 'Vehicle floor lost its spline placement'
        if self.travelled >= self.runtime_length+100 and len({x['station_id'] for x in self.runtime_arrivals}) == 3:
            assert all(x['distance_error_cm'] < 2 and x['speed_cm_per_second'] < .1 for x in self.runtime_arrivals), self.runtime_arrivals
            self.report['runtime'] = {'arrivals': self.runtime_arrivals, 'travelled_cm': self.travelled,
                'loop_length_cm': self.runtime_length, 'completed_laps': self.travelled/self.runtime_length,
                'temporary_time_dilation': 10, 'temporary_cruise_speed_cm_per_second': 1600}
            self.stop_runtime()
            self.transition('pie_stop')

    def stop_runtime(self):
        if self.runtime_train and self.arrival_callback:
            self.runtime_train.on_station_arrived.remove_callable(self.arrival_callback)
        if self.runtime_world:
            unreal.GameplayStatics.set_global_time_dilation(self.runtime_world, 1)
        self.runtime_train = None
        self.runtime_stations = []
        self.runtime_track = None
        self.runtime_world = None
        self.arrival_callback = None
        if self.editor.get_game_world():
            self.levels.editor_request_end_play()

    def finish(self, error=None):
        unreal.unregister_slate_post_tick_callback(self.handle)
        self.stop_runtime()
        self.report['asset_packages_unchanged'] = self.before == package_hash()
        self.report['elapsed_seconds'] = round(time.monotonic()-self.started, 3)
        self.report['passed'] = error is None and self.report['asset_packages_unchanged']
        if error:
            self.report['error'] = error
            unreal.log_error(error)
        OUTPUT.mkdir(parents=True, exist_ok=True)
        REPORT.write_text(json.dumps(self.report, indent=2)+'\n', encoding='utf-8')
        unreal.log('VILLAGE_RAIL_VERIFIED '+str(self.report['passed']))
        unreal.EditorPythonScripting.set_keep_python_script_alive(False)
        unreal.SystemLibrary.quit_editor()


VERIFIER = Verifier()
