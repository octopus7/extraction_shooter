"""One-off UE art import. Removed in the commit immediately following asset delivery."""
from pathlib import Path
import json
import math
import unreal

ROOT = Path(__file__).resolve().parents[2]
DEST = '/Game/Characters/CombatPatterns'
TOOLS = unreal.AssetToolsHelpers.get_asset_tools()
MESHES = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem) or unreal.get_default_object(unreal.StaticMeshEditorSubsystem)
LIB = unreal.MaterialEditingLibrary
MAT_SPECS = {
 'CP_Armor': ((0.68,0.72,0.65),0.42,0.39),
 'CP_Dark': ((0.032,0.045,0.052),0.76,0.34),
 'CP_Teal': ((0.018,0.22,0.23),0.48,0.33),
 'CP_Amber': ((1.0,0.26,0.015),0.15,0.28),
 'CP_Rubber': ((0.012,0.017,0.019),0.0,0.77),
}


def save(asset):
    assert asset.get_path_name().startswith(DEST+'/'), asset.get_path_name()
    assert unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False), asset.get_path_name()


def node(mat, cls, **props):
    result=LIB.create_material_expression(mat, cls)
    for key,value in props.items(): result.set_editor_property(key,value)
    return result


def material(name):
    path=f'{DEST}/Materials/{name}'
    mat=unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else None
    if not mat: mat=TOOLS.create_asset(name,DEST+'/Materials',unreal.Material,unreal.MaterialFactoryNew())
    LIB.delete_all_material_expressions(mat)
    return mat


def create_materials():
    result={}
    for name,(rgb,metal,rough) in MAT_SPECS.items():
        mat=material('M_'+name)
        color=node(mat,unreal.MaterialExpressionConstant3Vector,constant=unreal.LinearColor(*rgb,1))
        LIB.connect_material_property(color,'',unreal.MaterialProperty.MP_BASE_COLOR)
        for value,prop in [(metal,unreal.MaterialProperty.MP_METALLIC),(rough,unreal.MaterialProperty.MP_ROUGHNESS)]:
            scalar=node(mat,unreal.MaterialExpressionConstant,r=value)
            LIB.connect_material_property(scalar,'',prop)
        if name=='CP_Amber':
            glow=node(mat,unreal.MaterialExpressionMultiply)
            strength=node(mat,unreal.MaterialExpressionConstant,r=3.2)
            LIB.connect_material_expressions(color,'',glow,'A')
            LIB.connect_material_expressions(strength,'',glow,'B')
            LIB.connect_material_property(glow,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
        LIB.recompile_material(mat); save(mat); result[name]=mat
    for name,blend in [('M_CP_Effect',unreal.BlendMode.BLEND_TRANSLUCENT),('M_CP_Glow',unreal.BlendMode.BLEND_ADDITIVE),('M_CP_Smoke',unreal.BlendMode.BLEND_TRANSLUCENT)]:
        mat=material(name)
        mat.set_editor_property('blend_mode',blend)
        mat.set_editor_property('shading_model',unreal.MaterialShadingModel.MSM_UNLIT)
        mat.set_editor_property('two_sided',True)
        color=node(mat,unreal.MaterialExpressionVertexColor)
        strength=node(mat,unreal.MaterialExpressionScalarParameter,parameter_name='Intensity',default_value=1.0)
        glow=node(mat,unreal.MaterialExpressionMultiply)
        assert LIB.connect_material_expressions(color,'',glow,'A')
        LIB.connect_material_expressions(strength,'',glow,'B')
        LIB.connect_material_property(glow,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
        LIB.connect_material_property(color,'A',unreal.MaterialProperty.MP_OPACITY)
        LIB.recompile_material(mat); save(mat)
    return result


def import_mesh(source, materials, yaw):
    options=unreal.FbxImportUI()
    for k,v in {'import_mesh':True,'import_as_skeletal':False,'import_animations':False,'import_materials':False,'import_textures':False,'automated_import_should_detect_type':False,'mesh_type_to_import':unreal.FBXImportType.FBXIT_STATIC_MESH}.items(): options.set_editor_property(k,v)
    data=options.static_mesh_import_data
    for k,v in {'combine_meshes':True,'auto_generate_collision':False,'generate_lightmap_u_vs':True,'convert_scene':True,'convert_scene_unit':True,'force_front_x_axis':False,'transform_vertex_to_absolute':True,'build_nanite':False,'remove_degenerates':True,'import_uniform_scale':1.0,'import_rotation':unreal.Rotator(pitch=0,yaw=yaw,roll=0),'normal_import_method':unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS}.items(): data.set_editor_property(k,v)
    task=unreal.AssetImportTask()
    task.filename=str(source); task.destination_path=DEST+'/Meshes'; task.destination_name=source.stem
    task.automated=True; task.replace_existing=True; task.replace_existing_settings=True; task.save=True
    task.options=options; task.factory=unreal.FbxFactory()
    TOOLS.import_asset_tasks([task])
    mesh=unreal.load_asset(f'{DEST}/Meshes/{source.stem}')
    assert isinstance(mesh,unreal.StaticMesh), str(source)
    for index,slot in enumerate(mesh.get_editor_property('static_materials')):
        slot_name=str(slot.get_editor_property('material_slot_name'))
        if slot_name not in materials: slot_name=str(slot.get_editor_property('imported_material_slot_name'))
        assert slot_name in materials, (source.stem,slot_name)
        mesh.set_material(index,materials[slot_name])
    build=MESHES.get_lod_build_settings(mesh,0)
    for k,v in {'recompute_normals':False,'recompute_tangents':True,'use_mikk_t_space':True,'generate_lightmap_u_vs':True,'src_lightmap_index':0,'dst_lightmap_index':1}.items(): build.set_editor_property(k,v)
    MESHES.set_lod_build_settings(mesh,0,build)
    mesh.set_editor_property('light_map_coordinate_index',1)
    # Runtime hurtboxes own collision; art never adds a second gameplay collision surface.
    MESHES.remove_collisions(mesh)
    save(mesh)
    return mesh


materials=create_materials()
sources=sorted((ROOT/'Art/CombatPatterns/Turret').glob('SM_CP_*.fbx'))+sorted((ROOT/'Art/CombatPatterns/Robots').glob('SM_CP_*.fbx'))
assert len(sources)==9, [s.name for s in sources]
eye_source=next(s for s in sources if s.stem=='SM_CP_RobotEye')
eye=import_mesh(eye_source,materials,0.0)
center=eye.get_bounds().origin
assert math.hypot(center.x,center.y)>15, 'Eye pivot must be shell center with geometry in front'
yaw=-math.degrees(math.atan2(center.y,center.x))
if abs(yaw)<0.1: yaw=0.0
loaded={s.stem:import_mesh(s,materials,yaw) for s in sources}
center=loaded['SM_CP_RobotEye'].get_bounds().origin
assert center.x>15 and abs(center.y)<3,(yaw,center)
report={'passed':True,'import_yaw':yaw,'engine':unreal.SystemLibrary.get_engine_version(),'meshes':[]}
for name,mesh in loaded.items():
    bounds=mesh.get_bounds()
    report['meshes'].append({'name':name,'bounds_origin_cm':[bounds.origin.x,bounds.origin.y,bounds.origin.z],'size_cm':[2*bounds.box_extent.x,2*bounds.box_extent.y,2*bounds.box_extent.z],'vertices':MESHES.get_number_verts(mesh,0),'uv_channels':MESHES.get_num_uv_channels(mesh,0),'materials':[str(s.get_editor_property('material_interface').get_path_name()) for s in mesh.get_editor_property('static_materials')]})
assert all(m['vertices']>0 and m['uv_channels']>=1 for m in report['meshes'])
(ROOT/'Art/CombatPatterns/Validation/unreal_import.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
unreal.log('COMBAT_PATTERN_ART_IMPORT_PASSED')
