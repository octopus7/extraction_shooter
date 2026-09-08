"""Read-only UE 5.7 validation for the saved additional interior asset pack.

Run in an independent Python commandlet. This file does not import, create,
modify, or save Unreal assets and remains after the one-off importer is removed.
"""
from pathlib import Path
import hashlib
import json
import math
import unreal


ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / "TunaSweeper/SourceArt/Environment/InteriorAdditions"
DEST = "/Game/Environment/Bunker/InteriorAdditions"
TEXTURE_NAME = "T_InteriorAdditions_Atlas"
TEXTURE_PATH = f"{DEST}/Textures/{TEXTURE_NAME}"
SHOWCASE_PATH = f"{DEST}/Maps/L_InteriorAdditions_Showcase"
IMPORT_REPORT = SOURCE / "unreal_import_validation.json"


def read_manifest():
    manifest = json.loads((SOURCE / "model_manifest.json").read_text(encoding="utf-8"))
    assert manifest["assets"] and manifest["materials"], "Empty interior manifest"
    names = [entry["name"] for entry in manifest["assets"]]
    assert len(names) == len(set(names)), "Duplicate mesh names"
    material_names = [spec["name"] for spec in manifest["materials"].values()]
    assert len(material_names) == len(set(material_names)), "Duplicate material names"
    for name in names + material_names:
        assert name.replace("_", "").isalnum(), ("Unsafe asset name", name)
    for entry in manifest["assets"]:
        bounds = entry["bounds_cm"]
        assert len(bounds) == 6 and all(math.isfinite(v) for v in bounds), entry["name"]
        assert all(bounds[i + 3] > bounds[i] for i in range(3)), entry["name"]
        assert set(entry["materials"]).issubset(material_names), entry["name"]
        if "size_cm" in entry:
            assert max(abs(bounds[i + 3] - bounds[i] - entry["size_cm"][i]) for i in range(3)) < 0.1, entry["name"]
    return manifest


def protected_hashes():
    """Preserve the existing Agit kit and every existing project map."""
    content = ROOT / "TunaSweeper/Content"
    new_content = content / "Environment/Bunker/InteriorAdditions"
    paths = set((content / "Environment/Bunker/Agit").rglob("*.uasset"))
    paths.update(path for path in content.rglob("*.umap") if not path.is_relative_to(new_content))
    return {
        path.relative_to(ROOT).as_posix(): hashlib.sha256(path.read_bytes()).hexdigest()
        for path in sorted(paths)
    }


def assert_protected_unchanged(baseline):
    after = protected_hashes()
    assert baseline == after, "Existing Agit assets or project maps changed"
    return after


def material_specs(manifest):
    return {spec["name"]: spec for spec in manifest["materials"].values()}


def mesh_subsystem():
    # Commandlets do not instantiate editor subsystems. These mesh helpers
    # operate only on their supplied asset, so the class default is sufficient.
    return unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem) or unreal.get_default_object(unreal.StaticMeshEditorSubsystem)


def validate_showcase(manifest, expected_showcase):
    if not expected_showcase:
        return {"created": False}
    world = unreal.EditorLoadingAndSavingUtils.load_map(SHOWCASE_PATH)
    assert isinstance(world, unreal.World), "Showcase map did not reload"
    actors = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.StaticMeshActor)
    displayed = {}
    for actor in actors:
        mesh = actor.static_mesh_component.get_editor_property("static_mesh")
        if mesh and mesh.get_path_name().startswith(DEST + "/Meshes/"):
            name = mesh.get_name()
            assert name not in displayed, ("Duplicate showcase mesh", name)
            scale = actor.get_actor_scale3d()
            assert max(abs(v - 1.0) for v in (scale.x, scale.y, scale.z)) < 1e-6, name
            location = actor.get_actor_location()
            displayed[name] = [location.x, location.y, location.z]
    assert set(displayed) == {entry["name"] for entry in manifest["assets"]}, "Showcase asset coverage differs"
    labels = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.TextRenderActor)
    assert len(labels) >= len(manifest["assets"]), "Missing showcase labels"
    lights = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.DirectionalLight)
    assert len(lights) == 1, "Showcase must contain only one directional key light"
    key = lights[0]
    assert key.get_actor_label() == "Showcase_Key", "Unexpected showcase directional light"
    source_angle = key.get_component_by_class(unreal.DirectionalLightComponent).get_editor_property("light_source_angle")
    assert abs(source_angle - 45.0) < 1e-4, ("Key source angle must be 45 degrees", source_angle)
    return {"created": True, "path": SHOWCASE_PATH, "displayed_assets": displayed,
            "label_count": len(labels), "directional_light_count": len(lights),
            "key_light_source_angle_degrees": source_angle}


def validate(manifest=None, baseline=None, verification="fresh-process reload", showcase=None):
    manifest = manifest or read_manifest()
    previous = None
    if baseline is None:
        assert IMPORT_REPORT.exists(), "Import baseline is required for reload validation"
        previous = json.loads(IMPORT_REPORT.read_text(encoding="utf-8"))
        assert previous.get("passed"), "Import validation did not pass"
        baseline = previous["protected_assets_sha256"]
    if showcase is None:
        showcase = previous.get("showcase", {}).get("created", False) if previous else False
    before = assert_protected_unchanged(baseline)
    texture = unreal.load_asset(TEXTURE_PATH)
    assert isinstance(texture, unreal.Texture2D), TEXTURE_PATH
    assert texture.get_editor_property("srgb")
    assert texture.get_editor_property("lod_group") == unreal.TextureGroup.TEXTUREGROUP_WORLD
    assert not texture.get_editor_property("never_stream")
    material_report = []
    for name, spec in material_specs(manifest).items():
        mat = unreal.load_asset(f"{DEST}/Materials/{name}")
        assert isinstance(mat, unreal.Material), name
        lib = unreal.MaterialEditingLibrary
        sample = lib.get_material_property_input_node(mat, unreal.MaterialProperty.MP_BASE_COLOR)
        assert isinstance(sample, unreal.MaterialExpressionTextureSample), name + " lacks base-color sample"
        assert sample.get_editor_property("texture") == texture, name + " references another texture"
        values = {}
        for field, prop in (("metallic", unreal.MaterialProperty.MP_METALLIC), ("roughness", unreal.MaterialProperty.MP_ROUGHNESS)):
            node = lib.get_material_property_input_node(mat, prop)
            assert isinstance(node, unreal.MaterialExpressionConstant), (name, field)
            values[field] = node.get_editor_property("r")
            assert abs(values[field] - spec[field]) < 1e-5, (name, field, values[field])
        material_report.append({"path": mat.get_path_name(), **values})
    editor = mesh_subsystem()
    report = {"destination": DEST, "engine": unreal.SystemLibrary.get_engine_version(),
              "verification": verification, "texture": texture.get_path_name(),
              "texture_srgb": True, "materials": material_report, "assets": []}
    for entry in manifest["assets"]:
        mesh = unreal.load_asset(f"{DEST}/Meshes/{entry['name']}")
        assert isinstance(mesh, unreal.StaticMesh), entry["name"]
        vertices = editor.get_number_verts(mesh, 0)
        triangles = mesh.get_num_triangles(0)
        uv_count = editor.get_num_uv_channels(mesh, 0)
        collision_count = editor.get_simple_collision_count(mesh) + editor.get_convex_collision_count(mesh)
        assert vertices > 0 and triangles > 0 and uv_count >= 1, entry["name"]
        assert triangles == entry["triangles"], (entry["name"], "triangles", triangles, entry["triangles"])
        expected_collision_count = entry.get("collision_boxes")
        if expected_collision_count is None:
            assert collision_count > 0, (entry["name"], "No simple collision")
        else:
            assert collision_count == expected_collision_count, (entry["name"], collision_count, expected_collision_count)
        build = editor.get_lod_build_settings(mesh, 0)
        assert build.get_editor_property("generate_lightmap_u_vs")
        assert build.get_editor_property("use_full_precision_u_vs")
        assert mesh.get_editor_property("light_map_coordinate_index") == 1
        slots = list(mesh.get_editor_property("static_materials"))
        assigned = []
        for slot in slots:
            mat = slot.get_editor_property("material_interface")
            assert mat and mat.get_path_name().startswith(DEST + "/Materials/"), entry["name"]
            assigned.append(mat.get_name())
        assert set(assigned) == set(entry["materials"]), (entry["name"], assigned, entry["materials"])
        bounds = mesh.get_bounds()
        center = [bounds.origin.x, bounds.origin.y, bounds.origin.z]
        extent = [bounds.box_extent.x, bounds.box_extent.y, bounds.box_extent.z]
        actual = [center[i] - extent[i] for i in range(3)] + [center[i] + extent[i] for i in range(3)]
        error = max(abs(a - b) for a, b in zip(actual, entry["bounds_cm"]))
        assert error < 0.1, (entry["name"], "Bounds mismatch in cm", error, actual, entry["bounds_cm"])
        report["assets"].append({"name": entry["name"], "path": mesh.get_path_name(),
                                 "category": entry.get("category", "interior"), "vertices_lod0": vertices,
                                 "triangles_lod0": triangles, "source_uv_channels": uv_count,
                                 "lightmap_uv_channel": 1, "simple_collisions": collision_count,
                                 "materials": assigned, "bounds_cm": actual, "max_bounds_error_cm": error})
    report["showcase"] = validate_showcase(manifest, showcase)
    report["protected_assets_sha256"] = assert_protected_unchanged(before)
    report["existing_agit_and_maps_unchanged"] = True
    report["total_triangles"] = sum(entry["triangles_lod0"] for entry in report["assets"])
    report["passed"] = True
    return report


def main():
    report = validate()
    path = SOURCE / "unreal_reload_validation.json"
    path.write_text(json.dumps(report, indent=2), encoding="utf-8")
    unreal.log("INTERIOR_UE_RELOAD_VALIDATION_PASSED " + str(path))


if __name__ == "__main__":
    main()
