"""Read-only contract checks for the shallow, translucent water material."""
import json
import traceback
from pathlib import Path
import unreal

OUT = Path(__file__).resolve().parents[2] / "TunaSweeper/Saved/Automation/ShallowPuddle"
BASE = "/Game/Environment/ShallowPuddle/Materials/"
result = {"status": "failed"}
try:
    material = unreal.load_asset(BASE + "M_ShallowPuddle_Surface")
    assert material, "Dedicated shallow surface material has not been created"
    assert material.get_editor_property("shading_model") == unreal.MaterialShadingModel.MSM_THIN_TRANSLUCENT
    assert material.get_editor_property("translucency_lighting_mode") == unreal.TranslucencyLightingMode.TLM_SURFACE_PER_PIXEL_LIGHTING
    assert material.get_editor_property("allow_front_layer_translucency")
    assert not material.get_editor_property("disable_depth_test")
    for prop in (unreal.MaterialProperty.MP_NORMAL, unreal.MaterialProperty.MP_SPECULAR):
        assert unreal.MaterialEditingLibrary.get_material_property_input_node(material, prop), str(prop)
    parameters = set(map(str, unreal.MaterialEditingLibrary.get_scalar_parameter_names(material)))
    assert {"WaterSpecular", "WaterTransmission", "WaterOpticalDepthCm", "WaterRoughness", "RippleStrength", "RippleSpeed"} <= parameters, str(parameters)
    expressions = [e for e in unreal.ObjectIterator(unreal.MaterialExpression) if e.get_outer() == material]
    assert any(isinstance(e, unreal.MaterialExpressionThinTranslucentMaterialOutput) for e in expressions)
    assert not any(isinstance(e, unreal.MaterialExpressionSingleLayerWaterMaterialOutput) for e in expressions)
    instance = unreal.load_asset(BASE + "MI_ShallowPuddle_Surface")
    assert instance and instance.get_editor_property("parent") == material
    defaults = unreal.get_default_object(unreal.EditorAssetLibrary.load_blueprint_class("/Game/Environment/ShallowPuddle/BP_ShallowPuddle"))
    assert defaults.get_editor_property("water_material") == instance
    result.update(status="passed", material=material.get_path_name(), scalar_parameters=sorted(parameters))
except Exception:
    result["error"] = traceback.format_exc()
    unreal.log_error(result["error"])
OUT.mkdir(parents=True, exist_ok=True)
(OUT / "surface_contract.json").write_text(json.dumps(result, indent=2), encoding="utf-8")
unreal.log("PUDDLE_SURFACE_CONTRACT " + json.dumps(result))
if result["status"] != "passed":
    raise RuntimeError("Shallow surface contract failed; see surface_contract.json")
