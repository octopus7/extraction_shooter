"""One-off Blender builder. Removed immediately after the asset creation commit."""
import bpy, bmesh, math, json
import numpy as np
from pathlib import Path
from mathutils import Vector

ROOT = Path('D:/github/extraction_shooter')
OUT = ROOT / 'TunaSweeper/SourceArt/Environment/BarrierGate'
OUT.mkdir(parents=True, exist_ok=True)
bpy.ops.object.select_all(action='SELECT')
bpy.ops.object.delete(use_global=False)
scene = bpy.context.scene
scene.unit_settings.system = 'METRIC'
scene.unit_settings.scale_length = 1
scene.render.engine = 'CYCLES'
scene.cycles.samples = 8

def material(name, color, metallic=0, rough=.5):
    mat = bpy.data.materials.new(name); mat.use_nodes = True
    p = mat.node_tree.nodes.get('Principled BSDF')
    p.inputs['Base Color'].default_value = (*color, 1)
    p.inputs['Metallic'].default_value = metallic
    p.inputs['Roughness'].default_value = rough
    return mat

paint = material('Paint_Ivory', (.64,.68,.65), .3)
yellow = material('Paint_SafetyYellow', (.92,.48,.035), .25)
dark = material('Gasket_Graphite', (.035,.045,.047), .1)
metal = material('Metal_Steel', (.19,.23,.25), .75, .3)
white = material('Arm_White', (.82,.85,.83), .2)

def box(name, loc, size, mat, bevel=.008):
    bpy.ops.mesh.primitive_cube_add(size=1, location=loc)
    obj = bpy.context.object; obj.name = name; obj.dimensions = size
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    obj.data.materials.append(mat)
    if bevel:
        mod = obj.modifiers.new('Manufactured edges', 'BEVEL'); mod.width=bevel; mod.segments=2
        bpy.context.view_layer.objects.active=obj; bpy.ops.object.modifier_apply(modifier=mod.name)
    return obj

def cylinder(name, loc, radius, depth, mat, axis='Y'):
    rot=(math.pi/2,0,0) if axis=='Y' else (0,0,0)
    bpy.ops.mesh.primitive_cylinder_add(vertices=24, radius=radius, depth=depth, location=loc, rotation=rot)
    obj=bpy.context.object;obj.name=name
    bpy.ops.object.transform_apply(location=False,rotation=True,scale=True)
    obj.data.materials.append(mat)
    bevel=obj.modifiers.new('Rim', 'BEVEL');bevel.width=.002;bevel.segments=2
    bpy.ops.object.modifier_apply(modifier=bevel.name)
    return obj

parts=[]
parts.append(box('Anchored base', (0,0,.04), (.48,.56,.08), dark))
parts.append(box('Cabinet', (0,0,.56), (.38,.46,.98), paint, .025))
parts.append(box('Yellow cap', (0,0,1.08), (.41,.49,.09), yellow,.018))
parts.append(box('Service gasket', (0,-.238,.43), (.31,.018,.59), dark,.009))
parts.append(box('Service door', (0,-.252,.43), (.288,.018,.565), paint,.006))
parts.append(box('Lower stripe', (0,-.265,.185), (.272,.008,.055), yellow,.003))
parts.append(box('Sensor surround', (0,-.244,.79), (.29,.035,.105), dark,.015))
parts.append(cylinder('Hinge mount', (0,-.255,1.0), .13,.055, dark))
parts.append(cylinder('Hinge steel rim', (0,-.29,1.0), .106,.06, metal))
parts.append(cylinder('Shaft', (0,-.333,1.0), .045,.09, metal))
parts.append(cylinder('Lock', (.105,-.269,.56), .014,.012, metal))
parts.append(box('Key slot', (.105,-.276,.56), (.003,.002,.015), dark,.0005))
for x in [-.11,.11]:
    for z in [.19,.66]:
        parts.append(cylinder('Service screw', (x,-.267,z), .006,.007,metal))
for z in [.35,.385,.42,.455]:
    parts.append(box('Right vent', (.193,0,z), (.009,.22,.009),dark,.002))
for x in [-.185,.185]:
    for y in [-.22,.22]:
        parts.append(cylinder('Anchor bolt',(x,y,.087),.018,.018,metal,'Z'))

def join(items,name,pivot=(0,0,0)):
    bpy.ops.object.select_all(action='DESELECT')
    for obj in items:obj.select_set(True)
    bpy.context.view_layer.objects.active=items[0]
    bpy.ops.object.join()
    obj=bpy.context.object;obj.name=name
    scene.cursor.location=pivot;bpy.ops.object.origin_set(type='ORIGIN_CURSOR')
    return obj

housing=join(parts,'SM_BarrierHousing')
# Clamped bevels on thin panels can meet at duplicate vertices; weld them before UV unwrap.
bm=bmesh.new();bm.from_mesh(housing.data)
bmesh.ops.remove_doubles(bm,verts=list(bm.verts),dist=0.000001)
bmesh.ops.dissolve_degenerate(bm,edges=list(bm.edges),dist=0.000001)
bm.to_mesh(housing.data);bm.free();housing.data.update()
bpy.ops.object.mode_set(mode='EDIT');bpy.ops.mesh.select_all(action='SELECT')
bpy.ops.uv.smart_project(angle_limit=math.radians(66),island_margin=.012,area_weight=0)
bpy.ops.uv.average_islands_scale()
bpy.ops.uv.pack_islands(rotate=True,margin=.008)
bpy.ops.object.mode_set(mode='OBJECT')
image=bpy.data.images.new('T_BarrierHousing_BaseColor',width=1024,height=1024,alpha=False)
image.generated_color=(.025,.03,.035,1)
for mat in housing.data.materials:
    tree=mat.node_tree
    # Bake subtle edge/weather shading into the unique atlas, no tiled runtime shader.
    p=tree.nodes.get('Principled BSDF')
    ao=tree.nodes.new('ShaderNodeAmbientOcclusion');ao.inputs['Distance'].default_value=.06
    ao.inputs['Color'].default_value=p.inputs['Base Color'].default_value
    tree.links.new(ao.outputs['Color'],p.inputs['Base Color'])
    node=tree.nodes.new('ShaderNodeTexImage');node.image=image;tree.nodes.active=node
scene.render.bake.use_pass_direct=False;scene.render.bake.use_pass_indirect=False
scene.render.bake.use_pass_color=True;scene.render.bake.margin=6
bpy.ops.object.bake(type='DIFFUSE')
image.filepath_raw=str(OUT/'T_BarrierHousing_BaseColor.png');image.file_format='PNG';image.save()
atlas=material('M_BarrierHousing',(.5,.5,.5),.35,.48)
tex=atlas.node_tree.nodes.new('ShaderNodeTexImage');tex.image=image
atlas.node_tree.links.new(tex.outputs['Color'],atlas.node_tree.nodes.get('Principled BSDF').inputs['Base Color'])
housing.data.materials.clear();housing.data.materials.append(atlas)
for p in housing.data.polygons:p.material_index=0

arm=box('SM_BarrierArm',(1.75,-.36,1), (3.5,.075,.11),white,.01)
scene.cursor.location=(0,-.36,1);bpy.ops.object.origin_set(type='ORIGIN_CURSOR')
uv=arm.data.uv_layers.active
for poly in arm.data.polygons:
    for li in poly.loop_indices:
        co=arm.data.vertices[arm.data.loops[li].vertex_index].co
        if abs(poly.normal.x) > max(abs(poly.normal.y), abs(poly.normal.z)):
            uv.data[li].uv=(co.y/.15+.5,co.z/.15+.5)
        else:
            uv.data[li].uv=(co.x/.5, (co.z if abs(poly.normal.y)>abs(poly.normal.z) else co.y)/.15+.5)
stripe=bpy.data.images.new('T_BarrierArm_Stripes',width=256,height=128,alpha=False)
yy,xx=np.mgrid[0:128,0:256];mask=((xx/256+yy/128*.14)%1)<.45
pixels=np.ones((128,256,4),dtype=np.float32);pixels[:,:,:3]=(.84,.87,.85)
pixels[mask,:3]=(.66,.025,.018)
stripe.pixels.foreach_set(pixels.ravel());stripe.filepath_raw=str(OUT/'T_BarrierArm_Stripes.png');stripe.file_format='PNG';stripe.save()
armmat=material('M_BarrierArm',(.8,.8,.8),.2,.36)
node=armmat.node_tree.nodes.new('ShaderNodeTexImage');node.image=stripe
armmat.node_tree.links.new(node.outputs['Color'],armmat.node_tree.nodes.get('Principled BSDF').inputs['Base Color'])
arm.data.materials.clear();arm.data.materials.append(armmat)

redmat=material('M_BarrierLED_Red',(.6,.005,.002),.1,.25)
greenmat=material('M_BarrierLED_Green',(.002,.3,.015),.1,.25)
redmat.node_tree.nodes.get('Principled BSDF').inputs['Emission Color'].default_value=(1,.01,.002,1)
redmat.node_tree.nodes.get('Principled BSDF').inputs['Emission Strength'].default_value=2
red=cylinder('SM_BarrierRedLens',(-.09,-.268,.79),.033,.013,redmat)
green=cylinder('SM_BarrierGreenLens',(.09,-.268,.79),.033,.013,greenmat)
for obj in [red,green]:
    bpy.ops.object.select_all(action='DESELECT');obj.select_set(True)
    scene.cursor.location=(0,0,0);bpy.context.view_layer.objects.active=obj
    bpy.ops.object.origin_set(type='ORIGIN_CURSOR')

meshes=[housing,arm,red,green]
manifest=[]
for obj in meshes:
    pivot=obj.location.copy();obj.location=(0,0,0)
    bpy.ops.object.select_all(action='DESELECT');obj.select_set(True);bpy.context.view_layer.objects.active=obj
    bpy.ops.export_scene.fbx(filepath=str(OUT/(obj.name+'.fbx')),use_selection=True,object_types={'MESH'},
        apply_unit_scale=True,apply_scale_options='FBX_SCALE_NONE',axis_forward='-Y',axis_up='Z',
        use_mesh_modifiers=True,mesh_smooth_type='FACE',add_leaf_bones=False,bake_anim=False,path_mode='STRIP')
    obj.location=pivot
    obj.data.calc_loop_triangles()
    manifest.append(dict(name=obj.name,triangles=len(obj.data.loop_triangles),vertices=len(obj.data.vertices)))

# Studio preview lives only in the Blender source; no Unreal map or placed actor.
ground=box('Preview floor',(1.5,0,-.045),(200,200,.03),material('PreviewFloor',(.09,.12,.14)),0)
bpy.ops.object.camera_add(location=(5,-7,4))
camera=bpy.context.object
camera.rotation_euler=(Vector((1.45,0,.65))-camera.location).to_track_quat('-Z','Y').to_euler()
camera.data.type='ORTHO';camera.data.ortho_scale=5.1;scene.camera=camera
for loc,power,size in [((0,-3,5),700,5),((3,2,4),950,4)]:
    bpy.ops.object.light_add(type='AREA',location=loc)
    light=bpy.context.object;light.data.energy=power;light.data.shape='DISK';light.data.size=size
    light.rotation_euler=(Vector((1,0,.5))-light.location).to_track_quat('-Z','Y').to_euler()
scene.world.color=(.3,.3,.3)
scene.render.resolution_x=1200;scene.render.resolution_y=800;scene.render.resolution_percentage=100
scene.cycles.samples=24
scene.view_settings.view_transform='AgX'
scene.render.filepath=str(OUT/'BarrierGate_Preview.png')
for img in [image,stripe]: img.pack()
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'BarrierGate.blend'))
bpy.ops.render.render(write_still=True)
(OUT/'manifest.json').write_text(json.dumps(dict(meshes=manifest,housing_texture=1024,unique_housing_uv=True,arm_length_cm=350),indent=2))
print('BARRIER_MODEL_CREATED')
