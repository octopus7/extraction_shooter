"""Deterministic low-poly exposed roots. Blender 4.5 LTS; meters; no external assets."""
import bpy, bmesh, math, json
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parents[3]
OUT=ROOT/'TunaSweeper/SourceArt/Environment/ForestProps/ExposedRoots'
for folder in ['Models','Textures','Previews']: (OUT/folder).mkdir(parents=True,exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True)
scene=bpy.context.scene;scene.unit_settings.system='METRIC';scene.unit_settings.scale_length=1
verts=[];faces=[]
def root(points,widths,heights):
    base=len(verts);n=8
    for k,p in enumerate(points):
        tangent=Vector(points[min(k+1,len(points)-1)])-Vector(points[max(k-1,0)])
        across=Vector((-tangent.y,tangent.x,0)).normalized()
        for j in range(n):
            a=2*math.pi*j/n
            verts.append(tuple(Vector(p)+across*(math.cos(a)*widths[k])+Vector((0,0,math.sin(a)*heights[k]))))
    for k in range(len(points)-1):
        for j in range(n): faces.append((base+k*n+j,base+k*n+(j+1)%n,base+(k+1)*n+(j+1)%n,base+(k+1)*n+j))
    faces.append(tuple(base+j for j in reversed(range(n))))
    faces.append(tuple(base+(len(points)-1)*n+j for j in range(n)))
specs=[(-125,1.37,.23,-.17),(-66,1.66,.27,.12),(-6,1.48,.235,-.10),(54,1.30,.20,.16),(116,1.58,.235,-.14)]
paths=[]
for i,(angle,length,width,bend) in enumerate(specs):
    a=math.radians(angle);direction=Vector((math.cos(a),math.sin(a),0));side=Vector((-direction.y,direction.x,0))
    radii=[.28,.44,.68,.94,length-.20,length]
    z=[.185,.13,.055,.02,-.025,-.073]
    pts=[tuple(direction*r+side*(bend*math.sin(k/5*2.1))+Vector((0,0,z[k]))) for k,r in enumerate(radii)]
    root(pts,[width,width*.95,width*.77,width*.50,.052,.018],[.24,.20,.135,.084,.042,.014]);paths.append(pts)
    if i in (1,4):
        p=Vector(pts[2]);d=(direction+side*(.62 if i==1 else -.65)).normalized()
        branch=[tuple(p),tuple(p+d*.26+Vector((0,0,-.015))),tuple(p+d*.56+Vector((0,0,-.067))),tuple(p+d*.75+Vector((0,0,-.13)))]
        root(branch,[.115,.095,.045,.013],[.085,.063,.035,.011])
mesh=bpy.data.meshes.new('ExposedRoots_Geometry');mesh.from_pydata(verts,[],faces);mesh.update()
obj=bpy.data.objects.new('SM_ExposedRoots',mesh);scene.collection.objects.link(obj)
bpy.context.view_layer.objects.active=obj;obj.select_set(True)
bm=bmesh.new();bm.from_mesh(mesh);bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces));bmesh.ops.triangulate(bm,faces=list(bm.faces));bm.to_mesh(mesh);bm.free()
# UV1 is a real non-overlapping chart for baked lighting. UV0 samples a small opaque palette.
mesh.uv_layers.new(name='UV0_Palette');mesh.uv_layers.new(name='UV1_Lightmap');mesh.uv_layers.active_index=1
bpy.ops.object.mode_set(mode='EDIT');bpy.ops.mesh.select_all(action='SELECT');bpy.ops.uv.smart_project(angle_limit=1.15,island_margin=.025);bpy.ops.object.mode_set(mode='OBJECT')
colors=[(.37,.265,.155),(.43,.315,.195),(.49,.365,.235),(.53,.405,.272)]
im=bpy.data.images.new('T_ExposedRoots_Palette',width=64,height=64,alpha=False)
im.pixels=[c for y in range(64) for x in range(64) for c in (*colors[min(3,x//16)],1)]
im.filepath_raw=str(OUT/'Textures/T_ExposedRoots_Palette.png');im.file_format='PNG';im.save();im.pack();im.filepath='//Textures/T_ExposedRoots_Palette.png'
mat=bpy.data.materials.new('M_ExposedRoots');mat.use_nodes=True
bs=mat.node_tree.nodes.get('Principled BSDF');bs.inputs['Roughness'].default_value=.9
tex=mat.node_tree.nodes.new('ShaderNodeTexImage');tex.image=im;tex.interpolation='Closest';mat.node_tree.links.new(tex.outputs['Color'],bs.inputs['Base Color'])
mesh.materials.append(mat)
for poly in mesh.polygons:
    color=2 if poly.normal.z>.6 else 1 if poly.normal.z>-.1 else 0
    if poly.normal.z>.9 and poly.center.x>.25:color=3
    for k,li in enumerate(poly.loop_indices):mesh.uv_layers[0].data[li].uv=(color*.25+.09+(k==1)*.06,.4+(k==2)*.08)
mesh.uv_layers.active_index=0
obj['placement']='Origin Z=0 is soil plane. Negative-Z tips and underside are intentionally buried. Open sector on -X.'
obj['collision']='None; use supporting tree collision.'
bpy.context.view_layer.update()
def audit(o):
    m=o.data;m.calc_loop_triangles();bm=bmesh.new();bm.from_mesh(m)
    r={'vertices':len(m.vertices),'triangles':len(m.loop_triangles),'material_slots':len(m.materials),'uv_channels':len(m.uv_layers),
       'degenerate_triangles':sum(t.area<1e-10 for t in m.loop_triangles),'nonmanifold_edges':sum(not e.is_manifold for e in bm.edges),
       'bad_normals':sum(not all(math.isfinite(v) for v in p.normal) or p.normal.length<.99 for p in m.polygons),
       'bounds_m':[min(v.co[i] for v in m.vertices) for i in range(3)]+[max(v.co[i] for v in m.vertices) for i in range(3)],
       'origin':list(o.location),'scale':list(o.scale),'uv_degenerate_triangles':[]}
    for layer in m.uv_layers:
        count=0
        for t in m.loop_triangles:
            a,b,c=[layer.data[i].uv for i in t.loops];ab=b-a;ac=c-a
            count+=abs(ab.x*ac.y-ab.y*ac.x)<1e-10
        r['uv_degenerate_triangles'].append(count)
    assert not r['degenerate_triangles'] and not r['nonmanifold_edges'] and not r['bad_normals'] and not any(r['uv_degenerate_triangles']),r
    assert bm.calc_volume(signed=True)>0;bm.free();return r
report=audit(obj)
bpy.ops.export_scene.fbx(filepath=str(OUT/'Models/SM_ExposedRoots.fbx'),use_selection=True,object_types={'MESH'},axis_forward='-Y',axis_up='Z',apply_unit_scale=True,apply_scale_options='FBX_SCALE_UNITS',use_mesh_modifiers=True,mesh_smooth_type='FACE',use_tspace=True,add_leaf_bones=False,bake_anim=False,path_mode='COPY',embed_textures=True)
# Immediate clean FBX reload; actual UE axes and centimeters audited by import_unreal.py.
bpy.ops.import_scene.fbx(filepath=str(OUT/'Models/SM_ExposedRoots.fbx'))
loaded=bpy.context.selected_objects[0];roundtrip=audit(loaded)
assert max(abs(a-b) for a,b in zip(report['bounds_m'],roundtrip['bounds_m']))<1e-5
assert report['triangles']==roundtrip['triangles'];bpy.data.objects.remove(loaded,do_unlink=True)
report['fbx_reload']=roundtrip;report['passed']=True
(OUT/'blender_validation.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
scene.render.engine='CYCLES';scene.cycles.samples=32;scene.cycles.use_denoising=True
scene.render.resolution_x=1400;scene.render.resolution_y=1000;scene.render.resolution_percentage=100
scene.view_settings.view_transform='Standard'
scene.world=bpy.data.worlds.new('Studio');scene.world.use_nodes=True
scene.world.node_tree.nodes['Background'].inputs[0].default_value=(.48,.53,.58,1);scene.world.node_tree.nodes['Background'].inputs[1].default_value=.5
bpy.ops.object.light_add(type='AREA',location=(1,-3,6));bpy.context.object.data.energy=700;bpy.context.object.data.shape='DISK';bpy.context.object.data.size=5
bpy.ops.object.camera_add(location=(3,-4,3));camera=bpy.context.object;camera.data.type='ORTHO';scene.camera=camera
def camera_at(pos,target,scale):
    camera.location=pos;camera.rotation_euler=(Vector(target)-camera.location).to_track_quat('-Z','Y').to_euler();camera.data.ortho_scale=scale
def render(name,pos,target=(0,0,.1),scale=4.1):
    camera_at(pos,target,scale);scene.render.filepath=str(OUT/'Previews'/name);bpy.ops.render.render(write_still=True)
groundmat=bpy.data.materials.new('Preview_Soil');groundmat.diffuse_color=(.115,.145,.10,1)
bpy.ops.mesh.primitive_plane_add(size=200,location=(0,0,0));ground=bpy.context.object;ground.name='PREVIEW_ONLY_Soil';ground.data.materials.append(groundmat)
render('ExposedRoots_Hero.png',(3,-4,3))
render('ExposedRoots_Back.png',(-3,4,2.6))
render('ExposedRoots_Top.png',(0,0,6))
render('ExposedRoots_Side.png',(3,-4,.9))
# Reuse the original Wood stump as preview-only context, preserving its original texture.
with bpy.data.libraries.load(str(ROOT/'Blender/Wood.blend'),link=False) as (src,dst):dst.objects=[n for n in src.objects if 'StumpA' in n][:1]
stump=next((o for o in dst.objects if o and o.type=='MESH'),None)
if stump:
    scene.collection.objects.link(stump);stump.name='PREVIEW_ONLY_ExistingWoodStump'
    bpy.context.view_layer.update()
    stump.location=(0,0,0);stump.scale*=1.15/max(stump.dimensions)
    bpy.context.view_layer.update()
    bounds=[stump.matrix_world@Vector(v) for v in stump.bound_box]
    stump.location-=Vector(((min(v.x for v in bounds)+max(v.x for v in bounds))/2,(min(v.y for v in bounds)+max(v.y for v in bounds))/2,min(v.z for v in bounds)))
    bpy.context.view_layer.update()
    existing=bpy.data.images.load(str(ROOT/'Blender/textures/T_WoodCommon.png'),check_existing=False);existing.pack()
    contextmat=bpy.data.materials.new('PREVIEW_ONLY_ExistingWood');contextmat.use_nodes=True
    node=contextmat.node_tree.nodes.new('ShaderNodeTexImage');node.image=existing
    contextmat.node_tree.links.new(node.outputs['Color'],contextmat.node_tree.nodes.get('Principled BSDF').inputs['Base Color'])
    for slot in stump.material_slots:slot.material=contextmat
    render('ExposedRoots_WithExistingStump.png',(3,-4,3),target=(0,0,.35))
else:raise RuntimeError('Expected existing StumpA for composition preview')
copies=[]
for i,(x,y,yaw,s) in enumerate([(-3,-2,-.4,.85),(0,-2,.7,1),(3,-2,2,.9),(-3,1,1.4,1.05),(0,1,2.8,.8),(3,1,-1.2,1)]):
    for original in [obj,stump]:
        copy=original.copy();copy.data=original.data;scene.collection.objects.link(copy);copy.location=Vector((x,y,0))+original.location;copy.rotation_euler.z+=yaw;copy.scale*=s;copies.append(copy)
obj.hide_render=True;stump.hide_render=True
render('ExposedRoots_Repetition.png',(9,-12,13),target=(0,-.5,0),scale=13)
for copy in copies:bpy.data.objects.remove(copy,do_unlink=True)
obj.hide_render=False;stump.hide_render=True
camera_at((3,-4,3),(0,0,.1),4.1)
bpy.ops.object.select_all(action='DESELECT');obj.select_set(True);bpy.context.view_layer.objects.active=obj
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'ExposedRoots.blend'))
