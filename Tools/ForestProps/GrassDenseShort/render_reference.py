import bpy
import json
from pathlib import Path
from mathutils import Vector

ROOT=Path(__file__).resolve().parents[3]
OUT=ROOT/'TunaSweeper/SourceArt/Environment/GrassDenseShort/Reference'
bpy.ops.wm.read_factory_settings(use_empty=True)
scene=bpy.context.scene
scene.render.engine='CYCLES'; scene.cycles.samples=24
scene.cycles.use_denoising=True
scene.render.resolution_x=1500; scene.render.resolution_y=1000
scene.render.resolution_percentage=100
scene.world=bpy.data.worlds.new('World'); scene.world.use_nodes=True
scene.world.node_tree.nodes['Background'].inputs[0].default_value=(.55,.59,.65,1)
scene.world.node_tree.nodes['Background'].inputs[1].default_value=.7
textures={'Bush':OUT/'Bush.tga', 'GrassLow':ROOT/'Blender/textures/M_GrassLowBlade.png',
          'Flower':ROOT/'Blender/tex/T_Flower.png','SimpleTree':ROOT/'Blender/tex/T_SimpleTree.png',
          'Wood':ROOT/'Blender/textures/T_WoodCommon.png','RockBasic':ROOT/'Blender/tex/T_Rock_Basic.png'}
report={}
for i,(name,tex) in enumerate(textures.items()):
    if (OUT/(name+'_BaseColor.tga')).exists():tex=OUT/(name+'_BaseColor.tga')
    bpy.ops.import_scene.fbx(filepath=str(OUT/(name+'.fbx')))
    objects=[o for o in bpy.context.selected_objects if o.type=='MESH' and not o.name.startswith(('UCX_','UBX_','USP_','UCP_'))]
    for o in list(bpy.context.selected_objects):
        if o.type=='MESH' and o not in objects:bpy.data.objects.remove(o,do_unlink=True)
    coords=[o.matrix_world@Vector(c) for o in objects for c in o.bound_box]
    lo=Vector([min(v[j] for v in coords) for j in range(3)])
    hi=Vector([max(v[j] for v in coords) for j in range(3)])
    scale=1.9/max(hi-lo)
    center=(lo+hi)*.5; center.z=lo.z
    pos=Vector(((i%3-1)*3.1,(1-i//3)*3.1,0))
    mat=bpy.data.materials.new(name+'_Reference'); mat.use_nodes=True
    bs=mat.node_tree.nodes.get('Principled BSDF'); bs.inputs['Roughness'].default_value=.9
    tn=mat.node_tree.nodes.new('ShaderNodeTexImage');tn.image=bpy.data.images.load(str(tex))
    mat.node_tree.links.new(tn.outputs['Color'],bs.inputs['Base Color'])
    for o in objects:
        o.location=(o.location-center)*scale+pos; o.scale*=scale
        o.data.materials.clear();o.data.materials.append(mat)
        for p in o.data.polygons:p.material_index=0
    report[name]={'source_texture':str(tex.relative_to(ROOT)),'display_normalized':True}
    bpy.ops.object.text_add(location=pos+Vector((-1,-1.05,.015)))
    label=bpy.context.object;label.data.body=name;label.data.size=.23
bpy.ops.mesh.primitive_plane_add(size=200)
floor=bpy.context.object;floor.location.z=-.02
mat=bpy.data.materials.new('Ground');mat.diffuse_color=(.26,.24,.20,1);floor.data.materials.append(mat)
for loc,power,size in [((1,-3,8),1700,7),((-5,3,5),1100,6)]:
    bpy.ops.object.light_add(type='AREA',location=loc);o=bpy.context.object;o.data.energy=power;o.data.shape='DISK';o.data.size=size
bpy.ops.object.camera_add(location=(0,-9,12));cam=bpy.context.object
cam.rotation_euler=(Vector((0,1.4,.4))-cam.location).to_track_quat('-Z','Y').to_euler()
cam.data.type='ORTHO';cam.data.ortho_scale=10.4;scene.camera=cam
scene.render.filepath=str(OUT/'existing_nature_style.png');bpy.ops.render.render(write_still=True)
(OUT/'reference_render_notes.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
