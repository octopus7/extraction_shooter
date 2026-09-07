"""Deterministic curved grass; Blender 4.5 LTS, meters, no external dependencies.
blender -b --factory-startup -t 4 --python Tools/ForestProps/GrassLongCurved/build_grass.py
"""
import bpy, bmesh, math, random, json
from pathlib import Path
from mathutils import Vector

ROOT=Path(__file__).resolve().parents[3]
OUT=ROOT/'TunaSweeper/SourceArt/Environment/ForestProps/GrassLongCurved'
for folder in ['Models','Textures','Previews']: (OUT/folder).mkdir(parents=True,exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True)
scene=bpy.context.scene
scene.unit_settings.system='METRIC'; scene.unit_settings.scale_length=1.0
scene.render.engine='CYCLES'; scene.cycles.samples=24; scene.cycles.use_denoising=True
scene.render.resolution_x=1100; scene.render.resolution_y=1000; scene.render.resolution_percentage=100
scene.view_settings.view_transform='Standard'
scene.world=bpy.data.worlds.new('Studio');scene.world.use_nodes=True
scene.world.node_tree.nodes['Background'].inputs[0].default_value=(.28,.31,.26,1)
scene.world.node_tree.nodes['Background'].inputs[1].default_value=.65

# Four broad green strips, no noise, alpha or normal-map microtexture.
palette=[((.22,.28,.115),(.40,.49,.225)),((.19,.255,.105),(.35,.455,.205)),((.235,.295,.12),(.43,.50,.255)),((.20,.285,.13),(.365,.475,.245))]
im=bpy.data.images.new('T_GrassLongCurved_BaseColor',width=128,height=128,alpha=False)
pixels=[]
for y in range(128):
    t=y/127; q=min(1,t*1.7); q=q*q*(3-2*q)
    for x in range(128):
        a,b=palette[x//32]; pixels.extend([a[i]*(1-q)+b[i]*q for i in range(3)]+[1])
im.pixels.foreach_set(pixels); im.filepath_raw=str(OUT/'Textures/T_GrassLongCurved_BaseColor.png');im.file_format='PNG';im.save(); im.pack();im.filepath='//Textures/T_GrassLongCurved_BaseColor.png'
mat=bpy.data.materials.new('M_GrassLongCurved');mat.use_nodes=True
bsdf=mat.node_tree.nodes.get('Principled BSDF');bsdf.inputs['Roughness'].default_value=.88;bsdf.inputs['Specular IOR Level'].default_value=.25
tex=mat.node_tree.nodes.new('ShaderNodeTexImage');tex.image=im;mat.node_tree.links.new(tex.outputs['Color'],bsdf.inputs['Base Color'])
mat.diffuse_color=(.12,.19,.05,1);mat.use_backface_culling=False
rng=random.Random(6072026)
specs=[]
for group,(cx,cy,count,offset) in enumerate([(-.14,-.045,11,.0),(.155,.045,9,.75),(-.02,.17,6,1.8)]):
    for j in range(count):
        angle=offset+j*2.399963+rng.uniform(-.2,.2)
        outer=j%3!=0
        specs.append(dict(base=[cx+rng.uniform(-.045,.045),cy+rng.uniform(-.04,.04),0],angle=angle,
                          height=rng.uniform(.79,1.02) if not outer else rng.uniform(.60,.88),
                          reach=rng.uniform(.34,.55) if outer else rng.uniform(.19,.32),
                          droop=rng.uniform(.21,.36) if outer else rng.uniform(.03,.14),
                          width=rng.uniform(.049,.074),twist=rng.uniform(-.28,.28),strip=(j+group)%4))

def make_mesh(name,indices,rings,width_scale=1):
    verts=[];faces=[];uvs=[]
    for index in indices:
        s=specs[index];a=s['angle'];d=Vector((math.cos(a),math.sin(a),0));side=Vector((-d.y,d.x,0));base=Vector(s['base'])
        p=[base,base+d*.025+Vector((0,0,s['height']*.55)),base+d*s['reach']*.50+Vector((0,0,s['height']*1.17)),base+d*s['reach']+Vector((0,0,s['height']*(1-s['droop'])))]
        first=len(verts)
        for k in range(rings):
            t=1-(1-k/rings)**1.5;u=1-t;c=u**3*p[0]+3*u*u*t*p[1]+3*u*t*t*p[2]+t**3*p[3]
            tangent=(3*u*u*(p[1]-p[0])+6*u*t*(p[2]-p[1])+3*t*t*(p[3]-p[2])).normalized()
            cross=(side+Vector((0,0,s['twist']*t))).normalized();n=cross.cross(tangent).normalized()
            w=s['width']*(.38+.9*math.sin(math.pi*t)**.65)*(1-.55*t)*width_scale
            for col in range(3):
                v=c+cross*((col-1)*w*.5)+(n*w*.13*math.sin(math.pi*t) if col==1 else Vector((0,0,0)))
                v.z*=.85
                verts.append(tuple(v));uvs.append(((s['strip']+.18+.64*col/2)/4,.06+.88*t))
        tip=len(verts);p[3].z*=.85;verts.append(tuple(p[3]));uvs.append(((s['strip']+.5)/4,.94))
        for k in range(rings-1):
            for col in range(2):
                q=first+k*3+col
                faces.extend([(q,q+1,q+4),(q,q+4,q+3)])
        q=first+(rings-1)*3;faces.extend([(q,q+1,tip),(q+1,q+2,tip)])
    mesh=bpy.data.meshes.new(name);mesh.from_pydata(verts,[],faces);mesh.update()
    obj=bpy.data.objects.new(name,mesh);scene.collection.objects.link(obj);mesh.materials.append(mat)
    uv=mesh.uv_layers.new(name='UVMap')
    for poly in mesh.polygons:
        poly.use_smooth=True
        for li in poly.loop_indices:uv.data[li].uv=uvs[mesh.loops[li].vertex_index]
    return obj

lod0=make_mesh('SM_GrassLongCurved',range(26),10)
# Preserve outline-defining leaves in both lower LODs.
anchors=set()
for axis in range(3):
    for fn in [min,max]:
        anchors.add(fn(lod0.data.vertices,key=lambda v:v.co[axis]).index//31)
order=sorted(anchors)+[i for i in [0,11,20,2,13,22,5,17,25,8,15,23,3,6,9,12,16,19,21,24,1,4,7,10,14,18] if i not in anchors]
lods=[lod0,make_mesh('SM_GrassLongCurved_LOD1',sorted(order[:17]),6,1.08),
      make_mesh('SM_GrassLongCurved_LOD2',sorted(order[:10]),4,1.2)]
for obj,idx in zip(lods,[list(range(26)),sorted(order[:17]),sorted(order[:10])]):obj['blade_indices']=idx

def audit(obj):
    mesh=obj.data;mesh.calc_loop_triangles();bm=bmesh.new();bm.from_mesh(mesh)
    bad_edges=sum(1 for e in bm.edges if len(e.link_faces)>2)
    boundary=sum(1 for e in bm.edges if len(e.link_faces)==1)
    bm.free()
    assert bad_edges==0
    assert all(t.area>1e-9 for t in mesh.loop_triangles)
    assert all(abs(p.normal.length-1)<1e-5 for p in mesh.polygons)
    assert all(math.isfinite(x) for v in mesh.vertices for x in v.co)
    assert all(0<=x<=1 for u in mesh.uv_layers.active.data for x in u.uv)
    for tri in mesh.loop_triangles:
        a,b,c=[mesh.uv_layers.active.data[i].uv for i in tri.loops]
        assert abs((b.x-a.x)*(c.y-a.y)-(b.y-a.y)*(c.x-a.x))>1e-9
    bounds=[min(v.co[i] for v in mesh.vertices) for i in range(3)]+[max(v.co[i] for v in mesh.vertices) for i in range(3)]
    assert abs(bounds[2])<1e-8 and obj.location.length==0
    return {'name':obj.name,'vertices':len(mesh.vertices),'triangles':len(mesh.loop_triangles),'bounds_m':bounds,'dimensions_cm':[(bounds[i+3]-bounds[i])*100 for i in range(3)],'material_slots':1,'uv_channels':1,'intentional_open_boundary_edges':boundary,'nonmanifold_junction_edges':bad_edges,'degenerate_triangles':0,'passed':True}

manifest={'seed':6072026,'blender_version':bpy.app.version_string,'units':'Blender meters; UE centimeters','pivot':'root center on ground z=0','material':'M_GrassLongCurved','opaque':True,'two_sided':True,'alpha_used':False,'collision':'none','lods':[]}
for obj in lods:
    manifest['lods'].append(audit(obj))
    bpy.ops.object.select_all(action='DESELECT');obj.select_set(True);bpy.context.view_layer.objects.active=obj
    bpy.ops.export_scene.fbx(filepath=str(OUT/'Models'/(obj.name+'.fbx')),use_selection=True,object_types={'MESH'},axis_forward='-Y',axis_up='Z',apply_unit_scale=True,apply_scale_options='FBX_SCALE_ALL',bake_space_transform=False,use_mesh_modifiers=True,mesh_smooth_type='OFF',use_tspace=True,path_mode='COPY',embed_textures=True,add_leaf_bones=False,bake_anim=False)
    obj.select_set(False)
for obj in lods[1:]:obj.hide_render=True;obj.hide_set(True)
(OUT/'model_manifest.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')

preview=bpy.data.collections.new('PREVIEW_ONLY');scene.collection.children.link(preview)
def move_preview(obj):
    for c in list(obj.users_collection):c.objects.unlink(obj)
    preview.objects.link(obj)
bpy.ops.mesh.primitive_plane_add(size=200,location=(0,0,-.004));ground=bpy.context.object;ground.name='Preview_ground';move_preview(ground)
gm=bpy.data.materials.new('Preview_ground');gm.diffuse_color=(.20,.22,.16,1);ground.data.materials.append(gm)
for name,loc,power,size in [('Key',(-3,-4,6),500,4),('Fill',(4,1,4),260,4)]:
    data=bpy.data.lights.new(name,'AREA');data.energy=power;data.size=size
    obj=bpy.data.objects.new(name,data);preview.objects.link(obj);obj.location=loc;obj.rotation_euler=(-obj.location).to_track_quat('-Z','Y').to_euler()
data=bpy.data.cameras.new('ReviewCamera');cam=bpy.data.objects.new('ReviewCamera',data);preview.objects.link(cam);scene.camera=cam;data.type='ORTHO'
def render(name,loc,target,scale):
    cam.location=loc;cam.rotation_euler=(Vector(target)-cam.location).to_track_quat('-Z','Y').to_euler();data.ortho_scale=scale
    scene.render.filepath=str(OUT/'Previews'/(name+'.png'));bpy.ops.render.render(write_still=True)
render('Hero',(1.5,-2.7,1.8),(0,0,.38),1.65)
lods[0].select_set(True);bpy.context.view_layer.objects.active=lods[0]
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'GrassLongCurved.blend'))
render('Front',(0,-3,.7),(0,0,.40),1.65)
render('Back',(-1.6,2.7,1.6),(0,0,.38),1.65)
render('Top',(0,0,4),(0,0,0),1.65)
for obj in lods:obj.hide_render=True
for y in range(4):
    for x in range(4):
        obj=lods[0].copy();obj.data=lods[0].data;preview.objects.link(obj);obj.hide_render=False
        obj.location=((x-1.5)*1.05+rng.uniform(-.16,.16),(y-1.5)*1.02+rng.uniform(-.16,.16),0)
        obj.rotation_euler.z=rng.uniform(0,math.tau);s=rng.uniform(.82,1.13);obj.scale=(s,s,s)
render('Repeated',(4,-6,6),(0,0,.1),6.5)
print('GRASS_BUILD_PASSED '+json.dumps(manifest['lods']))
