"""Read-only fresh-process validation of the imported ATV mesh and animation."""
import unreal,json,math
from pathlib import Path
ROOT=Path('D:/github/extraction_shooter');OUT=ROOT/'TunaSweeper/SourceArt/Vehicles/ATV';DEST='/Game/Meshes/Props/ATV'
manifest=json.loads((OUT/'rig_manifest.json').read_text())
mesh=unreal.load_asset(DEST+'/SKM_ATV');assert isinstance(mesh,unreal.SkeletalMesh)
skeleton=mesh.get_editor_property('skeleton');assert isinstance(skeleton,unreal.Skeleton)
ref=unreal.AnimPoseExtensions.get_reference_pose(skeleton)
names=[str(n) for n in unreal.AnimPoseExtensions.get_bone_names(ref)]
assert set(names)=={b['name'] for b in manifest['bones']},names
poses={}
for bone in ['root']+['wheel_'+k for k in manifest['wheel_order']]:
 pose=unreal.AnimPoseExtensions.get_bone_pose(ref,bone,unreal.AnimPoseSpaces.WORLD)
 loc=pose.translation;rotation=pose.rotation;scale=pose.scale3d
 expected=next(b['head_cm'] for b in manifest['bones'] if b['name']==bone);expected=[expected[0],-expected[1],expected[2]]
 assert max(abs(a-b) for a,b in zip(loc.to_tuple(),expected))<.05,(bone,loc,expected)
 assert max(abs(v-1) for v in scale.to_tuple())<1e-4,(bone,scale)
 forward=rotation.rotate_vector(unreal.Vector(1,0,0));up=rotation.rotate_vector(unreal.Vector(0,0,1))
 assert forward.x>.999 and up.z>.999,(bone,rotation,forward,up)
 poses[bone]={'location_cm':list(loc.to_tuple()),'scale':list(scale.to_tuple()),'rotation_xyzw':[rotation.x,rotation.y,rotation.z,rotation.w]}
anim=unreal.load_asset(DEST+'/AN_ATV_RigCheck');assert isinstance(anim,unreal.AnimSequence)
options=unreal.AnimPoseEvaluationOptions()
animated=unreal.AnimPoseExtensions.get_anim_pose_at_time(anim,1.0,options)
travel={}
steering_axes={}
shock_scales={}
for key in manifest['wheel_order']:
 p=unreal.AnimPoseExtensions.get_bone_pose(animated,'wheel_'+key,unreal.AnimPoseSpaces.WORLD)
 travel[key]=p.translation.z-poses['wheel_'+key]['location_cm'][2]
 assert 6.5<abs(travel[key])<7.5,(key,travel[key])
 axle=p.rotation.rotate_vector(unreal.Vector(0,1,0))
 steering_axes[key]=list(axle.to_tuple())
 assert (.38<abs(axle.x)<.44) if key.startswith('F') else abs(axle.x)<.001,(key,axle)
 for prefix in ['shock_upper_','shock_lower_']:
  shock=unreal.AnimPoseExtensions.get_bone_pose(animated,prefix+key,unreal.AnimPoseSpaces.WORLD)
  assert max(abs(v-1) for v in shock.scale3d.to_tuple())<.001,(prefix,key,shock.scale3d)
 spring=unreal.AnimPoseExtensions.get_bone_pose(animated,'spring_'+key,unreal.AnimPoseSpaces.WORLD)
 shock_scales[key]=list(spring.scale3d.to_tuple())
 assert max(abs(v-1) for v in spring.scale3d.to_tuple())>.08
materials=[s.material_interface.get_path_name() if s.material_interface else None for s in mesh.get_editor_property('materials')]
assert all(materials)
report={'passed':True,'engine':unreal.SystemLibrary.get_engine_version(),'skeletal_mesh':mesh.get_path_name(),'skeleton':skeleton.get_path_name(),'bone_count':len(names),'bone_names':names,'root_and_wheel_reference_poses':poses,'inspection_clip':anim.get_path_name(),'pose_1s_wheel_travel_cm':travel,'pose_1s_wheel_axles':steering_axes,'pose_1s_spring_scales':shock_scales,'shock_cylinders_rigid':True,'materials':materials,'runtime_driving_implemented':False}
(OUT/'unreal_validation.json').write_text(json.dumps(report,indent=2))
unreal.log('ATV_UNREAL_VALIDATION_PASSED '+json.dumps(report))
