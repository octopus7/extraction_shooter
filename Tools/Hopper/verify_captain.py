"""Read-only UE audit of the saved Captain pilot, animations and 512px textures.

Run with -EnablePlugins=PythonScriptPlugin,EditorScriptingUtilities
-ExecutePythonScript=<this file>. Writes its report under Saved/Hopper only.
"""
import json, math, struct, traceback
from pathlib import Path
import unreal

ROOT=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
SOURCE=ROOT/'SourceArt/Characters/Hopper/Captain'
DEST='/Game/Characters/Hopper/Captain'
REPORT=ROOT/'Saved/Hopper/captain_verification.json'
result={'passed':False}
def load(path,cls):
 a=unreal.load_asset(path);assert isinstance(a,cls),path;return a
def vector(v):return [v.x,v.y,v.z]
def pose(clip,t):return unreal.AnimPoseExtensions.get_anim_pose_at_time(clip,t,unreal.AnimPoseEvaluationOptions())
def bone(p,name):return unreal.AnimPoseExtensions.get_bone_pose(p,name,unreal.AnimPoseSpaces.WORLD)
try:
 da=load('/Game/Characters/Hopper/DA_HopperVisual',unreal.HopperVisualData)
 mesh=load(DEST+'/SK_Captain_Pilot',unreal.SkeletalMesh)
 assert da.get_editor_property('pilot_mesh')==mesh,'Hopper must use the Captain mesh'
 skeleton=mesh.get_editor_property('skeleton')
 reference=unreal.AnimPoseExtensions.get_reference_pose(skeleton)
 names=[str(n) for n in unreal.AnimPoseExtensions.get_bone_names(reference)]
 expected=json.loads((SOURCE/'RigVerification.json').read_text(encoding='utf-8'))
 assert set(names)==set(expected['bones']),names
 clips={};summaries={}
 for name,duration in expected['clips'].items():
  clip=load(DEST+'/Animations/A_Captain_'+name,unreal.AnimSequence)
  assert da.get_editor_property('pilot_'+name.lower())==clip,name+' is disconnected'
  assert clip.get_editor_property('skeleton')==skeleton,name+' skeleton mismatch'
  length=clip.get_editor_property('sequence_length');assert abs(length-duration)<.002,(name,length)
  sampled=[]
  for f in range(11):
   p=pose(clip,length*f/10)
   for n in names:
    transform=bone(p,n)
    assert all(math.isfinite(v) for v in vector(transform.translation)+vector(transform.scale3d)),(name,n)
    assert max(abs(v-1) for v in vector(transform.scale3d))<.005,(name,n,'non-unit scale')
   assert max(abs(v) for v in vector(bone(p,'root').translation))<.01,(name,'root drift')
   assert bone(p,'root').rotation.rotate_vector(unreal.Vector(1,0,0)).x>.999,(name,'root direction differs from mesh')
   sampled.append({n:vector(bone(p,n).translation) for n in ['pelvis','hand_r','foot_l','foot_r']})
  clips[name]=clip;summaries[name]={'seconds':length,'samples':sampled}
 idle=pose(clips['Idle'],0);seated=pose(clips['Seated'],0)
 assert bone(idle,'pelvis').translation.z-bone(seated,'pelvis').translation.z>8,'Seated pelvis was not lowered'
 walk=summaries['Walk']['samples']
 assert max(p['foot_l'][0] for p in walk)-min(p['foot_l'][0] for p in walk)>4,'Left foot does not stride'
 assert max(p['foot_r'][0] for p in walk)-min(p['foot_r'][0] for p in walk)>4,'Right foot does not stride'
 gun=da.get_editor_property('gun_transform')
 hand=bone(idle,'hand_r')
 direction=hand.rotation.rotate_vector(gun.rotation.rotate_vector(unreal.Vector(1,0,0)))
 assert direction.x>.999,'Gun must aim along UE +X in idle aim pose'
 textures=[]
 for spec in expected['textures']:
  path=SOURCE/'Textures'/(spec['name']+'.png')
  assert struct.unpack('>II',path.read_bytes()[16:24])==(512,512),'PNG source must be downsampled'
  tex=load(DEST+'/Textures/'+spec['name'],unreal.Texture2D)
  assert (tex.blueprint_get_size_x(),tex.blueprint_get_size_y())==(512,512),'UE texture must be 512'
  assert tex.get_editor_property('max_texture_size')==512
  kind=spec['kind'];assert tex.get_editor_property('srgb')==(kind=='BaseColor')
  if kind=='Normal':
   assert tex.get_editor_property('compression_settings')==unreal.TextureCompressionSettings.TC_NORMALMAP
   assert tex.get_editor_property('flip_green_channel')
  textures.append({'path':tex.get_path_name(),'size':[512,512],'srgb':tex.get_editor_property('srgb')})
 materials=mesh.get_editor_property('materials');assert len(materials)==2
 for part,slot in zip(['Body','Outfit'],materials):
  mat=slot.material_interface;assert mat.get_path_name()==f'{DEST}/Materials/M_Captain_{part}.M_Captain_{part}'
  assert mat.get_editor_property('used_with_skeletal_mesh')
  for prop,kind in [(unreal.MaterialProperty.MP_BASE_COLOR,'BaseColor'),(unreal.MaterialProperty.MP_NORMAL,'Normal'),(unreal.MaterialProperty.MP_METALLIC,'MR'),(unreal.MaterialProperty.MP_ROUGHNESS,'MR')]:
   node=unreal.MaterialEditingLibrary.get_material_property_input_node(mat,prop)
   assert node and node.texture.get_name()==f'T_Captain_{part}_{kind}_512'
 result.update({'passed':True,'bones':names,'mesh':mesh.get_path_name(),'textures':textures,'clips':summaries,'gun_forward':vector(direction)})
except Exception:
 result['error']=traceback.format_exc();unreal.log_error(result['error'])
REPORT.parent.mkdir(parents=True,exist_ok=True);REPORT.write_text(json.dumps(result,indent=2),encoding='utf-8')
print('CAPTAIN_VERIFICATION',json.dumps({'passed':result['passed'],'error':result.get('error')}))
