"""Reusable warm masonry / oak barrel-vault kit for the second mole control room.

Author in centimetres; build with Blender --background --factory-startup --python.
Pass -- --verify for a read-only source/FBX round-trip with aperture probes.
"""
from pathlib import Path
import hashlib
import json
import math
import sys

import bpy
from mathutils import Vector

sys.path.insert(0, str(Path(__file__).resolve().parent))
import common as c
from common import begin, box, cylinder, ring, surface, collision, end

REF = 'References/User_Reference.png'


def extrude(name, outline, depth=24, mat='Stone', y=0, bevel=.25):
    """Closed XZ outline extruded along Y. No coincident caps or duplicate points."""
    clean = []
    for point in outline:
        if not clean or math.dist(point, clean[-1]) > 1e-6:
            clean.append(point)
    if len(clean) > 2 and math.dist(clean[0], clean[-1]) < 1e-6:
        clean.pop()
    assert len(clean) >= 3, (name, clean)
    verts = [(x, y - depth / 2, z) for x, z in clean]
    verts += [(x, y + depth / 2, z) for x, z in clean]
    n = len(clean)
    faces = [tuple(reversed(range(n))), tuple(range(n, 2 * n))]
    faces += [(i, (i + 1) % n, (i + 1) % n + n, i + n) for i in range(n)]
    return surface(name, verts, faces, mat, bevel, False)


def masonry_rect(name, x0, x1, z0, z1):
    """A recessed mortar core with broad, staggered sandstone faces on both sides."""
    box(name + '_recessed_core', ((x0 + x1) / 2, 0, (z0 + z1) / 2),
        (x1 - x0, 20.8, z1 - z0), 'Stone', .15)
    row = 0
    za = z0
    while za < z1 - .1:
        zb = min(z1, za + 30)
        offset = 25 if row % 2 else 0
        bounds = [x0]
        cursor = math.floor((x0 - offset) / 50) * 50 + offset
        while cursor <= x0 + .1:
            cursor += 50
        while cursor < x1 - .1:
            bounds.append(cursor)
            cursor += 50
        bounds.append(x1)
        for index, (xa, xb) in enumerate(zip(bounds, bounds[1:])):
            for sign in (-1, 1):
                # The core ends at 10.4 cm and the raised stones start at 10.5.
                box(f'{name}_course_{row}_{index}_{sign}',
                    ((xa + xb) / 2, sign * 11.25, (za + zb) / 2),
                    (max(.5, xb - xa - .8), 1.5, max(.5, zb - za - .8)),
                    'Stone', .38)
        row += 1
        za = zb


def stone_wall():
    begin('StoneWall200', (200, 24, 240), REF)
    c.current['module_grid_cm'] = 200
    masonry_rect('warm_sandstone', -100, 100, 0, 240)
    collision((0, 0, 120), (200, 24, 240))
    end()


def stone_window():
    begin('StoneWindowWall200', (200, 24, 240), REF)
    c.current['module_grid_cm'] = 200
    c.current['opening_cm'] = {'width': 100, 'height': 110, 'sill_z': 100,
        'bounds': [-50, -12, 100, 50, 12, 210], 'mullion_width': 3,
        'mullion_center_z': 155}
    c.current['use_note'] = 'Actual open four-pane window, no glass. Four wall UCX boxes; decorative mullions intentionally do not obstruct capsule collision.'
    for xa, xb in [(-100, -56), (56, 100)]:
        masonry_rect('window_stone_side', xa, xb, 0, 240)
    masonry_rect('window_below', -56, 56, 0, 94)
    masonry_rect('window_above', -56, 56, 216, 240)
    for x in (-53, 53):
        box('oak_window_jamb', (x, 0, 155), (6, 24, 110), 'Oak', .55)
    for z in (97, 213):
        box('oak_window_sill_lintel', (0, 0, z), (112, 24, 6), 'Oak', .55)
    box('oak_vertical_mullion', (0, 0, 155), (3, 12, 110), 'Oak', .3)
    for x in (-25.75, 25.75):
        box('oak_horizontal_mullion', (x, 0, 155), (48.5, 12, 3), 'Oak', .3)
    for x in (-75, 75):
        collision((x, 0, 120), (50, 24, 240))
    collision((0, 0, 50), (100, 24, 100))
    collision((0, 0, 225), (100, 24, 30))
    end()


def arch_ring(name, rx_outer, rz_outer, rx_inner, rz_inner, y=0, depth=24,
              z=0, mat='Stone', segments=32, bevel=.2):
    outline = [(math.cos(math.pi * i / segments) * rx_outer,
                z + math.sin(math.pi * i / segments) * rz_outer)
               for i in range(segments + 1)]
    outline += [(math.cos(math.pi * i / segments) * rx_inner,
                 z + math.sin(math.pi * i / segments) * rz_inner)
                for i in range(segments, -1, -1)]
    return extrude(name, outline, depth, mat, y, bevel)


def doorway():
    begin('ArchedDoorway200', (200, 24, 240), REF)
    c.current['opening_cm'] = {'width': 110, 'straight_jamb_height': 165,
        'arch_radius': 55, 'arch_peak_z': 220, 'bounds': [-55, -12, 0, 55, 12, 220]}
    c.current['collision_profile'] = 'Two straight jambs plus twenty-two 5 cm arch envelope bands. Bands cover masonry only, leaving conservative open passage.'
    for xa, xb in [(-100, -73), (73, 100)]:
        masonry_rect('door_outer_wall', xa, xb, 0, 240)
    for side in (-1, 1):
        for row in range(6):
            box('dressed_stone_jamb', (side * 64, 0, (row + .5) * 27.5),
                (18, 24, 27.0), 'Stone', .55)
        # Inset the opening-facing core edge too: equal X=55 side faces were
        # coplanar with the dressed blocks and made black vertical patches.
        box('jamb_recessed_core', (side * 64.2, 0, 82.5), (17.6, 20.8, 165), 'Stone', .1)
    arch_ring('arch_recessed_core', 72.6, 72.6, 55.4, 55.4, z=165, depth=20.8, bevel=0)
    count = 14
    for index in range(count):
        a0 = math.pi * index / count + (.003 if index else 0)
        a1 = math.pi * (index + 1) / count - (.003 if index < count - 1 else 0)
        outline = [(math.cos(a0) * 55, 165 + math.sin(a0) * 55),
                   (math.cos(a0) * 73, 165 + math.sin(a0) * 73),
                   (math.cos(a1) * 73, 165 + math.sin(a1) * 73),
                   (math.cos(a1) * 55, 165 + math.sin(a1) * 55)]
        extrude('individual_arch_keystone', outline, 24, 'Stone', bevel=.35)
    # Stone wedges fill the spandrel above the outer arch without overlapping it.
    for index in range(14):
        a0 = math.pi * index / 14
        a1 = math.pi * (index + 1) / 14
        x0, z0 = math.cos(a0) * 73, 165 + math.sin(a0) * 73
        x1, z1 = math.cos(a1) * 73, 165 + math.sin(a1) * 73
        extrude('stone_spandrel', [(x1, z1 + .08), (x0, z0 + .08),
                (x0, 240), (x1, 240)], 23.8, 'Stone', bevel=.16)
    for x in (-77.5, 77.5):
        collision((x, 0, 120), (45, 24, 240))
    for index in range(22):
        x0, x1 = -55 + index * 5, -50 + index * 5
        nearest = min(abs(x0), abs(x1)) if x0 * x1 > 0 else 0
        lower = 165 + math.sqrt(max(0, 55 ** 2 - nearest ** 2))
        collision(((x0 + x1) / 2, 0, (lower + 240) / 2), (5, 24, 240 - lower))
    end()


def wood_door():
    begin('ArchedWoodDoor', (106, 10, 218), REF)
    c.current['use_note'] = 'Separate closed oak leaf; place in the arch or rotate about an authored gameplay hinge. No opening interaction is implied.'
    c.current['profile_cm'] = {'width': 106, 'straight_height': 165, 'arch_radius': 53, 'height': 218}
    for index in range(8):
        x0, x1 = -53 + index * 13.25, -53 + (index + 1) * 13.25
        top = []
        for j in range(5):
            x = x1 - (x1 - x0) * j / 4
            top.append((x, 165 + math.sqrt(max(0, 53 ** 2 - x ** 2))))
        extrude('arched_oak_plank', [(x0, 0), (x1, 0)] + top, 6, 'Oak', bevel=.32)
    for facing in (-1, 1):
        for z in (26, 134):
            box('oak_door_crossbatten', (0, facing * 4, z), (104, 2, 11), 'Oak', .38)
            for x in (-45, 45):
                cylinder('forged_nail', (x, facing * 4.86, z), .8, .28, 'Iron', axis=(0, 1, 0), vertices=12, bevel=.04)
    for z in (55, 153):
        box('strap_hinge', (-35, -3.25, z), (30, .5, 5), 'Iron', .22)
        for x in (-48, -38, -23):
            cylinder('hinge_pin', (x, -3.6, z), .7, .2, 'Brass', axis=(0, 1, 0), vertices=12, bevel=.04)
    cylinder('brass_handle_backplate', (33, -3.5, 96), 4.6, 1, 'Brass', axis=(0, 1, 0), vertices=24, bevel=.12)
    ring('brass_pull_ring', (33, -4.55, 91), 4.4, .4, 'Brass', axis=(0, 1, 0))
    collision((0, 0, 82.5), (106, 10, 165))
    for index in range(14):
        x0, x1 = -53 + index * 106 / 14, -53 + (index + 1) * 106 / 14
        nearest = min(abs(x0), abs(x1)) if x0 * x1 > 0 else 0
        upper = 165 + math.sqrt(max(0, 53 ** 2 - nearest ** 2))
        collision(((x0 + x1) / 2, 0, (165 + upper) / 2), (x1 - x0, 10, upper - 165))
    end()


def oak_floor():
    begin('OakFloor200', (200, 200, 20), REF)
    c.current['module_grid_cm'] = 200
    c.current['walkable_top_z_cm'] = 20
    box('recessed_oak_subfloor', (0, 0, 8), (200, 200, 16), 'Oak', .2)
    for row in range(8):
        y = -87.5 + row * 25
        seam = -35 if row % 2 else 35
        for xa, xb in [(-100, seam), (seam, 100)]:
            box('aged_oak_floorboard', ((xa + xb) / 2, y, 18),
                (xb - xa - .3, 24.7, 4), 'Oak', .16)
            for x in (xa + 4, xb - 4):
                cylinder('recessed_board_nail', (x, y, 19.94), .3, .08, 'Iron', vertices=10, bevel=.015)
    collision((0, 0, 10), (200, 200, 20))
    end()


def oak_hatch():
    begin('OakHatch200', (200, 200, 20), REF)
    c.current['module_grid_cm'] = 200
    c.current['walkable_top_z_cm'] = 20
    c.current['opening_cm'] = {'width': 90, 'depth': 90, 'center': [0, 0], 'bounds': [-45, -45, 0, 45, 45, 20]}
    for side in (-1, 1):
        box('hatch_side_support', (side * 72.5, 0, 8), (55, 200, 16), 'Oak', .2)
        collision((side * 72.5, 0, 10), (55, 200, 20))
        for row in range(8):
            box('hatch_side_plank', (side * 75, -87.5 + row * 25, 18), (50, 24.7, 4), 'Oak', .16)
        box('hatch_end_support', (0, side * 72.5, 8), (90, 55, 16), 'Oak', .2)
        collision((0, side * 72.5, 10), (90, 55, 20))
        for shift in (-12.5, 12.5):
            box('hatch_end_plank', (0, side * 75 + shift, 18), (99.8, 24.7, 4), 'Oak', .16)
        box('hatch_inner_frame_x', (side * 47.5, 0, 18), (5, 99.8, 4), 'Oak', .25)
        box('hatch_inner_frame_y', (0, side * 47.5, 18), (90, 5, 4), 'Oak', .25)
    end()


def ellipse_height(x, rx, rz):
    return rz * math.sqrt(max(0, 1 - (x / rx) ** 2)) if abs(x) < rx else 0


def stone_vault():
    begin('StoneVault600', (600, 600, 120), REF)
    c.current['profile_cm'] = {'shape': 'half ellipse', 'outer_radius_x': 300, 'outer_radius_z': 120,
        'stone_inner_radius_x': 288, 'stone_inner_radius_z': 106, 'oak_rib_inner_radius_x': 278, 'oak_rib_inner_radius_z': 96}
    c.current['matching_end_cap'] = 'SM_MoleControlV2_VaultEnd600'
    c.current['collision_profile'] = 'Thirty 20 cm wide stepped box envelopes follow the roof and rib underside, keeping the whole room interior clear below eave height.'
    arch_ring('vault_recessed_mortar', 298.8, 118.8, 289, 107, depth=599.8, segments=48, bevel=0)
    sectors = 24
    for row in range(12):
        yc = -275 + row * 50
        for index in range(sectors):
            a0 = math.pi * index / sectors + (.002 if index else 0)
            a1 = math.pi * (index + 1) / sectors - (.002 if index < sectors - 1 else 0)
            # Keep the real crown vertex, even though other mortar seams are inset.
            if index == sectors // 2 - 1:
                a1 = math.pi / 2
            if index == sectors // 2:
                a0 = math.pi / 2
            outline = [(math.cos(a0) * 288, math.sin(a0) * 106),
                       (math.cos(a0) * 300, math.sin(a0) * 120),
                       (math.cos(a1) * 300, math.sin(a1) * 120),
                       (math.cos(a1) * 288, math.sin(a1) * 106)]
            extrude('barrel_vault_stone', outline, 49.4, 'Stone', yc, .32)
    for y in (-282, -140, 0, 140, 282):
        arch_ring('continuous_curved_oak_rib', 287.8, 105.8, 278, 96, y=y, depth=12, mat='Oak', segments=48, bevel=.3)
        for side in (-1, 1):
            box('oak_rib_iron_foot_shoe', (side * 283, y, 7), (8, 12.8, 14), 'Iron', .3)
    # The long ridge is below the stone and terminates cleanly at cross ribs.
    cuts = [-300, -288, -276, -146, -134, -6, 6, 134, 146, 276, 288, 300]
    for ya, yb in zip(cuts, cuts[1:]):
        if yb - ya < 14:
            continue
        box('ridge_oak_purlin', (0, (ya + yb) / 2, 101), (10, yb - ya - .2, 9.2), 'Oak', .3)
    # Exact modular 600 cm depth supplied by the two end ribs/stone edge caps.
    for y in (-299.7, 299.7):
        arch_ring('vault_end_edge', 300, 120, 288, 106, y=y, depth=.6, segments=48, bevel=0)
    for index in range(30):
        xa, xb = -300 + index * 20, -280 + index * 20
        nearest = min(abs(xa), abs(xb)) if xa * xb > 0 else 0
        lower = min(ellipse_height(xa, 278, 96), ellipse_height(xb, 278, 96))
        upper = ellipse_height(nearest, 300, 120)
        collision(((xa + xb) / 2, 0, (lower + upper) / 2), (20, 600, upper - lower))
    end()


def clip_rect(poly, xa, xb, za, zb):
    """Clip convex XZ polygon to a brick rectangle."""
    for axis, bound, greater in [(0, xa, True), (0, xb, False), (1, za, True), (1, zb, False)]:
        output = []
        for start, stop in zip(poly, poly[1:] + poly[:1]):
            inside_a = start[axis] >= bound if greater else start[axis] <= bound
            inside_b = stop[axis] >= bound if greater else stop[axis] <= bound
            if inside_a:
                output.append(start)
            if inside_a != inside_b:
                alpha = (bound - start[axis]) / (stop[axis] - start[axis])
                output.append(tuple(start[i] + alpha * (stop[i] - start[i]) for i in range(2)))
        poly = output
        if len(poly) < 3:
            return []
    return poly


def vault_end():
    begin('VaultEnd600', (600, 24, 120), REF)
    c.current['profile_cm'] = {'shape': 'filled half ellipse', 'outer_radius_x': 300, 'outer_radius_z': 120}
    c.current['matching_roof'] = 'SM_MoleControlV2_StoneVault600'
    outline = [(300 * math.cos(math.pi * i / 64), 120 * math.sin(math.pi * i / 64)) for i in range(65)]
    extrude('arched_end_recessed_core', outline, 20.8, 'Stone', bevel=0)
    for row in range(4):
        shift = 25 if row % 2 else 0
        for column in range(13):
            xa = -325 + column * 50 + shift
            xb = xa + 50
            face = clip_rect(outline, xa + .4, xb - .4, row * 30 + .4, (row + 1) * 30 - .4)
            if len(face) < 3:
                continue
            for side in (-1, 1):
                extrude('arched_end_stone_brick', face, 1.5, 'Stone', y=side * 11.25, bevel=.3)
    for index in range(30):
        xa, xb = -300 + index * 20, -280 + index * 20
        nearest = min(abs(xa), abs(xb)) if xa * xb > 0 else 0
        height = ellipse_height(nearest, 300, 120)
        collision(((xa + xb) / 2, 0, height / 2), (20, 24, height))
    end()


def detail_render(short_name, camera, target, ortho_scale, suffix=''):
    for obj in c.objects:
        obj.hide_render = not obj.name.endswith('_' + short_name)
        if not obj.hide_render:
            obj.location = (0, 0, 0)
    scene = bpy.context.scene
    scene.render.resolution_x = 1400
    scene.render.resolution_y = 1100
    scene.cycles.samples = 40
    target = Vector(target)
    cam = scene.camera
    cam.location = camera
    cam.rotation_euler = (target - cam.location).to_track_quat('-Z', 'Y').to_euler()
    cam.data.ortho_scale = ortho_scale
    for obj in scene.objects:
        if obj.type == 'LIGHT':
            obj.rotation_euler = (target - obj.location).to_track_quat('-Z', 'Y').to_euler()
    if suffix == '_Underside':
        data = bpy.data.lights.new('PREVIEW_UndersideFill', 'POINT')
        data.energy = 180
        data.shadow_soft_size = .8
        obj = bpy.data.objects.new('PREVIEW_UndersideFill', data)
        scene.collection.objects.link(obj)
        obj.location = (0, -.5, .45)
    scene.render.filepath = str(c.OUT / 'Previews' / ('Shell_' + short_name + suffix + '.png'))
    bpy.ops.render.render(write_still=True)


def build():
    c.init('Shell')
    for builder in [stone_wall, stone_window, doorway, wood_door, oak_floor, oak_hatch, stone_vault, vault_end]:
        builder()
    c.complete()
    render_details()


def render_details():
    detail_render('StoneWindowWall200', (2.4, -5, 2.8), (0, 0, 1.2), 3.5)
    detail_render('ArchedDoorway200', (2, -5, 2.8), (0, 0, 1.2), 3.5)
    detail_render('StoneVault600', (6, -9, 2.3), (0, 0, .55), 8.6)
    detail_render('StoneVault600', (3, -8, .65), (0, 1, .8), 8.6, '_Underside')


def verify():
    manifest = json.loads((c.OUT / 'Manifests/Shell.json').read_text(encoding='utf-8'))
    report = {'category': 'Shell', 'blender': bpy.app.version_string,
              'verification': 'fresh-process blend and FBX reload, exact bounds and aperture ray probes', 'assets': [], 'passed': False}
    bpy.ops.wm.open_mainfile(filepath=str(c.OUT / 'MoleControlV2_Shell.blend'))
    for entry in manifest['assets']:
        obj = bpy.data.objects.get(entry['name'])
        assert obj and obj.type == 'MESH', entry['name']
        assert len(obj.data.uv_layers) == 2, entry['name']
        assert max(abs(obj.dimensions[i] * 100 - entry['size_cm'][i]) for i in range(3)) < .05, entry['name']
    report['source_blend_reload_passed'] = True
    probes = {
        'StoneWindowWall200': [(-25, 0, 127), (25, 0, 127), (-25, 0, 182), (25, 0, 182)],
        'ArchedDoorway200': [(-53, 0, 80), (53, 0, 80), (0, 0, 218), (-30, 0, 208), (30, 0, 208)],
        'OakHatch200': [(-44, -44, 10), (0, 0, 10), (44, 44, 10)],
        'StoneVault600': [(0, 0, 80), (-150, 0, 60), (150, 0, 60)]}
    for entry in manifest['assets']:
        path = c.OUT / 'Models' / (entry['name'] + '.fbx')
        digest = hashlib.sha256(path.read_bytes()).hexdigest()
        bpy.ops.wm.read_factory_settings(use_empty=True)
        bpy.ops.import_scene.fbx(filepath=str(path), use_anim=False)
        meshes = [o for o in bpy.context.scene.objects if o.type == 'MESH' and not o.name.startswith('UCX_')]
        colliders = [o for o in bpy.context.scene.objects if o.type == 'MESH' and o.name.startswith('UCX_')]
        assert len(meshes) == 1, entry['name']
        obj = meshes[0]
        obj.data.calc_loop_triangles()
        assert len(obj.data.loop_triangles) == entry['triangles'], entry['name']
        assert all(t.area > 1e-13 for t in obj.data.loop_triangles), entry['name']
        assert len(obj.data.uv_layers) == 2, entry['name']
        assert all(math.isfinite(x) and -1e-6 <= x <= 1.000001 for uv in obj.data.uv_layers for loop in uv.data for x in loop.uv), entry['name']
        assert {m.name for m in obj.data.materials} == set(entry['materials']), entry['name']
        assert len(colliders) == entry['collision_boxes'], entry['name']
        assert all(len(o.data.vertices) == 8 for o in colliders), entry['name']
        points = [obj.matrix_world @ Vector(corner) for corner in obj.bound_box]
        bounds = [min(v[i] for v in points) * 100 for i in range(3)] + [max(v[i] for v in points) * 100 for i in range(3)]
        error = max(abs(actual - expected) for actual, expected in zip(bounds, entry['bounds_cm']))
        assert error < .05, (entry['name'], bounds, error)
        short_name = entry['name'].removeprefix('SM_MoleControlV2_')
        for probe in probes.get(short_name, []):
            point = Vector(probe) / 100
            for collider in colliders:
                local = collider.matrix_world.inverted() @ point
                minimum = [min(v.co[i] for v in collider.data.vertices) for i in range(3)]
                maximum = [max(v.co[i] for v in collider.data.vertices) for i in range(3)]
                assert not all(minimum[i] <= local[i] <= maximum[i] for i in range(3)), (entry['name'], 'blocked UCX aperture', probe)
        if short_name in ('StoneWindowWall200', 'ArchedDoorway200'):
            inv = obj.matrix_world.inverted()
            direction = (inv.to_3x3() @ Vector((0, 1, 0))).normalized()
            for x, _y, z in probes[short_name]:
                origin = inv @ Vector((x / 100, -.5, z / 100))
                hit, *_ = obj.ray_cast(origin, direction, distance=2)
                assert not hit, (entry['name'], 'opaque opening', x, z)
        assert hashlib.sha256(path.read_bytes()).hexdigest() == digest, entry['name']
        report['assets'].append({'name': entry['name'], 'triangles': entry['triangles'],
            'vertices': len(obj.data.vertices), 'collision_boxes': len(colliders), 'uv_channels': 2,
            'bounds_cm': bounds, 'max_bounds_error_cm': error, 'open_space_probes': probes.get(short_name, []),
            'sha256': digest, 'passed': True})
    report['total_triangles'] = sum(a['triangles'] for a in report['assets'])
    report['passed'] = True
    (c.OUT / 'Manifests/Shell_validation.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    print('MOLE_CONTROL_V2_SHELL_VERIFIED ' + str(len(report['assets'])))


if __name__ == '__main__':
    if '--verify' in sys.argv:
        verify()
    elif '--details' in sys.argv or '--underside' in sys.argv:
        bpy.ops.wm.open_mainfile(filepath=str(c.OUT / 'MoleControlV2_Shell.blend'))
        c.objects = [obj for obj in bpy.context.scene.objects if obj.type == 'MESH' and obj.name.startswith('SM_MoleControlV2_')]
        if '--underside' in sys.argv:
            detail_render('StoneVault600', (3, -8, .65), (0, 1, .8), 8.6, '_Underside')
        else:
            render_details()
    else:
        build()
