"""Build the reference-derived editable model, FBX exports and comparable renders."""
import bpy, bmesh, math, json
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'TunaSweeper/SourceArt/Props/NoiseEmitter'
bpy.ops.wm.open_mainfile(filepath=str(OUT/'original_reconstruction.blend'))
scene=bpy.context.scene
for ob in list(scene.objects):
    if ob.type=='MESH' and ob.name!='StudioFloor': bpy.data.objects.remove(ob,do_unlink=True)
def mat(name,color):
    m=bpy.data.materials.new(name); m.diffuse_color=(*color,1); m.use_nodes=True
    bs=m.node_tree.nodes.get('Principled BSDF'); bs.inputs['Base Color'].default_value=m.diffuse_color; bs.inputs['Roughness'].default_value=.72
    return m
mats=[mat('M_Noise_Ivory',(.72,.69,.57)),mat('M_Noise_Red',(.58,.032,.025)),mat('M_Noise_Interior',(.18,.012,.009)),mat('M_Noise_Charcoal',(.032,.04,.044))]
def cube(name,center,extent,mi,bevel):
    bpy.ops.mesh.primitive_cube_add(size=2,location=center); ob=bpy.context.object; ob.name=name; ob.scale=extent
    bpy.ops.object.transform_apply(location=False,rotation=False,scale=True); ob.data.materials.append(mats[mi])
    if bevel:
        mod=ob.modifiers.new('One segment edge chamfer','BEVEL'); mod.width=bevel; mod.segments=1
        bpy.ops.object.modifier_apply(modifier=mod.name)
    return ob
bodyparts=[cube('Footplate',(0,0,4),(26,26,4),0,1.6),cube('Pillar',(0,0,76),(16,16,70),0,1.5),cube('Hub',(0,0,154),(34,34,18),0,2),cube('Service panel',(0,-16.4,48),(9,.8,17),3,.6)]
def join(parts,name):
    bpy.ops.object.select_all(action='DESELECT')
    for ob in parts: ob.select_set(True)
    bpy.context.view_layer.objects.active=parts[0]; bpy.ops.object.join(); ob=parts[0]; ob.name=name
    scene.cursor.location=(0,0,0); bpy.ops.object.origin_set(type='ORIGIN_CURSOR'); return ob
body=join(bodyparts,'SM_NoiseEmitter_Body')
# Closed thick shell: from back through collar, outer flare, lip, inner flare, diaphragm.
rings=[(0,18),(9,20),(14,20),(110,50),(114,52),(118,52),(118,47),(113,46),(17,14),(13,14)]
verts=[(x,math.cos(i*math.pi/4)*r,math.sin(i*math.pi/4)*r) for x,r in rings for i in range(8)]
faces=[]; material_ids=[]
for j in range(len(rings)-1):
    for i in range(8):
        faces.append((j*8+i,j*8+(i+1)%8,(j+1)*8+(i+1)%8,(j+1)*8+i)); material_ids.append(3 if j<2 else (2 if j>=6 else 1))
faces.extend([tuple(reversed(range(8))),tuple(range(72,80))]); material_ids.extend([3,3])
me=bpy.data.meshes.new('Octagonal thick shell'); me.from_pydata(verts,[],faces); me.update()
horn=bpy.data.objects.new('SM_NoiseEmitter_Horn',me); scene.collection.objects.link(horn)
for m in mats: me.materials.append(m)
for poly,mi in zip(me.polygons,material_ids): poly.material_index=mi
def finish(ob):
    bpy.context.view_layer.objects.active=ob; bpy.ops.object.select_all(action='DESELECT'); ob.select_set(True)
    bm=bmesh.new(); bm.from_mesh(ob.data); bmesh.ops.recalc_face_normals(bm,faces=bm.faces); bm.to_mesh(ob.data); bm.free()
    bpy.ops.object.mode_set(mode='EDIT'); bpy.ops.mesh.select_all(action='SELECT'); bpy.ops.uv.smart_project(island_margin=.025); bpy.ops.object.mode_set(mode='OBJECT')
    ob.data.calc_loop_triangles()
    bm=bmesh.new(); bm.from_mesh(ob.data)
    stats={'vertices':len(ob.data.vertices),'faces':len(ob.data.polygons),'triangles':len(ob.data.loop_triangles),'nonmanifold_edges':sum(not e.is_manifold for e in bm.edges),'degenerate_faces':sum(f.calc_area()<1e-7 for f in bm.faces)}; bm.free()
    assert not stats['nonmanifold_edges'] and not stats['degenerate_faces'],stats
    return stats
stats={o.name:finish(o) for o in [body,horn]}
def export(obs,name):
    bpy.ops.object.select_all(action='DESELECT')
    for ob in obs: ob.select_set(True)
    bpy.context.view_layer.objects.active=obs[0]
    bpy.ops.export_scene.fbx(filepath=str(OUT/(name+'.fbx')),use_selection=True,object_types={'MESH'},axis_forward='-Y',axis_up='Z',apply_unit_scale=True,apply_scale_options='FBX_SCALE_UNITS',bake_anim=False,mesh_smooth_type='FACE',use_mesh_modifiers=True)
export([body],body.name); export([horn],horn.name)
horns=[]
for i in range(4):
    ob=horn if i==0 else horn.copy()
    if i:
        ob.data=horn.data.copy()
        scene.collection.objects.link(ob)
    angle=i*math.pi/2; ob.location=(34*math.cos(angle),34*math.sin(angle),154); ob.rotation_euler.z=angle; horns.append(ob)
export([body]+horns,'NoiseEmitter_Assembly')
stats['assembly_triangles']=stats[body.name]['triangles']+4*stats[horn.name]['triangles']
stats['bounds_cm']=[304,304,206]; stats['pivot']='Ground center; +Z up; reusable horn +X, pivot at narrow end'; stats['materials']={m.name:list(m.diffuse_color) for m in mats}
(OUT/'model_validation.json').write_text(json.dumps(stats,indent=2)+'\n')
scene.cycles.samples=48; scene.cycles.use_denoising=True
cam=scene.camera
for label,loc in [('final_front',(650,0,220)),('final_side',(0,-650,220)),('final_oblique',(480,-650,400)),('final_top',(0,0,800))]:
    cam.location=loc; cam.rotation_euler=(Vector((0,0,105))-cam.location).to_track_quat('-Z','Y').to_euler(); scene.render.filepath=str(OUT/(label+'.png')); bpy.ops.render.render(write_still=True)
cam.location=(480,-650,400); cam.rotation_euler=(Vector((0,0,105))-cam.location).to_track_quat('-Z','Y').to_euler()
bpy.ops.object.select_all(action='DESELECT'); body.select_set(True); bpy.context.view_layer.objects.active=body
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'NoiseEmitter.blend'))
