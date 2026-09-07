"""Import/reload only /Game/Nature/ForestProps/GrassLongCurved in UE 5.7.
Set GRASS_LONG_VERIFY_ONLY=1 to check saved assets in a fresh process.
"""
import unreal, json, os, hashlib, itertools
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3]
SOURCE=ROOT/'TunaSweeper/SourceArt/Environment/ForestProps/GrassLongCurved'
DEST='/Game/Nature/ForestProps/GrassLongCurved'
VERIFY=os.environ.get('GRASS_LONG_VERIFY_ONLY')=='1'
manifest=json.loads((SOURCE/'model_manifest.json').read_text())
assets=unreal.AssetToolsHelpers.get_asset_tools()
ed=unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem) or unreal.get_default_object(unreal.StaticMeshEditorSubsystem)
def protected():
    return {str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in (ROOT/'TunaSweeper/Content/Nature').rglob('*.uasset') if 'ForestProps' not in p.parts}
before=protected()
def save(obj):
    assert obj.get_path_name().startswith(DEST+'/')
    assert unreal.EditorAssetLibrary.save_loaded_asset(obj,only_if_is_dirty=False)
def import_task(file,name,options=None):
    t=unreal.AssetImportTask();t.filename=str(file);t.destination_path=DEST;t.destination_name=name;t.automated=True;t.replace_existing=True;t.replace_existing_settings=True;t.save=True
    if options:t.options=options;t.factory=unreal.FbxFactory()
    assets.import_asset_tasks([t])
    result=unreal.load_asset(DEST+'/'+name);assert result
    return result
if not VERIFY:
    texture=import_task(SOURCE/'Textures/T_GrassLongCurved_BaseColor.png','T_GrassLongCurved_BaseColor')
    texture.set_editor_property('srgb',True);texture.set_editor_property('compression_no_alpha',True)
    texture.set_editor_property('lod_group',unreal.TextureGroup.TEXTUREGROUP_WORLD)
    texture.set_editor_property('never_stream',False);save(texture)
    mat=unreal.load_asset(DEST+'/M_GrassLongCurved')
    if not mat:mat=assets.create_asset('M_GrassLongCurved',DEST,unreal.Material,unreal.MaterialFactoryNew())
    unreal.MaterialEditingLibrary.delete_all_material_expressions(mat)
    mat.set_editor_property('two_sided',True);mat.set_editor_property('blend_mode',unreal.BlendMode.BLEND_OPAQUE)
    node=unreal.MaterialEditingLibrary.create_material_expression(mat,unreal.MaterialExpressionTextureSample,-400,0);node.texture=texture
    unreal.MaterialEditingLibrary.connect_material_property(node,'RGB',unreal.MaterialProperty.MP_BASE_COLOR)
    for prop,val,y in [(unreal.MaterialProperty.MP_ROUGHNESS,.88,160),(unreal.MaterialProperty.MP_SPECULAR,.25,260)]:
        node=unreal.MaterialEditingLibrary.create_material_expression(mat,unreal.MaterialExpressionConstant,-250,y);node.r=val
        unreal.MaterialEditingLibrary.connect_material_property(node,'',prop)
    unreal.MaterialEditingLibrary.recompile_material(mat);save(mat)
    options=unreal.FbxImportUI()
    for key,val in {'import_mesh':True,'import_as_skeletal':False,'import_animations':False,'import_materials':False,'import_textures':False,'automated_import_should_detect_type':False,'mesh_type_to_import':unreal.FBXImportType.FBXIT_STATIC_MESH}.items():options.set_editor_property(key,val)
    data=options.static_mesh_import_data
    for key,val in {'combine_meshes':True,'auto_generate_collision':False,'generate_lightmap_u_vs':True,'convert_scene':True,'convert_scene_unit':True,'force_front_x_axis':False,'transform_vertex_to_absolute':True,'build_nanite':False,'remove_degenerates':True,'import_uniform_scale':1.0}.items():data.set_editor_property(key,val)
    data.set_editor_property('normal_import_method',unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS)
    mesh=import_task(SOURCE/'Models/SM_GrassLongCurved.fbx','SM_GrassLongCurved',options)
    mesh.set_material(0,mat)
    for index in [1,2]:
        assert ed.import_lod(mesh,index,str(SOURCE/'Models'/(manifest['lods'][index]['name']+'.fbx')))==index
    ed.remove_collisions(mesh)
    body=mesh.get_editor_property('body_setup');instance=body.get_editor_property('default_instance')
    instance.set_editor_property('collision_profile_name','NoCollision');instance.set_editor_property('collision_enabled',unreal.CollisionEnabled.NO_COLLISION)
    body.set_editor_property('default_instance',instance)
    for index in range(3):
        build=ed.get_lod_build_settings(mesh,index)
        for key,val in {'recompute_normals':False,'recompute_tangents':True,'use_full_precision_u_vs':True,'generate_lightmap_u_vs':True,'src_lightmap_index':0,'dst_lightmap_index':1,'distance_field_resolution_scale':0.0}.items():build.set_editor_property(key,val)
        ed.set_lod_build_settings(mesh,index,build)
        for section in range(mesh.get_num_sections(index)):ed.enable_section_collision(mesh,False,index,section)
    mesh.set_editor_property('light_map_coordinate_index',1)
    assert ed.set_lod_screen_sizes(mesh,[1.0,.20,.075])
    save(mesh)

mesh=unreal.load_asset(DEST+'/SM_GrassLongCurved');mat=unreal.load_asset(DEST+'/M_GrassLongCurved');texture=unreal.load_asset(DEST+'/T_GrassLongCurved_BaseColor')
assert isinstance(mesh,unreal.StaticMesh) and isinstance(mat,unreal.Material) and isinstance(texture,unreal.Texture2D)
assert mat.get_editor_property('two_sided') and mat.get_editor_property('blend_mode')==unreal.BlendMode.BLEND_OPAQUE
node=unreal.MaterialEditingLibrary.get_material_property_input_node(mat,unreal.MaterialProperty.MP_BASE_COLOR)
assert isinstance(node,unreal.MaterialExpressionTextureSample) and node.texture==texture
assert texture.get_editor_property('srgb') and texture.get_editor_property('compression_no_alpha')
assert len(mesh.static_materials)==1 and mesh.static_materials[0].material_interface==mat
assert ed.get_lod_count(mesh)==3
assert ed.get_simple_collision_count(mesh)+ed.get_convex_collision_count(mesh)==0
assert mesh.get_editor_property('body_setup').get_editor_property('default_instance').get_editor_property('collision_enabled')==unreal.CollisionEnabled.NO_COLLISION
report={'engine':unreal.SystemLibrary.get_engine_version(),'mode':'fresh-process reload' if VERIFY else 'import','mesh':mesh.get_path_name(),'material_slots':1,'two_sided':True,'blend_mode':'Opaque','collision':'NoCollision; no shapes; disabled per section','lods':[]}
for index,entry in enumerate(manifest['lods']):
    tri=mesh.get_num_triangles(index);assert tri==entry['triangles'],(index,tri,entry['triangles'])
    assert ed.get_num_uv_channels(mesh,index)>=1
    assert ed.get_lod_build_settings(mesh,index).get_editor_property('generate_lightmap_u_vs')
    assert not ed.is_section_collision_enabled(mesh,index,0)
    report['lods'].append({'lod':index,'triangles':tri,'vertices':ed.get_number_verts(mesh,index),'source_uv_channels':ed.get_num_uv_channels(mesh,index),'generated_lightmap_uv':1,'screen_size':ed.get_lod_screen_sizes(mesh)[index]})
b=mesh.get_bounds();bounds=[b.origin.x-b.box_extent.x,b.origin.y-b.box_extent.y,b.origin.z-b.box_extent.z,b.origin.x+b.box_extent.x,b.origin.y+b.box_extent.y,b.origin.z+b.box_extent.z]
source=manifest['lods'][0]['bounds_m'];candidates=[]
for perm in [(0,1,2),(1,0,2)]:
    for sx,sy in itertools.product([-1,1],repeat=2):
        signs=[sx,sy,1];expected=[0.]*6
        for i in range(3):
            pair=[source[perm[i]]*100*signs[i],source[perm[i]+3]*100*signs[i]]
            expected[i]=min(pair);expected[i+3]=max(pair)
        candidates.append((max(abs(a-b) for a,b in zip(bounds,expected)),perm,signs))
error,perm,signs=min(candidates);assert error<.02,(error,bounds)
assert abs(bounds[2])<.001
report.update(bounds_cm=bounds,dimensions_cm=[bounds[i+3]-bounds[i] for i in range(3)],axis_mapping={'blender_axes_for_ue_xyz':perm,'signs':signs,'max_error_cm':error},existing_nature_unchanged=before==protected(),passed=True)
assert report['existing_nature_unchanged']
(SOURCE/('unreal_reload_validation.json' if VERIFY else 'unreal_import_validation.json')).write_text(json.dumps(report,indent=2),encoding='utf-8')
unreal.log('GRASS_LONG_UE_PASSED')
