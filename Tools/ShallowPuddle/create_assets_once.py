"""One-off UE 5.7 authoring. Commit with assets, then remove in the next commit."""
from pathlib import Path
import unreal

DEST = "/Game/Environment/ShallowPuddle"
LIB = unreal.MaterialEditingLibrary
ASSETS = unreal.AssetToolsHelpers.get_asset_tools()


def node(mat, cls, **props):
    result = LIB.create_material_expression(mat, cls)
    for key, value in props.items():
        result.set_editor_property(key, value)
    return result


def scalar(mat, name, value):
    return node(mat, unreal.MaterialExpressionScalarParameter, parameter_name=name, default_value=value)


def vector(mat, name, values):
    return node(mat, unreal.MaterialExpressionVectorParameter, parameter_name=name,
                default_value=unreal.LinearColor(*values, 1.0))


def const(mat, value, prop):
    expression = node(mat, unreal.MaterialExpressionConstant, r=value)
    assert LIB.connect_material_property(expression, "", prop)


def custom(mat, code, inputs, output_type=unreal.CustomMaterialOutputType.CMOT_FLOAT1):
    pins = []
    for name in inputs:
        pin = unreal.CustomInput()
        pin.set_editor_property("input_name", name)
        pins.append(pin)
    expression = node(mat, unreal.MaterialExpressionCustom, code=code, output_type=output_type,
                      inputs=pins)
    for name, source in inputs.items():
        assert LIB.connect_material_expressions(source, "", expression, name), name
    return expression


def material(name):
    path = f"{DEST}/Materials/{name}"
    mat = unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else None
    if mat is None:
        mat = ASSETS.create_asset(name, f"{DEST}/Materials", unreal.Material, unreal.MaterialFactoryNew())
    assert mat
    LIB.delete_all_material_expressions(mat)
    return mat


def save_material(mat):
    LIB.layout_material_expressions(mat)
    LIB.recompile_material(mat)
    assert unreal.EditorAssetLibrary.save_loaded_asset(mat, False)


# CPU ContainsGroundPoint uses the same radius equation. Coordinates are world-space
# so the water surface and downward decal remain aligned for nonuniform size/yaw.
SHAPE = """
float3 d = WorldPos - PuddleCenter;
float2 p = float2(dot(d, PuddleAxisX), dot(d, PuddleAxisY)) / max(PuddleExtent.xy, 0.001);
float a = atan2(p.y, p.x);
float r = 0.8 + OutlineIrregularity * (0.1*sin(3*a) + 0.055*sin(5*a) + 0.025*cos(7*a));
float distanceToEdge = length(p) - r;
"""


def shape_inputs(mat):
    return {
        "WorldPos": node(mat, unreal.MaterialExpressionWorldPosition),
        "PuddleCenter": vector(mat, "PuddleCenter", (0, 0, 0)),
        "PuddleAxisX": vector(mat, "PuddleAxisX", (1, 0, 0)),
        "PuddleAxisY": vector(mat, "PuddleAxisY", (0, 1, 0)),
        "PuddleExtent": vector(mat, "PuddleExtent", (150, 100, 0)),
        "OutlineIrregularity": scalar(mat, "OutlineIrregularity", 0.8),
    }


water = material("M_ShallowPuddle_Water")
water.set_editor_property("blend_mode", unreal.BlendMode.BLEND_MASKED)
water.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_SINGLE_LAYER_WATER)
water.set_editor_property("tangent_space_normal", False)
water.set_editor_property("opacity_mask_clip_value", 0.5)
shape = shape_inputs(water)
mask = custom(water, SHAPE + "return 1.0 - smoothstep(-0.004, 0.004, distanceToEdge);", shape)
assert LIB.connect_material_property(mask, "", unreal.MaterialProperty.MP_OPACITY_MASK)
const(water, 0.0, unreal.MaterialProperty.MP_BASE_COLOR)
const(water, 0.0, unreal.MaterialProperty.MP_METALLIC)
const(water, 0.5, unreal.MaterialProperty.MP_SPECULAR)
const(water, 0.12, unreal.MaterialProperty.MP_OPACITY)
roughness = scalar(water, "WaterRoughness", 0.09)
assert LIB.connect_material_property(roughness, "", unreal.MaterialProperty.MP_ROUGHNESS)
normal_inputs = {
    "WorldPos": shape["WorldPos"],
    "Time": node(water, unreal.MaterialExpressionTime),
    "RippleStrength": scalar(water, "RippleStrength", 0.018),
    "RippleSpeed": scalar(water, "RippleSpeed", 0.6),
}
normal = custom(water, """
float t = Time * RippleSpeed;
float2 q = WorldPos.xy;
float nx = cos(dot(q,float2(0.035,0.024)) + t) + 0.4*cos(dot(q,float2(-0.068,0.044)) - t*0.8);
float ny = sin(dot(q,float2(-0.026,0.041)) - t*0.7) + 0.4*sin(dot(q,float2(0.052,0.061)) + t*1.1);
return normalize(float3(nx*RippleStrength, ny*RippleStrength, 1));
""", normal_inputs, unreal.CustomMaterialOutputType.CMOT_FLOAT3)
assert LIB.connect_material_property(normal, "", unreal.MaterialProperty.MP_NORMAL)
volume = node(water, unreal.MaterialExpressionSingleLayerWaterMaterialOutput)
for pin, name, value in (
    ("AbsorptionCoefficients", "Absorption", (0.025, 0.012, 0.008)),
    ("ScatteringCoefficients", "Scattering", (0.0015, 0.002, 0.0025)),
):
    assert LIB.connect_material_expressions(vector(water, name, value), "", volume, pin), pin
save_material(water)

wet = material("M_ShallowPuddle_WetEdge")
wet.set_editor_property("material_domain", unreal.MaterialDomain.MD_DEFERRED_DECAL)
wet.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
inputs = shape_inputs(wet)
inputs["WetEdgeWidth"] = scalar(wet, "WetEdgeWidth", 0.14)
inputs["Wetness"] = scalar(wet, "Wetness", 0.4)
opacity = custom(wet, SHAPE + "return (1.0-smoothstep(0.0, max(WetEdgeWidth,0.01), distanceToEdge))*Wetness;", inputs)
assert LIB.connect_material_property(opacity, "", unreal.MaterialProperty.MP_OPACITY)
assert LIB.connect_material_property(vector(wet, "WetGroundColor", (0.055, 0.045, 0.035)), "", unreal.MaterialProperty.MP_BASE_COLOR)
const(wet, 0.22, unreal.MaterialProperty.MP_ROUGHNESS)
save_material(wet)

bp_path = f"{DEST}/BP_ShallowPuddle"
bp = unreal.load_asset(bp_path) if unreal.EditorAssetLibrary.does_asset_exist(bp_path) else None
if bp is None:
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", unreal.TunaSweeperShallowPuddleActor)
    bp = ASSETS.create_asset("BP_ShallowPuddle", DEST, unreal.Blueprint, factory)
defaults = unreal.get_default_object(bp.generated_class())
defaults.set_editor_property("water_material", water)
defaults.set_editor_property("wet_edge_material", wet)
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
assert unreal.EditorAssetLibrary.save_loaded_asset(bp, False)

floor = material("M_ShallowPuddle_ReviewGround")
world_pos = node(floor, unreal.MaterialExpressionWorldPosition)
pattern = custom(floor, """
float checker = fmod(abs(floor(WorldPos.x/60) + floor(WorldPos.y/60)), 2);
return lerp(float3(0.23,0.20,0.16),float3(0.33,0.29,0.23),checker);
""", {"WorldPos": world_pos}, unreal.CustomMaterialOutputType.CMOT_FLOAT3)
assert LIB.connect_material_property(pattern, "", unreal.MaterialProperty.MP_BASE_COLOR)
const(floor, 0.85, unreal.MaterialProperty.MP_ROUGHNESS)
save_material(floor)

world = unreal.EditorLoadingAndSavingUtils.new_blank_map(False)


def spawn(cls, label, location, rotation=None):
    actor = unreal.EditorLevelLibrary.spawn_actor_from_class(cls, unreal.Vector(*location), rotation or unreal.Rotator())
    assert actor
    actor.set_actor_label(label)
    return actor


ground = spawn(unreal.StaticMeshActor, "Review_Ground", (0, 0, -10))
ground.static_mesh_component.set_static_mesh(unreal.load_asset("/Engine/BasicShapes/Cube"))
ground.static_mesh_component.set_material(0, floor)
ground.set_actor_scale3d(unreal.Vector(19, 12, 0.2))
ground.static_mesh_component.set_collision_profile_name("BlockAll")
for label, location, extent, rough, strength, yaw in (
    ("Puddle_Clear", (-440, 0, 3), (180, 130), 0.06, 0.01, 0),
    ("Puddle_Default", (0, 0, 6), (190, 135), 0.09, 0.018, 20),
    ("Puddle_Muddy", (440, 0, 4), (180, 120), 0.16, 0.025, -25),
):
    puddle = spawn(bp.generated_class(), label, location, unreal.Rotator(0, yaw, 0))
    puddle.set_editor_property("half_extent_cm", unreal.Vector2D(*extent))
    puddle.set_editor_property("water_roughness", rough)
    puddle.set_editor_property("ripple_strength", strength)
    if label == "Puddle_Muddy":
        puddle.set_editor_property("absorption", unreal.LinearColor(0.045, 0.065, 0.10, 1))
        puddle.set_editor_property("scattering", unreal.LinearColor(0.025, 0.019, 0.010, 1))
    puddle.refresh_puddle()

sun = spawn(unreal.DirectionalLight, "Review_Sun", (0, 0, 800), unreal.Rotator(-45, -35, 0))
sun.light_component.set_editor_property("intensity", 5.0)
sun.light_component.set_editor_property("atmosphere_sun_light", True)
sky = spawn(unreal.SkyLight, "Review_SkyLight", (0, 0, 500))
sky.light_component.set_editor_property("source_type", unreal.SkyLightSourceType.SLS_SPECIFIED_CUBEMAP)
sky.light_component.set_editor_property("cubemap", unreal.load_asset("/Engine/MapTemplates/Sky/DaylightAmbientCubemap"))
sky.light_component.set_editor_property("intensity", 1.0)
spawn(unreal.SkyAtmosphere, "Review_Atmosphere", (0, 0, 0))
camera = spawn(unreal.CameraActor, "Review_Camera", (780, -1250, 1400))
camera.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(camera.get_actor_location(), unreal.Vector(0, 0, 0)), False)
camera.camera_component.set_editor_property("field_of_view", 55)
spawn(unreal.PlayerStart, "Review_PlayerStart", (0, -360, 100))
# Raised narrow reference objects give the water something to reflect and make depth readable.
for x in (-540, -100, 340):
    stone = spawn(unreal.StaticMeshActor, "Review_Stone", (x, 55, 9))
    stone.static_mesh_component.set_static_mesh(unreal.load_asset("/Engine/BasicShapes/Sphere"))
    stone.set_actor_scale3d(unreal.Vector(0.42, 0.28, 0.25))
    stone.static_mesh_component.set_material(0, floor)
unreal.EditorLevelLibrary.set_level_viewport_camera_info(camera.get_actor_location(), camera.get_actor_rotation())
assert unreal.EditorLoadingAndSavingUtils.save_map(world, f"{DEST}/Maps/L_ShallowPuddle_Review")
unreal.log("SHALLOW_PUDDLE_ASSETS_CREATED")
