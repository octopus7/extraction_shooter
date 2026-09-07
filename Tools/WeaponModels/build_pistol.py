"""Author the two reference-driven Tuna weapon pistols in Blender 4.5.

Run with blender --background --factory-startup --python-exit-code 1 --python
Tools/WeaponModels/build_pistol.py.  Every coordinate below is UE centimetres.
The source remains deliberately editable: named components are collected in
hidden authoring collections while the exported game meshes are consolidated.
"""
from pathlib import Path
import math
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))
import bpy
import bmesh
from mathutils import Vector
import common as c


def raw_profile(name, points, width, y=0.0):
    """Closed x/z silhouette extruded along UE Y; points may be concave."""
    count = len(points)
    verts = [c.ue((x, yy, z)) for yy in (y-width/2, y+width/2) for x, z in points]
    faces = [tuple(reversed(range(count))), tuple(range(count, 2*count))]
    faces += [(i, (i+1) % count, (i+1) % count + count, i+count) for i in range(count)]
    mesh = bpy.data.meshes.new(name)
    mesh.from_pydata(verts, [], faces)
    mesh.update()
    bm=bmesh.new()
    bm.from_mesh(mesh)
    bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces))
    bm.to_mesh(mesh)
    bm.free()
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.collection.objects.link(obj)
    return obj


def quiet_uv(obj, material, inset=(0.15, 0.25, 0.42, 0.48)):
    """Sample a quiet genuine part of the atlas on broad painted surfaces."""
    col, row = c.TILES[material][:2]
    x0, y0, x1, y1 = inset
    for loop in obj.data.uv_layers.active.data:
        u = (loop.uv.x * 4-col-0.06)/0.88
        v = (loop.uv.y * 4-(3-row)-0.06)/0.88
        loop.uv = ((col+x0+(x1-x0)*u)/4, (3-row+y0+(y1-y0)*v)/4)
    return obj


def profile(name, points, width, material, y=0.0, bevel=0.10, quiet=False):
    obj = c.finish(raw_profile(name, points, width, y), name, material, bevel, True)
    if quiet:
        quiet_uv(obj, material)
    return obj


def subtract(obj, cutter):
    bpy.context.view_layer.objects.active = obj
    mod = obj.modifiers.new("Machined pocket", "BOOLEAN")
    mod.operation = "DIFFERENCE"
    mod.solver = "EXACT"
    mod.object = cutter
    bpy.ops.object.modifier_apply(modifier=mod.name)
    bpy.data.objects.remove(cutter, do_unlink=True)


def hole_x(obj, x, y, z, radius, length):
    bpy.ops.mesh.primitive_cylinder_add(vertices=32, radius=radius/100, depth=length/100, location=c.ue((x, y, z)))
    cutter = bpy.context.object
    cutter.rotation_euler = c.ue((1, 0, 0)).to_track_quat("Z", "Y").to_euler()
    subtract(obj, cutter)


def tube(name, start, end, z, outer, inner, material="Gunmetal", sides=32):
    verts = []
    for x, radius in ((start, outer), (end, outer), (start, inner), (end, inner)):
        for index in range(sides):
            a = index*math.tau/sides
            verts.append(c.ue((x, math.cos(a)*radius, z+math.sin(a)*radius)))
    faces=[]
    for i in range(sides):
        j=(i+1) % sides
        faces.extend([(i,j,sides+j,sides+i), (2*sides+j,2*sides+i,3*sides+i,3*sides+j),
                      (sides+i,sides+j,3*sides+j,3*sides+i), (j,i,2*sides+i,2*sides+j)])
    mesh=bpy.data.meshes.new(name)
    mesh.from_pydata(verts,[],faces)
    mesh.update()
    obj=bpy.data.objects.new(name,mesh)
    bpy.context.collection.objects.link(obj)
    return c.finish(obj,name,material,0.025,True)


def guard(name, outer, inner, width, material):
    n=len(outer)
    assert len(inner)==n
    verts=[c.ue((x,y,z)) for y in (-width/2,width/2) for loop in (outer,inner) for x,z in loop]
    faces=[]
    for i in range(n):
        j=(i+1)%n
        faces += [(i,j,n+j,n+i), (2*n+i,3*n+i,3*n+j,2*n+j),
                  (i,2*n+i,2*n+j,j), (n+j,3*n+j,3*n+i,n+i)]
    mesh=bpy.data.meshes.new(name)
    mesh.from_pydata(verts,[],faces)
    mesh.update()
    obj=bpy.data.objects.new(name,mesh)
    bpy.context.collection.objects.link(obj)
    obj=c.finish(obj,name,material,0.14,True)
    quiet_uv(obj,material)
    return obj


def screw(name, x, y, z, radius=0.19):
    c.cylinder(name, (x,y,z), radius, 0.075, "Steel", axis=(0,1,0), vertices=12, bevel_cm=0.025)
    c.box(name+"Slot",(x,y+(0.046 if y>0 else -0.046),z),(radius*1.20,0.014,0.05),"Charcoal",0.01)


def archive_components():
    collection=bpy.data.collections.new("EDITABLE_"+c.current["name"])
    bpy.context.scene.collection.children.link(collection)
    collection.hide_render=True
    collection.hide_viewport=True
    for part in c.parts:
        # Resolve bevel/Boolean n-gons explicitly so the editable source and FBX
        # have identical triangulation, including at the narrow muzzle shoulder.
        bm=bmesh.new()
        bm.from_mesh(part.data)
        bmesh.ops.remove_doubles(bm,verts=list(bm.verts),dist=1e-7)
        bmesh.ops.triangulate(bm,faces=list(bm.faces))
        slivers=[face for face in bm.faces if face.calc_area()<=1e-11]
        if slivers:
            print("PISTOL_REMOVE_ZERO_AREA",part.name,len(slivers))
            bmesh.ops.delete(bm,geom=slivers,context="FACES")
        bm.to_mesh(part.data)
        bm.free()
        part.data.update()
        part.data.calc_loop_triangles()
        invalid=[triangle for triangle in part.data.loop_triangles if triangle.area<=1e-13]
        if invalid:
            # Blender's final float mesh can collapse a Boolean seam even when
            # the intermediate BMesh reports a positive area. Remove only those
            # explicitly measured zero-area polygons from the triangulated mesh.
            print("PISTOL_REMOVE_FLOAT_COLLAPSE",part.name,len(invalid))
            bad_indices={triangle.polygon_index for triangle in invalid}
            bm=bmesh.new()
            bm.from_mesh(part.data)
            bm.faces.ensure_lookup_table()
            bmesh.ops.delete(bm,geom=[bm.faces[index] for index in bad_indices],context="FACES")
            bm.to_mesh(part.data)
            bm.free()
            part.data.update()
        duplicate=part.copy()
        duplicate.data=part.data.copy()
        collection.objects.link(duplicate)
    return collection


def slide(premium=False):
    if premium:
        contour=[(-10.70,2.35),(14.85,2.35),(16.40,2.10),(16.72,2.70),(16.50,4.82),
                 (13.70,5.03),(0.85,5.03),(-0.40,5.40),(-8.95,5.40),(-10.70,4.94)]
        width=3.62
        muzzle_z=3.76
    else:
        contour=[(-10.76,2.05),(16.45,2.05),(16.58,2.32),(16.58,5.02),(16.23,5.42),
                 (-10.40,5.42),(-10.76,5.00)]
        width=3.74
        muzzle_z=3.76
    obj=raw_profile("MachinedSlide",contour,width)
    hole_x(obj,15.4,0,muzzle_z,0.92,7.0)
    # This port is an actual top/right opening with a visible chamber hood.
    port=raw_profile("EjectionPortCutter",[(0.2,4.15),(4.75,4.15),(4.75,6.3),(0.2,6.3)],3.2,0.95)
    subtract(obj,port)
    for side in (-1,1):
        for index in range(7):
            x=-8.0+index*0.61
            points=[(x,2.38),(x+0.25,2.38),(x-0.42,4.97),(x-0.67,4.97)]
            cut=raw_profile("SerrationCutter",points,0.65,side*(width/2))
            subtract(obj,cut)
            profile("SerrationInset_%s_%02d"%(side,index),points,0.018,"Charcoal",side*(width/2-0.30),0.015)
    c.finish(obj,"MachinedSlide","Charcoal",0.11,True)
    # Separate lower slide shoulder, rear striker cover and functional chamber.
    for side in (-1,1):
        if premium:
            profile("SlideShoulder"+str(side),[(-2.0,2.48),(11.5,2.48),(12.15,3.0),(14.55,3.0),
                    (15.15,3.68),(15.08,4.40),(-1.1,4.40),(-2.0,4.68)],0.08,"Charcoal",side*1.84,0.045)
        else:
            profile("SlideShoulder"+str(side),[(-2.0,2.20),(16.21,2.20),(16.21,4.20),(9.1,4.20),
                    (8.65,4.68),(-1.4,4.68),(-2.0,4.42)],0.065,"Charcoal",side*1.885,0.035)
    hood=c.box("ExposedChamberHood",(2.48,0.16,4.72 if premium else 4.98),(4.20,2.26,0.35),"Steel",0.09)
    quiet_uv(hood,"Steel",(0.12,0.10,0.88,0.90))
    c.box("PortRearShadow",(0.38,0.82,4.31),(0.18,2.4,0.25),"Rubber",0.02)
    c.box("Extractor",(3.62,1.57,4.08),(1.95,0.30,0.38),"Gunmetal",0.055)
    c.box("StrikerBackplate",(-10.79,0,3.58),(0.12,2.65,2.38),"Gunmetal",0.16)
    c.box("BackplateLowerTongue",(-10.875,0,2.63),(0.065,0.72,0.25),"Charcoal",0.035)
    tube("VisibleBarrel",11.70,17.00,muzzle_z,0.80,0.555,"Gunmetal")
    tube("MuzzleCrown",16.80,17.00,muzzle_z,0.83,0.565,"Steel")
    c.cylinder("BoreDepth",(11.70,0,muzzle_z),0.554,0.035,"Rubber",vertices=32,bevel_cm=0)
    c.cylinder("RecoilGuidePlug",(16.61,0,2.36),0.30,0.16,"Gunmetal",vertices=20,bevel_cm=0.035)
    if premium:
        # Octagonal front sleeve is genuinely bored, not a black disc overlay.
        cap=raw_profile("FrontSleeve",[(15.98,2.06),(16.83,2.06),(16.89,2.45),
                        (16.89,4.58),(16.70,4.98),(15.98,5.04)],3.82)
        hole_x(cap,16.5,0,muzzle_z,0.91,3)
        c.finish(cap,"FacetedMuzzleSleeve","Gunmetal",0.24,True)
        for side in (-1,1):
            c.box("StatusInset"+str(side),(12.55,side*1.927,2.93),(1.75,0.07,0.25),"Rubber",0.08)
            c.box("StatusSlit"+str(side),(12.55,side*1.97,2.93),(1.27,0.034,0.10),"CyanAccent",0.04)


def sights(premium):
    # Both variants have a true open rear notch and a separate front post.
    rear=-9.34 if not premium else -9.08
    c.box("RearSightDovetail",(rear,0,5.43),(1.38,2.66,0.26),"Gunmetal",0.06)
    for side in (-1,1):
        profile("RearNotchEar"+str(side),[(rear-0.54,5.46),(rear+0.46,5.46),
                (rear+0.29,6.00),(rear-0.39,6.00)],0.66,"Gunmetal",side*0.73,0.045)
        c.cylinder("RearSightDot"+str(side),(rear-0.56,side*0.73,5.76),0.075,0.03,
                   "IvoryCeramic",axis=(1,0,0),vertices=12,bevel_cm=0.01)
    front=14.84 if not premium else 14.39
    c.box("FrontSightBase",(front,0,5.39),(1.42,1.44,0.25),"Gunmetal",0.06)
    profile("FrontSightPost",[(front-0.47,5.43),(front+0.37,5.43),(front+0.22,5.99),
            (front-0.29,5.99)],0.55,"Gunmetal",0,0.055)
    c.cylinder("FrontSightDot",(front-0.45,0,5.75),0.09,0.03,
               "IvoryCeramic",axis=(1,0,0),vertices=12,bevel_cm=0.01)
    if premium:
        # Flat optic-ready mounting pocket; retains the small iron-sight profile.
        c.box("OpticMountRecess",(-6.06,0,5.394),(3.60,2.12,0.035),"Rubber",0.11)
        c.box("OpticCoverPlate",(-6.06,0,5.414),(3.30,1.94,0.043),"Gunmetal",0.09)
        for x in (-7.15,-4.98):
            c.cylinder("OpticCoverScrew"+str(x),(x,0,5.457),0.135,0.035,"Steel",axis=(0,0,1),vertices=12,bevel_cm=0.025)
            c.box("OpticScrewSlot"+str(x),(x,0,5.482),(0.15,0.035,0.01),"Charcoal",0.005)


def standard():
    c.begin("SM_TunaWeapon_Pistol_Standard","Pistol","Standard","References/Pistol_Standard_Reference.png",
            "Practical polymer sidearm: straight machined slide, curved solid backstrap, rounded grip inserts, hollow barrel, open trigger guard and iron sights.")
    slide(False)
    frame=[(-11.0,1.91),(16.39,1.91),(16.39,0.30),(8.32,0.30),(8.32,-0.12),
           (-0.95,-0.12),(-2.08,-1.05),(-2.97,-2.95),(-5.20,-10.96),(-5.65,-11.39),
           (-10.41,-11.39),(-10.64,-10.96),(-8.56,-3.62),(-8.03,-2.10),(-8.16,-1.12),
           (-8.67,-0.30),(-10.80,0.43),(-11.0,1.03)]
    profile("PolymerFrame",frame,3.88,"WarmGrayPolymer",bevel=0.19,quiet=True)
    outer=[(-3.13,0.48),(4.35,0.48),(4.76,-0.36),(4.59,-2.76),(4.14,-3.97),(3.20,-4.38),(-1.78,-4.38),(-3.34,-3.41)]
    inner=[(-2.51,-0.29),(3.66,-0.29),(3.98,-0.64),(3.87,-2.63),(3.51,-3.25),(2.94,-3.51),(-1.53,-3.51),(-2.59,-2.89)]
    guard("RoundedTriggerGuard",outer,inner,2.74,"WarmGrayPolymer")
    trigger=[(-0.57,-0.24),(0.13,-0.24),(-0.16,-1.15),(-0.14,-1.80),(0.29,-2.56),
             (1.18,-3.08),(1.20,-3.35),(0.62,-3.29),(-0.21,-2.84),(-0.81,-2.12),(-1.06,-1.30)]
    profile("CurvedTrigger",trigger,0.67,"Gunmetal",bevel=0.085)
    profile("TriggerSafetyBlade",[(-0.12,-0.67),(0.02,-0.72),(-0.12,-1.73),(0.22,-2.30),
            (0.59,-2.65),(0.39,-2.77),(-0.09,-2.39),(-0.37,-1.79)],0.095,"Charcoal",0,0.025)
    panel=[(-7.85,-0.69),(-3.49,-1.22),(-3.08,-1.80),(-5.48,-10.43),(-5.90,-10.68),
           (-9.67,-10.55),(-9.94,-10.20),(-7.69,-2.74),(-7.44,-1.65)]
    for side in (-1,1):
        profile("GripPanelBorder"+str(side),panel,0.20,"Charcoal",side*1.96,0.18)
        inset=[(-7.57,-1.12),(-3.81,-1.58),(-3.54,-1.99),(-5.81,-10.12),(-6.0,-10.29),
               (-9.28,-10.18),(-9.47,-9.99),(-7.20,-2.63),(-7.13,-1.69)]
        profile("PebbledGripPanel"+str(side),inset,0.08,"BlackGrip",side*2.085,0.11)
        c.box("SlideReleaseSeat"+str(side),(-0.91,side*1.987,1.34),(3.50,0.09,0.97),"Charcoal",0.21)
        c.box("SlideRelease"+str(side),(-0.91,side*2.063,1.39),(3.11,0.14,0.69),"Steel",0.15)
        c.box("MagazineReleaseHousing"+str(side),(-3.02,side*1.91,-2.89),(1.02,0.34,1.27),"WarmGrayPolymer",0.20,rotation=(0,17,0))
        c.box("MagazineRelease"+str(side),(-3.04,side*2.095,-2.88),(0.70,0.075,0.85),"Gunmetal",0.12,rotation=(0,17,0))
        for index in range(3):
            c.box("ReleaseGroove_%s_%s"%(side,index),(-3.04,side*2.143,-3.11+index*0.23),(0.49,0.025,0.035),"Charcoal",0.009)
        screw("FramePin"+str(side),3.29,side*2.018,1.03,0.21)
        screw("SmallFramePin"+str(side),1.33,side*1.998,0.41,0.14)
    for index in range(6):
        x=-3.67-index*0.275
        z=-5.18-index*0.96
        profile("FrontstrapTraction%02d"%index,[(x-0.12,z+0.22),(x+0.29,z+0.16),
                (x+0.16,z-0.27),(x-0.24,z-0.23)],2.45,"BlackGrip",bevel=0.07)
    heel=[(-10.72,-11.35),(-5.07,-11.35),(-4.99,-11.75),(-5.21,-12.00),(-10.79,-12.00),(-10.91,-11.78)]
    profile("MagazineHeel",heel,4.50,"Magazine",bevel=0.085)
    c.box("MagazineFloorLatch",(-7.85,0,-11.962),(1.15,1.3,0.07),"Charcoal",0.07)
    c.box("AccessoryRailSpine",(10.35,0,-0.16),(5.86,2.54,0.36),"WarmGrayPolymer",0.08)
    for index in range(4):
        c.box("AccessoryRailLug%02d"%index,(8.20+index*1.33,0,-0.46),(0.65,2.78,0.37),"Charcoal",0.06)
    sights(False)
    c.collision_box((2.8,0,2.6),(28.0,4.2,6.5))
    c.collision_box((-6.9,0,-6.15),(8.0,4.5,11.7))
    c.collision_box((0.8,0,-2.0),(7.8,2.9,5.0))
    c.socket("MuzzleSocket",(17.0,0,3.76))
    c.socket("LaserSightSocket",(12.15,0,-0.72))
    c.socket("ShellEjectionSocket",(2.65,2.03,4.36),(0,0,0))
    archive_components()
    return c.end((28.0,4.5,18.0),0.12)


def premium():
    c.begin("SM_TunaWeapon_Pistol_Premium","Pistol","Premium","References/Pistol_Premium_Reference.png",
            "70% modern / 30% restrained near-future sidearm: low stepped wedge slide, faceted monocoque frame/grip, octagonal muzzle sleeve, small ivory inserts, optic-ready notch cover and narrow cyan status slit.")
    slide(True)
    frame=[(-10.82,2.04),(15.92,2.04),(16.21,1.04),(15.69,-0.07),(7.24,-0.07),
           (6.35,-0.53),(4.79,-0.53),(4.05,-1.07),(-1.39,-1.07),(-2.69,-2.80),
           (-4.80,-10.55),(-4.21,-11.39),(-5.06,-11.55),(-10.42,-11.18),
           (-10.52,-10.68),(-8.35,-3.55),(-7.88,-2.07),(-8.24,-1.19),(-9.29,-0.39),(-11.0,0.45)]
    profile("FacetedMonocoqueFrame",frame,3.86,"GraphiteCeramic",bevel=0.15,quiet=True)
    outer=[(-3.02,-0.01),(5.05,-0.01),(5.21,-0.65),(4.22,-3.47),(3.45,-4.40),
           (2.83,-4.59),(-1.86,-4.40),(-3.35,-3.48)]
    inner=[(-2.36,-0.78),(4.03,-0.78),(4.17,-1.04),(3.50,-3.06),(3.06,-3.65),
           (2.63,-3.85),(-1.45,-3.73),(-2.56,-3.02)]
    guard("AngularTriggerGuard",outer,inner,2.75,"GraphiteCeramic")
    trigger=[(-0.56,-0.69),(0.20,-0.69),(-0.30,-1.86),(0.29,-3.22),(0.77,-3.61),
             (0.28,-3.70),(-0.24,-3.40),(-1.02,-1.86)]
    profile("FacetedTrigger",trigger,0.70,"Gunmetal",bevel=0.055)
    profile("TriggerSafetyBlade",[(-0.07,-1.04),(0.09,-1.08),(-0.24,-1.95),
            (0.29,-3.17),(0.13,-3.29),(-0.53,-1.92)],0.12,"Steel",bevel=0.02)
    for side in (-1,1):
        y=side*1.98
        profile("SculptedFramePlate"+str(side),[(-10.56,1.77),(-4.23,1.77),(-3.39,0.71),
                (0.64,0.71),(1.44,1.63),(11.37,1.63),(11.37,0.45),(6.57,0.45),(5.54,-0.57),
                (4.70,-0.64),(5.12,0.49),(-1.46,0.45),(-3.43,-1.46),(-6.52,-0.12),(-9.91,0.37)],
                0.15,"GraphiteCeramic",y,0.095,quiet=True)
        profile("RearIvoryInsert"+str(side),[(-9.65,1.89),(-4.83,1.89),(-5.64,0.88),
                (-9.31,1.03),(-9.77,1.25)],0.13,"IvoryCeramic",side*2.08,0.09,quiet=True)
        profile("FrontIvoryInsert"+str(side),[(11.77,1.82),(14.96,1.82),(15.55,1.15),
                (15.26,0.57),(11.73,0.72)],0.12,"IvoryCeramic",side*2.08,0.08,quiet=True)
        panel=[(-7.49,-0.68),(-3.42,-1.86),(-3.10,-2.50),(-5.10,-10.63),(-5.77,-11.02),
               (-9.93,-10.75),(-9.66,-9.65),(-7.51,-2.84),(-7.17,-1.63)]
        profile("GripArmourBezel"+str(side),panel,0.18,"Gunmetal",side*1.96,0.11,quiet=True)
        # Two separate pebbled fields, with an angular structural ridge between.
        profile("GripUpperTexture"+str(side),[(-7.27,-1.13),(-3.81,-2.14),(-3.56,-2.64),
                (-4.04,-4.73),(-5.98,-5.53),(-7.49,-4.64),(-6.99,-2.76),(-6.78,-1.84)],
                0.065,"BlackGrip",side*2.08,0.045)
        profile("GripLowerTexture"+str(side),[(-7.66,-5.04),(-6.02,-5.98),(-4.22,-5.25),
                (-5.52,-10.35),(-5.92,-10.57),(-9.36,-10.34)],0.065,"BlackGrip",side*2.08,0.045)
        profile("GripFacetRidge"+str(side),[(-7.53,-4.57),(-5.98,-5.53),(-4.01,-4.68),
                (-4.09,-5.05),(-6.03,-5.94),(-7.65,-4.95)],0.09,"Charcoal",side*2.106,0.035)
        profile("SlideReleaseSeat"+str(side),[(-4.57,0.64),(-2.69,0.64),(-2.44,0.29),
                (-2.72,-0.28),(-4.42,-0.28),(-4.70,0.05)],0.13,"Rubber",side*2.04,0.11)
        profile("SlideRelease"+str(side),[(-4.34,0.49),(-2.82,0.49),(-2.67,0.25),
                (-2.88,-0.11),(-4.30,-0.11),(-4.50,0.08)],0.14,"Titanium",side*2.145,0.065,quiet=True)
        c.box("TakedownSeat"+str(side),(4.26,side*2.047,0.04),(1.27,0.13,0.69),"Rubber",0.22)
        c.box("TakedownLever"+str(side),(4.26,side*2.142,0.04),(1.02,0.09,0.43),"Titanium",0.18)
        profile("MagazineReleaseHousing"+str(side),[(-2.97,-2.82),(-2.20,-3.03),(-2.02,-3.52),
                (-2.41,-4.0),(-3.25,-3.66),(-3.35,-3.15)],0.26,"Titanium",side*1.965,0.10)
        c.box("MagazineRelease"+str(side),(-2.72,side*2.132,-3.38),(0.61,0.08,0.66),"Gunmetal",0.10,rotation=(0,-17,0))
        for index in range(3):
            c.box("ReleaseTraction_%s_%s"%(side,index),(-2.72,side*2.18,-3.56+index*0.17),(0.41,0.02,0.035),"Charcoal",0.005)
        screw("ReceiverPin"+str(side),-1.96,side*2.095,0.05,0.14)
    profile("FrontstrapInset",[(-2.97,-4.43),(-3.44,-4.67),(-5.19,-10.46),
            (-4.83,-10.56),(-4.49,-10.11)],2.54,"BlackGrip",bevel=0.06)
    profile("FlaredMagazineHeel",[(-10.53,-10.83),(-5.10,-11.04),(-4.27,-10.75),(-3.93,-11.55),
            (-4.34,-12.00),(-10.94,-12.00),(-11.0,-11.63)],4.50,"Charcoal",bevel=0.08)
    profile("HeelBasePlate",[(-10.81,-11.60),(-4.18,-11.61),(-4.34,-11.99),(-10.91,-11.99)],4.31,"Magazine",bevel=0.035)
    c.box("MagazineFloorLatch",(-7.60,0,-11.965),(1.30,1.40,0.06),"Rubber",0.08)
    profile("UnderbarrelRailSpine",[(6.68,-0.10),(15.30,-0.10),(15.15,-0.66),(9.48,-0.66),
            (8.94,-0.87),(6.89,-0.68)],2.70,"Charcoal",bevel=0.075)
    for index in range(4):
        c.box("AccessoryRailLug%02d"%index,(10.05+index*1.28,0,-0.83),(0.63,2.89,0.33),"Gunmetal",0.055)
    sights(True)
    c.collision_box((2.9,0,2.5),(28.0,4.30,6.7))
    c.collision_box((-6.8,0,-6.2),(8.4,4.5,11.6))
    c.collision_box((0.7,0,-2.2),(8.3,3.0,5.2))
    c.socket("MuzzleSocket",(17.0,0,3.76))
    c.socket("LaserSightSocket",(12.85,0,-1.03))
    c.socket("ShellEjectionSocket",(2.65,1.97,4.36),(0,0,0))
    archive_components()
    return c.end((28.0,4.5,18.0),0.12)


def render_previews(standard_obj,premium_obj):
    camera=c.setup_studio((1500,1000))
    camera.data.type="ORTHO"
    camera.data.ortho_scale=0.35
    scene=bpy.context.scene
    scene.view_settings.exposure=-1.0
    scene.render.image_settings.color_mode="RGB"
    scene.render.film_transparent=False
    for obj,style in ((standard_obj,"Standard"),(premium_obj,"Premium")):
        for label,view,scale in (("LeftSide",(3,-75,-1),0.345),
                                 ("FrontThreeQuarter",(52,-64,25),0.37),
                                 ("GameCamera",(32,-45,62),0.405)):
            camera.data.ortho_scale=scale
            c.render_view(camera,c.OUT/"Previews"/f"Pistol_{style}_{label}.png",view,(3,0,-2.5),[obj])
    standard_obj.location=c.ue((0,0,11.7))
    premium_obj.location=c.ue((0,0,-11.7))
    camera.data.ortho_scale=0.49
    scene.render.resolution_x=1500
    scene.render.resolution_y=1500
    c.render_view(camera,c.OUT/"Previews"/"Pistol_PairComparison.png",(18,-83,17),(3,0,-2.5),[standard_obj,premium_obj])
    standard_obj.location=(0,0,0)
    premium_obj.location=(0,0,0)
    for obj in c.objects:
        obj.hide_render=False
    # Keep the saved editable file at the UE pivot and organized for inspection.
    scene.render.resolution_x=1500
    scene.render.resolution_y=1000
    camera.data.ortho_scale=0.37
    camera.location=c.ue((52,-64,25))
    camera.rotation_euler=(c.ue((3,0,-2.5))-camera.location).to_track_quat("-Z","Y").to_euler()
    premium_obj.hide_set(True)
    bpy.ops.object.select_all(action="DESELECT")
    standard_obj.select_set(True)
    bpy.context.view_layer.objects.active=standard_obj
    bpy.context.scene["ReviewInstructions"]="Standard visible. Unhide Premium and hide Standard for alternate. EDITABLE collections contain per-component source meshes; all game meshes use the same origin and atlas."


c.init("Pistol")
bpy.context.preferences.filepaths.save_version=0
standard_obj=standard()
premium_obj=premium()
render_previews(standard_obj,premium_obj)
c.save_family()
