"""UE 5.7 asset-only import or fresh-process reload. No level/BP changes.
BUSH_SPREADING_VERIFY_ONLY=1 performs a read-only audit.
"""
from pathlib import Path
import unreal,json,os,itertools,hashlib
ROOT=Path(__file__).resolve().parents[3]
OUT=ROOT/'TunaSweeper/SourceArt/ForestProps/BushSpreading'
DEST='/Game/Nature/ForestProps/BushSpreading'
VERIFY=os.environ.get('BUSH_SPREADING_VERIFY_ONLY')=='1'
source=json.loads((OUT/'model_validation.json').read_text())
assert source['passed']
assets=unreal.AssetToolsHelpers.get_asset_tools()
editor=unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem) or unreal.get_default_object(unreal.StaticMeshEditorSubsystem)
def save(asset):
    assert asset.get_path_name().startswith(DEST+'/')
    assert unreal.EditorAssetLibrary.save_loaded_asset(asset,only_if_is_dirty=False)
def import_file(file,name,options=None):
    task=unreal.AssetImportTask();task.filename=str(file);task.destination_path=DEST;task.destination_name=name
    task.automated=True;task.replace_existing=True;task.replace_existing_settings=True;task.save=True
    if options:task.options=options;task.factory=unreal.FbxFactory()
    assets.import_asset_tasks([task])
    return unreal.load_asset(DEST+'/'+name)
if not VERIFY:
    texture=import_file(OUT/'Textures/T_BushSpreading_Palette.png','T_BushSpreading_Palette')
    texture.set_editor_property('srgb',True);texture.set_editor_property('lod_group',unreal.TextureGroup.TEXTUREGROUP_WORLD)
    texture.set_editor_property('never_stream',False)
    # Tiny constant-color palette: no mip chain, so adjacent color cells never
    # merge at distance. Every face samples strictly inside a flat color cell.
    texture.set_editor_property('mip_gen_settings',unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    save(texture)
    mat=unreal.load_asset(DEST+'/M_BushSpreading')
    if not mat:mat=assets.create_asset('M_BushSpreading',DEST,unreal.Material,unreal.MaterialFactoryNew())
    mat.set_editor_property('blend_mode',unreal.BlendMode.BLEND_OPAQUE)
    mat.set_editor_property('two_sided',False)
    unreal.MaterialEditingLibrary.delete_all_material_expressions(mat)
    node=unreal.MaterialEditingLibrary.create_material_expression(mat,unreal.MaterialExpressionTextureSample,-400,0);node.texture=texture
    unreal.MaterialEditingLibrary.connect_material_property(node,'RGB',unreal.MaterialProperty.MP_BASE_COLOR)
    for prop,val,y in [(unreal.MaterialProperty.MP_ROUGHNESS,.86,160),(unreal.MaterialProperty.MP_SPECULAR,.25,260),(unreal.MaterialProperty.MP_METALLIC,0,360)]:
        node=unreal.MaterialEditingLibrary.create_material_expression(mat,unreal.MaterialExpressionConstant,-250,y);node.r=val
        unreal.MaterialEditingLibrary.connect_material_property(node,'',prop)
    unreal.MaterialEditingLibrary.recompile_material(mat);save(mat)
    options=unreal.FbxImportUI()
    for key,value in {'import_mesh':True,'import_as_skeletal':False,'import_animations':False,'import_materials':False,'import_textures':False,'automated_import_should_detect_type':False,'mesh_type_to_import':unreal.FBXImportType.FBXIT_STATIC_MESH}.items():options.set_editor_property(key,value)
    data=options.static_mesh_import_data
    for key,value in {'combine_meshes':True,'auto_generate_collision':False,'generate_lightmap_u_vs':False,'convert_scene':True,'convert_scene_unit':True,'force_front_x_axis':False,'transform_vertex_to_absolute':True,'build_nanite':False,'remove_degenerates':True,'import_uniform_scale':1.0,'normal_import_method':unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS}.items():data.set_editor_property(key,value)
    mesh=import_file(OUT/'SM_BushSpreading.fbx','SM_BushSpreading',options)
    mesh.set_material(0,mat)
    build=editor.get_lod_build_settings(mesh,0)
    for key,value in {'recompute_normals':False,'recompute_tangents':True,'use_full_precision_u_vs':True,'generate_lightmap_u_vs':False}.items():build.set_editor_property(key,value)
    editor.set_lod_build_settings(mesh,0,build)
    mesh.set_editor_property('light_map_coordinate_index',1)
    mesh.set_editor_property('light_map_resolution',128)
    mesh.set_editor_property('allow_cpu_access',False)
    body=mesh.get_editor_property('body_setup')
    body.set_editor_property('collision_trace_flag',unreal.CollisionTraceFlag.CTF_USE_SIMPLE_AS_COMPLEX)
    instance=body.get_editor_property('default_instance')
    instance.set_editor_property('collision_profile_name','NoCollision')
    instance.set_editor_property('collision_enabled',unreal.CollisionEnabled.NO_COLLISION)
    body.set_editor_property('default_instance',instance)
    mesh.set_editor_property('has_navigation_data',False)
    save(mesh)
mesh=unreal.load_asset(DEST+'/SM_BushSpreading');mat=unreal.load_asset(DEST+'/M_BushSpreading');texture=unreal.load_asset(DEST+'/T_BushSpreading_Palette')
assert isinstance(mesh,unreal.StaticMesh) and isinstance(mat,unreal.Material) and isinstance(texture,unreal.Texture2D)
b=mesh.get_bounds();center=[b.origin.x,b.origin.y,b.origin.z];extent=[b.box_extent.x,b.box_extent.y,b.box_extent.z]
bounds=[center[i]-extent[i] for i in range(3)]+[center[i]+extent[i] for i in range(3)]
candidates=[]
for perm in [(0,1,2),(1,0,2)]:
    for sx,sy in itertools.product((-1,1),repeat=2):
        signs=[sx,sy,1];expected=[0]*6
        for i in range(3):
            values=[source['source']['bounds_m'][perm[i]+j]*100*signs[i] for j in [0,3]]
            expected[i]=min(values);expected[i+3]=max(values)
        candidates.append((max(abs(a-b) for a,b in zip(expected,bounds)),perm,signs))
error,permutation,signs=min(candidates)
assert error<.05,(bounds,error)
assert abs(bounds[2])<.001
uv=editor.get_num_uv_channels(mesh,0);assert uv==2,uv
collisions=editor.get_simple_collision_count(mesh)+editor.get_convex_collision_count(mesh);assert collisions==0
slots=list(mesh.static_materials);assert len(slots)==1 and slots[0].material_interface==mat
assert mat.get_editor_property('blend_mode')==unreal.BlendMode.BLEND_OPAQUE and not mat.get_editor_property('two_sided')
node=unreal.MaterialEditingLibrary.get_material_property_input_node(mat,unreal.MaterialProperty.MP_BASE_COLOR)
assert isinstance(node,unreal.MaterialExpressionTextureSample) and node.texture==texture
body=mesh.get_editor_property('body_setup')
assert body.get_editor_property('collision_trace_flag')==unreal.CollisionTraceFlag.CTF_USE_SIMPLE_AS_COMPLEX
assert str(body.get_editor_property('default_instance').get_editor_property('collision_profile_name'))=='NoCollision'
assert body.get_editor_property('default_instance').get_editor_property('collision_enabled')==unreal.CollisionEnabled.NO_COLLISION
assert not mesh.get_editor_property('has_navigation_data')
report={'passed':True,'verification':'fresh process reload' if VERIFY else 'import','engine':unreal.SystemLibrary.get_engine_version(),'mesh':mesh.get_path_name(),'material':mat.get_path_name(),'texture':texture.get_path_name(),'dimensions_cm':[x*2 for x in extent],'bounds_cm':bounds,'source_uv_channels':uv,'material_slots':len(slots),'collision_primitives':collisions,'opaque':True,'two_sided':False,'source_triangles':source['source']['triangles'],'lod0_vertices':editor.get_number_verts(mesh,0),'axis_mapping':{'blender_axis_indices_for_ue_xyz':permutation,'signs':signs,'max_bounds_error_cm':error}}
report['collision_profile']='NoCollision';report['navigation_data']=False
report['lod0_triangles']=mesh.get_num_triangles(0)
assert report['lod0_triangles']==source['source']['triangles']
report['source_sha256']={name:hashlib.sha256((OUT/name).read_bytes()).hexdigest() for name in ['SM_BushSpreading.fbx','Textures/T_BushSpreading_Palette.png']}
(OUT/('unreal_reload_validation.json' if VERIFY else 'unreal_import_validation.json')).write_text(json.dumps(report,indent=2))
unreal.log('BUSH_SPREADING_UE_VALIDATION_PASSED')
