"""UE 5.7 import or read-only fresh-process reload, restricted to this prop folder."""
import hashlib
import json
import os
from pathlib import Path
import unreal

ROOT=Path(__file__).resolve().parents[3]
OUT=ROOT/'TunaSweeper/SourceArt/Environment/GrassDenseShort'
DEST='/Game/Nature/ForestProps/GrassDenseShort'
VERIFY=os.environ.get('GRASS_DENSE_MODE')=='reload'
manifest=json.loads((OUT/'model_manifest.json').read_text())
assets=unreal.AssetToolsHelpers.get_asset_tools()
sub=unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem) or unreal.get_default_object(unreal.StaticMeshEditorSubsystem)
def protected():
    return {str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest()
            for d in ['Bush','GrassLow','Flower','SimpleTree','Wood','RockBasic']
            for p in (ROOT/'TunaSweeper/Content/Nature'/d).glob('*.uasset')}
before=protected()
def save(asset):
    assert asset.get_path_name().startswith(DEST+'/')
    assert unreal.EditorAssetLibrary.save_loaded_asset(asset,only_if_is_dirty=False)
def task(path,name,options=None):
    t=unreal.AssetImportTask();t.filename=str(path);t.destination_path=DEST;t.destination_name=name
    t.automated=True;t.replace_existing=True;t.save=True
    if options:t.options=options;t.factory=unreal.FbxFactory()
    assets.import_asset_tasks([t])

if not VERIFY:
    task(OUT/'Textures/T_GrassDenseShort_BaseColor.png','T_GrassDenseShort_BaseColor')
tex=unreal.load_asset(DEST+'/T_GrassDenseShort_BaseColor');assert isinstance(tex,unreal.Texture2D)
if not VERIFY:
    tex.set_editor_property('srgb',True)
    tex.set_editor_property('lod_group',unreal.TextureGroup.TEXTUREGROUP_WORLD)
    tex.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_DEFAULT)
    save(tex)
mat=unreal.load_asset(DEST+'/M_GrassDenseShort')
if not VERIFY:
    if not mat:mat=assets.create_asset('M_GrassDenseShort',DEST,unreal.Material,unreal.MaterialFactoryNew())
    mat.set_editor_property('two_sided',True)
    mat.set_editor_property('blend_mode',unreal.BlendMode.BLEND_OPAQUE)
    mat.set_editor_property('used_with_instanced_static_meshes',True)
    unreal.MaterialEditingLibrary.delete_all_material_expressions(mat)
    node=unreal.MaterialEditingLibrary.create_material_expression(mat,unreal.MaterialExpressionTextureSample,-450,0)
    node.set_editor_property('texture',tex)
    unreal.MaterialEditingLibrary.connect_material_property(node,'RGB',unreal.MaterialProperty.MP_BASE_COLOR)
    for value,prop,y in [(.92,unreal.MaterialProperty.MP_ROUGHNESS,160),(.18,unreal.MaterialProperty.MP_SPECULAR,230),(0,unreal.MaterialProperty.MP_METALLIC,300)]:
        n=unreal.MaterialEditingLibrary.create_material_expression(mat,unreal.MaterialExpressionConstant,-200,y)
        n.set_editor_property('r',value);unreal.MaterialEditingLibrary.connect_material_property(n,'',prop)
    unreal.MaterialEditingLibrary.recompile_material(mat);save(mat)
assert isinstance(mat,unreal.Material)
if not VERIFY:
    options=unreal.FbxImportUI()
    for key,value in {'import_mesh':True,'import_as_skeletal':False,'import_animations':False,
                      'import_materials':False,'import_textures':False,'automated_import_should_detect_type':False,
                      'mesh_type_to_import':unreal.FBXImportType.FBXIT_STATIC_MESH}.items():options.set_editor_property(key,value)
    data=options.static_mesh_import_data
    for key,value in {'combine_meshes':True,'auto_generate_collision':False,'generate_lightmap_u_vs':False,
                      'convert_scene':True,'convert_scene_unit':True,'force_front_x_axis':False,
                      'transform_vertex_to_absolute':True,'build_nanite':False,'remove_degenerates':True,
                      'import_uniform_scale':1.0,'normal_import_method':unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS}.items():data.set_editor_property(key,value)
    task(OUT/'Models/SM_GrassDenseShort.fbx','SM_GrassDenseShort',options)
mesh=unreal.load_asset(DEST+'/SM_GrassDenseShort');assert isinstance(mesh,unreal.StaticMesh)
if not VERIFY:
    mesh.set_material(0,mat)
    sub.remove_collisions(mesh)
    body=mesh.get_editor_property('body_setup')
    instance=body.get_editor_property('default_instance')
    instance.set_editor_property('collision_profile_name','NoCollision')
    body.set_editor_property('default_instance',instance)
    build=sub.get_lod_build_settings(mesh,0)
    build.set_editor_property('generate_lightmap_u_vs',False)
    build.set_editor_property('recompute_normals',False)
    build.set_editor_property('recompute_tangents',True)
    build.set_editor_property('use_full_precision_u_vs',True)
    sub.set_lod_build_settings(mesh,0,build)
    save(mesh)
b=mesh.get_bounds()
bounds=[b.origin.x-b.box_extent.x,b.origin.y-b.box_extent.y,b.origin.z-b.box_extent.z,
        b.origin.x+b.box_extent.x,b.origin.y+b.box_extent.y,b.origin.z+b.box_extent.z]
s=manifest['bounds_m']
expected=[s[0]*100,-s[4]*100,s[2]*100,s[3]*100,-s[1]*100,s[5]*100]
error=max(abs(a-b) for a,b in zip(bounds,expected))
assert error<.01,('bounds/axis mismatch',bounds,expected)
assert sub.get_num_uv_channels(mesh,0)==1
assert mesh.get_num_triangles(0)==manifest['triangles']
assert mesh.get_num_lods()==1 and mesh.get_num_sections(0)==1
assert sub.get_simple_collision_count(mesh)==0 and sub.get_convex_collision_count(mesh)==0
assert len(mesh.static_materials)==1 and mesh.static_materials[0].material_interface==mat
assert mat.get_editor_property('two_sided') and mat.get_editor_property('blend_mode')==unreal.BlendMode.BLEND_OPAQUE
assert mat.get_editor_property('used_with_instanced_static_meshes')
assert tex.get_editor_property('srgb')
assert unreal.MaterialEditingLibrary.get_material_property_input_node(mat,unreal.MaterialProperty.MP_BASE_COLOR).texture==tex
assert str(mesh.get_editor_property('body_setup').get_editor_property('default_instance').get_editor_property('collision_profile_name'))=='NoCollision'
assert protected()==before
report={'passed':True,'mode':'fresh-process reload' if VERIFY else 'import','engine':unreal.SystemLibrary.get_engine_version(),
        'host':'Content-only UE 5.7 validation project; Content junction targets actual TunaSweeper Content',
        'asset':mesh.get_path_name(),'bounds_cm':bounds,'max_axis_bounds_error_cm':error,
        'vertices_lod0':sub.get_number_verts(mesh,0),'triangles_lod0':mesh.get_num_triangles(0),
        'lod_count':mesh.get_num_lods(),'sections_lod0':mesh.get_num_sections(0),'uv_channels':sub.get_num_uv_channels(mesh,0),
        'material_slots':1,'two_sided':True,'blend_mode':'Opaque','collision_primitives':0,
        'collision_profile':'NoCollision','instanced_static_mesh_usage':True,'texture_srgb':True,
        'existing_nature_assets_unchanged':True,'protected_assets_sha256':before}
(OUT/('unreal_reload_validation.json' if VERIFY else 'unreal_import_validation.json')).write_text(json.dumps(report,indent=2),encoding='utf-8')
unreal.log('GRASS_UE_VALIDATION_PASSED')
