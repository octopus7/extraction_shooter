"""One-off UE importer: commit with assets, then remove after verification.

Only the new InteriorAdditions folder is writable through this script. Existing
Agit assets and maps are hashed before/after. Use verify_unreal.py for later audits.
"""
import importlib.util
import json
import math
import os
from pathlib import Path
import unreal


VERIFY_PATH = Path(__file__).with_name("verify_unreal.py")
MODULE_SPEC = importlib.util.spec_from_file_location("interior_verify", VERIFY_PATH)
verify = importlib.util.module_from_spec(MODULE_SPEC)
MODULE_SPEC.loader.exec_module(verify)
SOURCE, DEST = verify.SOURCE, verify.DEST
MANIFEST = verify.read_manifest()
ASSETS = unreal.AssetToolsHelpers.get_asset_tools()


def save(asset):
    assert asset.get_path_name().startswith(DEST + "/"), "Refusing to save an existing asset"
    assert unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False), asset.get_path_name()


def import_texture():
    filename = SOURCE / "Textures" / (verify.TEXTURE_NAME + ".png")
    assert filename.is_file(), str(filename)
    task = unreal.AssetImportTask()
    task.filename = str(filename)
    task.destination_path = DEST + "/Textures"
    task.destination_name = verify.TEXTURE_NAME
    task.automated = True
    task.replace_existing = True
    task.save = True
    ASSETS.import_asset_tasks([task])
    texture = unreal.load_asset(verify.TEXTURE_PATH)
    assert isinstance(texture, unreal.Texture2D), verify.TEXTURE_PATH
    texture.set_editor_property("srgb", True)
    texture.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_WORLD)
    texture.set_editor_property("power_of_two_mode", unreal.TexturePowerOfTwoSetting.STRETCH_TO_POWER_OF_TWO)
    texture.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_FROM_TEXTURE_GROUP)
    texture.set_editor_property("never_stream", False)
    save(texture)
    return texture


def create_materials(texture):
    materials = {}
    lib = unreal.MaterialEditingLibrary
    for name, spec in verify.material_specs(MANIFEST).items():
        mat = unreal.load_asset(f"{DEST}/Materials/{name}")
        if mat is None:
            mat = ASSETS.create_asset(name, DEST + "/Materials", unreal.Material, unreal.MaterialFactoryNew())
        assert isinstance(mat, unreal.Material), name
        lib.delete_all_material_expressions(mat)
        sample = lib.create_material_expression(mat, unreal.MaterialExpressionTextureSample, -400, -160)
        sample.set_editor_property("texture", texture)
        lib.connect_material_property(sample, "RGB", unreal.MaterialProperty.MP_BASE_COLOR)
        for field, prop, y in (("metallic", unreal.MaterialProperty.MP_METALLIC, 40),
                               ("roughness", unreal.MaterialProperty.MP_ROUGHNESS, 160)):
            value = float(spec[field])
            assert 0.0 <= value <= 1.0, (name, field, value)
            node = lib.create_material_expression(mat, unreal.MaterialExpressionConstant, -350, y)
            node.set_editor_property("r", value)
            lib.connect_material_property(node, "", prop)
        lib.recompile_material(mat)
        save(mat)
        materials[name] = mat
    return materials


def import_meshes(materials):
    editor = verify.mesh_subsystem()
    for entry in MANIFEST["assets"]:
        filename = SOURCE / "Models" / (entry["name"] + ".fbx")
        assert filename.is_file(), str(filename)
        options = unreal.FbxImportUI()
        for key, value in {"import_mesh": True, "import_as_skeletal": False, "import_animations": False,
                           "import_materials": False, "import_textures": False,
                           "automated_import_should_detect_type": False,
                           "mesh_type_to_import": unreal.FBXImportType.FBXIT_STATIC_MESH}.items():
            options.set_editor_property(key, value)
        data = options.static_mesh_import_data
        for key, value in {"combine_meshes": True, "auto_generate_collision": "collision_boxes" not in entry,
                           "one_convex_hull_per_ucx": True, "generate_lightmap_u_vs": True,
                           "convert_scene": True, "convert_scene_unit": True, "force_front_x_axis": False,
                           "transform_vertex_to_absolute": True, "build_nanite": False,
                           "remove_degenerates": True, "import_uniform_scale": 1.0}.items():
            data.set_editor_property(key, value)
        data.set_editor_property("normal_import_method", unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS)
        data.set_editor_property("normal_generation_method", unreal.FBXNormalGenerationMethod.BUILT_IN)
        task = unreal.AssetImportTask()
        task.filename = str(filename)
        task.destination_path = DEST + "/Meshes"
        task.destination_name = entry["name"]
        task.automated = True
        task.replace_existing = True
        task.replace_existing_settings = True
        task.save = True
        task.options = options
        task.factory = unreal.FbxFactory()
        ASSETS.import_asset_tasks([task])
        mesh = unreal.load_asset(f"{DEST}/Meshes/{entry['name']}")
        assert isinstance(mesh, unreal.StaticMesh), entry["name"]
        for index, slot in enumerate(mesh.get_editor_property("static_materials")):
            name = str(slot.get_editor_property("material_slot_name"))
            if name not in materials:
                name = str(slot.get_editor_property("imported_material_slot_name"))
            assert name in materials, ("Unknown imported material", entry["name"], name)
            mesh.set_material(index, materials[name])
        build = editor.get_lod_build_settings(mesh, 0)
        for key, value in {"recompute_tangents": True, "use_mikk_t_space": False,
                           "use_full_precision_u_vs": True, "use_high_precision_tangent_basis": True,
                           "generate_lightmap_u_vs": True, "src_lightmap_index": 0,
                           "dst_lightmap_index": 1}.items():
            build.set_editor_property(key, value)
        editor.set_lod_build_settings(mesh, 0, build)
        mesh.set_editor_property("light_map_coordinate_index", 1)
        save(mesh)


def spawn(world, actor_class, location, rotation=(0.0, 0.0, 0.0), scale=(1.0, 1.0, 1.0)):
    # Runtime spawn API avoids absent EditorActorSubsystems in commandlet mode.
    # UE exposes NativeMakeFunc through the struct constructor, not MathLibrary.
    transform = unreal.Transform(location=unreal.Vector(*location), rotation=unreal.Rotator(*rotation), scale=unreal.Vector(*scale))
    # BlueprintInternalUseOnly functions lack Python glue; call_method is the
    # supported reflection entry point for those existing engine UFunctions.
    gameplay = unreal.get_default_object(unreal.GameplayStatics)
    actor = gameplay.call_method("BeginDeferredActorSpawnFromClass", args=(
        world, actor_class, transform, unreal.SpawnActorCollisionHandlingMethod.ALWAYS_SPAWN))
    assert actor, actor_class
    actor = gameplay.call_method("FinishSpawningActor", args=(actor, transform))
    assert actor
    return actor


def create_showcase():
    world = unreal.EditorLoadingAndSavingUtils.new_blank_map(False)
    assert world, "Could not create separate showcase map"
    entries = sorted(MANIFEST["assets"], key=lambda entry: (entry.get("category", ""), entry["name"]))
    columns = min(5, math.ceil(math.sqrt(len(entries))))
    max_x = max(entry["bounds_cm"][3] - entry["bounds_cm"][0] for entry in entries)
    max_y = max(entry["bounds_cm"][4] - entry["bounds_cm"][1] for entry in entries)
    cell_x, cell_y = max_x + 140.0, max_y + 150.0
    rows = math.ceil(len(entries) / columns)
    for index, entry in enumerate(entries):
        x = (index % columns - (columns - 1) / 2.0) * cell_x
        y = (index // columns - (rows - 1) / 2.0) * cell_y
        z = -entry["bounds_cm"][2]
        location = entry.get("showcase_location_cm", [x, y, z])
        rotation = entry.get("showcase_rotation_deg", [0.0, 0.0, 0.0])
        actor = spawn(world, unreal.StaticMeshActor, location, rotation)
        actor.set_actor_label(entry["name"])
        actor.set_folder_path("InteriorAdditions/" + entry.get("category", "interior"))
        actor.static_mesh_component.set_static_mesh(unreal.load_asset(f"{DEST}/Meshes/{entry['name']}"))
        label = spawn(world, unreal.TextRenderActor, [location[0], location[1] - cell_y * 0.40, 2.0], [90.0, 90.0, 0.0])
        label.set_actor_label("Label_" + entry["name"])
        label.set_folder_path("InteriorAdditions/Labels")
        text = label.get_component_by_class(unreal.TextRenderComponent)
        text.set_text(entry.get("display_name", entry["name"].replace("SM_InteriorAdditions_", "")))
        text.set_world_size(14.0)
        text.set_horizontal_alignment(unreal.HorizTextAligment.EHTA_CENTER)
        text.set_text_render_color(unreal.Color(45, 36, 34, 255))
    floor = spawn(world, unreal.StaticMeshActor, [0.0, 0.0, -5.0], scale=[columns * cell_x / 100.0, rows * cell_y / 100.0, 0.1])
    floor.set_actor_label("Showcase_Floor")
    floor.static_mesh_component.set_static_mesh(unreal.load_asset("/Engine/BasicShapes/Cube"))
    for name, rotation, intensity, color in (
        ("Showcase_Key", [-55.0, -35.0, 0.0], 3.0, unreal.LinearColor(1.0, 0.90, 0.78, 1.0)),
        ("Showcase_Fill", [-35.0, 145.0, 0.0], 1.3, unreal.LinearColor(0.77, 0.85, 1.0, 1.0)),
    ):
        light = spawn(world, unreal.DirectionalLight, [0.0, 0.0, 600.0], rotation)
        light.set_actor_label(name)
        component = light.get_component_by_class(unreal.DirectionalLightComponent)
        component.set_mobility(unreal.ComponentMobility.MOVABLE)
        component.set_intensity(intensity)
        component.set_light_color(color)
    distance = max(columns * cell_x, rows * cell_y)
    camera = spawn(world, unreal.CameraActor, [distance * 0.45, -distance * 0.72, distance * 0.9], [-48.0, 122.0, 0.0])
    camera.set_actor_label("Showcase_OverviewCamera")
    assert verify.SHOWCASE_PATH.startswith(DEST + "/Maps/")
    assert unreal.EditorLoadingAndSavingUtils.save_map(world, verify.SHOWCASE_PATH), "Showcase save failed"


def main():
    baseline = verify.protected_hashes()
    if os.environ.get("INTERIOR_REUSE_IMPORTED") != "1":
        texture = import_texture()
        materials = create_materials(texture)
        import_meshes(materials)
    showcase = os.environ.get("INTERIOR_SKIP_SHOWCASE") != "1"
    if showcase:
        create_showcase()
    report = verify.validate(MANIFEST, baseline, verification="import process", showcase=showcase)
    verify.IMPORT_REPORT.write_text(json.dumps(report, indent=2), encoding="utf-8")
    unreal.log("INTERIOR_UE_IMPORT_VALIDATION_PASSED " + str(verify.IMPORT_REPORT))


if __name__ == "__main__":
    main()
