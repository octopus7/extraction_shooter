"""Deterministic, opaque low-poly sparse shrub. Blender 4.5 LTS, meters.

All geometry, UVs and six flat color swatches are authored here; no external
image generation or painted/noisy texture is needed. Only this unique output
folder is written. The preview stage is excluded from FBX.
"""
import bpy, bmesh, math, random, json
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parents[3]
OUT=ROOT/'TunaSweeper/SourceArt/Environment/ForestProps/BushSparse'
for sub in ['Models','Textures','Previews']:(OUT/sub).mkdir(parents=True,exist_ok=True)
random.seed(91063)
bpy.ops.wm.read_factory_settings(use_empty=True)
scene=bpy.context.scene;scene.unit_settings.system='METRIC';scene.unit_settings.scale_length=1
scene.render.engine='CYCLES';scene.cycles.samples=40;scene.cycles.use_denoising=True
scene.render.resolution_x=1200;scene.render.resolution_y=1000;scene.render.resolution_percentage=100
scene.view_settings.view_transform='AgX'
scene.world=bpy.data.worlds.new('Soft outdoor studio');scene.world.use_nodes=True
scene.world.node_tree.nodes['Background'].inputs[0].default_value=(.48,.55,.61,1)
scene.world.node_tree.nodes['Background'].inputs[1].default_value=.55
# Deliberately broad, solid color regions: muted greens + warm Wood-family browns.
swatches=[(65,96,40),(78,110,47),(90,120,54),(70,102,43),(104,77,45),(126,94,55)]
tex=bpy.data.images.new('T_BushSparse_Palette',width=192,height=32,alpha=False)
pixels=[]
for y in range(32):
    for x in range(192):pixels.extend([c/255 for c in swatches[x//32]]+[1])
tex.pixels=pixels;tex.filepath_raw=str(OUT/'Textures/T_BushSparse_Palette.png');tex.file_format='PNG';tex.save();tex.pack();tex.filepath='//Textures/T_BushSparse_Palette.png'
mat=bpy.data.materials.new('M_BushSparse');mat.use_nodes=True
bsdf=mat.node_tree.nodes.get('Principled BSDF');bsdf.inputs['Roughness'].default_value=.88
node=mat.node_tree.nodes.new('ShaderNodeTexImage');node.image=tex;node.interpolation='Closest'
mat.node_tree.links.new(node.outputs['Color'],bsdf.inputs['Base Color'])
verts=[];faces=[];tiles=[];branch_count=0;leaf_count=0

def piece(v,f,tile):
    off=len(verts);verts.extend(v);faces.extend([tuple(off+i for i in p) for p in f]);tiles.extend([tile]*len(f))

def branch(points,radii):
    global branch_count
    branch_count+=1;v=[];f=[];n=5
    for j,p in enumerate(points):
        tangent=Vector(points[min(j+1,len(points)-1)])-Vector(points[max(0,j-1)])
        q=tangent.to_track_quat('Z','Y')
        for i in range(n):v.append(Vector(p)+q@Vector((radii[j]*math.cos(i*math.tau/n),radii[j]*math.sin(i*math.tau/n),0)))
    f.append(tuple(reversed(range(n))))
    for j in range(len(points)-1):
        for i in range(n):f.append((j*n+i,j*n+(i+1)%n,(j+1)*n+(i+1)%n,(j+1)*n+i))
    f.append(tuple((len(points)-1)*n+i for i in range(n)))
    piece(v,f,4 if branch_count%3 else 5)

def leaf(base,direction,length,width,tile):
    global leaf_count
    leaf_count+=1
    forward=Vector(direction).normalized();side=forward.cross(Vector((0,0,1))).normalized()
    up=side.cross(forward).normalized()
    base=Vector(base)
    # Six-point outline, shallow ridge above and below: a closed solid leaf.
    outline=[(0,0),(.30,-.48),(.70,-.41),(1,0),(.67,.43),(.27,.47)]
    v=[base+forward*(a*length)+side*(b*width) for a,b in outline]
    v += [base+forward*(length*.48)+up*.019,base+forward*(length*.48)-up*.007]
    f=[(i,(i+1)%6,6) for i in range(6)]+[((i+1)%6,i,7) for i in range(6)]
    piece(v,f,tile)

def spray(tip,azimuth,scale,index):
    # Loose terminal clusters; asymmetry prevents a regular flower/star shape.
    for j,(turn,lift,size) in enumerate([(-.78,.24,.93),(.20,.32,1.08),(1.12,.05,.83),(-1.65,.42,.74)]):
        a=azimuth+turn+random.uniform(-.12,.12)
        d=Vector((math.cos(a),math.sin(a),lift))
        offset=Vector((math.cos(azimuth),math.sin(azimuth),.3))*(j%2)*.021
        leaf(Vector(tip)+offset,d,scale*size,scale*.59, (index+j//2)%4)

# Seven irregular canes, two terminal sprays each, exposed lower branch lengths.
canes=[(-152,.45,.39),(-101,.43,.55),(-42,.49,.40),(14,.44,.59),(72,.41,.44),(128,.43,.54),(174,.30,.64)]
for i,(degrees,radius,height) in enumerate(canes):
    a=math.radians(degrees);d=Vector((math.cos(a),math.sin(a),0));side=Vector((-d.y,d.x,0))
    p0=d*.035;p0.z=0
    p1=d*(radius*.24)+side*.025;p1.z=height*.35
    p2=d*(radius*.67)+side*(.024 if i%2 else -.018);p2.z=height*.73
    tip=d*radius;tip.z=height
    branch([p0,p1,p2,tip],[.018,.014,.009,.0035])
    spray(tip,a,.20+random.uniform(-.015,.025),i)
    fork=p2+side*(.125 if i%2 else -.14)+d*.055+Vector((0,0,.035))
    branch([p1.lerp(p2,.64),fork.lerp(p2,.3),fork],[.008,.005,.0025])
    spray(fork,a+(.9 if i%2 else -.85),.16+random.uniform(-.012,.012),i+1)
    # One mid-height leaf per cane, avoiding a bald miniature-tree appearance.
    leaf(p1.lerp(p2,.58),d+side*(.7 if i%2 else -.7)+Vector((0,0,.2)),.145,.085,i%4)

ground_z=min(v.z for v in verts)
verts=[Vector((v.x,v.y,v.z-ground_z)) for v in verts]
mesh=bpy.data.meshes.new('BushSparse_Geometry');mesh.from_pydata(verts,[],faces);mesh.materials.append(mat);mesh.update()
obj=bpy.data.objects.new('SM_BushSparse',mesh);scene.collection.objects.link(obj)
bpy.context.view_layer.objects.active=obj;obj.select_set(True)
uv=mesh.uv_layers.new(name='PaletteUV')
for poly,tile in zip(mesh.polygons,tiles):
    # Every polygon has nonzero UV area, safely inset 25% inside its color tile.
    n=len(poly.loop_indices)
    for j,loop in enumerate(poly.loop_indices):uv.data[loop].uv=((tile+.5+.24*math.cos(j*math.tau/n))/6,.5+.24*math.sin(j*math.tau/n))
bm=bmesh.new();bm.from_mesh(mesh);bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces));bmesh.ops.triangulate(bm,faces=list(bm.faces));bm.to_mesh(mesh);bm.free()
mesh.uv_layers.new(name='LightmapUV');mesh.uv_layers.active_index=1
bpy.ops.object.mode_set(mode='EDIT');bpy.ops.mesh.select_all(action='SELECT');bpy.ops.uv.smart_project(angle_limit=math.radians(66),island_margin=.012);bpy.ops.object.mode_set(mode='OBJECT');mesh.uv_layers.active_index=0
for p in mesh.polygons:p.use_smooth=False
bm=bmesh.new();bm.from_mesh(mesh)
assert all(f.calc_area()>1e-10 for f in bm.faces)
assert all(e.is_manifold for e in bm.edges)
assert all(math.isfinite(c) for v in mesh.vertices for c in v.co)
assert all(abs(p.normal.length-1)<1e-5 for p in mesh.polygons)
assert all(0<=c<=1 for layer in mesh.uv_layers for data in layer.data for c in data.uv)
bm.free()
lo=[min(v.co[i] for v in mesh.vertices) for i in range(3)];hi=[max(v.co[i] for v in mesh.vertices) for i in range(3)]
assert abs(lo[2])<1e-6
report={'name':obj.name,'blender_version':bpy.app.version_string,'seed':91063,'units':'meters','bounds_m':lo+hi,'dimensions_m':[hi[i]-lo[i] for i in range(3)],'pivot_m':[0,0,0],'vertices':len(mesh.vertices),'triangles':len(mesh.polygons),'uv_channels':[l.name for l in mesh.uv_layers],'material_slots':1,'leaf_count':leaf_count,'branch_components':branch_count,'closed_manifold_components':True,'degenerate_faces':0,'finite_unit_normals':True,'alpha':False,'two_sided_required':False,'collision_meshes':0,'palette_srgb_8bit':swatches,'fbx_export':{'axis_forward':'-Y','axis_up':'Z','apply_unit_scale':True,'apply_scale_options':'FBX_SCALE_UNITS'},'passed':True}
(OUT/'model_manifest.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
bpy.ops.export_scene.fbx(filepath=str(OUT/'Models/SM_BushSparse.fbx'),use_selection=True,object_types={'MESH'},axis_forward='-Y',axis_up='Z',apply_unit_scale=True,apply_scale_options='FBX_SCALE_UNITS',mesh_smooth_type='FACE',use_mesh_modifiers=True,add_leaf_bones=False,bake_anim=False,path_mode='COPY',embed_textures=True)
# A review stage in its own nonexported collection.
stage=bpy.data.collections.new('PREVIEW_ONLY_NOT_EXPORTED');scene.collection.children.link(stage)
def stage_object(o):
    for c in list(o.users_collection):c.objects.unlink(o)
    stage.objects.link(o)
bpy.ops.mesh.primitive_plane_add(size=200,location=(0,0,-.008));floor=bpy.context.object;stage_object(floor)
ground=bpy.data.materials.new('Preview Ground');ground.diffuse_color=(.21,.235,.19,1);floor.data.materials.append(ground)
for loc,power,size in [((-3,-4,7),650,5),((3,1,4),260,4)]:
    bpy.ops.object.light_add(type='AREA',location=loc);light=bpy.context.object;stage_object(light);light.data.energy=power;light.data.shape='DISK';light.data.size=size;light.rotation_euler=(-light.location).to_track_quat('-Z','Y').to_euler()
bpy.ops.object.camera_add();cam=bpy.context.object;stage_object(cam);cam.data.type='ORTHO';scene.camera=cam
def render(name,loc,target,scale):
    cam.location=loc;cam.rotation_euler=(Vector(target)-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.ortho_scale=scale
    scene.render.filepath=str(OUT/'Previews'/name);bpy.ops.render.render(write_still=True)
render('BushSparse_Hero.png',(1.9,-2.7,1.8),(0,0,.29),1.65)
render('BushSparse_Front.png',(0,-3,.76),(0,0,.31),1.58)
render('BushSparse_Back.png',(-1.3,2.6,1.3),(0,0,.29),1.65)
render('BushSparse_Top.png',(0,-.01,4),(0,0,0),1.65)
copies=[]
for row in range(3):
    for col in range(4):
        if row==1 and col==1:continue
        clone=bpy.data.objects.new('Preview_instance',mesh);stage.objects.link(clone);copies.append(clone)
        clone.location=((col-1)*1.35+random.uniform(-.1,.1),(row-1)*1.28+random.uniform(-.1,.1),0)
        clone.rotation_euler.z=random.uniform(-math.pi,math.pi);s=random.uniform(.85,1.13);clone.scale=(s,s,s)
scene.render.resolution_x=1500;scene.render.resolution_y=1050
render('BushSparse_Repeated.png',(5,-7,8),(.55,0,.12),7.2)
for c in copies:bpy.data.objects.remove(c,do_unlink=True)
scene.render.resolution_x=1200;scene.render.resolution_y=1000
cam.location=(1.9,-2.7,1.8);cam.rotation_euler=(Vector((0,0,.29))-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.ortho_scale=1.65
bpy.ops.object.select_all(action='DESELECT');obj.select_set(True);bpy.context.view_layer.objects.active=obj
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'BushSparse.blend'))
print('BUSH_SPARSE_SOURCE_PASSED',json.dumps(report))
