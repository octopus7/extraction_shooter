"""One-off UE 5.7 importer. Commit with generated assets, then remove."""
import importlib.util
import json
import math
import os
from pathlib import Path
import unreal


VERIFY_PATH = Path(__file__).with_name("verify_unreal.py")
MODULE_SPEC = importlib.util.spec_from_file_location("facility_verify", VERIFY_PATH)
verify = importlib.util.module_from_spec(MODULE_SPEC)
MODULE_SPEC.loader.exec_module(verify)
SOURCE, DEST = verify.SOURCE, verify.DEST
MANIFEST = verify.read_manifest()
ASSETS = unreal.AssetToolsHelpers.get_asset_tools()


def save(asset):
    assert asset.get_path_name().startswith(DEST + "/"), "Refusing to save outside additive destination"
    assert unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False), asset.get_path_name()


def import_texture(name=verify.TEXTURE_NAME):
    filename = SOURCE / "Textures" / (name + ".png")
    assert filename.is_file(), str(filename)
    task = unreal.AssetImportTask()
    task.factory = unreal.TextureFactory()
    task.filename = str(filename)
    task.destination_path = DEST + "/Textures"
    task.destination_name = name
    task.automated = True
    task.replace_existing = True
    task.save = True
    ASSETS.import_asset_tasks([task])
    texture = unreal.load_asset(f"{DEST}/Textures/{name}")
    assert isinstance(texture, unreal.Texture2D), name
    texture.set_editor_property("srgb", True)
    texture.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_WORLD)
    texture.set_editor_property("power_of_two_mode", unreal.TexturePowerOfTwoSetting.STRETCH_TO_POWER_OF_TWO)
    texture.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_FROM_TEXTURE_GROUP)
    texture.set_editor_property("never_stream", False)
    save(texture)
    return texture


def create_forest_material():
    texture=import_texture(verify.FOREST_TEXTURE_NAME)
    mat=unreal.load_asset(verify.FOREST_MATERIAL_PATH)
    if mat is None:
        mat=ASSETS.create_asset('M_MoleControlV2_ForestImpostor',DEST+'/Materials',unreal.Material,unreal.MaterialFactoryNew())
    lib=unreal.MaterialEditingLibrary
    lib.delete_all_material_expressions(mat)
    mat.set_editor_property('two_sided',True)
    mat.set_editor_property('shading_model',unreal.MaterialShadingModel.MSM_UNLIT)
    sample=lib.create_material_expression(mat,unreal.MaterialExpressionTextureSampleParameter2D,-500,0)
    sample.set_editor_property('parameter_name','ForestTexture');sample.set_editor_property('texture',texture)
    tint=lib.create_material_expression(mat,unreal.MaterialExpressionVectorParameter,-500,160)
    tint.set_editor_property('parameter_name','ForestTint')
    tint.set_editor_property('default_value',unreal.LinearColor(.7,.7,.7,1))
    color=lib.create_material_expression(mat,unreal.MaterialExpressionMultiply,-230,0)
    lib.connect_material_expressions(sample,'RGB',color,'A');lib.connect_material_expressions(tint,'RGB',color,'B')
    lib.connect_material_property(color,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    lib.recompile_material(mat);save(mat)


def create_master(texture):
    master = unreal.load_asset(verify.MASTER_PATH)
    if master is not None:
        assert isinstance(master, unreal.Material)
        return master
    master = ASSETS.create_asset("M_MoleControlV2_Master", DEST + "/Materials", unreal.Material, unreal.MaterialFactoryNew())
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
        if 'texture' in spec:
            custom=unreal.load_asset(f"{DEST}/Textures/{spec['texture']}")
            assert isinstance(custom,unreal.Texture2D)
            info=unreal.MaterialParameterInfo();info.set_editor_property('name','Atlas')
            value=unreal.TextureParameterValue();value.set_editor_property('parameter_info',info);value.set_editor_property('parameter_value',custom)
            instance.set_editor_property('texture_parameter_values',[value])
        lib.update_material_instance(instance)
        save(instance)
        instances[name] = instance
    return instances


def import_meshes(materials):
    editor = verify.mesh_subsystem()
    for entry in MANIFEST["assets"]:
        filename = SOURCE / "Models" / f"{entry['name']}.fbx"
        assert filename.is_file(), str(filename)
        mesh_path = f"{DEST}/Meshes/{entry['name']}"
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
        save(mesh)


def spawn(world, cls, xyz, yaw=0):
    transform=unreal.Transform(location=unreal.Vector(*xyz),rotation=unreal.Rotator(pitch=0,yaw=yaw,roll=0),scale=unreal.Vector(1,1,1))
    gameplay=unreal.get_default_object(unreal.GameplayStatics)
    actor=gameplay.call_method("BeginDeferredActorSpawnFromClass",args=(world,cls,transform,unreal.SpawnActorCollisionHandlingMethod.ALWAYS_SPAWN))
    assert actor
    return gameplay.call_method("FinishSpawningActor",args=(actor,transform))

def create_levels():
    for level in verify.read_layout()["levels"]:
        world=unreal.EditorLoadingAndSavingUtils.new_blank_map(False)
        assert world
        for entry in level["placements"]:
            actor=spawn(world,unreal.StaticMeshActor,entry["location_cm"],entry["yaw_deg"])
            actor.set_actor_label(entry["label"]);actor.set_folder_path(entry["zone"])
            comp=actor.static_mesh_component
            comp.set_static_mesh(unreal.load_asset(entry.get('asset_path',f"{DEST}/Meshes/{entry['mesh']}")))
            comp.set_collision_profile_name("BlockAll")
            comp.set_collision_enabled(unreal.CollisionEnabled.QUERY_AND_PHYSICS)
        player=spawn(world,unreal.PlayerStart,level["player_start"])
        player.set_actor_label("MoleV2_PlayerStart")
        attic="Control" in level["name"]
        for label,pitch,yaw,intensity,color in (
            ("Key",-65,-40,3.0,(1,.80,.60) if attic else (1,.92,.82)),
            ("Fill",-45,140,1.7,(.85,.88,1)),
        ):
            light=spawn(world,unreal.DirectionalLight,[0,0,1500])
            light.set_actor_label("Lighting_"+label)
            light.set_actor_rotation(unreal.Rotator(pitch=pitch,yaw=yaw,roll=0),False)
            comp=light.get_component_by_class(unreal.DirectionalLightComponent)
            comp.set_mobility(unreal.ComponentMobility.MOVABLE)
            comp.set_intensity(intensity);comp.set_light_color(unreal.LinearColor(*color,1))
            comp.set_editor_property('volumetric_scattering_intensity',0.0)
        for entry in level["placements"]:
            if entry["mesh"].endswith("RoseTableLamp"):
                xyz=entry["location_cm"]
                light=spawn(world,unreal.PointLight,[xyz[0],xyz[1],xyz[2]+33])
                light.set_actor_label("Practical_"+entry["label"])
                comp=light.get_component_by_class(unreal.PointLightComponent)
                comp.set_mobility(unreal.ComponentMobility.MOVABLE)
                comp.set_intensity(1000);comp.set_attenuation_radius(450)
                # Diffuse cloth practical: the opaque modeled bulb/shade must not
                # trap all illumination inside the lamp. Window shadows remain on.
                comp.set_editor_property('cast_shadows',False)
                comp.set_light_color(unreal.LinearColor(1,.72,.47,1))
                comp.set_editor_property('volumetric_scattering_intensity',.15)
        if attic:
            for label,xyz,power,color in [('CeilingBounce',[0,0,585],4000,(1,.84,.69)),('MapTask',[0,115,495],300,(1,.79,.52))]:
                light=spawn(world,unreal.PointLight,xyz);light.set_actor_label('Lighting_'+label)
                comp=light.get_component_by_class(unreal.PointLightComponent);comp.set_mobility(unreal.ComponentMobility.MOVABLE)
                comp.set_intensity(power);comp.set_attenuation_radius(600);comp.set_light_color(unreal.LinearColor(*color,1))
                comp.set_editor_property('source_radius',35.0)
                if label == 'CeilingBounce':
                    comp.set_editor_property('cast_shadows',False)
                comp.set_editor_property('volumetric_scattering_intensity',.1)
            spec=level['forest']
            forest=spawn(world,unreal.StaticMeshActor,spec['location_cm'])
            forest.set_actor_label(spec['label']);forest.set_folder_path('Exterior')
            pitch,yaw,roll=spec['rotation_deg']
            forest.set_actor_rotation(unreal.Rotator(pitch=pitch,yaw=yaw,roll=roll),False)
            forest.set_actor_scale3d(unreal.Vector(*spec['scale']))
            comp=forest.static_mesh_component
            comp.set_static_mesh(unreal.load_asset('/Engine/BasicShapes/Plane'))
            comp.set_material(0,unreal.load_asset(verify.FOREST_MATERIAL_PATH))
            comp.set_collision_profile_name('NoCollision');comp.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
            comp.set_editor_property('cast_shadow',False)
            spec=level['window_light']
            fog=spawn(world,unreal.ExponentialHeightFog,[0,0,320]);fog.set_actor_label('Lighting_WindowVolumetricFog')
            comp=fog.get_component_by_class(unreal.ExponentialHeightFogComponent)
            comp.set_fog_density(spec['fog_density']);comp.set_fog_height_falloff(.001)
            comp.set_volumetric_fog(True);comp.set_volumetric_fog_scattering_distribution(.2)
            comp.set_volumetric_fog_extinction_scale(spec['fog_extinction_scale'])
            comp.set_volumetric_fog_distance(spec['fog_view_distance_cm'])
            light=spawn(world,unreal.SpotLight,spec['location_cm']);light.set_actor_label('Lighting_WindowSunBeam')
            light.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(unreal.Vector(*spec['location_cm']),unreal.Vector(*spec['target_cm'])),False)
            comp=light.get_component_by_class(unreal.SpotLightComponent)
            comp.set_mobility(unreal.ComponentMobility.MOVABLE)
            comp.set_editor_property('intensity_units',unreal.LightUnits.CANDELAS)
            comp.set_intensity(spec['intensity_candela']);comp.set_attenuation_radius(1600)
            comp.set_inner_cone_angle(spec['inner_cone_deg']);comp.set_outer_cone_angle(spec['outer_cone_deg'])
            comp.set_light_color(unreal.LinearColor(1,.82,.57,1))
            comp.set_editor_property('volumetric_scattering_intensity',spec['volumetric_scattering_intensity'])
            comp.set_cast_volumetric_shadow(True)
            post=spawn(world,unreal.PostProcessVolume,[0,0,450]);post.set_actor_label('Lighting_StableExposure')
            post.set_editor_property('unbound',True)
            settings=post.get_editor_property('settings')
            settings.set_editor_property('override_auto_exposure_min_brightness',True)
            settings.set_editor_property('override_auto_exposure_max_brightness',True)
            settings.set_editor_property('auto_exposure_min_brightness',1.5)
            settings.set_editor_property('auto_exposure_max_brightness',1.5)
            post.set_editor_property('settings',settings)
        camera=spawn(world,unreal.CameraActor,level["camera"]["location_cm"])
        camera.set_actor_label("MoleV2_ReviewCamera")
        camera.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(
            unreal.Vector(*level["camera"]["location_cm"]),unreal.Vector(*level["camera"]["target_cm"])),False)
        camera.camera_component.set_field_of_view(50)
        if attic:
            spec=level['interior_camera']
            camera=spawn(world,unreal.CameraActor,spec['location_cm']);camera.set_actor_label('MoleV2_InteriorCamera')
            camera.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(unreal.Vector(*spec['location_cm']),unreal.Vector(*spec['target_cm'])),False)
            camera.camera_component.set_field_of_view(spec['fov'])
        assert unreal.EditorLoadingAndSavingUtils.save_map(world,DEST+"/Maps/"+level["name"])

def main():
    baseline=verify.protected_hashes()
    if os.environ.get("MOLE_V2_LAYOUT_ONLY")!="1":
        texture=import_texture()
        import_texture('T_MoleControlV2_Rug')
        materials=create_material_instances(create_master(texture),texture)
        create_forest_material()
        import_meshes(materials)
    create_levels()
    report=verify.validate(MANIFEST,baseline,verification="import process")
    verify.IMPORT_REPORT.write_text(json.dumps(report,indent=2),encoding="utf-8")
    unreal.log("MOLE_V2_UE_IMPORT_VALIDATION_PASSED")

if __name__=="__main__":
    main()
