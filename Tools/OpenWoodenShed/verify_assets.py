"""Read-only verification; run with the UE PythonScript commandlet."""
import json
from pathlib import Path
import unreal

ROOT = '/Game/Environment/Buildings/OpenWoodenShed'
registry = unreal.AssetRegistryHelpers.get_asset_registry()
registry.search_all_assets(True)
entries = registry.get_assets_by_path(ROOT, recursive=True)
assert len(entries) == 9, [(str(e.asset_name), str(e.asset_class_path)) for e in entries]
loaded = [entry.get_asset() for entry in entries]
texture = unreal.load_asset(ROOT + '/Textures/T_Shed_Atlas_BaseColor')
assert texture and sum(isinstance(a, unreal.Texture2D) for a in loaded) == 1
materials = [a for a in loaded if isinstance(a, unreal.MaterialInstanceConstant)]
meshes = [a for a in loaded if isinstance(a, unreal.StaticMesh)]
assert len(materials) == 5 and len(meshes) == 3
for mat in materials:
    assert mat.get_name().startswith('MI_')
    textures = [p.parameter_value for p in mat.get_editor_property('texture_parameter_values')]
    assert textures == ([] if mat.get_name() == 'MI_Lantern_WarmGlass' else [texture]), (mat.get_name(), textures)
for mesh in meshes:
    assert mesh.get_name().startswith('SM_')
    assert all(slot.material_interface in materials for slot in mesh.static_materials)

bp = unreal.load_asset('/Game/Interaction/DemoEnding/BP_FoodWarehouse')
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
subsystem = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
lib = unreal.SubobjectDataBlueprintFunctionLibrary
components = {}
for handle in subsystem.k2_gather_subobject_data_for_blueprint(bp):
    data = lib.get_data(handle)
    obj = lib.get_object_for_blueprint(data, bp)
    if isinstance(obj, unreal.StaticMeshComponent):
        name = str(lib.get_variable_name(data))
        components[name] = obj
        assert obj.static_mesh in meshes, (name, obj.static_mesh)
        location, scale = obj.relative_location, obj.relative_scale3d
        assert (location.x, location.y, location.z) == (0, 0, 0)
        assert (scale.x, scale.y, scale.z) == (1, 1, 1)
        rotation = obj.relative_rotation
        assert (rotation.pitch, rotation.yaw, rotation.roll) == (0, 0, 0)
        assert not obj.get_editor_property('override_materials')
        if name.startswith('Door'):
            parent = lib.get_data(lib.get_parent_handle(data))
            assert str(lib.get_variable_name(parent)) == 'WarehouseMesh'
assert set(components) == {'WarehouseMesh', 'DoorLeft', 'DoorRight'}, components
assert components['WarehouseMesh'].static_mesh.get_name() == 'SM_Shed_Body_Shelves_Props'
for side in ('Left', 'Right'):
    assert components['Door' + side].static_mesh.get_name() == 'SM_Door_' + side + '_Open_HingePivot'
cdo = unreal.get_default_object(bp.generated_class())
assert str(cdo.get_editor_property('required_quest_id')) == 'demo_q4_todays_reward'
assert cdo.get_editor_property('food_item_id') == 3004
assert cdo.get_editor_property('food_quantity') == 1
assert str(cdo.get_editor_property('interaction_display_name_string_key')) == 'ui.interaction.take_tuna_can'
result = {'status': 'passed', 'textures': 1, 'materials': 5, 'meshes': 3,
          'components': {name: c.static_mesh.get_path_name() for name, c in components.items()}}
output = Path(unreal.Paths.project_saved_dir()) / 'Automation/OpenWoodenShed/verification.json'
output.parent.mkdir(parents=True, exist_ok=True)
output.write_text(json.dumps(result, indent=2), encoding='utf-8')
print('SHED_VERIFIED ' + json.dumps(result))
