from pathlib import Path
import bpy, json, os
from mathutils import Vector
ROOT=Path(__file__).resolve().parents[3]
OUT=ROOT/'TunaSweeper/SourceArt/ForestProps/BushSpreading'
report=json.loads((OUT/'reference_inventory.json').read_text()) if (OUT/'reference_inventory.json').exists() and os.environ.get('BUSH_REFERENCE_ONLY') else {}
for file in ['SM_GrassLow.blend','SM_Flower.blend','SM_SimpleTree.blend','Wood.blend','RockBasic.blend','Bush']:
    if os.environ.get('BUSH_REFERENCE_ONLY') and file!=os.environ['BUSH_REFERENCE_ONLY']:continue
    if file=='Bush':
        bpy.ops.wm.read_factory_settings(use_empty=True)
        bpy.ops.import_scene.fbx(filepath=str(OUT/'Previews/reference_Bush.fbx'))
    else: bpy.ops.wm.open_mainfile(filepath=str(ROOT/'Blender'/file))
    if bpy.context.object and bpy.context.object.mode!='OBJECT': bpy.ops.object.mode_set(mode='OBJECT')
    for o in list(bpy.context.scene.objects):
        if o.name.startswith(('UCX_','UBX_')):bpy.data.objects.remove(o,do_unlink=True)
    meshes=[o for o in bpy.context.scene.objects if o.type=='MESH' and not o.hide_render]
    report[file]=[{'name':o.name,'dimensions':list(o.dimensions),'polygons':len(o.data.polygons),'materials':[m.name for m in o.data.materials if m]} for o in meshes]
    # Reconstruct base-color-only preview graphs from repository textures because
    # the legacy source materials reference unavailable external paint resources.
    texture={'SM_GrassLow.blend':'textures/M_GrassLowBlade.png','SM_Flower.blend':'tex/T_Flower.png','SM_SimpleTree.blend':'tex/T_SimpleTree.png','Wood.blend':'textures/T_WoodCommon.png','RockBasic.blend':'tex/T_Rock_Basic.png'}
    texpath=OUT/'Previews/reference_Bush.tga' if file=='Bush' else ROOT/'Blender'/texture[file]
    for mat in bpy.data.materials:
        mat.use_nodes=True;mat.node_tree.nodes.clear()
        output=mat.node_tree.nodes.new('ShaderNodeOutputMaterial');bsdf=mat.node_tree.nodes.new('ShaderNodeBsdfPrincipled')
        bsdf.inputs['Base Color'].default_value=(.26,.29,.24,1);bsdf.inputs['Roughness'].default_value=.85
        mat.node_tree.links.new(bsdf.outputs['BSDF'],output.inputs['Surface'])
        if file=='SM_SimpleTree.blend': texpath=OUT/'Previews'/('reference_T_SimplePineTree.tga' if 'Pine' in mat.name else 'reference_T_SimpleTree.tga')
        if texpath.exists():
            node=mat.node_tree.nodes.new('ShaderNodeTexImage');node.image=bpy.data.images.load(str(texpath),check_existing=False)
            mat.node_tree.links.new(node.outputs['Color'],bsdf.inputs['Base Color'])
    for o in list(bpy.context.scene.objects):
        if o.type!='MESH': bpy.data.objects.remove(o,do_unlink=True)
    # Preserve source geometry/materials; place each object in a visible row.
    for i,o in enumerate(meshes):
        o.hide_set(False);o.hide_render=False
        scale=1.6/max(o.dimensions)
        o.scale*=scale
        bpy.context.view_layer.update()
        pts=[o.matrix_world@Vector(v) for v in o.bound_box]
        center=Vector(((min(p.x for p in pts)+max(p.x for p in pts))/2,(min(p.y for p in pts)+max(p.y for p in pts))/2,min(p.z for p in pts)))
        o.location+=Vector(((i%5)*2.0,(i//5)*2,0))-center
    scene=bpy.context.scene
    scene.render.engine='CYCLES';scene.cycles.samples=16;scene.cycles.use_denoising=True
    scene.render.resolution_x=1400;scene.render.resolution_y=850;scene.render.resolution_percentage=100
    scene.world=bpy.data.worlds.new('ReferenceWorld');scene.world.use_nodes=True
    scene.world.node_tree.nodes['Background'].inputs[0].default_value=(.4,.4,.4,1)
    scene.world.node_tree.nodes['Background'].inputs[1].default_value=.7
    pts=[o.matrix_world@Vector(v) for o in meshes for v in o.bound_box]
    center=Vector(((min(p.x for p in pts)+max(p.x for p in pts))/2,(min(p.y for p in pts)+max(p.y for p in pts))/2,.4))
    bpy.ops.object.camera_add(location=center+Vector((3,-6,5)))
    cam=bpy.context.object;cam.rotation_euler=(center-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.type='ORTHO'
    cam.data.ortho_scale=max(5,max(p.x for p in pts)-min(p.x for p in pts)+2,(max(p.y for p in pts)-min(p.y for p in pts))*1.7+3)
    scene.camera=cam
    bpy.ops.object.light_add(type='AREA',location=center+Vector((0,-3,6)))
    bpy.context.object.data.energy=1200;bpy.context.object.data.shape='DISK';bpy.context.object.data.size=7
    scene.view_settings.view_transform='Standard'
    scene.render.filepath=str(OUT/'Previews'/('reference_'+file.replace('.blend','')+'.png'))
    bpy.ops.render.render(write_still=True)
(OUT/'reference_inventory.json').write_text(json.dumps(report,indent=2))
