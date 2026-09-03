import unreal


DESTINATION_PATH = "/Game/UI/WorldQuestMarker"


def create_expression(material, expression_class, x, y):
    expression = unreal.MaterialEditingLibrary.create_material_expression(
        material, expression_class, x, y
    )
    if expression is None:
        raise RuntimeError(f"Could not create material expression: {expression_class}")
    return expression


def set_parameter(expression, name, default):
    expression.set_editor_property("parameter_name", name)
    expression.set_editor_property("default_value", default)


def connect(source, destination, input_name, source_output=""):
    if not unreal.MaterialEditingLibrary.connect_material_expressions(
        source, source_output, destination, input_name
    ):
        raise RuntimeError(
            f"Could not connect {source.get_name()}:{source_output} to "
            f"{destination.get_name()}:{input_name}"
        )


def connect_property(source, material_property):
    if not unreal.MaterialEditingLibrary.connect_material_property(source, "", material_property):
        raise RuntimeError(f"Could not connect {source.get_name()} to {material_property}")


def ensure_material(name):
    path = f"{DESTINATION_PATH}/{name}"
    material = unreal.load_asset(path)
    if material is None:
        material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            name, DESTINATION_PATH, unreal.Material, unreal.MaterialFactoryNew()
        )
    if material is None:
        raise RuntimeError(f"Could not create material: {path}")
    material.modify()
    unreal.MaterialEditingLibrary.delete_all_material_expressions(material)
    return material


def build_surface_material():
    material = ensure_material("M_QuestMarkerSurface")
    material.set_editor_property("two_sided", True)

    color = create_expression(material, unreal.MaterialExpressionVectorParameter, -700, -220)
    set_parameter(color, "MarkerColor", unreal.LinearColor(1.0, 0.58, 0.055, 1.0))
    emissive_strength = create_expression(material, unreal.MaterialExpressionScalarParameter, -700, 0)
    set_parameter(emissive_strength, "EmissiveStrength", 0.2)
    emissive = create_expression(material, unreal.MaterialExpressionMultiply, -380, -80)
    connect(color, emissive, "A")
    connect(emissive_strength, emissive, "B")
    metallic = create_expression(material, unreal.MaterialExpressionScalarParameter, -700, 180)
    set_parameter(metallic, "Metallic", 0.5)
    roughness = create_expression(material, unreal.MaterialExpressionScalarParameter, -700, 300)
    set_parameter(roughness, "Roughness", 0.35)

    connect_property(color, unreal.MaterialProperty.MP_BASE_COLOR)
    connect_property(emissive, unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    connect_property(metallic, unreal.MaterialProperty.MP_METALLIC)
    connect_property(roughness, unreal.MaterialProperty.MP_ROUGHNESS)
    unreal.MaterialEditingLibrary.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material, only_if_is_dirty=False)


def build_halo_material():
    material = ensure_material("M_QuestMarkerHalo")
    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_ADDITIVE)
    material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    material.set_editor_property("two_sided", True)

    glow_color = create_expression(material, unreal.MaterialExpressionVectorParameter, -820, -220)
    set_parameter(glow_color, "GlowColor", unreal.LinearColor(1.0, 0.55, 0.035, 1.0))
    glow_strength = create_expression(material, unreal.MaterialExpressionScalarParameter, -820, -70)
    set_parameter(glow_strength, "GlowStrength", 1.65)
    opacity = create_expression(material, unreal.MaterialExpressionScalarParameter, -820, 260)
    set_parameter(opacity, "Opacity", 0.55)
    vertex_color = create_expression(material, unreal.MaterialExpressionVertexColor, -820, 100)

    emissive = create_expression(material, unreal.MaterialExpressionMultiply, -500, -170)
    connect(glow_color, emissive, "A")
    connect(glow_strength, emissive, "B")
    vertex_tint = create_expression(material, unreal.MaterialExpressionMultiply, -240, -130)
    connect(emissive, vertex_tint, "A")
    connect(vertex_color, vertex_tint, "B", "A")
    opacity_multiply = create_expression(material, unreal.MaterialExpressionMultiply, -400, 220)
    connect(vertex_color, opacity_multiply, "A", "A")
    connect(opacity, opacity_multiply, "B")

    connect_property(vertex_tint, unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    connect_property(opacity_multiply, unreal.MaterialProperty.MP_OPACITY)
    unreal.MaterialEditingLibrary.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material, only_if_is_dirty=False)


build_surface_material()
build_halo_material()
unreal.log("Created world quest marker materials.")
