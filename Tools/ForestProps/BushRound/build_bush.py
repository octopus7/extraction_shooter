"""Deterministic, closed-leaf low bush. Blender 4.5 LTS, metres, no external packages."""
import bpy,bmesh,math,random,json
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parents[3]
OUT=ROOT/'TunaSweeper/SourceArt/Environment/ForestProps/BushRound'
for d in ['Models','Textures','Previews']:(OUT/d).mkdir(parents=True,exist_ok=True)
random.seed(90631)
bpy.ops.wm.read_factory_settings(use_empty=True)
s=bpy.context.scene;s.unit_settings.system='METRIC';s.unit_settings.scale_length=1
s.render.engine='CYCLES';s.cycles.samples=24;s.cycles.use_denoising=True
s.render.resolution_x=1200;s.render.resolution_y=1000;s.render.resolution_percentage=100
s.view_settings.view_transform='AgX'
s.world=bpy.data.worlds.new('World');s.world.use_nodes=True
s.world.node_tree.nodes['Background'].inputs[0].default_value=(.55,.62,.70,1)
s.world.node_tree.nodes['Background'].inputs[1].default_value=.35
# sRGB swatches: broad colour blocks, no detail noise, alpha entirely one.
palette=[(70,104,44),(82,116,50),(93,125,57),(112,89,60)]
im=bpy.data.images.new('T_BushRound_Palette',width=128,height=128,alpha=False)
pix=[]
for y in range(128):
    for x in range(128):pix.extend([v/255 for v in palette[x//32]]+[1])
im.pixels=pix;im.filepath_raw=str(OUT/'Textures/T_BushRound_Palette.png');im.file_format='PNG';im.save();im.pack();im.filepath='//Textures/T_BushRound_Palette.png'
mat=bpy.data.materials.new('M_BushRound');mat.use_nodes=True
bs=mat.node_tree.nodes.get('Principled BSDF');bs.inputs['Roughness'].default_value=.86;bs.inputs['Specular IOR Level'].default_value=.18
n=mat.node_tree.nodes.new('ShaderNodeTexImage');n.image=im
mat.node_tree.links.new(n.outputs['Color'],bs.inputs['Base Color'])
verts=[];faces=[];tiles=[]
def part(v,f,tile):
    offset=len(verts);verts.extend(v);faces.extend([tuple(offset+i for i in face) for face in f]);tiles.extend([tile]*len(f))
def leaf(center,direction,length,width,tile,roll):
    # Oval octagonal lamina; shallow upper and lower ridge. Closed 16-triangle solid.
    c=Vector(center);d=Vector(direction).normalized()
    q=d.to_track_quat('Y','Z');q=q@__import__('mathutils').Quaternion((0,1,0),roll)
    ring=[(0,-.5,0),(.38,-.35,0),(.5,0,0),(.32,.42,0),(0,.5,0),(-.32,.42,0),(-.5,0,0),(-.38,-.35,0)]
    local=[Vector((x*width,y*length,z)) for x,y,z in ring]
    local += [Vector((0,0,.014)),Vector((0,0,-.008))]
    f=[]
    for j in range(8):f.extend([(8,j,(j+1)%8),(9,(j+1)%8,j)])
    part([c+q@v for v in local],f,tile)
def branch(a,b,r):
    a=Vector(a);b=Vector(b);q=(b-a).to_track_quat('Z','Y')
    v=[p+q@Vector((math.cos(i*math.tau/6)*rad,math.sin(i*math.tau/6)*rad,0)) for p,rad in [(a,r),(b,r*.50)] for i in range(6)]
    f=[tuple(reversed(range(6))),tuple(range(6,12))]+[(i,(i+1)%6,(i+1)%6+6,i+6) for i in range(6)]
    part(v,f,3)
clusters=[((-.31,-.22,.31),(.29,.26,.24),0),((.26,-.25,.35),(.30,.25,.27),1),((.43,.10,.30),(.23,.23,.22),1),((.09,.31,.39),(.31,.27,.27),0),((-.34,.27,.34),(.29,.25,.23),1),((-.12,.02,.48),(.32,.28,.28),1),((.23,.04,.46),(.25,.24,.26),2)]
for k,(center,radius,tile) in enumerate(clusters):
    branch((0,0,0),(center[0],center[1],center[2]),.029 if k<5 else .034)
    # Compact core closes sightlines through the crown without alpha planes.
    bm=bmesh.new();bmesh.ops.create_icosphere(bm,subdivisions=1,radius=1)
    bm.verts.ensure_lookup_table();bm.verts.index_update()
    part([Vector(center)+Vector((v.co.x*radius[0]*.85,v.co.y*radius[1]*.85,v.co.z*radius[2]*.85)) for v in bm.verts],[tuple(v.index for v in f.verts) for f in bm.faces],tile);bm.free()
    # Golden-angle samples prevent radial rosettes and repeated horizontal bands.
    for j in range(19):
        z=-.50+1.48*(j+.5)/19;theta=j*2.39996+k*.91
        r=math.sqrt(1-z*z);normal=Vector((r*math.cos(theta),r*math.sin(theta),z))
        pos=Vector(center)+Vector((normal.x*radius[0],normal.y*radius[1],normal.z*radius[2]))
        # Growth points out and upward. Large leaves make the silhouette legible.
        direction=Vector((normal.x,normal.y,.38+normal.z*.5))
        leaf(pos,direction,random.uniform(.23,.32),random.uniform(.15,.21),tile,random.uniform(-.8,.8))
mesh=bpy.data.meshes.new('BushRound_Geometry');mesh.from_pydata(verts,[],faces);mesh.update()
obj=bpy.data.objects.new('SM_BushRound',mesh);s.collection.objects.link(obj);obj.data.materials.append(mat)
bm=bmesh.new();bm.from_mesh(mesh);bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces));bm.to_mesh(mesh);bm.free()
uv=mesh.uv_layers.new(name='UV0_Palette')
for p,t in zip(mesh.polygons,tiles):
    for j,li in enumerate(p.loop_indices):
        a=math.tau*j/len(p.loop_indices)
        uv.data[li].uv=((t+.5+math.cos(a)*.22)/4,.5+math.sin(a)*.25)
bpy.context.view_layer.objects.active=obj;obj.select_set(True)
mesh.uv_layers.new(name='UV1_Lightmap');mesh.uv_layers.active_index=1
bpy.ops.object.mode_set(mode='EDIT');bpy.ops.mesh.select_all(action='SELECT');bpy.ops.uv.smart_project(island_margin=.015);bpy.ops.object.mode_set(mode='OBJECT')
mesh.uv_layers.active_index=0;mesh.uv_layers[0].active_render=True
tri=obj.modifiers.new('Explicit triangles','TRIANGULATE');bpy.ops.object.modifier_apply(modifier=tri.name)
mesh=obj.data
ground=min(v.co.z for v in mesh.vertices)
for v in mesh.vertices:v.co.z=(v.co.z-ground)*.80
mesh.update()
def audit(o):
    m=o.data;m.calc_loop_triangles();bm=bmesh.new();bm.from_mesh(m)
    bounds=[min((o.matrix_world@v.co)[i] for v in m.vertices) for i in range(3)]+[max((o.matrix_world@v.co)[i] for v in m.vertices) for i in range(3)]
    r={'triangles':len(m.loop_triangles),'vertices':len(m.vertices),'bounds_m':bounds,'dimensions_m':[bounds[i+3]-bounds[i] for i in range(3)],'nonmanifold_edges':sum(not e.is_manifold for e in bm.edges),'degenerate_faces':sum(f.calc_area()<1e-10 for f in bm.faces),'uv_layers':len(m.uv_layers),'material_slots':len(m.materials),'normal_errors':sum(not all(math.isfinite(v) for v in p.normal) or p.normal.length<.99 for p in m.polygons)}
    bm.free()
    r['uv_out_of_range']=sum(any(not math.isfinite(v) or v<0 or v>1 for v in l.uv) for u in m.uv_layers for l in u.data)
    r['degenerate_uv_triangles']=sum(abs((u.data[t.loops[1]].uv-u.data[t.loops[0]].uv).cross(u.data[t.loops[2]].uv-u.data[t.loops[0]].uv))<1e-10 for u in m.uv_layers for t in m.loop_triangles)
    assert not any(r[k] for k in ['nonmanifold_edges','degenerate_faces','normal_errors','uv_out_of_range','degenerate_uv_triangles']),r
    assert abs(bounds[2])<1e-7 and len(m.materials)==1 and len(m.uv_layers)==2
    return r
report=audit(obj)
report.update(seed=90631,pivot_m=list(obj.location),unit='metres',fbx_axes={'forward':'-Y','up':'Z'},material='M_BushRound',texture='T_BushRound_Palette',collision='none',alpha='opaque',leaf_geometry='closed oval octagonal lamina, 16 triangles per leaf',leaf_count=133)
bpy.ops.export_scene.fbx(filepath=str(OUT/'Models/SM_BushRound.fbx'),use_selection=True,object_types={'MESH'},axis_forward='-Y',axis_up='Z',apply_unit_scale=True,apply_scale_options='FBX_SCALE_ALL',bake_space_transform=False,use_triangles=True,mesh_smooth_type='FACE',bake_anim=False,path_mode='COPY',embed_textures=True)
# Reload the actual FBX, compare transformed metric bounds, topology and UVs.
bpy.ops.object.select_all(action='DESELECT');bpy.ops.import_scene.fbx(filepath=str(OUT/'Models/SM_BushRound.fbx'))
reload=[o for o in bpy.context.selected_objects if o.type=='MESH'];assert len(reload)==1
fbx=audit(reload[0]);assert fbx['triangles']==report['triangles']
assert max(abs(a-b) for a,b in zip(fbx['bounds_m'],report['bounds_m']))<1e-5
report['fbx_reload']=fbx
for o in reload:bpy.data.objects.remove(o,do_unlink=True)
(OUT/'model_manifest.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
obj.select_set(True);bpy.context.view_layer.objects.active=obj
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'BushRound.blend'))
# Presentation scene is intentionally added after source save / FBX export.
bpy.ops.mesh.primitive_plane_add(size=200,location=(0,0,-.004));floor=bpy.context.object
fm=bpy.data.materials.new('Preview_Ground');fm.diffuse_color=(.14,.155,.12,1);floor.data.materials.append(fm)
bpy.ops.object.light_add(type='AREA',location=(-3,-4,6));bpy.context.object.data.energy=650;bpy.context.object.data.size=5
bpy.ops.object.light_add(type='AREA',location=(3,2,4));bpy.context.object.data.energy=350;bpy.context.object.data.size=4
bpy.ops.object.camera_add();cam=bpy.context.object;cam.data.type='ORTHO';s.camera=cam
def render(name,loc,target,scale):
    cam.location=loc;cam.rotation_euler=(Vector(target)-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.ortho_scale=scale
    s.render.filepath=str(OUT/'Previews'/f'{name}.png');bpy.ops.render.render(write_still=True)
render('BushRound_Hero',(1.7,-2.3,1.6),(0,0,.36),2.0)
render('BushRound_Back',(-1.7,2.3,1.35),(0,0,.36),2.0)
render('BushRound_Front',(0,-3,.40),(0,0,.40),1.85)
render('BushRound_Top',(0,-.001,4),(0,0,.3),1.9)
obj.hide_render=True
for i in range(4):
    for j in range(4):
        dup=bpy.data.objects.new(f'PreviewOnly_{i}_{j}',obj.data);s.collection.objects.link(dup)
        dup.location=((i-1.5)*1.32+random.uniform(-.18,.18),(j-1.5)*1.32+random.uniform(-.18,.18),0)
        dup.rotation_euler.z=random.uniform(0,math.tau);dup.scale=(random.uniform(.83,1.12),)*3
s.render.resolution_x=1500;s.render.resolution_y=1100
render('BushRound_Repeated',(5,-7,8),(0,0,.15),8.2)
print('BUSHROUND_SOURCE_PASS '+json.dumps(report))
