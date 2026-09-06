"""UE 5.7: imports only the new prop folder; never saves BP, level or memo data.

Set MEMO_VERIFY_ONLY=1 for read-only fresh-process verification after import.
"""
import hashlib
import json
import os
from pathlib import Path
import unreal

ROOT=Path(__file__).resolve().parents[2]
SOURCE=ROOT/'TunaSweeper/SourceArt/Memo/StorageDevice'
DEST='/Game/Meshes/Props/MemoStorageDevice'
SPEC=json.loads((SOURCE/'model_manifest.json').read_text())
VERIFY=os.environ.get('MEMO_VERIFY_ONLY')=='1'
assets=unreal.AssetToolsHelpers.get_asset_tools()
editor=unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
if editor is None:
    editor=unreal.get_default_object(unreal.StaticMeshEditorSubsystem)

def protected_hashes():
    result={}
    for p in (ROOT/'TunaSweeper/Content').rglob('*'):
        if not p.is_file():
            continue
        if p.suffix=='.umap' or p.name.startswith('BP_') or (p.parent.name in ('Interaction','Data') and 'Memo' in p.name):
            result[str(p.relative_to(ROOT))]=hashlib.sha256(p.read_bytes()).hexdigest()
    return result

def save(asset):
    assert asset.get_path_name().startswith(DEST+'/')
    assert unreal.EditorAssetLibrary.save_loaded_asset(asset,only_if_is_dirty=False)

before=protected_hashes()
mats={}
for name,spec in SPEC['materials'].items():
    path=f'{DEST}/Materials/{name}'
    mat=unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else None
    if not VERIFY:
        if not mat:
            mat=assets.create_asset(name,DEST+'/Materials',unreal.Material,unreal.MaterialFactoryNew())
        assert isinstance(mat,unreal.Material)
        mat.set_editor_property('two_sided',False)
        unreal.MaterialEditingLibrary.delete_all_material_expressions(mat)
        color=unreal.MaterialEditingLibrary.create_material_expression(mat,unreal.MaterialExpressionConstant3Vector,-400,-120)
        color.set_editor_property('constant',unreal.LinearColor(*spec['color']))
        unreal.MaterialEditingLibrary.connect_material_property(color,'',unreal.MaterialProperty.MP_BASE_COLOR)
        for key,prop,y in [('metallic',unreal.MaterialProperty.MP_METALLIC,40),('roughness',unreal.MaterialProperty.MP_ROUGHNESS,150)]:
            node=unreal.MaterialEditingLibrary.create_material_expression(mat,unreal.MaterialExpressionConstant,-400,y)
            node.set_editor_property('r',spec[key])
            unreal.MaterialEditingLibrary.connect_material_property(node,'',prop)
        if spec['emission']:
            emission=unreal.MaterialEditingLibrary.create_material_expression(mat,unreal.MaterialExpressionConstant3Vector,-400,270)
            emission.set_editor_property('constant',unreal.LinearColor(*[v*spec['emission'] for v in spec['color'][:3]],1))
            unreal.MaterialEditingLibrary.connect_material_property(emission,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
        unreal.MaterialEditingLibrary.recompile_material(mat)
        save(mat)
    assert isinstance(mat,unreal.Material),path
    assert not mat.get_editor_property('two_sided')
    node=unreal.MaterialEditingLibrary.get_material_property_input_node(mat,unreal.MaterialProperty.MP_BASE_COLOR)
    actual=node.get_editor_property('constant')
    assert max(abs(a-b) for a,b in zip([actual.r,actual.g,actual.b],spec['color'][:3]))<1e-5
    for key,prop in [('metallic',unreal.MaterialProperty.MP_METALLIC),('roughness',unreal.MaterialProperty.MP_ROUGHNESS)]:
        node=unreal.MaterialEditingLibrary.get_material_property_input_node(mat,prop)
        assert abs(node.get_editor_property('r')-spec[key])<1e-5
    emission=unreal.MaterialEditingLibrary.get_material_property_input_node(mat,unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    if spec['emission']:
        actual=emission.get_editor_property('constant')
        assert max(abs(a-b*spec['emission']) for a,b in zip([actual.r,actual.g,actual.b],spec['color'][:3]))<1e-5
    else:
        assert emission is None
    mats[name]=mat

mesh_path=f"{DEST}/{SPEC['name']}"
if not VERIFY:
    options=unreal.FbxImportUI()
    for key,value in {'import_mesh':True,'import_as_skeletal':False,'import_animations':False,
                      'import_materials':False,'import_textures':False,'automated_import_should_detect_type':False,
                      'mesh_type_to_import':unreal.FBXImportType.FBXIT_STATIC_MESH}.items():
        options.set_editor_property(key,value)
    data=options.static_mesh_import_data
    for key,value in {'combine_meshes':True,'auto_generate_collision':False,'one_convex_hull_per_ucx':True,
                      'generate_lightmap_u_vs':True,'convert_scene':True,'convert_scene_unit':True,
                      'force_front_x_axis':False,'transform_vertex_to_absolute':True,'build_nanite':False,
                      'remove_degenerates':True,'import_uniform_scale':1.0}.items():
        data.set_editor_property(key,value)
    data.set_editor_property('normal_import_method',unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS)
    task=unreal.AssetImportTask()
    task.filename=str(SOURCE/'Models'/f"{SPEC['name']}.fbx")
    task.destination_path=DEST
    task.destination_name=SPEC['name']
    task.automated=True
    task.replace_existing=True
    task.replace_existing_settings=True
    task.save=True
    task.options=options
    task.factory=unreal.FbxFactory()
    assets.import_asset_tasks([task])
    mesh=unreal.load_asset(mesh_path)
    assert isinstance(mesh,unreal.StaticMesh)
    for i,slot in enumerate(mesh.get_editor_property('static_materials')):
        name=str(slot.get_editor_property('material_slot_name'))
        assert name in mats,name
        mesh.set_material(i,mats[name])
    build=editor.get_lod_build_settings(mesh,0)
    for key,value in {'recompute_normals':False,'recompute_tangents':True,'use_mikk_t_space':True,
                      'use_full_precision_u_vs':True,'generate_lightmap_u_vs':True,
                      'src_lightmap_index':0,'dst_lightmap_index':1}.items():
        build.set_editor_property(key,value)
    editor.set_lod_build_settings(mesh,0,build)
    mesh.set_editor_property('light_map_coordinate_index',1)
    mesh.set_editor_property('light_map_resolution',64)
    save(mesh)

mesh=unreal.load_asset(mesh_path)
assert isinstance(mesh,unreal.StaticMesh)
bounds=mesh.get_bounds()
center=[bounds.origin.x,bounds.origin.y,bounds.origin.z]
extent=[bounds.box_extent.x,bounds.box_extent.y,bounds.box_extent.z]
actual=[center[i]-extent[i] for i in range(3)]+[center[i]+extent[i] for i in range(3)]
error=max(abs(a-b) for a,b in zip(actual,SPEC['bounds_cm']))
assert error<.01,('UE centimeters / +X connector / bottom pivot',actual,SPEC['bounds_cm'])
slots=list(mesh.get_editor_property('static_materials'))
assert [str(s.get_editor_property('material_slot_name')) for s in slots]==list(SPEC['materials'])
for slot in slots:
    name=str(slot.get_editor_property('material_slot_name'))
    assert slot.get_editor_property('material_interface')==mats[name]
collision_count=editor.get_simple_collision_count(mesh)+editor.get_convex_collision_count(mesh)
assert collision_count==1
assert editor.get_num_uv_channels(mesh,0)>=1
build=editor.get_lod_build_settings(mesh,0)
assert build.get_editor_property('generate_lightmap_u_vs')
assert not build.get_editor_property('recompute_normals')
assert mesh.get_editor_property('light_map_coordinate_index')==1
after=protected_hashes()
assert before==after,'Existing map/BP/memo data was changed'
report={'passed':True,'mode':'fresh-process reload' if VERIFY else 'import',
        'engine':unreal.SystemLibrary.get_engine_version(),'mesh':mesh.get_path_name(),
        'bounds_cm':actual,'max_bounds_error_cm':error,'vertices_lod0':editor.get_number_verts(mesh,0),
        'source_uv_channels':editor.get_num_uv_channels(mesh,0),'lightmap_uv_channel':1,
        'collision_boxes':collision_count,'materials':[m.get_path_name() for m in mats.values()],
        'texture_dependencies':[],'protected_asset_count':len(after),
        'protected_assets_unchanged':True}
(SOURCE/('unreal_reload_validation.json' if VERIFY else 'unreal_import_validation.json')).write_text(json.dumps(report,indent=2),encoding='utf-8')
unreal.log('MEMO_UE_VALIDATION_PASSED')
