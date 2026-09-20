"""One-off title set migration; removed immediately after validated asset commit."""
import unreal

ROOT = '/Game/UI/Title'
assets = unreal.AssetToolsHelpers.get_asset_tools()
texture = unreal.load_asset(ROOT + '/T_TitleMatteLake')
assert texture, 'Import the matte with the project UI texture importer first'
mat = unreal.load_asset(ROOT + '/M_TitleMatteLake')
if not mat:
    mat = assets.create_asset('M_TitleMatteLake', ROOT, unreal.Material, unreal.MaterialFactoryNew())
    mat.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_UNLIT)
    mat.set_editor_property('two_sided', True)
    lib = unreal.MaterialEditingLibrary
    sample = lib.create_material_expression(mat, unreal.MaterialExpressionTextureSample, -500, 0)
    sample.texture = texture
    exposure = lib.create_material_expression(mat, unreal.MaterialExpressionEyeAdaptation, -500, 200)
    divide = lib.create_material_expression(mat, unreal.MaterialExpressionDivide, -250, 0)
    lib.connect_material_expressions(sample, 'RGB', divide, 'A')
    lib.connect_material_expressions(exposure, '', divide, 'B')
    lib.connect_material_property(divide, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    lib.recompile_material(mat)
unreal.EditorAssetLibrary.save_loaded_asset(mat)

bp = unreal.load_asset(ROOT + '/BP_TitleStudio')
if not bp:
    factory = unreal.BlueprintFactory()
    factory.set_editor_property('parent_class', unreal.TunaSweeperTitleStudioActor)
    bp = assets.create_asset('BP_TitleStudio', ROOT, unreal.Blueprint, factory)
sub = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
lib = unreal.SubobjectDataBlueprintFunctionLibrary
for handle in sub.k2_gather_subobject_data_for_blueprint(bp):
    obj = lib.get_object_for_blueprint(lib.get_data(handle), bp)
    if isinstance(obj, unreal.StaticMeshComponent) and obj.get_name() == 'MatteBackdrop':
        obj.set_material(0, mat)
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
unreal.EditorAssetLibrary.save_loaded_asset(bp)
character_bp = unreal.load_asset(ROOT + '/BP_TitlePresentationActor')
unreal.BlueprintEditorLibrary.compile_blueprint(character_bp)
unreal.EditorAssetLibrary.save_loaded_asset(character_bp, only_if_is_dirty=False)

world = unreal.EditorLoadingAndSavingUtils.load_map('/Game/Maps/IntroMap')
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
character = next(a for a in actors.get_all_level_actors() if isinstance(a, unreal.TunaSweeperTitlePresentationActor))
studio = next((a for a in actors.get_all_level_actors() if isinstance(a, unreal.TunaSweeperTitleStudioActor)), None)
if not studio:
    studio = actors.spawn_actor_from_class(bp.generated_class(), character.get_actor_location(), character.get_actor_rotation())
studio.set_actor_label('TS_TitleStudio')
studio.set_editor_property('presentation_actor', character)
studio.set_editor_property('show_studio_geometry', False)
# Construction refresh also saves the backdrop transform for non-realtime previews.
studio.set_actor_transform(studio.get_actor_transform(), False, False)
for component in studio.get_components_by_class(unreal.StaticMeshComponent):
    if component.get_name() == 'MatteBackdrop':
        component.set_material(0, mat)
assert unreal.EditorLoadingAndSavingUtils.save_map(world, '/Game/Maps/IntroMap')
print('TITLE_STUDIO_MIGRATION_OK')
unreal.SystemLibrary.quit_editor()
