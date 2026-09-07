"""Render existing Blender nature sources without saving or modifying them."""
import bpy, json
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parents[3]
OUT=ROOT/'TunaSweeper/SourceArt/Environment/ForestProps/ExposedRoots'
report={}
for stem in ['Wood','SM_GrassLow','SM_Flower','SM_SimpleTree','RockBasic','Bush']:
    if stem=='Bush':
        bpy.ops.wm.read_factory_settings(use_empty=True)
        bpy.ops.import_scene.fbx(filepath=str(OUT/'Reference_Bush.fbx'))
    else:bpy.ops.wm.open_mainfile(filepath=str(ROOT/'Blender'/f'{stem}.blend'))
    if bpy.context.object and bpy.context.object.mode!='OBJECT': bpy.ops.object.mode_set(mode='OBJECT')
    scene=bpy.context.scene
    for o in scene.objects:
        if o.name.startswith(('UCX_','UBX_')) or (stem=='SM_SimpleTree' and 'Pine' in o.name):o.hide_render=True
    meshes=[o for o in scene.objects if o.type=='MESH' and not o.hide_render]
    report[stem]=[{'name':o.name,'dimensions':list(o.dimensions),'faces':len(o.data.polygons),'materials':[m.name if m else None for m in o.data.materials]} for o in meshes]
    texture={'Wood':ROOT/'Blender/textures/T_WoodCommon.png','SM_GrassLow':ROOT/'Blender/textures/M_GrassLowBlade.png','SM_Flower':ROOT/'Blender/tex/T_Flower.png','SM_SimpleTree':OUT/'Reference_Tree.tga','RockBasic':ROOT/'Blender/tex/T_Rock_Basic.png','Bush':OUT/'Reference_Bush.tga'}[stem]
    im=bpy.data.images.load(str(texture),check_existing=False)
    mat=bpy.data.materials.new('ReferenceBaseColor');mat.use_nodes=True
    tex=mat.node_tree.nodes.new('ShaderNodeTexImage');tex.image=im
    bs=mat.node_tree.nodes.get('Principled BSDF');bs.inputs['Roughness'].default_value=.88
    mat.node_tree.links.new(tex.outputs['Color'],bs.inputs['Base Color'])
    mat.node_tree.links.new(tex.outputs['Alpha'],bs.inputs['Alpha'])
    for index,o in enumerate(meshes):
        for slot in o.material_slots: slot.material=mat
        scale=1/max(o.dimensions)
        o.scale*=scale
        bpy.context.view_layer.update()
        low=Vector(tuple(min((o.matrix_world@Vector(v))[i] for v in o.bound_box) for i in range(3)))
        o.location+=Vector(((index%4)*1.5,(index//4)*1.5,0))-low
    bpy.context.view_layer.update()
    for o in list(scene.objects):
        if o.type in {'LIGHT','CAMERA'}: bpy.data.objects.remove(o,do_unlink=True)
    bounds=[o.matrix_world@Vector(v) for o in meshes for v in o.bound_box]
    low=Vector(tuple(min(v[i] for v in bounds) for i in range(3)))
    high=Vector(tuple(max(v[i] for v in bounds) for i in range(3)))
    center=(low+high)/2; size=max(high-low)
    bpy.ops.object.camera_add(location=center+Vector((1,-1.5,1.35))*size)
    camera=bpy.context.object;camera.rotation_euler=(center-camera.location).to_track_quat('-Z','Y').to_euler()
    camera.data.type='ORTHO';camera.data.ortho_scale=size*1.45;camera.data.clip_end=size*20+100
    scene.camera=camera
    scene.world=bpy.data.worlds.new('ReferenceWorld');scene.world.use_nodes=True
    scene.world.node_tree.nodes['Background'].inputs[0].default_value=(.4,.45,.5,1)
    scene.world.node_tree.nodes['Background'].inputs[1].default_value=.7
    bpy.ops.object.light_add(type='SUN',location=center+Vector((0,-1,2))*size)
    bpy.context.object.rotation_euler=(.4,-.5,-.4);bpy.context.object.data.energy=2
    scene.render.engine='CYCLES';scene.cycles.samples=16;scene.cycles.use_denoising=True
    scene.render.resolution_x=1000;scene.render.resolution_y=800;scene.render.resolution_percentage=100
    scene.view_settings.view_transform='Standard'
    scene.render.filepath=str(OUT/'Previews'/f'Reference_{stem}.png')
    bpy.ops.render.render(write_still=True)
(OUT/'reference_inventory.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
