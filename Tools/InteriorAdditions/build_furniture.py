"""Additive cottage furniture, modeled procedurally in centimeters for UE5.

Run with Blender's --background --python. Uses the image-generated material atlas.
The category manifest records dimensions matched to existing in-game furniture.
"""
import math
import os
import sys

sys.path.insert(0, os.path.dirname(__file__))
from common import *

TAU = math.tau


def rounded_outline(width, depth, radius, z, center=(0, 0), segments=6):
    points = []
    for cx, cy, angle in ((width / 2 - radius, depth / 2 - radius, 0),
                          (-width / 2 + radius, depth / 2 - radius, 90),
                          (-width / 2 + radius, -depth / 2 + radius, 180),
                          (width / 2 - radius, -depth / 2 + radius, 270)):
        for step in range(segments + 1):
            a = math.radians(angle + step * 90 / segments)
            points.append((center[0] + cx + radius * math.cos(a),
                           center[1] + cy + radius * math.sin(a), z))
    points.append(points[0])
    return points


def pillow(name, loc, size, mat='Pink', seam=True, rotate=0):
    """Soft sewn cushion with broad billow and pinched perimeter."""
    verts, faces = [], []
    rings, sides = 12, 40
    def signed(value, power):
        return math.copysign(abs(value) ** power, value)
    for j in range(rings + 1):
        phi = -math.pi / 2 + math.pi * j / rings
        band = max(.0001, math.cos(phi)) ** .45
        for i in range(sides):
            theta = TAU * i / sides
            x = size[0] / 2 * signed(math.cos(theta), .45) * band
            y = size[1] / 2 * signed(math.sin(theta), .45) * band
            z = size[2] / 2 * signed(math.sin(phi), .7)
            x, y = x * math.cos(rotate) - y * math.sin(rotate), x * math.sin(rotate) + y * math.cos(rotate)
            verts.append((loc[0] + x, loc[1] + y, loc[2] + z))
    for j in range(rings):
        for i in range(sides):
            k = j * sides + i
            faces.append((k, j * sides + (i + 1) % sides,
                          (j + 1) * sides + (i + 1) % sides, k + sides))
    surface(name, verts, faces, mat, smooth=True)
    if seam:
        pts = rounded_outline(size[0] * .99, size[1] * .99, min(size[:2]) * .15, 0)
        pts = [(loc[0] + x * math.cos(rotate) - y * math.sin(rotate),
                loc[1] + x * math.sin(rotate) + y * math.cos(rotate), loc[2]) for x, y, z in pts]
        pipe(name + '_welt', pts, .30, 'Ivory')


def turned_leg(name, x, y, height, mat='Oak', radius=3.4):
    cylinder(name + '_shaft', (x, y, height * .48), radius * .68, height * .66, mat, vertices=20, bevel=.45)
    for z, r, h in ((height * .10, radius, height * .16),
                    (height * .31, radius * .88, height * .12),
                    (height * .70, radius, height * .14),
                    (height * .89, radius * 1.12, height * .22)):
        sphere(name + '_turned', (x, y, z), (r * 2, r * 2, h), mat)
    cylinder(name + '_foot', (x, y, 1.5), radius * .85, 3, mat, vertices=20, bevel=.8)


def handle(name, x, y, z, width=10, mat='Brass'):
    pipe(name, [(x-width/2, y+1.2, z), (x-width/2, y-1.2, z),
                (x-width/2+1, y-2, z), (x+width/2-1, y-2, z),
                (x+width/2, y-1.2, z), (x+width/2, y+1.2, z)], .65, mat)
    for dx in [-width/2, width/2]:
        sphere(name + '_boss', (x+dx, y+.2, z), (2.3, 1, 2.3), mat)


def panel(name, loc, size, mat='Pink'):
    box(name + '_frame', loc, size, mat, bevel=1.2)
    box(name + '_inset', (loc[0], loc[1]-.7, loc[2]), (size[0]-5, size[1], size[2]-5), mat, bevel=1.1)


def dining_table():
    begin('DiningTable', (148.605, 131.477, 63.221), reference='SM_KitchenTable; BunkerMap component scale 1; cozy pink gingham dining table')
    for x in [-41, 41]:
        for y in [-65, 65]:
            turned_leg('table_leg', x, y, 70, 'Pink', 5)
    box('oak_apron', (0, 0, 65), (92, 141, 10), 'Oak', 2)
    box('solid_wood_top', (0, 0, 72), (104, 153, 7), 'Oak', 4)
    # Tablecloth drops over the long sides and curves gently around the ends.
    xs = [-57, -55, -52, -45, 0, 45, 52, 55, 57]
    ys = [-81, -78, -76, -70, -35, 0, 35, 70, 76, 78, 81]
    verts, faces = [], []
    for j, y in enumerate(ys):
        for i, x in enumerate(xs):
            drop = max(0, abs(x)-52) * 2.0 + max(0, abs(y)-76) * 1.5
            verts.append((x, y, 76.6 - drop + .25 * math.cos(y*.18)))
            if i and j:
                k = j*len(xs)+i
                faces.append((k-len(xs)-1, k-len(xs), k, k-1))
    surface('draped_gingham_tablecloth', verts, faces, 'Gingham', smooth=True)
    collision((0, 0, 38), (100, 149, 76))
    end()


def stool():
    begin('Stool', (61.344, 55.710, 65.725), reference='SM_Stool; BunkerMap component scale 1; round pink padded kitchen stool')
    for a in [math.pi/4 + i*math.pi/2 for i in range(4)]:
        x, y = 13.5*math.cos(a), 13.5*math.sin(a)
        turned_leg('stool_leg', x, y, 35, 'Oak', 3)
    ring('lower_bracing', (0,0,15), 13.2, 1.0, 'Walnut')
    cylinder('seat_wood', (0,0,35), 20.5, 6, 'Oak', bevel=1.5)
    sphere('plump_round_seat', (0,0,40), (45,45,12), 'Pink')
    ring('seat_piping', (0,0,39), 22, .45, 'Ivory')
    for a in [i*TAU/5 for i in range(5)]:
        sphere('upholstery_button', (8*math.cos(a),8*math.sin(a),45), (1.5,1.5,.8), 'Brass')
    collision((0,0,22), (38,38,44))
    end()


def sofa():
    begin('Sofa', (257.818, 115.852, 127.141), reference='SM_Couch native 211.978/95.252/104.535cm times BunkerMap component scale 1.21625516')
    for x in [-76,76]:
        for y in [-28,28]:
            turned_leg('sofa_bun_foot',x,y,16,'Walnut',4.5)
    box('sofa_base', (0,0,25), (178,73,28), 'Gingham', 7)
    pipe('base_welt', rounded_outline(176,71,7,31), .65, 'Ivory')
    box('upholstered_back', (0,29,56), (178,22,60), 'Gingham', 8)
    for x in [-82,82]:
        box('rolled_arm_support',(x,0,45),(24,79,45),'Gingham',9)
        cylinder('rounded_arm_roll',(x,-1,63),11,75,'Gingham',axis=(0,1,0),vertices=32,bevel=3)
        ring('arm_roll_end_piping',(x,-39,63),9.3,.55,'Ivory',axis=(0,1,0))
        sphere('arm_scroll_button',(x,-40,63),(3.4,1.5,3.4),'Brass')
    for x in [-35,35]:
        pillow('seat_cushion',(x,-7,42),(68,60,14),'Gingham')
        box('tufted_back_cushion',(x,14,66),(67,20,38),'Gingham',9)
        # Tuft buttons and short crossed seam pulls break up the large back cushions.
        for dx in [-19,0,19]:
            for z in [57,73]:
                sphere('covered_tuft_button',(x+dx,3.8,z),(2.0,1.0,2.0),'Ivory')
                for s in [-1,1]:
                    pipe('tuft_pull',[(x+dx-2,4,z+s*2),(x+dx,3.7,z),(x+dx+2,4,z-s*2)],.16,'Pink')
    pillow('left_throw_pillow',(-54,-1,56),(27,30,10),'Floral',rotate=-.22)
    pillow('right_throw_pillow',(55,-2,56),(26,31,10),'Gingham',rotate=.23)
    collision((0,0,35), (181,78,70))
    end()


def coffee_table():
    begin('CoffeeTable', (164.899, 104.887, 44.877), reference='SM_TeaTable native 52.810/33.591/14.372cm times BunkerMap component scale 3.12249993')
    for x in [-39,39]:
        for y in [-24,24]:
            turned_leg('coffee_table_leg',x,y,43,'Ivory',3.4)
    box('coffee_lower_shelf',(0,0,15),(85,53,3),'Oak',2)
    box('coffee_apron',(0,0,39),(90,57,8),'Ivory',2)
    box('coffee_table_bead',(0,0,44),(101,67,3),'Brass',1.5)
    box('coffee_rounded_top',(0,0,46),(102,68,6),'Ivory',5)
    # A small woven runner remains part of the independently placeable table.
    box('table_runner',(0,0,49.2),(31,63,.35),'Lace',.1)
    collision((0,0,24.5),(99,65,49))
    end()


def bed():
    begin('Bed', (188.124, 205.026, 114.638), reference='SM_Agit_F_000; actual BunkerMap bedroom component scale 1; related SM_LunaBed reference')
    for x in [-65,65]:
        for y in [-91,91]:
            turned_leg('bed_foot',x,y,33,'Oak',4.5)
    box('bed_frame',(0,0,29),(143,210,16),'Oak',3)
    box('bed_mattress',(0,0,43),(140,202,21),'Linen',9)
    # Curved wooden headboard: frame, upholstered inset, scalloped crest and finials.
    box('headboard_wood',(0,99,76),(145,9,70),'Oak',3)
    box('headboard_padded_inset',(0,93,80),(128,5,52),'Floral',2)
    for x in [-66,66]:
        cylinder('headboard_post',(x,100,71),4.5,82,'Oak',bevel=1)
        sphere('headboard_finial',(x,100,114),(10,10,10),'Brass')
    crest=[(-66,106),(-52,109),(-37,108),(-20,113),(0,116),(20,113),(37,108),(52,109),(66,106)]
    outline=crest+[(x,z-5) for x,z in reversed(crest)]
    count=len(outline)
    verts=[(x,y,z) for y in [92,97] for x,z in outline]
    faces=[tuple(reversed(range(count))),tuple(range(count,count*2))]
    faces += [(i,(i+1)%count,(i+1)%count+count,i+count) for i in range(count)]
    surface('headboard_scalloped_crest',verts,faces,'Oak',bevel=.6)
    for x in [-34,34]:
        pillow('sleeping_pillow',(x,66,58),(61,42,13),'Linen',rotate=x*.0015)
    # Organic sewn duvet uses a grid draped down both sides and the foot.
    nx, ny = 24, 30
    verts, faces = [], []
    for j in range(ny+1):
        y = -111 + j*160/ny
        for i in range(nx+1):
            x = -73 + i*146/nx
            side = max(0,abs(x)-62)
            foot = max(0,-y-92)
            z = 56 - 1.8*side - 1.25*foot + 1.5*math.sin(y*.10+x*.06) + .7*math.cos(x*.2)
            verts.append((x,y,z))
            if i and j:
                k=j*(nx+1)+i
                faces.append((k-nx-2,k-nx-1,k,k-1))
    surface('billowing_duvet',verts,faces,'Gingham',smooth=True)
    pipe('duvet_foot_binding',[(x,-111,56-1.8*max(0,abs(x)-62)-23.75+1.5*math.sin(-11.1+x*.06)+.7*math.cos(x*.2)) for x in range(-73,74,4)],.45,'Ivory')
    pillow('duvet_folded_top',(0,41,58),(137,19,7),'Linen')
    collision((0,0,28),(145,211,56))
    collision((0,99,77),(145,12,77))
    end()


def nightstand():
    begin('Nightstand', (55.115, 58.789, 71), reference='SM_Agit_F_002; separate bedside cabinet from original lamp combination')
    for x in [-17,17]:
        for y in [-14,14]:
            turned_leg('nightstand_foot',x,y,12,'Oak',2.5)
    box('nightstand_carcass',(0,0,31),(44,39,42),'Oak',2)
    box('nightstand_crown',(0,0,55),(48,43,4),'Oak',2)
    for z in [22,41]:
        panel('pink_drawer',(0,-20,z),(39,3,16),'Pink')
        handle('drawer_pull',0,-22,z,9)
    collision((0,0,28.5),(46,41,57))
    end()


def bedside_lamp():
    begin('BedsideLamp', (36, 36, 56.866), reference='SM_Agit_F_002; separate pleated ivory bedside lamp from original 127.866cm combination')
    cylinder('lamp_foot',(0,0,2),11,4,'Brass',bevel=1)
    sphere('floral_lamp_base',(0,0,11),(17,17,20),'Floral')
    cylinder('lamp_neck',(0,0,24),1.8,13,'Brass',bevel=.5)
    ring('neck_ring',(0,0,21),4,.8,'Brass')
    verts, faces=[],[]
    sides=96
    for z,rad in [(27,16),(46,9.5)]:
        for i in range(sides):
            a=TAU*i/sides
            r=rad+(.55 if i%2 else -.55)
            verts.append((r*math.cos(a),r*math.sin(a),z))
    for i in range(sides):
        faces.append((i,(i+1)%sides,(i+1)%sides+sides,i+sides))
    surface('pleated_linen_shade',verts,faces,'Linen',smooth=False)
    ring('shade_bottom_binding',(0,0,27),16,.65,'Ivory')
    ring('shade_top_binding',(0,0,46),9.5,.6,'Ivory')
    sphere('lamp_finial',(0,0,46.5),(3,3,3),'Brass')
    sphere('lamp_bulb',(0,0,32),(6,6,9),'Ivory')
    collision((0,0,23),(24,24,46))
    end()


def bedroom_cabinet():
    begin('BedroomCabinet', (66.9, 58.8, 90), reference='Low two-door cabinet from Home_Reference.png; bedside-sized addition')
    for x in [-25,25]:
        for y in [-17,17]:
            turned_leg('cabinet_bun_foot',x,y,10,'Oak',3.5)
    box('cabinet_body',(0,0,47),(62,45,74),'Oak',3)
    box('cabinet_plinth',(0,0,12),(66,49,6),'Oak',2)
    box('cabinet_crown',(0,0,86),(67,50,8),'Oak',3)
    for x in [-15,15]:
        panel('cabinet_door',(x,-24,48),(28,4,65),'Pink')
        panel('door_upper_inset',(x,-26,53),(21,1,47),'Pink')
        sphere('door_knob',(x+(-9 if x>0 else 9),-30,47),(3,3,3),'Brass')
        for z in [25,73]:
            box('brass_hinge',(x+(13 if x>0 else -13),-27,z),(1.7,1,5),'Brass',.5)
    pipe('cabinet_crown_inlay',[(-29,-27,85),(-17,-27,86),(0,-27,88),(17,-27,86),(29,-27,85)],.65,'Brass')
    collision((0,0,45),(66,50,90))
    end()


def rug(short_name, dimensions, oval=False):
    begin(short_name, dimensions, reference='Scalloped coral rug with cream lace border')
    sides=192
    sx,sy=dimensions[0]/2,dimensions[1]/2
    bands=[(0,.0,'Rug'),(.77,.7,'Rug'),(.80,.8,'Ivory'),(.84,.8,'Rug'),(.88,.8,'Lace'),(1,.7,'Lace')]
    for b in range(1,len(bands)):
        r0,z0,_=bands[b-1]
        r1,z1,mat=bands[b]
        verts,faces=[],[]
        for r,z in [(r0,z0),(r1,z1)]:
            for i in range(sides):
                a=TAU*i/sides
                scallop=1+.019*math.cos(a*(40 if oval else 36))*(r**8)
                verts.append((sx*r*math.cos(a)*scallop,sy*r*math.sin(a)*scallop,z))
        for i in range(sides):
            faces.append((i,(i+1)%sides,(i+1)%sides+sides,i+sides))
        surface('rug_'+str(b),verts,faces,mat,smooth=True)
    # Raised embroidered medallions follow the perimeter at an even spacing.
    for i in range(36):
        a=TAU*i/36
        ring('embroidered_oval',(sx*.92*math.cos(a),sy*.92*math.sin(a),.9),2.2,.35,'Ivory',scale=(1,.75,.5))
    collision((0,0,.5),(dimensions[0]*.94,dimensions[1]*.94,1))
    end()


def floor_cushion():
    begin('FloorCushion',(56.984,61.425,21.462),reference='SM_Agit_E_005; BunkerMap component scale 1; pink gingham square floor cushion')
    pillow('gingham_floor_cushion',(0,0,5.5),(48,48,11),'Gingham')
    for x in [-10,10]:
        for y in [-10,10]:
            sphere('cushion_button',(x,y,10.7),(1.9,1.9,.6),'Ivory')
    for x in [-23,23]:
        pipe('corner_tie',[(x,20,5),(x*1.03,24,4),(x*.94,25,3.8)],.4,'Linen')
    collision((0,0,5),(45,45,10))
    end()


init('Furniture')
dining_table()
stool()
sofa()
coffee_table()
bed()
nightstand()
bedside_lamp()
bedroom_cabinet()
rug('RoundRug',(185,185,1.5))
rug('OvalRug',(235,150,1.5),True)
floor_cushion()
complete()
