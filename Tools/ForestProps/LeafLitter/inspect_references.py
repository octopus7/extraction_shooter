"""Read existing nature sources, render representative meshes without modifying them."""
from pathlib import Path
import bpy, json
from mathutils import Vector
ROOT=Path(__file__).resolve().parents[3]
OUT=ROOT/'TunaSweeper/SourceArt/Environment/ForestProps/LeafLitter'
report={}
for file in ['SM_GrassLow.blend','SM_Flower.blend','SM_SimpleTree.blend','Wood.blend','RockBasic.blend','Bush.fbx']:
    if file.endswith('.fbx'):
        bpy.ops.wm.read_factory_settings(use_empty=True)
        bpy.ops.import_scene.fbx(filepath=str(OUT/'Previews/Reference_Bush.fbx'))
        im=bpy.data.images.load(str(OUT/'Previews/Reference_Bush.tga'))
        for mat in bpy.data.materials:
            mat.use_nodes=True;node=mat.node_tree.nodes.new('ShaderNodeTexImage');node.image=im
            mat.node_tree.links.new(node.outputs['Color'],mat.node_tree.nodes.get('Principled BSDF').inputs['Base Color'])
    else:bpy.ops.wm.open_mainfile(filepath=str(ROOT/'Blender'/file))
    meshes=[o for o in bpy.context.scene.objects if o.type=='MESH']
    report[file]=[{'name':o.name,'dimensions':list(o.dimensions),'faces':len(o.data.polygons)} for o in meshes]
    # Keep representative complete mesh; tree and wood sources contain multiple variants.
    choices=[o for o in meshes if not o.hide_render]
    obj=max(choices or meshes,key=lambda o:len(o.data.polygons))
    for o in list(bpy.context.scene.objects):
        if o!=obj: bpy.data.objects.remove(o,do_unlink=True)
    obj.hide_render=False;obj.hide_set(False)
    coords=[obj.matrix_world@Vector(v) for v in obj.bound_box]
    lo=Vector([min(v[i] for v in coords) for i in range(3)])
    hi=Vector([max(v[i] for v in coords) for i in range(3)])
    center=(lo+hi)/2; size=max(hi-lo)
    for im in bpy.data.images:
        if not im.packed_file and im.source=='FILE':
            candidates=[p for p in (ROOT/'Blender').rglob(im.filepath.replace('\\','/').split('/')[-1]) if p.is_file()]
            if candidates: im.filepath=str(candidates[0]);im.reload()
    scene=bpy.context.scene;scene.render.engine='CYCLES';scene.cycles.samples=12
    scene.render.resolution_x=650;scene.render.resolution_y=650;scene.render.resolution_percentage=100
    # Common matte study lighting; retain source base color/UV and suppress baked PBR glare.
    for mat in bpy.data.materials:
        if mat.use_nodes:
            for bs in [n for n in mat.node_tree.nodes if n.type=='BSDF_PRINCIPLED']:
                for key,value in [('Metallic',0),('Roughness',.9),('Specular IOR Level',.18)]:
                    for link in list(bs.inputs[key].links):mat.node_tree.links.remove(link)
                    bs.inputs[key].default_value=value
    scene.view_settings.view_transform='AgX'
    scene.world=bpy.data.worlds.new('ReferenceWorld');scene.world.use_nodes=True
    scene.world.node_tree.nodes['Background'].inputs[0].default_value=(.22,.25,.29,1)
    scene.world.node_tree.nodes['Background'].inputs[1].default_value=.6
    bpy.ops.object.camera_add(location=center+Vector((1.3,-1.8,1.3))*size)
    cam=bpy.context.object;cam.rotation_euler=(center-cam.location).to_track_quat('-Z','Y').to_euler()
    cam.data.type='ORTHO';cam.data.ortho_scale=size*1.45;scene.camera=cam
    bpy.ops.object.light_add(type='AREA',location=center+Vector((0,-1,2))*size)
    bpy.context.object.data.energy=120*size*size;bpy.context.object.data.shape='DISK';bpy.context.object.data.size=size*3
    scene.render.filepath=str(OUT/'Previews'/('Reference_'+Path(file).stem+'.png'))
    bpy.ops.render.render(write_still=True)
(OUT/'reference_inventory.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
