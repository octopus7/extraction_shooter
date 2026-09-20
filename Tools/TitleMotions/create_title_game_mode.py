"""One-off title GameMode asset creation; remove after the asset commit."""
import unreal

world = unreal.EditorLoadingAndSavingUtils.load_map('/Game/Maps/IntroMap')
assert world.get_world_settings().get_editor_property('default_game_mode') is None, 'Map already overrides GameMode; prefix would not apply'
factory = unreal.BlueprintFactory()
factory.set_editor_property('parent_class', unreal.TunaSweeperGameMode)
bp = unreal.AssetToolsHelpers.get_asset_tools().create_asset('BP_TitleGameMode', '/Game/Core', unreal.Blueprint, factory)
assert bp, 'Failed to create title GameMode'
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
cdo = unreal.get_default_object(bp.generated_class())
cdo.set_editor_property('default_pawn_class', None)
cdo.set_editor_property('spectator_class', None)
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
assert unreal.EditorAssetLibrary.save_loaded_asset(bp, False)
assert cdo.get_editor_property('default_pawn_class') is None
assert cdo.get_editor_property('player_controller_class') == unreal.TunaSweeperPlayerController.static_class()
unreal.log('TITLE_GAME_MODE_ASSET_CREATED')
