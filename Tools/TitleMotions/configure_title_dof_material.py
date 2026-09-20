"""One-off saved matte setup; remove immediately after the asset commit."""
import unreal

mat = unreal.load_asset('/Game/UI/Title/M_TitleMatteLake')
assert mat
mat.set_editor_property('translucency_pass', unreal.MaterialTranslucencyPass.MTP_BEFORE_DOF)
mat.set_editor_property('output_translucent_velocity', True)
unreal.MaterialEditingLibrary.recompile_material(mat)
assert unreal.EditorAssetLibrary.save_loaded_asset(mat, False)
assert mat.get_editor_property('translucency_pass') == unreal.MaterialTranslucencyPass.MTP_BEFORE_DOF
assert mat.get_editor_property('output_translucent_velocity')
unreal.log('TITLE_DOF_MATERIAL_SAVED')
