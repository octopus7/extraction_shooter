"""Modular industrial room shells and a ladder access kit, authored in cm.

Build: Blender --background --factory-startup --python build_architecture.py
Read-only FBX round trip: same command followed by -- --verify
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
from common import begin, box, cylinder, ring, pipe, surface, collision, end

REFERENCE = 'References/Basement_Reference.png; References/ControlRoom_Reference.png'
ATTIC_REFERENCE = 'References/ControlRoom_Attic_Reference.png'


def wall(width):
    begin('Wall' + str(width), (width, 20, 300), REFERENCE)
    c.current['module_grid_cm'] = width
    # Three broad precast courses. Geometry stays within the repeatable 20 cm shell.
    for row in range(3):
        box('concrete_course_' + str(row), (0, 0, row * 100 + 50),
            (width, 19.8, 100), 'Concrete', .22)
    for facing in (-1, 1):
        y = facing * 9.94
        for z in (100, 200):
            box('course_reveal', (0, y, z), (width - .8, .12, .5), 'Iron', .025)
        for x in (-width / 2 + .55, width / 2 - .55):
            box('module_joint', (x, y, 150), (.45, .12, 299), 'Iron', .02)
        box('base_channel', (0, facing * 9.65, 5), (width, .7, 10), 'Iron', .15)
        for x in (-width * .34, width * .34):
            for z in (27, 127, 227):
                cylinder('precast_anchor', (x, facing * 9.96, z), .68, .08,
                         'Iron', axis=(0, 1, 0), vertices=12, bevel=.025)
    collision((0, 0, 150), (width, 20, 300))
    end()


def wall_corner():
    begin('WallCorner', (20, 20, 300), REFERENCE)
    c.current['module_grid_cm'] = 20
    box('concrete_column', (0, 0, 150), (19.8, 19.8, 300), 'Concrete', .2)
    for x in (-9.65, 9.65):
        box('edge_angle_x', (x, 0, 150), (.7, 20, 300), 'Iron', .14)
    for y in (-9.65, 9.65):
        box('edge_angle_y', (0, y, 150), (20, .7, 300), 'Iron', .14)
    for z in (3, 297):
        box('end_band', (0, 0, z), (20, 20, 6), 'Steel', .2)
    collision((0, 0, 150), (20, 20, 300))
    end()


def doorway():
    begin('Doorway200', (200, 20, 300), REFERENCE)
    c.current['opening_cm'] = {'width': 120, 'height': 230, 'center_x': 0,
                               'bounds': [-60, -10, 0, 60, 10, 230]}
    for x in (-80, 80):
        box('concrete_jamb', (x, 0, 115), (40, 20, 230), 'Concrete', .2)
        collision((x, 0, 115), (40, 20, 230))
    box('concrete_lintel', (0, 0, 265), (200, 20, 70), 'Concrete', .2)
    collision((0, 0, 265), (200, 20, 70))
    for facing in (-1, 1):
        for x in (-62, 62):
            box('steel_jamb_trim', (x, facing * 9.8, 115), (4, .4, 230), 'Iron', .1)
        box('steel_lintel_trim', (0, facing * 9.8, 232), (128, .4, 4), 'Iron', .1)
        for x in (-80, 80):
            box('toe_armor', (x, facing * 9.8, 7), (39.6, .4, 14), 'Iron', .1)
            box('caution_jamb_marker', (x * .795, facing * 9.88, 52),
                (5, .2, 26), 'Hazard', .04)
        for x in (-90, 90):
            cylinder('lintel_anchor', (x, facing * 9.9, 271), .7, .2,
                     'Iron', axis=(0, 1, 0), vertices=12, bevel=.025)
    end()


def floor_panel():
    begin('Floor200', (200, 200, 20), REFERENCE)
    c.current['module_grid_cm'] = 200
    c.current['walkable_top_z_cm'] = 20
    box('structural_slab', (0, 0, 9), (200, 200, 18), 'Concrete', .25)
    box('floor_finish', (0, 0, 19), (200, 200, 2), 'Concrete', .08)
    for side in (-1, 1):
        box('x_expansion_seam', (side * 99.7, 0, 19.94), (.45, 199.4, .12), 'Iron', .025)
        box('y_expansion_seam', (0, side * 99.7, 19.94), (199.4, .45, .12), 'Iron', .025)
    collision((0, 0, 10), (200, 200, 20))
    end()


def threshold():
    begin('Threshold120', (120, 24, 4), REFERENCE)
    box('threshold_base', (0, 0, 2), (120, 24, 4), 'Steel', .35)
    for y in (-7.5, -2.5, 2.5, 7.5):
        box('recessed_grip_line', (0, y, 3.88), (112, .8, .2), 'Iron', .08)
    for x in (-56, 56):
        for y in (-8, 8):
            cylinder('fixing_screw', (x, y, 3.93), .6, .1, 'Iron', vertices=12, bevel=.02)
    collision((0, 0, 2), (120, 24, 4))
    end()


def ladder():
    begin('Ladder300', (75, 30, 330), REFERENCE)
    c.current['climb_height_cm'] = 300
    c.current['rung_count'] = 11
    c.current['rung_spacing_cm'] = 27
    c.current['use_note'] = 'Visual ladder with segmented collision; climb interaction is authored separately.'
    for x in (-34.5, 34.5):
        # Straight rails retain a predictable climb plane; curved grab handles rise 30 cm above the landing.
        cylinder('vertical_rail', (x, -10, 150), 3, 300, 'Steel', vertices=20, bevel=.15)
        pipe('upper_grab_handle', [(x, -10, 298), (x, -10, 316),
                                  (x, -6, 325), (x, 3, 327), (x, 12, 327)], 3, 'Steel')
        cylinder('warning_grip_sleeve', (x, -10, 307), 3.02, 15, 'Hazard', vertices=20, bevel=.08)
        for z in (15, 145, 275):
            box('wall_standoff', (x, 1, z), (5, 22, 4), 'Iron', .35)
            box('wall_anchor_plate', (x, 13.5, z), (6, 3, 14), 'Iron', .35)
            for dz in (-4, 4):
                cylinder('anchor_bolt', (x, 11.8, z + dz), .9, .8,
                         'Steel', axis=(0, 1, 0), vertices=6, bevel=.04)
        collision((x, -10, 150), (6, 6, 300))
        collision((x, -9, 313), (6, 8, 30))
        collision((x, 4, 327), (6, 22, 6))
    for index in range(11):
        z = 15 + index * 27
        cylinder('step_' + str(index), (0, -12, z), 2.5, 64,
                 'Steel', axis=(1, 0, 0), vertices=16, bevel=.15)
        box('step_grip', (0, -12, z + 1.1), (53, 6, 2), 'Rubber', .25)
        collision((0, -12, z), (64, 6, 5))
    end()


def hatch_landing():
    begin('HatchLanding200', (200, 200, 20), REFERENCE)
    c.current['opening_cm'] = {'width': 90, 'depth': 90, 'center': [0, 0],
                               'bounds': [-45, -45, 0, 45, 45, 20]}
    c.current['walkable_top_z_cm'] = 20
    pieces = [((-72.5, 0, 10), (55, 200, 20)),
              ((72.5, 0, 10), (55, 200, 20)),
              ((0, -72.5, 10), (90, 55, 20)),
              ((0, 72.5, 10), (90, 55, 20))]
    for index, (loc, size) in enumerate(pieces):
        box('open_platform_' + str(index), (loc[0], loc[1], 9.8),
            (size[0], size[1], 19.6), 'Concrete', .16)
        collision(loc, size)
    for x in (-47, 47):
        box('inner_edge_armor_x', (x, 0, 18.7), (4, 98, 2.6), 'Iron', .15)
        box('warning_stripe_x', (x * 1.13, 0, 19.88), (7, 105, .24), 'Hazard', .025)
    for y in (-47, 47):
        box('inner_edge_armor_y', (0, y, 18.7), (90, 4, 2.6), 'Iron', .15)
        box('warning_stripe_y', (0, y * 1.13, 19.88), (98, 7, .24), 'Hazard', .025)
    for x in (-78, 78):
        for y in (-78, 78):
            cylinder('deck_bolt', (x, y, 19.7), 1.1, .6, 'Steel', vertices=6, bevel=.04)
    end()


def guardrail():
    begin('Guardrail200', (200, 12, 110), REFERENCE)
    for x in (-95, 95):
        box('post_baseplate', (x, 0, 1), (10, 12, 2), 'Iron', .25)
        box('square_post', (x, 0, 54), (6, 6, 106), 'Iron', .6)
        for y in (-4.2, 4.2):
            cylinder('post_anchor', (x, y, 2.3), .65, .6, 'Steel', vertices=6, bevel=.04)
        collision((x, 0, 55), (10, 12, 110))
    box('caution_top_rail', (0, 0, 107), (200, 8, 6), 'Hazard', .7)
    box('mid_rail', (0, 0, 58), (188, 5, 4), 'Steel', .55)
    box('toe_board', (0, 0, 12), (188, 3, 16), 'Iron', .35)
    collision((0, 0, 107), (200, 8, 6))
    collision((0, 0, 58), (188, 5, 4))
    collision((0, 0, 12), (188, 3, 16))
    end()


def wall_lamp():
    begin('WallLamp', (80, 18, 22), REFERENCE)
    box('wall_mount', (0, 7.5, 11), (80, 3, 22), 'Iron', .6)
    box('weatherproof_housing', (0, 0, 11), (76, 14, 18), 'Steel', 1.7)
    box('recessed_black_seal', (0, -7.3, 11), (69, 1.2, 13), 'Rubber', 1)
    box('warm_glass_diffuser', (0, -8.3, 11), (65, 1.4, 10), 'Amber', .7)
    for z in (5.2, 16.8):
        box('cage_horizontal', (0, -8.55, z), (68, .9, 1.2), 'Iron', .25)
    for x in (-33.2, -11, 11, 33.2):
        box('cage_vertical', (x, -8.5, 11), (1, 1, 12), 'Iron', .25)
    collision((0, 0, 11), (80, 18, 22))
    end()


def wall_vent():
    begin('WallVent', (100, 14, 60), REFERENCE)
    box('mounting_flange', (0, 6, 30), (100, 2, 60), 'Steel', .45)
    for x in (-46, 46):
        box('vent_side_frame', (x, -1, 30), (4, 12, 56), 'Iron', .55)
    for z in (4, 56):
        box('vent_horizontal_frame', (0, -1, z), (88, 12, 4), 'Iron', .55)
    box('black_inner_shadow', (0, 4.5, 30), (88, .6, 48), 'Rubber', .25)
    for index in range(8):
        z = 9 + index * 6
        # A folded louver profile, with a small undercut instead of a painted line.
        verts = [(-44, -6.8, z - 1), (44, -6.8, z - 1), (44, -2.2, z + 1.8), (-44, -2.2, z + 1.8),
                 (-44, -6.8, z - 1.8), (44, -6.8, z - 1.8), (44, -2.2, z + 1), (-44, -2.2, z + 1)]
        faces = [(0, 1, 2, 3), (7, 6, 5, 4), (0, 4, 5, 1), (3, 2, 6, 7), (0, 3, 7, 4), (1, 5, 6, 2)]
        surface('folded_louver', verts, faces, 'Steel', .08, True)
    for x in (-47, 47):
        for z in (3, 57):
            cylinder('vent_screw', (x, 4.8, z), .75, .3, 'Iron', axis=(0, 1, 0), vertices=12, bevel=.03)
    collision((0, 0, 30), (100, 14, 60))
    end()


def extruded_xz(name, outline, depth, material='Wood'):
    """Closed flat extrusion for a timber plank, bracket or sloping gable beam."""
    count = len(outline)
    verts = [(x, -depth / 2, z) for x, z in outline]
    verts += [(x, depth / 2, z) for x, z in outline]
    faces = [tuple(reversed(range(count))), tuple(range(count, count * 2))]
    faces += [(i, (i + 1) % count, (i + 1) % count + count, i + count) for i in range(count)]
    return surface(name, verts, faces, material, .12, False)


def wood_wall():
    begin('WoodWall200', (200, 20, 220), ATTIC_REFERENCE)
    c.current['module_grid_cm'] = 200
    # Recess the infill behind the frame on every exposed boundary. Shared
    # front-facing polygons would flicker or render black after FBX import.
    box('timber_backing', (0, 0, 110), (199.4, 11.4, 219.4), 'Wood', .12)
    for facing in (-1, 1):
        for index in range(10):
            box('vertical_tongue_and_groove', (-90 + index * 20, facing * 7.4, 110),
                (19.65, 3.2, 195.4), 'Wood', .2)
    for x in (-94, 0, 94):
        box('exposed_upright', (x, 0, 110), (12 if x else 10, 20, 220), 'Wood', .65)
    for z in (6, 214):
        for x in (-46.5, 46.5):
            box('exposed_crossbeam', (x, 0, z), (82.8, 20, 12), 'Wood', .65)
    for side in (-1, 1):
        xz = [(side * 87, 163), (side * 87, 176), (side * 48, 208), (side * 34, 208)]
        extruded_xz('timber_knee_brace', xz, 18.8)
        for facing in (-1, 1):
            for z in (27, 195):
                cylinder('blackened_frame_pin', (side * 94, facing * 9.9, z), .6, .2,
                         'Iron', axis=(0, 1, 0), vertices=12, bevel=.025)
    collision((0, 0, 110), (200, 20, 220))
    end()


def wood_gable():
    begin('WoodGable600', (600, 20, 140), ATTIC_REFERENCE)
    c.current['profile_cm'] = [[-300, 0], [0, 140], [300, 0]]
    c.current['collision_profile'] = 'Twenty-four 25 cm wide stepped box envelopes; at most 11.667 cm above the timber slope.'
    for index in range(24):
        grid_x0 = -300 + index * 25
        grid_x1 = grid_x0 + 25
        # Infill stops below the sloping timber cap. Matching the exact slope
        # would stack its narrow top face on the cap and create black seams.
        x0 = max(-298, grid_x0)
        x1 = min(298, grid_x1)
        h0 = 140 * (1 - abs(x0) / 300)
        h1 = 140 * (1 - abs(x1) / 300)
        outline = [(x0, .3), (x1, .3), (x1, h1 - .4), (x0, h0 - .4)]
        extruded_xz('gable_board_' + str(index), outline, 14)
        height = max(140 * (1 - abs(grid_x0) / 300), 140 * (1 - abs(grid_x1) / 300))
        collision(((grid_x0 + grid_x1) / 2, 0, height / 2), (25, 20, height))
    # Broad timber members sit inside the actual triangle, so modular width and ridge remain exact.
    extruded_xz('lower_tie_beam', [(-300, 0), (300, 0), (274.286, 12), (-274.286, 12)], 19.4)
    extruded_xz('left_sloping_beam', [(-300, 0), (-274.286, 0), (0, 128), (0, 140)], 20)
    extruded_xz('right_sloping_beam', [(0, 128), (274.286, 0), (300, 0), (0, 140)], 20)
    extruded_xz('central_king_post', [(-6, 12.2), (6, 12.2), (6, 134.1), (0, 136.9), (-6, 134.1)], 18.8)
    for x in (-150, 150):
        height = 140 * (1 - (abs(x) + 4) / 300)
        box('short_vertical_stud', (x, 0, (height + 12.2) / 2), (8, 18, height - 12.2), 'Wood', .45)
    end()


def wood_floor():
    begin('WoodFloor200', (200, 200, 20), ATTIC_REFERENCE)
    c.current['module_grid_cm'] = 200
    c.current['walkable_top_z_cm'] = 20
    for x in (-94, 0, 94):
        box('floor_joist', (x, 0, 8), (12, 200, 16), 'Wood', .35)
    for y in (-94, 94):
        for x in (-47, 47):
            box('end_joist', (x, y, 8), (81.8, 12, 16), 'Wood', .35)
    for index in range(10):
        y = -90 + index * 20
        box('broad_floorboard', (0, y, 17.95), (200, 19.7, 3.9), 'Wood', .18)
        for x in (-85, 85):
            cylinder('floorboard_nail', (x, y, 19.94), .28, .12, 'Iron', vertices=10, bevel=.015)
    collision((0, 0, 10), (200, 200, 20))
    end()


def wood_hatch():
    begin('WoodHatchLanding200', (200, 200, 20), ATTIC_REFERENCE)
    c.current['opening_cm'] = {'width': 90, 'depth': 90, 'center': [0, 0],
                               'bounds': [-45, -45, 0, 45, 45, 20]}
    c.current['walkable_top_z_cm'] = 20
    for x in (-72.5, 72.5):
        box('side_support', (x, 0, 8), (55, 200, 16), 'Wood', .3)
        for index in range(10):
            # Reserve the six-centimetre opening trim rather than burying a
            # second top surface underneath its visible top face.
            board_x = math.copysign(75.5, x)
            box('side_floorboard', (board_x, -90 + index * 20, 17.9), (49, 19.7, 3.8), 'Wood', .16)
        collision((x, 0, 10), (55, 200, 20))
    for y in (-72.5, 72.5):
        box('end_support', (0, y, 8), (90, 55, 16), 'Wood', .3)
        for offset in (-16.333, 0, 16.333):
            board_y = math.copysign(75.5, y) + offset
            box('end_floorboard', (0, board_y, 17.9), (102, 16.033, 3.8), 'Wood', .16)
        collision((0, y, 10), (90, 55, 20))
    for x in (-48, 48):
        box('inner_timber_frame_x', (x, 0, 17.5), (6, 102, 4.6), 'Wood', .25)
    for y in (-48, 48):
        box('inner_timber_frame_y', (0, y, 17.5), (90, 6, 4.6), 'Wood', .25)
    for x in (-51, 51):
        for y in (-51, 51):
            box('corner_iron_strap', (x, y, 19.875), (9, 9, .15), 'Iron', .06)
            cylinder('strap_pin', (x, y, 19.97), .62, .06, 'Steel', vertices=12, bevel=.012)
    end()


def attic_lamp():
    begin('AtticLamp', (35, 35, 50), ATTIC_REFERENCE)
    c.current['use_note'] = 'Warm timber-framed lantern, floor-centred origin; place on shelves or attach to a wall support.'
    box('wood_lantern_base', (0, 0, 2.5), (35, 35, 5), 'Wood', 1.2)
    box('lower_iron_ring', (0, 0, 6), (32, 32, 2), 'Iron', .45)
    box('amber_frosted_glass', (0, 0, 21.5), (25, 25, 29), 'Amber', .9)
    for x in (-14.5, 14.5):
        for y in (-14.5, 14.5):
            box('wood_glass_mullion', (x, y, 21), (4, 4, 30), 'Wood', .6)
    box('upper_iron_ring', (0, 0, 37), (33, 33, 3), 'Iron', .5)
    verts = [(-17.5, -17.5, 38), (17.5, -17.5, 38), (17.5, 17.5, 38), (-17.5, 17.5, 38),
             (-4, -4, 44), (4, -4, 44), (4, 4, 44), (-4, 4, 44)]
    surface('sloped_lantern_roof', verts,
            [(3, 2, 1, 0), (4, 5, 6, 7), (0, 1, 5, 4), (1, 2, 6, 5), (2, 3, 7, 6), (3, 0, 4, 7)], 'Iron', .35, True)
    ring('hanging_eye', (0, 0, 45), 4.5, .5, 'Iron', axis=(0, 1, 0))
    collision((0, 0, 22), (35, 35, 44))
    collision((0, 0, 47), (10, 3, 6))
    end()


def wood_window_wall():
    begin('WoodWindowWall200', (200, 20, 220), ATTIC_REFERENCE)
    c.current['module_grid_cm'] = 200
    c.current['opening_cm'] = {'width': 95, 'height': 95, 'sill_z': 85,
                               'bounds': [-47.5, -10, 85, 47.5, 10, 180],
                               'mullion_width': 3, 'mullion_center_z': 132.5}
    c.current['use_note'] = 'Open timber window with a thin cross mullion. No glazing or opaque surface spans the opening.'
    # Four independent wall pieces make a real through-opening in both mesh and collision.
    for side in (-1, 1):
        box('side_wall_core', (side * 73.75, 0, 110), (51.9, 11.4, 219.4), 'Wood', .12)
        collision((side * 73.75, 0, 110), (52.5, 20, 220))
        for facing in (-1, 1):
            for offset in (-17.5, 0, 17.5):
                box('side_wall_board', (side * 73.75 + offset, facing * 7.4, 110),
                    (17.2, 3.2, 219.4), 'Wood', .18)
    for z, height in ((42.5, 85), (200, 40)):
        box('above_below_window_core', (0, 0, z), (94.4, 11.4, height - .6), 'Wood', .12)
        collision((0, 0, z), (95, 20, height))
        for facing in (-1, 1):
            for index in range(5):
                box('short_wall_board', (-38 + index * 19, facing * 7.4, z),
                    (18.7, 3.2, height - .6), 'Wood', .18)
    for x in (-94, 94):
        box('outer_timber_post', (x, 0, 110), (12, 20, 220), 'Wood', .65)
    for z in (6, 214):
        box('outer_timber_crossbeam', (0, 0, z), (175.8, 20, 12), 'Wood', .65)
    for x in (-51.5, 51.5):
        box('window_jamb', (x, 0, 132.5), (8, 20, 95), 'Wood', .4)
    for z in (81, 184):
        box('window_sill_lintel', (0, 0, z), (111, 20, 8), 'Wood', .4)
    box('window_vertical_mullion', (0, 0, 132.5), (3, 12, 95), 'Wood', .22)
    for x in (-24.5, 24.5):
        box('window_horizontal_mullion', (x, 0, 132.5), (46, 12, 3), 'Wood', .22)
    collision((0, 0, 132.5), (3, 12, 95))
    collision((0, 0, 132.5), (95, 12, 3))
    end()


def wood_roof():
    begin('WoodRoof600', (600, 600, 140), ATTIC_REFERENCE)
    c.current['profile_cm'] = [[-300, 0], [0, 140], [300, 0]]
    c.current['matching_gable'] = 'SM_FacilityRooms_WoodGable600'
    c.current['collision_profile'] = 'Twenty-four 25 cm wide box bands follow the 3 cm roof skin; maximum envelope thickness 14.667 cm. Exposed decorative rafters are not separately blocked.'
    for index in range(24):
        x0 = -300 + index * 25
        x1 = x0 + 25
        h0 = 140 * (1 - abs(x0) / 300)
        h1 = 140 * (1 - abs(x1) / 300)
        low0, low1 = max(0, h0 - 3), max(0, h1 - 3)
        outline = [(x0, low0), (x1, low1)]
        if h1 > low1:
            outline.append((x1, h1))
        if h0 > low0:
            outline.append((x0, h0))
        extruded_xz('roof_board_' + str(index), outline, 600)
        lower, upper = min(low0, low1), max(h0, h1)
        collision(((x0 + x1) / 2, 0, (lower + upper) / 2), (25, 600, upper - lower))
    for y in (-290, -200, -100, 0, 100, 200, 290):
        left = extruded_xz('exposed_left_rafter', [(-270, 1), (0, 127), (0, 137), (-270, 11)], 10)
        left.location.y = y / 100
        right = extruded_xz('exposed_right_rafter', [(0, 127), (270, 1), (270, 11), (0, 137)], 10)
        right.location.y = y / 100
    box('internal_ridge_beam', (0, 0, 127), (10, 600, 12), 'Wood', .35)
    for x in (-280, 280):
        box('internal_eave_beam', (x, 0, 3), (12, 600, 6), 'Wood', .35)
    end()


def refresh_catalog():
    # A 6 m gable needs a wider slot than the common four-column prop grid.
    placements = [(-6, -3), (-2, -3), (2, -3), (6, -3),
                  (-6, 1), (-2, 1), (2, 1), (6, 1),
                  (-6, 5), (-2, 5), (2, 5), (6, 5),
                  (-3, 9), (3, 9), (6, 9), (6, 12), (-3, 12), (-3, 17)]
    for obj, (x, y) in zip(c.objects, placements):
        obj.location = (x, y, 0)
    scene = bpy.context.scene
    target = Vector((0, 7, .5))
    cam = scene.camera
    cam.location = target + Vector((8, -16, 20))
    cam.rotation_euler = (target - cam.location).to_track_quat('-Z', 'Y').to_euler()
    cam.data.ortho_scale = 27
    scene.render.resolution_x = 2000
    scene.render.resolution_y = 2200
    for obj in scene.objects:
        if obj.type == 'LIGHT':
            obj.rotation_euler = (target - obj.location).to_track_quat('-Z', 'Y').to_euler()
    bpy.ops.wm.save_as_mainfile(filepath=str(c.OUT / 'FacilityRooms_Architecture.blend'))
    scene.render.filepath = str(c.OUT / 'Previews/Architecture.png')
    bpy.ops.render.render(write_still=True)


def render_window_detail():
    """Render the exported FBX, with a distant backdrop visible through the four open panes."""
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.fbx(filepath=str(c.OUT / 'Models/SM_FacilityRooms_WoodWindowWall200.fbx'), use_anim=False)
    for obj in bpy.context.scene.objects:
        if obj.name.startswith('UCX_'):
            obj.hide_render = True
    atlas = bpy.data.images.load(str(c.OUT / 'Textures/T_FacilityRooms_Atlas.png'), check_existing=True)
    for mat in bpy.data.materials:
        if not mat.use_nodes:
            continue
        for node in mat.node_tree.nodes:
            if node.type == 'TEX_IMAGE':
                node.image = atlas
    scene = bpy.context.scene
    scene.render.engine = 'CYCLES'
    scene.cycles.samples = 32
    scene.cycles.use_denoising = True
    scene.render.resolution_x = 1100
    scene.render.resolution_y = 1100
    scene.render.resolution_percentage = 100
    scene.view_settings.view_transform = 'AgX'
    scene.world = bpy.data.worlds.new('WindowReviewWorld')
    scene.world.use_nodes = True
    scene.world.node_tree.nodes['Background'].inputs['Color'].default_value = (.55, .65, .7, 1)
    scene.world.node_tree.nodes['Background'].inputs['Strength'].default_value = .5
    bpy.ops.mesh.primitive_plane_add(size=100, location=(0, 0, -.01))
    floor = bpy.context.object
    floor.name = 'PREVIEW_Only_Floor'
    mat = bpy.data.materials.new('PREVIEW_Only_Floor_Material')
    mat.use_nodes = True
    mat.node_tree.nodes['Principled BSDF'].inputs['Base Color'].default_value = (.28, .27, .24, 1)
    mat.node_tree.nodes['Principled BSDF'].inputs['Roughness'].default_value = .9
    floor.data.materials.append(mat)
    # A preview-only distant colored card makes the physical view-through obvious.
    bpy.ops.mesh.primitive_cube_add(size=1, location=(0, 1.8, 1.5))
    backdrop = bpy.context.object
    backdrop.name = 'PREVIEW_Only_Distant_Backdrop'
    backdrop.dimensions = (4, .02, 3)
    mat = bpy.data.materials.new('PREVIEW_Only_Distant_Backdrop_Material')
    mat.use_nodes = True
    mat.node_tree.nodes['Principled BSDF'].inputs['Base Color'].default_value = (.13, .3, .25, 1)
    mat.node_tree.nodes['Principled BSDF'].inputs['Roughness'].default_value = .9
    backdrop.data.materials.append(mat)
    target = Vector((0, 0, 1.1))
    for name, location, power, size in [('Key', (-3, -4, 5), 650, 4), ('Fill', (3, -2, 3), 400, 3), ('Rim', (0, 2, 4), 700, 3)]:
        data = bpy.data.lights.new(name, 'AREA')
        data.energy = power
        data.size = size
        obj = bpy.data.objects.new(name, data)
        scene.collection.objects.link(obj)
        obj.location = location
        obj.rotation_euler = (target - obj.location).to_track_quat('-Z', 'Y').to_euler()
    data = bpy.data.cameras.new('WindowReviewCamera')
    cam = bpy.data.objects.new('WindowReviewCamera', data)
    scene.collection.objects.link(cam)
    cam.location = (2.9, -5, 2.8)
    cam.rotation_euler = (target - cam.location).to_track_quat('-Z', 'Y').to_euler()
    data.type = 'ORTHO'
    data.ortho_scale = 3.5
    scene.camera = cam
    scene.render.filepath = str(c.OUT / 'Previews/Architecture_WoodWindowWall200.png')
    bpy.ops.render.render(write_still=True)


def build():
    c.init('Architecture')
    wall(200)
    wall(100)
    wall_corner()
    doorway()
    floor_panel()
    threshold()
    ladder()
    hatch_landing()
    guardrail()
    wall_lamp()
    wall_vent()
    wood_wall()
    wood_gable()
    wood_floor()
    wood_hatch()
    attic_lamp()
    wood_window_wall()
    wood_roof()
    c.complete()
    refresh_catalog()
    render_window_detail()


def verify_fbx():
    """Reload exports; inspect actual mesh/UCX geometry and traversal openings."""
    manifest = json.loads((c.OUT / 'Manifests/Architecture.json').read_text(encoding='utf-8'))
    report = {'category': 'Architecture', 'blender': bpy.app.version_string,
              'verification': 'fresh-process FBX reload and open-space probes',
              'assets': [], 'passed': False}
    bpy.ops.wm.open_mainfile(filepath=str(c.OUT / 'FacilityRooms_Architecture.blend'))
    for entry in manifest['assets']:
        obj = bpy.data.objects.get(entry['name'])
        assert obj is not None and obj.type == 'MESH', (entry['name'], 'missing editable source mesh')
        assert len(obj.data.uv_layers) == 2, (entry['name'], 'source UV channels')
        assert max(abs(obj.dimensions[a] * 100 - entry['size_cm'][a]) for a in range(3)) < .05, (entry['name'], 'source dimensions')
    report['source_blend_reload_passed'] = True
    open_probes = {
        'Doorway200': [(-59, 0, 100), (0, 0, 229), (59, 0, 100)],
        'HatchLanding200': [(-44, -44, 10), (0, 0, 10), (44, 44, 10)],
        'WoodHatchLanding200': [(-44, -44, 10), (0, 0, 10), (44, 44, 10)],
        'Ladder300': [(0, -11, 28), (0, -11, 163)],
        'Guardrail200': [(0, 0, 36), (0, 0, 82)],
        'WoodWindowWall200': [(-22.5, 0, 107.5), (22.5, 0, 107.5),
                              (-22.5, 0, 157.5), (22.5, 0, 157.5),
                              (-46, 0, 86), (46, 0, 179)],
        'WoodRoof600': [(0, 0, 100), (-150, 0, 40), (150, 0, 40)],
    }

    def contains(obj, point_cm):
        point = obj.matrix_world.inverted() @ (Vector(point_cm) / 100)
        minimum = [min(v.co[a] for v in obj.data.vertices) for a in range(3)]
        maximum = [max(v.co[a] for v in obj.data.vertices) for a in range(3)]
        return all(minimum[a] - 1e-7 <= point[a] <= maximum[a] + 1e-7 for a in range(3))

    for entry in manifest['assets']:
        path = c.OUT / 'Models' / (entry['name'] + '.fbx')
        digest = hashlib.sha256(path.read_bytes()).hexdigest()
        bpy.ops.wm.read_factory_settings(use_empty=True)
        bpy.ops.import_scene.fbx(filepath=str(path), use_anim=False)
        meshes = [o for o in bpy.context.scene.objects if o.type == 'MESH' and not o.name.startswith('UCX_')]
        colliders = [o for o in bpy.context.scene.objects if o.type == 'MESH' and o.name.startswith('UCX_')]
        assert len(meshes) == 1, (entry['name'], 'render mesh count')
        obj = meshes[0]
        obj.data.calc_loop_triangles()
        assert len(obj.data.loop_triangles) == entry['triangles'], (entry['name'], 'triangle count')
        assert all(t.area > 1e-13 for t in obj.data.loop_triangles), (entry['name'], 'degenerate triangle')
        assert all(math.isfinite(x) for v in obj.data.vertices for x in v.co), (entry['name'], 'nonfinite vertex')
        assert len(obj.data.uv_layers) == 2, (entry['name'], 'UV0 atlas and UV1 lightmap')
        for uv in obj.data.uv_layers:
            assert all(math.isfinite(x) and -1e-6 <= x <= 1.000001 for loop in uv.data for x in loop.uv), (entry['name'], 'UV range')
        assert {m.name for m in obj.data.materials} == set(entry['materials']), (entry['name'], 'material slots')
        assert len(colliders) == entry['collision_boxes'], (entry['name'], 'UCX count')
        assert all(len(o.data.vertices) == 8 for o in colliders), (entry['name'], 'non-box UCX')
        points = [obj.matrix_world @ Vector(corner) for corner in obj.bound_box]
        bounds = [min(v[a] for v in points) * 100 for a in range(3)]
        bounds += [max(v[a] for v in points) * 100 for a in range(3)]
        error = max(abs(actual - expected) for actual, expected in zip(bounds, entry['bounds_cm']))
        assert error < .05, (entry['name'], 'bounds cm', bounds, error)
        short_name = entry['name'].removeprefix('SM_FacilityRooms_')
        probes = open_probes.get(short_name, [])
        for probe in probes:
            assert not any(contains(o, probe) for o in colliders), (entry['name'], 'blocked opening', probe)
        if short_name == 'WoodWindowWall200':
            inv = obj.matrix_world.inverted()
            direction = (inv.to_3x3() @ Vector((0, 1, 0))).normalized()
            for x, _y, z in probes:
                origin = inv @ Vector((x / 100, -.5, z / 100))
                hit, *_ = obj.ray_cast(origin, direction, distance=2)
                assert not hit, (entry['name'], 'opaque render geometry blocks window', x, z)
            for x, z in [(0, 107.5), (22.5, 132.5)]:
                origin = inv @ Vector((x / 100, -.5, z / 100))
                hit, *_ = obj.ray_cast(origin, direction, distance=2)
                assert hit, (entry['name'], 'missing window mullion', x, z)
        assert hashlib.sha256(path.read_bytes()).hexdigest() == digest, 'verification modified FBX'
        report['assets'].append({'name': entry['name'], 'triangles': entry['triangles'],
                                'vertices': len(obj.data.vertices), 'uv_channels': 2,
                                'collision_boxes': len(colliders), 'bounds_cm': bounds,
                                'max_bounds_error_cm': error, 'open_space_probes': probes,
                                'sha256': digest, 'passed': True})
    report['total_triangles'] = sum(a['triangles'] for a in report['assets'])
    report['passed'] = True
    (c.OUT / 'Manifests/Architecture_validation.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    print('FACILITY_ARCHITECTURE_FBX_PASSED ' + str(len(report['assets'])))


if __name__ == '__main__':
    if '--verify' in sys.argv:
        verify_fbx()
    else:
        build()
