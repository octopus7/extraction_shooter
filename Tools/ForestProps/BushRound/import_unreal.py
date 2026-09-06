"""UE 5.7 isolated-folder import; BUSHROUND_VERIFY_ONLY=1 reloads without saves."""
import unreal,json,os,itertools
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3]
OUT=ROOT/'TunaSweeper/SourceArt/Environment/ForestProps/BushRound'
DEST='/Game/Nature/ForestProps/BushRound'
manifest=json.loads((OUT/'model_manifest.json').read_text())
verify=os.environ.get('BUSHROUND_VERIFY_ONLY')=='1'
tools=unreal.AssetToolsHelpers.get_asset_tools()
ed=unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem) or unreal.get_default_object(unreal.StaticMeshEditorSubsystem)
def save(a):
    assert a.get_path_name().startswith(DEST+'/')
    assert unreal.EditorAssetLibrary.save_loaded_asset(a,only_if_is_dirty=False)
def task(file,name,options=None):
    t=unreal.AssetImportTask();t.filename=str(file);t.destination_path=DEST;t.destination_name=name;t.automated=True;t.replace_existing=True;t.save=True
    if options:t.options=options;t.factory=unreal.FbxFactory()
    tools.import_asset_tasks([t]);assert t.imported_object_paths
if not verify:
    task(OUT/'Textures/T_BushRound_Palette.png','T_BushRound_Palette')
tex=unreal.load_asset(DEST+'/T_BushRound_Palette');assert isinstance(tex,unreal.Texture2D)
if not verify:
    tex.set_editor_property('srgb',True);tex.set_editor_property('lod_group',unreal.TextureGroup.TEXTUREGROUP_WORLD);save(tex)
mat=unreal.load_asset(DEST+'/M_BushRound')
if not verify:
    if not mat:mat=tools.create_asset('M_BushRound',DEST,unreal.Material,unreal.MaterialFactoryNew())
    mat.set_editor_property('two_sided',False);mat.set_editor_property('blend_mode',unreal.BlendMode.BLEND_OPAQUE)
    unreal.MaterialEditingLibrary.delete_all_material_expressions(mat)
    n=unreal.MaterialEditingLibrary.create_material_expression(mat,unreal.MaterialExpressionTextureSample,-350,0);n.texture=tex
    unreal.MaterialEditingLibrary.connect_material_property(n,'RGB',unreal.MaterialProperty.MP_BASE_COLOR)
    for prop,val,y in [(unreal.MaterialProperty.MP_ROUGHNESS,.86,180),(unreal.MaterialProperty.MP_SPECULAR,.18,280)]:
        n=unreal.MaterialEditingLibrary.create_material_expression(mat,unreal.MaterialExpressionConstant,-350,y);n.r=val
        unreal.MaterialEditingLibrary.connect_material_property(n,'',prop)
    unreal.MaterialEditingLibrary.recompile_material(mat);save(mat)
    opt=unreal.FbxImportUI()
    for k,v in {'import_mesh':True,'import_as_skeletal':False,'import_animations':False,'import_materials':False,'import_textures':False,'automated_import_should_detect_type':False,'mesh_type_to_import':unreal.FBXImportType.FBXIT_STATIC_MESH}.items():opt.set_editor_property(k,v)
    data=opt.static_mesh_import_data
    for k,v in {'combine_meshes':True,'auto_generate_collision':False,'generate_lightmap_u_vs':False,'convert_scene':True,'convert_scene_unit':True,'force_front_x_axis':False,'transform_vertex_to_absolute':True,'build_nanite':False,'remove_degenerates':True,'import_uniform_scale':1.0,'normal_import_method':unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS}.items():data.set_editor_property(k,v)
    task(OUT/'Models/SM_BushRound.fbx','SM_BushRound',opt)
mesh=unreal.load_asset(DEST+'/SM_BushRound');assert isinstance(mesh,unreal.StaticMesh)
if not verify:
    mesh.set_material(0,mat)
    build=ed.get_lod_build_settings(mesh,0)
    for k,v in {'recompute_normals':False,'recompute_tangents':True,'generate_lightmap_u_vs':False,'use_full_precision_u_vs':True}.items():build.set_editor_property(k,v)
    ed.set_lod_build_settings(mesh,0,build)
    mesh.set_editor_property('light_map_coordinate_index',1);mesh.set_editor_property('light_map_resolution',64)
    body=mesh.get_editor_property('body_setup');inst=body.get_editor_property('default_instance');inst.set_editor_property('collision_profile_name','NoCollision');body.set_editor_property('default_instance',inst)
    save(mesh)
b=mesh.get_bounds();actual=[b.origin.x-b.box_extent.x,b.origin.y-b.box_extent.y,b.origin.z-b.box_extent.z,b.origin.x+b.box_extent.x,b.origin.y+b.box_extent.y,b.origin.z+b.box_extent.z]
candidates=[]
for perm in [(0,1,2),(1,0,2)]:
    for sx,sy in itertools.product([-1,1],repeat=2):
        signs=(sx,sy,1);expected=[0]*6
        for i in range(3):
            vals=[manifest['bounds_m'][perm[i]+j]*100*signs[i] for j in (0,3)]
            expected[i]=min(vals);expected[i+3]=max(vals)
        candidates.append((max(abs(a-b) for a,b in zip(actual,expected)),perm,signs))
error,perm,signs=min(candidates);assert error<.02,(error,actual)
slots=list(mesh.static_materials)
report={'engine':unreal.SystemLibrary.get_engine_version(),'mode':'fresh-process reload' if verify else 'import','mesh':mesh.get_path_name(),'bounds_cm':actual,'dimensions_cm':[actual[i+3]-actual[i] for i in range(3)],'axis_mapping':{'blender_axis_for_ue_xyz':perm,'signs':signs,'max_error_cm':error},'source_uv_channels':ed.get_num_uv_channels(mesh,0),'vertices':ed.get_number_verts(mesh,0),'material_slots':len(slots),'simple_collisions':ed.get_simple_collision_count(mesh),'convex_collisions':ed.get_convex_collision_count(mesh),'collision_profile':str(mesh.get_editor_property('body_setup').get_editor_property('default_instance').get_editor_property('collision_profile_name')),'opaque':mat.get_editor_property('blend_mode')==unreal.BlendMode.BLEND_OPAQUE,'two_sided':mat.get_editor_property('two_sided'),'texture_srgb':tex.get_editor_property('srgb')}
assert report['source_uv_channels']==2 and report['material_slots']==1
assert report['simple_collisions']==report['convex_collisions']==0 and report['collision_profile']=='NoCollision'
assert report['opaque'] and not report['two_sided'] and report['texture_srgb']
assert slots[0].material_interface==mat
n=unreal.MaterialEditingLibrary.get_material_property_input_node(mat,unreal.MaterialProperty.MP_BASE_COLOR);assert n.texture==tex
assert abs(actual[2])<.02
report['passed']=True
(OUT/('unreal_reload_validation.json' if verify else 'unreal_import_validation.json')).write_text(json.dumps(report,indent=2),encoding='utf-8')
unreal.log('BUSHROUND_UE_PASS '+json.dumps(report))
