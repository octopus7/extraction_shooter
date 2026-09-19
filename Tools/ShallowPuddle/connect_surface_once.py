"""One-off default material hookup; preserve source maps and legacy material."""
import json
from pathlib import Path
import unreal

BASE = "/Game/Environment/ShallowPuddle"
surface = unreal.load_asset(BASE + "/Materials/M_ShallowPuddle_Surface")
assert surface
instance_path = BASE + "/Materials/MI_ShallowPuddle_Surface"
assert not unreal.EditorAssetLibrary.does_asset_exist(instance_path)
instance = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
    "MI_ShallowPuddle_Surface", BASE + "/Materials", unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
unreal.MaterialEditingLibrary.set_material_instance_parent(instance, surface)
for name, value in (("WaterSpecular", 1.0), ("WaterTransmission", 0.78), ("WaterOpticalDepthCm", 3.0)):
    # UE 5.7's setter returns false even after writing; verify the actual value.
    unreal.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(instance, name, value)
    assert abs(unreal.MaterialEditingLibrary.get_material_instance_scalar_parameter_value(instance, name) - value) < 0.0001
unreal.MaterialEditingLibrary.update_material_instance(instance)
assert unreal.EditorAssetLibrary.save_loaded_asset(instance, False)
blueprint = unreal.load_asset(BASE + "/BP_ShallowPuddle")
defaults = unreal.get_default_object(blueprint.generated_class())
previous = defaults.get_editor_property("water_material").get_path_name()
defaults.set_editor_property("water_material", instance)
unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
assert unreal.EditorAssetLibrary.save_loaded_asset(blueprint, False)

report = {"previous_default": previous, "default": instance.get_path_name(), "maps": {}}
for path in (BASE + "/Maps/L_ShallowPuddle_Review", "/Game/Maps/DemoRaidMap", "/Game/Maps/DemoBoxRaidMap"):
    world = unreal.EditorLoadingAndSavingUtils.load_map(path)
    assert world
    actors = unreal.GameplayStatics.get_all_actors_of_class(world, blueprint.generated_class())
    report["maps"][path] = [{"actor": a.get_actor_label(), "material": a.get_editor_property("water_material").get_path_name()} for a in actors]
out = Path(__file__).resolve().parents[2] / "TunaSweeper/Saved/Automation/ShallowPuddle/surface_connection.json"
out.write_text(json.dumps(report, indent=2), encoding="utf-8")
unreal.log("PUDDLE_SURFACE_CONNECTED " + json.dumps(report))
