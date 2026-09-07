"""Render exported existing Nature geometry with its own base-color texture."""
import bpy, math, json
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parents[3]
OUT=ROOT/'TunaSweeper/SourceArt/Environment/ForestProps/BushSparse'
bpy.ops.wm.read_factory_settings(use_empty=True)
scene=bpy.context.scene
scene.render.engine='CYCLES';scene.cycles.samples=24;scene.cycles.use_denoising=True
scene.render.resolution_x=1500;scene.render.resolution_y=1000;scene.render.resolution_percentage=100
scene.world=bpy.data.worlds.new('ReferenceWorld');scene.world.use_nodes=True
scene.world.node_tree.nodes['Background'].inputs[0].default_value=(.38,.41,.43,1)
scene.world.node_tree.nodes['Background'].inputs[1].default_value=.6
scene.view_settings.view_transform='AgX'
report=json.loads((OUT/'References/existing_asset_audit.json').read_text())
for i,entry in enumerate(report):
    folder=entry['category'];bpy.ops.object.select_all(action='DESELECT')
    bpy.ops.import_scene.fbx(filepath=str(OUT/'References'/(folder+'.fbx')))
    objects=[o for o in bpy.context.selected_objects if o.type=='MESH']
    coords=[o.matrix_world@v.co for o in objects for v in o.data.vertices]
    lo=Vector([min(v[a] for v in coords) for a in range(3)]);hi=Vector([max(v[a] for v in coords) for a in range(3)])
    scale=1.6/max(hi-lo);offset=Vector(((i%3-1)*2.4,(1-i//3)*2.5,0))
    mat=bpy.data.materials.new(folder);mat.use_nodes=True
    bsdf=mat.node_tree.nodes.get('Principled BSDF');bsdf.inputs['Roughness'].default_value=.9
    texpath=OUT/'References'/(folder+'.tga')
    if texpath.exists():
        node=mat.node_tree.nodes.new('ShaderNodeTexImage');node.image=bpy.data.images.load(str(texpath))
        mat.node_tree.links.new(node.outputs['Color'],bsdf.inputs['Base Color'])
        if 'MASKED' in entry['materials'][0]['blend_mode']:
            mat.node_tree.links.new(node.outputs['Alpha'],bsdf.inputs['Alpha'])
    else:bsdf.inputs['Base Color'].default_value=(.04,.18,.025,1)
    center=Vector(((lo.x+hi.x)/2,(lo.y+hi.y)/2,lo.z))
    for o in objects:
        matrix=o.matrix_world.copy()
        for v in o.data.vertices:v.co=(matrix@v.co-center)*scale+offset
        o.matrix_world.identity();o.data.materials.clear();o.data.materials.append(mat)
        for p in o.data.polygons:p.material_index=0
    bpy.ops.object.text_add(location=offset+Vector((-.88,-1.0,.01)))
    t=bpy.context.object;t.data.body=folder;t.data.size=.19
    tmat=bpy.data.materials.get('Label') or bpy.data.materials.new('Label');tmat.diffuse_color=(.025,.035,.04,1);t.data.materials.append(tmat)
bpy.ops.mesh.primitive_plane_add(size=200,location=(0,0,-.02));floor=bpy.context.object
mat=bpy.data.materials.new('Ground');mat.diffuse_color=(.24,.27,.25,1);floor.data.materials.append(mat)
bpy.ops.object.light_add(type='AREA',location=(-3,-4,8));bpy.context.object.data.energy=1500;bpy.context.object.data.shape='DISK';bpy.context.object.data.size=7
bpy.ops.object.camera_add(location=(5,-10,11));cam=bpy.context.object;cam.rotation_euler=(Vector((0,1,.2))-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.type='ORTHO';cam.data.ortho_scale=9.1;scene.camera=cam
scene.render.filepath=str(OUT/'Previews/ExistingNature_Reference.png');bpy.ops.render.render(write_still=True)
