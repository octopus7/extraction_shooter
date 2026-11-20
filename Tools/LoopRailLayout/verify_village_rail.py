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
SNAPSHOT = ROOT / 'Docs/level_layout_snapshots/RaidVillage_RailLayout_20261115.json'
MAP = '/Game/MainRaid/RaidVillage'
RUNTIME = '-villagerailruntime' in unreal.SystemLibrary.get_command_line().lower()
REPORT = OUTPUT / ('runtime.json' if RUNTIME else 'verification.json')
MAX_GRADE_DEGREES = 6.0


def vector(v):
    return [v.x, v.y, v.z]


def package_hash():
    return hashlib.sha256((ROOT / 'TunaSweeper/Content/MainRaid/RaidVillage.umap').read_bytes()).hexdigest()


def hit_tuple(result):
    if isinstance(result, tuple):
        result = next((item for item in result if isinstance(item, unreal.HitResult)), None)
    return result.to_tuple() if result and result.to_tuple()[0] else None


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
            elif (self.state == 'settle' and self.frames >= 30
                  and not unreal.NavigationSystemV1.is_navigation_being_built(self.editor.get_editor_world())):
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

    def vertical_hit(self, x, y, ignore):
        # Nudge by at most a millimetre to avoid tracing exactly along a triangle edge.
        for dx, dy in ((0, 0), (0, .1), (.1, 0), (-.1, -.1)):
            result = unreal.SystemLibrary.line_trace_single(
                self.world, unreal.Vector(x+dx, y+dy, 10000), unreal.Vector(x+dx, y+dy, -10000),
                unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, False, ignore, unreal.DrawDebugTrace.NONE, True)
            hit = hit_tuple(result)
            if hit:
                return hit
        raise AssertionError(f'No supporting surface at {(x, y)}')

    def ignore_except(self, *keep):
        return [actor for actor in self.actors if actor not in keep]

    def check_plan_view(self):
        # The terrain fit may subdivide the curve and change arc lengths. Compare the
        # horizontal locus to the authored Hermite spans, independently of input keys.
        nodes = json.loads(SNAPSHOT.read_text(encoding='utf-8-sig'))['nodes']
        max_error = 0.0
        for start, end in zip(nodes, nodes[1:] + nodes[:1]):
            p0, p1 = unreal.Vector(*start['p']), unreal.Vector(*end['p'])
            leave, arrive = unreal.Vector(*start['leave']), unreal.Vector(*end['arrive'])
            count = max(16, math.ceil(math.dist(start['p'], end['p']) / 100))
            for i in range(count):
                t = i / count
                point = (p0*((1+2*t)*(1-t)**2) + leave*(t*(1-t)**2)
                         + p1*(t*t*(3-2*t)) + arrive*(t*t*(t-1)))
                distance = self.track.find_distance(point)
                for _ in range(3):
                    pose = self.track.sample(distance)
                    delta = point - pose.translation
                    forward = pose.rotation.vector_forward()
                    distance += (delta.x*forward.x + delta.y*forward.y)/(forward.x**2 + forward.y**2)
                match = self.track.sample(distance).translation
                max_error = max(max_error, math.hypot(point.x-match.x, point.y-match.y))
        assert max_error < 2, f'Horizontal route moved by {max_error:.2f} cm'
        return max_error

    def check_station(self, station):
        assert station.track == self.track and station.get_editor_property('enabled')
        assert abs(station.platform_length-3800) < .01
        head = station.get_actor_location()
        stop = station.get_stop_distance()
        assert (self.track.sample(stop).translation-head).length() < 1, station.get_actor_label()
        yaw = math.radians(station.get_actor_rotation().yaw)
        forward = unreal.Vector(math.cos(yaw), math.sin(yaw), 0)
        side = 1 if station.get_editor_property('platform_on_right') else -1
        right = unreal.Vector(-forward.y*side, forward.x*side, 0)
        for offset in range(-3350, 451, 50):
            on_track = self.track.sample(stop+offset).translation-head
            assert abs(on_track.x*right.x+on_track.y*right.y) < 2, 'Platform must follow the track in plan view'

        # Move the editor preview train only. Use collision on the fitted platform,
        # including its grade, instead of assuming the mesh is a scaled unit cube.
        assert self.train.arrive_at_station(station)
        station_only = self.ignore_except(station)
        steps = []
        for car in range(1, self.train.vehicle_count):
            floor = self.train.get_vehicle_floor(car).get_world_transform()
            for door_x in (-356, 356):
                door = floor.transform_location(unreal.Vector(door_x, side*110, 10))
                # A point just outside the car is clear; a point 10cm out must be on the platform.
                gap = door + right*2
                hit = unreal.SystemLibrary.line_trace_single(
                    self.world, gap+unreal.Vector(0,0,25), gap-unreal.Vector(0,0,25),
                    unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, False, station_only, unreal.DrawDebugTrace.NONE, True)
                assert not hit_tuple(hit), 'Platform intrudes into the vehicle clearance'
                edge = door + right*10
                support = self.vertical_hit(edge.x, edge.y, station_only)
                assert support[10] == station.platform
                steps.append(abs(support[5].z-door.z))
        assert max(steps) <= 5, f'Boarding step at {station.station_id}: {max(steps):.2f} cm'

        toe_gaps = []
        for x in (170, 250, 330):
            toe = head + forward*x + right*(115+station.platform_width+298)
            support = self.vertical_hit(toe.x, toe.y, station_only)
            assert support[10] == station.access_ramp
            ground = self.vertical_hit(toe.x, toe.y, self.terrain_only)[5].z
            toe_gaps.append(support[5].z-ground)
        assert all(-1 <= gap <= 5 for gap in toe_gaps), toe_gaps
        return {'id': str(station.station_id), 'max_boarding_step_cm': max(steps), 'ramp_toe_gaps_cm': toe_gaps}

    def check_crossing(self, deck):
        center = deck.get_actor_location()
        pose = self.track.sample(self.track.find_distance(center))
        right = pose.rotation.vector_right()
        forward = pose.rotation.vector_forward()
        surfaces = self.ignore_except(deck, self.bed, self.landscape)
        max_step = 0.0
        for along in (-150, 0, 150):
            points = []
            for across in range(-450, 451, 15):
                point = center + right*across + forward*along
                points.append(self.vertical_hit(point.x, point.y, surfaces)[5])
            path = unreal.NavigationSystemV1.find_path_to_location_synchronously(
                self.world, points[0]+unreal.Vector(0,0,3), points[-1]+unreal.Vector(0,0,3))
            assert path and path.is_valid() and not path.is_partial(), 'North crossing is unreachable'
            assert math.dist(vector(path.path_points[-1])[:2], vector(points[-1])[:2]) < 12
            for a, b in zip(points, points[1:]):
                max_step = max(max_step, abs(a.z-b.z))
                up = unreal.Vector(0,0,94)
                blocked = unreal.SystemLibrary.capsule_trace_single(
                    self.world, a+up, b+up, 34, 88, unreal.TraceTypeQuery.TRACE_TYPE_QUERY1,
                    False, [self.train], unreal.DrawDebugTrace.NONE, True)
                assert not hit_tuple(blocked), f'Blocked north crossing at {a}'
        assert max_step <= 6, f'Crossing step: {max_step:.2f} cm'
        return max_step

    def validate_saved_map(self):
        self.world = self.editor.get_editor_world()
        self.actors = list(unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors())
        labels = {actor.get_actor_label(): actor for actor in self.actors}
        self.landscape = labels['TS_Village_Landscape']
        self.bed = labels['TS_Village_GravelTrackBed']
        deck = labels['TS_Village_NorthTimberCrossing']
        self.terrain_only = self.ignore_except(self.landscape)
        bed_only = self.ignore_except(self.bed)
        self.track, = [a for a in self.actors if isinstance(a, unreal.LoopRailTrack)]
        self.train, = [a for a in self.actors if isinstance(a, unreal.LoopRailTrain)]
        stations = sorted((a for a in self.actors if isinstance(a, unreal.LoopRailStation)), key=lambda a: str(a.station_id))
        assert [str(a.station_id) for a in stations] == ['village_01', 'village_02', 'village_03']
        assert self.track.is_usable_loop() and self.train.has_usable_track() and self.train.track == self.track
        assert self.train.vehicle_count == 4 and abs(self.train.get_consist_length()-3840) < .01
        length = self.track.get_length()
        assert self.track.sleepers.get_instance_count() == math.ceil(length/self.track.sleeper_spacing)
        assert self.track.rails.get_instance_count() == 2*math.ceil(length/self.track.rail_segment_length)
        assert self.track.sleepers.static_mesh.get_name() == 'SM_VillageRailSleeper'
        assert self.track.rails.static_mesh.get_name() == 'SM_VillageSteelRail'
        for suffix in ('In', 'Out'):
            assert labels['TS_Village_NorthCrossbuck_'+suffix].static_mesh_component.static_mesh.get_name() == 'SM_VillageCrossbuck'

        sample_count = math.ceil(length/100)
        clearance_ignores = [self.track, self.train, *stations]
        ground_gaps, ballast_gaps, grades = [], [], []
        for i in range(sample_count):
            pose = self.track.sample(i*length/sample_count)
            p = pose.translation
            grades.append(abs(pose.rotation.rotator().pitch))
            for offset in (-self.track.gauge/2, 0, self.track.gauge/2):
                point = p + pose.rotation.vector_right()*offset
                ground_gaps.append(point.z-self.vertical_hit(point.x, point.y, self.terrain_only)[5].z)
                ballast_gaps.append(point.z-self.vertical_hit(point.x, point.y, bed_only)[5].z)
            center = p + unreal.Vector(0, 0, 225)
            hit = hit_tuple(unreal.SystemLibrary.box_trace_single_by_profile(
                self.world, center, center+unreal.Vector(0,0,.1), unreal.Vector(450,115,150),
                pose.rotation.rotator(), 'Pawn', False, clearance_ignores, unreal.DrawDebugTrace.NONE, True))
            assert not hit, f'Car clearance blocked at {p}: {hit[9].get_actor_label() if hit else ""}'
        assert max(grades) <= MAX_GRADE_DEGREES, max(grades)
        assert min(ground_gaps) > 0, 'Landscape protrudes through a rail'
        assert min(ballast_gaps) > 1 and max(ballast_gaps) < 15, 'Rail bed misses the sleepers or covers the rail head'
        spline = self.track.spline
        joins = []
        for i in range(spline.get_number_of_spline_points()):
            d = spline.get_distance_along_spline_at_spline_point(i)
            before = self.track.sample(d-1).rotation.vector_forward()
            after = self.track.sample(d+1).rotation.vector_forward()
            cosine = before.x*after.x + before.y*after.y + before.z*after.z
            joins.append(math.degrees(math.acos(max(-1, min(1, cosine)))))
        assert max(joins) < .5, f'Spline tangent break: {max(joins):.2f} degrees'
        for actor in [self.track, self.bed, *stations]:
            center, extent = actor.get_actor_bounds(False)
            assert abs(center.x)+extent.x < 15000 and abs(center.y)+extent.y < 15000, actor.get_actor_label()
        self.report['structure'] = {
            'length_cm': length, 'rail_samples': sample_count, 'max_grade_degrees': max(grades), 'max_join_angle_degrees': max(joins),
            'max_horizontal_error_cm': self.check_plan_view(), 'rail_ground_clearance_cm': [min(ground_gaps), max(ground_gaps)],
            'rail_ballast_clearance_cm': [min(ballast_gaps), max(ballast_gaps)],
            'crossing_max_step_cm': self.check_crossing(deck), 'stations': [self.check_station(station) for station in stations]}
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
