import unreal


MATERIAL_PATH = "/Game/Characters/Robot/Materials"
MASTER_MATERIAL_NAME = "M_Robot_Customizable"
BASE_TEXTURE_PATH = f"{MATERIAL_PATH}/T_Robot"
MASK_TEXTURE_PATH = f"{MATERIAL_PATH}/T_Robot_BodyColorMask"
BASE_ENEMY_BLUEPRINT_PATH = "/Game/Blueprints/BP_QuadrupedGunEnemy"
VARIANT_BLUEPRINT_PATH = "/Game/Blueprints/Enemies/QuadrupedVariants"

VARIANTS = (
    {
        "suffix": "BrightGray",
        "color": unreal.LinearColor(0.82, 0.85, 0.90, 1.0),
        "metallic": 0.0,
        "roughness": 0.42,
        "specular": 0.50,
    },
    {
        "suffix": "Red",
        "color": unreal.LinearColor(0.82, 0.035, 0.025, 1.0),
        "metallic": 0.0,
        "roughness": 0.34,
        "specular": 0.50,
    },
    {
        "suffix": "Blue",
        "color": unreal.LinearColor(0.025, 0.18, 0.85, 1.0),
        "metallic": 0.05,
        "roughness": 0.28,
        "specular": 0.50,
    },
    {
        "suffix": "Gold",
        "color": unreal.LinearColor(1.0, 0.48, 0.035, 1.0),
        "metallic": 1.0,
        "roughness": 0.22,
        "specular": 0.50,
    },
)


def require_asset(asset_path):
    asset = unreal.load_asset(asset_path)
    if asset is None:
        raise RuntimeError(f"Required asset is missing: {asset_path}")
    return asset


def save_asset(asset):
    asset.modify()
    if not unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False):
        raise RuntimeError(f"Could not save asset: {asset.get_path_name()}")


def set_parameter_metadata(expression, name, group, sort_priority):
    expression.set_editor_property("parameter_name", name)
    try:
        expression.set_editor_property("group", group)
        expression.set_editor_property("sort_priority", sort_priority)
    except Exception:
        # Parameter grouping is editor presentation only; asset behavior does not depend on it.
        pass


def create_expression(material, expression_class, x, y):
    expression = unreal.MaterialEditingLibrary.create_material_expression(
        material,
        expression_class,
        x,
        y,
    )
    if expression is None:
        raise RuntimeError(f"Could not create expression: {expression_class}")
    return expression


def create_scalar_parameter(material, name, default, group, sort_priority, x, y):
    expression = create_expression(
        material,
        unreal.MaterialExpressionScalarParameter,
        x,
        y,
    )
    set_parameter_metadata(expression, name, group, sort_priority)
    expression.set_editor_property("default_value", default)
    try:
        expression.set_editor_property("slider_min", 0.0)
        expression.set_editor_property("slider_max", 1.0)
    except Exception:
        pass
    return expression


def connect(source, source_output, destination, destination_input):
    if not unreal.MaterialEditingLibrary.connect_material_expressions(
        source,
        source_output,
        destination,
        destination_input,
    ):
        raise RuntimeError(
            f"Could not connect {source.get_name()}:{source_output} to "
            f"{destination.get_name()}:{destination_input}"
        )


def create_masked_scalar_lerp(material, mask_sample, base_parameter, body_parameter, x, y):
    lerp = create_expression(material, unreal.MaterialExpressionLinearInterpolate, x, y)
    connect(base_parameter, "", lerp, "A")
    connect(body_parameter, "", lerp, "B")
    connect(mask_sample, "R", lerp, "Alpha")
    return lerp


def ensure_master_material(asset_tools, base_texture, mask_texture):
    asset_path = f"{MATERIAL_PATH}/{MASTER_MATERIAL_NAME}"
    material = unreal.load_asset(asset_path)
    if material is None:
        material = asset_tools.create_asset(
            MASTER_MATERIAL_NAME,
            MATERIAL_PATH,
            unreal.Material,
            unreal.MaterialFactoryNew(),
        )
    if material is None:
        raise RuntimeError(f"Could not create material: {asset_path}")

    material.modify()
    unreal.MaterialEditingLibrary.delete_all_material_expressions(material)

    base_sample = create_expression(
        material,
        unreal.MaterialExpressionTextureSampleParameter2D,
        -1200,
        -260,
    )
    set_parameter_metadata(base_sample, "BaseTexture", "Textures", 0)
    base_sample.set_editor_property("texture", base_texture)
    base_sample.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)

    mask_sample = create_expression(
        material,
        unreal.MaterialExpressionTextureSampleParameter2D,
        -1200,
        120,
    )
    set_parameter_metadata(mask_sample, "BodyColorMask", "Textures", 1)
    mask_sample.set_editor_property("texture", mask_texture)
    mask_sample.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_MASKS)

    body_color = create_expression(
        material,
        unreal.MaterialExpressionVectorParameter,
        -920,
        -420,
    )
    set_parameter_metadata(body_color, "BodyColor", "Body Color", 0)
    body_color.set_editor_property("default_value", unreal.LinearColor.WHITE)

    tinted_base = create_expression(material, unreal.MaterialExpressionMultiply, -640, -260)
    connect(base_sample, "RGB", tinted_base, "A")
    connect(body_color, "", tinted_base, "B")

    tint_strength = create_scalar_parameter(
        material,
        "TintStrength",
        1.0,
        "Body Color",
        1,
        -920,
        -80,
    )
    tint_alpha = create_expression(material, unreal.MaterialExpressionMultiply, -640, 80)
    connect(mask_sample, "R", tint_alpha, "A")
    connect(tint_strength, "", tint_alpha, "B")

    final_base_color = create_expression(
        material,
        unreal.MaterialExpressionLinearInterpolate,
        -340,
        -200,
    )
    connect(base_sample, "RGB", final_base_color, "A")
    connect(tinted_base, "", final_base_color, "B")
    connect(tint_alpha, "", final_base_color, "Alpha")

    base_metallic = create_scalar_parameter(
        material, "BaseMetallic", 0.0, "Base PBR", 0, -920, 260
    )
    body_metallic = create_scalar_parameter(
        material, "BodyMetallic", 0.0, "Body PBR", 0, -920, 360
    )
    metallic = create_masked_scalar_lerp(
        material, mask_sample, base_metallic, body_metallic, -340, 320
    )

    base_roughness = create_scalar_parameter(
        material, "BaseRoughness", 0.5, "Base PBR", 1, -920, 500
    )
    body_roughness = create_scalar_parameter(
        material, "BodyRoughness", 0.4, "Body PBR", 1, -920, 600
    )
    roughness = create_masked_scalar_lerp(
        material, mask_sample, base_roughness, body_roughness, -340, 540
    )

    base_specular = create_scalar_parameter(
        material, "BaseSpecular", 0.5, "Base PBR", 2, -920, 740
    )
    body_specular = create_scalar_parameter(
        material, "BodySpecular", 0.5, "Body PBR", 2, -920, 840
    )
    specular = create_masked_scalar_lerp(
        material, mask_sample, base_specular, body_specular, -340, 760
    )

    if not unreal.MaterialEditingLibrary.connect_material_property(
        final_base_color,
        "",
        unreal.MaterialProperty.MP_BASE_COLOR,
    ):
        raise RuntimeError("Could not connect Base Color material property")
    if not unreal.MaterialEditingLibrary.connect_material_property(
        metallic,
        "",
        unreal.MaterialProperty.MP_METALLIC,
    ):
        raise RuntimeError("Could not connect Metallic material property")
    if not unreal.MaterialEditingLibrary.connect_material_property(
        roughness,
        "",
        unreal.MaterialProperty.MP_ROUGHNESS,
    ):
        raise RuntimeError("Could not connect Roughness material property")
    if not unreal.MaterialEditingLibrary.connect_material_property(
        specular,
        "",
        unreal.MaterialProperty.MP_SPECULAR,
    ):
        raise RuntimeError("Could not connect Specular material property")

    unreal.MaterialEditingLibrary.recompile_material(material)
    save_asset(material)
    return material


def ensure_material_instance(asset_tools, parent_material, base_texture, mask_texture, variant):
    asset_name = f"MI_Robot_{variant['suffix']}"
    asset_path = f"{MATERIAL_PATH}/{asset_name}"
    instance = unreal.load_asset(asset_path)
    if instance is None:
        instance = asset_tools.create_asset(
            asset_name,
            MATERIAL_PATH,
            unreal.MaterialInstanceConstant,
            unreal.MaterialInstanceConstantFactoryNew(),
        )
    if instance is None:
        raise RuntimeError(f"Could not create material instance: {asset_path}")

    unreal.MaterialEditingLibrary.set_material_instance_parent(instance, parent_material)
    unreal.MaterialEditingLibrary.set_material_instance_texture_parameter_value(
        instance, "BaseTexture", base_texture
    )
    unreal.MaterialEditingLibrary.set_material_instance_texture_parameter_value(
        instance, "BodyColorMask", mask_texture
    )
    unreal.MaterialEditingLibrary.set_material_instance_vector_parameter_value(
        instance, "BodyColor", variant["color"]
    )
    scalar_values = {
        "TintStrength": 1.0,
        "BaseMetallic": 0.0,
        "BaseRoughness": 0.5,
        "BaseSpecular": 0.5,
        "BodyMetallic": variant["metallic"],
        "BodyRoughness": variant["roughness"],
        "BodySpecular": variant["specular"],
    }
    for parameter_name, value in scalar_values.items():
        unreal.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(
            instance,
            parameter_name,
            value,
        )

    unreal.MaterialEditingLibrary.update_material_instance(instance)
    save_asset(instance)
    return instance


def ensure_variant_blueprint(asset_tools, base_blueprint, material_instance, variant):
    asset_name = f"BP_QuadrupedGunEnemy_{variant['suffix']}"
    asset_path = f"{VARIANT_BLUEPRINT_PATH}/{asset_name}"
    blueprint = unreal.load_asset(asset_path)
    if blueprint is None:
        factory = unreal.BlueprintFactory()
        factory.set_editor_property("parent_class", base_blueprint.generated_class())
        blueprint = asset_tools.create_asset(
            asset_name,
            VARIANT_BLUEPRINT_PATH,
            unreal.Blueprint,
            factory,
        )
    if blueprint is None:
        raise RuntimeError(f"Could not create enemy blueprint: {asset_path}")

    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    generated_class = blueprint.generated_class()
    if generated_class is None:
        raise RuntimeError(f"Enemy blueprint has no generated class: {asset_path}")

    defaults = unreal.get_default_object(generated_class)
    mesh = defaults.get_component_by_class(unreal.SkeletalMeshComponent)
    if mesh is None:
        raise RuntimeError(f"Enemy blueprint has no character mesh: {asset_path}")

    defaults.modify()
    mesh.modify()
    mesh.set_material(0, material_instance)
    save_asset(blueprint)

    saved_material = mesh.get_material(0)
    if saved_material is None or saved_material.get_path_name() != material_instance.get_path_name():
        raise RuntimeError(f"Material override did not persist in memory: {asset_path}")
    return blueprint


def main():
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    unreal.EditorAssetLibrary.make_directory(MATERIAL_PATH)
    unreal.EditorAssetLibrary.make_directory(VARIANT_BLUEPRINT_PATH)

    base_texture = require_asset(BASE_TEXTURE_PATH)
    mask_texture = require_asset(MASK_TEXTURE_PATH)
    base_blueprint = require_asset(BASE_ENEMY_BLUEPRINT_PATH)
    if base_blueprint.generated_class() is None:
        raise RuntimeError(f"Base enemy blueprint is not compiled: {BASE_ENEMY_BLUEPRINT_PATH}")

    master_material = ensure_master_material(asset_tools, base_texture, mask_texture)
    created_assets = [master_material.get_path_name()]

    for variant in VARIANTS:
        material_instance = ensure_material_instance(
            asset_tools,
            master_material,
            base_texture,
            mask_texture,
            variant,
        )
        enemy_blueprint = ensure_variant_blueprint(
            asset_tools,
            base_blueprint,
            material_instance,
            variant,
        )
        created_assets.extend(
            (material_instance.get_path_name(), enemy_blueprint.get_path_name())
        )

    unreal.log("ROBOT_VARIANT_SETUP_SUCCESS")
    for asset_path in created_assets:
        unreal.log(f"ROBOT_VARIANT_ASSET={asset_path}")


main()
