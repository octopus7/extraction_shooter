"""Fresh-process, read-only verification for the three Main raid region maps."""

from __future__ import annotations

from pathlib import Path
import hashlib
import json
import math
import time
import traceback

import unreal


ROOT = Path(__file__).resolve().parents[2]
REPORT = ROOT / "TunaSweeper/Saved/MainRaidLevels/verification.json"
EXPECTED_MATERIAL = "/Game/Materials/Landscape/M_LandScape.M_LandScape"
EXPECTED_LAYERS = {"Grass", "GrassDark", "Dirt", "Rock"}
MAPS = {
    "RaidForest": {
        "path": "/Game/MainRaid/RaidForest",
        "required_labels": {
            "TS_Forest_Landscape",
            "TS_Forest_PlayerStart",
            "TS_Forest_Destination",
            "TS_Forest_ReviewCamera",
            "TS_Forest_Trees",
            "TS_Forest_Rocks",
        },
        # The final cliff rise is reserved for the undecided transport
        # solution; the continuous walkable route ends at its approach.
        "route": [(-22000, -5000), (-15000, -2500), (-8000, 1500), (0, -800), (9000, 1200), (16500, -1000), (21500, -100)],
        "min_mesh_references": 6,
    },
    "RaidVillage": {
        "path": "/Game/MainRaid/RaidVillage",
        "required_labels": {
            "TS_Village_Landscape",
            "TS_Village_PlayerStart",
            "TS_Village_District_Farm",
            "TS_Village_District_Workshop",
            "TS_Village_District_Depot",
            "TS_Village_Highland",
            "TS_Village_LoopRail",
            "TS_Village_ReviewCamera",
        },
        "route": [(-22000, -12000), (-15500, -7500), (-9000, -3500), (-3000, 0), (5000, 2500), (12000, 6500), (19000, 10000)],
        "min_mesh_references": 8,
    },
    "RaidPlains": {
        "path": "/Game/MainRaid/RaidPlains",
        "required_labels": {
            "TS_Plains_Landscape",
            "TS_Plains_PlayerStart",
            "TS_Plains_SSTOBlockout",
            "TS_Plains_LandingZone",
            "TS_Plains_ReviewCamera",
            "TS_Plains_Rocks",
        },
        "route": [(-22000, -9000), (-15000, -6000), (-8000, -2500), (0, 500), (8500, 3500), (15500, 7000), (22000, 9000)],
        "min_mesh_references": 5,
    },
}


def package_hashes():
    folder = ROOT / "TunaSweeper/Content/MainRaid"
    return {
        path.name: hashlib.sha256(path.read_bytes()).hexdigest()
        for path in sorted(folder.glob("RaidForest.umap"))
        + sorted(folder.glob("RaidVillage.umap"))
        + sorted(folder.glob("RaidPlains.umap"))
    }


def vector(value):
    return [round(value.x, 3), round(value.y, 3), round(value.z, 3)]


def densify_route(points, spacing_cm=500.0):
    samples = [tuple(points[0])]
    for (start_x, start_y), (end_x, end_y) in zip(points, points[1:]):
        distance = math.hypot(end_x - start_x, end_y - start_y)
        steps = max(1, math.ceil(distance / spacing_cm))
        for step in range(1, steps + 1):
            alpha = step / steps
            samples.append((
                start_x + (end_x - start_x) * alpha,
                start_y + (end_y - start_y) * alpha,
            ))
    return samples


def mesh_path(component):
    try:
        mesh = component.get_editor_property("static_mesh")
    except Exception:
        return None
    return mesh.get_path_name() if mesh else None


def blocking_hit(result):
    if result is None:
        return False, None
    if isinstance(result, unreal.HitResult):
        return True, result
    if isinstance(result, tuple):
        hit = next((item for item in result if isinstance(item, unreal.HitResult)), None)
        flag = next((item for item in result if isinstance(item, bool)), hit is not None)
        return bool(flag), hit
    return bool(result), None


class MainRaidVerifier:
    def __init__(self):
        self.started = time.monotonic()
        self.levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
        self.actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
        if not self.levels or not self.actors:
            raise RuntimeError("Run this verifier in the full Unreal Editor")
        self.before = package_hashes()
        self.names = list(MAPS)
        self.index = 0
        self.state = "load"
        self.ticks = 0
        self.handle = None
        self.report = {
            "passed": False,
            "engine_version": unreal.SystemLibrary.get_engine_version(),
            "verification": "fresh-process saved-map reload, structure, collision and route samples",
            "maps": {},
        }

    def start(self):
        unreal.EditorPythonScripting.set_keep_python_script_alive(True)
        self.handle = unreal.register_slate_post_tick_callback(self.tick)
        unreal.log("MAIN_RAID_LEVEL_VERIFICATION_STARTED")

    def tick(self, _delta_seconds):
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
            raise TimeoutError(f"Timed out during {self.state}")
        self.ticks += 1
        if self.state == "load":
            name = self.names[self.index]
            spec = MAPS[name]
            if not unreal.EditorAssetLibrary.does_asset_exist(spec["path"]):
                raise AssertionError(f"Missing map asset: {spec['path']}")
            if not self.levels.load_level(spec["path"]):
                raise AssertionError(f"Unable to load map: {spec['path']}")
            self.state = "settle"
            self.ticks = 0
            unreal.log(f"MAIN_RAID_LEVEL_LOADED {spec['path']}")
        elif self.state == "settle":
            if self.ticks < 30:
                return
            self.validate_current()
            self.index += 1
            if self.index >= len(self.names):
                self.finish()
            else:
                self.state = "load"
                self.ticks = 0

    def validate_current(self):
        name = self.names[self.index]
        spec = MAPS[name]
        world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
        all_actors = self.actors.get_all_level_actors()
        labels = {actor.get_actor_label(): actor for actor in all_actors}
        assert len(labels) == len(all_actors), f"{name}: duplicate actor labels"
        missing_labels = sorted(spec["required_labels"] - labels.keys())
        assert not missing_labels, f"{name}: missing labels {missing_labels}"

        landscape_actors = [actor for actor in all_actors if actor.get_class().get_name() == "Landscape"]
        assert len(landscape_actors) == 1, f"{name}: expected one Landscape, found {len(landscape_actors)}"
        landscape = landscape_actors[0]
        material = landscape.get_editor_property("landscape_material")
        assert material and material.get_path_name() == EXPECTED_MATERIAL, (name, material)
        landscape_components = [
            component for component in landscape.get_components_by_class(unreal.PrimitiveComponent)
            if component.get_class().get_name() == "LandscapeComponent"
        ]
        assert len(landscape_components) == 64, f"{name}: expected 64 Landscape components, got {len(landscape_components)}"
        layer_names = {str(layer_name) for layer_name in landscape.get_target_layer_names()}
        assert layer_names == EXPECTED_LAYERS, f"{name}: landscape layers {sorted(layer_names)}"

        starts = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.PlayerStart)
        assert len(starts) == 1, f"{name}: expected one PlayerStart"
        assert starts[0].get_actor_label() in spec["required_labels"]
        assert unreal.GameplayStatics.get_all_actors_of_class(world, unreal.DirectionalLight), f"{name}: no DirectionalLight"
        assert unreal.GameplayStatics.get_all_actors_of_class(world, unreal.SkyLight), f"{name}: no SkyLight"
        assert unreal.GameplayStatics.get_all_actors_of_class(world, unreal.SkyAtmosphere), f"{name}: no SkyAtmosphere"

        mesh_references = sorted({
            path
            for actor in all_actors
            for component in actor.get_components_by_class(unreal.StaticMeshComponent)
            if (path := mesh_path(component))
        })
        assert len(mesh_references) >= spec["min_mesh_references"], (name, mesh_references)

        if name == "RaidVillage":
            track_cls = unreal.load_class(None, "/Script/LoopRail.LoopRailTrack")
            train_cls = unreal.load_class(None, "/Script/LoopRail.LoopRailTrain")
            station_cls = unreal.load_class(None, "/Script/LoopRail.LoopRailStation")
            tracks = unreal.GameplayStatics.get_all_actors_of_class(world, track_cls)
            trains = unreal.GameplayStatics.get_all_actors_of_class(world, train_cls)
            stations = unreal.GameplayStatics.get_all_actors_of_class(world, station_cls)
            assert len(tracks) == 1 and len(trains) == 1 and len(stations) == 3
            assert tracks[0].is_usable_loop() and tracks[0].get_length() > 60000
            assert trains[0].get_editor_property("track") == tracks[0]
            assert all(station.get_editor_property("track") == tracks[0] for station in stations)
        elif name == "RaidPlains":
            ssto = labels["TS_Plains_SSTOBlockout"]
            ssto_parts = [c for c in ssto.get_components_by_class(unreal.StaticMeshComponent) if mesh_path(c)]
            assert len(ssto_parts) >= 7, f"{name}: SSTO blockout needs a readable multi-part silhouette"

        player_start_location = starts[0].get_actor_location()
        start_ground_trace = unreal.SystemLibrary.line_trace_single(
            world,
            unreal.Vector(player_start_location.x, player_start_location.y, 15000),
            unreal.Vector(player_start_location.x, player_start_location.y, -15000),
            unreal.TraceTypeQuery.TRACE_TYPE_QUERY1,
            False, [starts[0]], unreal.DrawDebugTrace.NONE, True,
        )
        start_hit, start_result = blocking_hit(start_ground_trace)
        assert start_hit and start_result is not None, f"{name}: PlayerStart has no ground collision"
        start_ground = start_result.to_tuple()[5]
        start_clearance = player_start_location.z - start_ground.z
        assert start_clearance >= 95.0, f"{name}: PlayerStart is only {start_clearance:.2f} cm above ground"
        start_capsule_trace = unreal.SystemLibrary.capsule_trace_single(
            world, player_start_location, player_start_location + unreal.Vector(0, 0, 1),
            42.0, 90.0, unreal.TraceTypeQuery.TRACE_TYPE_QUERY1,
            False, [starts[0]], unreal.DrawDebugTrace.NONE, True,
        )
        start_blocked, _ = blocking_hit(start_capsule_trace)
        assert not start_blocked, f"{name}: PlayerStart capsule volume is obstructed"

        route = []
        route_heights = []
        route_samples = densify_route(spec["route"])
        for index, (x, y) in enumerate(route_samples):
            trace = unreal.SystemLibrary.line_trace_single(
                world, unreal.Vector(x, y, 15000), unreal.Vector(x, y, -15000),
                unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, False, [],
                unreal.DrawDebugTrace.NONE, True,
            )
            hit, result = blocking_hit(trace)
            assert hit and result is not None, f"{name}: route point {index} has no ground collision"
            # UE 5.7's Python struct wrapper exposes this as a direct field,
            # not through UObject-style get_editor_property().
            impact = result.to_tuple()[5]
            route_heights.append(impact.z)
            clearance = unreal.SystemLibrary.capsule_trace_single(
                world,
                unreal.Vector(x, y, impact.z + 105),
                unreal.Vector(x, y, impact.z + 106),
                42.0, 90.0,
                unreal.TraceTypeQuery.TRACE_TYPE_QUERY1,
                False, [], unreal.DrawDebugTrace.NONE, True,
            )
            blocked, _ = blocking_hit(clearance)
            assert not blocked, f"{name}: route point {index} is obstructed"
            route.append({"xy_cm": [x, y], "ground_z_cm": round(impact.z, 2), "clear": True})

        max_slope_degrees = 0.0
        max_slope_segment = None
        for (a_x, a_y), (b_x, b_y), a_z, b_z in zip(route_samples, route_samples[1:], route_heights, route_heights[1:]):
            horizontal = math.hypot(b_x - a_x, b_y - a_y)
            slope_degrees = math.degrees(math.atan2(abs(b_z - a_z), horizontal))
            if slope_degrees > max_slope_degrees:
                max_slope_degrees = slope_degrees
                max_slope_segment = ((a_x, a_y, a_z), (b_x, b_y, b_z))
        assert max_slope_degrees <= 18.0, (
            f"{name}: sampled route slope {max_slope_degrees:.2f} degrees at {max_slope_segment}"
        )

        unreal.SystemLibrary.execute_console_command(world, "MAP CHECK")
        self.report["maps"][name] = {
            "path": spec["path"],
            "actor_count": len(all_actors),
            "landscape_components": len(landscape_components),
            "landscape_material": material.get_path_name(),
            "landscape_layers": sorted(layer_names),
            "player_start_cm": vector(starts[0].get_actor_location()),
            "player_start_ground_clearance_cm": round(start_clearance, 2),
            "mesh_references": mesh_references,
            "route_samples": route,
            "max_sampled_route_slope_degrees": round(max_slope_degrees, 3),
            "map_check_requested": True,
        }

    def finish(self, error=None):
        if self.handle is not None:
            unreal.unregister_slate_post_tick_callback(self.handle)
            self.handle = None
        after = package_hashes()
        changed = sorted(key for key in set(self.before) | set(after) if self.before.get(key) != after.get(key))
        self.report["asset_packages_unchanged"] = not changed
        self.report["changed_asset_packages"] = changed
        self.report["elapsed_seconds"] = round(time.monotonic() - self.started, 3)
        if error:
            self.report["error"] = error
        self.report["passed"] = error is None and not changed and len(self.report["maps"]) == len(MAPS)
        REPORT.parent.mkdir(parents=True, exist_ok=True)
        REPORT.write_text(json.dumps(self.report, indent=2), encoding="utf-8")
        (unreal.log if self.report["passed"] else unreal.log_error)(
            ("MAIN_RAID_LEVEL_VERIFICATION_PASSED " if self.report["passed"] else "MAIN_RAID_LEVEL_VERIFICATION_FAILED ")
            + str(REPORT)
        )
        if error:
            unreal.log_error(error)
        unreal.EditorPythonScripting.set_keep_python_script_alive(False)
        unreal.SystemLibrary.quit_editor()


def main():
    command_line = unreal.SystemLibrary.get_command_line().lower()
    if "-run=" in command_line:
        raise RuntimeError("Verification requires a full editor process for collision registration")
    verifier = MainRaidVerifier()
    verifier.start()
    globals()["MAIN_RAID_VERIFIER"] = verifier


if __name__ == "__main__":
    try:
        main()
    except Exception:
        REPORT.parent.mkdir(parents=True, exist_ok=True)
        REPORT.write_text(json.dumps({"passed": False, "error": traceback.format_exc()}, indent=2), encoding="utf-8")
        unreal.log_error(traceback.format_exc())
        unreal.EditorPythonScripting.set_keep_python_script_alive(False)
        unreal.SystemLibrary.quit_editor()
        raise
