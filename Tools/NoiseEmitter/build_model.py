"""Blender 4.5: faithful original reconstruction and authored noise prop."""
import bpy, math, json, sys
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'TunaSweeper/SourceArt/Props/NoiseEmitter'
OUT.mkdir(parents=True,exist_ok=True)
bpy.ops.object.select_all(action='SELECT'); bpy.ops.object.delete(use_global=False)
scene=bpy.context.scene
scene.unit_settings.system='METRIC'; scene.unit_settings.scale_length=0.01
def material(name,color):
    m=bpy.data.materials.new(name); m.diffuse_color=(*color[:3],1); m.use_nodes=True
    bs=m.node_tree.nodes.get('Principled BSDF'); bs.inputs['Base Color'].default_value=m.diffuse_color; bs.inputs['Roughness'].default_value=.72
    return m
def mesh(name,verts,faces,mat):
    me=bpy.data.meshes.new(name); me.from_pydata(verts,[],faces); me.update()
    ob=bpy.data.objects.new(name,me); scene.collection.objects.link(ob); me.materials.append(mat)
    return ob
def box(name,center,extent,mat):
    bpy.ops.mesh.primitive_cube_add(size=2,location=center); ob=bpy.context.object; ob.name=name; ob.scale=extent
    bpy.ops.object.transform_apply(location=False,rotation=False,scale=True); ob.data.materials.append(mat); return ob
definition=json.loads((ROOT/'TunaSweeper/Content/Data/PeriodicNoiseEmitterMeshes.json').read_text())['mesh_definitions'][0]
mats={k:material(k,v) for k,v in definition['materials'].items()}
for p in definition['parts']:
    if p['type']=='box': box(p['name'],p['center'],p['extent'],mats[p['material']]); continue
    axis=Vector(p['axis']); a=Vector((0,0,1)).cross(axis).normalized(); b=axis.cross(a).normalized(); c=Vector(p['base_center']); n=p['sides']
    verts=[tuple(c+axis*d+(a*math.cos(i*2*math.pi/n)+b*math.sin(i*2*math.pi/n))*r) for d,r in [(0,p['inner_radius']),(p['length'],p['outer_radius'])] for i in range(n)]
    faces=[(i,(i+1)%n,(i+1)%n+n,i+n) for i in range(n)]
    if p['cap_back']: faces.append(tuple(reversed(range(n))))
    if p['cap_front']: faces.append(tuple(range(n,2*n)))
    mesh(p['name'],verts,faces,mats[p['material']])
objects=[o for o in scene.objects if o.type=='MESH']
scene.render.engine='CYCLES'; scene.cycles.samples=24
scene.world.color=(.35,.35,.35)
scene.render.resolution_x=1000; scene.render.resolution_y=850; scene.render.resolution_percentage=100
scene.view_settings.view_transform='AgX'
floor=box('StudioFloor',(0,0,-5),(900,900,5),material('Studio',(.12,.15,.18)))
for name,loc,power,size in [('Key',(200,-350,600),6500000,400),('Fill',(-400,-150,300),3500000,350),('Rim',(100,400,500),5000000,300)]:
    data=bpy.data.lights.new(name,'AREA'); data.energy=power; data.shape='DISK'; data.size=size
    ob=bpy.data.objects.new(name,data); scene.collection.objects.link(ob); ob.location=loc; ob.rotation_euler=(Vector((0,0,110))-ob.location).to_track_quat('-Z','Y').to_euler()
bpy.ops.object.camera_add(); cam=bpy.context.object; cam.data.type='ORTHO'; cam.data.ortho_scale=430; scene.camera=cam
for label,loc in [('original_front',(650,0,220)),('original_side',(0,-650,220)),('original_oblique',(480,-650,400))]:
    cam.location=loc; cam.rotation_euler=(Vector((0,0,105))-cam.location).to_track_quat('-Z','Y').to_euler(); scene.render.filepath=str(OUT/(label+'.png')); bpy.ops.render.render(write_still=True)
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'original_reconstruction.blend'))
(OUT/'original_definition.json').write_text(json.dumps(definition,indent=2)+'\n')
