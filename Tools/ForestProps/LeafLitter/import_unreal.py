"""Import only /Game/Nature/ForestProps/LeafLitter; fresh-process audit with env flag."""
from pathlib import Path
import unreal, json, os, itertools
ROOT=Path(__file__).resolve().parents[3]
OUT=ROOT/'TunaSweeper/SourceArt/Environment/ForestProps/LeafLitter'
DEST='/Game/Nature/ForestProps/LeafLitter'
manifest=json.loads((OUT/'model_manifest.json').read_text())
verify=os.environ.get('LEAFLITTER_VERIFY_ONLY')=='1'
at=unreal.AssetToolsHelpers.get_asset_tools()
sms=unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem) or unreal.get_default_object(unreal.StaticMeshEditorSubsystem)
def save(a):
    assert a.get_path_name().startswith(DEST+'/')
    assert unreal.EditorAssetLibrary.save_loaded_asset(a,only_if_is_dirty=False)
def task(file,name,options=None):
    t=unreal.AssetImportTask();t.filename=str(file);t.destination_path=DEST;t.destination_name=name
    t.automated=True;t.replace_existing=True;t.replace_existing_settings=True;t.save=True
    if options:t.options=options;t.factory=unreal.FbxFactory()
    at.import_asset_tasks([t])
if not verify:task(OUT/'Textures/T_LeafLitter_Palette.png','T_LeafLitter_Palette')
tex=unreal.load_asset(DEST+'/T_LeafLitter_Palette');assert isinstance(tex,unreal.Texture2D)
if not verify:
    tex.set_editor_property('srgb',True);tex.set_editor_property('lod_group',unreal.TextureGroup.TEXTUREGROUP_WORLD)
    # Constant swatches need no high-frequency filtering. Clamp avoids edge wrapping.
    tex.set_editor_property('address_x',unreal.TextureAddress.TA_CLAMP);tex.set_editor_property('address_y',unreal.TextureAddress.TA_CLAMP)
    save(tex)
mat=unreal.load_asset(DEST+'/M_LeafLitter')
if not verify:
    if not mat:mat=at.create_asset('M_LeafLitter',DEST,unreal.Material,unreal.MaterialFactoryNew())
    unreal.MaterialEditingLibrary.delete_all_material_expressions(mat)
    mat.set_editor_property('two_sided',False);mat.set_editor_property('blend_mode',unreal.BlendMode.BLEND_OPAQUE)
    node=unreal.MaterialEditingLibrary.create_material_expression(mat,unreal.MaterialExpressionTextureSample,-350,0);node.texture=tex
    unreal.MaterialEditingLibrary.connect_material_property(node,'RGB',unreal.MaterialProperty.MP_BASE_COLOR)
    for value,prop,y in [(.92,unreal.MaterialProperty.MP_ROUGHNESS,150),(0,unreal.MaterialProperty.MP_METALLIC,250),(.18,unreal.MaterialProperty.MP_SPECULAR,350)]:
        n=unreal.MaterialEditingLibrary.create_material_expression(mat,unreal.MaterialExpressionConstant,-350,y);n.r=value
        unreal.MaterialEditingLibrary.connect_material_property(n,'',prop)
    unreal.MaterialEditingLibrary.recompile_material(mat);save(mat)
    ui=unreal.FbxImportUI()
    for k,v in {'import_mesh':True,'import_as_skeletal':False,'import_animations':False,'import_materials':False,'import_textures':False,'automated_import_should_detect_type':False,'mesh_type_to_import':unreal.FBXImportType.FBXIT_STATIC_MESH}.items():ui.set_editor_property(k,v)
    for k,v in {'combine_meshes':True,'auto_generate_collision':False,'generate_lightmap_u_vs':True,'convert_scene':True,'convert_scene_unit':True,'force_front_x_axis':False,'transform_vertex_to_absolute':True,'build_nanite':False,'remove_degenerates':True,'import_uniform_scale':1.0,'normal_import_method':unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS}.items():ui.static_mesh_import_data.set_editor_property(k,v)
    task(OUT/'Models/SM_LeafLitter.fbx','SM_LeafLitter',ui)
mesh=unreal.load_asset(DEST+'/SM_LeafLitter');assert isinstance(mesh,unreal.StaticMesh)
if not verify:
    mesh.set_material(0,mat)
    build=sms.get_lod_build_settings(mesh,0)
    build.set_editor_property('use_full_precision_u_vs',True)
    build.set_editor_property('generate_lightmap_u_vs',True)
    sms.set_lod_build_settings(mesh,0,build)
    mesh.set_editor_property('light_map_coordinate_index',1)
    body=mesh.get_editor_property('body_setup')
    instance=body.get_editor_property('default_instance')
    instance.set_editor_property('collision_profile_name','NoCollision')
    body.set_editor_property('default_instance',instance)
    mesh.set_editor_property('has_navigation_data',False)
    save(mesh)
bound=mesh.get_bounds();lo=[bound.origin.x-bound.box_extent.x,bound.origin.y-bound.box_extent.y,bound.origin.z-bound.box_extent.z];hi=[bound.origin.x+bound.box_extent.x,bound.origin.y+bound.box_extent.y,bound.origin.z+bound.box_extent.z]
actual=lo+hi;candidates=[]
for perm in [(0,1,2),(1,0,2)]:
    for sx,sy in itertools.product([-1,1],repeat=2):
        signs=[sx,sy,1];expected=[0]*6
        for i in range(3):
            v=[manifest['bounds_m'][perm[i]]*100*signs[i],manifest['bounds_m'][perm[i]+3]*100*signs[i]]
            expected[i]=min(v);expected[i+3]=max(v)
        candidates.append((max(abs(a-b) for a,b in zip(expected,actual)),perm,signs))
error,perm,signs=min(candidates);assert error<.05
assert abs(lo[2])<.005
assert sms.get_num_uv_channels(mesh,0)>=1
assert sms.get_simple_collision_count(mesh)+sms.get_convex_collision_count(mesh)==0
assert len(mesh.static_materials)==1 and mesh.get_material(0)==mat
assert not mat.get_editor_property('two_sided') and mat.get_editor_property('blend_mode')==unreal.BlendMode.BLEND_OPAQUE
assert unreal.MaterialEditingLibrary.get_material_property_input_node(mat,unreal.MaterialProperty.MP_BASE_COLOR).texture==tex
assert str(mesh.get_editor_property('body_setup').get_editor_property('default_instance').get_editor_property('collision_profile_name'))=='NoCollision'
assert not mesh.get_editor_property('has_navigation_data')
assert not mesh.get_editor_property('nanite_settings').get_editor_property('enabled')
assert sms.get_lod_build_settings(mesh,0).get_editor_property('generate_lightmap_u_vs')
assert mesh.get_editor_property('light_map_coordinate_index')==1
report={'passed':True,'engine':unreal.SystemLibrary.get_engine_version(),'verification':'fresh-process reload' if verify else 'import','host':'content-only UE 5.7 with actual TunaSweeper Content mount','mesh':mesh.get_path_name(),'bounds_cm':actual,'max_bounds_error_cm':error,'axis_mapping':{'indices':perm,'signs':signs},'vertices_lod0':sms.get_number_verts(mesh,0),'source_uv_channels':sms.get_num_uv_channels(mesh,0),'material_slots':1,'opaque':True,'two_sided':False,'collisions':0,'collision_profile':'NoCollision','navigation':False,'texture_srgb':tex.srgb}
(OUT/('unreal_reload_validation.json' if verify else 'unreal_import_validation.json')).write_text(json.dumps(report,indent=2))
unreal.log('LEAFLITTER_UE_PASSED '+json.dumps(report))
