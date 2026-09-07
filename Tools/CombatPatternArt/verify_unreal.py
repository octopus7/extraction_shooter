"""Read-only reload verification for the reusable combat-pattern art pack.

Run with UE 5.7 -run=pythonscript -script=<absolute path to this file>.
No Unreal assets, actors, classes, or material graphs are created, edited, or
saved. The only write is Art/CombatPatterns/Validation/unreal_verify.json.
This verifier remains useful after the one-off import/generation code is removed.
"""
from pathlib import Path
import hashlib
import json
import math
import traceback

import unreal


ROOT = Path(__file__).resolve().parents[2]
ART = ROOT / "Art/CombatPatterns"
CONTENT = ROOT / "TunaSweeper/Content/Characters/CombatPatterns"
DEST = "/Game/Characters/CombatPatterns"
REPORT_PATH = ART / "Validation/unreal_verify.json"
EXPECTED_MESHES = {
    "SM_CP_TurretBase", "SM_CP_TurretHead", "SM_CP_TurretTube", "SM_CP_Missile",
    "SM_CP_ChargeChassis", "SM_CP_RobotShell", "SM_CP_RobotEye",
    "SM_CP_RobotLeg", "SM_CP_RobotFoot",
}
PBR_SPECS = {
    "CP_Armor": ((0.68, 0.72, 0.65), 0.42, 0.39),
    "CP_Dark": ((0.032, 0.045, 0.052), 0.76, 0.34),
    "CP_Teal": ((0.018, 0.22, 0.23), 0.48, 0.33),
    "CP_Amber": ((1.0, 0.26, 0.015), 0.15, 0.28),
    "CP_Rubber": ((0.012, 0.017, 0.019), 0.0, 0.77),
}
VFX_SPECS = {
    "M_CP_Effect": unreal.BlendMode.BLEND_TRANSLUCENT,
    "M_CP_Glow": unreal.BlendMode.BLEND_ADDITIVE,
    "M_CP_Smoke": unreal.BlendMode.BLEND_TRANSLUCENT,
}


def package_hashes():
    """Fingerprint saved packages, without opening or changing the packages."""
    return {
        path.relative_to(ROOT).as_posix(): hashlib.sha256(path.read_bytes()).hexdigest()
        for path in sorted(CONTENT.rglob("*"))
        if path.is_file() and path.suffix in {".uasset", ".uexp", ".ubulk", ".umap"}
    }


def check(condition, message):
    if not condition:
        raise AssertionError(message)


def expected_geometry():
    """Normalize the two independently authored source-art manifest formats."""
    entries = {}
    for folder in ("Turret", "Robots"):
        manifest = json.loads((ART / folder / "manifest.json").read_text(encoding="utf-8"))
        for entry in manifest["assets"]:
            name = entry["name"]
            check(name not in entries, "Duplicate source mesh " + name)
            if "bounds_min_cm" in entry:
                minimum, maximum = entry["bounds_min_cm"], entry["bounds_max_cm"]
                size = entry["dimensions_cm"]
            else:
                pairs = entry["bounds_blender_m"]
                minimum = [axis[0] * 100.0 for axis in pairs]
                maximum = [axis[1] * 100.0 for axis in pairs]
                size = entry["dimensions_ue_cm"]
            check(all(math.isfinite(value) for value in minimum + maximum + size), name + " has invalid source bounds")
            entries[name] = {
                "size_cm": size, "minimum_cm": minimum, "maximum_cm": maximum,
                "source_triangles": entry["triangles"],
                "source_material_slots": entry["material_slots"],
            }
    check(set(entries) == EXPECTED_MESHES, "Source manifests must contain exactly the nine combat meshes")
    return entries


def validate_pbr(name, spec):
    path = f"{DEST}/Materials/M_{name}"
    mat = unreal.load_asset(path)
    check(isinstance(mat, unreal.Material), "Missing PBR Material " + path)
    check(mat.get_editor_property("blend_mode") == unreal.BlendMode.BLEND_OPAQUE, name + " must be opaque")
    check(mat.get_editor_property("shading_model") == unreal.MaterialShadingModel.MSM_DEFAULT_LIT, name + " must use Default Lit")
    lib = unreal.MaterialEditingLibrary
    color = lib.get_material_property_input_node(mat, unreal.MaterialProperty.MP_BASE_COLOR)
    check(isinstance(color, unreal.MaterialExpressionConstant3Vector), name + " has no base-color constant")
    value = color.get_editor_property("constant")
    rgb = [value.r, value.g, value.b]
    check(max(abs(a - b) for a, b in zip(rgb, spec[0])) < 0.00001, name + " base color differs")
    values = {}
    for field, prop, expected in (
        ("metallic", unreal.MaterialProperty.MP_METALLIC, spec[1]),
        ("roughness", unreal.MaterialProperty.MP_ROUGHNESS, spec[2]),
    ):
        expression = lib.get_material_property_input_node(mat, prop)
        check(isinstance(expression, unreal.MaterialExpressionConstant), name + " lacks " + field)
        values[field] = expression.get_editor_property("r")
        check(abs(values[field] - expected) < 0.00001, name + " " + field + " differs")
    emissive = lib.get_material_property_input_node(mat, unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    if name == "CP_Amber":
        check(isinstance(emissive, unreal.MaterialExpressionMultiply), name + " has no boosted emissive output")
    return {"name": mat.get_name(), "path": mat.get_path_name(), "base_color_linear": rgb,
            "blend": "opaque", "shading": "default_lit", "emissive_connected": emissive is not None,
            **values}


def validate_vfx(name, blend):
    path = f"{DEST}/Materials/{name}"
    mat = unreal.load_asset(path)
    check(isinstance(mat, unreal.Material), "Missing VFX Material " + path)
    check(mat.get_editor_property("blend_mode") == blend, name + " blend mode differs")
    check(mat.get_editor_property("shading_model") == unreal.MaterialShadingModel.MSM_UNLIT, name + " must be unlit")
    check(mat.get_editor_property("two_sided"), name + " must be two-sided")
    lib = unreal.MaterialEditingLibrary
    emissive = lib.get_material_property_input_node(mat, unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    opacity = lib.get_material_property_input_node(mat, unreal.MaterialProperty.MP_OPACITY)
    check(isinstance(emissive, unreal.MaterialExpressionMultiply), name + " has no scaled emissive output")
    check(isinstance(opacity, unreal.MaterialExpressionVertexColor), name + " opacity is not driven by vertex color")
    inputs = list(lib.get_inputs_for_material_expression(mat, emissive))
    check(len(inputs) == 2, name + " emissive Multiply must have two inputs")
    check(isinstance(inputs[0], unreal.MaterialExpressionVertexColor),
          name + " emissive Multiply A is not connected to vertex color")
    check(isinstance(inputs[1], unreal.MaterialExpressionScalarParameter),
          name + " emissive Multiply B is not connected to a scalar parameter")
    check(inputs[0] == opacity, name + " emissive and opacity use different vertex-color nodes")
    intensity = inputs[1]
    check(str(intensity.get_editor_property("parameter_name")) == "Intensity",
          name + " emissive scalar must be the runtime Intensity parameter")
    intensity_default = intensity.get_editor_property("default_value")
    check(math.isfinite(intensity_default) and intensity_default > 0.0,
          name + " emissive Intensity default must be positive")
    opacity_channel = lib.get_material_property_input_node_output_name(mat, unreal.MaterialProperty.MP_OPACITY)
    check(opacity_channel == "A", name + " opacity must use vertex-color alpha output A")
    return {"name": name, "path": mat.get_path_name(), "blend": str(blend), "shading": "unlit",
            "two_sided": True, "emissive_connected": True, "opacity_vertex_color": True,
            "emissive_multiply_inputs": ["VertexColor", "ScalarParameter:Intensity"],
            "intensity_default": intensity_default, "opacity_source_output": opacity_channel}


def validate_mesh(name, expected, editor):
    path = f"{DEST}/Meshes/{name}"
    mesh = unreal.load_asset(path)
    check(isinstance(mesh, unreal.StaticMesh), "Missing StaticMesh " + path)
    vertices = editor.get_number_verts(mesh, 0)
    triangles = mesh.get_num_triangles(0)
    uvs = editor.get_num_uv_channels(mesh, 0)
    simple = editor.get_simple_collision_count(mesh)
    convex = editor.get_convex_collision_count(mesh)
    check(vertices > 0 and triangles > 0, name + " has no rendered geometry")
    check(uvs >= 1, name + " has no source UV channel")
    check(simple == 0 and convex == 0, f"{name} has unwanted art collision: simple={simple}, convex={convex}")

    assigned = []
    for slot in mesh.get_editor_property("static_materials"):
        slot_name = str(slot.get_editor_property("material_slot_name"))
        if slot_name not in PBR_SPECS:
            slot_name = str(slot.get_editor_property("imported_material_slot_name"))
        check(slot_name in expected["source_material_slots"] and slot_name in PBR_SPECS,
              name + " has an unknown material slot " + slot_name)
        mat = slot.get_editor_property("material_interface")
        expected_path = f"{DEST}/Materials/M_{slot_name}.M_{slot_name}"
        check(mat is not None and mat.get_path_name() == expected_path,
              name + " slot " + slot_name + " does not reference its shared PBR material")
        assigned.append({"slot": slot_name, "path": mat.get_path_name()})
    # FBX intentionally drops unused material slots, e.g. CP_Armor on RobotEye.
    check(len(assigned) > 0, name + " has no material slots")

    bounds = mesh.get_bounds()
    center = [bounds.origin.x, bounds.origin.y, bounds.origin.z]
    extent = [bounds.box_extent.x, bounds.box_extent.y, bounds.box_extent.z]
    size = [value * 2.0 for value in extent]
    minimum = [center[i] - extent[i] for i in range(3)]
    maximum = [center[i] + extent[i] for i in range(3)]
    check(all(math.isfinite(value) for value in center + size), name + " has nonfinite bounds")
    size_error = max(abs(a - b) for a, b in zip(size, expected["size_cm"]))
    bounds_error = max(abs(a - b) for a, b in zip(minimum + maximum, expected["minimum_cm"] + expected["maximum_cm"]))
    check(size_error < 0.1, f"{name} centimeter dimensions differ by {size_error}")
    check(bounds_error < 0.1, f"{name} local pivot or orientation differs by {bounds_error} cm")
    if name == "SM_CP_RobotEye":
        check(center[0] > 15.0 and abs(center[1]) < 0.1, "RobotEye must face physical UE +X")

    build = editor.get_lod_build_settings(mesh, 0)
    check(not build.get_editor_property("recompute_normals"), name + " does not preserve authored normals")
    check(build.get_editor_property("recompute_tangents"), name + " lacks generated tangents")
    return {
        "name": name, "path": mesh.get_path_name(), "size_cm": size,
        "bounds_origin_cm": center, "bounds_min_cm": minimum, "bounds_max_cm": maximum,
        "expected_size_cm": expected["size_cm"], "max_size_error_cm": size_error,
        "max_local_bounds_error_cm": bounds_error, "render_vertices_lod0": vertices,
        "render_triangles_lod0": triangles, "source_triangles": expected["source_triangles"],
        "source_uv_channels": uvs,
        "lightmap_generation_enabled": bool(build.get_editor_property("generate_lightmap_u_vs")),
        "lightmap_coordinate_index": mesh.get_editor_property("light_map_coordinate_index"),
        "simple_collisions": simple, "convex_collisions": convex, "materials": assigned,
        "authored_normals_preserved": True, "tangents_generated": True,
    }


def main():
    report = {
        "passed": False, "engine": unreal.SystemLibrary.get_engine_version(),
        "destination": DEST, "verification": "independent saved-package reload; no native CDO checks",
        "read_only_asset_check": True, "meshes": [], "pbr_materials": [], "vfx_materials": [], "errors": [],
    }
    before = package_hashes()
    try:
        geometry = expected_geometry()
        check(len(before) >= 17, "Expected nine mesh and eight material packages on disk")
        editor = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem) or unreal.get_default_object(unreal.StaticMeshEditorSubsystem)
        check(editor is not None, "StaticMeshEditorSubsystem is unavailable")
        for name, spec in PBR_SPECS.items():
            try:
                report["pbr_materials"].append(validate_pbr(name, spec))
            except Exception as error:
                report["errors"].append({"asset": name, "error": str(error)})
        for name, blend in VFX_SPECS.items():
            try:
                report["vfx_materials"].append(validate_vfx(name, blend))
            except Exception as error:
                report["errors"].append({"asset": name, "error": str(error)})
        for name in sorted(geometry):
            try:
                report["meshes"].append(validate_mesh(name, geometry[name], editor))
            except Exception as error:
                report["errors"].append({"asset": name, "error": str(error)})
    except Exception as error:
        report["errors"].append({"stage": "verification setup", "error": str(error), "traceback": traceback.format_exc()})
    after = package_hashes()
    report["asset_packages_unchanged"] = before == after
    report["asset_packages_sha256"] = after
    if before != after:
        report["errors"].append({"stage": "read-only package check", "error": "Saved art package hashes changed during verification"})
    report["counts"] = {"meshes": len(report["meshes"]), "pbr_materials": len(report["pbr_materials"]), "vfx_materials": len(report["vfx_materials"])}
    report["passed"] = not report["errors"] and report["counts"] == {"meshes": 9, "pbr_materials": 5, "vfx_materials": 3}
    REPORT_PATH.parent.mkdir(parents=True, exist_ok=True)
    REPORT_PATH.write_text(json.dumps(report, indent=2), encoding="utf-8")
    if not report["passed"]:
        raise RuntimeError("COMBAT_PATTERN_ART_VERIFY_FAILED " + json.dumps(report["errors"]) + " Report: " + str(REPORT_PATH))
    unreal.log("COMBAT_PATTERN_ART_VERIFY_PASSED " + str(REPORT_PATH))


if __name__ == "__main__":
    main()
