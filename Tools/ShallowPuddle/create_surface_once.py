"""One-off conversion. Commit with assets, then remove per repository policy."""
import json
from pathlib import Path
import unreal

LIB = unreal.MaterialEditingLibrary
ROOT = "/Game/Environment/ShallowPuddle/Materials/"
OUT = Path(__file__).resolve().parents[2] / "TunaSweeper/Saved/Automation/ShallowPuddle"
source = unreal.load_asset(ROOT + "M_ShallowPuddle_Water")
assert source
destination = ROOT + "M_ShallowPuddle_Surface"
assert not unreal.EditorAssetLibrary.does_asset_exist(destination), "Refusing to overwrite an existing surface asset"
material = unreal.AssetToolsHelpers.get_asset_tools().duplicate_asset("M_ShallowPuddle_Surface", ROOT.rstrip("/"), source)
assert material
mask = LIB.get_material_property_input_node(material, unreal.MaterialProperty.MP_OPACITY_MASK)
assert mask, "Original outline must survive conversion"
expressions = [e for e in unreal.ObjectIterator(unreal.MaterialExpression) if e.get_outer() == material]
assert expressions, "Could not inspect cloned material graph"
absorption = next(e for e in expressions if isinstance(e, unreal.MaterialExpressionVectorParameter)
                  and str(e.get_editor_property("parameter_name")) == "Absorption")
for e in expressions:
    if isinstance(e, unreal.MaterialExpressionSingleLayerWaterMaterialOutput):
        LIB.delete_material_expression(material, e)

def node(cls):
    return LIB.create_material_expression(material, cls)

def scalar(name, value, minimum=0.0, maximum=1.0):
    n = node(unreal.MaterialExpressionScalarParameter)
    for k, v in {"parameter_name": name, "default_value": value, "slider_min": minimum,
                 "slider_max": maximum, "group": "Shallow Water Surface"}.items():
        n.set_editor_property(k, v)
    return n

def prop(n, p):
    assert LIB.connect_material_property(n, "", p)

material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_THIN_TRANSLUCENT)
material.set_editor_property("translucency_lighting_mode", unreal.TranslucencyLightingMode.TLM_SURFACE_PER_PIXEL_LIGHTING)
material.set_editor_property("allow_front_layer_translucency", True)
material.set_editor_property("screen_space_reflections", True)
material.set_editor_property("two_sided", True)
material.set_editor_property("disable_depth_test", False)
material.set_editor_property("translucency_pass", unreal.MaterialTranslucencyPass.MTP_BEFORE_DOF)
prop(scalar("WaterSpecular", 1.0), unreal.MaterialProperty.MP_SPECULAR)
# Thin-surface transmission owns the water tint; no opaque coating is added.
zero = node(unreal.MaterialExpressionConstant)
zero.set_editor_property("r", 0.0)
prop(zero, unreal.MaterialProperty.MP_OPACITY)
transmission = scalar("WaterTransmission", 0.78)
optical_depth = scalar("WaterOpticalDepthCm", 3.0, 0.0, 30.0)
tint = node(unreal.MaterialExpressionCustom)
tint.set_editor_property("description", "Art-directed transmission independent of terrain depth")
tint.set_editor_property("code", "return saturate(Transmission) * exp(-max(Absorption.rgb, 0.0) * max(OpticalDepth, 0.0));")
tint.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT3)
inputs = []
for name in ("Absorption", "Transmission", "OpticalDepth"):
    input_ = unreal.CustomInput()
    input_.set_editor_property("input_name", name)
    inputs.append(input_)
tint.set_editor_property("inputs", inputs)
for n, e in (("Absorption", absorption), ("Transmission", transmission), ("OpticalDepth", optical_depth)):
    assert LIB.connect_material_expressions(e, "", tint, n)
thin = node(unreal.MaterialExpressionThinTranslucentMaterialOutput)
assert LIB.connect_material_expressions(tint, "", thin, "TransmittanceColor")
assert LIB.connect_material_expressions(mask, "", thin, "SurfaceCoverage")
LIB.layout_material_expressions(material)
LIB.recompile_material(material)
assert unreal.EditorAssetLibrary.save_loaded_asset(material, only_if_is_dirty=False)
OUT.mkdir(parents=True, exist_ok=True)
(OUT / "surface_created.json").write_text(json.dumps({"source": source.get_path_name(), "surface": material.get_path_name()}, indent=2), encoding="utf-8")
unreal.log("PUDDLE_SURFACE_CREATED " + material.get_path_name())
