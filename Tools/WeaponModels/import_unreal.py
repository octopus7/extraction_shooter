"""One-off UE 5.7 importer. Commit with generated assets, then remove."""
import importlib.util
import json
import math
import os
from pathlib import Path
import unreal


VERIFY_PATH = Path(__file__).with_name("verify_unreal.py")
MODULE_SPEC = importlib.util.spec_from_file_location("weapon_verify", VERIFY_PATH)
verify = importlib.util.module_from_spec(MODULE_SPEC)
MODULE_SPEC.loader.exec_module(verify)
SOURCE, DEST = verify.SOURCE, verify.DEST
MANIFEST = verify.read_manifest()
ASSETS = unreal.AssetToolsHelpers.get_asset_tools()


def save(asset):
    assert asset.get_path_name().startswith(DEST + "/"), "Refusing to save outside additive destination"
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


def create_master(texture):
    master = unreal.load_asset(verify.MASTER_PATH)
    if master is not None:
        assert isinstance(master, unreal.Material)
        return master
    master = ASSETS.create_asset("M_TunaWeapon_Master", DEST + "/Materials", unreal.Material, unreal.MaterialFactoryNew())
    assert isinstance(master, unreal.Material)
    lib = unreal.MaterialEditingLibrary
    lib.delete_all_material_expressions(master)
    sample = lib.create_material_expression(master, unreal.MaterialExpressionTextureSampleParameter2D, -700, -120)
    sample.set_editor_property("parameter_name", "Atlas")
    sample.set_editor_property("texture", texture)
    tint = lib.create_material_expression(master, unreal.MaterialExpressionVectorParameter, -700, 40)
    tint.set_editor_property("parameter_name", "Tint")
    tint.set_editor_property("default_value", unreal.LinearColor(1.0, 1.0, 1.0, 1.0))
    tint.set_editor_property("group", "Color")
    color = lib.create_material_expression(master, unreal.MaterialExpressionMultiply, -410, -80)
    lib.connect_material_expressions(sample, "RGB", color, "A")
    lib.connect_material_expressions(tint, "RGB", color, "B")
    lib.connect_material_property(color, "", unreal.MaterialProperty.MP_BASE_COLOR)
    metallic = lib.create_material_expression(master, unreal.MaterialExpressionScalarParameter, -350, 80)
    metallic.set_editor_property("parameter_name", "Metallic")
    metallic.set_editor_property("default_value", 0.0)
    metallic.set_editor_property("group", "Surface")
    lib.connect_material_property(metallic, "", unreal.MaterialProperty.MP_METALLIC)
    roughness = lib.create_material_expression(master, unreal.MaterialExpressionScalarParameter, -350, 170)
    roughness.set_editor_property("parameter_name", "Roughness")
    roughness.set_editor_property("default_value", 0.5)
    roughness.set_editor_property("group", "Surface")
    lib.connect_material_property(roughness, "", unreal.MaterialProperty.MP_ROUGHNESS)
    strength = lib.create_material_expression(master, unreal.MaterialExpressionScalarParameter, -350, 270)
    strength.set_editor_property("parameter_name", "EmissionStrength")
    strength.set_editor_property("default_value", 0.0)
    strength.set_editor_property("group", "Color")
    emission = lib.create_material_expression(master, unreal.MaterialExpressionMultiply, -100, 140)
    lib.connect_material_expressions(color, "", emission, "A")
    lib.connect_material_expressions(strength, "", emission, "B")
    lib.connect_material_property(emission, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    lib.recompile_material(master)
    save(master)
    return master


def create_material_instances(master, texture):
    instances = {}
    lib = unreal.MaterialEditingLibrary
    assert "Tint" in {str(value) for value in lib.get_vector_parameter_names(master)}
    assert {"Metallic", "Roughness", "EmissionStrength"}.issubset(
        {str(value) for value in lib.get_scalar_parameter_names(master)})
    assert "Atlas" in {str(value) for value in lib.get_texture_parameter_names(master)}
    for name, spec in verify.material_specs(MANIFEST).items():
        instance = unreal.load_asset(f"{DEST}/Materials/{name}")
        if instance is None:
            instance = ASSETS.create_asset(name, DEST + "/Materials", unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
        assert isinstance(instance, unreal.MaterialInstanceConstant), name
        instance.set_editor_property("parent", master)
        tint_info = unreal.MaterialParameterInfo()
        tint_info.set_editor_property("name", "Tint")
        tint_value = unreal.VectorParameterValue()
        tint_value.set_editor_property("parameter_info", tint_info)
        tint_value.set_editor_property("parameter_value", unreal.LinearColor(1.0, 1.0, 1.0, 1.0))
        scalar_values = []
        for parameter_name, parameter_value in {
                "Metallic": spec["metallic"],
                "Roughness": spec["roughness"],
                "EmissionStrength": spec["emission"],
        }.items():
            info = unreal.MaterialParameterInfo()
            info.set_editor_property("name", parameter_name)
            value = unreal.ScalarParameterValue()
            value.set_editor_property("parameter_info", info)
            value.set_editor_property("parameter_value", float(parameter_value))
            scalar_values.append(value)
        instance.set_editor_property("vector_parameter_values", [tint_value])
        instance.set_editor_property("scalar_parameter_values", scalar_values)
        lib.update_material_instance(instance)
        save(instance)
        instances[name] = instance
    return instances


def add_sockets(mesh, entry):
    for spec in entry["sockets"]:
        old_socket = mesh.find_socket(spec["name"])
        if old_socket is not None:
            mesh.remove_socket(old_socket)
        socket = unreal.new_object(unreal.StaticMeshSocket.static_class(), mesh, spec["name"])
        socket.set_editor_property("socket_name", spec["name"])
        socket.set_editor_property("relative_location", unreal.Vector(*spec["location_cm"]))
        socket.set_editor_property("relative_rotation", unreal.Rotator(*spec["rotation_deg"]))
        socket.set_editor_property("relative_scale", unreal.Vector(*spec["scale"]))
        mesh.add_socket(socket)


def import_meshes(materials):
    editor = verify.mesh_subsystem()
    for entry in MANIFEST["assets"]:
        filename = SOURCE / "Models" / f"{entry['name']}.fbx"
        assert filename.is_file(), str(filename)
        mesh_path = f"{DEST}/Meshes/{entry['name']}"
        if unreal.EditorAssetLibrary.does_asset_exist(mesh_path):
            assert unreal.EditorAssetLibrary.delete_asset(mesh_path), mesh_path
        options = unreal.FbxImportUI()
        for key, value in {"import_mesh": True, "import_as_skeletal": False, "import_animations": False,
                           "import_materials": False, "import_textures": False,
                           "automated_import_should_detect_type": False,
                           "mesh_type_to_import": unreal.FBXImportType.FBXIT_STATIC_MESH}.items():
            options.set_editor_property(key, value)
        data = options.static_mesh_import_data
        for key, value in {"combine_meshes": True, "auto_generate_collision": False,
                           "one_convex_hull_per_ucx": True, "generate_lightmap_u_vs": True,
                           "convert_scene": True, "convert_scene_unit": True, "force_front_x_axis": False,
                           "transform_vertex_to_absolute": True, "build_nanite": False,
                           "remove_degenerates": True, "import_uniform_scale": 1.0}.items():
            data.set_editor_property(key, value)
        data.set_editor_property("import_rotation", unreal.Rotator(0.0, 0.0, -90.0))
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
        mesh = unreal.load_asset(mesh_path)
        assert isinstance(mesh, unreal.StaticMesh), entry["name"]
        for index, slot in enumerate(mesh.get_editor_property("static_materials")):
            name = str(slot.get_editor_property("material_slot_name"))
            if name not in materials:
                name = str(slot.get_editor_property("imported_material_slot_name"))
            assert name in materials, (entry["name"], "unknown imported material", name)
            mesh.set_material(index, materials[name])
        build = editor.get_lod_build_settings(mesh, 0)
        for key, value in {"recompute_tangents": True, "use_mikk_t_space": False,
                           "use_full_precision_u_vs": True, "use_high_precision_tangent_basis": True,
                           "generate_lightmap_u_vs": True, "src_lightmap_index": 0, "dst_lightmap_index": 1}.items():
            build.set_editor_property(key, value)
        editor.set_lod_build_settings(mesh, 0, build)
        mesh.set_editor_property("light_map_coordinate_index", 1)
        add_sockets(mesh, entry)
        save(mesh)


def spawn(world, actor_class, location, rotation=(0.0, 0.0, 0.0), scale=(1.0, 1.0, 1.0)):
    transform = unreal.Transform(location=unreal.Vector(*location), rotation=unreal.Rotator(*rotation), scale=unreal.Vector(*scale))
    gameplay = unreal.get_default_object(unreal.GameplayStatics)
    actor = gameplay.call_method("BeginDeferredActorSpawnFromClass", args=(world, actor_class, transform, unreal.SpawnActorCollisionHandlingMethod.ALWAYS_SPAWN))
    assert actor, actor_class
    actor = gameplay.call_method("FinishSpawningActor", args=(actor, transform))
    assert actor
    return actor


def create_showcase():
    world = unreal.EditorLoadingAndSavingUtils.new_blank_map(False)
    assert world
    category_order = {"SMG": 0, "AR": 1, "Pistol": 2}
    style_order = {"Standard": 0, "Premium": 1}
    entries = sorted(MANIFEST["assets"], key=lambda entry: (category_order[entry["category"]], style_order[entry["style"]]))
    for entry in entries:
        row = category_order[entry["category"]]
        column = style_order[entry["style"]]
        location = [(column - 0.5) * 105.0, (row - 1.0) * 125.0, -entry["bounds_cm"][2] + 10.0]
        actor = spawn(world, unreal.StaticMeshActor, location)
        actor.set_actor_label(entry["name"])
        actor.set_folder_path("TunaWeaponCollection/" + entry["category"])
        actor.static_mesh_component.set_static_mesh(unreal.load_asset(f"{DEST}/Meshes/{entry['name']}"))
        label = spawn(world, unreal.TextRenderActor, [location[0], location[1] - 38.0, 4.0], [90.0, 90.0, 0.0])
        label.set_actor_label("Label_" + entry["name"])
        label.set_folder_path("TunaWeaponCollection/Labels")
        text = label.get_component_by_class(unreal.TextRenderComponent)
        text.set_text(f"{entry['category']} {entry['style']}")
        text.set_world_size(13.0)
        text.set_horizontal_alignment(unreal.HorizTextAligment.EHTA_CENTER)
        text.set_text_render_color(unreal.Color(215, 225, 235, 255))
    floor = spawn(world, unreal.StaticMeshActor, [0.0, 0.0, -5.0], scale=[2.2, 4.4, 0.1])
    floor.set_actor_label("Showcase_Floor")
    floor.static_mesh_component.set_static_mesh(unreal.load_asset("/Engine/BasicShapes/Cube"))
    for name, rotation, intensity, color in (
        ("Showcase_Key", [-55.0, -35.0, 0.0], 3.0, unreal.LinearColor(1.0, 0.93, 0.84, 1.0)),
        ("Showcase_Fill", [-35.0, 145.0, 0.0], 1.4, unreal.LinearColor(0.72, 0.84, 1.0, 1.0)),
    ):
        light = spawn(world, unreal.DirectionalLight, [0.0, 0.0, 500.0], rotation)
        light.set_actor_label(name)
        component = light.get_component_by_class(unreal.DirectionalLightComponent)
        component.set_mobility(unreal.ComponentMobility.MOVABLE)
        component.set_intensity(intensity)
        component.set_light_color(color)
    camera = spawn(world, unreal.CameraActor, [20.0, -520.0, 420.0], [-38.0, 92.0, 0.0])
    camera.set_actor_label("Showcase_OverviewCamera")
    assert verify.SHOWCASE_PATH.startswith(DEST + "/Maps/")
    assert unreal.EditorLoadingAndSavingUtils.save_map(world, verify.SHOWCASE_PATH)


def main():
    baseline = verify.protected_hashes()
    if os.environ.get("WEAPON_REUSE_IMPORTED") != "1":
        texture = import_texture()
        master = create_master(texture)
        materials = create_material_instances(master, texture)
        import_meshes(materials)
    showcase = os.environ.get("WEAPON_SKIP_SHOWCASE") != "1"
    if showcase:
        create_showcase()
    report = verify.validate(MANIFEST, baseline, verification="import process", showcase=showcase)
    verify.IMPORT_REPORT.write_text(json.dumps(report, indent=2), encoding="utf-8")
    unreal.log("WEAPON_UE_IMPORT_VALIDATION_PASSED " + str(verify.IMPORT_REPORT))


if __name__ == "__main__":
    main()
