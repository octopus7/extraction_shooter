"""Build the combat robot family without opening or changing a live Blender file.

Coordinates in this source are UE-oriented centimetres (+X front, +Y right).
Stored Blender geometry is metres, +X forward, Z up. Exported FBX files carry
unit metadata and contain one joined, UV-unwrapped render mesh at local origin.
"""
import bpy
import bmesh
import json
import math
from pathlib import Path
from mathutils import Vector

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / 'Art/CombatPatterns/Robots'
OUT.mkdir(parents=True, exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True)
scene = bpy.context.scene
scene.unit_settings.system = 'METRIC'
scene.unit_settings.scale_length = 1.0
MATERIALS = {
    'CP_Armor': ((0.70, 0.67, 0.56, 1), .34, .38),
    'CP_Dark': ((0.043, 0.060, 0.065, 1), .75, .33),
    'CP_Teal': ((0.055, 0.27, 0.28, 1), .42, .35),
    'CP_Amber': ((1.0, .30, .035, 1), .20, .28),
    'CP_Rubber': ((.018, .024, .027, 1), .0, .73),
}
mats = {}
for key, (color, metal, roughness) in MATERIALS.items():
    m = bpy.data.materials.new(key)
    m.diffuse_color = color
    m.use_nodes = True
    p = m.node_tree.nodes.get('Principled BSDF')
    p.inputs['Base Color'].default_value = color
    p.inputs['Metallic'].default_value = metal
    p.inputs['Roughness'].default_value = roughness
    if key == 'CP_Amber':
        p.inputs['Emission Color'].default_value = color
        p.inputs['Emission Strength'].default_value = 2.5
    mats[key] = m

parts = []
assets = {}
entries = []


def cv(v):
    return Vector(v) / 100


def finish(obj, label, mat, bevel=0, smooth=True):
    obj.name = label
    bpy.ops.object.select_all(action='DESELECT')
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    if bevel:
        mod = obj.modifiers.new('Machined edge radii', 'BEVEL')
        mod.width = bevel / 100
        mod.segments = 1
        bpy.ops.object.modifier_apply(modifier=mod.name)
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bmesh.ops.remove_doubles(bm, verts=list(bm.verts), dist=1e-7)
    bmesh.ops.recalc_face_normals(bm, faces=list(bm.faces))
    bm.to_mesh(obj.data)
    bm.free()
    obj.data.materials.clear()
    obj.data.materials.append(mats[mat])
    for poly in obj.data.polygons:
        poly.use_smooth = smooth
    if smooth and bevel:
        mod = obj.modifiers.new('Weighted manufacturing normals', 'WEIGHTED_NORMAL')
        mod.keep_sharp = True
        bpy.ops.object.modifier_apply(modifier=mod.name)
    parts.append(obj)
    return obj


def box(label, loc, size, mat='CP_Armor', bevel=1, rotation=0):
    bpy.ops.mesh.primitive_cube_add(size=1, location=cv(loc))
    obj = bpy.context.object
    obj.dimensions = Vector(size) / 100
    obj.rotation_euler.y = math.radians(rotation)
    return finish(obj, label, mat, bevel)


def cylinder(label, loc, radius, depth, mat='CP_Dark', axis=(0, 0, 1), vertices=20, bevel=.4):
    bpy.ops.mesh.primitive_cylinder_add(vertices=vertices, radius=radius / 100,
                                      depth=depth / 100, location=cv(loc))
    obj = bpy.context.object
    obj.rotation_euler = cv(axis).to_track_quat('Z', 'Y').to_euler()
    return finish(obj, label, mat, bevel)


def rod(label, a, b, radius, mat='CP_Dark', vertices=12):
    a, b = Vector(a), Vector(b)
    return cylinder(label, (a + b) / 2, radius, (b - a).length, mat,
                    axis=b-a, vertices=vertices, bevel=.25)


def surface(label, verts, faces, mat='CP_Armor', bevel=0, smooth=False):
    mesh = bpy.data.meshes.new(label)
    mesh.from_pydata([cv(v) for v in verts], [], faces)
    mesh.update()
    obj = bpy.data.objects.new(label, mesh)
    scene.collection.objects.link(obj)
    return finish(obj, label, mat, bevel, smooth)


def patch(label, az0, az1, p0, p1, radius, mat='CP_Armor'):
    """Closed low-poly spherical armor tile, no coplanar gaps or tiny strips."""
    nu, nv = 4, 3
    verts, faces = [], []
    for r in (radius, radius - 1.2):
        for j in range(nv + 1):
            p = math.radians(p0 + (p1 - p0) * j / nv)
            for i in range(nu + 1):
                a = math.radians(az0 + (az1 - az0) * i / nu)
                verts.append((r*math.sin(p)*math.cos(a), r*math.sin(p)*math.sin(a), r*math.cos(p)))
    count = (nu + 1) * (nv + 1)
    for j in range(nv):
        for i in range(nu):
            a = j * (nu + 1) + i
            faces.extend([(a, a+1, a+nu+2, a+nu+1),
                          (count+a+nu+1, count+a+nu+2, count+a+1, count+a)])
    perimeter = list(range(nu+1)) + [(j+1)*(nu+1)-1 for j in range(1,nv+1)]
    perimeter += [nv*(nu+1)+i for i in range(nu-1,-1,-1)]
    perimeter += [j*(nu+1) for j in range(nv-1,0,-1)]
    for i,a in enumerate(perimeter):
        b = perimeter[(i+1) % len(perimeter)]
        faces.append((a, count+a, count+b, b))
    return surface(label, verts, faces, mat, smooth=True)


def prism(label, profile_xz, ycenter, width, mat='CP_Armor', bevel=1):
    n = len(profile_xz)
    verts = [(x,ycenter+y,z) for y in (-width/2,width/2) for x,z in profile_xz]
    faces = [tuple(range(n-1,-1,-1)), tuple(range(n,2*n))]
    faces += [(i,(i+1)%n,(i+1)%n+n,i+n) for i in range(n)]
    return surface(label, verts, faces, mat, bevel, smooth=True)


def end_asset(name, expected_height=None):
    global parts
    bpy.ops.object.select_all(action='DESELECT')
    for obj in parts:
        obj.select_set(True)
    bpy.context.view_layer.objects.active = parts[0]
    bpy.ops.object.join()
    obj = bpy.context.object
    obj.name = name
    bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)
    scene.cursor.location = (0,0,0)
    bpy.ops.object.origin_set(type='ORIGIN_CURSOR')
    # Normalize stable five-slot palette order across every exported mesh.
    old = [m.name for m in obj.data.materials]
    keys = list(MATERIALS)
    indices = [keys.index(old[p.material_index]) for p in obj.data.polygons]
    obj.data.materials.clear()
    for key in keys:
        obj.data.materials.append(mats[key])
    for poly, idx in zip(obj.data.polygons, indices):
        poly.material_index = idx
    bpy.ops.object.mode_set(mode='EDIT')
    bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.uv.smart_project(angle_limit=math.radians(66), island_margin=.02)
    bpy.ops.object.mode_set(mode='OBJECT')
    obj.data.calc_loop_triangles()
    assert all(math.isfinite(c) for v in obj.data.vertices for c in v.co)
    assert all(t.area > 1e-12 for t in obj.data.loop_triangles), name
    bounds = [[min(v.co[a] for v in obj.data.vertices), max(v.co[a] for v in obj.data.vertices)] for a in range(3)]
    dims = [(b-a)*100 for a,b in bounds]
    assert obj.location.length < 1e-7 and max(abs(c-1) for c in obj.scale) < 1e-7
    if expected_height is not None:
        assert abs(dims[2]-expected_height) < 2, (name,dims)
    bpy.ops.export_scene.fbx(filepath=str(OUT / (name + '.fbx')), use_selection=True,
                            object_types={'MESH'}, add_leaf_bones=False,
                            apply_unit_scale=True, apply_scale_options='FBX_SCALE_ALL',
                            axis_forward='-Y', axis_up='Z', use_mesh_modifiers=True,
                            mesh_smooth_type='FACE', bake_anim=False, path_mode='AUTO')
    entry = {'name':name, 'file':name+'.fbx', 'triangles':len(obj.data.loop_triangles),
             'vertices':len(obj.data.vertices), 'dimensions_ue_cm':[round(dims[0],4),round(dims[1],4),round(dims[2],4)],
             'bounds_blender_m':bounds, 'material_slots':keys,
             'uv_layers':[l.name for l in obj.data.uv_layers], 'pivot_blender_m':list(obj.location),
             'blender_forward':'+X', 'unreal_forward':'+X', 'fbx_axis_forward':'-Y'}
    entries.append(entry)
    assets[name] = obj
    parts = []
    return obj


# Roll / walk scout: armor seams remain visible from gameplay height.
bpy.ops.mesh.primitive_uv_sphere_add(segments=24, ring_count=12, radius=.337)
finish(bpy.context.object, 'Sphere mechanical core', 'CP_Rubber')
for k in range(8):
    az0, az1 = k*45+2.2, (k+1)*45-2.2
    patch('Upper segmented armor %02d'%k, az0,az1,14,66,36,
          'CP_Teal' if k in (2,5) else 'CP_Armor')
    patch('Lower segmented armor %02d'%k, az0,az1,114,166,36,
          'CP_Teal' if k in (2,5) else 'CP_Armor')
    if k not in (0,7):
        patch('Equatorial armor %02d'%k,az0,az1,70,110,35.7,
              'CP_Teal' if k in (2,5) else 'CP_Armor')
    else:
        patch('Front visor surround %02d'%k,az0,az1,69,79,35.5,'CP_Dark')
        patch('Front chin %02d'%k,az0,az1,101,111,35.5,'CP_Armor')
cylinder('Top docking rim',(0,0,34.1),9.2,2.0,'CP_Dark',vertices=24)
cylinder('Teal top cap',(0,0,35.2),7.6,1.6,'CP_Teal',vertices=24)
box('Top amber index',(0,0,36.15),(7.5,2.6,.55),'CP_Amber',.25)
cylinder('Lower leg socket',(0,0,-34),8.5,2.0,'CP_Dark',vertices=20)
for sign in (-1,1):
    cylinder('Lateral roll hub',(0,sign*34.0,0),10.5,2.5,'CP_Dark',axis=(0,1,0),vertices=20)
    cylinder('Lateral hub armor',(0,sign*35.45,0),7.6,1.4,'CP_Teal',axis=(0,1,0),vertices=20)
    cylinder('Hub center fastener',(0,sign*36.3,0),3.0,.65,'CP_Armor',axis=(0,1,0),vertices=8,bevel=.2)
end_asset('SM_CP_RobotShell')

box('Visor recessed casing',(32.8,0,2.5),(5.0,33.0,13.0),'CP_Dark',2.0)
box('Visor black glass',(35.1,0,2.5),(1.7,29,9),'CP_Rubber',1.2)
for sign in (-1,1):
    box('Amber optical slit',(36.0,sign*7.5,3.2),(1.3,11.4,3.0),'CP_Amber',.75)
box('Optical center divider',(36.2,0,2.6),(1.3,2.6,8.5),'CP_Dark',.45)
end_asset('SM_CP_RobotEye')

box('Telescoping hip mount',(-1,0,24),(11,12,18),'CP_Dark',1.0)
box('Hip piston facing',(5.0,0,26),(1.6,7.5,13),'CP_Armor',.35)
cylinder('Upper leg joint',(0,0,15.0),6.0,17.0,'CP_Dark',axis=(0,1,0),vertices=16)
for sign in (-1,1):
    cylinder('Upper joint bolt',(0,sign*8.7,15),3.5,1.0,'CP_Armor',axis=(0,1,0),vertices=8,bevel=.25)
box('Upper strut',(-1,0,5.5),(11,12,19),'CP_Dark',1.1)
box('Shin armor',(2.2,0,-8),(15,15,19),'CP_Teal',2.0,rotation=-8)
box('Front shin insert',(9.0,0,-7.0),(2,9,11),'CP_Armor',.8,rotation=-8)
rod('Rear piston ram',(-7,0,9),(-6,0,-15),2.1,'CP_Armor')
rod('Piston lower barrel',(-6.5,0,-1),(-6,0,-17),3.1,'CP_Dark')
cylinder('Ankle actuator',(1.5,0,-17.5),5,14,'CP_Dark',axis=(0,1,0),vertices=16)
end_asset('SM_CP_RobotLeg',55.5)

box('Boot rubber sole',(0,0,-3.3),(32,21,3.4),'CP_Rubber',1.4)
box('Armored toe',(1,0,.6),(30,20,8.8),'CP_Armor',2.0)
box('Teal boot instep',(-4,0,4.1),(12,13,1.5),'CP_Teal',.6)
for y in (-6.2,0,6.2):
    box('Toe impact rib',(15.5,y,.0),(1.0,2,5),'CP_Dark',.4)
end_asset('SM_CP_RobotFoot',10)

# Heavy rammer: broad armored nose and readable tread-driven silhouette.
box('Main armored core',(-8,0,72),(70,59,71),'CP_Dark',6)
box('Lower hull',(-2,0,36),(84,62,26),'CP_Teal',4)
prism('Upper sloped carapace',[(-38,88),(-35,122),(-20,139),(17,139),(33,122),(33,88)],0,62,'CP_Armor',2.3)
for side in (-1,1):
    # The rubber track is a closed extruded belt with inner void.
    outer,inner = [],[]
    for cx,angles in ((32,range(-90,91,18)),(-32,range(90,271,18))):
        for angle in angles:
            a=math.radians(angle)
            outer.append((cx+20*math.cos(a),22+20*math.sin(a)))
            inner.append((cx+14*math.cos(a),22+14*math.sin(a)))
    n=len(outer)
    verts=[]
    for y in (side*36-side*10,side*36+side*10):
        verts.extend([(x,y,z) for x,z in outer])
        verts.extend([(x,y,z) for x,z in inner])
    faces=[]
    for i in range(n):
        j=(i+1)%n
        faces += [(i,j,n+j,n+i),(2*n+i,3*n+i,3*n+j,2*n+j),
                  (i,2*n+i,2*n+j,j),(n+i,n+j,3*n+j,3*n+i)]
    surface('Rubber continuous track',verts,faces,'CP_Rubber',smooth=False)
    for i,(x,z) in enumerate(outer):
        x2,z2=outer[(i+1)%n]
        angle=math.degrees(math.atan2(z2-z,x2-x))
        box('Raised tread shoe',((x+x2)/2,side*36,(z+z2)/2),
            (math.hypot(x2-x,z2-z)*.86,21,2.6),'CP_Dark',.4,rotation=-angle)
    for x in (-31,0,31):
        cylinder('Road wheel',(x,side*36,22),13,17,'CP_Dark',axis=(0,1,0),vertices=20,bevel=.6)
        cylinder('Wheel teal rim',(x,side*46,22),9.5,1.8,'CP_Teal',axis=(0,1,0),vertices=16)
        cylinder('Wheel center nut',(x,side*47.2,22),3.9,1.0,'CP_Armor',axis=(0,1,0),vertices=8)
    prism('Track armored fender',[(-50,43),(-40,54),(39,54),(49,43)],side*36,23,'CP_Armor',1.4)
    prism('Side teal armor',[(-34,70),(-32,119),(-18,130),(10,130),(24,113),(26,70)],side*31.5,5,'CP_Teal',1.2)
    for x in (-23,-13,-3,7):
        box('Inset side vent',(x,side*34.2,104),(4.4,1.5,18),'CP_Rubber',.45,rotation=-15)
    box('Fender amber position marker',(34,side*37,55.0),(10,8,2),'CP_Amber',.4)

# Thick front impact plate comprises three chamfered slabs, broadest silhouette.
prism('Rammer central shield',[(44,29),(52,40),(50,105),(34,125),(24,124),(34,33)],0,41,'CP_Armor',1.8)
for side in (-1,1):
    prism('Shield outer cheek',[(40,34),(49,44),(46,102),(32,119),(23,115),(31,39)],side*28.5,17,'CP_Armor',1.7)
    prism('Shield teal stripe',[(49.3,47),(51.4,49),(49.5,95),(46.7,99)],side*14.0,6.0,'CP_Teal',.35)
    for z in (50,96):
        cylinder('Shield heavy fastener',(50,side*31,z),2.5,2.2,'CP_Dark',axis=(1,0,0),vertices=8,bevel=.2)
box('Front lower impact bar',(48,0,31),(13,77,13),'CP_Dark',2)
box('Impact bar warm insert',(55,0,31),(1.4,31,5),'CP_Armor',.4)
box('Sensor armored housing',(14,0,138),(30,48,22),'CP_Dark',4)
box('Sensor upper hood',(13,0,148),(35,53,4),'CP_Armor',1.4)
box('Sensor recessed visor',(30.0,0,139),(2.0,41,10),'CP_Rubber',1.8)
for side in (-1,1):
    box('Heavy amber optical bar',(31.3,side*10.4,139),(1.0,16,3.2),'CP_Amber',.8)
box('Sensor visor center',(32,0,139),(2,4,12),'CP_Dark',.5)
cylinder('Rear service cap',(-22,0,140),12,3,'CP_Teal',vertices=24,bevel=.7)
cylinder('Hatch locking lug',(-22,0,142),4,1.6,'CP_Dark',vertices=8)
for side in (-1,1):
    box('Rear heat exchanger',(-42,side*16,92),(8,21,42),'CP_Dark',1.5)
    for z in (79,86,93,100,107):
        box('Cooling fin',(-47,side*16,z),(2,18,2),'CP_Rubber',.35)
end_asset('SM_CP_ChargeChassis',150)

print('TRIANGLE_BUDGET '+json.dumps({e['name']:e['triangles'] for e in entries}),flush=True)
assert sum(e['triangles']*(2 if e['name'] in ('SM_CP_RobotLeg','SM_CP_RobotFoot') else 1)
           for e in entries if e['name']!='SM_CP_ChargeChassis') < 8000
assert entries[-1]['triangles'] < 18000

# Self-contained FBX round-trip validation; source objects are retained in scene.
checks=[]
for entry in entries:
    before=set(bpy.data.objects)
    bpy.ops.import_scene.fbx(filepath=str(OUT/entry['file']))
    imported=[o for o in bpy.data.objects if o not in before]
    meshes=[o for o in imported if o.type=='MESH']
    assert len(meshes)==1,entry['name']
    obj=meshes[0]
    obj.data.calc_loop_triangles()
    bounds=[[min((obj.matrix_world@v.co)[a] for v in obj.data.vertices),
             max((obj.matrix_world@v.co)[a] for v in obj.data.vertices)] for a in range(3)]
    error=max(abs(bounds[a][b]-entry['bounds_blender_m'][a][b]) for a in range(3) for b in range(2))
    assert error<1e-5,(entry['name'],error)
    assert len(obj.data.loop_triangles)==entry['triangles']
    assert obj.data.uv_layers.active and all(m is not None for m in obj.data.materials)
    assert len(obj.data.materials)==5
    checks.append({'name':entry['name'],'max_bounds_error_m':error,'passed':True})
    for o in imported:
        bpy.data.objects.remove(o,do_unlink=True)

# Source assets remain hidden in an authored-mesh collection, preview duplicates
# show the exact exported geometry both folded and standing beside the rammer.
source_collection=bpy.data.collections.new('EXPORTS - local origins')
scene.collection.children.link(source_collection)
for obj in assets.values():
    for collection in list(obj.users_collection):
        collection.objects.unlink(obj)
    source_collection.objects.link(obj)
    obj.hide_render=True
    obj.hide_set(True)


def instance(asset, label, loc, yaw=0):
    obj=assets[asset].copy()
    obj.data=assets[asset].data
    scene.collection.objects.link(obj)
    obj.name=label
    obj.location=cv(loc)
    obj.rotation_euler.z=math.radians(yaw)
    obj.hide_render=False
    obj.hide_set(False)
    return obj


instance('SM_CP_RobotShell','ROLL - shell',(0,-145,36))
instance('SM_CP_RobotEye','ROLL - eyes',(0,-145,36))
instance('SM_CP_RobotShell','WALK - shell',(0,-25,96))
instance('SM_CP_RobotEye','WALK - eyes',(0,-25,96))
for side in (-1,1):
    instance('SM_CP_RobotLeg','WALK - leg',(0,-25+side*17,33))
    instance('SM_CP_RobotFoot','WALK - foot',(3,-25+side*17,5))
instance('SM_CP_ChargeChassis','RAMMER',(0,112,0))

floor=bpy.data.materials.new('Preview floor')
floor.diffuse_color=(.038,.052,.058,1)
floor.use_nodes=True
floor.node_tree.nodes.get('Principled BSDF').inputs['Base Color'].default_value=floor.diffuse_color
floor.node_tree.nodes.get('Principled BSDF').inputs['Roughness'].default_value=.64
bpy.ops.mesh.primitive_plane_add(size=200,location=(0,0,-.006))
bpy.context.object.name='Studio floor - preview only'
bpy.context.object.data.materials.append(floor)
world=bpy.data.worlds.new('Combat art studio')
world.use_nodes=True
world.node_tree.nodes['Background'].inputs[0].default_value=(.13,.17,.20,1)
world.node_tree.nodes['Background'].inputs[1].default_value=.35
scene.world=world
for label,loc,energy,color,size in [
        ('Large soft key',(-3,-4,6),950,(1.0,.91,.77),5),
        ('Cool rim',(2,3,4),1100,(.50,.75,1),4),
        ('Front fill',(1,-4,2),250,(.75,.87,1),3)]:
    data=bpy.data.lights.new(label,'AREA')
    data.energy=energy
    data.color=color
    data.shape='DISK'
    data.size=size
    obj=bpy.data.objects.new(label,data)
    scene.collection.objects.link(obj)
    obj.location=loc
    obj.rotation_euler=(Vector((0,0,.6))-obj.location).to_track_quat('-Z','Y').to_euler()
bpy.ops.object.camera_add(location=(6.6,-3.4,4.1))
camera=bpy.context.object
camera.name='Family art review camera'
camera.rotation_euler=(Vector((0,-.05,.70))-camera.location).to_track_quat('-Z','Y').to_euler()
camera.data.type='ORTHO'
camera.data.ortho_scale=4.80
scene.camera=camera
scene.render.engine='CYCLES'
scene.cycles.samples=40
scene.cycles.use_denoising=True
scene.render.resolution_x=1800
scene.render.resolution_y=1100
scene.render.resolution_percentage=100
scene.view_settings.view_transform='AgX'
scene.render.image_settings.file_format='PNG'
scene.render.filepath=str(OUT/'preview.png')
scene.render.film_transparent=False
for area in bpy.context.screen.areas:
    if area.type=='VIEW_3D':
        area.spaces.active.region_3d.view_perspective='CAMERA'
        area.spaces.active.shading.type='MATERIAL'
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'CombatRobots.blend'))
bpy.ops.render.render(write_still=True)
manifest={'collection':'Combat pattern robot family','coordinate_system':'Blender +X physical front, Z up, metres; FBX -Y forward/Zup; UE +X front, cm',
          'assets':entries,'fbx_roundtrip':checks,
          'assembly_ue_cm':{'shell':[0,0,30],'eye_relative_to_shell':[0,0,0],
                            'left_leg':[0,-17,-33],'right_leg':[0,17,-33],
                            'left_foot':[3,-17,-61],'right_foot':[3,17,-61],
                            'charger_pivot':'ground center'},
          'minion_assembled_triangles':sum(e['triangles']*(2 if e['name'] in ('SM_CP_RobotLeg','SM_CP_RobotFoot') else 1)
                                         for e in entries if e['name']!='SM_CP_ChargeChassis'),
          'validation_passed':True}
(OUT/'manifest.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
print('COMBAT_ROBOT_ART_VALIDATED '+json.dumps({e['name']:e['triangles'] for e in entries}))
