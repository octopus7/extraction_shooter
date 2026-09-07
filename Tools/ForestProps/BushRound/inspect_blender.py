import bpy,json,math
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parents[3]
OUT=ROOT/'TunaSweeper/SourceArt/Environment/ForestProps/BushRound'
bpy.ops.wm.read_factory_settings(use_empty=True)
s=bpy.context.scene;s.render.engine='CYCLES';s.cycles.samples=24;s.cycles.use_denoising=True
s.render.resolution_x=1500;s.render.resolution_y=900;s.render.resolution_percentage=100
s.world=bpy.data.worlds.new('World');s.world.use_nodes=True;s.world.node_tree.nodes['Background'].inputs[0].default_value=(.45,.45,.45,1)
report={}
for i,(name,file) in enumerate([('GrassLow','SM_GrassLow.blend'),('Flower','SM_Flower.blend'),('SimpleTree','SM_SimpleTree.blend'),('Wood','Wood.blend'),('RockBasic','RockBasic.blend'),('Bush',None)]):
    before=set(bpy.data.objects)
    if file:
        with bpy.data.libraries.load(str(ROOT/'Blender'/file),link=False) as (a,b):b.objects=a.objects
        objects=[o for o in b.objects if o and o.type=='MESH']
        for o in objects:
            if o.name not in s.collection.objects:s.collection.objects.link(o)
        for im in bpy.data.images:
            if im.source=='FILE' and not im.packed_file:
                matches=list((ROOT/'Blender').rglob(Path(im.filepath.replace('\\','/')).name))
                if matches:im.filepath=str(matches[0]);im.reload()
    else:
        bpy.ops.import_scene.fbx(filepath=str(OUT/'Bush_Reference.fbx'))
        objects=[o for o in set(bpy.data.objects)-before if o.type=='MESH']
        tex=bpy.data.images.load(str(OUT/'Bush_Reference.tga'))
        for o in objects:
            for m in o.data.materials:
                if m:
                    m.use_nodes=True;n=m.node_tree.nodes.new('ShaderNodeTexImage');n.image=tex
                    m.node_tree.links.new(n.outputs['Color'],m.node_tree.nodes.get('Principled BSDF').inputs['Base Color'])
    # Show a representative object, not every co-located source variation.
    objects.sort(key=lambda o:len(o.data.polygons),reverse=True)
    keep=objects[:1]
    for o in objects[1:]:bpy.data.objects.remove(o,do_unlink=True)
    o=keep[0];o.hide_render=False;o.hide_set(False)
    coords=[o.matrix_world@Vector(v) for v in o.bound_box]
    lo=Vector(tuple(min(v[j] for v in coords) for j in range(3)));hi=Vector(tuple(max(v[j] for v in coords) for j in range(3)))
    dims=hi-lo;scale=2.1/max(dims);offset=Vector(((i%3-1)*3.3,(i//3)*3.3,0))
    for v in o.data.vertices:v.co=(o.matrix_world@v.co-Vector(((lo.x+hi.x)/2,(lo.y+hi.y)/2,lo.z)))*scale+offset
    o.matrix_world.identity()
    report[name]={'source':file or '/Game/Nature/Bush','object':o.name,'source_dimensions':list(dims),'faces':len(o.data.polygons)}
    bpy.ops.object.text_add(location=offset+Vector((0,-1.4,.02)));t=bpy.context.object;t.data.body=name;t.data.align_x='CENTER';t.data.size=.23
bpy.ops.mesh.primitive_plane_add(size=200);floor=bpy.context.object;floor.location.z=-.03
m=bpy.data.materials.new('Neutral');m.diffuse_color=(.19,.20,.18,1);floor.data.materials.append(m)
bpy.ops.object.light_add(type='AREA',location=(0,-3,10));bpy.context.object.data.energy=1700;bpy.context.object.data.shape='DISK';bpy.context.object.data.size=8
bpy.ops.object.camera_add(location=(7,-11,12));c=bpy.context.object;c.rotation_euler=(Vector((0,1.7,.5))-c.location).to_track_quat('-Z','Y').to_euler();c.data.type='ORTHO';c.data.ortho_scale=11.2;s.camera=c
s.render.filepath=str(OUT/'Previews/ExistingStyle.png');bpy.ops.render.render(write_still=True)
(OUT/'existing_blender.json').write_text(json.dumps(report,indent=2))
