"""Fresh-process validation of the saved backpack assets; never imports or saves."""
import unreal,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];OUT=ROOT/'TunaSweeper/SourceArt/Props/Backpacks';DEST='/Game/Props/Backpacks'
ed=unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem) or unreal.get_default_object(unreal.StaticMeshEditorSubsystem)
report={'passed':False,'engine':unreal.SystemLibrary.get_engine_version(),'icons':[],'models':[]}
source=json.loads((OUT/'source_validation.json').read_text());assert source['passed']
for i in range(1,6):
 a=unreal.load_asset(f'/Game/UI/Icons/T_UIIcon_Backpack_Tier{i}');assert isinstance(a,unreal.Texture2D)
 assert a.blueprint_get_size_x()==256 and a.blueprint_get_size_y()==256
 assert a.get_editor_property('lod_group')==unreal.TextureGroup.TEXTUREGROUP_UI
 assert a.get_editor_property('mip_gen_settings')==unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS
 assert a.get_editor_property('srgb')
 report['icons'].append(a.get_path_name())
textures=[]
for expected in source['models']:
 name=expected['mesh'].removeprefix('SM_Backpack_');mesh=unreal.load_asset(DEST+'/Meshes/'+expected['mesh']);assert isinstance(mesh,unreal.StaticMesh)
 assert mesh.get_num_triangles(0)==expected['triangles']<500
 assert ed.get_num_uv_channels(mesh,0)==2 and len(mesh.static_materials)==1
 mat=mesh.static_materials[0].material_interface;node=unreal.MaterialEditingLibrary.get_material_property_input_node(mat,unreal.MaterialProperty.MP_BASE_COLOR);tex=node.texture
 assert tex.get_name()==Path(expected['texture']).stem
 assert tex.blueprint_get_size_x()==512 and tex.blueprint_get_size_y()==512
 textures.append(tex.get_path_name());bounds=mesh.get_bounds();dims=[bounds.box_extent.x*2,bounds.box_extent.y*2,bounds.box_extent.z*2]
 assert max(abs(a-b) for a,b in zip(dims,expected['dimensions_cm']))<.03,(name,dims,expected['dimensions_cm'])
 report['models'].append({'mesh':mesh.get_path_name(),'triangles':mesh.get_num_triangles(0),'uv_channels':2,'material':mat.get_path_name(),'texture':tex.get_path_name(),'dimensions_cm':dims})
assert len(set(textures))==5
report['passed']=True
(OUT/'unreal_validation.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
unreal.log('BACKPACK_ASSET_VALIDATION_PASS '+json.dumps(report))
