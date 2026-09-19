"""Read-only ATV source/FBX structural checks; optional rendered pose inspection."""
import bpy,json,hashlib,math,sys
from pathlib import Path
from mathutils import Vector
ROOT=Path('D:/github/extraction_shooter');OUT=ROOT/'TunaSweeper/SourceArt/Vehicles/ATV'
manifest=json.loads((OUT/'rig_manifest.json').read_text())
assert hashlib.sha256((ROOT/manifest['source']).read_bytes()).hexdigest()==manifest['source_sha256']
bpy.ops.wm.open_mainfile(filepath=str(ROOT/'Blender/SKM_ATV.blend'))
scene=bpy.context.scene;scene.frame_set(1);arm=bpy.data.objects['Armature'];meshes=[o for o in scene.objects if o.type=='MESH']
assert set(arm.data.bones.keys())=={b['name'] for b in manifest['bones']}
assert tuple(arm.scale)==(1,1,1)
for key in manifest['wheel_order']:
 bone=arm.data.bones['wheel_'+key]
 assert bone.parent.name=='root'
 assert (bone.tail_local-bone.head_local).normalized().dot(Vector((1,0,0)))>.99999
for obj in meshes:
 assert len(obj.vertex_groups)==1,obj.name
 assert obj.vertex_groups[0].name in arm.data.bones
 assert len(obj.data.uv_layers)>0
 for vertex in obj.data.vertices:
  assert all(math.isfinite(x) for x in vertex.co)
  assert len(vertex.groups)==1 and abs(vertex.groups[0].weight-1)<1e-7
scene.frame_set(31)
for key in manifest['wheel_order']:
 matrix=arm.pose.bones['wheel_'+key].matrix
 assert all(abs(c.length-1)<1e-4 for c in matrix.to_3x3().col),'wheel deformation scale'
 assert abs(matrix.translation.z-arm.data.bones['wheel_'+key].head_local.z)>6.9
scene.frame_set(1)
points=[obj.matrix_world@v.co for obj in meshes for v in obj.data.vertices]
bounds=[[min(p[a] for p in points),max(p[a] for p in points)] for a in range(3)]
report={'passed':True,'blender_version':bpy.app.version_string,'source_unchanged':True,'bones':len(arm.data.bones),'mesh_parts':len(meshes),'rigid_weights':True,'four_wheel_axes_x_forward':True,'pose_rigid_wheels':True,'bind_bounds_blender_cm':bounds}

if '--render' in sys.argv:
 scene.render.engine='CYCLES';scene.cycles.samples=24;scene.cycles.use_denoising=True
 scene.render.resolution_x=1280;scene.render.resolution_y=1000;scene.render.resolution_percentage=100
 scene.world.use_nodes=True;scene.world.node_tree.nodes['Background'].inputs['Color'].default_value=(.12,.14,.18,1);scene.world.node_tree.nodes['Background'].inputs['Strength'].default_value=.55
 target=Vector((-2,0,55))
 for name,loc,power,size in [('Key',(160,-240,300),1800000,210),('Fill',(-180,150,200),1300000,180),('Rim',(50,230,300),1800000,180)]:
  data=bpy.data.lights.new(name,'AREA');data.energy=power;data.size=size
  light=bpy.data.objects.new(name,data);scene.collection.objects.link(light);light.location=loc;light.rotation_euler=(target-light.location).to_track_quat('-Z','Y').to_euler()
 data=bpy.data.cameras.new('PreviewCamera');camera=bpy.data.objects.new('PreviewCamera',data);scene.collection.objects.link(camera);scene.camera=camera;data.type='ORTHO';data.ortho_scale=255
 camera.location=(320,-350,245);camera.rotation_euler=(target-camera.location).to_track_quat('-Z','Y').to_euler()
 for frame,name in [(1,'ATV_Rest'),(31,'ATV_Steering_Suspension')]:
  scene.frame_set(frame);scene.render.filepath=str(OUT/'Previews'/f'{name}.png');bpy.ops.render.render(write_still=True)
 scene.frame_set(31);camera.location=(260,-330,95);camera.rotation_euler=(Vector((15,0,39))-camera.location).to_track_quat('-Z','Y').to_euler();data.ortho_scale=215
 scene.render.filepath=str(OUT/'Previews/ATV_Suspension_Detail.png');bpy.ops.render.render(write_still=True)

# Import the exported FBX into a clean scene in the same centimeter convention.
before=hashlib.sha256((OUT/'SKM_ATV.fbx').read_bytes()).hexdigest()
bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
bpy.ops.import_scene.fbx(filepath=str(OUT/'SKM_ATV.fbx'),use_anim=False,automatic_bone_orientation=False)
imported=[o for o in bpy.context.scene.objects if o.type=='MESH'];arms=[o for o in bpy.context.scene.objects if o.type=='ARMATURE']
assert len(arms)==1
assert {b['name'] for b in manifest['bones']}==set(arms[0].data.bones.keys())
points=[o.matrix_world@v.co for o in imported for v in o.data.vertices]
fbx_bounds=[[min(p[a] for p in points),max(p[a] for p in points)] for a in range(3)]
error=max(abs(bounds[a][j]-fbx_bounds[a][j]) for a in range(3) for j in range(2))
assert error<.02,(bounds,fbx_bounds,error)
assert before==hashlib.sha256((OUT/'SKM_ATV.fbx').read_bytes()).hexdigest()
report.update({'fbx_roundtrip_passed':True,'fbx_bounds_blender_cm':fbx_bounds,'max_fbx_bounds_error_cm':error,'fbx_sha256':before})
(OUT/'blender_validation.json').write_text(json.dumps(report,indent=2))
print('ATV_BLENDER_VALIDATION_PASSED',json.dumps(report))
