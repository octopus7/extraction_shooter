"""UE5.7 isolated-folder import and fresh-process reload verification."""
import unreal, json, os, itertools
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3]
OUT=ROOT/'TunaSweeper/SourceArt/Environment/ForestProps/ExposedRoots'
DEST='/Game/Nature/ForestProps/ExposedRoots'
VERIFY=os.environ.get('EXPOSED_ROOTS_VERIFY')=='1'
tools=unreal.AssetToolsHelpers.get_asset_tools();sub=unreal.get_default_object(unreal.StaticMeshEditorSubsystem)
def save(asset):
    assert asset.get_path_name().startswith(DEST+'/')
    assert unreal.EditorAssetLibrary.save_loaded_asset(asset,only_if_is_dirty=False)
if not VERIFY:
    t=unreal.AssetImportTask();t.filename=str(OUT/'Textures/T_ExposedRoots_Palette.png');t.destination_path=DEST;t.destination_name='T_ExposedRoots_Palette';t.automated=True;t.replace_existing=True;t.save=True
    tools.import_asset_tasks([t])
texture=unreal.load_asset(DEST+'/T_ExposedRoots_Palette');assert texture
if not VERIFY:
    texture.set_editor_property('srgb',True);texture.set_editor_property('lod_group',unreal.TextureGroup.TEXTUREGROUP_WORLD);save(texture)
material=unreal.load_asset(DEST+'/M_ExposedRoots')
if not VERIFY:
    if not material:material=tools.create_asset('M_ExposedRoots',DEST,unreal.Material,unreal.MaterialFactoryNew())
    unreal.MaterialEditingLibrary.delete_all_material_expressions(material)
    material.set_editor_property('two_sided',False);material.set_editor_property('blend_mode',unreal.BlendMode.BLEND_OPAQUE)
    tex=unreal.MaterialEditingLibrary.create_material_expression(material,unreal.MaterialExpressionTextureSample,-300,0);tex.texture=texture
    unreal.MaterialEditingLibrary.connect_material_property(tex,'RGB',unreal.MaterialProperty.MP_BASE_COLOR)
    for prop,value,y in [(unreal.MaterialProperty.MP_ROUGHNESS,.9,200),(unreal.MaterialProperty.MP_METALLIC,0,300)]:
        node=unreal.MaterialEditingLibrary.create_material_expression(material,unreal.MaterialExpressionConstant,-200,y);node.r=value
        unreal.MaterialEditingLibrary.connect_material_property(node,'',prop)
    unreal.MaterialEditingLibrary.recompile_material(material);save(material)
    opt=unreal.FbxImportUI()
    for key,value in {'import_mesh':True,'import_as_skeletal':False,'import_animations':False,'import_materials':False,'import_textures':False,'automated_import_should_detect_type':False,'mesh_type_to_import':unreal.FBXImportType.FBXIT_STATIC_MESH}.items():opt.set_editor_property(key,value)
    data=opt.static_mesh_import_data
    for key,value in {'combine_meshes':True,'auto_generate_collision':False,'generate_lightmap_u_vs':False,'convert_scene':True,'convert_scene_unit':True,'force_front_x_axis':False,'transform_vertex_to_absolute':True,'build_nanite':False,'import_uniform_scale':1.0,'normal_import_method':unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS}.items():data.set_editor_property(key,value)
    task=unreal.AssetImportTask();task.filename=str(OUT/'Models/SM_ExposedRoots.fbx');task.destination_path=DEST;task.destination_name='SM_ExposedRoots';task.automated=True;task.replace_existing=True;task.save=True;task.options=opt;task.factory=unreal.FbxFactory()
    tools.import_asset_tasks([task])
    mesh=unreal.load_asset(DEST+'/SM_ExposedRoots');assert mesh
    mesh.set_material(0,material)
    build=sub.get_lod_build_settings(mesh,0)
    for key,value in {'recompute_normals':False,'recompute_tangents':True,'generate_lightmap_u_vs':False,'use_full_precision_u_vs':True}.items():build.set_editor_property(key,value)
    sub.set_lod_build_settings(mesh,0,build)
    mesh.set_editor_property('light_map_coordinate_index',1);mesh.set_editor_property('light_map_resolution',64)
    body=mesh.get_editor_property('body_setup');instance=body.get_editor_property('default_instance');instance.set_editor_property('collision_profile_name','NoCollision');body.set_editor_property('default_instance',instance)
    save(mesh)
mesh=unreal.load_asset(DEST+'/SM_ExposedRoots');assert mesh and material and texture
b=mesh.get_bounds();center=[b.origin.x,b.origin.y,b.origin.z];ext=[b.box_extent.x,b.box_extent.y,b.box_extent.z]
actual=[center[i]-ext[i] for i in range(3)]+[center[i]+ext[i] for i in range(3)]
source=json.loads((OUT/'blender_validation.json').read_text())['bounds_m']
candidates=[]
for perm in [(0,1,2),(1,0,2)]:
    for sx,sy in itertools.product((-1,1),repeat=2):
        signs=[sx,sy,1];expected=[0]*6
        for i in range(3):
            values=[source[perm[i]]*100*signs[i],source[perm[i]+3]*100*signs[i]];expected[i]=min(values);expected[i+3]=max(values)
        candidates.append((max(abs(a-b) for a,b in zip(actual,expected)),perm,signs))
error,perm,signs=min(candidates);assert error<.01,(error,actual,source)
uv=sub.get_num_uv_channels(mesh,0);assert uv==2
collisions=sub.get_simple_collision_count(mesh)+sub.get_convex_collision_count(mesh);assert collisions==0
assert len(mesh.static_materials)==1 and mesh.get_material(0)==material
assert material.get_editor_property('blend_mode')==unreal.BlendMode.BLEND_OPAQUE and not material.get_editor_property('two_sided')
assert unreal.MaterialEditingLibrary.get_material_property_input_node(material,unreal.MaterialProperty.MP_BASE_COLOR).texture==texture
profile=str(mesh.get_editor_property('body_setup').get_editor_property('default_instance').get_editor_property('collision_profile_name'));assert profile=='NoCollision',profile
report={'passed':True,'mode':'fresh-process reload' if VERIFY else 'import','engine':unreal.SystemLibrary.get_engine_version(),'path':mesh.get_path_name(),'bounds_cm':actual,'dimensions_cm':[2*v for v in ext],'axis_mapping':{'blender_indices':perm,'signs':signs,'max_error_cm':error},'uv_channels':uv,'simple_collisions':collisions,'collision_profile':profile,'material_slots':1,'opaque':True,'two_sided':False,'vertices':sub.get_number_verts(mesh,0)}
(OUT/('unreal_reload_validation.json' if VERIFY else 'unreal_import_validation.json')).write_text(json.dumps(report,indent=2),encoding='utf-8')
unreal.log('EXPOSED_ROOTS_VALIDATION_PASSED')
