"""Editable SMG pair built in Blender 4.5 from the approved final references.

Run: blender --background --factory-startup --python-exit-code 1 --python Tools/WeaponModels/build_smg.py
All design coordinates are centimetres in the common Unreal axis contract.
"""
from pathlib import Path
import math
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))
import bpy
import bmesh
from mathutils import Vector
import common as c


def recalculate_normals(mesh):
    bm=bmesh.new()
    bm.from_mesh(mesh)
    bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces))
    bm.to_mesh(mesh)
    bm.free()


def profile_mesh(name, points, width, y=0):
    n = len(points)
    vertices = [c.ue((x, y + side * width / 2, z)) for side in (-1, 1) for x, z in points]
    faces = [tuple(range(n - 1, -1, -1)), tuple(range(n, 2 * n))]
    faces += [(i, (i + 1) % n, (i + 1) % n + n, i + n) for i in range(n)]
    mesh = bpy.data.meshes.new(name)
    mesh.from_pydata(vertices, [], faces)
    mesh.update()
    recalculate_normals(mesh)
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.collection.objects.link(obj)
    return obj


def profile(name, points, width, material='Charcoal', bevel=0.10, y=0):
    return c.finish(profile_mesh(name, points, width, y), name, material, bevel)


def frame(name, outer, inner, width, material='Charcoal', bevel=0.10, y=0):
    assert len(outer) == len(inner)
    n = len(outer)
    vertices = [c.ue((x, y + side * width / 2, z)) for side in (-1, 1) for loop in (outer, inner) for x, z in loop]
    faces = []
    for i in range(n):
        j = (i + 1) % n
        faces.extend([(i, j, j + n, i + n), (i + 2*n, i + 3*n, j + 3*n, j + 2*n),
                      (i, i + 2*n, j + 2*n, j), (i + n, j + n, j + 3*n, i + 3*n)])
    mesh = bpy.data.meshes.new(name)
    mesh.from_pydata(vertices, [], faces)
    mesh.update()
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.collection.objects.link(obj)
    return c.finish(obj, name, material, bevel)


def capsule_points(cx, cz, length, height, steps=6):
    radius = height / 2
    straight = max(0, length / 2 - radius)
    pts = []
    for center, start in ((cx + straight, -90), (cx - straight, 90)):
        for i in range(steps + 1):
            angle = math.radians(start + 180 * i / steps)
            pts.append((center + radius * math.cos(angle), cz + radius * math.sin(angle)))
    return pts


def difference(target, cutter):
    bpy.ops.object.select_all(action='DESELECT')
    target.select_set(True)
    bpy.context.view_layer.objects.active = target
    modifier = target.modifiers.new('Machined opening', 'BOOLEAN')
    modifier.operation = 'DIFFERENCE'
    modifier.solver = 'EXACT'
    modifier.object = cutter
    bpy.ops.object.modifier_apply(modifier=modifier.name)
    bpy.data.objects.remove(cutter, do_unlink=True)


def verify_parts():
    for obj in c.parts:
        obj.data.calc_loop_triangles()
        bad=[triangle for triangle in obj.data.loop_triangles if triangle.area <= 1e-13]
        assert not bad, (obj.name, 'degenerate triangulation', len(bad))


def hollow_tube(name, x_start, x_end, z, outer, inner, material='Gunmetal', sides=24, port_rows=(0,)):
    vertices = []
    for x, radius in ((x_start, outer), (x_end, outer), (x_start, inner), (x_end, inner)):
        for i in range(sides):
            a = 2 * math.pi * i / sides
            vertices.append(c.ue((x, radius * math.cos(a), z + radius * math.sin(a))))
    faces = []
    for i in range(sides):
        j = (i + 1) % sides
        faces += [(i, j, j+sides, i+sides), (i+2*sides, i+3*sides, j+3*sides, j+2*sides),
                  (i, i+2*sides, j+2*sides, j), (i+sides, j+sides, j+3*sides, i+3*sides)]
    mesh = bpy.data.meshes.new(name)
    mesh.from_pydata(vertices, [], faces)
    mesh.update()
    recalculate_normals(mesh)
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.collection.objects.link(obj)
    for dz in port_rows:
        difference(obj,profile_mesh('FlashPortCut',capsule_points((x_start+x_end)/2,z+dz,1.40,.31),outer*2+.5))
    return c.finish(obj, name, material, 0.04)


def screw(name, x, y, z, radius=0.23, material='Steel'):
    c.cylinder(name + 'Recess', (x, y, z), radius * 1.30, .08, 'Rubber', (0, 1, 0), 12, .018)
    c.cylinder(name + 'Fastener', (x, y + (.05 if y > 0 else -.05), z), radius, .12, material, (0, 1, 0), 12, .025)
    c.box(name + 'Socket', (x, y + (.12 if y > 0 else -.12), z), (radius*.65, .025, radius*.38), 'Charcoal', .008)


def ring_sight(name, x, z, outer=.77, inner=.46):
    # Sight is a genuine aperture perpendicular to the barrel.
    n = 16
    verts = []
    for dx in (-.22, .22):
        for radius in (outer, inner):
            for i in range(n):
                a = 2*math.pi*i/n
                verts.append(c.ue((x+dx, radius*math.cos(a), z+radius*math.sin(a))))
    faces = []
    for i in range(n):
        j=(i+1)%n
        faces += [(i,j,j+n,i+n),(i+2*n,i+3*n,j+3*n,j+2*n),
                  (i,i+2*n,j+2*n,j),(i+n,j+n,j+3*n,i+3*n)]
    mesh=bpy.data.meshes.new(name)
    mesh.from_pydata(verts,[],faces)
    mesh.update()
    obj=bpy.data.objects.new(name,mesh)
    bpy.context.collection.objects.link(obj)
    c.finish(obj,name,'Gunmetal',.035)


def standard():
    c.begin('SM_TunaWeapon_SMG_Standard', 'SMG', 'Standard', 'References/SMG_Standard_Reference.png',
            'Compact industrial blowback silhouette: sculpted fixed olive stock, stamped slab receiver, twin-row perforated handguard, curved fluted steel magazine, tan ribbed grip and protected aperture sights. Hollow muzzle and full trigger/stock construction.')
    # Broad readable receiver with a separate lower shell and pressed panels.
    profile('UpperReceiver', [(-12,1.0),(-12,5.45),(-11.3,5.9),(7.7,5.9),(8.3,5.35),(8.3,1.0)], 4.1, 'Charcoal', .15)
    profile('LowerReceiver', [(-11.7,1.2),(8.1,1.2),(7.6,-1.3),(3,-3.55),(-1,-3.1),(-1,-1.4),(-10.8,-1.4)], 3.75, 'Charcoal', .13)
    c.box('TopSeam', (-1.7,0,5.76), (19.1,3.8,.28), 'Gunmetal', .055)
    for side in (-1,1):
        profile('StampedSidePanel', [(-11.2,.15),(-11.2,2.60),(0.1,2.60),(.3,.1)], .16, 'Gunmetal', .045, side*2.1)
        c.box('BoltPortDark', (3.45,side*2.105,3.30), (5.2,.18,1.25), 'Rubber', .18)
        c.box('BoltVisibleSteel', (3.45,side*2.22,3.15), (4.25,.10,.48), 'Steel', .055)
        c.box('PortLip', (3.45,side*2.28,2.64), (5.15,.12,.14), 'Gunmetal', .022)
        screw('RearTakedown', -10.25,side*2.26,.72,.24,'Titanium')
        screw('FrontTakedown', 5.9,side*1.99,-1.33,.25,'Titanium')
        screw('ReceiverRivet', 4.6,side*2.14,4.6,.13,'Gunmetal')
    # Fixed stock has sloping shoulder transition and a separate rubber butt pad.
    profile('FixedStock', [(-23.1,5.1),(-17.5,5.1),(-14.1,5.5),(-12.2,5.35),(-12.2,1.25),(-17.3,1.25),(-21.1,-4.45),(-23.1,-4.45)], 3.5,'OlivePolymer',.23)
    profile('StockButtPad', [(-24,4.75),(-23.48,5.18),(-22.65,5.1),(-22.65,-4.5),(-23.55,-4.5),(-24,-4.0)], 3.85,'Rubber',.17)
    c.box('StockFerrule', (-12.25,0,3.23),(.72,4.03,4.72),'Gunmetal',.13)
    for side in (-1,1):
        profile('StockReinforcement', [(-22.45,4.46),(-17.4,4.46),(-14,4.88),(-13.1,4.78),(-13.1,2.0),(-17.8,2.0),(-21.55,-3.5),(-22.45,-3.5)],.10,'OlivePolymer',.09,side*1.78)
        screw('ButtPin',-21.95,side*1.91,1.0,.20,'Gunmetal')
    for z in (-3.25,-2.3,-1.35,-.4,.55,1.5,2.45,3.4):
        c.box('ButtTread',(-23.95,0,z),(.10,3.55,.22),'BlackGrip',.028)
    # Handguard has four capsule cuts through both sides and a recessed barrel.
    handguard = profile_mesh('PerforatedHandguard',[(8,.8),(8,5.55),(9.0,5.85),(18.8,5.85),(19.4,5.25),(19.4,1.0),(18.6,.55),(9.0,.55)],5.6)
    bore = profile_mesh('InteriorBore',[(7.9,1.7),(7.9,4.6),(19.5,4.6),(19.5,1.7)],3.5)
    difference(handguard,bore)
    for x in (11.25,16.15):
        for z in (2.0,4.35):
            difference(handguard,profile_mesh('VentCut',capsule_points(x,z,3.4,.64),6.0))
    c.finish(handguard,'PerforatedHandguard','OlivePolymer',.11)
    c.cylinder('BarrelInside', (14.3,0,3.15), .63, 13.2,'Gunmetal',(1,0,0),20,.035)
    for x in (8.1,19.2):
        c.box('HandguardClamp',(x,0,3.15),(.45,5.75,5.20),'Gunmetal',.16)
        for side in (-1,1):
            screw('GuardClampScrew',x,side*2.86,4.35,.20,'Steel')
    c.box('HandguardBottomRail',(13.6,0,.45),(8.9,2.2,.48),'Charcoal',.08)
    c.rail_teeth('LowerAccessoryRib',9.75,17.7,0,.10,10,'Gunmetal',.42,.3)
    c.cylinder('BarrelShoulder',(20.25,0,3.15),.83,1.65,'Steel',(1,0,0),24,.05)
    c.cylinder('MuzzleCollar',(21.10,0,3.15),1.03,.60,'Gunmetal',(1,0,0),24,.07)
    hollow_tube('OpenFlashHider',21.35,24,3.15,1.13,.65)
    # Pistol grip and open trigger guard are shaped polygon extrusions.
    profile('GripCore',[(-10.8,-1.15),(-6.3,-1.15),(-6.1,-2.8),(-8.7,-9.25),(-9.5,-9.70),(-13.0,-8.3),(-13.3,-7.55),(-10.2,-2.6)],3.20,'OlivePolymer',.18)
    for side in (-1,1):
        profile('GripInset',[(-10.25,-3.35),(-7.15,-3.70),(-9.35,-8.65),(-12.18,-7.8)],.16,'TanGrip',.10,side*1.65)
        for i in range(7):
            z=-4.02-i*.55
            center_x=-8.93-i*.255
            profile('MoldedGripRib',[(center_x-1.30,z+.13),(center_x+1.25,z-.16),(center_x+1.13,z-.32),(center_x-1.40,z-.02)],.10,'WarmGrayPolymer',.04,side*1.76)
    frame('TriggerGuard',[(-6.95,-1.05),(-.9,-1.05),(-.40,-1.8),(-.80,-4.42),(-1.65,-4.85),(-6.23,-4.85),(-7,-4.1),(-7.2,-2.0)],
          [(-6.48,-1.66),(-1.44,-1.66),(-1.03,-2.06),(-1.35,-3.97),(-1.88,-4.28),(-5.91,-4.28),(-6.43,-3.74),(-6.67,-2.13)],1.4,'Gunmetal',.07)
    profile('CurvedTrigger',[(-4.9,-1.46),(-4.06,-1.46),(-4.44,-2.35),(-4.50,-3.0),(-4.19,-3.75),(-4.5,-3.86),(-5.07,-3.10),(-5.2,-2.36)],.52,'Steel',.055)
    # Curved magazine: curvature is modeled in the shell and longitudinal ribs.
    profile('CurvedMagazine',[(.55,-2.05),(4.9,-2.05),(5.10,-5.55),(5.75,-8.9),(7.20,-11.95),(3.2,-13),(1.75,-9.65),(.94,-5.9)],2.52,'Magazine',.13)
    profile('MagazineFloorplate',[(3.0,-12.3),(7.32,-11.18),(7.68,-11.98),(3.3,-13.0)],2.93,'Gunmetal',.07)
    c.box('MagazineFeedCollar',(2.70,0,-2.10),(4.7,2.98,1.0),'Charcoal',.11)
    for side in (-1,1):
        for offset in (.92,2.25,3.58):
            profile('MagazineLongFlute',[(offset,-3.15),(offset+.20,-6.0),(offset+1.0,-9.45),(offset+2.15,-11.99),
                    (offset+2.46,-11.87),(offset+1.32,-9.38),(offset+.53,-5.96),(offset+.34,-3.15)],.09,'Gunmetal',.035,side*1.30)
    c.box('MagazineCatch',(1.0,1.95,-1.4),(1.2,.40,.64),'Steel',.07)
    profile('MagazineReleasePaddle',[(.6,-2.35),(1.4,-2.35),(1.25,-4.4),(.69,-4.6),(.44,-4.14)],.90,'Gunmetal',.08)
    c.cylinder('SelectorAxle',(-8.1,-2.31,.8),.46,.28,'Steel',(0,1,0),16,.04)
    profile('SelectorLever',[(-8.2,.5),(-8.8,.55),(-10.1,1.65),(-9.88,2.03),(-8.0,1.14)],.25,'Titanium',.06,-2.48)
    c.cylinder('ChargeHandleStem',(1.35,-2.58,3.18),.24,.64,'Steel',(0,1,0),12,.02)
    c.box('ChargingHandle',(1.35,-2.84,3.18),(1.35,.32,.49),'Gunmetal',.08)
    # Sights make the exact top extent 9 cm.
    for x in (-10.2,18.15):
        profile('SightBase',[(x-.85,5.7),(x+.85,5.7),(x+.53,7.4),(x-.53,7.4)],1.68,'Gunmetal',.085)
        ring_sight('ApertureSight',x,8.0,1.0,.65)
        screw('SightAdjustment',x,-.91,6.65,.25,'Steel')
    c.box('FrontSightPost',(18.15,0,7.66),(.24,.20,1.03),'Steel',.022)
    c.collision_box((-2.4,0,2.8),(42.9,5.8,6.3))
    c.collision_box((-9.6,0,-5.2),(6.0,3.4,9.1))
    c.collision_box((4.1,0,-7.65),(6.3,3.0,10.8))
    c.socket('MuzzleSocket',(24,0,3.15))
    c.socket('LaserSightSocket',(16.8,-2.88,3.15))
    c.socket('ShellEjectionSocket',(3.5,2.34,3.3),(0,0,0))
    return c.end((48,6,22),.15)


def premium():
    c.begin('SM_TunaWeapon_SMG_Premium','SMG','Premium','References/SMG_Premium_Reference.png',
            'Approximately 70 percent modern / 30 percent near-future: angular forged receiver, skeletal adjustable stock, straight compact magazine, sculpted grip, ivory heatshield plates, compact enclosed reflex optic and twin-channel side sensor. Functional modern silhouette retained with restrained cyan accents.')
    profile('ForgedUpper',[(-11.8,.75),(-11.8,5.6),(-10.9,6.25),(-4.4,6.25),(-3.75,6.05),(7.9,6.05),(8.65,5.35),(8.65,.8)],4.25,'Gunmetal',.16)
    profile('ForgedLower',[(-11.45,1.0),(8.45,1.0),(8.25,-1.35),(5.28,-1.70),(4.6,-3.68),(-.2,-3.84),(-1.25,-2.1),(-7.45,-2.1),(-9.25,-1.55),(-11.4,-.45)],3.85,'Charcoal',.14)
    for side in (-1,1):
        profile('UpperFacetPanel',[(-10.95,5.44),(-4.95,5.66),(-4.10,5.05),(4.6,5.05),(4.6,2.58),(-10.95,2.58)],.12,'GraphiteCeramic',.055,side*2.19)
        profile('LowerStructuralPanel',[(-10.55,1.64),(-4.22,1.64),(-3.48,.67),(2.25,.67),(2.86,1.25),(7.4,1.25),(7.1,-.73),(4.6,-1.05),(3.95,-2.88),(-.07,-3.13),(-.66,-1.43),(-8.9,-1.43)],.10,'Charcoal',.055,side*1.99)
        screw('RearReceiverPin',-10.25,side*2.30,4.76,.24,'Titanium')
        screw('LowerReceiverPin',6.9,side*2.10,.06,.29,'Titanium')
        c.box('StatusRecess',(.60,side*2.27,2.48),(2.25,.10,.25),'Rubber',.04)
        c.box('StatusIndicator',(.63,side*2.335,2.48),(1.54,.045,.12),'CyanAccent',.025)
    c.box('EjectionPort',(1.0,2.31,3.76),(4.15,.17,1.23),'Rubber',.15)
    c.box('BoltFace',(1.25,2.42,3.58),(3.38,.07,.63),'Steel',.05)
    c.box('EjectionPortUpperLip',(1.0,2.47,4.40),(4.35,.17,.18),'Gunmetal',.04)
    # Skeleton stock: modern tube and a real triangular opening.
    c.cylinder('StockTube',(-14.2,0,3.45),1.25,5.15,'Gunmetal',(1,0,0),20,.09)
    c.cylinder('StockLockRing',(-11.92,0,3.45),1.61,.48,'Charcoal',(1,0,0),20,.06)
    frame('SkeletonStock',[(-23.26,5.35),(-16.28,5.35),(-15.3,3.6),(-16.25,.62),(-21.70,-5.0),(-23.26,-4.6)],
          [(-22.03,3.85),(-17.23,3.85),(-16.75,3.03),(-17.22,1.66),(-21.62,-2.88),(-22.03,-2.42)],3.05,'Charcoal',.17)
    profile('AdjustableCheekRest',[(-23.25,5.6),(-16.9,5.6),(-15.78,4.98),(-17.13,2.8),(-21.38,2.8),(-22.3,3.13)],3.9,'GraphiteCeramic',.14)
    profile('ButtPad',[(-24,5.15),(-23.55,5.52),(-22.72,5.32),(-22.72,-4.75),(-23.52,-5.00),(-24,-4.43)],3.76,'Rubber',.16)
    for side in (-1,1):
        profile('StockBraceInset',[(-21.45,-2.99),(-17.05,1.74),(-16.8,1.28),(-21.30,-3.8)],.11,'Titanium',.055,side*1.57)
        screw('StockAdjustPivot',-19.65,side*1.7,1.70,.24,'Steel')
        screw('StockSlingPivot',-22.0,side*1.64,-3.75,.31,'Titanium')
    c.box('StockAdjustmentLever',(-17.98,0,.72),(2.56,2.35,.56),'Gunmetal',.10)
    for z in (-3.95,-2.9,-1.85,-.8,.25,1.3,2.35,3.4,4.45):
        c.box('ButtTractionRib',(-23.97,0,z),(.10,3.52,.24),'BlackGrip',.03)
    # Polygonal free-float handguard has its own hollow longitudinal cavity.
    hg=profile_mesh('FreeFloatHandguard',[(8.10,.37),(8.10,5.82),(8.83,6.17),(19.3,6.17),(20.05,5.25),(20.05,1.2),(19.25,.34)],5.35)
    difference(hg,profile_mesh('BarrelTunnel',[(7.8,1.65),(7.8,4.60),(20.2,4.60),(20.2,1.65)],3.2))
    for x,length in ((10.3,2.2),(14.15,3.1),(18.0,1.8)):
        difference(hg,profile_mesh('UpperVent',capsule_points(x,5.48,length,.34),6.0))
    c.finish(hg,'FreeFloatHandguard','GraphiteCeramic',.11)
    c.cylinder('FreeFloatBarrel',(14.1,0,3.08),.64,13.2,'Steel',(1,0,0),24,.04)
    for side in (-1,1):
        profile('RearIvoryHeatShield',[(8.7,4.91),(14.23,4.91),(14.55,4.45),(14.55,2.02),(13.05,2.02),(12.47,1.34),(9.18,1.65),(8.7,2.22)],.34,'IvoryCeramic',.12,side*2.53)
        profile('ForwardIvoryHeatShield',[(14.77,4.91),(18.97,4.91),(19.48,4.32),(19.48,1.89),(18.80,1.39),(15.10,1.39),(14.77,1.95)],.34,'IvoryCeramic',.12,side*2.53)
        c.box('HeatshieldBottomVent',(17.27,side*2.72,1.83),(2.33,.085,.22),'Rubber',.045)
        for x,z in ((9.33,4.21),(13.9,4.21),(18.88,4.03)):
            screw('CeramicFastener',x,side*2.755,z,.19,'Titanium')
    c.box('UnderslungRail',(13.90,0,.28),(9.8,2.2,.45),'Charcoal',.07)
    c.rail_teeth('UnderslungRailTooth',9.3,18.4,0,-.07,11,'Gunmetal',.38,.30)
    c.box('ContinuousTopRail',(6.6,0,6.28),(25.35,1.84,.25),'Charcoal',.04)
    c.rail_teeth('PicatinnyTooth',-5.9,18.9,0,6.53,27,'Gunmetal',.44,.24)
    c.cylinder('BarrelEndShoulder',(20.3,0,3.08),.89,1.24,'Gunmetal',(1,0,0),24,.06)
    c.cylinder('MuzzleMount',(21.1,0,3.08),1.0,.61,'Steel',(1,0,0),24,.035)
    hollow_tube('PortedCompensator',21.35,24,3.08,1.10,.66,port_rows=(-.38,.38))
    # Sculpted modular grip, stronger forward rake, and an enlarged angular guard.
    profile('ErgonomicGrip',[(-9.10,-1.15),(-5.46,-1.15),(-5.25,-3.15),(-7.30,-9.1),(-7.88,-9.44),(-11.35,-8.34),(-11.64,-7.69),(-9.37,-2.85)],3.15,'Charcoal',.20)
    for side in (-1,1):
        profile('GripTexturedInlay',[(-9.41,-3.83),(-6.53,-4.05),(-8.03,-8.62),(-10.84,-7.84)],.16,'BlackGrip',.09,side*1.62)
        profile('GripUpperFacet',[(-8.74,-2.15),(-5.9,-2.25),(-5.77,-3.48),(-8.90,-3.4)],.10,'GraphiteCeramic',.07,side*1.6)
        screw('GripInlayScrew',-9.71,side*1.78,-7.43,.16,'Titanium')
    profile('GripHeel',[(-11.39,-8.01),(-7.39,-9.08),(-7.67,-9.55),(-11.59,-8.52)],3.40,'Gunmetal',.075)
    frame('EnlargedTriggerGuard',[(-5.78,-1.50),(.1,-1.50),(.67,-2.17),(.15,-4.4),(-1.0,-4.74),(-5.35,-4.61),(-6.05,-3.9),(-6.13,-2.4)],
          [(-5.24,-2.05),(-.42,-2.05),(.04,-2.39),(-.39,-3.83),(-1.07,-4.17),(-5.0,-4.06),(-5.47,-3.65),(-5.58,-2.55)],1.55,'Gunmetal',.07)
    profile('MatchTrigger',[(-3.55,-1.81),(-2.92,-1.81),(-3.24,-2.56),(-3.15,-3.26),(-2.70,-3.72),(-3.00,-3.9),(-3.67,-3.25),(-3.81,-2.58)],.52,'Titanium',.05)
    # Straight stick magazine: narrow shell with distinct armored base and windows.
    profile('StraightMagazine',[(.48,-2.88),(4.05,-2.88),(4.16,-12.63),(3.87,-12.90),(.68,-12.90),(.48,-12.63)],2.35,'Magazine',.10)
    profile('MagazineBoot',[(.30,-10.90),(1.20,-10.90),(1.7,-11.62),(4.30,-11.62),(4.30,-12.65),(3.93,-13),(.66,-13),(.30,-12.55)],2.76,'GraphiteCeramic',.09)
    for side in (-1,1):
        c.box('MagazineRearFlute',(.95,side*1.21,-7.40),(.30,.09,7.35),'Gunmetal',.04)
        c.box('MagazineFrontFlute',(3.62,side*1.21,-7.40),(.25,.09,7.35),'Gunmetal',.04)
        for z in (-5.15,-7.49,-9.8):
            window=[(2.82+pz,z+px) for px,pz in capsule_points(0,0,1.65,.49)]
            profile('MagazineWitnessWindow',window,.08,'SmokedPolymer',.025,side*1.23)
        for x in (.92,1.39,3.66):
            c.box('MagazineBootGroove',(x,side*1.40,-12.41),(.13,.055,.62),'Rubber',.025)
    # Restrained mechanical controls on both sides.
    for side in (-1,1):
        c.cylinder('SelectorPivot',(-8.3,side*2.10,.25),.36,.20,'Steel',(0,1,0),16,.04)
        profile('AmbiSelector',[(-8.54,.10),(-8.0,.0),(-7.21,.59),(-7.31,.96),(-7.68,1.04),(-8.54,.51)],.24,'Titanium',.06,side*2.23)
        c.box('ReleaseHousing',(-.68,side*2.12,.14),(2.08,.34,.98),'Rubber',.09)
        profile('MagazineReleaseButton',[(-1.43,.38),(.05,.38),(.15,.07),(-.11,-.15),(-1.43,-.15)],.15,'Titanium',.08,side*2.33)
    # Small side sensor stays within the 6-cm width contract.
    profile('SensorHousing',[(4.43,2.38),(4.43,4.71),(4.83,5.05),(7.78,5.05),(8.12,4.59),(8.12,2.63),(7.71,2.29),(4.83,2.29)],.68,'Charcoal',.11,-2.61)
    for x,r in ((5.32,.39),(6.94,.58)):
        c.cylinder('SensorLensBezel',(x,-2.955,3.58),r,.085,'Titanium',(0,1,0),20,.035)
        c.cylinder('SensorLensDark',(x,-3.005,3.58),r*.78,.018,'SmokedPolymer',(0,1,0),20,.012)
        c.cylinder('SensorLensGlass',(x,-3.016,3.58),r*.53,.010,'VioletLens',(0,1,0),20,.008)
    for x,z in ((4.73,4.72),(7.76,2.64)):
        c.cylinder('SensorHousingScrew',(x,-2.96,z),.12,.035,'Steel',(0,1,0),10,.014)
    # Reflex sight has an open viewing tunnel, faceted hood, glass and adjustment.
    c.box('OpticRailClamp',(-3.88,0,6.85),(4.90,2.38,.50),'Gunmetal',.09)
    profile('OpticSideSupport',[(-6.27,7.01),(-3.63,7.01),(-3.63,8.75),(-4.34,8.88),(-5.64,7.45),(-6.27,7.38)],.26,'Charcoal',.05,-1.12)
    profile('OpticSideSupport',[(-6.27,7.01),(-3.63,7.01),(-3.63,8.75),(-4.34,8.88),(-5.64,7.45),(-6.27,7.38)],.26,'Charcoal',.05,1.12)
    c.box('OpticRoof',(-4.02,0,8.81),(1.32,2.55,.38),'Charcoal',.07)
    c.box('OpticWindow',(-3.74,0,7.97),(.075,1.91,1.45),'SmokedPolymer',.015)
    c.box('OpticCyanFrameL',(-3.68,-.86,7.95),(.09,.055,1.25),'CyanAccent',.008)
    c.box('OpticCyanFrameR',(-3.68,.86,7.95),(.09,.055,1.25),'CyanAccent',.008)
    c.box('OpticEmitter',(-5.37,0,7.17),(.39,.32,.20),'VioletLens',.035)
    for side in (-1,1):
        screw('OpticAdjust',-4.45,side*1.32,7.49,.21,'Gunmetal')
    c.box('FrontBackupSight',(19.08,0,6.69),(.83,1.43,.39),'Gunmetal',.09)
    c.collision_box((-2.25,0,2.8),(43.5,5.95,7.15))
    c.collision_box((-8.7,0,-5.35),(5.6,3.5,8.55))
    c.collision_box((2.30,0,-7.97),(4.1,2.8,10.06))
    c.socket('MuzzleSocket',(24,0,3.08))
    c.socket('LaserSightSocket',(7.95,-2.97,3.58))
    c.socket('ShellEjectionSocket',(1.0,2.50,3.76),(0,0,0))
    verify_parts()
    return c.end((48,6,22),.15)


def previews(standard_obj,premium_obj):
    camera=c.setup_studio((1440,960))
    for light in bpy.data.lights:
        light.energy *= .35
    camera.data.type='ORTHO'
    camera.data.ortho_scale=.59
    scene=bpy.context.scene
    scene.render.film_transparent=False
    scene.render.image_settings.color_mode='RGBA'
    for obj,label in ((standard_obj,'Standard'),(premium_obj,'Premium')):
        c.render_view(camera,c.OUT/'Previews'/f'SMG_{label}_Left.png',(0,-95,0),(0,0,-2),(obj,))
        c.render_view(camera,c.OUT/'Previews'/f'SMG_{label}_FrontThreeQuarter.png',(66,-95,40),(0,0,-2),(obj,))
        c.render_view(camera,c.OUT/'Previews'/f'SMG_{label}_TopGame.png',(24,-58,95),(0,0,-2),(obj,))
    standard_obj.location=c.ue((0,0,13.5))
    premium_obj.location=c.ue((0,0,-13.5))
    camera.data.ortho_scale=.64
    scene.render.resolution_x=1600
    scene.render.resolution_y=1600
    c.render_view(camera,c.OUT/'Previews'/'SMG_PairComparison.png',(0,-100,3),(0,0,-2),(standard_obj,premium_obj))
    standard_obj.location=(0,0,0)
    premium_obj.location=(0,0,0)
    for obj,label in ((standard_obj,'Standard'),(premium_obj,'Premium')):
        collection=bpy.data.collections.new('SMG_'+label)
        scene.collection.children.link(collection)
        for old_collection in list(obj.users_collection):
            old_collection.objects.unlink(obj)
        collection.objects.link(obj)
    standard_obj.hide_render=True
    standard_obj.hide_set(True)
    premium_obj.hide_render=False
    camera.data.ortho_scale=.59
    camera.location=c.ue((66,-95,40))
    camera.rotation_euler=(c.ue((0,0,-2))-camera.location).to_track_quat('-Z','Y').to_euler()
    scene.render.resolution_x=1440
    scene.render.resolution_y=960
    bpy.ops.object.select_all(action='DESELECT')
    premium_obj.select_set(True)
    bpy.context.view_layer.objects.active=premium_obj
    for screen in bpy.data.screens:
        for area in screen.areas:
            if area.type=='VIEW_3D':
                area.spaces.active.region_3d.view_distance=.65
                area.spaces.active.region_3d.view_location=c.ue((0,0,-2))
                area.spaces.active.region_3d.view_rotation=camera.rotation_euler.to_quaternion()
                area.spaces.active.shading.type='MATERIAL'
    bpy.ops.wm.save_as_mainfile(filepath=str(c.OUT/'TunaWeaponCollection_SMG.blend'))


def validate_exports():
    for entry in c.entries:
        bpy.ops.wm.read_factory_settings(use_empty=True)
        bpy.ops.import_scene.fbx(filepath=str(c.OUT/'Models'/(entry['name']+'.fbx')))
        obj=bpy.data.objects[entry['name']]
        assert len(obj.data.materials)==len(entry['materials'])
        assert {material.name for material in obj.data.materials}==set(entry['materials'])
        assert obj.data.uv_layers.active
        assert max(abs(v) for v in obj.location)<1e-7
        assert max(abs(v-1) for v in obj.scale)<1e-6
        assert max(abs(v) for v in obj.rotation_euler)<1e-6
        bounds=c._bounds_ue_cm(obj)
        size=[bounds[i+3]-bounds[i] for i in range(3)]
        assert max(abs(a-b) for a,b in zip(size,entry['size_cm']))<.001
        colliders=[mesh for mesh in bpy.context.scene.objects if mesh.name.startswith('UCX_'+entry['name']+'_')]
        assert len(colliders)==3
        assert all(len(collider.data.vertices)==8 for collider in colliders)
        obj.data.calc_loop_triangles()
        assert len(obj.data.loop_triangles)==entry['triangles']
        assert all(triangle.area>1e-13 for triangle in obj.data.loop_triangles)
        print('SMG_FBX_ROUNDTRIP_OK',entry['name'],'cm',size,'tris',entry['triangles'],
              'materials',len(obj.data.materials),'UCX',len(colliders),flush=True)


if __name__=='__main__':
    c.init('SMG')
    bpy.context.preferences.filepaths.save_version=0
    standard_obj=standard()
    premium_obj=premium()
    c.save_family()
    previews(standard_obj,premium_obj)
    validate_exports()
