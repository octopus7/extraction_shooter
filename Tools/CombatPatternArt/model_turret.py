"""Author the reusable missile turret and payload; run with Blender --background.

Source coordinates are meters, +X front and +Z up.  FBX exports carry meter units
for UE's automatic centimeter conversion.  Each exported mesh is rooted at zero;
the .blend presents the assembled turret plus a separate missile for art review.
"""
import bpy
import json
import math
from pathlib import Path
from mathutils import Vector

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "Art" / "CombatPatterns" / "Turret"
OUT.mkdir(parents=True, exist_ok=True)
bpy.ops.object.select_all(action='SELECT')
bpy.ops.object.delete(use_global=False)
for datablock in list(bpy.data.materials):
    bpy.data.materials.remove(datablock)

PALETTE = {
    "CP_Armor": ((0.64, 0.61, 0.51, 1), .48, .34),
    "CP_Dark": ((.043, .059, .068, 1), .78, .3),
    "CP_Teal": ((.075, .29, .29, 1), .55, .32),
    "CP_Amber": ((1.0, .33, .028, 1), .32, .23),
    "CP_Rubber": ((.018, .023, .026, 1), .05, .74),
}
MATERIALS = {}
for name, (color, metal, rough) in PALETTE.items():
    mat = bpy.data.materials.new(name)
    mat.diffuse_color = color
    mat.use_nodes = True
    bsdf = mat.node_tree.nodes.get('Principled BSDF')
    bsdf.inputs['Base Color'].default_value = color
    bsdf.inputs['Metallic'].default_value = metal
    bsdf.inputs['Roughness'].default_value = rough
    if name == 'CP_Amber':
        bsdf.inputs['Emission Color'].default_value = color
        bsdf.inputs['Emission Strength'].default_value = 2.0
    MATERIALS[name] = mat

parts = []
def finish(obj, name, material='CP_Armor', bevel=0):
    obj.name = name
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    obj.data.materials.append(MATERIALS[material])
    if bevel:
        modifier = obj.modifiers.new('Manufactured edge chamfer', 'BEVEL')
        modifier.width = bevel
        modifier.segments = 1
        modifier.affect = 'EDGES'
        bpy.context.view_layer.objects.active = obj
        bpy.ops.object.modifier_apply(modifier=modifier.name)
    parts.append(obj)
    return obj

def box(name, loc, size, material='CP_Armor', bevel=.012, rotation=None):
    bpy.ops.mesh.primitive_cube_add(size=1, location=loc)
    obj = bpy.context.object
    obj.dimensions = size
    if rotation:
        obj.rotation_euler = rotation
    return finish(obj, name, material, bevel)

def cylinder(name, loc, radius, depth, material='CP_Dark', vertices=12, rotation=None, bevel=.006):
    bpy.ops.mesh.primitive_cylinder_add(vertices=vertices, radius=radius, depth=depth, location=loc)
    obj = bpy.context.object
    if rotation:
        obj.rotation_euler = rotation
    return finish(obj, name, material, bevel)

def cone(name, loc, radius1, radius2, depth, material='CP_Armor', vertices=12):
    bpy.ops.mesh.primitive_cone_add(vertices=vertices, radius1=radius1, radius2=radius2, depth=depth, location=loc)
    return finish(bpy.context.object, name, material, .002)

def ring(name, loc, outside, inside, depth, material='CP_Dark', count=16):
    verts = []
    for z in (-depth / 2, depth / 2):
        for radius in (outside, inside):
            for n in range(count):
                a = 2 * math.pi * n / count
                verts.append((loc[0] + radius * math.cos(a), loc[1] + radius * math.sin(a), loc[2] + z))
    faces = []
    for n in range(count):
        j = (n + 1) % count
        faces.extend([(n,j,count+j,count+n),(2*count+n,3*count+n,3*count+j,2*count+j),
                      (n,2*count+n,2*count+j,j),(count+n,count+j,3*count+j,3*count+n)])
    mesh = bpy.data.meshes.new(name)
    mesh.from_pydata(verts, [], faces)
    mesh.update()
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.collection.objects.link(obj)
    obj.data.materials.append(MATERIALS[material])
    parts.append(obj)
    return obj

def bolt(name, loc, radius=.013, axis='Z'):
    rot = (0,math.pi/2,0) if axis=='X' else ((math.pi/2,0,0) if axis=='Y' else None)
    return cylinder(name, loc, radius, .011, 'CP_Dark', 6, rot, 0)

def beam(name, start, end, width, depth, material='CP_Dark', bevel=.008):
    middle = (Vector(start) + Vector(end)) / 2
    vec = Vector(end) - Vector(start)
    obj = box(name, middle, (width, depth, vec.length), material, bevel)
    obj.rotation_euler = vec.to_track_quat('Z', 'Y').to_euler()
    return obj

def join_asset(name):
    bpy.ops.object.select_all(action='DESELECT')
    for obj in parts:
        obj.select_set(True)
    bpy.context.view_layer.objects.active = parts[0]
    bpy.ops.object.join()
    obj = bpy.context.object
    obj.name = name
    bpy.context.scene.cursor.location = (0,0,0)
    bpy.ops.object.origin_set(type='ORIGIN_CURSOR')
    bpy.ops.object.transform_apply(location=False,rotation=True,scale=True)
    # Slot order is contractual across imported art so the runtime can assign
    # shared materials by name with consistent semantics on all robot parts.
    old_materials = list(obj.data.materials)
    indices = [list(PALETTE).index(old_materials[p.material_index].name) for p in obj.data.polygons]
    obj.data.materials.clear()
    for mat in MATERIALS.values():
        obj.data.materials.append(mat)
    for poly, index in zip(obj.data.polygons, indices):
        poly.material_index = index
        poly.use_smooth = False
    bpy.ops.object.mode_set(mode='EDIT')
    bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.mesh.normals_make_consistent(inside=False)
    bpy.ops.uv.smart_project(angle_limit=math.radians(66), island_margin=.02)
    bpy.ops.object.mode_set(mode='OBJECT')
    parts.clear()
    return obj

# Four planted corner feet and broad rotating pedestal: the silhouette reads
# immediately as a deployed machine even from the gameplay overhead camera.
for sx in (-1,1):
    for sy in (-1,1):
        x,y = sx*.335,sy*.335
        box('Elastomer planted sole', (x,y,.039),(.30,.26,.078),'CP_Rubber',.019)
        box('Chamfered stabilizer shoe',(x,y,.095),(.31,.27,.096),'CP_Armor',.025)
        box('Stabilizer teal inset',(x,y,.147),(.165,.11,.009),'CP_Teal',.004)
        beam('Stabilizer diagonal arm',(sx*.15,sy*.15,.25),(x,y,.145),.115,.13)
        cylinder('Foot hinge',(sx*.245,sy*.245,.193),.049,.125,'CP_Dark',12,(math.pi/2,0,0),.004)
        for offset in (-.091,.091):
            bolt('Recessed shoe bolt',(x+offset,y,.151),.014)
        box('Foot perimeter amber marker',(x+sx*.148,y,.111),(.011,.078,.019),'CP_Amber',.003)
cylinder('Turret bearing rubber isolation',(0,0,.115),.255,.13,'CP_Rubber',16,bevel=.007)
cylinder('Octagonal pedestal lower skirt',(0,0,.203),.292,.13,'CP_Armor',8,bevel=.015)
cylinder('Teal pedestal belt',(0,0,.278),.264,.047,'CP_Teal',16,bevel=.004)
cylinder('Dark bearing drive',(0,0,.331),.232,.061,'CP_Dark',16,bevel=.008)
ring('Bearing top protective ring',(0,0,.378),.257,.182,.045,'CP_Armor',16)
cylinder('Head spindle',(0,0,.404),.128,.108,'CP_Dark',12,bevel=.008)
for n in range(8):
    angle=(n+.5)*math.tau/8
    bolt('Bearing fastener',(.225*math.cos(angle),.225*math.sin(angle),.403),.014)
for sy in (-1,1):
    box('Cooling bank',(0,sy*.269,.203),(.225,.02,.078),'CP_Dark',.004)
    for i in range(5):
        box('Vertical radiator rib',(-.082+i*.041,sy*.282,.203),(.013,.012,.061),'CP_Teal',.003)
base = join_asset('SM_CP_TurretBase')

# The control head, centered on its yaw pivot, carries robust cheeks, a
# recessed amber target lens, lifting handles and an accessible rear service bay.
box('Main armored shell',(0,0,0),(.51,.44,.36),'CP_Armor',.05)
box('Lower floating dark frame',(0,0,-.188),(.46,.42,.095),'CP_Dark',.024)
box('Teal raised control crown',(-.012,0,.19),(.343,.305,.08),'CP_Teal',.02)
box('Crown service inset',(-.02,0,.234),(.215,.188,.012),'CP_Dark',.004)
for sy in (-1,1):
    box('Upper armor shoulder',(0,sy*.213,.128),(.419,.059,.104),'CP_Armor',.012)
    cylinder('Tube tilt trunnion',(0,sy*.264,-.018),.119,.099,'CP_Dark',12,(math.pi/2,0,0),.006)
    cylinder('Tube trunnion cap',(0,sy*.319,-.018),.073,.02,'CP_Teal',8,(math.pi/2,0,0),.004)
    # Exposed loop handles atop shell.
    for x in (-.113,.113):
        box('Top handle foot',(x,sy*.146,.243),(.032,.032,.049),'CP_Dark',.005)
    box('Top carrying handle',(0,sy*.146,.266),(.258,.032,.03),'CP_Dark',.006)
box('Forward optical socket',(.261,0,.018),(.053,.264,.145),'CP_Dark',.024)
box('Lens teal rim',(.292,0,.022),(.023,.193,.091),'CP_Teal',.015)
box('Amber target slit',(.307,0,.022),(.012,.15,.045),'CP_Amber',.008)
for sy in (-1,1):
    bolt('Optic case hex fastener',(.29,sy*.107,.019),.012,'X')
    box('Front lower amber tick',(.267,sy*.162,-.117),(.012,.054,.018),'CP_Amber',.003)
box('Rear access door',(-.264,0,-.021),(.027,.317,.25),'CP_Dark',.017)
box('Rear access teal armor',(-.279,0,.015),(.012,.259,.114),'CP_Teal',.006)
for i in range(6):
    box('Rear heat exchanger fin',(-.286,-.095+i*.038,-.078),(.018,.016,.064),'CP_Armor',.003)
for sy in (-1,1):
    for z in (-.146,.15):
        bolt('Rear service bolt',(-.283,sy*.119,z),.012,'X')
head = join_asset('SM_CP_TurretHead')

# One reusable vertical launch pod, rooted at its lower mounting face. Actual
# recessed open muzzle, dark bore, clamps and a discrete loaded tip are modeled.
cylinder('Tube rear shock mount',(0,0,.058),.136,.116,'CP_Dark',12,bevel=.009)
cylinder('Launch pod armored body',(0,0,.25),.132,.34,'CP_Armor',12,bevel=.012)
for z in (.146,.383):
    ring('Dark pod clamp',(0,0,z),.143,.121,.033,'CP_Dark',12)
    box('Clamp release buckle',(.148,0,z),(.041,.083,.053),'CP_Teal',.006)
for sy in (-1,1):
    box('Pod side teal cassette',(0,sy*.128,.266),(.112,.024,.159),'CP_Teal',.007)
    box('Loaded indicator',(.064,sy*.137,.276),(.022,.01,.097),'CP_Amber',.003)
    for z in (.219,.322):
        bolt('Pod cassette bolt',(-.038,sy*.145,z),.010,'Y')
ring('Raised launch muzzle',(0,0,.434),.149,.101,.062,'CP_Armor',12)
ring('Teal muzzle identifier ring',(0,0,.473),.139,.104,.018,'CP_Teal',12)
ring('Deep graphite bore liner',(0,0,.429),.104,.092,.108,'CP_Dark',12)
cylinder('Deep bore shadow',(0,0,.378),.092,.006,'CP_Rubber',12,bevel=0)
cone('Loaded missile exposed tip',(0,0,.447),.079,.004,.143,'CP_Dark',12)
for n in range(4):
    a=n*math.pi/2
    bolt('Muzzle ring bolt',(.123*math.cos(a),.123*math.sin(a),.484),.011)
tube = join_asset('SM_CP_TurretTube')

# Falling missile is +Z nose, bottom-pivoted for predictable flight placement.
# Faceted nose, separate payload and drive sections, fins and exhaust cavity.
ring('Exhaust outer casing',(0,0,.058),.065,.040,.116,'CP_Dark',12)
ring('Exhaust interior',(0,0,.030),.043,.031,.044,'CP_Armor',12)
cylinder('Rocket propellant core',(0,0,.189),.071,.245,'CP_Dark',12,bevel=.004)
cylinder('Missile warm armored fuselage',(0,0,.482),.082,.385,'CP_Armor',12,bevel=.008)
cylinder('Teal missile forward band',(0,0,.665),.084,.068,'CP_Teal',12,bevel=.003)
cone('Shaped penetrator',(0,0,.811),.08,.003,.278,'CP_Armor',12)
ring('Payload separation line',(0,0,.612),.084,.078,.018,'CP_Dark',12)
ring('Fin assembly band',(0,0,.204),.076,.068,.030,'CP_Teal',12)
for n in range(4):
    angle=n*math.pi/2
    # Fin profile tapers at the top and has a squared root; local clockwise
    # surfaces are corrected before export.
    coords=[(.063,-.012,.065),(.115,-.012,.089),(.112,-.012,.255),(.066,-.012,.34),
            (.063,.012,.065),(.115,.012,.089),(.112,.012,.255),(.066,.012,.34)]
    coords=[(x*math.cos(angle)-y*math.sin(angle),x*math.sin(angle)+y*math.cos(angle),z) for x,y,z in coords]
    mesh=bpy.data.meshes.new('Missile stabilizer fin')
    mesh.from_pydata(coords,[],[(0,3,2,1),(4,5,6,7),(0,1,5,4),(1,2,6,5),(2,3,7,6),(3,0,4,7)])
    mesh.update()
    obj=bpy.data.objects.new('Teal clipped stabilizer',mesh)
    bpy.context.collection.objects.link(obj)
    obj.data.materials.append(MATERIALS['CP_Teal'])
    parts.append(obj)
    # Long high-contrast guide stripe through each of the four body faces.
    x,y=.081*math.cos(angle),.081*math.sin(angle)
    box('Amber guidance marker',(x,y,.525),(.009,.026,.083),'CP_Amber',.002,(0,0,angle))
missile = join_asset('SM_CP_Missile')

assets=[base,head,tube,missile]
manifest={
    'coordinate_system': 'Source meters; +X front; +Z up. FBX axis_forward=-Y, axis_up=Z; unit conversion to UE centimeters.',
    'material_slot_order':list(PALETTE),
    'assembly_ue_cm': {
        'base': {'mesh':base.name,'location':[0,0,0],'rotation_pitch_yaw_roll':[0,0,0],'scale':[1,1,1]},
        'head': {'mesh':head.name,'location':[0,0,69],'rotation_pitch_yaw_roll':[0,0,0],'scale':[1,1,1]},
        'left_tube_relative_head': {'mesh':tube.name,'location':[0,-28,-4],'rotation_pitch_yaw_roll':[0,0,0],'scale':[1,1,1]},
        'right_tube_relative_head': {'mesh':tube.name,'location':[0,28,-4],'rotation_pitch_yaw_roll':[0,0,0],'scale':[1,1,1]},
        'missile': {'mesh':missile.name,'pivot':'exhaust bottom center','local_nose_axis':'+Z','falling_rotation_pitch_yaw_roll':[180,0,0],'scale':[1,1,1]},
    },
    'assets':[],
}
for obj in assets:
    bpy.ops.object.select_all(action='DESELECT')
    obj.select_set(True)
    bpy.context.view_layer.objects.active=obj
    bpy.ops.export_scene.fbx(filepath=str(OUT/(obj.name+'.fbx')), use_selection=True,
        object_types={'MESH'},axis_forward='-Y',axis_up='Z',apply_unit_scale=True,
        apply_scale_options='FBX_SCALE_UNITS',bake_space_transform=False,mesh_smooth_type='FACE',
        use_mesh_modifiers=True,use_triangles=True,add_leaf_bones=False,bake_anim=False,
        path_mode='AUTO')
    obj.data.calc_loop_triangles()
    points=[obj.matrix_world@Vector(p) for p in obj.bound_box]
    mins=[min(p[i] for p in points)*100 for i in range(3)]
    maxs=[max(p[i] for p in points)*100 for i in range(3)]
    manifest['assets'].append({'name':obj.name,'file':obj.name+'.fbx','triangles':len(obj.data.loop_triangles),
        'vertices':len(obj.data.vertices),'dimensions_cm':[round(v*100,4) for v in obj.dimensions],
        'bounds_min_cm':[round(v,4) for v in mins],'bounds_max_cm':[round(v,4) for v in maxs],
        'pivot_cm':list(obj.location),'uv_maps':[uv.name for uv in obj.data.uv_layers],
        'material_slots':[mat.name for mat in obj.data.materials]})

# Validate the actual interchange files in isolated collections. Reimported
# object dimensions, UV presence, topology and finite normals must all match.
validation=[]
for source in assets:
    previous=set(bpy.data.objects)
    bpy.ops.import_scene.fbx(filepath=str(OUT/(source.name+'.fbx')), use_custom_normals=True)
    imported=[o for o in bpy.data.objects if o not in previous]
    imported_meshes=[o for o in imported if o.type=='MESH']
    assert len(imported_meshes)==1, (source.name, imported_meshes)
    obj=imported_meshes[0]
    delta=max(abs(a-b) for a,b in zip(source.dimensions,obj.dimensions))
    assert delta < .00002, (source.name,list(source.dimensions),list(obj.dimensions))
    assert len(obj.data.uv_layers)>0
    assert all(math.isfinite(v) for p in obj.data.polygons for v in p.normal)
    assert all(abs(v)<.000001 for v in obj.location), (source.name,list(obj.location))
    obj.data.calc_loop_triangles()
    validation.append({'name':source.name,'fbx_reimport_dimensions_cm':[round(v*100,4) for v in obj.dimensions],
        'fbx_reimport_triangles':len(obj.data.loop_triangles),'max_dimension_delta_m':delta,
        'pivot_origin_valid':True,'uv_valid':True,'finite_normals':True})
    for obj in imported:
        bpy.data.objects.remove(obj,do_unlink=True)
manifest['fbx_validation']=validation
assert sum(a['triangles'] for a in manifest['assets'][:2])+2*manifest['assets'][2]['triangles']<12000
assert manifest['assets'][3]['triangles']<2000

# Review scene uses instances of the actual exported assets. Both 3/4 and top
# renders share the same design; the .blend remains fully editable.
head.location=(0,0,.69)
tube.location=(0,-.28,.65)
other_tube=tube.copy()
other_tube.data=tube.data
other_tube.name='PREVIEW_RightTube'
bpy.context.collection.objects.link(other_tube)
other_tube.location=(0,.28,.65)
missile.location=(.16,1.07,0)

scene=bpy.context.scene
scene.unit_settings.system='METRIC'
scene.unit_settings.scale_length=1.0
scene.render.engine='CYCLES'
scene.cycles.samples=48
scene.cycles.use_denoising=True
scene.render.resolution_x=1350
scene.render.resolution_y=1150
scene.render.resolution_percentage=100
scene.render.image_settings.file_format='PNG'
scene.world.color=(.12,.12,.12)
scene.world.use_nodes=True
scene.world.node_tree.nodes['Background'].inputs[0].default_value=(.17,.20,.22,1)
scene.world.node_tree.nodes['Background'].inputs[1].default_value=.4
scene.view_settings.view_transform='AgX'

groundmat=bpy.data.materials.new('Studio_Ground')
groundmat.diffuse_color=(.075,.093,.11,1)
groundmat.use_nodes=True
groundmat.node_tree.nodes['Principled BSDF'].inputs['Base Color'].default_value=(.075,.093,.11,1)
groundmat.node_tree.nodes['Principled BSDF'].inputs['Roughness'].default_value=.86
bpy.ops.mesh.primitive_plane_add(size=200,location=(0,0,-.004))
ground=bpy.context.object
ground.name='STUDIO_Ground'
ground.data.materials.append(groundmat)
for name,loc,power,size,color in [('Key',(2,-3,5),650,4,(1,.88,.72)),
        ('Fill',(-3,-1,3),420,3,(.64,.83,1)),('Rim',(0,4,4),780,3,(.7,.97,1))]:
    bpy.ops.object.light_add(type='AREA',location=loc)
    light=bpy.context.object
    light.name='STUDIO_'+name
    light.data.energy=power
    light.data.shape='DISK'
    light.data.size=size
    light.data.color=color
    light.rotation_euler=(Vector((0,.25,.5))-light.location).to_track_quat('-Z','Y').to_euler()
bpy.ops.object.camera_add(location=(3.4,-4.5,3.3))
camera=bpy.context.object
camera.name='STUDIO_ThreeQuarter'
camera.rotation_euler=(Vector((.0,.37,.49))-camera.location).to_track_quat('-Z','Y').to_euler()
camera.data.type='ORTHO'
camera.data.ortho_scale=2.28
camera.data.lens=45
scene.camera=camera
scene.render.filepath=str(OUT/'turret_threequarter.png')
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'CP_MissileTurret.blend'))
bpy.ops.render.render(write_still=True)
camera.location=(0,.34,6)
camera.rotation_euler=(0,0,-math.pi/2)
camera.data.ortho_scale=2.28
scene.render.filepath=str(OUT/'turret_top.png')
bpy.ops.render.render(write_still=True)

# Combine untouched render pixels in Blender, without a third-party dependency.
front=bpy.data.images.load(str(OUT/'turret_threequarter.png'),check_existing=False)
top=bpy.data.images.load(str(OUT/'turret_top.png'),check_existing=False)
width,height=front.size
preview=bpy.data.images.new('Combat turret art review',width=width*2,height=height)
a=list(front.pixels[:]);b=list(top.pixels[:]);combined=[0.0]*(width*2*height*4)
for row in range(height):
    s=row*width*4;d=row*width*8
    combined[d:d+width*4]=a[s:s+width*4]
    combined[d+width*4:d+width*8]=b[s:s+width*4]
preview.pixels.foreach_set(combined)
preview.filepath_raw=str(OUT/'turret_preview.png')
preview.file_format='PNG'
preview.save()
manifest['preview']='turret_preview.png'
manifest['blend']='CP_MissileTurret.blend'
(OUT/'manifest.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
print('COMBAT_ART_VALIDATED '+json.dumps(manifest))
