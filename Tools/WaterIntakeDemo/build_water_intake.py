"""Build the demo intake as modular Blender meshes. Blender 4.5 LTS, meters.

Run: blender -b --factory-startup --python Tools/WaterIntakeDemo/build_water_intake.py
Only this script's new scene is built; existing project .blend files are never opened.
"""
import bpy
import bmesh
import json
import math
import random
from pathlib import Path
from mathutils import Vector

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / 'TunaSweeper/SourceArt/Environment/WaterIntakeDemo'
TEX = OUT / 'Textures/T_WaterIntakeDemo_Atlas_BaseColor.png'
random.seed(2709)
bpy.ops.wm.read_factory_settings(use_empty=True)
scene = bpy.context.scene
scene.unit_settings.system = 'METRIC'
scene.unit_settings.scale_length = 1.0
scene.render.engine = 'CYCLES'
scene.cycles.samples = 40
scene.cycles.use_denoising = True
scene.render.resolution_x = 1600
scene.render.resolution_y = 1200
scene.render.resolution_percentage = 100
scene.view_settings.view_transform = 'AgX'
scene.world = bpy.data.worlds.new('StudioWorld')
scene.world.use_nodes = True
scene.world.node_tree.nodes['Background'].inputs['Color'].default_value = (0.65, .72, .78, 1)
scene.world.node_tree.nodes['Background'].inputs['Strength'].default_value = .45

atlas = bpy.data.images.load(str(TEX))
atlas.name = 'T_WaterIntakeDemo_Atlas_BaseColor'
atlas.pack()
atlas.filepath = '//Textures/T_WaterIntakeDemo_Atlas_BaseColor.png'
material_specs = {
    'M_IntakeDemo_PaintedMetal': (.20, .68),
    'M_IntakeDemo_Steel': (.65, .48),
    'M_IntakeDemo_Concrete': (0.0, .92),
    'M_IntakeDemo_Organic': (0.0, .88),
    'M_IntakeDemo_Rubber': (0.0, .85),
}
mats = {}
for name, (metal, rough) in material_specs.items():
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    bsdf = mat.node_tree.nodes.get('Principled BSDF')
    bsdf.inputs['Metallic'].default_value = metal
    bsdf.inputs['Roughness'].default_value = rough
    node = mat.node_tree.nodes.new('ShaderNodeTexImage')
    node.image = atlas
    node.interpolation = 'Linear'
    mat.node_tree.links.new(node.outputs['Color'], bsdf.inputs['Base Color'])
    mats[name] = mat
TILES = { 'blue': (0, 0, 'PaintedMetal'), 'steel': (1, 0, 'Steel'),
          'concrete': (2, 0, 'Concrete'), 'ivory': (3, 0, 'PaintedMetal'),
          'orange': (0, 1, 'PaintedMetal'), 'leaf': (1, 1, 'Organic'),
          'wood': (2, 1, 'Organic'), 'rubber': (3, 1, 'Rubber') }
groups = {}
collisions = {}
active_group = 'SM_IntakeDemo_GateFrame'


def finish(obj, name, tile='steel', bevel=.015, smooth=False):
    obj.name = name
    bpy.context.view_layer.objects.active = obj
    obj.select_set(True)
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    if bevel:
        mod = obj.modifiers.new('Manufactured edge bevel', 'BEVEL')
        mod.width = bevel
        mod.segments = 2
        bpy.ops.object.modifier_apply(modifier=mod.name)
    for poly in obj.data.polygons:
        poly.use_smooth = smooth
    if smooth:
        mod = obj.modifiers.new('Weighted corner normals', 'WEIGHTED_NORMAL')
        mod.keep_sharp = True
        mod.weight = 50
        bpy.ops.object.modifier_apply(modifier=mod.name)
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bmesh.ops.dissolve_degenerate(bm, dist=0.000001, edges=list(bm.edges))
    bm.to_mesh(obj.data)
    bm.free()
    # Smart-project each manufactured part, then place every island inside one tile.
    bpy.ops.object.mode_set(mode='EDIT')
    bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.uv.smart_project(angle_limit=math.radians(66), island_margin=.025)
    bpy.ops.object.mode_set(mode='OBJECT')
    col, row, material = TILES[tile]
    for uv in obj.data.uv_layers.active.data:
        uv.uv.x = (col + .09 + .82 * uv.uv.x) / 4
        uv.uv.y = ((1-row) + .09 + .82 * uv.uv.y) / 2
    obj.data.materials.append(mats['M_IntakeDemo_' + material])
    groups.setdefault(active_group, []).append(obj)
    obj.select_set(False)
    return obj


def box(name, loc, size, tile='steel', bevel=.015):
    bpy.ops.mesh.primitive_cube_add(size=1, location=loc)
    obj = bpy.context.object
    obj.dimensions = size
    return finish(obj, name, tile, bevel)


def cylinder(name, loc, radius, depth, tile='steel', axis=(0,0,1), vertices=24, bevel=.008):
    bpy.ops.mesh.primitive_cylinder_add(vertices=vertices, radius=radius, depth=depth, location=loc)
    obj = bpy.context.object
    obj.rotation_euler = Vector(axis).to_track_quat('Z','Y').to_euler()
    return finish(obj, name, tile, bevel, True)


def rod(name, a, b, radius=.025, tile='steel', vertices=12):
    a, b = Vector(a), Vector(b)
    return cylinder(name, (a+b)/2, radius, (b-a).length, tile, b-a, vertices, .004)


def ring(name, loc, major, minor, tile='steel', axis=(0,0,1), segments=32):
    bpy.ops.mesh.primitive_torus_add(major_segments=segments, minor_segments=8,
                                   location=loc, major_radius=major, minor_radius=minor)
    obj = bpy.context.object
    obj.rotation_euler = Vector(axis).to_track_quat('Z','Y').to_euler()
    return finish(obj, name, tile, 0, True)


def pipe(name, points, radius=.13, tile='blue'):
    pts = [Vector(p) for p in points]
    samples = [pts[0]]
    for i in range(1, len(pts)-1):
        p, prev, nxt = pts[i], pts[i-1], pts[i+1]
        r = min(.24, (p-prev).length*.35, (nxt-p).length*.35)
        a = p + (prev-p).normalized()*r
        b = p + (nxt-p).normalized()*r
        samples.append(a)
        for j in range(1, 9):
            t=j/8
            samples.append((1-t)**2*a + 2*(1-t)*t*p + t*t*b)
    samples.append(pts[-1])
    verts, faces = [], []
    n=16
    previous_right = None
    for i,p in enumerate(samples):
        tangent = (samples[min(i+1,len(samples)-1)] - samples[max(i-1,0)]).normalized()
        if previous_right is None:
            guide = Vector((0,0,1)) if abs(tangent.z)<.9 else Vector((1,0,0))
            right=tangent.cross(guide).normalized()
        else:
            right=(previous_right-tangent*previous_right.dot(tangent)).normalized()
        previous_right=right
        up=right.cross(tangent).normalized()
        for k in range(n):
            verts.append(p + radius*(math.cos(k*2*math.pi/n)*right + math.sin(k*2*math.pi/n)*up))
    for i in range(len(samples)-1):
        for k in range(n):
            a=i*n+k; b=i*n+(k+1)%n
            faces.append((a,b,b+n,a+n))
    faces += [tuple(reversed(range(n))), tuple((len(samples)-1)*n+k for k in range(n))]
    mesh=bpy.data.meshes.new(name)
    mesh.from_pydata(verts, [], faces)
    mesh.update()
    obj=bpy.data.objects.new(name,mesh)
    scene.collection.objects.link(obj)
    return finish(obj,name,tile,0,True)


def flange(name, loc, radius=.19, axis=(0,1,0), tile='blue'):
    loc=Vector(loc); axis=Vector(axis).normalized()
    cylinder(name,loc,radius,.075,tile,axis,32)
    guide=Vector((0,0,1)) if abs(axis.z)<.9 else Vector((1,0,0))
    right=axis.cross(guide).normalized(); up=right.cross(axis).normalized()
    for j in range(6):
        angle=j*math.tau/6
        p=loc+radius*.77*(math.cos(angle)*right+math.sin(angle)*up)+axis*.044
        cylinder(name+'_bolt',p,.018,.027,'steel',axis,6,.002)


def collision_box(loc,size):
    collisions.setdefault(active_group,[]).append((tuple(loc),tuple(size)))


# Sluice-gate abutments: sloped upstream faces, vertical inner faces.
for side in (-1,1):
    x=side*1.64
    verts=[(x+dx,y,z) for dx in (-.26,.26) for y,z in ((-1.18,0),(.95,0),(.95,2.3),(-.32,2.3))]
    faces=[(0,3,2,1),(4,5,6,7),(0,1,5,4),(1,2,6,5),(2,3,7,6),(3,0,4,7)]
    mesh=bpy.data.meshes.new('Tapered_abutment')
    mesh.from_pydata(verts,[],faces);mesh.update()
    obj=bpy.data.objects.new('Sloped_concrete_abutment',mesh);scene.collection.objects.link(obj)
    finish(obj,'Sloped_concrete_abutment','concrete',.035)
    collision_box((x,-.1,1.15),(.52,1.7,2.3))
    box('Abutment_footing',(x,-.08,.06),(.7,2.37,.12),'concrete',.025)
    # Formwork joins and plugs, large enough to read without surface noise.
    for z in (.55,1.18,1.8):
        box('Concrete_formwork_joint',(side*1.908,.30,z),(.008,1.20,.009),'steel',.001)
        for y in (.03,.65):
            cylinder('Formwork_tie_plug',(side*1.913,y,z+.15),.025,.008,'concrete',(1,0,0),8,.001)
    box('Gate_guide_outer',(side*1.375,-.035,1.14),(.11,.17,2.14),'steel',.006)
    box('Gate_guide_lip',(side*1.31,-.13,1.14),(.04,.05,2.12),'steel',.004)
    box('Gate_guide_back',(side*1.31,.07,1.14),(.04,.05,2.12),'steel',.004)
box('Maintenance_walkway',(0,.23,2.32),(3.94,1.1,.23),'concrete',.035)
collision_box((0,.23,2.32),(3.94,1.1,.23))
box('Gate_header',(0,.04,2.10),(2.85,.28,.2),'steel',.018)
collision_box((0,.04,2.1),(2.85,.28,.2))
for i in range(8):
    h=(i+1)*2.43/8
    y=-1.35+i*.29
    box('Access_stair_%02d'%i,(2.24,y,h/2),(.73,.32,h),'concrete',.012)
    collision_box((2.24,y,h/2),(.73,.32,h))
# Railings on the rear edge and on front flanks, with center hoist accessible.
for y in (-.30,.75):
    for x in (-1.85,-.9,.9,1.85):
        cylinder('Railing_base',(x,y,2.46),.085,.055,'steel',vertices=12)
        rod('Safety_post',(x,y,2.48),(x,y,3.16),.022)
    for z in (2.80,3.16):
        if y>0:
            rod('Rear_safety_rail',(-1.85,y,z),(1.85,y,z),.021)
        else:
            for a,b in ((-1.85,-.5),(.5,1.85)):
                rod('Front_safety_rail',(a,y,z),(b,y,z),.021)
for z in (2.8,3.16):
    rod('End_safety_rail',(-1.85,-.3,z),(-1.85,.75,z),.021)
# Lift screw, bearings and portal.
for x in (-.26,.26):
    box('Hoist_pedestal',(x,.0,2.48),(.22,.35,.13),'steel')
    box('Hoist_upright',(x,0,2.88),(.10,.16,.72),'steel',.008)
box('Hoist_crossbeam',(0,0,3.22),(.69,.25,.11),'steel',.012)
rod('Gate_lifting_spindle',(0,0,1.98),(0,0,3.61),.032,vertices=16)
for i in range(30):
    ring('Spindle_thread',(0,0,2.53+i*.027),.036,.009,'steel',segments=12)
cylinder('Hoist_bearing',(0,0,3.27),.092,.085,'steel')
ring('Hoist_handwheel',(0,0,3.37),.19,.019,'steel')
for j in range(4):
    a=j*math.pi/2
    rod('Hoist_wheel_spoke',(0,0,3.37),(.185*math.cos(a),.185*math.sin(a),3.37),.014)

active_group='SM_IntakeDemo_Screen'
# Flat vertical bar screen, slides between the concrete gate's metal guides.
for x in (-1.28,1.28):
    box('Screen_side',(x,-.035,1.085),(.105,.15,1.99),'steel',.009)
for z in (.15,2.02):
    box('Screen_crossbar',(0,-.035,z),(2.66,.18,.13),'steel',.008)
    collision_box((0,-.035,z),(2.66,.18,.13))
for j in range(19):
    x=-1.17+j*2.34/18
    box('Vertical_screen_bar_%02d'%j,(x,-.035,1.08),(.047,.11,1.81),'steel',.006)
    collision_box((x,-.035,1.08),(.047,.11,1.81))
for x in (-1.2,1.2):
    ring('Screen_lift_eye',(x,-.035,2.135),.055,.017,'steel',(0,1,0),20)
    for z in (.15,2.02):
        cylinder('Screen_frame_bolt',(x,-.137,z),.025,.03,'steel',(0,1,0),6,.003)

active_group='SM_IntakeDemo_Pump'
# Compact bank-side pump: suction connects behind gate, discharge passes repair valve.
box('Pump_skid',(-3.15,.30,.22),(1.42,1.93,.12),'steel',.035)
for x in (-3.51,-2.79):
    box('Pump_mount', (x,.06,.39),(.17,1.28,.27),'blue')
cylinder('Pump_volute',(-3.15,-.18,.83),.45,.30,'blue',(0,1,0),40,.035)
cylinder('Pump_front_cover',(-3.15,-.355,.83),.36,.075,'blue',(0,1,0),32,.018)
flange('Pump_intake_flange',(-3.15,-.405,.83),.24,(0,-1,0))
cylinder('Motor_body',(-3.15,.51,.83),.285,.94,'blue',(0,1,0),32,.025)
cylinder('Motor_rear_shroud',(-3.15,1.02,.83),.31,.17,'blue',(0,1,0),32,.018)
for j in range(16):
    a=j*math.tau/16
    obj=box('Motor_cooling_fin',(-3.15+.30*math.cos(a),.51,.83+.30*math.sin(a)),(.038,.70,.085),'blue',.006)
    obj.rotation_euler.y=math.pi/2-a
for x in (-3.40,-3.25,-3.10,-2.95):
    box('Motor_vent', (x,1.111,.84),(.035,.012,.28),'rubber',.01)
box('Motor_terminal_box',(-2.8,.55,.91),(.23,.33,.27),'blue')
for j in range(8):
    a=j*math.tau/8
    cylinder('Volute_cover_bolt',(-3.15+.393*math.cos(a),-.377,.83+.393*math.sin(a)),.025,.04,'steel',(0,1,0),6,.003)
pipe('Intake_pipe',[(-3.15,-.45,.83),(-3.15,-.92,.83),(-3.15,-.92,.36),(-2.33,-.92,.36),(-2.33,1.12,.36),(-.95,1.12,.36)],.135)
flange('Intake_service_flange',(-2.33,.73,.36),.20)
cylinder('Submerged_intake_mouth',(-.944,1.12,.36),.115,.012,'rubber',(1,0,0))
pipe('Discharge_riser',[(-3.15,-.05,1.19),(-3.15,-.05,1.39),(-3.15,1.38,1.39)],.115)
flange('Discharge_front',(-3.15,1.34,1.39),.17)
# Valve body and permanent spindle, wheels are separate swap meshes.
cylinder('Repair_valve_body',(-3.15,1.56,1.39),.18,.35,'blue',(0,1,0),24,.023)
cylinder('Repair_valve_bonnet',(-3.15,1.56,1.55),.135,.16,'blue',vertices=24)
flange('Valve_bonnet',(-3.15,1.56,1.66),.145,(0,0,1))
cylinder('Permanent_valve_stem',(-3.15,1.56,1.78),.035,.2,'steel',vertices=12)
pipe('Bunker_supply_outlet',[(-3.15,1.75,1.39),(-3.15,2.08,1.39),(-3.15,2.08,.16)],.115)
flange('Discharge_rear',(-3.15,1.78,1.39),.17)
for y in (-.75,1.08):
    box('Pipe_support',(-2.33,y,.20),(.29,.13,.25),'steel')
# Accessible control panel faces upstream / front.
box('Control_cabinet',(-4.20,.54,.99),(.56,.38,1.30),'ivory',.035)
box('Cabinet_plinth',(-4.20,.54,.28),(.61,.43,.15),'steel')
box('Cabinet_door',(-4.20,.335,1.0),(.48,.035,1.14),'ivory',.014)
box('Panel_recess',(-4.20,.309,1.27),(.31,.013,.27),'steel',.012)
for x,tile in ((-4.29,'orange'),(-4.11,'leaf')):
    cylinder('Panel_status_lens',(x,.29,1.29),.045,.029,tile,(0,1,0),16,.007)
box('Cabinet_handle',(-4.02,.285,.86),(.026,.045,.22),'steel',.009)
for z in (.57,1.36):
    cylinder('Cabinet_hinge',(-4.44,.32,z),.018,.11,'steel')
pipe('Electrical_conduit',[(-4.2,.66,.4),(-4.2,.85,.3),(-3.6,.85,.3),(-3.6,.6,.56),(-3.15,.6,.56)],.018,'rubber')
collision_box((-3.15,.35,.73),(1.12,1.84,1.04))
collision_box((-4.20,.54,.96),(.58,.42,1.39))
collision_box((-3.15,1.55,1.4),(.42,.46,.44))

active_group='SM_IntakeDemo_PumpFoundation'
box('Pump_foundation',(-3.57,.42,.09),(2.31,2.12,.18),'concrete',.04)
collision_box((-3.57,.42,.09),(2.31,2.12,.18))
box('Outlet_anchor',(-3.15,2.08,.11),(.53,.51,.22),'concrete',.025)
collision_box((-3.15,2.08,.11),(.53,.51,.22))

wheel_origin=Vector((-3.15,1.56,1.88))
active_group='SM_IntakeDemo_ValveHandle_Repaired'
ring('Repair_wheel',wheel_origin,.235,.027,'orange',segments=32)
cylinder('Repair_wheel_hub',wheel_origin,.063,.065,'orange',vertices=16)
for j in range(4):
    a=j*math.pi/2
    rod('Repair_wheel_spoke',wheel_origin, wheel_origin+Vector((.224*math.cos(a),.224*math.sin(a),0)),.019,'orange')
cylinder('Repair_wheel_nut',wheel_origin+Vector((0,0,.047)),.035,.04,'steel',vertices=6,bevel=.003)

active_group='SM_IntakeDemo_ValveHandle_Broken'
cylinder('Broken_wheel_hub',wheel_origin,.061,.065,'orange',vertices=16)
rod('Broken_spoke_stub',wheel_origin,wheel_origin+Vector((.115,0,.025)),.021,'orange')
rod('Broken_spoke_short',wheel_origin,wheel_origin+Vector((-.06,.014,0)),.021,'orange')
cylinder('Broken_wheel_nut',wheel_origin+Vector((0,0,.047)),.035,.04,'steel',vertices=6,bevel=.003)

active_group='SM_IntakeDemo_ScreenDebris'
# Chunky individual twigs and closed leaf meshes; no alpha or procedural texture.
for i in range(17):
    x=random.uniform(-1.17,1.17); z=random.uniform(.22,.70)
    a=(x,-.25-random.random()*.12,z)
    b=(max(-1.23,min(1.23,x+random.uniform(-.7,.7))),-.25-random.random()*.15,z+random.uniform(.13,.40))
    rod('Caught_branch',a,b,random.uniform(.016,.032),'wood',8)
    av,bv=Vector(a),Vector(b)
    branch=av.lerp(bv,.55)
    rod('Branch_fork',branch,branch+Vector((random.uniform(-.23,.23),-.04,.18)),.012,'wood',7)
for i in range(68):
    center=Vector((random.uniform(-1.18,1.18),random.uniform(-.43,-.24),random.uniform(.16,.78)))
    length=random.uniform(.13,.30); width=random.uniform(.04,.10)
    angle=random.uniform(-1.6,1.6)
    up=Vector((math.sin(angle),0,math.cos(angle)))
    right=Vector((math.cos(angle),0,-math.sin(angle)))
    verts=[center-up*length/2,center-right*width,center+up*length/2,center+right*width,center+Vector((0,-.028,0)),center+Vector((0,.008,0))]
    faces=[(0,1,4),(1,2,4),(2,3,4),(3,0,4),(1,0,5),(2,1,5),(3,2,5),(0,3,5)]
    mesh=bpy.data.meshes.new('Closed_leaf');mesh.from_pydata(verts,[],faces);mesh.update()
    obj=bpy.data.objects.new('Caught_leaf',mesh);scene.collection.objects.link(obj)
    finish(obj,'Caught_leaf','leaf',0)

# Merge by gameplay part, keeping material slots and UVs. Handle pivots are local.
assets={}
manifest={'units':'meters in Blender; centimeters in UE', 'atlas':str(TEX.relative_to(ROOT)),
          'materials':{k:{'metallic':v[0],'roughness':v[1]} for k,v in material_specs.items()},
          'assets':[], 'notes':'The supplied sluice reference supersedes the old cylindrical screen concept. No Blueprint is modified.'}
for name,parts in groups.items():
    bpy.ops.object.select_all(action='DESELECT')
    for obj in parts: obj.select_set(True)
    bpy.context.view_layer.objects.active=parts[0]
    bpy.ops.object.join()
    obj=bpy.context.object;obj.name=name;obj.data.name=name+'_Mesh'
    # Apply current normals/mesh orientation and eliminate duplicate material slots.
    bpy.ops.object.transform_apply(location=False,rotation=True,scale=True)
    pivot=wheel_origin if 'ValveHandle' in name else Vector((0,0,0))
    scene.cursor.location=pivot
    bpy.ops.object.origin_set(type='ORIGIN_CURSOR')
    # Consistent outward polygon winding for generated custom mesh pieces.
    bpy.ops.object.mode_set(mode='EDIT')
    bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.mesh.normals_make_consistent(inside=False)
    bpy.ops.object.mode_set(mode='OBJECT')
    obj.data.calc_loop_triangles()
    positions=[obj.matrix_world@Vector(v) for v in obj.bound_box]
    bounds=[min(v[i] for v in positions) for i in range(3)]+[max(v[i] for v in positions) for i in range(3)]
    uv=obj.data.uv_layers.active
    assert uv and all(math.isfinite(c) and 0<=c<=1 for item in uv.data for c in item.uv)
    assert all(math.isfinite(c) for v in obj.data.vertices for c in v.co)
    assert not any(t.area<1e-10 for t in obj.data.loop_triangles), name+' has degenerate triangles'
    assets[name]=obj
    entry={'name':name,'vertices':len(obj.data.vertices),'triangles':len(obj.data.loop_triangles),
           'bounds_world_m':bounds,'placement_blender_m':list(pivot),
           'materials':[m.name for m in obj.data.materials], 'collision_boxes':len(collisions.get(name,[]))}
    manifest['assets'].append(entry)
    # Every FBX contains exactly one render mesh and explicit convex collision boxes.
    obj.location=(0,0,0)
    export_objs=[obj]
    for idx,(loc,size) in enumerate(collisions.get(name,[])):
        bpy.ops.mesh.primitive_cube_add(size=1,location=Vector(loc)-pivot)
        col=bpy.context.object;col.name='UCX_'+name+'_%02d'%idx;col.dimensions=size
        bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
        export_objs.append(col)
    bpy.ops.object.select_all(action='DESELECT')
    for item in export_objs:item.select_set(True)
    bpy.context.view_layer.objects.active=obj
    bpy.ops.export_scene.fbx(filepath=str(OUT/'Models'/f'{name}.fbx'),use_selection=True,
        object_types={'MESH'},apply_unit_scale=True,apply_scale_options='FBX_SCALE_NONE',
        axis_forward='-Y',axis_up='Z',use_mesh_modifiers=True,mesh_smooth_type='FACE',
        add_leaf_bones=False,bake_anim=False,path_mode='STRIP',use_custom_props=False)
    for col in export_objs[1:]:bpy.data.objects.remove(col,do_unlink=True)
    obj.location=pivot
    obj.select_set(False)
manifest['total_triangles']=sum(a['triangles'] for a in manifest['assets'])
assert manifest['total_triangles']<120000
(OUT/'model_manifest.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')

# Neutral studio preview objects are clearly separated from exported assets.
preview_collection=bpy.data.collections.new('PREVIEW_ONLY_NOT_EXPORTED')
scene.collection.children.link(preview_collection)
def preview_link(obj):
    for col in list(obj.users_collection):col.objects.unlink(obj)
    preview_collection.objects.link(obj)
bpy.ops.mesh.primitive_plane_add(size=200,location=(0,0,-.065))
ground=bpy.context.object;ground.name='Preview_ground';preview_link(ground)
mat=bpy.data.materials.new('Preview_ground_material');mat.diffuse_color=(.23,.27,.29,1)
ground.data.materials.append(mat)
for name,loc,power,size in [('Key',(-5,-6,9),2200,7),('Fill',(4,-2,6),1500,6),('Rim',(-1,6,8),2300,5)]:
    data=bpy.data.lights.new(name,'AREA');data.energy=power;data.shape='DISK';data.size=size
    obj=bpy.data.objects.new(name,data);preview_collection.objects.link(obj);obj.location=loc
    obj.rotation_euler=(Vector((-1,0,1))-obj.location).to_track_quat('-Z','Y').to_euler()
data=bpy.data.cameras.new('ReviewCamera');cam=bpy.data.objects.new('ReviewCamera',data)
preview_collection.objects.link(cam);scene.camera=cam;data.type='ORTHO';data.ortho_scale=9.4
def camera(loc,target,scale):
    cam.location=loc;cam.rotation_euler=(Vector(target)-cam.location).to_track_quat('-Z','Y').to_euler()
    data.ortho_scale=scale
camera((8,-12,9),(-1.05,.20,1.55),9.0)
assets['SM_IntakeDemo_ValveHandle_Broken'].hide_render=True
assets['SM_IntakeDemo_ValveHandle_Broken'].hide_set(True)
scene.render.image_settings.file_format='PNG'
scene.render.filepath=str(OUT/'Previews/IntakeDemo_Assembly.png')
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'WaterIntakeDemo.blend'))
bpy.ops.render.render(write_still=True)
camera((-.9,-13,4.5),(-.9,.15,1.65),8.4)
scene.render.filepath=str(OUT/'Previews/IntakeDemo_Front.png')
bpy.ops.render.render(write_still=True)
camera((5,9,8),(-1.0,.25,1.5),9)
scene.render.filepath=str(OUT/'Previews/IntakeDemo_Back.png')
bpy.ops.render.render(write_still=True)
assets['SM_IntakeDemo_ScreenDebris'].hide_render=True
assets['SM_IntakeDemo_ValveHandle_Repaired'].hide_render=True
assets['SM_IntakeDemo_ValveHandle_Broken'].hide_render=False
camera((7,-11,8),(-1.05,.2,1.55),9)
scene.render.filepath=str(OUT/'Previews/IntakeDemo_Clean_BrokenValve.png')
bpy.ops.render.render(write_still=True)
print('INTAKE_BUILD_COMPLETE '+json.dumps({'assets':len(assets),'triangles':manifest['total_triangles']}))
