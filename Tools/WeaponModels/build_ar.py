"""Author the two reference-led Tuna AR meshes and their review renders.

Run with Blender 4.5 --background --factory-startup --python-exit-code 1.
Coordinates supplied to the shared helpers are Unreal centimetres.
"""
from pathlib import Path
import math
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))
import bpy
import bmesh
from mathutils import Vector
import common as c


def profile(name, outline, depth, material="Charcoal", y=0, bevel=0.12):
    """Extrude an intentionally designed X/Z silhouette, including concavities."""
    n = len(outline)
    verts = [c.ue((x, y + side * depth / 2, z)) for side in (-1, 1) for x, z in outline]
    faces = [tuple(reversed(range(n))), tuple(range(n, n * 2))]
    faces += [(i, (i + 1) % n, (i + 1) % n + n, i + n) for i in range(n)]
    mesh = bpy.data.meshes.new(name)
    mesh.from_pydata(verts, [], faces)
    mesh.update()
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.collection.objects.link(obj)
    return c.finish(obj, name, material, bevel, True)


def loft(name, sections, material="Charcoal", bevel=0.1):
    """Eight-sided receiver/forend with independent width, top and belly stations."""
    verts = []
    for x, width, bottom, top, chamfer in sections:
        h = width / 2
        cross = [(-h + chamfer, bottom), (h - chamfer, bottom), (h, bottom + chamfer),
                 (h, top - chamfer), (h - chamfer, top), (-h + chamfer, top),
                 (-h, top - chamfer), (-h, bottom + chamfer)]
        verts += [c.ue((x, y, z)) for y, z in cross]
    faces = [tuple(reversed(range(8))), tuple(range(len(verts) - 8, len(verts)))]
    for j in range(len(sections) - 1):
        for i in range(8):
            faces.append((j * 8 + i, j * 8 + (i + 1) % 8, (j + 1) * 8 + (i + 1) % 8, (j + 1) * 8 + i))
    mesh = bpy.data.meshes.new(name)
    mesh.from_pydata(verts, [], faces)
    mesh.update()
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.collection.objects.link(obj)
    return c.finish(obj, name, material, bevel, True)


def cut_slot(obj, name, x, z, length, height, through_depth=12, bevel=0.15):
    """True through-cut, not a black rectangle on the surface."""
    bpy.ops.mesh.primitive_cube_add(size=1, location=c.ue((x, 0, z)))
    cutter = bpy.context.object
    cutter.name = name + "_TemporaryCut"
    cutter.dimensions = (through_depth / 100, length / 100, height / 100)
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    mod = cutter.modifiers.new("Rounded slot ends", "BEVEL")
    mod.width = min(bevel, height * 0.45) / 100
    mod.segments = 3
    bpy.ops.object.modifier_apply(modifier=mod.name)
    bpy.context.view_layer.objects.active = obj
    mod = obj.modifiers.new(name, "BOOLEAN")
    mod.operation = "DIFFERENCE"
    mod.solver = "EXACT"
    mod.object = cutter
    bpy.ops.object.modifier_apply(modifier=mod.name)
    bpy.data.objects.remove(cutter, do_unlink=True)


def tidy(obj, material, edge=0.045):
    """Bevel newly cut vent rims and restore the atlas mapping."""
    bpy.context.view_layer.objects.active = obj
    if edge:
        mod = obj.modifiers.new("Machined slot lips", "BEVEL")
        mod.width = edge / 100
        mod.segments = 2
        bpy.ops.object.modifier_apply(modifier=mod.name)
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bmesh.ops.recalc_face_normals(bm, faces=list(bm.faces))
    bm.to_mesh(obj.data)
    bm.free()
    c._uv_to_tile(obj, material)


def tube(name, x, z, length, outer, inner, material="Gunmetal", segments=24):
    verts = []
    for xx in (x - length / 2, x + length / 2):
        for r in (outer, inner):
            for i in range(segments):
                a = 2 * math.pi * i / segments
                verts.append(c.ue((xx, r * math.cos(a), z + r * math.sin(a))))
    faces = []
    for i in range(segments):
        k = (i + 1) % segments
        faces += [(i, k, 2 * segments + k, 2 * segments + i),
                  (segments + k, segments + i, 3 * segments + i, 3 * segments + k),
                  (k, i, segments + i, segments + k),
                  (2 * segments + i, 2 * segments + k, 3 * segments + k, 3 * segments + i)]
    mesh = bpy.data.meshes.new(name)
    mesh.from_pydata(verts, [], faces)
    mesh.update()
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.collection.objects.link(obj)
    return c.finish(obj, name, material, 0.045, True)


def fastener(name, x, y, z, radius=0.28, material="Steel"):
    sign = 1 if y >= 0 else -1
    c.cylinder(name + "Seat", (x, y, z), radius * 1.28, 0.12, "Charcoal", (0, sign, 0), 16, 0.025)
    c.cylinder(name, (x, y + sign * 0.075, z), radius, 0.10, material, (0, sign, 0), 12, 0.022)
    c.box(name + "Hex", (x, y + sign * 0.135, z), (radius * 0.72, 0.025, radius * 0.72), "Rubber", 0.015)


def guard_and_trigger(premium):
    name = "Refined" if premium else "Forged"
    # Eight-profile closed guard loop; the lower edge is intentionally radiused.
    outline = [(-5.8, -3.3), (0.65, -3.3), (1.15, -4.1), (0.65, -6.8),
               (-0.4, -7.5), (-4.4, -7.5), (-5.5, -6.5)]
    obj = profile(name + "TriggerGuard", outline, 1.9, "Gunmetal", bevel=0.13)
    cut_slot(obj, "OpenGuard", -2.4, -5.25, 4.9, 3.15, bevel=0.65)
    tidy(obj, "Gunmetal", 0.035)
    profile(name + "CurvedTrigger", [(-2.9, -3.2), (-2.25, -3.25), (-2.5, -4.7),
            (-2.25, -5.65), (-1.65, -6.4), (-2.2, -6.2), (-3.0, -5.15), (-3.25, -4.35)],
            0.62, "Steel" if premium else "Gunmetal", bevel=0.08)


def receiver_controls(premium):
    for side in (-1, 1):
        y = side * (3.06 if premium else 2.88)
        # Separate selector hub, lever and readable detent marks.
        c.cylinder("SelectorHub", (-6.3, y, -2.1), 0.54, 0.22, "Gunmetal", (0, side, 0), 20, 0.05)
        profile("SelectorLever", [(-6.45, -1.94), (-8.1, -2.18), (-8.3, -2.5),
                (-7.95, -2.65), (-6.4, -2.4)], 0.27, "Titanium" if premium else "Steel", y + side * .15, 0.065)
        for xx, zz in [(-6.55, -1.07), (-5.35, -1.8), (-6.2, -3.15)]:
            c.box("SelectorDetent", (xx, y + side * .08, zz), (.19, .08, .38), "IvoryCeramic", .025)
        c.box("MagazineReleaseSeat", (0.5, y, -1.65), (1.4, .18, .83), "Charcoal", .12)
        c.box("MagazineRelease", (0.5, y + side * .16, -1.65), (1.0, .18, .62), "Titanium" if premium else "Gunmetal", .10)
        for k in range(3):
            c.box("ReleaseKnurl", (.2 + k * .30, y + side * .265, -1.65), (.08, .07, .47), "BlackGrip", .02)
        for x, z in [(-8.9, -1.5), (-.9, -3.9), (7.6, -.9)]:
            fastener("ReceiverPin", x, y, z, .24, "Gunmetal")
    # Right-hand physical ejection recess and moving bolt cover.
    c.box("EjectionWell", (1.3, 2.95 if premium else 2.75, 2.0), (7.1, .18, 1.58), "Rubber", .15)
    c.box("BoltSurface", (1.55, 3.055 if premium else 2.86, 2.07), (5.6, .11, .95), "Steel", .16)
    c.box("BoltTravelLip", (1.3, 3.12 if premium else 2.92, 2.91), (7.25, .2, .25), "Titanium", .03)
    c.cylinder("ForwardAssistSeat", (-6.1, 2.78, 2.1), .62, 1.0, "Charcoal", (0, 1, 0), 16, .06)
    c.cylinder("ForwardAssistCap", (-6.1, 3.32, 2.1), .49, .36, "Gunmetal", (0, 1, 0), 16, .05)
    c.box("ChargingSlot", (-5.0, -2.89, 2.4), (6.2, .15, .7), "Rubber", .13)
    c.box("ChargingBar", (-6.0, -3.02, 2.4), (3.8, .25, .38), "Steel", .08)
    c.box("ChargingHandle", (-4.15, -3.27, 2.4), (.85, .46, .73), "Gunmetal", .12)


def mag_strip(name, path, width, y, material, depth=.12):
    outline = [(x - width / 2, z) for x, z in path]
    outline += [(x + width / 2, z) for x, z in reversed(path)]
    return profile(name, outline, depth, material, y, .045)


def prepare_export_part(obj):
    """Resolve boolean n-gons and remove numerical zero-area rim slivers."""
    calm_atlas_detail(obj)
    bm=bmesh.new()
    bm.from_mesh(obj.data)
    bmesh.ops.remove_doubles(bm,verts=list(bm.verts),dist=1e-7)
    bmesh.ops.dissolve_degenerate(bm,edges=list(bm.edges),dist=1e-7)
    bmesh.ops.triangulate(bm,faces=list(bm.faces))
    tiny=[face for face in bm.faces if face.calc_area() <= 1e-11]
    if tiny:
        print("NUMERICAL_SLIVERS_REMOVED",obj.name,len(tiny))
        bmesh.ops.delete(bm,geom=tiny,context="FACES_ONLY")
    loose=[edge for edge in bm.edges if not edge.link_faces]
    if loose:
        bmesh.ops.delete(bm,geom=loose,context="EDGES")
    isolated=[vertex for vertex in bm.verts if not vertex.link_edges]
    if isolated:
        bmesh.ops.delete(bm,geom=isolated,context="VERTS")
    bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces))
    bm.to_mesh(obj.data)
    bm.free()


def standard():
    c.begin("SM_TunaWeapon_AR_Standard", "AR", "Standard", "References/AR_Standard_Reference.png",
            "Practical industrial rifle: solid fixed stock, broad perforated olive handguard, forged receiver, stamped curved steel magazine, iron sights; six structural families differ from Premium.")
    # Solid fixed stock, tapering naturally into a high receiver attachment.
    profile("FixedStockCore", [(-30, 4.0), (-28.8, 4.9), (-17.2, 4.9), (-13.0, 5.15),
            (-10.4, 4.4), (-10.3, -.7), (-15.0, -1.35), (-26.8, -9.7), (-29.3, -9.4), (-30, -8.4)],
            4.8, "OlivePolymer", bevel=.35)
    profile("FixedRubberButt", [(-30, 4), (-29.35, 4.5), (-28.65, 4.25),
            (-28.4, -9.4), (-29.35, -9.65), (-30, -8.65)], 5.35, "Rubber", bevel=.26)
    for side in (-1, 1):
        profile("FixedStockInset", [(-27.4, 1.1), (-17.3, 2.45), (-17.1, .55), (-26.65, -6.4), (-27.4, -6.55)],
                .16, "OlivePolymer", y=side * 2.4, bevel=.18)
        profile("FixedStockRaisedEdge", [(-27.6, 1.5), (-17.0, 2.82), (-17.0, 2.4), (-27.3, 1.03)],
                .15, "OlivePolymer", y=side * 2.49, bevel=.05)
        fastener("StockAnchor", -27.15, side * 2.55, -7.9, .34, "Gunmetal")
        c.box("SlingEyeBack", (-28.25, side * 2.5, -1.7), (1, .2, 3.25), "Charcoal", .18)
        c.box("SlingEyeOpening", (-28.25, side * 2.64, -1.7), (.42, .08, 2.25), "Rubber", .14)
    for z in [-7.5, -6, -4.5, -3, -1.5, 0, 1.5, 3]:
        c.box("ButtTread", (-29.86, 0, z), (.12, 4.75, .25), "BlackGrip", .07)
    c.box("StockInterface", (-10.5, 0, 1.7), (1.5, 5.5, 6.2), "Gunmetal", .18)
    loft("ForgedUpper", [(-10, 5.2, .2, 5.15, .75), (-7.4, 5.7, .2, 5.25, .6),
         (7.4, 5.7, .2, 5.25, .6), (9.2, 5.4, .2, 4.9, .65)], "Charcoal", .16)
    profile("ForgedLower", [(-10, .6), (8.7, .6), (8.7, -4.6), (1.1, -5.4),
            (-.25, -4.3), (-4.5, -4.25), (-5.8, -3.45), (-9.6, -3.4)], 5.05, "Charcoal", bevel=.17)
    for side in (-1, 1):
        profile("LowerReceiverPlate", [(-9.45, -.05), (8.2, -.05), (8.2, -3.9), (1.7, -4.85),
                (.3, -3.65), (-5.3, -3.62), (-6.05, -2.9), (-9.45, -2.9)], .13, "Gunmetal", y=side * 2.56, bevel=.08)
        c.box("UpperReceiverSeam", (-.4, side * 2.89, .22), (18.0, .08, .15), "Steel", .025)
    receiver_controls(False)
    # Ergonomic, slanted grip; moulded edge and ribs have actual depth.
    grip_outline = [(-8.7, -3.0), (-4.45, -3.5), (-5.7, -6), (-7.95, -12.9),
                    (-8.85, -13.75), (-13.0, -12.15), (-10.0, -5.8)]
    profile("OliveGrip", grip_outline, 3.35, "OlivePolymer", bevel=.32)
    for side in (-1, 1):
        profile("GripInset", [(-9.9, -6.2), (-6.5, -6.4), (-8.6, -12.5), (-12.05, -11.3)],
                .10, "BlackGrip", y=side * 1.71, bevel=.1)
        for j in range(7):
            z = -7.0 - j * .62
            x = -6.15 - j * .21
            profile("GripFingerRib", [(x-.8,z+.14),(x+.12,z+.12),(x-.06,z-.09),(x-.86,z-.07)],
                    .16,"OlivePolymer",y=side*1.79,bevel=.035)
    profile("GripHeel", [(-13, -12.0), (-8.1, -13.65), (-8.55, -14.25), (-13.4, -12.65)], 3.64, "OlivePolymer", bevel=.12)
    guard_and_trigger(False)
    # Multi-station magazine sweep and curved longitudinal stamping ribs.
    profile("StampedMagazine", [(1.65,-4.6),(7.05,-4.05),(7.6,-8.0),(8.65,-12.0),(10.8,-15.6),
            (5.4,-17),(3.35,-12.8),(2.2,-8.25)], 3.45, "Magazine", bevel=.2)
    for side in (-1, 1):
        for offset in (0, 1.3, 2.6, 3.9):
            path=[(2.4+offset,-5.25),(2.9+offset,-8.5),(4.0+offset,-12.5),(5.65+offset,-15.8)]
            mag_strip("StampedLongitudinalRib", path, .20, side*1.8, "Gunmetal", .18)
        mag_strip("MagazineEdgeFlange",[(2.05,-5),(2.6,-9),(3.75,-13),(5.6,-16.65)],.19,side*1.76,"Steel",.14)
    profile("StampedFloorplate", [(5.35,-16.35),(10.65,-14.95),(11.0,-15.85),(5.45,-17)],3.8,"Gunmetal",bevel=.10)
    c.cylinder("BarrelChamber", (10.0,0,2.0), 1.35, 4, "Gunmetal", vertices=24)
    c.cylinder("Barrel", (21.5,0,2.0), .86, 24, "Gunmetal", vertices=24)
    hand = loft("OliveHandguard",[(9.2,5.95,-.95,4.95,.85),(11.0,6.3,-1.1,5.0,.95),
                  (24.7,5.8,-.8,4.7,.9)],"OlivePolymer",.2)
    for z in (3.42,1.20):
        for x in (13.0,17.45,21.9):
            cut_slot(hand,"Vent",x,z,3.25,.68,bevel=.27)
    tidy(hand,"OlivePolymer",.055)
    for x in (9.45,24.65):
        loft("HandguardBand",[(x-.35,6.0,-1.0,4.9,.9),(x+.35,6.0,-1.0,4.9,.9)],"Gunmetal",.07)
    for side in (-1,1):
        for x in (11.8,14.3,16.8,19.3,21.8,23.4):
            c.box("MouldedPalmRib",(x,side*2.98,-.03),(.38,.22,1.15),"OlivePolymer",.11)
        fastener("ForendRetainer",10.1,side*3.14,2.05,.31,"Gunmetal")
    c.box("PracticalHandStop", (22.9,0,-1.65),(1.0,3.0,1.8),"OlivePolymer",.3)
    c.cylinder("GasBlock",(26.35,0,2.0),1.28,1.55,"Charcoal",vertices=16)
    c.cylinder("GasTube",(27.4,0,3.65),.4,4.1,"Gunmetal",vertices=16)
    c.cylinder("MuzzleThreadCollar",(32.8,0,2),1.12,1.0,"Steel",vertices=24)
    muzzle=tube("PortedFlashHider",34.45,2,3.1,1.24,.69,"Gunmetal")
    for z in (2.0,):
        cut_slot(muzzle,"MuzzleSidePort",34.45,z,1.5,.58,through_depth=5,bevel=.22)
    tidy(muzzle,"Gunmetal",.025)
    tube("FlashHiderRim",35.72,2,.56,1.25,.69,"Steel")
    # Two open iron sights, no optic on the industrial variant.
    c.box("RearSightFoot",(-7.55,0,5.53),(3.2,3.1,.62),"Gunmetal",.10)
    profile("RearSightRamp",[(-9.0,5.65),(-8.2,7.55),(-7.1,7.85),(-6.25,6.1),(-6.0,5.65)],2.3,"Charcoal",bevel=.13)
    rear=profile("RearAperture",[(-8.1,6.6),(-8.1,8.45),(-7.7,9),(-6.9,9),(-6.5,8.45),(-6.5,6.6)],1.2,"Gunmetal",bevel=.09)
    # Sight tunnel runs forward along the bore.
    bpy.ops.mesh.primitive_cylinder_add(vertices=20,radius=.0034,depth=.04,location=c.ue((-7.3,0,8.0)))
    cutter=bpy.context.object
    cutter.rotation_euler=c.ue((1,0,0)).to_track_quat("Z","Y").to_euler()
    bpy.context.view_layer.objects.active=rear
    mod=rear.modifiers.new("SightAperture","BOOLEAN");mod.object=cutter;mod.operation="DIFFERENCE"
    bpy.ops.object.modifier_apply(modifier=mod.name);bpy.data.objects.remove(cutter,do_unlink=True)
    tidy(rear,"Gunmetal",.025)
    c.box("FrontSightSeat",(26.35,0,4.5),(1.7,2.65,1.3),"Gunmetal",.12)
    for side in (-1,1):
        profile("FrontSightWing",[(25.7,4.8),(25.7,8.55),(26.15,9),(26.8,8.7),(26.95,4.8)],.4,"Gunmetal",y=side*.95,bevel=.09)
    c.box("FrontSightPost",(26.35,0,7),( .36,.34,2.05),"Steel",.035)
    c.collision_box((-11,0,.1),(38,5.8,10.0))
    c.collision_box((22.5,0,2.0),(27,6.6,7.0))
    c.collision_box((-.7,0,-9.2),(25,4.0,15.4))
    c.socket("MuzzleSocket",(36,0,2))
    c.socket("LaserSightSocket",(22,3.35,1.7))
    c.socket("ShellEjectionSocket",(2.5,3.3,2.1),(0,0,0))
    for part in c.parts:
        prepare_export_part(part)
    return c.end((66,7,26),.07)


def premium():
    c.begin("SM_TunaWeapon_AR_Premium","AR","Premium","References/AR_Premium_Reference.png",
            "70% modern rifle / 30% restrained near-future: subtly tapered faceted receiver, slim cut-through handguard, ivory ceramic side armour, compact optic and sensor, adjustable open stock, modular curved magazine, refined grip.")
    # Adjustable buffer stock with a visible tube and an open triangular brace.
    c.cylinder("BufferTube",(-16.1,0,2.3),1.17,11.2,"Gunmetal",vertices=24)
    for x in (-12.7,-13.15,-13.6):
        c.cylinder("BufferIndexRing",(x,0,2.3),1.29,.22,"Steel",vertices=24,bevel_cm=.035)
    profile("AdjustableStockShell",[(-29.7,4.9),(-26.4,5.35),(-17.0,5.2),(-15.25,4.2),(-16.2,.65),
            (-20.6,-1.65),(-23.1,-2.0),(-27.0,-9.0),(-29.8,-8.65)],4.6,"GraphiteCeramic",bevel=.24)
    stock=c.parts[-1]
    # Large purposeful open stock frame: polygon boolean, preserving a diagonal load path.
    cutter=profile("TemporaryStockVoid",[(-27.3,-1.9),(-22.5,-2.4),(-27.2,-7.2)],10,"Rubber",bevel=.26)
    c.parts.remove(cutter)
    bpy.context.view_layer.objects.active=stock
    mod=stock.modifiers.new("OpenStockBrace","BOOLEAN");mod.object=cutter;mod.operation="DIFFERENCE";mod.solver="EXACT"
    bpy.ops.object.modifier_apply(modifier=mod.name);bpy.data.objects.remove(cutter,do_unlink=True)
    tidy(stock,"GraphiteCeramic",.07)
    profile("AdjustableButtPad",[(-30,4.65),(-29.1,5.0),(-28.7,4.4),(-28.8,-8.9),(-29.45,-9.2),(-30,-8.5)],5.2,"Rubber",bevel=.23)
    profile("CheekRiser",[(-27.9,5.25),(-18.0,5.25),(-16.9,4.5),(-18.1,2.55),(-26.35,2.55)],5.05,"Charcoal",bevel=.22)
    for side in (-1,1):
        profile("StockIvoryInlay",[(-27.65,-1.75),(-26.1,-1.75),(-25.45,-2.3),(-27.4,-6.9),(-27.8,-6.3)],
                .15,"IvoryCeramic",y=side*2.38,bevel=.10)
        c.box("AdjustmentLatch",(-20.6,side*1.6,-1.15),(2.2,.8,.5),"Steel",.12)
        fastener("StockAdjustmentPin",-18.0,side*2.45,.7,.29,"Titanium")
        fastener("StockBracePin",-27.7,side*2.46,-7.9,.26,"Gunmetal")
    for z in (-7.7,-6.5,-5.3,-4.1,-2.9,-1.7,-.5,.7,1.9,3.1):
        c.box("ButtMicroRib",(-29.89,0,z),(.13,4.5,.25),"BlackGrip",.05)
    # Receiver has a controlled taper in the core, not an oversized sci-fi shell.
    loft("FacetedUpper",[(-11.3,4.8,-.2,4.2,.9),(-9.8,6.0,-.45,5.6,1.0),
          (-3.8,6.0,-.45,5.6,.9),(7.4,5.3,-.2,5.0,.7),(9.3,5.0,.2,4.9,.55)],"Charcoal",.14)
    profile("PremiumLower",[(-10.4,.25),(9.0,.25),(8.7,-4.8),(1.15,-5.7),(-.2,-4.2),
            (-5.0,-4.2),(-6.0,-3.3),(-8.8,-3.25),(-10.4,-2.2)],5.2,"GraphiteCeramic",bevel=.19)
    for side in (-1,1):
        profile("FacetedReceiverSide",[(-9.6,4.5),(-3.5,4.7),(6.6,4.2),(5.9,3.25),
                (-7.7,3.1),(-9.0,2.7)],.15,"Gunmetal",y=side*2.91,bevel=.06)
        profile("ReceiverLowerInset",[(1.4,-.75),(7.7,-.35),(7.4,-4.05),(1.45,-4.8)],.14,"Charcoal",y=side*2.62,bevel=.1)
        profile("MagazineWellRim",[(1.0,-4.8),(8.8,-3.9),(8.8,-4.6),(1.15,-5.65)],.22,"Titanium",y=side*2.7,bevel=.06)
    receiver_controls(True)
    profile("PremiumGripCore",[(-8.5,-3.0),(-4.5,-3.65),(-5.4,-5.6),(-8.35,-13.7),
            (-9.5,-14.25),(-12.85,-12.65),(-10.2,-5.75)],3.5,"GraphiteCeramic",bevel=.30)
    for side in (-1,1):
        profile("PremiumGripPalm",[(-9.6,-6.45),(-6.2,-7.0),(-8.5,-13.25),(-12.1,-11.9)],.13,"BlackGrip",y=side*1.78,bevel=.11)
        profile("GripBackSpine",[(-10.25,-5.75),(-9.85,-6),(-12.1,-11.85),(-12.65,-12.15)],.13,"IvoryCeramic",y=side*1.77,bevel=.08)
        fastener("GripInsertScrew",-10.0,side*1.89,-10.2,.19,"Gunmetal")
    profile("PremiumGripHeel",[(-12.9,-12.6),(-8.35,-14.0),(-8.6,-14.6),(-13.15,-13.1)],3.75,"Charcoal",bevel=.12)
    guard_and_trigger(True)
    # Thicker polymer magazine, curved with stepped external reinforcement.
    profile("ModularMagazine",[(1.7,-4.7),(7.1,-4.2),(7.5,-8.0),(8.4,-11.8),(10.6,-15.5),
            (5.25,-17),(3.3,-12.6),(2.25,-8.7)],3.75,"GraphiteCeramic",bevel=.23)
    for side in (-1,1):
        for offset in (0,3.9):
            mag_strip("MagazineReinforcement",[(2.65+offset,-5.25),(3.1+offset,-9),(4.1+offset,-12.5),(5.8+offset,-15.85)],.38,side*1.94,"Gunmetal",.23)
        # Distinct modules follow the curve instead of floating over a straight prism.
        for outline in (
            [(3.4,-5.5),(6.4,-5.2),(6.75,-7.85),(3.8,-8.2)],
            [(3.95,-8.6),(6.9,-8.15),(7.65,-10.9),(4.7,-11.55)],
            [(4.85,-11.9),(7.8,-11.3),(9.25,-14.15),(6.1,-15.0)]):
            profile("MagazineInsetPanel",outline,.15,"Magazine",y=side*1.97,bevel=.13)
        for path in ([ (3.05,-8.35),(7.3,-7.8) ],[ (4.0,-11.75),(8.2,-10.85) ]):
            profile("MagazineCrossRib",[(path[0][0],path[0][1]+.12),(path[1][0],path[1][1]+.12),
                (path[1][0],path[1][1]-.13),(path[0][0],path[0][1]-.13)],.15,"Gunmetal",y=side*2.04,bevel=.045)
    profile("MagazineBumper",[(5.12,-16.4),(10.45,-14.92),(10.95,-15.83),(5.32,-17)],4.08,"Rubber",bevel=.15)
    profile("MagazineBumperEdge",[(5.20,-16.28),(10.45,-14.87),(10.63,-15.24),(5.38,-16.74)],4.16,"Titanium",bevel=.08)
    c.cylinder("PremiumBarrel",(21.8,0,2.3),.72,23.6,"Gunmetal",vertices=24)
    c.cylinder("PremiumBarrelCollar",(10.2,0,2.3),1.38,2.5,"Steel",vertices=24)
    # Hollow handguard section plus paired independent ceramic armour plates.
    hand=loft("SlimHandguard",[(8.7,5.25,-.35,4.95,.75),(11.1,5.05,-.30,4.82,.75),
            (25.3,4.30,.05,4.3,.65),(27.2,4.1,.20,4.15,.6)],"Gunmetal",.13)
    for x in (14.1,18.1,22.1,25.6):
        cut_slot(hand,"UpperCoolingCut",x,3.60,2.9,.55,bevel=.23)
        cut_slot(hand,"LowerCoolingCut",x,.84,2.9,.58,bevel=.22)
    for x in (14.1,18.1,22.1):
        cut_slot(hand,"MainCoolingCut",x,2.27,3.05,.70,bevel=.25)
    tidy(hand,"Gunmetal",.038)
    for side in (-1,1):
        panel=profile("IvoryForendPanel",[(11.0,3.28),(25.85,3.18),(26.05,1.45),(25.2,.15),
                      (12.0,.1),(10.65,1.0)],.20,"IvoryCeramic",y=side*2.35,bevel=.1)
        for x in (14.1,18.1,22.1):
            cut_slot(panel,"CeramicVent",x,2.27,3.05,.70,bevel=.25)
        for x in (14.1,18.1,22.1,25.6):
            cut_slot(panel,"CeramicLowerVent",x,.84,2.9,.58,bevel=.22)
        tidy(panel,"IvoryCeramic",.03)
        for x,z in ((11.7,2.55),(25.45,2.67),(12.0,.4)):
            fastener("CeramicPanelScrew",x,side*2.50,z,.21,"Titanium")
    # Real front opening and a slender under rail / integrated hand stop.
    tube("FrontBarrelRing",27.1,2.25,1.1,1.77,.83,"Charcoal",segments=16)
    c.box("LowerRailSpine",(20.4,0,-.43),(12.8,1.7,.55),"Charcoal",.075)
    c.rail_teeth("LowerRailSlot",14.5,25.2,0,-.76,13,"Gunmetal",.5,.32)
    profile("AngledHandstop",[(11.0,-.28),(15.2,-.28),(14.5,-1.25),(12.9,-1.55),
            (12.55,-3.55),(11.85,-3.55),(10.9,-1.15)],2.25,"GraphiteCeramic",bevel=.18)
    for side in (-1,1):
        profile("HandstopFace",[(11.8,-.95),(13.9,-.95),(13.45,-1.3),(12.45,-1.65),(12.2,-2.8)],.1,"Gunmetal",y=side*1.17,bevel=.05)
    # Small side sensor; width defines the exact 7 cm asset envelope.
    c.box("SensorMount",(10.25,-2.71,1.4),(3.8,.58,1.45),"Charcoal",.18)
    profile("SensorHousing",[(8.3,1.7),(11.3,1.7),(12,1.05),(11.3,.25),(8.5,.25)],.67,"GraphiteCeramic",y=-3.165,bevel=.13)
    c.box("SensorLight",(10.1,-3.48,.97),(1.55,.04,.23),"CyanAccent",.055)
    c.cylinder("SensorForwardLens",(11.88,-3.03,1.02),.29,.14,"CyanAccent",vertices=16,bevel_cm=.02)
    c.box("TopRailSpine",(7.0,0,5.26),(34.5,1.6,.34),"Charcoal",.055)
    c.rail_teeth("TopRailTeeth",-9.7,23.5,0,5.56,35,"Gunmetal",.49,.28)
    # Closed reflex optic housing with forward-facing glass, mounting foot and dials.
    c.box("OpticClamp",(-3.8,0,6.06),(7.7,2.75,.72),"Charcoal",.14)
    optic=loft("CompactOpticShell",[(-7.6,3.25,6.33,8.4,.45),(-6.6,3.65,6.4,9.0,.55),
               (-1.2,3.45,6.4,8.9,.45),(.0,3.05,6.65,8.70,.40)],"GraphiteCeramic",.11)
    # Dark recessed lens surround and cyan central aperture are genuine geometry.
    c.box("OpticForwardRecess",(.045,0,7.62),(.12,2.40,1.58),"Rubber",.28)
    c.box("OpticGlass",(.12,0,7.62),(.075,1.96,1.15),"VioletLens",.23)
    c.box("OpticReticleGlint",(.175,0,7.62),(.035,.14,.64),"CyanAccent",.035)
    c.box("OpticRearLens",(-7.64,0,7.3),(.07,2.1,1.14),"SmokedPolymer",.17)
    for side in (-1,1):
        profile("OpticSideShell",[(-6.6,8.72),(-4.0,8.72),(-4.3,7.95),(-3.8,6.6),(-6.8,6.6),(-7.2,7.4)],
                .13,"Charcoal",y=side*1.82,bevel=.075)
        c.cylinder("OpticTurret",(-6.4,side*2.02,7.15),.59,.40,"Gunmetal",(0,side,0),20,.055)
        c.cylinder("OpticTurretCap",(-6.4,side*2.26,7.15),.41,.10,"Steel",(0,side,0),16,.03)
        c.box("TurretNotch",(-6.4,side*2.34,7.15),(.35,.04,.10),"Rubber",.02)
        fastener("OpticClampBolt",-2.2,side*1.47,6.0,.23,"Titanium")
    c.box("FrontBackupSight",(25.05,0,4.68),(2.3,1.7,.64),"Charcoal",.12)
    c.cylinder("FrontBackupPivot",(25.3,0,4.95),.42,2.0,"Steel",(0,1,0),16,.035)
    for x in (28.1,30.0,32.0):
        c.cylinder("BarrelMachiningBand",(x,0,2.3),.82,.28,"Titanium",vertices=24,bevel_cm=.03)
    c.cylinder("CompensatorNeck",(32.45,0,2.3),1.02,.85,"Gunmetal",vertices=24)
    muzzle=tube("PremiumCompensator",34.35,2.3,3.3,1.23,.66,"Gunmetal",segments=24)
    for x in (33.6,34.7):
        for z in (1.85,2.76):
            cut_slot(muzzle,"CompensatorSidePort",x,z,.71,.37,through_depth=5,bevel=.13)
    tidy(muzzle,"Gunmetal",.025)
    tube("CompensatorFrontRing",35.78,2.3,.44,1.24,.66,"Titanium")
    c.collision_box((-11,0,.3),(38,6.0,10.6))
    c.collision_box((22.2,0,2.3),(27.6,5.4,6.4))
    c.collision_box((-.8,0,-9.3),(25.5,4.2,15.4))
    c.socket("MuzzleSocket",(36,0,2.3))
    c.socket("LaserSightSocket",(12,-3.03,1.02))
    c.socket("ShellEjectionSocket",(2.5,3.3,2.1),(0,0,0))
    for part in c.parts:
        prepare_export_part(part)
    return c.end((66,7,26),.07)


def calm_atlas_detail(obj):
    """Use a quieter subregion for painted panels; grip/magazine keep full texture."""
    for polygon in obj.data.polygons:
        key=obj.data.materials[polygon.material_index].name.removeprefix("M_TunaWeapon_")
        if key in {"BlackGrip","TanGrip","Rubber","Steel","Titanium"}:
            continue
        col,row,*_=c.TILES[key]
        cx=(col+.5)/4;cy=(3-row+.5)/4
        target_x=(col+.72)/4;target_y=(3-row+.74)/4
        for index in polygon.loop_indices:
            uv=obj.data.uv_layers.active.data[index].uv
            # Cropped atlas avoids stamping an entire manufactured panel into
            # each little surface while retaining ImageGen paint variation.
            uv.x=target_x+(uv.x-cx)*.13
            uv.y=target_y+(uv.y-cy)*.13


def main():
    c.init("AR")
    bpy.context.preferences.filepaths.save_version=0
    standard_obj=standard()
    premium_obj=premium()
    # The UVs written by end() are already valid atlas UVs. Keep exported data
    # and editable source identical; no render-only recolouring is applied.
    camera=c.setup_studio((1500,950))
    bpy.context.scene.view_settings.exposure=-1.35
    camera.data.type="ORTHO"
    camera.data.ortho_scale=.78
    bpy.context.scene.render.film_transparent=False
    for obj,style in ((standard_obj,"Standard"),(premium_obj,"Premium")):
        c.render_view(camera,c.OUT/"Previews"/f"AR_{style}_Left.png",(3,-140,-2),(3,0,-3),[obj])
        c.render_view(camera,c.OUT/"Previews"/f"AR_{style}_FrontThreeQuarter.png",(84,-130,65),(3,0,-3),[obj])
        c.render_view(camera,c.OUT/"Previews"/f"AR_{style}_GameTop.png",(48,-100,145),(3,0,-3),[obj])
    # Pair review uses equal-scale meshes, translated for the render only.
    standard_obj.location.z=.17
    premium_obj.location.z=-.17
    camera.data.ortho_scale=.85
    bpy.context.scene.render.resolution_x=1500
    bpy.context.scene.render.resolution_y=1250
    c.render_view(camera,c.OUT/"Previews"/"AR_PairComparison.png",(3,-160,-3),(3,0,-3),c.objects)
    standard_obj.location=(0,0,0)
    premium_obj.location=(0,0,0)
    for obj in c.objects:
        obj.hide_render=False
    # Both attachment pivots stay at identity. Show Standard on opening the
    # editable family file; Premium remains available in the Outliner.
    premium_obj.hide_set(True)
    c.save_family()


if __name__=="__main__":
    main()
