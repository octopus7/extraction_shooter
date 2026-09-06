"""Deterministic, opaque low-poly leaf litter. Blender 4.5 LTS, meters.
Run blender -b --factory-startup --python Tools/ForestProps/LeafLitter/build_leaf_litter.py
"""
from pathlib import Path
import bpy, bmesh, json, math, random
from mathutils import Vector
ROOT=Path(__file__).resolve().parents[3]
OUT=ROOT/'TunaSweeper/SourceArt/Environment/ForestProps/LeafLitter'
for d in ['Models','Textures','Previews']: (OUT/d).mkdir(parents=True,exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True)
sc=bpy.context.scene;sc.unit_settings.system='METRIC';sc.unit_settings.scale_length=1
rng=random.Random(90682)
# A tiny swatch atlas, no noise or alpha. All faces sample safely inside one swatch.
palette=['66543C','786347','82724E','636344','74734D','90805B','534A36','80704F']
img=bpy.data.images.new('T_LeafLitter_Palette',width=128,height=32,alpha=False)
pixels=[]
for y in range(32):
    for x in range(128):
        c=palette[x//16];pixels.extend([int(c[i:i+2],16)/255 for i in (0,2,4)]+[1])
img.pixels=pixels;img.filepath_raw=str(OUT/'Textures/T_LeafLitter_Palette.png');img.file_format='PNG';img.save();img.pack()
img.filepath='//Textures/T_LeafLitter_Palette.png'
mat=bpy.data.materials.new('M_LeafLitter');mat.use_nodes=True
bs=mat.node_tree.nodes.get('Principled BSDF');bs.inputs['Roughness'].default_value=.92
bs.inputs['Specular IOR Level'].default_value=.18
tex=mat.node_tree.nodes.new('ShaderNodeTexImage');tex.image=img
mat.node_tree.links.new(tex.outputs['Color'],bs.inputs['Base Color'])
verts=[];faces=[];face_tiles=[]
def leaf(x,y,z,length,width,angle,tile,curl):
    # Six-point broad oval rim and a raised longitudinal fold. No veins/stems.
    # Two faceted halves share ridge vertices. A 1.5 mm underside closes every leaf.
    points=[(-.52,0,.10),(-.24,-.48,0),(.23,-.43,.06),(.52,0,.26),(.20,.46,.03),(-.26,.42,0),(-.16,0,.72),(.20,0,.66)]
    start=len(verts);ca=math.cos(angle);sa=math.sin(angle)
    for bottom in [False,True]:
        for a,b,c in points:
            xx=a*length;yy=b*width
            verts.append((x+xx*ca-yy*sa,y+xx*sa+yy*ca,z+c*curl-(.0015 if bottom else 0)))
    top=[(0,1,6),(1,2,7),(1,7,6),(2,3,7),(3,4,7),(4,5,6),(4,6,7),(5,0,6)]
    for f in top:
        faces.append(tuple(start+i for i in f));face_tiles.append(tile)
    for f in top:
        faces.append(tuple(start+8+i for i in reversed(f)));face_tiles.append(tile)
    for i in range(6):
        j=(i+1)%6
        faces.extend([(start+i,start+8+i,start+8+j),(start+i,start+8+j,start+j)])
        face_tiles.extend([tile,tile])
# Four offset lobes interlock into an irregular carpet, with open soil at the edge.
centers=[(-.30,-.13,.34,.24),(.18,-.13,.43,.27),(-.14,.23,.35,.27),(.37,.20,.26,.22)]
placements=[]
for ci,(cx,cy,rx,ry) in enumerate(centers):
    for k in range(13):
        a=k*2.399963+ci*.8;r=math.sqrt((k+.4)/13)
        x=cx+math.cos(a)*rx*r;y=cy+math.sin(a)*ry*r
        length=rng.uniform(.23,.34);width=length*rng.uniform(.50,.76)
        z=.006+(1-r)*.017+ci*.003+rng.uniform(0,.008)
        # Color follows spatial patches, with controlled low-contrast variation.
        tile=([0,1,2,7] if x<.03 else [2,3,4,5])[rng.randrange(4)]
        placements.append((x,y,z,length,width,rng.uniform(0,math.tau),tile,rng.uniform(.016,.033)))
# Sparse, smaller perimeter leaves are part of this single mesh, not separate props.
for i,(x,y,a) in enumerate([(-.78,-.12,.4),(-.58,-.46,1.1),(-.11,-.51,2.8),(.54,-.42,2),(.83,.04,1.3),(.64,.48,.2),(.15,.64,2.5),(-.42,.53,.7)]):
    placements.append((x,y,.004,rng.uniform(.15,.22),.10,a,[1,3,4,7][i%4],.017))
for p in placements:leaf(*p)
# Guarantee the minimum is exactly the placement plane, origin remains center XY.
floor=min(v[2] for v in verts);verts=[(x,y,z-floor) for x,y,z in verts]
mesh=bpy.data.meshes.new('SM_LeafLitter');mesh.from_pydata(verts,[],faces);mesh.update()
obj=bpy.data.objects.new('SM_LeafLitter',mesh);sc.collection.objects.link(obj);mesh.materials.append(mat)
uv=mesh.uv_layers.new(name='UVMap')
for p,tile in zip(mesh.polygons,face_tiles):
    # Nonzero UV triangles, padded by >5 pixels from palette borders.
    for j,li in enumerate(p.loop_indices):uv.data[li].uv=((tile+.36+(j==1)*.25)/8,.40+(j==2)*.20)
bm=bmesh.new();bm.from_mesh(mesh);bmesh.ops.recalc_face_normals(bm,faces=bm.faces);bm.to_mesh(mesh);bm.free()
mesh.calc_loop_triangles()
bm=bmesh.new();bm.from_mesh(mesh)
degenerate=sum(f.calc_area()<1e-10 for f in bm.faces)
nonmanifold=sum(not e.is_manifold for e in bm.edges)
bm.free()
assert degenerate==0 and nonmanifold==0
assert len(mesh.materials)==1 and len(mesh.uv_layers)==1
assert all(math.isfinite(c) for v in mesh.vertices for c in v.co)
bounds=[min(v.co[i] for v in mesh.vertices) for i in range(3)]+[max(v.co[i] for v in mesh.vertices) for i in range(3)]
manifest={'name':obj.name,'seed':90682,'leaf_count':len(placements),'triangles':len(mesh.loop_triangles),'vertices':len(mesh.vertices),'bounds_m':bounds,'dimensions_cm':[(bounds[i+3]-bounds[i])*100 for i in range(3)],'pivot_m':[0,0,0],'material_slots':1,'uv_channels':1,'degenerate_faces':degenerate,'nonmanifold_edges':nonmanifold,'closed_leaf_shells':True,'texture':'Textures/T_LeafLitter_Palette.png','palette_srgb_hex':palette,'alpha':False,'collision':False,'passed':True}
(OUT/'model_manifest.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
bpy.context.view_layer.objects.active=obj;obj.select_set(True)
bpy.ops.export_scene.fbx(filepath=str(OUT/'Models/SM_LeafLitter.fbx'),use_selection=True,object_types={'MESH'},apply_unit_scale=True,apply_scale_options='FBX_SCALE_NONE',axis_forward='-Y',axis_up='Z',use_mesh_modifiers=True,mesh_smooth_type='FACE',use_triangles=True,add_leaf_bones=False,bake_anim=False,path_mode='COPY',embed_textures=True)
# Studio belongs only to the .blend; only the named mesh is exported.
sc.render.engine='CYCLES';sc.cycles.samples=48;sc.cycles.use_denoising=True
sc.render.resolution_x=1200;sc.render.resolution_y=900;sc.render.resolution_percentage=100
sc.view_settings.view_transform='AgX'
sc.world=bpy.data.worlds.new('LeafLitterWorld');sc.world.use_nodes=True
sc.world.node_tree.nodes['Background'].inputs[0].default_value=(.58,.65,.72,1)
sc.world.node_tree.nodes['Background'].inputs[1].default_value=.4
bpy.ops.mesh.primitive_plane_add(size=200,location=(0,0,-.003));ground=bpy.context.object;ground.name='PREVIEW_Ground'
gm=bpy.data.materials.new('PREVIEW_Soil');gm.use_nodes=True
gm.node_tree.nodes['Principled BSDF'].inputs['Base Color'].default_value=(.092,.106,.069,1)
gm.node_tree.nodes['Principled BSDF'].inputs['Roughness'].default_value=1;ground.data.materials.append(gm)
bpy.ops.object.light_add(type='AREA',location=(-1.5,-2,4));light=bpy.context.object;light.data.energy=400;light.data.size=4
bpy.ops.object.camera_add();cam=bpy.context.object;sc.camera=cam;cam.data.type='ORTHO'
def render(name,location,target,scale):
    cam.location=location;cam.rotation_euler=(Vector(target)-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.ortho_scale=scale
    sc.render.filepath=str(OUT/'Previews'/name);bpy.ops.render.render(write_still=True)
render('LeafLitter_Hero.png',(1.7,-2.2,2.7),(0,0,.02),2.20)
render('LeafLitter_Top.png',(0,0,4),(0,0,0),2.20)
render('LeafLitter_Reverse.png',(-1.8,2.3,1.4),(0,0,.02),2.20)
render('LeafLitter_Profile.png',(0,-3,.42),(0,0,.02),2.20)
# Nine linked copies; mixed scales/yaws expose repeating silhouette from game distance.
for row in range(3):
    for col in range(3):
        if row==1 and col==1:continue
        cp=obj.copy();cp.data=obj.data;sc.collection.objects.link(cp);cp.name='PREVIEW_Repeat'
        cp.location=((col-1)*1.5+rng.uniform(-.2,.2),(row-1)*1.25+rng.uniform(-.15,.15),0)
        cp.rotation_euler.z=rng.uniform(0,math.tau);s=rng.uniform(.8,1.15);cp.scale=(s,s,s)
render('LeafLitter_Repeat.png',(4,-5,7),(0,0,0),6.7)
for o in list(sc.objects):
    if o.name.startswith('PREVIEW_Repeat'):bpy.data.objects.remove(o,do_unlink=True)
cam.location=(1.7,-2.2,2.7);cam.rotation_euler=(Vector((0,0,.02))-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.ortho_scale=2.2
bpy.ops.object.select_all(action='DESELECT');obj.select_set(True);bpy.context.view_layer.objects.active=obj
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'LeafLitter.blend'))
print('LEAFLITTER_BUILD_PASSED',json.dumps(manifest))
