"""Reload the saved SSTO map and verify structure and registered collision.

Run in a full editor process with -ExecutePythonScript, not PythonScript commandlet.
This script never saves maps, assets, or editor settings. Results go under Saved.
"""

from __future__ import annotations

import hashlib
import json
import math
from pathlib import Path
import time
import traceback

import unreal


ROOT = Path(__file__).resolve().parents[2]
SAVED = ROOT / "TunaSweeper/Saved/SSTOLander_20261003"
REPORT = SAVED / "verification.json"
PLACEMENT = ROOT / "TunaSweeper/SourceArt/Environment/SSTO_Lander/Manifests/SSTO_Placement.json"
MANIFEST = ROOT / "TunaSweeper/SourceArt/Environment/SSTO_Lander/Manifests/SSTO_Lander.json"
MAP_PATH = "/Game/MainRaid/RaidPlains"
ASSET_ROOT = "/Game/MainRaid/SSTO/"
SHIP_BP = ASSET_ROOT + "BP_SSTOLander"
SHELL_BP = ASSET_ROOT + "BP_SSTOUpperShell"
REVEAL_MATERIAL = ASSET_ROOT + "Materials/M_SSTO_VerticalReveal"
ENDPOINT_CLASS = "/Script/TunaSweeper.TunaSweeperLadderTransferActor"
REVEAL_CLASS = "/Script/TunaSweeper.TunaSweeperVerticalOcclusionRevealComponent"
RADIUS_CM = 34.0
HALF_HEIGHT_CM = 88.0
FLOOR_TOLERANCE_CM = 12.0
CAPSULE_FLOOR_GAP_CM = 2.0
MAX_WALK_SLOPE_DEGREES = 44.0


def vector(value):
    return [round(value.x, 4), round(value.y, 4), round(value.z, 4)]


def rotation(value):
    return [round(value.pitch, 4), round(value.yaw, 4), round(value.roll, 4)]


def close(actual, expected, tolerance=0.1):
    return len(actual) == len(expected) and all(abs(a - b) <= tolerance for a, b in zip(actual, expected))


def package_hashes():
    content = ROOT / "TunaSweeper/Content/MainRaid"
    paths = list(content.glob("*.umap")) + list((content / "SSTO").rglob("*.uasset"))
    return {str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(paths)}


def hit_result(result):
    if isinstance(result, unreal.HitResult):
        return result.to_tuple() if result.to_tuple()[0] else None
    if isinstance(result, tuple):
        hit = next((value for value in result if isinstance(value, unreal.HitResult)), None)
        return hit.to_tuple() if hit and hit.to_tuple()[0] else None
    return None


def mesh_components(actor):
    return [c for c in actor.get_components_by_class(unreal.StaticMeshComponent) if c.static_mesh]


def material_chain(material):
    result = []
    while material:
        path = material.get_path_name()
        assert path not in result, f"Cyclic material ancestry: {path}"
        result.append(path)
        if isinstance(material, unreal.Material):
            break
        material = material.get_editor_property("parent")
    return result


def actor_snapshot(actor):
    center, extent = actor.get_actor_bounds(False)
    return {
        "label": actor.get_actor_label(),
        "location_cm": vector(actor.get_actor_location()),
        "rotation_pyr": rotation(actor.get_actor_rotation()),
        "scale3d": vector(actor.get_actor_scale3d()),
        "bounds_center_cm": vector(center),
        "bounds_extent_cm": vector(extent),
    }


def densify_path(points, spacing_cm=40.0):
    assert len(points) >= 2, "Each floor path needs at least two points"
    result = [list(points[0])]
    for start, end in zip(points, points[1:]):
        distance = math.dist(start[:2], end[:2])
        steps = max(1, math.ceil(distance / spacing_cm))
        for step in range(1, steps + 1):
            alpha = step / steps
            result.append([a + (b - a) * alpha for a, b in zip(start, end)])
    return result


class Verifier:
    def __init__(self):
        self.before = package_hashes()
        self.started = time.monotonic()
        self.state = "load"
        self.frames = 0
        self.busy = False
        self.report = {
            "passed": False,
            "engine_version": unreal.SystemLibrary.get_engine_version(),
            "verification": "fresh saved-map reload, structure, floor traces and Pawn capsule sweeps",
            "capsule_radius_cm": RADIUS_CM,
            "capsule_half_height_cm": HALF_HEIGHT_CM,
            "runtime_interaction_tested": False,
            "checks": {},
        }
        self.levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
        self.editor_actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
        assert self.levels and self.editor_actors, "Use a full Unreal Editor process"
        unreal.EditorPythonScripting.set_keep_python_script_alive(True)
        self.handle = unreal.register_slate_post_tick_callback(self.tick)

    def tick(self, _delta):
        if self.busy:
            return
        self.busy = True
        try:
            assert time.monotonic() - self.started < 300, "SSTO verification timed out"
            if self.state == "load":
                assert MANIFEST.is_file(), f"Missing source manifest: {MANIFEST}"
                assert PLACEMENT.is_file(), f"Missing placement evidence: {PLACEMENT}"
                self.manifest = json.loads(MANIFEST.read_text(encoding="utf-8"))
                self.placement = json.loads(PLACEMENT.read_text(encoding="utf-8"))
                assert self.placement.get("passed"), "Placement report did not pass"
                assert self.levels.load_level(MAP_PATH), f"Unable to load {MAP_PATH}"
                self.state = "settle"
            else:
                self.frames += 1
                if self.frames >= 30:
                    self.validate()
                    self.finish()
        except Exception:
            self.finish(traceback.format_exc())
        finally:
            self.busy = False

    def validate(self):
        unreal.AutomationLibrary.finish_loading_before_screenshot()
        self.world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
        self.actors = list(self.editor_actors.get_all_level_actors())
        labels = {}
        for actor in self.actors:
            labels.setdefault(actor.get_actor_label(), []).append(actor)

        def unique(label):
            matches = labels.get(label, [])
            assert len(matches) == 1, f"Expected one {label}, found {len(matches)}"
            return matches[0]

        ship = unique("TS_Plains_SSTOLander")
        assert not labels.get("TS_Plains_SSTOBlockout"), "Old SSTO blockout still exists"
        assert ship.get_class() == unreal.load_class(None, SHIP_BP + ".BP_SSTOLander_C"), "Wrong ship Blueprint class"
        snapshot = actor_snapshot(ship)
        assert math.hypot(*snapshot["location_cm"][:2]) <= 1.0, "Ship is not centered on the map"
        pitch, yaw, roll = snapshot["rotation_pyr"]
        assert abs(yaw - 35.0) <= 1.0, f"Unexpected heading: {yaw}"
        assert 1.0 <= abs(pitch) <= 10.0 and 4.0 <= abs(roll) <= 12.0, "Ship must retain visible, walkable tilt"
        assert close(snapshot["scale3d"], [1, 1, 1]), "Ship scale must preserve authored centimeters"
        for key in ("location_cm", "rotation_pyr"):
            assert close(snapshot[key], self.placement["ship"][key]), f"Saved ship {key} differs from placement report"

        children = list(ship.get_components_by_class(unreal.ChildActorComponent))
        shell_class = unreal.load_class(None, SHELL_BP + ".BP_SSTOUpperShell_C")
        matching = [c for c in children if c.get_editor_property("child_actor_class") == shell_class]
        assert len(matching) == 1, "Expected one upper-shell child actor"
        shell = matching[0].get_editor_property("child_actor")
        assert shell, "Upper-shell child actor was not instantiated after map reload"
        lower_parts, upper_parts = mesh_components(ship), mesh_components(shell)
        assert lower_parts, "Ship has no permanent visible mesh parts"
        assert len(upper_parts) == 3, f"Expected roof, upper hull and glass; found {len(upper_parts)} meshes"
        expected_reveal = {part["name"] for part in self.manifest["parts"] if part["reveal"]}
        assert {c.static_mesh.get_name() for c in upper_parts} == expected_reveal, "Upper-shell reveal parts differ from source manifest"
        reveal_cls = unreal.load_class(None, REVEAL_CLASS)
        assert reveal_cls, "Vertical reveal native class is unavailable"
        reveal = shell.get_components_by_class(reveal_cls)
        assert len(reveal) == 1, "Upper shell must use one vertical reveal component"
        assert not ship.get_components_by_class(reveal_cls), "Permanent hull must remain outside upper-shell reveal"
        preserve_property = "preserve_source_materials"
        assert reveal[0].get_editor_property(preserve_property), "Upper shell must preserve source palette materials"
        self.report["checks"]["ship"] = dict(snapshot, permanent_components=len(lower_parts), reveal_components=len(upper_parts))

        self.report['built_triangle_counts']={c.static_mesh.get_name():c.static_mesh.get_num_triangles(0) for c in lower_parts+upper_parts}
        self.check_meshes(ship, lower_parts + upper_parts, upper_parts)
        self.check_boundaries(labels)
        self.check_endpoints(unique)
        self.report["checks"]["interior"] = self.check_paths("interior_floor_paths_cm", ship, shell)
        self.report["checks"]["exterior"] = self.check_paths("external_approach_paths_cm", ship, shell)
        unreal.SystemLibrary.execute_console_command(self.world, "MAP CHECK")
        self.report["map_check_requested"] = True

    def check_meshes(self, ship, components, upper_parts):
        meshes = {c.static_mesh.get_path_name(): c.static_mesh for c in components}
        source_parts = {part["name"]: part for part in self.manifest["parts"]}
        assert {mesh.get_name() for mesh in meshes.values()} == set(source_parts), "Missing or extra source mesh parts"
        assert all(path.startswith(ASSET_ROOT) for path in meshes), "Ship references meshes outside its own asset folder"
        mesh_editor = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
        inverse = ship.get_actor_transform()
        local_points = []
        entries = []
        material_paths = set()
        checked_materials = {}
        for component in components:
            bounds = component.static_mesh.get_bounds()
            transform = component.get_world_transform()
            for sx in (-1, 1):
                for sy in (-1, 1):
                    for sz in (-1, 1):
                        point = bounds.origin + unreal.Vector(bounds.box_extent.x * sx, bounds.box_extent.y * sy, bounds.box_extent.z * sz)
                        world_point = unreal.MathLibrary.transform_location(transform, point)
                        local_points.append(vector(unreal.MathLibrary.inverse_transform_location(inverse, world_point)))
            mesh = component.static_mesh
            source = source_parts[mesh.get_name()]
            flag = mesh_editor.get_collision_complexity(mesh)
            if source["collision"]:
                assert flag == unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE, f"Mesh must use authored surface collision: {mesh.get_path_name()} ({flag})"
                assert component.get_collision_enabled() != unreal.CollisionEnabled.NO_COLLISION, f"Collision disabled on {component.get_name()}"
                assert component.get_collision_response_to_channel(unreal.CollisionChannel.ECC_PAWN) == unreal.CollisionResponseType.ECR_BLOCK, f"Pawn collision disabled on {component.get_name()}"
            else:
                assert component.get_collision_enabled() == unreal.CollisionEnabled.NO_COLLISION, f"Source specifies no collision on {component.get_name()}"
            assert mesh.get_num_triangles(0) == source['unreal_triangles'], f"Imported built triangle count differs: {mesh.get_path_name()} actual={mesh.get_num_triangles(0)} expected={source['unreal_triangles']}"
            actual_bounds = vector(bounds.origin - bounds.box_extent) + vector(bounds.origin + bounds.box_extent)
            assert close(actual_bounds, source["bounds_cm"], 0.1), f"Imported bounds/axes differ from source: {mesh.get_path_name()}"
            for slot in mesh.static_materials:
                material = slot.material_interface
                assert material, f"Unassigned material on {mesh.get_path_name()}"
                assert material.get_path_name().startswith(ASSET_ROOT), f"Non-SSTO mesh material: {material.get_path_name()}"
                material_paths.add(material.get_path_name())
                source_name = str(slot.material_slot_name)
                if source_name not in self.manifest["materials"]:
                    source_name = str(slot.imported_material_slot_name)
                assert source_name in source["materials"], f"Unexpected material slot on {mesh.get_path_name()}: {source_name}"
                if material.get_path_name() not in checked_materials:
                    checked_materials[material.get_path_name()] = self.check_palette(material, source_name)
            if component in upper_parts:
                for material in component.get_materials():
                    chain = material_chain(material)
                    assert any(path.split(".")[0] == REVEAL_MATERIAL for path in chain), f"Upper shell does not inherit the reveal shader: {chain}"
            entries.append({"component": component.get_name(), "mesh": mesh.get_path_name(), "triangles": mesh.get_num_triangles(0), "collision": str(flag)})
        minimum = [min(point[axis] for point in local_points) for axis in range(3)]
        maximum = [max(point[axis] for point in local_points) for axis in range(3)]
        size = [hi - lo for lo, hi in zip(minimum, maximum)]
        assert 2200 <= size[0] <= 3100 and 1500 <= size[1] <= 2200, f"Unexpected local craft size in cm: {size}"
        material = unreal.load_asset(REVEAL_MATERIAL)
        assert isinstance(material, unreal.Material), "Reveal base material missing"
        assert material.get_editor_property("blend_mode") == unreal.BlendMode.BLEND_MASKED
        parameters = set(str(name) for name in unreal.MaterialEditingLibrary.get_scalar_parameter_names(material))
        assert {"VerticalRevealActive", "VerticalRevealStartZ", "VerticalRevealFadeHeightCm"} <= parameters
        assert unreal.MaterialEditingLibrary.get_material_property_input_node(material, unreal.MaterialProperty.MP_OPACITY_MASK), "Reveal material has no opacity-mask graph"
        self.report["checks"]["assets"] = {"meshes": entries, "materials": sorted(material_paths), "palette": checked_materials, "local_bounds_min_cm": minimum, "local_bounds_max_cm": maximum, "local_size_cm": size}

    def check_palette(self, material, source_name):
        assert isinstance(material, unreal.MaterialInstanceConstant), f"Expected authored palette MI: {material.get_path_name()}"
        expected = self.manifest["materials"][source_name]
        lib = unreal.MaterialEditingLibrary
        color = lib.get_material_instance_vector_parameter_value(material, "BaseColor")
        actual = {"BaseColor": [color.r, color.g, color.b, color.a]}
        assert close(actual["BaseColor"], expected["base_color"], 0.0001), f"Palette color differs for {source_name}"
        for parameter, field in (("Metallic", "metallic"), ("Roughness", "roughness"), ("EmissionStrength", "emission_strength")):
            actual[parameter] = lib.get_material_instance_scalar_parameter_value(material, parameter)
            assert abs(actual[parameter] - expected[field]) < 0.0001, f"Palette {parameter} differs for {source_name}"
        return actual

    def check_boundaries(self, labels):
        expected = self.placement["preserved_boundaries"]
        expected_labels = {f"TS_Plains_Boundary_{side}" for side in ("North", "South", "East", "West")}
        assert {item["label"] for item in expected} == expected_labels, "Placement must contain all four original boundary snapshots"
        actual = []
        for before in expected:
            matches = labels.get(before["label"], [])
            assert len(matches) == 1 and isinstance(matches[0], unreal.BlockingVolume), f"Boundary missing: {before['label']}"
            actor = matches[0]
            after = actor_snapshot(actor)
            for field in ("location_cm", "rotation_pyr", "scale3d", "bounds_center_cm", "bounds_extent_cm"):
                assert close(after[field], before[field]), f"Boundary modified: {before['label']} {field}"
            assert actor.get_actor_enable_collision() and not actor.get_editor_property("is_editor_only_actor")
            brush = actor.get_component_by_class(unreal.BrushComponent)
            assert str(brush.get_collision_profile_name()) == "InvisibleWall"
            assert brush.get_collision_response_to_channel(unreal.CollisionChannel.ECC_PAWN) == unreal.CollisionResponseType.ECR_BLOCK
            actual.append(after)
        self.report["checks"]["preserved_boundaries"] = actual

    def check_endpoints(self, unique):
        endpoint_cls = unreal.load_class(None, ENDPOINT_CLASS)
        assert endpoint_cls, "Native ladder class is unavailable; compile before verification"
        entry, exit_actor = unique("TS_Plains_SSTO_Entry"), unique("TS_Plains_SSTO_Exit")
        entries = []
        for actor, other in ((entry, exit_actor), (exit_actor, entry)):
            assert actor.get_class() == endpoint_cls, f"Wrong native endpoint class: {actor.get_actor_label()}"
            assert actor.get_editor_property("target_endpoint") == other, f"Ladder endpoint is not paired: {actor.get_actor_label()}"
            assert abs(actor.get_editor_property("max_source_height_difference_cm") - 120.0) < 0.01
            assert abs(actor.get_editor_property("arrival_clearance_cm") - CAPSULE_FLOOR_GAP_CM) < 0.01
            arrival = actor.get_editor_property("arrival_point")
            assert isinstance(arrival, unreal.SceneComponent), "ArrivalPoint must be a floor-level SceneComponent"
            location = vector(arrival.get_world_location())
            floor = self.floor_trace(location)
            assert abs(floor[5].z - location[2]) <= FLOOR_TOLERANCE_CM, f"Arrival point is not on its floor: {actor.get_actor_label()}"
            self.clear_capsule(location, location)
            entries.append({"label": actor.get_actor_label(), "target": other.get_actor_label(), "arrival_point_cm": location, "ground_z_cm": floor[5].z})
        self.report["checks"]["endpoints"] = entries

    def floor_trace(self, point, exact_height=True):
        start = unreal.Vector(point[0], point[1], point[2] + 40.0)
        end = unreal.Vector(point[0], point[1], point[2] - 100.0)
        result = unreal.SystemLibrary.line_trace_single(self.world, start, end, unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, False, [], unreal.DrawDebugTrace.NONE, True)
        hit = hit_result(result)
        assert hit, f"No registered floor collision at {point}"
        assert not exact_height or abs(hit[5].z - point[2]) <= FLOOR_TOLERANCE_CM, f"Floor differs from placement sample at {point}: hit Z={hit[5].z}"
        normal = hit[7]
        slope = math.degrees(math.acos(max(-1.0, min(1.0, normal.z))))
        assert slope <= MAX_WALK_SLOPE_DEGREES, f"Floor too steep at {point}: {slope:.2f} degrees"
        return hit

    def clear_capsule(self, start_floor, end_floor):
        offset = HALF_HEIGHT_CM + CAPSULE_FLOOR_GAP_CM
        start = unreal.Vector(start_floor[0], start_floor[1], start_floor[2] + offset)
        end = unreal.Vector(end_floor[0], end_floor[1], end_floor[2] + offset)
        if start == end:
            end.z += 0.1
        result = unreal.SystemLibrary.capsule_trace_single_by_profile(self.world, start, end, RADIUS_CM, HALF_HEIGHT_CM, "Pawn", False, [], unreal.DrawDebugTrace.NONE, True)
        hit = hit_result(result)
        if hit:
            actor = hit[9]
            label = actor.get_actor_label() if actor else "unknown actor"
            raise AssertionError(f"Pawn capsule blocked between {start_floor} and {end_floor}: {label}, start_penetrating={hit[1]}")

    def check_paths(self, key, ship, shell):
        paths = self.placement.get(key, [])
        assert paths, f"Placement report requires nonempty {key}"
        records = []
        for index, authored in enumerate(paths):
            samples = densify_path(authored)
            floors = []
            for point in samples:
                hit = self.floor_trace(point, key == "interior_floor_paths_cm")
                if key == "interior_floor_paths_cm":
                    assert hit[9] in (ship, shell), f"Interior sample hits an actor outside the SSTO: {point}"
                floor = [point[0], point[1], hit[5].z]
                self.clear_capsule(floor, floor)
                floors.append(floor)
            maximum_slope = 0.0
            for start, end in zip(floors, floors[1:]):
                distance = math.dist(start[:2], end[:2])
                if distance > 0.01:
                    slope = math.degrees(math.atan2(abs(end[2] - start[2]), distance))
                    maximum_slope = max(maximum_slope, slope)
                    assert slope <= MAX_WALK_SLOPE_DEGREES, f"Unwalkable step/slope in {key} path {index}: {slope:.2f}"
                self.clear_capsule(start, end)
            records.append({"path": index, "sample_count": len(samples), "capsule_tests": len(samples) * 2 - 1, "max_sampled_slope_degrees": maximum_slope, "floor_samples_cm": floors})
        return records

    def finish(self, error=None):
        unreal.unregister_slate_post_tick_callback(self.handle)
        after = package_hashes()
        changed = sorted(path for path in set(self.before) | set(after) if self.before.get(path) != after.get(path))
        self.report["asset_packages_unchanged"] = not changed
        self.report["changed_asset_packages"] = changed
        self.report["elapsed_seconds"] = round(time.monotonic() - self.started, 3)
        self.report["passed"] = error is None and not changed
        if error:
            self.report["error"] = error
            unreal.log_error(error)
        REPORT.parent.mkdir(parents=True, exist_ok=True)
        REPORT.write_text(json.dumps(self.report, indent=2), encoding="utf-8")
        unreal.log("SSTO_VERIFICATION_" + ("PASSED " if self.report["passed"] else "FAILED ") + str(REPORT))
        unreal.EditorPythonScripting.set_keep_python_script_alive(False)
        unreal.SystemLibrary.quit_editor()


def main():
    assert "-run=" not in unreal.SystemLibrary.get_command_line().lower(), "Use -ExecutePythonScript for registered collision"
    globals()["SSTO_VERIFIER"] = Verifier()


if __name__ == "__main__":
    try:
        main()
    except Exception:
        REPORT.parent.mkdir(parents=True, exist_ok=True)
        REPORT.write_text(json.dumps({"passed": False, "error": traceback.format_exc()}, indent=2), encoding="utf-8")
        unreal.log_error(traceback.format_exc())
        unreal.EditorPythonScripting.set_keep_python_script_alive(False)
        unreal.SystemLibrary.quit_editor()
