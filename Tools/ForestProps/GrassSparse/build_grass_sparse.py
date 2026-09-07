"""Deterministic Blender 4.5 build. Metres, Z up, root pivot at terrain Z=0."""
import bpy, bmesh, json, math, random
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parents[3]
OUT=ROOT/'TunaSweeper/SourceArt/Environment/ForestProps/GrassSparse'
for d in ['Models','Textures','Previews']: (OUT/d).mkdir(parents=True,exist_ok=True)
NAME='SM_GrassSparse'; MAT='M_GrassSparse'; TEX='T_GrassSparse_BaseColor'
bpy.ops.wm.read_factory_settings(use_empty=True)
scene=bpy.context.scene;scene.unit_settings.system='METRIC';scene.unit_settings.scale_length=1.0
rng=random.Random(6019)
# Four wide padded colour lanes. Broad gradient only; no noise, opacity, or normal map.
im=bpy.data.images.new(TEX,width=128,height=128,alpha=False)
pixels=[]
for y in range(128):
    t=y/127; t=t*t*(3-2*t)
    for x in range(128):
        lane=x//32; variation=[.94,1.0,1.045,.98][lane]
        base=[.255,.355,.125];tip=[.43,.54,.235]
        pixels.extend([(base[i]*(1-t)+tip[i]*t)*variation for i in range(3)]+[1.0])
im.pixels.foreach_set(pixels);im.filepath_raw=str(OUT/'Textures'/(TEX+'.png'));im.file_format='PNG';im.save();im.pack();im.filepath='//Textures/'+TEX+'.png'
mat=bpy.data.materials.new(MAT);mat.use_nodes=True;mat.diffuse_color=(.18,.25,.065,1)
bsdf=mat.node_tree.nodes.get('Principled BSDF');bsdf.inputs['Roughness'].default_value=.87
bsdf.inputs['Specular IOR Level'].default_value=.18
tex=mat.node_tree.nodes.new('ShaderNodeTexImage');tex.image=im
mat.node_tree.links.new(tex.outputs['Color'],bsdf.inputs['Base Color'])
verts=[];faces=[];uvs=[]
tufts=[(-.44,-.19,7,.29),(.32,-.30,6,.235),(.46,.23,7,.31),(-.19,.36,6,.26),(-.52,.23,4,.18)]
for tx,ty,count,height in tufts:
    phase=rng.uniform(0,math.tau)
    for j in range(count):
        a=phase+j*math.tau/count+rng.uniform(-.18,.18)
        forward=Vector((math.cos(a),math.sin(a),0));side=Vector((-math.sin(a),math.cos(a),0))
        h=height*rng.uniform(.75,1.15);lean=rng.uniform(.10,.22);width=rng.uniform(.033,.049)
        root=Vector((tx+rng.uniform(-.025,.025),ty+rng.uniform(-.025,.025),0))
        lane=rng.randrange(4);idx=len(verts);localuv=[]
        for level,(z,bend,w) in enumerate([(0,0,.28),(.40,.20,1.0),(.77,.59,.66)]):
            center=root+forward*(lean*bend)+Vector((0,0,h*z))
            for k in [-1,0,1]:
                p=center+side*(width*w*k*.5)
                if k==0:p+=forward*(width*.13)
                verts.append(tuple(p));localuv.append(((lane+.20+(k+1)*.30)/4,.035+z*.92))
        verts.append(tuple(root+forward*lean+Vector((0,0,h))));localuv.append(((lane+.5)/4,.97))
        for row in range(2):
            for col in range(2):
                q=row*3+col
                for f in [(q,q+1,q+4),(q,q+4,q+3)]:
                    faces.append(tuple(idx+i for i in f));uvs.append([localuv[i] for i in f])
        for f in [(6,7,9),(7,8,9)]:
            faces.append(tuple(idx+i for i in f));uvs.append([localuv[i] for i in f])
mesh=bpy.data.meshes.new(NAME);mesh.from_pydata(verts,[],faces);mesh.update()
obj=bpy.data.objects.new(NAME,mesh);scene.collection.objects.link(obj);mesh.materials.append(mat)
uv=mesh.uv_layers.new(name='UV0_Color')
for poly,coords in zip(mesh.polygons,uvs):
    for li,co in zip(poly.loop_indices,coords):uv.data[li].uv=co
# Open folded leaves are intentional; a two-sided opaque material displays backs.
bm=bmesh.new();bm.from_mesh(mesh);bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces));bm.to_mesh(mesh);bm.free();mesh.update()
uv=mesh.uv_layers.active
bpy.context.view_layer.objects.active=obj;obj.select_set(True)
bounds=[min(v.co[i] for v in mesh.vertices) for i in range(3)]+[max(v.co[i] for v in mesh.vertices) for i in range(3)]
mesh.calc_loop_triangles();assert all(p.area>1e-10 for p in mesh.polygons)
assert all(math.isfinite(c) for v in mesh.vertices for c in v.co)
assert all(abs(p.normal.length-1)<1e-5 for p in mesh.polygons)
assert all(0<=c<=1 for l in uv.data for c in l.uv)
assert obj.location.length==0 and abs(bounds[2])<1e-8
manifest={'name':NAME,'blender_version':bpy.app.version_string,'seed':6019,'units':'metres','pivot':[0,0,0],'bounds_m':bounds,'size_cm':[(bounds[i+3]-bounds[i])*100 for i in range(3)],'vertices':len(mesh.vertices),'triangles':len(mesh.loop_triangles),'tufts':len(tufts),'blades':sum(t[2] for t in tufts),'materials':[MAT],'texture':TEX,'texture_resolution':[128,128],'uv_channels':1,'two_sided':True,'alpha':False,'collision':False,'degenerate_faces':0,'finite_unit_normals':True,'fbx_forward':'-Y','fbx_up':'Z','passed':True}
(OUT/'model_manifest.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
bpy.ops.export_scene.fbx(filepath=str(OUT/'Models'/(NAME+'.fbx')),use_selection=True,object_types={'MESH'},global_scale=1,apply_unit_scale=True,apply_scale_options='FBX_SCALE_UNITS',axis_forward='-Y',axis_up='Z',use_mesh_modifiers=True,mesh_smooth_type='FACE',use_tspace=True,bake_anim=False,path_mode='COPY',embed_textures=True)
# Studio helpers are excluded from FBX, grouped separately in the editable blend.
preview=bpy.data.collections.new('PREVIEW_ONLY');scene.collection.children.link(preview)
def move_preview(o):
    for c in list(o.users_collection):c.objects.unlink(o)
    preview.objects.link(o)
def flatmat(name,color):
    m=bpy.data.materials.new(name);m.use_nodes=True;n=m.node_tree.nodes.get('Principled BSDF');n.inputs['Base Color'].default_value=(*color,1);n.inputs['Roughness'].default_value=1;return m
bpy.ops.mesh.primitive_plane_add(size=200,location=(0,0,-.004));ground=bpy.context.object;ground.name='Preview_Ground';ground.data.materials.append(flatmat('Preview_Earth',(.18,.145,.105)));move_preview(ground)
scene.render.engine='CYCLES';scene.cycles.samples=32;scene.cycles.use_denoising=True
scene.render.resolution_x=1200;scene.render.resolution_y=900;scene.render.resolution_percentage=100
scene.view_settings.view_transform='AgX'
scene.world=bpy.data.worlds.new('Soft daylight');scene.world.use_nodes=True;scene.world.node_tree.nodes['Background'].inputs[0].default_value=(.65,.72,.78,1);scene.world.node_tree.nodes['Background'].inputs[1].default_value=.5
bpy.ops.object.light_add(type='AREA',location=(-2,-3,5));light=bpy.context.object;light.data.energy=450;light.data.size=4;move_preview(light)
bpy.ops.object.camera_add();cam=bpy.context.object;cam.data.type='ORTHO';scene.camera=cam;move_preview(cam)
def shot(name,loc,scale,target=(0,0,.10)):
    cam.location=loc;cam.rotation_euler=(Vector(target)-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.ortho_scale=scale
    scene.render.filepath=str(OUT/'Previews'/(name+'.png'));bpy.ops.render.render(write_still=True)
shot('01_Hero',(1.5,-2.2,1.45),1.82)
shot('02_Back',(-1.5,2.2,1.45),1.82)
shot('03_Top',(0,0,3),1.82)
shot('04_LowSide',(2.5,0,.6),1.82)
copies=[]
for i in range(4):
    for j in range(4):
        c=obj.copy();c.data=obj.data;preview.objects.link(c);copies.append(c)
        c.location=((i-1.5)*1.30+rng.uniform(-.19,.19),(j-1.5)*1.13+rng.uniform(-.14,.14),0)
        c.rotation_euler.z=rng.uniform(0,math.tau);s=rng.uniform(.82,1.12);c.scale=(s,s,s)
obj.hide_render=True
shot('05_Repeated_16',(4,-6,7),7.2,target=(0,0,0))
for c in copies:bpy.data.objects.remove(c,do_unlink=True)
obj.hide_render=False
cam.location=(1.5,-2.2,1.45);cam.rotation_euler=(Vector((0,0,.1))-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.ortho_scale=1.82
bpy.ops.object.select_all(action='DESELECT');obj.select_set(True);bpy.context.view_layer.objects.active=obj
for area in bpy.context.screen.areas:
    if area.type=='VIEW_3D':area.spaces.active.region_3d.view_perspective='CAMERA'
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'GrassSparse.blend'))
print('GRASS_SPARSE_BUILD_PASS '+json.dumps(manifest))
