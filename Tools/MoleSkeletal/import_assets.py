"""One-off UE 5.7 skeletal/animation import. Does not alter narrative data."""
import unreal as u
from pathlib import Path
src=Path('D:/github/extraction_shooter/TunaSweeper/SourceArt/Characters/Mole')
dest='/Game/Characters/NPC/Mole'
tools=u.AssetToolsHelpers.get_asset_tools()
def run(name,animation=False,skeleton=None):
    task=u.AssetImportTask(); task.filename=str(src/(name+'.fbx')); task.destination_path=dest; task.destination_name=name
    task.automated=True; task.replace_existing=True; task.save=True
    opts=u.FbxImportUI(); opts.set_editor_property('automated_import_should_detect_type',False)
    opts.set_editor_property('import_as_skeletal',True); opts.set_editor_property('import_mesh',not animation)
    opts.set_editor_property('import_animations',animation); opts.set_editor_property('import_materials',False); opts.set_editor_property('import_textures',False)
    opts.set_editor_property('create_physics_asset',False)
    opts.set_editor_property('mesh_type_to_import',u.FBXImportType.FBXIT_ANIMATION if animation else u.FBXImportType.FBXIT_SKELETAL_MESH)
    if skeleton: opts.set_editor_property('skeleton',skeleton)
    data=opts.get_editor_property('anim_sequence_import_data' if animation else 'skeletal_mesh_import_data')
    data.set_editor_property('convert_scene',True); data.set_editor_property('convert_scene_unit',True)
    if animation:
        data.set_editor_property('animation_length',u.FBXAnimationLengthImportType.FBXALIT_EXPORTED_TIME)
        data.set_editor_property('use_default_sample_rate',False); data.set_editor_property('custom_sample_rate',30)
    else:
        data.set_editor_property('normal_import_method',u.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS_AND_TANGENTS)
    task.options=opts; tools.import_asset_tasks([task]); print('IMPORTED',name,list(task.imported_object_paths))
    asset=u.load_asset(dest+'/'+name); assert asset, name
    return asset
skm=run('SKM_MoleDummy'); skeleton=skm.get_editor_property('skeleton')
assert u.EditorAssetLibrary.save_loaded_asset(skeleton,False)
material=u.load_asset(dest+'/M_Mole'); mats=skm.get_editor_property('materials')
for mat in mats: mat.set_editor_property('material_interface',material)
skm.set_editor_property('materials',mats); u.EditorAssetLibrary.save_loaded_asset(skm)
clips={}
for n in ['Idle_Breathe','Turn_InPlace','Walk_InPlace','Walk_Forward','Turn_Left_90','Turn_Right_90']:
    clips[n]=run('A_Mole_'+n,True,skeleton)
    u.MoleAssetSetupLibrary.assign_skeleton(clips[n],skeleton)
    clips[n].set_editor_property('enable_root_motion',False)
    clips[n].set_editor_property('force_root_lock',n in ['Idle_Breathe','Turn_InPlace','Walk_InPlace'])
    u.EditorAssetLibrary.save_loaded_asset(clips[n])
factory=u.BlendSpaceFactory1D(); factory.set_editor_property('target_skeleton',skeleton)
bs=u.load_asset(dest+'/BS_Mole_IdleTurn') or tools.create_asset('BS_Mole_IdleTurn',dest,u.BlendSpace1D,factory)
u.MoleAssetSetupLibrary.assign_skeleton(bs,skeleton)
params=bs.get_editor_property('blend_parameters')
params[0].set_editor_property('display_name','Turn Amount'); params[0].set_editor_property('min',0.0); params[0].set_editor_property('max',1.0)
bs.set_editor_property('blend_parameters',params)
samples=[]
for name,x in [('Idle_Breathe',0.0),('Turn_InPlace',1.0)]:
    s=u.BlendSample(); s.set_editor_property('animation',clips[name]); s.set_editor_property('sample_value',u.Vector(x,0,0)); s.set_editor_property('rate_scale',1.0); samples.append(s)
bs.set_editor_property('sample_data',samples)
bs.set_editor_property('target_weight_interpolation_speed_per_sec',5.0)
bs.set_editor_property('target_weight_interpolation_ease_in_out',True)
u.MoleAssetSetupLibrary.finalize_blend_space(bs)
assert u.EditorAssetLibrary.save_loaded_asset(bs)
print('MOLE_ASSETS_READY',skm.get_path_name(),skeleton.get_path_name(),bs.get_path_name())
