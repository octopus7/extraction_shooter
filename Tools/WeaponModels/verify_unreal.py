"""Read-only UE 5.7 reload validation for the Tuna weapon model pack.

This module never imports, creates, modifies, or saves Unreal assets.  It is
kept after the one-off importer is removed.
"""
from pathlib import Path
import hashlib
import json
import math
import unreal


ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / "TunaSweeper/SourceArt/Weapons/TunaWeaponCollection"
# The script may intentionally run against the user's original project because
# a detached worktree has no compiled third-party editor modules.  Source art
# stays in this task; protected/content paths follow the project UE loaded.
PROJECT_ROOT = Path(unreal.Paths.project_dir()).resolve().parent
DEST = "/Game/Weapons/TunaWeaponCollection"
TEXTURE_NAME = "T_TunaWeaponCollection_Atlas"
TEXTURE_PATH = f"{DEST}/Textures/{TEXTURE_NAME}"
MASTER_PATH = f"{DEST}/Materials/M_TunaWeapon_Master"
SHOWCASE_PATH = f"{DEST}/Maps/L_TunaWeaponCollection_Showcase"
IMPORT_REPORT = SOURCE / "unreal_import_validation.json"


def read_manifest():
    manifest = json.loads((SOURCE / "model_manifest.json").read_text(encoding="utf-8"))
    assert len(manifest["assets"]) == 6 and manifest["materials"], "Expected six weapon meshes"
    names = [entry["name"] for entry in manifest["assets"]]
    assert len(names) == len(set(names))
    assert {entry["category"] for entry in manifest["assets"]} == {"SMG", "AR", "Pistol"}
    assert {entry["style"] for entry in manifest["assets"]} == {"Standard", "Premium"}
    for entry in manifest["assets"]:
        assert entry["name"].replace("_", "").isalnum(), entry["name"]
        bounds = entry["bounds_cm"]
        assert len(bounds) == 6 and all(math.isfinite(value) for value in bounds)
        assert all(bounds[i + 3] > bounds[i] for i in range(3))
        assert max(abs(bounds[i + 3] - bounds[i] - entry["size_cm"][i]) for i in range(3)) < 0.1
        sockets = {socket["name"]: socket for socket in entry["sockets"]}
        assert set(sockets) == {"MuzzleSocket", "LaserSightSocket", "ShellEjectionSocket"}
        assert sockets["MuzzleSocket"]["location_cm"][0] >= bounds[3] - 1.5
        assert sockets["ShellEjectionSocket"]["location_cm"][1] > 0
    return manifest


def protected_hashes():
    """Protect legacy weapon art/BPs and every pre-existing map."""
    content = PROJECT_ROOT / "TunaSweeper/Content"
    new_content = content / "Weapons/TunaWeaponCollection"
    paths = set()
    for relative in ("Weapons", "Meshes/Weapons", "Materials", "UI/Icons"):
        base = content / relative
        if not base.exists():
            continue
        for path in base.rglob("*.uasset"):
            if not path.is_relative_to(new_content):
                # Materials/UI can be large; restrict those roots to existing
                # weapon-named assets while protecting all legacy weapon dirs.
                if relative in {"Materials", "UI/Icons"} and not any(token in path.name.lower() for token in ("weapon", "rifle", "smg", "pistol", "oldgun")):
                    continue
                paths.add(path)
    paths.update(path for path in content.rglob("*.umap") if not path.is_relative_to(new_content))
    return {path.relative_to(PROJECT_ROOT).as_posix(): hashlib.sha256(path.read_bytes()).hexdigest() for path in sorted(paths)}


def assert_protected_unchanged(baseline):
    current = protected_hashes()
    assert current == baseline, "Existing weapon assets or project maps changed"
    return current


def material_specs(manifest):
    return {spec["name"]: spec for spec in manifest["materials"].values()}


def mesh_subsystem():
    return unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem) or unreal.get_default_object(unreal.StaticMeshEditorSubsystem)


def validate_showcase(manifest, expected):
    if not expected:
        return {"created": False}
    world = unreal.EditorLoadingAndSavingUtils.load_map(SHOWCASE_PATH)
    assert isinstance(world, unreal.World), "Showcase map did not reload"
    displayed = {}
    for actor in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.StaticMeshActor):
        mesh = actor.static_mesh_component.get_editor_property("static_mesh")
        if mesh and mesh.get_path_name().startswith(DEST + "/Meshes/"):
            assert mesh.get_name() not in displayed, mesh.get_name()
            scale = actor.get_actor_scale3d()
            assert max(abs(value - 1.0) for value in (scale.x, scale.y, scale.z)) < 1e-6
            displayed[mesh.get_name()] = [actor.get_actor_location().x, actor.get_actor_location().y, actor.get_actor_location().z]
    assert set(displayed) == {entry["name"] for entry in manifest["assets"]}
    labels = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.TextRenderActor)
    lights = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.DirectionalLight)
    cameras = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.CameraActor)
    assert len(labels) >= 6 and len(lights) >= 2 and len(cameras) >= 1
    return {"created": True, "path": SHOWCASE_PATH, "displayed_assets": displayed,
            "label_count": len(labels), "directional_light_count": len(lights), "camera_count": len(cameras)}


def validate(manifest=None, baseline=None, verification="fresh-process reload", showcase=None):
    manifest = manifest or read_manifest()
    previous = None
    if baseline is None:
        assert IMPORT_REPORT.exists(), "Import baseline is required"
        previous = json.loads(IMPORT_REPORT.read_text(encoding="utf-8"))
        assert previous.get("passed")
        baseline = previous["protected_assets_sha256"]
    if showcase is None:
        showcase = previous.get("showcase", {}).get("created", False) if previous else False
    before = assert_protected_unchanged(baseline)
    texture = unreal.load_asset(TEXTURE_PATH)
    assert isinstance(texture, unreal.Texture2D), TEXTURE_PATH
    assert texture.get_editor_property("srgb")
    assert texture.get_editor_property("lod_group") == unreal.TextureGroup.TEXTUREGROUP_WORLD
    assert texture.get_editor_property("power_of_two_mode") == unreal.TexturePowerOfTwoSetting.STRETCH_TO_POWER_OF_TWO
    assert not texture.get_editor_property("never_stream")
    master = unreal.load_asset(MASTER_PATH)
    assert isinstance(master, unreal.Material), MASTER_PATH
    lib = unreal.MaterialEditingLibrary
    assert isinstance(lib.get_material_property_input_node(master, unreal.MaterialProperty.MP_BASE_COLOR), unreal.MaterialExpressionMultiply)
    material_report = []
    for name, spec in material_specs(manifest).items():
        material = unreal.load_asset(f"{DEST}/Materials/{name}")
        assert isinstance(material, unreal.MaterialInstanceConstant), name
        assert material.get_editor_property("parent") == master
        parameter_names = {str(value) for value in lib.get_vector_parameter_names(material)}
        assert "Tint" in parameter_names, (name, parameter_names)
        tint = lib.get_material_instance_vector_parameter_value(material, "Tint")
        atlas = lib.get_material_instance_texture_parameter_value(material, "Atlas")
        metallic = lib.get_material_instance_scalar_parameter_value(material, "Metallic")
        roughness = lib.get_material_instance_scalar_parameter_value(material, "Roughness")
        emission = lib.get_material_instance_scalar_parameter_value(material, "EmissionStrength")
        assert atlas == texture, name
        assert abs(metallic - spec["metallic"]) < 1e-5 and abs(roughness - spec["roughness"]) < 1e-5
        assert abs(emission - spec["emission"]) < 1e-5
        material_report.append({"path": material.get_path_name(), "tint_parameter": [tint.r, tint.g, tint.b, tint.a],
                                "metallic": metallic, "roughness": roughness, "emission": emission})
    editor = mesh_subsystem()
    report = {"destination": DEST, "engine": unreal.SystemLibrary.get_engine_version(),
              "verification": verification, "texture": texture.get_path_name(), "texture_srgb": True,
              "editable_color_parameter": "Tint", "materials": material_report, "assets": []}
    previous_assets = {entry["name"]: entry for entry in previous.get("assets", [])} if previous else {}
    for entry in manifest["assets"]:
        mesh = unreal.load_asset(f"{DEST}/Meshes/{entry['name']}")
        assert isinstance(mesh, unreal.StaticMesh), entry["name"]
        vertices = editor.get_number_verts(mesh, 0)
        triangles = mesh.get_num_triangles(0)
        uv_count = editor.get_num_uv_channels(mesh, 0)
        collision_count = editor.get_simple_collision_count(mesh) + editor.get_convex_collision_count(mesh)
        expected_imported_triangles = previous_assets.get(entry["name"], {}).get("triangles_lod0")
        triangle_ok = (triangles == expected_imported_triangles if expected_imported_triangles is not None
                       else entry["triangles"] * 0.80 <= triangles <= entry["triangles"])
        assert vertices > 0 and triangle_ok and uv_count >= 2, (
            entry["name"], "vertices", vertices, "triangles", triangles,
            "source_triangles", entry["triangles"], "reload_expected", expected_imported_triangles,
            "uv_channels", uv_count)
        assert collision_count == entry["collision_boxes"] and collision_count >= 2, entry["name"]
        build = editor.get_lod_build_settings(mesh, 0)
        assert build.get_editor_property("generate_lightmap_u_vs") and build.get_editor_property("use_full_precision_u_vs")
        assert build.get_editor_property("use_high_precision_tangent_basis")
        assert mesh.get_editor_property("light_map_coordinate_index") == 1
        assigned = []
        for slot in mesh.get_editor_property("static_materials"):
            material = slot.get_editor_property("material_interface")
            assert isinstance(material, unreal.MaterialInstanceConstant)
            assert material.get_path_name().startswith(DEST + "/Materials/")
            assigned.append(material.get_name())
        assert set(assigned) == set(entry["materials"]), (entry["name"], assigned, entry["materials"])
        bounds = mesh.get_bounds()
        center = [bounds.origin.x, bounds.origin.y, bounds.origin.z]
        extent = [bounds.box_extent.x, bounds.box_extent.y, bounds.box_extent.z]
        actual = [center[i] - extent[i] for i in range(3)] + [center[i] + extent[i] for i in range(3)]
        error = max(abs(actual[i] - entry["bounds_cm"][i]) for i in range(6))
        assert error < 0.1, (entry["name"], "bounds", error, actual, entry["bounds_cm"])
        sockets = {}
        for socket_spec in entry["sockets"]:
            socket = mesh.find_socket(socket_spec["name"])
            assert isinstance(socket, unreal.StaticMeshSocket), (entry["name"], socket_spec["name"])
            location = socket.get_editor_property("relative_location")
            rotation = socket.get_editor_property("relative_rotation")
            scale = socket.get_editor_property("relative_scale")
            actual_location = [location.x, location.y, location.z]
            actual_rotation = [rotation.roll, rotation.pitch, rotation.yaw]
            actual_scale = [scale.x, scale.y, scale.z]
            assert max(abs(a - b) for a, b in zip(actual_location, socket_spec["location_cm"])) < 0.01
            assert max(abs(a - b) for a, b in zip(actual_rotation, socket_spec["rotation_deg"])) < 0.01
            assert max(abs(a - b) for a, b in zip(actual_scale, socket_spec["scale"])) < 0.001
            sockets[socket_spec["name"]] = {"location_cm": actual_location, "rotation_deg": actual_rotation, "scale": actual_scale}
        report["assets"].append({"name": entry["name"], "path": mesh.get_path_name(), "category": entry["category"],
                                 "style": entry["style"], "vertices_lod0": vertices, "triangles_lod0": triangles,
                                 "uv_channels": uv_count, "lightmap_uv_channel": 1, "simple_collisions": collision_count,
                                 "materials": assigned, "bounds_cm": actual, "max_bounds_error_cm": error, "sockets": sockets})
    report["showcase"] = validate_showcase(manifest, showcase)
    report["protected_assets_sha256"] = assert_protected_unchanged(before)
    report["existing_weapons_and_maps_unchanged"] = True
    report["total_triangles"] = sum(entry["triangles_lod0"] for entry in report["assets"])
    report["passed"] = True
    return report


def main():
    report = validate()
    path = SOURCE / "unreal_reload_validation.json"
    path.write_text(json.dumps(report, indent=2), encoding="utf-8")
    unreal.log("WEAPON_UE_RELOAD_VALIDATION_PASSED " + str(path))


if __name__ == "__main__":
    main()
