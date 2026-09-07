"""UE 5.7 asset import / fresh-process reload. Saves only GrassSparse assets."""
import unreal, json, os, hashlib
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3];OUT=ROOT/'TunaSweeper/SourceArt/Environment/ForestProps/GrassSparse'
DEST='/Game/Nature/ForestProps/GrassSparse'
m=json.loads((OUT/'model_manifest.json').read_text());verify=os.environ.get('GRASS_SPARSE_VERIFY_ONLY')=='1'
at=unreal.AssetToolsHelpers.get_asset_tools();ed=unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem) or unreal.get_default_object(unreal.StaticMeshEditorSubsystem)
def hashes():
    return {str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for folder in ['Bush','GrassLow','Flower','SimpleTree','Wood','RockBasic'] for p in (ROOT/'TunaSweeper/Content/Nature'/folder).glob('*.uasset')}
before=hashes()
def save(a):
    assert a.get_path_name().startswith(DEST+'/')
    assert unreal.EditorAssetLibrary.save_loaded_asset(a,only_if_is_dirty=False)
def task(filename,name,options=None):
    t=unreal.AssetImportTask();t.filename=str(filename);t.destination_path=DEST;t.destination_name=name;t.automated=True;t.replace_existing=True;t.save=True
    if options:t.options=options;t.factory=unreal.FbxFactory()
    at.import_asset_tasks([t]);assert t.imported_object_paths
if not verify:task(OUT/'Textures'/(m['texture']+'.png'),m['texture'])
texture=unreal.load_asset(DEST+'/'+m['texture']);assert isinstance(texture,unreal.Texture2D)
if not verify:
    texture.set_editor_property('srgb',True);texture.set_editor_property('lod_group',unreal.TextureGroup.TEXTUREGROUP_WORLD);save(texture)
name=m['materials'][0];mat=unreal.load_asset(DEST+'/'+name)
if not verify:
    if mat is None:mat=at.create_asset(name,DEST,unreal.Material,unreal.MaterialFactoryNew())
    unreal.MaterialEditingLibrary.delete_all_material_expressions(mat)
    mat.set_editor_property('two_sided',True);mat.set_editor_property('blend_mode',unreal.BlendMode.BLEND_OPAQUE)
    node=unreal.MaterialEditingLibrary.create_material_expression(mat,unreal.MaterialExpressionTextureSample,-400,0);node.texture=texture
    unreal.MaterialEditingLibrary.connect_material_property(node,'RGB',unreal.MaterialProperty.MP_BASE_COLOR)
    for i,(prop,value) in enumerate([(unreal.MaterialProperty.MP_ROUGHNESS,.87),(unreal.MaterialProperty.MP_SPECULAR,.18)]):
        n=unreal.MaterialEditingLibrary.create_material_expression(mat,unreal.MaterialExpressionConstant,-400,160+i*100);n.r=value;unreal.MaterialEditingLibrary.connect_material_property(n,'',prop)
    unreal.MaterialEditingLibrary.recompile_material(mat);save(mat)
if not verify:
    opt=unreal.FbxImportUI();opt.import_mesh=True;opt.import_as_skeletal=False;opt.import_materials=False;opt.import_textures=False;opt.import_animations=False;opt.automated_import_should_detect_type=False;opt.mesh_type_to_import=unreal.FBXImportType.FBXIT_STATIC_MESH
    data=opt.static_mesh_import_data
    for k,v in {'combine_meshes':True,'auto_generate_collision':False,'generate_lightmap_u_vs':True,'convert_scene':True,'convert_scene_unit':True,'force_front_x_axis':False,'transform_vertex_to_absolute':True,'build_nanite':False,'remove_degenerates':True,'import_uniform_scale':1.0}.items():data.set_editor_property(k,v)
    data.normal_import_method=unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS
    task(OUT/'Models'/(m['name']+'.fbx'),m['name'],opt)
mesh=unreal.load_asset(DEST+'/'+m['name']);assert isinstance(mesh,unreal.StaticMesh)
if not verify:
    mesh.set_material(0,mat)
    body=mesh.get_editor_property('body_setup');instance=body.get_editor_property('default_instance')
    instance.set_editor_property('collision_profile_name','NoCollision');instance.set_editor_property('collision_enabled',unreal.CollisionEnabled.NO_COLLISION)
    body.set_editor_property('default_instance',instance)
    mesh.set_editor_property('light_map_coordinate_index',1)
    # Decorative blades never participate in navigation or distance-field occlusion.
    mesh.set_editor_property('has_navigation_data',False)
    save(mesh)
assert mat.get_editor_property('two_sided') and mat.get_editor_property('blend_mode')==unreal.BlendMode.BLEND_OPAQUE
color=unreal.MaterialEditingLibrary.get_material_property_input_node(mat,unreal.MaterialProperty.MP_BASE_COLOR);assert color.texture==texture
assert len(mesh.get_editor_property('static_materials'))==1 and mesh.get_editor_property('static_materials')[0].material_interface==mat
assert ed.get_num_uv_channels(mesh,0)>=1
assert ed.get_simple_collision_count(mesh)==0 and ed.get_convex_collision_count(mesh)==0
body=mesh.get_editor_property('body_setup');instance=body.get_editor_property('default_instance')
assert str(instance.get_editor_property('collision_profile_name'))=='NoCollision'
assert instance.get_editor_property('collision_enabled')==unreal.CollisionEnabled.NO_COLLISION
assert mesh.get_num_triangles(0)==m['triangles']
assert not mesh.get_editor_property('has_navigation_data')
b=mesh.get_bounds();actual=[b.origin.x-b.box_extent.x,b.origin.y-b.box_extent.y,b.origin.z-b.box_extent.z,b.origin.x+b.box_extent.x,b.origin.y+b.box_extent.y,b.origin.z+b.box_extent.z]
# Standard Blender FBX -Y/Z maps (x,y,z) metres to UE (x,-y,z) centimetres.
s=m['bounds_m'];expected=[s[0]*100,-s[4]*100,s[2]*100,s[3]*100,-s[1]*100,s[5]*100]
error=max(abs(a-b) for a,b in zip(actual,expected));assert error<.1,(actual,expected)
assert abs(actual[2])<.001
after=hashes();assert before==after
report={'passed':True,'engine':unreal.SystemLibrary.get_engine_version(),'mode':'fresh-process reload' if verify else 'import','asset':mesh.get_path_name(),'bounds_cm':actual,'max_bounds_error_cm':error,'axis_mapping':'UE cm = (Blender X, -Blender Y, Blender Z) * 100','source_uv_channels':ed.get_num_uv_channels(mesh,0),'vertices_lod0':ed.get_number_verts(mesh,0),'triangles_lod0':mesh.get_num_triangles(0),'material_slots':len(mesh.get_editor_property('static_materials')),'opaque':True,'two_sided':True,'collision_profile':'NoCollision','collision_shapes':0,'navigation':False,'existing_assets_unchanged':before==after,'protected_asset_hashes':after}
(OUT/('unreal_reload_validation.json' if verify else 'unreal_import_validation.json')).write_text(json.dumps(report,indent=2),encoding='utf-8')
unreal.log('GRASS_SPARSE_UE_PASS')
