"""Read-only style inspection of existing Blender sources; render normalized examples."""
import bpy, json
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parents[3]
OUT=ROOT/'TunaSweeper/SourceArt/Environment/ForestProps/GrassSparse/Reference'
OUT.mkdir(parents=True,exist_ok=True)
report=[]
for filename in ['SM_GrassLow.blend','SM_Flower.blend','SM_SimpleTree.blend','Wood.blend','RockBasic.blend']:
    bpy.ops.wm.open_mainfile(filepath=str(ROOT/'Blender'/filename))
    objects=[o for o in bpy.context.scene.objects if o.type=='MESH']
    report.append({'source':'Blender/'+filename,'objects':[{'name':o.name,'dimensions':list(o.dimensions),'polygons':len(o.data.polygons),'materials':[m.name if m else None for m in o.data.materials]} for o in objects], 'images':[{'name':i.name,'path':i.filepath,'packed':bool(i.packed_file)} for i in bpy.data.images]})
    # Select one representative existing mesh; preserve its material and geometry.
    preferences={'Wood.blend':'SM_StumpA','RockBasic.blend':'RockM_1','SM_GrassLow.blend':'SM_GrassLow','SM_SimpleTree.blend':'SM_SimpleTree'}
    obj=next((o for o in objects if o.name==preferences.get(filename)),objects[0])
    for o in list(bpy.context.scene.objects):
        if o!=obj: bpy.data.objects.remove(o,do_unlink=True)
    obj.hide_render=False;obj.hide_set(False)
    points=[obj.matrix_world@Vector(c) for c in obj.bound_box]
    center=sum(points,Vector())/8
    size=max(obj.dimensions)
    bpy.ops.object.camera_add(location=center+Vector((1.3,-1.9,1.25))*size)
    cam=bpy.context.object;cam.rotation_euler=(center-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.type='ORTHO';cam.data.ortho_scale=size*1.4
    scene=bpy.context.scene;scene.camera=cam;scene.render.engine='CYCLES';scene.cycles.samples=16
    scene.world=bpy.data.worlds.new('ReferenceWorld');scene.world.use_nodes=True;scene.world.node_tree.nodes['Background'].inputs[0].default_value=(.55,.55,.55,1);scene.world.node_tree.nodes['Background'].inputs[1].default_value=.65
    bpy.ops.object.light_add(type='AREA',location=center+Vector((1,-1,2))*size);bpy.context.object.data.energy=450*size*size;bpy.context.object.data.shape='DISK';bpy.context.object.data.size=size*2
    scene.render.resolution_x=640;scene.render.resolution_y=640;scene.render.resolution_percentage=100
    scene.view_settings.view_transform='Standard';scene.render.filepath=str(OUT/(Path(filename).stem+'.png'))
    bpy.ops.render.render(write_still=True)
(OUT/'source_inspection.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
