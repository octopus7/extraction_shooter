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
# Project convention: +X is north, +Y is east (Docs/game_conventions.md).
BOUNDARY_SIDES = {"North": (0, 1), "South": (0, -1), "East": (1, 1), "West": (1, -1)}
BOUNDARY_INNER_CM = 25000.0
BOUNDARY_OUTER_CM = 26000.0
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
        return bool(result.to_tuple()[0]), result
    if isinstance(result, tuple):
        hit = next((item for item in result if isinstance(item, unreal.HitResult)), None)
        flag = next((item for item in result if isinstance(item, bool)), hit is not None)
        return bool(flag) and hit is not None and bool(hit.to_tuple()[0]), hit
    return bool(result), None


def validate_boundaries(name, world, all_actors, landscape):
    """Check saved brush geometry and exercise its registered Pawn collision."""
    region = name.removeprefix("Raid")
    expected = {f"TS_{region}_Boundary_{side}" for side in BOUNDARY_SIDES}
    volumes = [actor for actor in all_actors if isinstance(actor, unreal.BlockingVolume)]
    assert len(volumes) == 4, f"{name}: expected four BlockingVolumes, found {len(volumes)}"
    by_label = {actor.get_actor_label(): actor for actor in volumes}
    assert set(by_label) == expected, f"{name}: boundary labels {sorted(by_label)}"
    ignore_except_boundaries = [actor for actor in all_actors if actor not in volumes]
    ignore_except_landscape = [actor for actor in all_actors if actor != landscape]
    land_origin, land_extent = landscape.get_actor_bounds(False)
    floor = min(-5000.0, land_origin.z - land_extent.z - 5000.0)
    ceiling = max(20000.0, land_origin.z + land_extent.z + 10000.0)
    bounds = {}
    volume_report = []

    for side, (axis, sign) in BOUNDARY_SIDES.items():
        label = f"TS_{region}_Boundary_{side}"
        actor = by_label[label]
        assert str(actor.get_folder_path()) == "MapBoundary", f"{label}: wrong Outliner folder"
        assert not actor.get_editor_property("is_editor_only_actor"), f"{label}: editor-only actor"
        assert actor.get_editor_property("hidden"), f"{label}: must be hidden in game"
        assert actor.get_actor_enable_collision(), f"{label}: actor collision disabled"
        brushes = actor.get_components_by_class(unreal.BrushComponent)
        assert len(brushes) == 1, f"{label}: expected one BrushComponent"
        brush = brushes[0]
        assert not brush.get_editor_property("is_editor_only"), f"{label}: editor-only brush"
        builder = actor.get_editor_property("brush_builder")
        assert builder and builder.get_class().get_name() == "CubeBuilder", f"{label}: expected box brush"
        assert all(builder.get_editor_property(axis_name) == 200.0 for axis_name in ("x", "y", "z")), (
            f"{label}: unexpected source box dimensions"
        )
        assert str(brush.get_collision_profile_name()) == "InvisibleWall", f"{label}: wrong collision profile"
        assert brush.get_collision_enabled() == unreal.CollisionEnabled.QUERY_AND_PHYSICS, f"{label}: wrong collision mode"
        assert brush.get_collision_object_type() == unreal.CollisionChannel.ECC_WORLD_STATIC, f"{label}: wrong object type"
        for channel, response in (
            (unreal.CollisionChannel.ECC_PAWN, unreal.CollisionResponseType.ECR_BLOCK),
            (unreal.CollisionChannel.ECC_VISIBILITY, unreal.CollisionResponseType.ECR_IGNORE),
            (unreal.CollisionChannel.ECC_CAMERA, unreal.CollisionResponseType.ECR_BLOCK),
        ):
            assert brush.get_collision_response_to_channel(channel) == response, f"{label}: wrong {channel} response"

        origin, extent, _radius = unreal.SystemLibrary.get_component_bounds(brush)
        lower = [origin.x - extent.x, origin.y - extent.y, origin.z - extent.z]
        upper = [origin.x + extent.x, origin.y + extent.y, origin.z + extent.z]
        expected_lower = [-BOUNDARY_OUTER_CM, -BOUNDARY_OUTER_CM, floor]
        expected_upper = [BOUNDARY_OUTER_CM, BOUNDARY_OUTER_CM, ceiling]
        expected_lower[axis] = BOUNDARY_INNER_CM if sign > 0 else -BOUNDARY_OUTER_CM
        expected_upper[axis] = BOUNDARY_OUTER_CM if sign > 0 else -BOUNDARY_INNER_CM
        assert all(abs(actual - target) <= 1.0 for actual, target in zip(lower + upper, expected_lower + expected_upper)), (
            f"{label}: brush bounds {lower}, {upper}; expected {expected_lower}, {expected_upper}"
        )
        bounds[side] = (lower, upper)
        volume_report.append({
            "label": label, "bounds_min_cm": lower, "bounds_max_cm": upper,
            "brush_shape": "Box", "collision_profile": "InvisibleWall",
            "pawn_response": "Block", "visibility_response": "Ignore", "camera_response": "Block",
            "runtime_collision_enabled": True,
        })

    for x_side in ("North", "South"):
        for y_side in ("East", "West"):
            a_min, a_max = bounds[x_side]
            b_min, b_max = bounds[y_side]
            assert all(min(a_max[i], b_max[i]) - max(a_min[i], b_min[i]) >= 999.0 for i in range(3)), (
                f"{name}: {x_side}/{y_side} corner does not have overlapping collision"
            )

    def ground_height(x, y):
        result = unreal.SystemLibrary.line_trace_single(
            world, unreal.Vector(x, y, ceiling), unreal.Vector(x, y, floor),
            unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, False, ignore_except_landscape,
            unreal.DrawDebugTrace.NONE, True,
        )
        hit, detail = blocking_hit(result)
        assert hit and detail is not None, f"{name}: no landscape collision near perimeter at {(x, y)}"
        return detail.to_tuple()[5].z

    def sweep(start, end, expected_labels):
        result = unreal.SystemLibrary.capsule_trace_single_by_profile(
            world, start, end, 42.0, 90.0, "Pawn", False,
            ignore_except_boundaries, unreal.DrawDebugTrace.NONE, True,
        )
        hit, detail = blocking_hit(result)
        if not expected_labels:
            assert not hit, f"{name}: inner control sweep unexpectedly blocked"
            return None
        assert hit and detail is not None, f"{name}: escaped perimeter from {vector(start)} to {vector(end)}"
        # Read bBlockingHit from the struct: a returned HitResult alone does not mean a hit.
        assert detail.to_tuple()[0], f"{name}: perimeter returned a non-blocking hit"
        broken = detail.to_tuple()
        hit_actor = broken[9]
        hit_label = hit_actor.get_actor_label() if hit_actor else None
        assert hit_label in expected_labels, f"{name}: expected {expected_labels}, hit {hit_label}"
        assert not broken[1], f"{name}: perimeter sweep began inside collision at {vector(start)}"
        return hit_label

    samples = []
    offsets = [-24500.0] + list(range(-22500, 22501, 2500)) + [24500.0]
    for side, (axis, sign) in BOUNDARY_SIDES.items():
        expected_label = f"TS_{region}_Boundary_{side}"
        for offset in offsets:
            start_xy = [float(offset), float(offset)]
            end_xy = list(start_xy)
            start_xy[axis] = sign * (BOUNDARY_INNER_CM - 300.0)
            end_xy[axis] = sign * (BOUNDARY_OUTER_CM + 300.0)
            near_ground = ground_height(*start_xy) + 105.0
            assert floor + 90.0 < near_ground < ceiling - 90.0, f"{name}: wall does not cover perimeter ground"
            for height_name, z in (("near_ground", near_ground), ("high", ceiling - 200.0)):
                start, end = unreal.Vector(*start_xy, z), unreal.Vector(*end_xy, z)
                hit_label = sweep(start, end, {expected_label})
                samples.append({"side": side, "height": height_name, "start_cm": vector(start), "hit_actor": hit_label})

    for x_side, x_sign in (("North", 1), ("South", -1)):
        for y_side, y_sign in (("East", 1), ("West", -1)):
            x, y = x_sign * 24700.0, y_sign * 24700.0
            for height_name, z in (("near_ground", ground_height(x, y) + 105.0), ("high", ceiling - 200.0)):
                start, end = unreal.Vector(x, y, z), unreal.Vector(x_sign * 26300.0, y_sign * 26300.0, z)
                hit_label = sweep(start, end, {f"TS_{region}_Boundary_{x_side}", f"TS_{region}_Boundary_{y_side}"})
                samples.append({"corner": f"{x_side}{y_side}", "height": height_name, "start_cm": vector(start), "hit_actor": hit_label})

    for z in (ground_height(0.0, 0.0) + 105.0, ceiling - 200.0):
        sweep(unreal.Vector(0, 0, z), unreal.Vector(1000, 1000, z), set())
    return {
        "volume_count": len(volumes), "volumes": volume_report,
        "inner_limit_cm": BOUNDARY_INNER_CM, "corner_overlap_verified": True,
        "registered_collision_verified_by_pawn_sweeps": True,
        "outward_capsule_sweep_count": len(samples), "outward_capsule_sweeps": samples,
        "interior_control_sweep_count": 2, "interior_controls_clear": True,
    }


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

        boundary_report = validate_boundaries(name, world, all_actors, landscape)
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
            "boundaries": boundary_report,
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
