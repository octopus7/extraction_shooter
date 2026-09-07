"""Deterministic low grass geometry, packed texture, FBX and presentation renders.
Blender 4.5 LTS: blender -b --factory-startup -t 6 --python-exit-code 1 --python <this file>
"""
import bpy
import json
import math
import random
from pathlib import Path
from mathutils import Vector

ROOT=Path(__file__).resolve().parents[3]
OUT=ROOT/'TunaSweeper/SourceArt/Environment/GrassDenseShort'
NAME='SM_GrassDenseShort'
for folder in ['Models','Textures','Previews']: (OUT/folder).mkdir(parents=True,exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True)
scene=bpy.context.scene
scene.unit_settings.system='METRIC';scene.unit_settings.scale_length=1
rng=random.Random(906413)

# Four broad, muted greens; each vertical lane has only a gentle root-to-tip shift.
# No noise, normal map, transparency, flowers, or separate shadow/base plate.
palette=[(72,99,43),(81,107,49),(90,114,55),(69,96,46)]
tex=bpy.data.images.new('T_GrassDenseShort_BaseColor',width=64,height=64,alpha=False)
pixels=[]
for y in range(64):
    shade=.78+.22*y/63
    for x in range(64): pixels.extend([v/255*shade for v in palette[x//16]]+[1])
tex.pixels.foreach_set(pixels)
tex.filepath_raw=str(OUT/'Textures/T_GrassDenseShort_BaseColor.png');tex.file_format='PNG';tex.save();tex.pack()
tex.filepath='//Textures/T_GrassDenseShort_BaseColor.png'
mat=bpy.data.materials.new('M_GrassDenseShort');mat.use_nodes=True
bs=mat.node_tree.nodes.get('Principled BSDF');bs.inputs['Roughness'].default_value=.92
bs.inputs['Specular IOR Level'].default_value=.18
node=mat.node_tree.nodes.new('ShaderNodeTexImage');node.image=tex
mat.node_tree.links.new(node.outputs['Color'],bs.inputs['Base Color'])
mat.diffuse_color=(.23,.33,.12,1);mat.use_backface_culling=False

verts=[];faces=[];uvs=[];blades=[]
local_faces=[(0,1,4),(0,4,3),(1,2,5),(1,5,4),(3,4,6),(4,5,6)]
local_uv=[(0,0),(.5,0),(1,0),(0,.56),(.5,.56),(1,.56),(.5,1)]
# Irrregular radial point distribution; four lobes break the outline without isolated spikes.
for tuft in range(43):
    a=tuft*2.39996323+rng.uniform(-.15,.15)
    r=math.sqrt((tuft+.4)/43)
    lobe=1+.12*math.sin(3*a+.8)+.07*math.cos(5*a)
    cx=math.cos(a)*r*.34*lobe;cy=math.sin(a)*r*.285*lobe
    height=rng.uniform(.115,.17)*(1-.15*r)
    for j in range(5):
        theta=a+j*math.tau/5+rng.uniform(-.4,.4)
        d=Vector((math.cos(theta),math.sin(theta),0));side=Vector((-d.y,d.x,0))
        h=height*rng.uniform(.76,1.15)
        width=rng.uniform(.021,.034)
        bend=rng.uniform(.042,.095)
        root=Vector((cx,cy,0))+d*rng.uniform(0,.016)
        middle=root+d*bend*.31+Vector((0,0,h*.60))
        tip=root+d*bend+Vector((0,0,h))
        points=[root-side*width*.38,root+Vector((0,0,.002)),root+side*width*.38,
                middle-side*width*.42,middle-d*.004+Vector((0,0,.0025)),middle+side*width*.42,tip]
        start=len(verts);verts.extend([tuple(p) for p in points])
        lane=(tuft//4+j%2)%4
        for f in local_faces:
            faces.append(tuple(start+k for k in f))
            uvs.append([((lane+.17+.66*local_uv[k][0])/4,.06+.88*local_uv[k][1]) for k in f])
        blades.append({'tuft':tuft,'height_m':h,'lane':lane})
mesh=bpy.data.meshes.new(NAME);mesh.from_pydata(verts,[],faces);mesh.update()
obj=bpy.data.objects.new(NAME,mesh);scene.collection.objects.link(obj)
mesh.materials.append(mat)
uv=mesh.uv_layers.new(name='UVMap')
for p,p_uv in zip(mesh.polygons,uvs):
    for li,value in zip(p.loop_indices,p_uv):uv.data[li].uv=value
    p.use_smooth=False
bpy.context.view_layer.objects.active=obj;obj.select_set(True)
mesh.calc_loop_triangles()
assert all(p.area>1e-9 for p in mesh.polygons)
assert all(abs(p.normal.length-1)<1e-5 for p in mesh.polygons)
assert all(math.isfinite(x) for v in mesh.vertices for x in v.co)
assert min(v.co.z for v in mesh.vertices)==0
assert len(mesh.materials)==1 and len(mesh.uv_layers)==1
assert all(0<u.uv.x<1 and 0<u.uv.y<1 for u in uv.data)
uv_areas=[]
for p in mesh.polygons:
    a,b,c=[uv.data[i].uv for i in p.loop_indices]
    uv_areas.append(abs((b-a).x*(c-a).y-(b-a).y*(c-a).x)*.5)
assert min(uv_areas)>1e-8
bounds=[min(v.co[i] for v in mesh.vertices) for i in range(3)]+[max(v.co[i] for v in mesh.vertices) for i in range(3)]
manifest={'name':NAME,'seed':906413,'blades':len(blades),'tufts':43,'vertices':len(mesh.vertices),
          'triangles':len(mesh.loop_triangles),'bounds_m':bounds,'dimensions_cm':[(bounds[i+3]-bounds[i])*100 for i in range(3)],
          'pivot_m':[0,0,0],'unit_scale_m':1,'fbx_axis_forward':'-Y','fbx_axis_up':'Z',
          'ue_axis_mapping':'Blender X -> UE X, Blender Y -> UE -Y, Blender Z -> UE Z (validated against signed bounds)',
          'materials':['M_GrassDenseShort'],'uv_channels':1,'min_uv_triangle_area':min(uv_areas),
          'degenerate_triangles':0,'invalid_normals':0,'two_sided':True,'alpha_mode':'OPAQUE','collision':False,
          'note':'Open leaf ribbons intentional: opaque two-sided material; overlapping UVs intentional for palette reuse. Dynamic lighting; no baked lightmap UV.'}
(OUT/'model_manifest.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
bpy.ops.export_scene.fbx(filepath=str(OUT/f'Models/{NAME}.fbx'),use_selection=True,object_types={'MESH'},
    global_scale=1,apply_unit_scale=True,apply_scale_options='FBX_SCALE_UNITS',axis_forward='-Y',axis_up='Z',
    use_mesh_modifiers=True,mesh_smooth_type='FACE',use_triangles=True,add_leaf_bones=False,
    bake_anim=False,path_mode='COPY',embed_textures=True)

# Asset stays at the origin in the blend. Presentation objects are clearly named.
scene.render.engine='CYCLES';scene.cycles.samples=32;scene.cycles.use_denoising=True
scene.render.resolution_x=1200;scene.render.resolution_y=900;scene.render.resolution_percentage=100
scene.view_settings.view_transform='AgX'
scene.world=bpy.data.worlds.new('Preview_World');scene.world.use_nodes=True
scene.world.node_tree.nodes['Background'].inputs[0].default_value=(.6,.66,.73,1)
scene.world.node_tree.nodes['Background'].inputs[1].default_value=.65
bpy.ops.mesh.primitive_plane_add(size=200);floor=bpy.context.object;floor.name='Preview_Ground';floor.location.z=-.003
ground=bpy.data.materials.new('Preview_GroundMaterial');ground.diffuse_color=(.19,.17,.125,1);floor.data.materials.append(ground)
for loc,power,size in [((1,-2,4),450,4),((-3,1,2),180,3)]:
    bpy.ops.object.light_add(type='AREA',location=loc);o=bpy.context.object;o.name='Preview_Softbox'
    o.data.energy=power;o.data.size=size;o.rotation_euler=(-o.location).to_track_quat('-Z','Y').to_euler()
bpy.ops.object.camera_add();cam=bpy.context.object;cam.name='Preview_Camera';cam.data.type='ORTHO';scene.camera=cam
def render(name,location,target=(0,0,.055),scale=1.08):
    cam.location=location;cam.rotation_euler=(Vector(target)-cam.location).to_track_quat('-Z','Y').to_euler()
    cam.data.ortho_scale=scale;scene.render.filepath=str(OUT/f'Previews/{name}.png');bpy.ops.render.render(write_still=True)
render('01_hero',(1,-1.4,.95))
render('02_reverse',(-1,1.4,.65))
render('03_top',(0,0,2.5),target=(0,0,0))
render('04_profile',(0,-2,.32))
copies=[]
for y in range(5):
    for x in range(5):
        if x==2 and y==2:continue
        o=bpy.data.objects.new(f'Preview_Repeat_{x}_{y}',mesh);scene.collection.objects.link(o)
        o.location=((x-2)*.59+rng.uniform(-.085,.085),(y-2)*.54+rng.uniform(-.085,.085),0)
        o.rotation_euler.z=rng.uniform(0,math.tau);s=rng.uniform(.88,1.12);o.scale=(s,s,s)
        copies.append(o)
render('05_repeat_25',(2,-3.4,4),scale=4.15)
for o in copies:o.hide_render=True;o.hide_set(True)
render('06_game_distance',(2,-3,3.8),scale=3.6)
cam.location=(1,-1.4,.95);cam.rotation_euler=(Vector((0,0,.055))-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.ortho_scale=1.08
bpy.ops.object.select_all(action='DESELECT');obj.select_set(True);bpy.context.view_layer.objects.active=obj
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/f'{NAME}.blend'))
print('GRASS_SOURCE_VALIDATED '+json.dumps(manifest))
