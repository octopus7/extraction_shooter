"""One-off migration of imported shed assets onto the food warehouse Blueprint."""
from pathlib import Path
import json
import unreal

ROOT = "/Game/Environment/Buildings/OpenWoodenShed"
BP_PATH = "/Game/Interaction/DemoEnding/BP_FoodWarehouse"
assets = unreal.EditorAssetLibrary
registry = unreal.AssetRegistryHelpers.get_asset_registry()
registry.search_all_assets(True)
loaded = [unreal.load_asset(p) for p in assets.list_assets(ROOT)]
assert all(loaded)
# Load every referencer before consolidating so replacement is persisted.
options = unreal.AssetRegistryDependencyOptions(True, True, True, True, True)
for asset in loaded:
    for ref in registry.get_referencers(asset.get_path_name().split('.')[0], options):
        assert str(ref).startswith(ROOT + "/"), str(ref)
        unreal.load_asset(str(ref))

texture = unreal.load_asset(ROOT + "/Textures/Shed_Atlas_BaseColor")
duplicates = [unreal.load_asset(ROOT + "/Textures/Shed_Atlas_BaseColor" + str(i)) for i in (1, 2, 3)]
assert texture and all(duplicates)
assert assets.consolidate_assets(texture, duplicates)
renamed = []
for asset in loaded:
    if asset in duplicates:
        continue
    prefix = "T_" if isinstance(asset, unreal.Texture2D) else "MI_" if isinstance(asset, unreal.MaterialInstanceConstant) else "SM_"
    old = asset.get_path_name().split('.')[0]
    new = old.rsplit('/', 1)[0] + '/' + prefix + asset.get_name()
    assert assets.rename_asset(old, new), old
    renamed.append(asset)

bp = unreal.load_asset(BP_PATH)
subsystem = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
lib = unreal.SubobjectDataBlueprintFunctionLibrary
handles = subsystem.k2_gather_subobject_data_for_blueprint(bp)
root_handle = next(h for h in handles if str(lib.get_variable_name(lib.get_data(h))) == 'WarehouseMesh')
root = lib.get_object_for_blueprint(lib.get_data(root_handle), bp)
root.set_static_mesh(unreal.load_asset(ROOT + '/StaticMeshes/SM_Shed_Body_Shelves_Props'))
root.set_editor_property('override_materials', [])
for side in ('Left', 'Right'):
    handle, reason = subsystem.add_new_subobject(unreal.AddNewSubobjectParams(
        parent_handle=root_handle, new_class=unreal.StaticMeshComponent, blueprint_context=bp))
    assert lib.is_handle_valid(handle), str(reason)
    assert subsystem.rename_subobject(handle, 'Door' + side)
    component = lib.get_object_for_blueprint(lib.get_data(handle), bp)
    component.set_static_mesh(unreal.load_asset(ROOT + '/StaticMeshes/SM_Door_' + side + '_Open_HingePivot'))
    # Interchange baked the GLB node transforms into these mesh vertices.
    component.set_editor_property('relative_location', unreal.Vector(0, 0, 0))
    component.set_editor_property('relative_rotation', unreal.Rotator(0, 0, 0))
    component.set_editor_property('relative_scale3d', unreal.Vector(1, 1, 1))
    component.set_editor_property('mobility', root.get_editor_property('mobility'))
    component.set_collision_profile_name('BlockAll')
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
for asset in renamed + [bp]:
    assert assets.save_loaded_asset(asset, only_if_is_dirty=False), asset.get_path_name()
print('SHED_MIGRATION_SAVED')
