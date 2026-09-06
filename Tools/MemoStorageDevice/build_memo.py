"""Reproducible Blender 4.5 LTS prop. Modeling inputs are centimeters.

No generated raster art is needed: four constant PBR materials preserve the
existing memo's blue/graphite/cyan palette without noisy miniature texture detail.
"""
import bpy
import bmesh
import json
import math
import sys
from pathlib import Path
from mathutils import Vector

ROOT = Path(__file__).resolve().parents[2]
LOW_POLY = '--low-poly' in sys.argv
OUT = ROOT / 'TunaSweeper/SourceArt/Memo/StorageDevice'
if LOW_POLY:
    OUT = OUT / 'LowPoly'
for folder in ['Models', 'Previews']:
    (OUT/folder).mkdir(parents=True, exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True)
scene = bpy.context.scene
scene.unit_settings.system = 'METRIC'
scene.unit_settings.scale_length = 1.0
NAME = 'SM_MemoStorageDevice_LowPoly' if LOW_POLY else 'SM_MemoStorageDevice'
SPECS = {
    'M_MemoDevice_Shell': {'color': [.045, .13, .28, 1], 'metallic': .35, 'roughness': .5, 'emission': 0},
    'M_MemoDevice_Guard': {'color': [.025, .035, .045, 1], 'metallic': .05, 'roughness': .78, 'emission': 0},
    'M_MemoDevice_Metal': {'color': [.34, .40, .44, 1], 'metallic': .8, 'roughness': .34, 'emission': 0},
    'M_MemoDevice_Status': {'color': [.025, .75, .9, 1], 'metallic': .1, 'roughness': .28, 'emission': 2.0},
}
mats = {}
for name, spec in SPECS.items():
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    mat.diffuse_color = spec['color']
    shader = mat.node_tree.nodes['Principled BSDF']
    shader.inputs['Base Color'].default_value = spec['color']
    shader.inputs['Metallic'].default_value = spec['metallic']
    shader.inputs['Roughness'].default_value = spec['roughness']
    shader.inputs['Emission Color'].default_value = spec['color']
    shader.inputs['Emission Strength'].default_value = spec['emission']
    mats[name.rsplit('_', 1)[1]] = mat
parts = []

def finish(obj, name, material, bevel):
    obj.name = name
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)
    if bevel:
        mod = obj.modifiers.new('Readable edge bevel', 'BEVEL')
        mod.width = bevel/100
        mod.segments = 1 if LOW_POLY else 2
        bpy.ops.object.modifier_apply(modifier=mod.name)
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bmesh.ops.remove_doubles(bm, verts=list(bm.verts), dist=1e-7)
    bmesh.ops.dissolve_degenerate(bm, edges=list(bm.edges), dist=1e-7)
    bm.to_mesh(obj.data)
    bm.free()
    for face in obj.data.polygons:
        face.use_smooth = not (LOW_POLY and bevel == 0)
    mod = obj.modifiers.new('Weighted surface normals', 'WEIGHTED_NORMAL')
    mod.keep_sharp = True
    mod.weight = 50
    bpy.ops.object.modifier_apply(modifier=mod.name)
    obj.data.materials.append(mats[material])
    parts.append(obj)
    return obj

def box(name, loc, size, material, bevel=.06):
    bpy.ops.mesh.primitive_cube_add(size=1, location=Vector(loc)/100)
    obj = bpy.context.object
    obj.dimensions = Vector(size)/100
    return finish(obj, name, material, bevel)

def plate(name, loc, size, cut, material, bevel=.06):
    x, y, z = (v/2 for v in size)
    outline = [(-x+cut,-y),(x-cut,-y),(x,-y+cut),(x,y-cut),
               (x-cut,y),(-x+cut,y),(-x,y-cut),(-x,-y+cut)]
    verts = [((vx+loc[0])/100,(vy+loc[1])/100,(vz+loc[2])/100)
             for vz in [-z,z] for vx,vy in outline]
    faces = [tuple(reversed(range(8))), tuple(range(8,16))]
    faces += [(i,(i+1)%8,(i+1)%8+8,i+8) for i in range(8)]
    mesh = bpy.data.meshes.new(name)
    mesh.from_pydata(verts, [], faces)
    mesh.update()
    obj = bpy.data.objects.new(name, mesh)
    scene.collection.objects.link(obj)
    bpy.ops.object.select_all(action='DESELECT')
    obj.select_set(True)
    return finish(obj, name, material, bevel)

if LOW_POLY:
    sys.path.insert(0, str(Path(__file__).resolve().parent))
    from low_poly_geometry import build
    build(box, plate)
else:
    # Bottom rests at Z=0; body length X=11 cm, connector projects towards +X.
    plate('Impact guard lower', (0,0,.44), (10.9,6.9,.64), .75, 'Guard', .12)
    plate('Blue chassis', (0,0,1.0), (10.6,6.6,1.18), .65, 'Shell', .11)
    plate('Lid seam', (0,0,1.47), (10.25,6.25,.16), .58, 'Guard', .035)
    plate('Blue lid', (-.1,0,1.66), (9.85,5.95,.32), .64, 'Shell', .1)
    plate('Top inset border', (-.55,0,1.827), (7.2,4.65,.07), .6, 'Guard', .018)
    plate('Top inset metal rim', (-.55,0,1.863), (6.95,4.4,.07), .55, 'Metal', .015)
    plate('Recessed data panel', (-.55,0,1.902), (6.73,4.18,.08), .51, 'Shell', .026)
    # Broad corner armor wraps the height and remains legible from the gameplay view.
    for x in [-4.65, 4.65]:
        for y in [-2.65, 2.65]:
            plate('Corner bumper', (x,y,1.06), (1.7,1.7,1.96), .43, 'Guard', .1)
            box('Armor inlay', (x,y,2.038), (.72,.76,.065), 'Metal', .06)
    for y in [-3.29,3.29]:
        for x in [-1.7,0,1.7]:
            box('Grip rib', (x,y,.99), (.55,.27,.94), 'Guard', .07)
    # A substantial socket shroud, black tongue and 5 broad silver contacts.
    plate('Connector root', (5.18,0,1.0), (1.0,3.85,1.16), .2, 'Guard', .06)
    box('Connector top', (5.72,0,1.46), (1.56,3.25,.2), 'Metal', .055)
    box('Connector bottom', (5.72,0,.61), (1.56,3.25,.2), 'Metal', .055)
    for y in [-1.51,1.51]:
        box('Connector side', (5.72,y,1.035), (1.56,.23,.75), 'Metal', .045)
    box('Connector cavity', (5.26,0,1.035), (.12,2.83,.72), 'Guard', .015)
    box('Contact tongue', (5.82,0,.89), (1.14,2.6,.21), 'Guard', .045)
    for y in [-.96,-.48,0,.48,.96]:
        box('Contact', (5.99,y,1.007), (.85,.24,.04), 'Metal', .009)
    # One small status indicator and an embossed data-stack pictogram; no narrative.
    plate('Indicator bezel', (3.49,0,1.839), (.85,2.63,.11), .2, 'Guard', .025)
    plate('Status lens', (3.49,0,1.913), (.4,1.92,.065), .13, 'Status', .025)
    for i in range(3):
        box('Data stack glyph', (-.76+i*.44,0,1.965), (.18,1.2,.05), 'Metal', .016)
    # Underside service plate, kept above the bottom seating plane.
    plate('Underside plate', (0,0,.09), (7.7,4.7,.11), .45, 'Shell', .018)
    for y in [-1.52,1.52]:
        box('Underside skid', (0,y,.025), (5.7,.39,.05), 'Guard', .012)

bpy.ops.object.select_all(action='DESELECT')
for obj in parts:
    obj.select_set(True)
bpy.context.view_layer.objects.active = parts[0]
bpy.ops.object.join()
model = bpy.context.object
model.name = NAME
scene.cursor.location = (0,0,0)
bpy.ops.object.origin_set(type='ORIGIN_CURSOR')
bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
# Put shell in slot zero, matching the native actor's single material override.
old_slots = list(model.data.materials)
indices = [list(SPECS).index(old_slots[p.material_index].name) for p in model.data.polygons]
model.data.materials.clear()
for name in SPECS:
    model.data.materials.append(bpy.data.materials[name])
for p, idx in zip(model.data.polygons,indices):
    p.material_index = idx
bpy.ops.object.mode_set(mode='EDIT')
bpy.ops.mesh.select_all(action='SELECT')
bpy.ops.uv.smart_project(angle_limit=math.radians(66), island_margin=.015)
bpy.ops.object.mode_set(mode='OBJECT')
model.data.uv_layers.active.name = 'UV0'
tri = model.modifiers.new('Deterministic triangulation', 'TRIANGULATE')
tri.keep_custom_normals = True
bpy.ops.object.modifier_apply(modifier=tri.name)
model.data.calc_loop_triangles()
bm = bmesh.new()
bm.from_mesh(model.data)
assert all(e.is_manifold for e in bm.edges), 'Each intersecting solid must be closed'
assert all(f.calc_area()>1e-12 for f in bm.faces), 'Degenerate triangle'
assert all(math.isfinite(c) for v in bm.verts for c in v.co)
bm.free()
coords = [v.co for v in model.data.vertices]
bounds = [min(p[i] for p in coords)*100 for i in range(3)]+[max(p[i] for p in coords)*100 for i in range(3)]
assert abs(bounds[2])<.001
assert all(-.0001<=c<=1.0001 for uv in model.data.uv_layers.active.data for c in uv.uv)
manifest = {'name':NAME, 'blender':bpy.app.version_string, 'bounds_cm':bounds,
    'size_cm':[bounds[i+3]-bounds[i] for i in range(3)],
    'triangles':len(model.data.loop_triangles), 'vertices':len(model.data.vertices),
    'closed_solids':len(parts), 'uv_channels':1, 'materials':SPECS,
    'axis_contract':'X length, connector +X, Z up; Y symmetric',
    'pivot':'Origin at body bottom center; connector makes bounds center asymmetric in X',
    'collision_boxes':1, 'texture_dependencies':[], 'checks_passed':True}
if LOW_POLY:
    assert manifest['triangles'] + 12 < 1000, manifest['triangles']
    manifest['triangle_budget'] = 999
    manifest['triangles_including_collision'] = manifest['triangles'] + 12
# One lightweight conservative body collision box, including protruding connector.
bpy.ops.mesh.primitive_cube_add(size=1, location=Vector(((bounds[0]+bounds[3])/200,0,bounds[5]/200)))
collision = bpy.context.object
collision.name = 'UCX_'+NAME+'_00'
collision.dimensions = Vector(manifest['size_cm'])/100
bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
bpy.ops.object.select_all(action='DESELECT')
model.select_set(True)
collision.select_set(True)
bpy.context.view_layer.objects.active = model
bpy.ops.export_scene.fbx(filepath=str(OUT/'Models'/f'{NAME}.fbx'), use_selection=True,
    object_types={'MESH'}, apply_unit_scale=True, apply_scale_options='FBX_SCALE_NONE',
    axis_forward='-Y', axis_up='Z', mesh_smooth_type='FACE', bake_anim=False,
    add_leaf_bones=False, path_mode='STRIP')
collision.hide_render = True
collision.hide_set(True)
collision.display_type = 'WIRE'
(OUT/'model_manifest.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')

# Studio-only collection is excluded from FBX, clearly separated in the .blend.
studio = bpy.data.collections.new('PREVIEW ONLY - cameras lights floor')
scene.collection.children.link(studio)
def studio_move(obj):
    for c in list(obj.users_collection):
        c.objects.unlink(obj)
    studio.objects.link(obj)
def aim(obj, point=(0,0,.009)):
    obj.rotation_euler = (Vector(point)-obj.location).to_track_quat('-Z','Y').to_euler()
bpy.ops.mesh.primitive_plane_add(size=200)
floor = bpy.context.object
floor.name = 'Studio floor'
floor.location.z = -.001
studio_move(floor)
mat = bpy.data.materials.new('Preview backdrop')
mat.diffuse_color = (.12,.145,.17,1)
mat.use_nodes = True
mat.node_tree.nodes['Principled BSDF'].inputs['Base Color'].default_value=(.12,.145,.17,1)
mat.node_tree.nodes['Principled BSDF'].inputs['Roughness'].default_value=.83
floor.data.materials.append(mat)
for name,loc,energy,size in [('Key',(0,-.2,.3),.6,.22),('Fill',(.16,.14,.16),.3,.18),('Rim',(-.2,.1,.2),.45,.13)]:
    bpy.ops.object.light_add(type='AREA',location=loc)
    light=bpy.context.object
    light.name=name
    light.data.energy=energy
    light.data.shape='DISK'
    light.data.size=size
    aim(light)
    studio_move(light)
scene.world=bpy.data.worlds.new('Studio world')
scene.world.use_nodes=True
scene.world.node_tree.nodes['Background'].inputs[0].default_value=(.3,.35,.42,1)
scene.world.node_tree.nodes['Background'].inputs[1].default_value=.4
bpy.ops.object.camera_add(location=(.17,-.21,.23))
cam=bpy.context.object
cam.name='Review camera'
studio_move(cam)
scene.camera=cam
cam.data.type='ORTHO'
cam.data.ortho_scale=.18
aim(cam)
scene.render.engine='CYCLES'
scene.cycles.samples=32
scene.cycles.use_denoising=True
scene.render.resolution_x=1000
scene.render.resolution_y=800
scene.render.resolution_percentage=100
scene.render.image_settings.file_format='PNG'
scene.view_settings.view_transform='AgX'
scene.view_settings.look='AgX - Medium High Contrast'
views={'Hero':(.17,-.21,.23),'Rear':(-.18,.21,.16),'Top':(0,0,.3),'Bottom':(.14,-.2,-.25),'Connector':(.3,-.04,.06)}
for name,loc in views.items():
    floor.hide_render=name=='Bottom'
    bpy.data.objects['Key'].location.z = -.3 if name=='Bottom' else .3
    aim(bpy.data.objects['Key'])
    cam.location=loc
    aim(cam)
    scene.render.filepath=str(OUT/'Previews'/f'MemoDevice_{name}.png')
    bpy.ops.render.render(write_still=True)
floor.hide_render=False
bpy.data.objects['Key'].location.z=.3
aim(bpy.data.objects['Key'])
cam.location=views['Hero']
aim(cam)
bpy.ops.object.select_all(action='DESELECT')
model.select_set(True)
bpy.context.view_layer.objects.active=model
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/f'{NAME}.blend'))
print('MEMO_MODEL_VALIDATION_PASSED '+json.dumps(manifest['size_cm']))
