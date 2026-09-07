"""One-off in-place correction of the VFX vertex-colour output connection."""
import unreal
lib=unreal.MaterialEditingLibrary
for name in ['M_CP_Effect','M_CP_Glow','M_CP_Smoke']:
    mat=unreal.load_asset('/Game/Characters/CombatPatterns/Materials/'+name)
    glow=lib.get_material_property_input_node(mat,unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    color=lib.get_material_property_input_node(mat,unreal.MaterialProperty.MP_OPACITY)
    assert isinstance(glow,unreal.MaterialExpressionMultiply)
    assert isinstance(color,unreal.MaterialExpressionVertexColor)
    assert lib.connect_material_expressions(color,'',glow,'A')
    lib.recompile_material(mat)
    assert unreal.EditorAssetLibrary.save_loaded_asset(mat,only_if_is_dirty=False)
unreal.log('COMBAT_ART_COLOR_CONNECTIONS_REPAIRED')
