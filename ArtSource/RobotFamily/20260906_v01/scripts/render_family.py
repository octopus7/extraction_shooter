"""Render the real four-model lineup; no AI retouching of geometry previews."""
import bpy,math
from pathlib import Path
from mathutils import Vector
OUT=Path(__file__).resolve().parents[1]
bpy.ops.wm.read_factory_settings(use_empty=True)
scene=bpy.context.scene;scene.unit_settings.system='METRIC';scene.unit_settings.scale_length=.01
names=["Q1_Scout","Q2_Bulwark","H1_Carrier","B1_Sentry"]
right=Vector((.33035,.94386,0))
for name,offset in zip(names,[-440,-150,155,435]):
 with bpy.data.libraries.load(str(OUT/(name+".blend")),link=False) as (src,dst):
  dst.objects=[n for n in src.objects if n=="Armature" or n=="SK_"+name]
 for ob in dst.objects:bpy.context.collection.objects.link(ob)
 rig=next(o for o in dst.objects if o.type=='ARMATURE')
 rig.name=name+"_Rig";rig.animation_data_clear();rig.location=right*offset
 for pb in rig.pose.bones:pb.rotation_euler=(0,0,0)
target=Vector((0,0,70));direction=Vector((1000,-350,730)).normalized()
bpy.ops.object.camera_add(location=target+direction*1600)
cam=bpy.context.object;cam.rotation_euler=(-direction).to_track_quat('-Z','Y').to_euler()
cam.data.type='ORTHO';cam.data.ortho_scale=1170;cam.data.clip_end=10000;scene.camera=cam
up=cam.rotation_euler.to_quaternion()@Vector((0,1,0))
labelmat=bpy.data.materials.new("Preview_Label");labelmat.diffuse_color=(.82,.86,.88,1)
for name,offset in zip(names,[-440,-150,155,435]):
 bpy.ops.object.text_add(location=target+right*offset+up*156)
 ob=bpy.context.object;ob.rotation_euler=cam.rotation_euler;ob.data.body=name.replace("_"," ").upper()
 ob.data.align_x='CENTER';ob.data.size=16;ob.data.materials.append(labelmat)
bpy.ops.mesh.primitive_plane_add(size=2400,location=(0,0,-.3))
floor=bpy.context.object;floor.name="Preview_Floor"
m=bpy.data.materials.new("Preview_Floor");m.diffuse_color=(.095,.12,.15,1);m.use_nodes=True;m.node_tree.nodes["Principled BSDF"].inputs["Base Color"].default_value=(.095,.12,.15,1);m.node_tree.nodes["Principled BSDF"].inputs["Roughness"].default_value=.85;floor.data.materials.append(m)
scene.world=bpy.data.worlds.new("Studio");scene.world.use_nodes=True;scene.world.node_tree.nodes["Background"].inputs[0].default_value=(.17,.20,.24,1);scene.world.node_tree.nodes["Background"].inputs[1].default_value=.5
for loc,power,size in [((400,-450,800),16000000,650),((100,500,550),9000000,650),((-500,0,650),10000000,600)]:
 bpy.ops.object.light_add(type='AREA',location=loc);o=bpy.context.object;o.data.energy=power;o.data.shape='DISK';o.data.size=size;o.rotation_euler=(target-o.location).to_track_quat('-Z','Y').to_euler()
scene.render.engine='CYCLES';scene.cycles.samples=32;scene.cycles.use_denoising=True
scene.render.resolution_x=1920;scene.render.resolution_y=760;scene.render.resolution_percentage=100;scene.view_settings.view_transform='AgX'
scene.render.image_settings.file_format='PNG';scene.render.filepath=str(OUT/"previews/RobotFamily_Overview.png")
bpy.context.preferences.filepaths.save_version=0
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/"RobotFamily_Overview.blend"))
bpy.ops.render.render(write_still=True)
