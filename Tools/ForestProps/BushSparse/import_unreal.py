"""UE 5.7: import into the unique folder; BUSH_SPARSE_VERIFY_ONLY=1 reloads."""
from pathlib import Path
import unreal,json,os,itertools
ROOT=Path(__file__).resolve().parents[3]
OUT=ROOT/'TunaSweeper/SourceArt/Environment/ForestProps/BushSparse'
DEST='/Game/Nature/ForestProps/BushSparse'
VERIFY=os.environ.get('BUSH_SPARSE_VERIFY_ONLY')=='1'
spec=json.loads((OUT/'model_manifest.json').read_text())
assets=unreal.AssetToolsHelpers.get_asset_tools()
ed=unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem) or unreal.get_default_object(unreal.StaticMeshEditorSubsystem)
def save(a):
    assert a.get_path_name().startswith(DEST+'/')
    assert unreal.EditorAssetLibrary.save_loaded_asset(a,only_if_is_dirty=False)
def import_task(filename,name,options=None):
    t=unreal.AssetImportTask();t.filename=str(filename);t.destination_path=DEST;t.destination_name=name;t.automated=True;t.replace_existing=True;t.replace_existing_settings=True;t.save=True
    if options:t.options=options;t.factory=unreal.FbxFactory()
    assets.import_asset_tasks([t])
    a=unreal.load_asset(DEST+'/'+name);assert a;return a
if not VERIFY:
    tex=import_task(OUT/'Textures/T_BushSparse_Palette.png','T_BushSparse_Palette')
    tex.set_editor_property('srgb',True)
    tex.set_editor_property('mip_gen_settings',unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    tex.set_editor_property('filter',unreal.TextureFilter.TF_NEAREST)
    tex.set_editor_property('lod_group',unreal.TextureGroup.TEXTUREGROUP_WORLD)
    save(tex)
    mat=unreal.load_asset(DEST+'/M_BushSparse') or assets.create_asset('M_BushSparse',DEST,unreal.Material,unreal.MaterialFactoryNew())
    unreal.MaterialEditingLibrary.delete_all_material_expressions(mat)
    mat.set_editor_property('two_sided',False);mat.set_editor_property('blend_mode',unreal.BlendMode.BLEND_OPAQUE)
    t=unreal.MaterialEditingLibrary.create_material_expression(mat,unreal.MaterialExpressionTextureSample,-400,0);t.texture=tex
    unreal.MaterialEditingLibrary.connect_material_property(t,'RGB',unreal.MaterialProperty.MP_BASE_COLOR)
    for value,prop,y in [(.88,unreal.MaterialProperty.MP_ROUGHNESS,150),(0,unreal.MaterialProperty.MP_METALLIC,250)]:
        c=unreal.MaterialEditingLibrary.create_material_expression(mat,unreal.MaterialExpressionConstant,-300,y);c.r=value
        unreal.MaterialEditingLibrary.connect_material_property(c,'',prop)
    unreal.MaterialEditingLibrary.recompile_material(mat);save(mat)
    options=unreal.FbxImportUI()
    for k,v in {'import_mesh':True,'import_as_skeletal':False,'import_animations':False,'import_materials':False,'import_textures':False,'automated_import_should_detect_type':False,'mesh_type_to_import':unreal.FBXImportType.FBXIT_STATIC_MESH}.items():options.set_editor_property(k,v)
    for k,v in {'combine_meshes':True,'auto_generate_collision':False,'generate_lightmap_u_vs':False,'convert_scene':True,'convert_scene_unit':True,'force_front_x_axis':False,'transform_vertex_to_absolute':True,'build_nanite':False,'remove_degenerates':True,'import_uniform_scale':1.0,'normal_import_method':unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS}.items():options.static_mesh_import_data.set_editor_property(k,v)
    mesh=import_task(OUT/'Models/SM_BushSparse.fbx','SM_BushSparse',options)
    assert len(mesh.static_materials)==1;mesh.set_material(0,mat)
    mesh.set_editor_property('light_map_coordinate_index',1);mesh.set_editor_property('light_map_resolution',64)
    mesh.set_editor_property('has_navigation_data',False)
    build=ed.get_lod_build_settings(mesh,0)
    for k,v in {'recompute_normals':False,'recompute_tangents':True,'generate_lightmap_u_vs':False,'use_full_precision_u_vs':True}.items():build.set_editor_property(k,v)
    ed.set_lod_build_settings(mesh,0,build)
    body=mesh.get_editor_property('body_setup');instance=body.get_editor_property('default_instance');instance.set_editor_property('collision_profile_name','NoCollision');body.set_editor_property('default_instance',instance)
    save(mesh)
mesh=unreal.load_asset(DEST+'/SM_BushSparse');mat=unreal.load_asset(DEST+'/M_BushSparse');tex=unreal.load_asset(DEST+'/T_BushSparse_Palette')
assert isinstance(mesh,unreal.StaticMesh) and isinstance(mat,unreal.Material) and isinstance(tex,unreal.Texture2D)
assert len(mesh.static_materials)==1 and mesh.static_materials[0].material_interface==mat
assert mat.get_editor_property('blend_mode')==unreal.BlendMode.BLEND_OPAQUE and not mat.get_editor_property('two_sided')
node=unreal.MaterialEditingLibrary.get_material_property_input_node(mat,unreal.MaterialProperty.MP_BASE_COLOR);assert node.texture==tex
assert ed.get_num_uv_channels(mesh,0)==2
collision=ed.get_simple_collision_count(mesh)+ed.get_convex_collision_count(mesh);assert collision==0
profile=str(mesh.get_editor_property('body_setup').get_editor_property('default_instance').get_editor_property('collision_profile_name'));assert profile=='NoCollision'
assert not mesh.get_editor_property('has_navigation_data')
b=mesh.get_bounds();center=[b.origin.x,b.origin.y,b.origin.z];extent=[b.box_extent.x,b.box_extent.y,b.box_extent.z]
bounds=[center[i]-extent[i] for i in range(3)]+[center[i]+extent[i] for i in range(3)]
candidates=[]
for p in [(0,1,2),(1,0,2)]:
    for sx,sy in itertools.product([-1,1],repeat=2):
        signs=[sx,sy,1];expected=[0]*6
        for i in range(3):
            pair=[spec['bounds_m'][p[i]]*100*signs[i],spec['bounds_m'][p[i]+3]*100*signs[i]]
            expected[i]=min(pair);expected[i+3]=max(pair)
        candidates.append((max(abs(a-b) for a,b in zip(expected,bounds)),p,signs))
error,p,signs=min(candidates);assert error<.01;assert abs(bounds[2])<.01
report={'engine':unreal.SystemLibrary.get_engine_version(),'project':unreal.Paths.get_project_file_path(),'mode':'fresh-process reload' if VERIFY else 'import','mesh':mesh.get_path_name(),'bounds_cm':bounds,'dimensions_cm':[extent[i]*2 for i in range(3)],'axis_mapping':{'blender_indices_for_ue_xyz':p,'signs':signs,'error_cm':error},'vertices':ed.get_number_verts(mesh,0),'source_uv_channels':ed.get_num_uv_channels(mesh,0),'material_slots':1,'texture_srgb':tex.srgb,'opaque':True,'two_sided':False,'collision_shapes':collision,'collision_profile':profile,'navigation_data':False,'passed':True}
(OUT/('unreal_reload_validation.json' if VERIFY else 'unreal_import_validation.json')).write_text(json.dumps(report,indent=2),encoding='utf-8')
unreal.log('BUSH_SPARSE_UE_VALIDATION_PASSED')
