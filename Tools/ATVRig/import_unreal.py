"""One-off UE 5.7 import of the authored ATV skeletal mesh and inspection clip."""
import unreal,json
from pathlib import Path
ROOT=Path('D:/github/extraction_shooter');OUT=ROOT/'TunaSweeper/SourceArt/Vehicles/ATV';DEST='/Game/Meshes/Props/ATV'
manifest=json.loads((OUT/'rig_manifest.json').read_text())
assets=unreal.AssetToolsHelpers.get_asset_tools()
def import_fbx(filename,name,animation=False,skeleton=None):
 ui=unreal.FbxImportUI()
 for key,value in {'import_mesh':not animation,'import_as_skeletal':True,'import_animations':animation,'import_materials':False,'import_textures':False,'create_physics_asset':False,'automated_import_should_detect_type':False,'mesh_type_to_import':unreal.FBXImportType.FBXIT_ANIMATION if animation else unreal.FBXImportType.FBXIT_SKELETAL_MESH}.items():ui.set_editor_property(key,value)
 if skeleton:ui.skeleton=skeleton
 data=ui.anim_sequence_import_data if animation else ui.skeletal_mesh_import_data
 for key,value in {'convert_scene':True,'convert_scene_unit':True,'force_front_x_axis':False,'import_uniform_scale':1.0}.items():data.set_editor_property(key,value)
 if animation:
  data.set_editor_property('animation_length',unreal.FBXAnimationLengthImportType.FBXALIT_EXPORTED_TIME)
  data.set_editor_property('use_default_sample_rate',False)
  data.set_editor_property('custom_sample_rate',30)
 else:
  data.set_editor_property('normal_import_method',unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS)
  data.set_editor_property('import_meshes_in_bone_hierarchy',True)
  data.set_editor_property('use_t0_as_ref_pose',False)
 task=unreal.AssetImportTask();task.filename=str(OUT/filename);task.destination_path=DEST;task.destination_name=name;task.automated=True;task.replace_existing=True;task.save=True;task.factory=unreal.FbxFactory();task.options=ui
 assets.import_asset_tasks([task])
 unreal.log('ATV_IMPORTED '+str(task.imported_object_paths))
 return task.imported_object_paths

import_fbx('SKM_ATV.fbx','SKM_ATV')
mesh=unreal.load_asset(DEST+'/SKM_ATV');assert isinstance(mesh,unreal.SkeletalMesh)
skeleton=mesh.get_editor_property('skeleton');assert skeleton
for desc in manifest['materials']:
 name=desc['name'];path=DEST+'/'+name
 mat=unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else None
 if not mat:mat=assets.create_asset(name,DEST,unreal.Material,unreal.MaterialFactoryNew())
 unreal.MaterialEditingLibrary.delete_all_material_expressions(mat)
 color=unreal.MaterialEditingLibrary.create_material_expression(mat,unreal.MaterialExpressionConstant3Vector,-400,0)
 color.set_editor_property('constant',unreal.LinearColor(*desc['base_color']))
 unreal.MaterialEditingLibrary.connect_material_property(color,'',unreal.MaterialProperty.MP_BASE_COLOR)
 for i,(prop,value) in enumerate([(unreal.MaterialProperty.MP_METALLIC,desc['metallic']),(unreal.MaterialProperty.MP_ROUGHNESS,desc['roughness'])]):
  expr=unreal.MaterialEditingLibrary.create_material_expression(mat,unreal.MaterialExpressionConstant,-400,150+i*100);expr.set_editor_property('r',value);unreal.MaterialEditingLibrary.connect_material_property(expr,'',prop)
 unreal.MaterialEditingLibrary.recompile_material(mat);unreal.EditorAssetLibrary.save_loaded_asset(mat)
slots=mesh.get_editor_property('materials')
for index,slot in enumerate(slots):
 name=str(slot.get_editor_property('imported_material_slot_name'))
 material=unreal.load_asset(DEST+'/'+name)
 assert material,(name,'missing material')
 slot.set_editor_property('material_interface',material)
 slots[index]=slot
mesh.set_editor_property('materials',slots)
unreal.EditorAssetLibrary.save_loaded_asset(mesh,only_if_is_dirty=False);unreal.EditorAssetLibrary.save_loaded_asset(skeleton,only_if_is_dirty=False)
import_fbx('ATV_RigCheck.fbx','AN_ATV_RigCheck',True,skeleton)
report={'skeletal_mesh':mesh.get_path_name(),'skeleton':skeleton.get_path_name(),'materials':[str(s.material_interface.get_path_name()) for s in slots]}
(OUT/'unreal_import.json').write_text(json.dumps(report,indent=2))
unreal.log('ATV_IMPORT_COMPLETE '+json.dumps(report))
