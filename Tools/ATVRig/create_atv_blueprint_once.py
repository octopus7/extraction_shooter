"""One-off Blueprint authoring; remove in the commit after the generated asset."""
import unreal

folder = '/Game/Blueprints/Vehicles/ATV'
name = 'BP_ATV_TypeA'
path = f'{folder}/{name}'
assert not unreal.EditorAssetLibrary.does_asset_exist(path), f'Refusing to overwrite {path}'
factory = unreal.BlueprintFactory()
factory.set_editor_property('parent_class', unreal.TunaSweeperATVActor)
bp = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, folder, unreal.Blueprint, factory)
assert bp, 'Blueprint creation failed'
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
defaults = unreal.get_default_object(bp.generated_class())
assert isinstance(defaults, unreal.TunaSweeperATVActor)
assert defaults.get_editor_property('mount_component').get_interaction_type() == unreal.TunaSweeperInteractionType.VEHICLE_MOUNT
assert unreal.EditorAssetLibrary.save_loaded_asset(bp, only_if_is_dirty=False)
unreal.log(f'ATV_BLUEPRINT_CREATED: {path}')
