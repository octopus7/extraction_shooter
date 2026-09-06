"""UE 5.7 commandlet: import/verify the separate concrete garage kit.

UnrealEditor-Cmd TunaSweeper.uproject -EnablePlugins=PythonScriptPlugin,EditorScriptingUtilities
  -run=pythonscript -script=.../import_unreal.py -unattended -nullrhi -nosplash
Set GARAGE_VERIFY_ONLY=1 in the environment for a fresh-process read-only audit.
"""
from pathlib import Path
import hashlib
import json
import os
import unreal

ROOT=Path(__file__).resolve().parents[2]
SOURCE=ROOT/'TunaSweeper/SourceArt/Environment/GarageDoorConcrete'
DEST='/Game/Environment/Bunker/GarageDoorConcrete'
MANIFEST=json.loads((SOURCE/'model_manifest.json').read_text(encoding='utf-8'))
VERIFY_ONLY=os.environ.get('GARAGE_VERIFY_ONLY')=='1'
assets=unreal.AssetToolsHelpers.get_asset_tools()
mesh_editor=unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
if mesh_editor is None:
    # Commandlets do not instantiate editor subsystems; these read-only helpers
    # operate solely on the supplied mesh and can run on their class default.
    mesh_editor=unreal.get_default_object(unreal.StaticMeshEditorSubsystem)
texture_name='T_GarageConcrete_Atlas'
texture_path=f'{DEST}/Textures/{texture_name}'


def protected_hashes():
    paths=list((ROOT/'TunaSweeper/Content/Environment/Bunker/GarageDoor').rglob('*.uasset'))
    return {str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in paths}


def save(asset):
    if not asset.get_path_name().startswith(DEST+'/'):
        raise RuntimeError('Refusing to save outside new garage folder')
    if not unreal.EditorAssetLibrary.save_loaded_asset(asset,only_if_is_dirty=False):
        raise RuntimeError('Asset save failed: '+asset.get_path_name())


before=protected_hashes()
if not VERIFY_ONLY:
    task=unreal.AssetImportTask()
    task.filename=str(SOURCE/'Textures'/f'{texture_name}.png')
    task.destination_path=DEST+'/Textures'
    task.destination_name=texture_name
    task.automated=True;task.replace_existing=True;task.save=True
    assets.import_asset_tasks([task])
texture=unreal.load_asset(texture_path)
assert isinstance(texture,unreal.Texture2D),texture_path
if not VERIFY_ONLY:
    texture.set_editor_property('srgb',True)
    texture.set_editor_property('lod_group',unreal.TextureGroup.TEXTUREGROUP_WORLD)
    texture.set_editor_property('power_of_two_mode',unreal.TexturePowerOfTwoSetting.STRETCH_TO_POWER_OF_TWO)
    texture.set_editor_property('mip_gen_settings',unreal.TextureMipGenSettings.TMGS_FROM_TEXTURE_GROUP)
    texture.set_editor_property('never_stream',False)
    save(texture)

materials={}
for spec in MANIFEST['materials'].values():
    name=spec['name']
    path=f'{DEST}/Materials/{name}'
    mat=unreal.load_asset(path)
    if not VERIFY_ONLY:
        if not mat:
            mat=assets.create_asset(name,DEST+'/Materials',unreal.Material,unreal.MaterialFactoryNew())
        assert mat
        unreal.MaterialEditingLibrary.delete_all_material_expressions(mat)
        tex=unreal.MaterialEditingLibrary.create_material_expression(mat,unreal.MaterialExpressionTextureSample,-450,-140)
        tex.set_editor_property('texture',texture)
        unreal.MaterialEditingLibrary.connect_material_property(tex,'RGB',unreal.MaterialProperty.MP_BASE_COLOR)
        for field,prop,y in [('metallic',unreal.MaterialProperty.MP_METALLIC,80),('roughness',unreal.MaterialProperty.MP_ROUGHNESS,180)]:
            node=unreal.MaterialEditingLibrary.create_material_expression(mat,unreal.MaterialExpressionConstant,-350,y)
            node.set_editor_property('r',spec[field])
            unreal.MaterialEditingLibrary.connect_material_property(node,'',prop)
        unreal.MaterialEditingLibrary.recompile_material(mat)
        save(mat)
    assert isinstance(mat,unreal.Material),path
    # Inspect the persisted graph: NullRHI has no compiled shader texture list.
    color_input=unreal.MaterialEditingLibrary.get_material_property_input_node(mat,unreal.MaterialProperty.MP_BASE_COLOR)
    assert isinstance(color_input,unreal.MaterialExpressionTextureSample),path+' missing base-color sample'
    assert color_input.get_editor_property('texture').get_path_name()==texture.get_path_name(),path+' missing atlas'
    materials[name]=mat

# Separate LED and shared CPD lens materials. Default signal emission is zero.
lib=unreal.MaterialEditingLibrary
for kind in ['LED','Signal']:
    name='M_GarageConcrete_'+kind
    mat=unreal.load_asset(f'{DEST}/Materials/{name}')
    if not VERIFY_ONLY:
        if not mat:
            mat=assets.create_asset(name,DEST+'/Materials',unreal.Material,unreal.MaterialFactoryNew())
        lib.delete_all_material_expressions(mat)
        color=lib.create_material_expression(mat,unreal.MaterialExpressionVectorParameter,-600,0)
        color.set_editor_property('parameter_name','SignalColor' if kind=='Signal' else 'LEDColor')
        color.set_editor_property('default_value',unreal.LinearColor(.015,1,.06,1) if kind=='Signal' else unreal.LinearColor(.65,.85,1,1))
        strength=lib.create_material_expression(mat,unreal.MaterialExpressionScalarParameter,-600,200)
        strength.set_editor_property('parameter_name','EmissiveStrength')
        strength.set_editor_property('default_value',0.0 if kind=='Signal' else 5.0)
        if kind=='Signal':
            for node,index in [(strength,0),(color,1)]:
                node.set_editor_property('use_custom_primitive_data',True)
                node.set_editor_property('primitive_data_index',index)
        emission=lib.create_material_expression(mat,unreal.MaterialExpressionMultiply,-200,200)
        lib.connect_material_expressions(color,'RGB',emission,'A')
        lib.connect_material_expressions(strength,'',emission,'B')
        lib.connect_material_property(emission,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
        base=lib.create_material_expression(mat,unreal.MaterialExpressionMultiply,-200,-100)
        base.set_editor_property('const_b',.12 if kind=='Signal' else 1.0)
        lib.connect_material_expressions(color,'RGB',base,'A')
        lib.connect_material_property(base,'',unreal.MaterialProperty.MP_BASE_COLOR)
        rough=lib.create_material_expression(mat,unreal.MaterialExpressionConstant,-200,350)
        rough.set_editor_property('r',.28)
        lib.connect_material_property(rough,'',unreal.MaterialProperty.MP_ROUGHNESS)
        lib.recompile_material(mat);save(mat)
    assert isinstance(mat,unreal.Material)
    emission=lib.get_material_property_input_node(mat,unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    assert isinstance(emission,unreal.MaterialExpressionMultiply)
    inputs=lib.get_inputs_for_material_expression(mat,emission)
    color=next(n for n in inputs if isinstance(n,unreal.MaterialExpressionVectorParameter))
    strength=next(n for n in inputs if isinstance(n,unreal.MaterialExpressionScalarParameter))
    if kind=='Signal':
        assert strength.get_editor_property('default_value')==0
        for node,index in [(strength,0),(color,1)]:
            assert node.get_editor_property('use_custom_primitive_data')
            assert node.get_editor_property('primitive_data_index')==index
    else:
        assert strength.get_editor_property('default_value')==5
    materials[name]=mat

if not VERIFY_ONLY:
    for entry in MANIFEST['assets']:
        options=unreal.FbxImportUI()
        options.set_editor_property('import_mesh',True)
        options.set_editor_property('import_as_skeletal',False)
        options.set_editor_property('import_animations',False)
        options.set_editor_property('import_materials',False)
        options.set_editor_property('import_textures',False)
        options.set_editor_property('automated_import_should_detect_type',False)
        options.set_editor_property('mesh_type_to_import',unreal.FBXImportType.FBXIT_STATIC_MESH)
        data=options.static_mesh_import_data
        for key,value in {'combine_meshes':True,'auto_generate_collision':True,
                          'one_convex_hull_per_ucx':True,'generate_lightmap_u_vs':True,
                          'convert_scene':True,'convert_scene_unit':True,'force_front_x_axis':False,
                          'transform_vertex_to_absolute':True,'build_nanite':False,
                          'remove_degenerates':True,'import_uniform_scale':1.0}.items():
            data.set_editor_property(key,value)
        data.set_editor_property('normal_import_method',unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS)
        data.set_editor_property('normal_generation_method',unreal.FBXNormalGenerationMethod.BUILT_IN)
        task=unreal.AssetImportTask()
        task.filename=str(SOURCE/'Models'/(entry['name']+'.fbx'))
        task.destination_path=DEST+'/Meshes';task.destination_name=entry['name']
        task.automated=True;task.replace_existing=True;task.replace_existing_settings=True
        task.save=True;task.options=options;task.factory=unreal.FbxFactory()
        assets.import_asset_tasks([task])
        mesh=unreal.load_asset(f"{DEST}/Meshes/{entry['name']}")
        assert isinstance(mesh,unreal.StaticMesh),entry['name']
        slots=list(mesh.get_editor_property('static_materials'))
        for index,slot in enumerate(slots):
            name=str(slot.get_editor_property('material_slot_name'))
            if name not in materials:
                name=str(slot.get_editor_property('imported_material_slot_name'))
            assert name in materials,('Unknown slot',entry['name'],name)
            mesh.set_material(index,materials[name])
        build=mesh_editor.get_lod_build_settings(mesh,0)
        for key,value in {'recompute_tangents':True,'use_mikk_t_space':False,
                          'use_full_precision_u_vs':True,'use_high_precision_tangent_basis':True,
                          'generate_lightmap_u_vs':True,'src_lightmap_index':0,'dst_lightmap_index':1}.items():
            build.set_editor_property(key,value)
        mesh_editor.set_lod_build_settings(mesh,0,build)
        mesh.set_editor_property('light_map_coordinate_index',1)
        save(mesh)

report={'destination':DEST,'engine':unreal.SystemLibrary.get_engine_version(),
        'verification':'fresh-process reload' if VERIFY_ONLY else 'import process',
        'texture':texture.get_path_name(),'texture_srgb':texture.get_editor_property('srgb'),
        'materials':[m.get_path_name() for m in materials.values()], 'assets':[]}
assert report['texture_srgb']
for entry in MANIFEST['assets']:
    mesh=unreal.load_asset(f"{DEST}/Meshes/{entry['name']}")
    assert isinstance(mesh,unreal.StaticMesh),entry['name']
    verts=mesh_editor.get_number_verts(mesh,0)
    uv_count=mesh_editor.get_num_uv_channels(mesh,0)
    collision_count=mesh_editor.get_simple_collision_count(mesh)+mesh_editor.get_convex_collision_count(mesh)
    # GetNumUVChannels queries source mesh UVs; generated lightmap UVs live in render data.
    build=mesh_editor.get_lod_build_settings(mesh,0)
    assert verts>0 and uv_count>=1,(entry['name'],verts,uv_count)
    assert build.get_editor_property('generate_lightmap_u_vs')
    assert mesh.get_editor_property('light_map_coordinate_index')==1
    assert collision_count>0,(entry['name'],collision_count)
    slots=list(mesh.get_editor_property('static_materials'))
    for slot in slots:
        mat=slot.get_editor_property('material_interface')
        assert mat and mat.get_path_name().startswith(DEST+'/Materials/')
    bounds=mesh.get_bounds()
    center=[bounds.origin.x,bounds.origin.y,bounds.origin.z]
    extent=[bounds.box_extent.x,bounds.box_extent.y,bounds.box_extent.z]
    actual=[center[i]-extent[i] for i in range(3)]+[center[i]+extent[i] for i in range(3)]
    report['assets'].append({'name':entry['name'],'path':mesh.get_path_name(),'vertices_lod0':verts,
                            'source_uv_channels':uv_count,'generated_lightmap_uv_channel':1,
                            'simple_collisions':collision_count,
                            'material_slots':len(slots),'bounds_cm':actual})

# Verify the centered native X/Y/Z dimension contract against loaded render bounds.
for source,imported in zip(MANIFEST['assets'],report['assets']):
    actual=imported['bounds_cm']
    expected=[-v/2 for v in source['size_cm']]+[v/2 for v in source['size_cm']]
    error=max(abs(a-b) for a,b in zip(expected,actual))
    assert error<.1,('Native size/center mismatch',source['name'],error)
    imported['max_bounds_error_cm']=error
after=protected_hashes()
assert before==after,'Existing garage/BP assets changed'
report['existing_garage_assets_unchanged']=True
report['protected_assets_sha256']=after
report['passed']=True
result_path=SOURCE/('unreal_reload_validation.json' if VERIFY_ONLY else 'unreal_import_validation.json')
result_path.write_text(json.dumps(report,indent=2),encoding='utf-8')
unreal.log('GARAGE_UE_VALIDATION_PASSED '+str(result_path))
