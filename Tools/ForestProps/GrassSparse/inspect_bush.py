import bpy
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parents[3];OUT=ROOT/'TunaSweeper/SourceArt/Environment/ForestProps/GrassSparse/Reference'
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=str(OUT/'Bush_reference.fbx'))
objects=[o for o in bpy.context.scene.objects if o.type=='MESH']
for o in objects:
    if o.name!='Bush_Combined':bpy.data.objects.remove(o,do_unlink=True)
objects=[o for o in bpy.context.scene.objects if o.type=='MESH']
mat=bpy.data.materials.new('ReferenceBush');mat.use_nodes=True
node=mat.node_tree.nodes.new('ShaderNodeTexImage');node.image=bpy.data.images.load(str(OUT/'Bush_BaseColor.tga'))
bs=mat.node_tree.nodes.get('Principled BSDF');bs.inputs['Roughness'].default_value=.85;mat.node_tree.links.new(node.outputs['Color'],bs.inputs['Base Color'])
for o in objects:o.data.materials.clear();o.data.materials.append(mat)
points=[o.matrix_world@Vector(v) for o in objects for v in o.bound_box];center=sum(points,Vector())/len(points);size=max(max(p[i] for p in points)-min(p[i] for p in points) for i in range(3))
bpy.ops.object.camera_add(location=center+Vector((1.3,-1.9,1.25))*size);cam=bpy.context.object;cam.rotation_euler=(center-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.type='ORTHO';cam.data.ortho_scale=size*1.4
s=bpy.context.scene;s.camera=cam;s.render.engine='CYCLES';s.cycles.samples=24;s.world=bpy.data.worlds.new('World');s.world.use_nodes=True;s.world.node_tree.nodes['Background'].inputs[1].default_value=.6
bpy.ops.object.light_add(type='AREA',location=center+Vector((-1,-2,3))*size);bpy.context.object.data.energy=200*size*size;bpy.context.object.data.size=size*3
s.render.resolution_x=640;s.render.resolution_y=640;s.render.resolution_percentage=100;s.view_settings.view_transform='AgX';s.render.filepath=str(OUT/'Bush.png');bpy.ops.render.render(write_still=True)
