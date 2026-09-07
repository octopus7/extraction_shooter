"""Vintage oak/brass observation consoles for the mole's second control room.

Reusable, deterministic Blender authoring. Units are centimetres; front is -Y.
Run with Blender --background --factory-startup --python Tools/MoleControlV2/build_console.py.
The supplied reference informs the consoles, not its pictured characters.
"""
import sys, math, json
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import bpy
from mathutils import Vector
import common as c

REF = 'References/User_Reference.png: curved oak console, parchment map and brass analog controls'
TILT = math.radians(25)
V = Vector((0, math.sin(TILT), math.cos(TILT)))
N = Vector((0, -math.cos(TILT), math.sin(TILT)))


def face(center, x, h, d=0):
    return Vector(center) + Vector((x, 0, 0)) + V*h + N*d


def panel_box(name, center, size, mat='Iron', bevel=.7):
    ob = c.box(name, center, size, mat, bevel)
    ob.rotation_euler.x = -TILT
    return ob


def profile_x(name, profile_yz, x0, x1, mat='Oak', bevel=.8):
    n = len(profile_yz)
    vertices = [(x, y, z) for x in (x0, x1) for y, z in profile_yz]
    faces = [tuple(reversed(range(n))), tuple(range(n, 2*n))]
    faces += [(i, (i+1)%n, (i+1)%n+n, i+n) for i in range(n)]
    return c.surface(name, vertices, faces, mat, bevel, False)


def rr(width, height, radius, steps=6):
    points = []
    for cx, cy, a0 in [(width/2-radius, height/2-radius, 0),
                       (-width/2+radius, height/2-radius, 90),
                       (-width/2+radius, -height/2+radius, 180),
                       (width/2-radius, -height/2+radius, 270)]:
        for i in range(steps+1):
            a = math.radians(a0 + i*90/steps)
            points.append((cx+radius*math.cos(a), cy+radius*math.sin(a)))
    return points


def face_round_rect(name, center, width, height, radius, offset, mat):
    outline = rr(width, height, radius)
    verts = [face(center, 0, 0, offset)]
    verts += [face(center, x, z, offset) for x, z in outline]
    faces = [(0, i+1, (i+1)%len(outline)+1) for i in range(len(outline))]
    return c.surface(name, verts, faces, mat)


def face_frame(name, center, width, height, radius, border, offset, mat='Brass'):
    outer = rr(width, height, radius)
    inner = rr(width-2*border, height-2*border, max(.5, radius-border))
    n = len(outer)
    verts = []
    for depth, outline in [(offset, outer), (offset, inner),
                            (offset-.9, outer), (offset-.9, inner)]:
        verts.extend(face(center, x, z, depth) for x, z in outline)
    faces = []
    for i in range(n):
        j = (i+1)%n
        faces.extend([(i,j,n+j,n+i), (2*n+i,2*n+j,j,i),
                      (n+i,n+j,3*n+j,3*n+i)])
    return c.surface(name, verts, faces, mat, 0, True)


def gauge(name, center, x, h, radius=9, depth=3, material='Dial'):
    p = face(center,x,h,depth)
    c.cylinder(name+'_case',p,radius+1.15,2.7,'Brass',N,32,.3)
    c.cylinder(name+'_face',p+N*1.6,radius,.45,material,N,32,.12)
    c.ring(name+'_rim',p+N*1.9,radius+.1,.55,'Brass',N)
    # Physical scale marks and needle remain legible even when the map atlas is small.
    for i in range(9):
        a = math.radians(-125+i*31.25)
        direction = Vector((math.sin(a),0,0)) + V*math.cos(a)
        pp = p + direction*(radius*.76) + N*2.05
        tick=c.box(name+'_tick_%02d'%i,pp,(.45,.35,radius*.15),'Iron',.06)
        tick.rotation_euler.x=-TILT
        tick.rotation_euler.rotate_axis('Y',-a)
    a = math.radians(48)
    end = p + Vector((math.sin(a),0,0))*radius*.66 + V*(math.cos(a)*radius*.66)+N*2.3
    c.pipe(name+'_needle',[p+N*2.3,end],.24,'Iron')
    c.cylinder(name+'_pivot',p+N*2.4,1.0,.55,'Brass',N,20,.12)


def lamp(name, center, x, h, mat='Amber', radius=2.2, depth=3):
    p=face(center,x,h,depth)
    c.cylinder(name+'_socket',p,radius+1.1,1.8,'Brass',N,24,.25)
    c.cylinder(name+'_glass',p+N*1.3,radius,1.7,mat,N,24,.4)


def knob(name, center, x, h, radius=2.6, depth=4):
    p=face(center,x,h,depth)
    c.cylinder(name+'_base',p,radius+1,.8,'Brass',N,24,.1)
    c.cylinder(name+'_grip',p+N*1.4,radius,3,'Iron',N,24,.3)
    c.cylinder(name+'_cap',p+N*3,radius*.7,.3,'Brass',N,24,.08)
    c.pipe(name+'_mark',[p+N*3.2,p+N*3.2+V*(radius*.63)],.16,'Cream')


def top_knob(name, x, y, z, radius=2.5):
    axis=Vector((0,-.259,.966))
    c.cylinder(name+'_socket',(x,y,z),radius+1.2,1,'Brass',axis,24,.12)
    c.cylinder(name+'_grip',Vector((x,y,z))+axis*2.5,radius,4,'Iron',axis,24,.32)
    c.cylinder(name+'_cap',Vector((x,y,z))+axis*4.6,radius*.75,.45,'Brass',axis,24,.08)


def grille(name, loc, width, height, orientation='vertical'):
    x,y,z=loc
    c.box(name+'_recess',(x,y+.3,z),(width,1,height),'Iron',.6)
    for k in range(max(3, int(width/3))):
        xx=x-width/2+1.8+k*(width-3.6)/(max(3,int(width/3))-1)
        c.box(name+'_bar_%02d'%k,(xx,y-.4,z),(.65,1.0,height-2),'Brass',.18)
    for k in (-1,1):
        c.box(name+'_cross_%d'%k,(x,y-.9,z+k*height*.26),(width-2,.7,.65),'Brass',.12)


def console():
    c.begin('GrandMapConsole',(360,115,210),REF)
    # Twin storage pedestals and a genuine open knee space under the middle desk.
    for x in (-137,137):
        c.box('pedestal_plinth',(x,5,5),(67,91,10),'Oak',1.2)
        c.box('pedestal_body',(x,5,45),(60,85,72),'Oak',1.5)
        c.box('pedestal_inset',(x,-38.5,46),(49,2,59),'Iron',.55)
        c.box('raised_door',(x,-40,46),(43,2,53),'Oak',1.2)
        c.box('inner_door_panel',(x,-41.3,46),(33,.7,42),'Oak',.45)
        c.cylinder('door_keyhole',(x+15,-42.1,44),1.4,.7,'Brass',(0,-1,0),20,.12)
        c.pipe('door_pull',[(x+13,-42,52),(x+13,-45,56),(x+13,-42,60)],.7,'Brass')
        for z in (25,65):
            c.box('hinge',(x-22,-42,z),(3,1.8,6),'Brass',.4)
        c.collision((x,5,43),(65,91,86))
    c.box('rear_crossbrace',(0,43,65),(275,8,14),'Oak',1)
    # Both rails terminate inside the continuous cheeks, avoiding coincident
    # exterior side faces where a full-width tabletop would intersect them.
    c.box('desk_lower_edge',(0,-1,86),(332,110,7),'Oak',1.8)
    c.box('desk_bullnose',(0,-3,91),(332,109,7),'Oak',2)
    c.collision((0,0,91),(360,112,15))
    c.collision((0,45,140),(333,18,100))
    # Continuous bent silhouette: not a rectangular monitor bolted on a table.
    profile=[(-56.5,84),(-57.5,91),(-53,102),(-41,111),(-26,119),
             (-17,132),(-7,157),(4,183),(12,199),(24,208),(36,210),
             (57.5,210),(57.5,83)]
    for x0,x1 in [(-180,-166),(166,180)]:
        profile_x('curved_oak_cheek',profile,x0,x1,'Oak',1.1)
        x=(x0+x1)/2
        c.pipe('cheek_brass_pinstripe',[(x,-54,94),(x,-37,115),(x,-20,135),
                                      (x,-2,176),(x,15,201),(x,37,207)],.55,'Brass')
        c.collision((x,29,152),(14,57,116))
    c.box('rear_carcase',(0,47,152),(332,19,111),'Oak',1.5)
    c.box('crown_shelf',(0,39,207),(331,36,6),'Oak',1.2)
    c.box('crown_brass_edge',(0,20.7,206.5),(328,.7,1.3),'Brass',.2)
    center=(0,9,158)
    panel_box('sloped_face_backing',face(center,0,0,-4),(329,8,98),'Oak',1.5)
    # The map is inset behind two independent rounded frames, with no glass plane.
    face_round_rect('map_dark_reveal',center,228,86,8,.2,'Iron')
    face_frame('map_outer_moulding',center,233,91,9,3.3,1.3,'Oak')
    face_frame('map_brass_bezel',center,226,84,7,2.0,2.0,'Brass')
    face_round_rect('parchment_situation_map',center,221.6,79.6,4.8,1.0,'Map')
    # Inclined button banks sit on both sides of the single integrated map display.
    for s in (-1,1):
        x=s*143
        panel_box('instrument_bank',face(center,x,0,.9),(43,2.5,91),'Iron',1.1)
        for h in (-37,37):
            panel_box('bank_border',face(center,x,h,2.35),(38,.8,1.1),'Brass',.2)
        gauge('large_analog',center,x,-13,8.5,3)
        gauge('small_analog',center,x,10,6.5,3)
        for dx,mat in [(-11,'Amber'),(0,'Green'),(11,'Green')]:
            lamp('status',center,x+dx,31,mat,2.0,3)
        for dx in (-10,10):
            knob('bank_selector',center,x+dx,-33,2.4,3)
        p=face(center,x,23,3)
        panel_box('bank_nameplate',p,(24,.9,3.2),'Brass',.25)
        for j in range(3):
            panel_box('plate_etch',face(center,x-6+j*6,23,3.6),(3.5,.3,.45),'Iron',.04)
    # Work surface slopes gently up into the upright console body.
    slab=c.box('inclined_control_worktop',(0,-30,105),(329,53,5),'Oak',1.7)
    slab.rotation_euler.x=math.radians(15)
    c.collision((0,-27,107),(331,51,23))
    for s in (-1,1):
        for idx in range(3):
            x=s*(30+idx*28)
            y=-37+(idx%2)*7
            z=105+(y+30)*math.tan(math.radians(15))+3.1
            plaque=c.box('selector_plate',(x,y,z),(17,16,.8),'Brass',.6)
            plaque.rotation_euler.x=math.radians(15)
            top_knob('main_selector',x,y,z+.7,3.4 if idx==1 else 2.7)
    for s in (-1,1):
        for row in range(2):
            for col in range(3):
                x=s*139+(col-1)*9
                y=-41+row*14
                z=105+(y+30)*math.tan(math.radians(15))+4
                axis=Vector((0,-.259,.966))
                c.cylinder('pushbutton_socket',(x,y,z),3.4,1.1,'Brass',axis,24,.15)
                c.cylinder('pushbutton',Vector((x,y,z))+axis*1.3,2.4,1.8,
                           ('Amber' if col==0 else 'Green' if col==1 else 'Iron'),axis,24,.35)
    # Map reading lamp with brass gooseneck, hood and warm emitter.
    c.pipe('reading_light_arm',[(0,45,207),(0,15,209),(0,-4,204)],1.3,'Brass')
    c.box('reading_light_hood',(0,-6,202),(42,13,5),'Brass',1.8)
    c.box('reading_light_diffuser',(0,-6,199.2),(35,8,.6),'Amber',.5)
    c.end()


def radio_dial(name,x,y,z,radius=9):
    c.cylinder(name+'_bezel',(x,y,z),radius+1.0,1.8,'Brass',(0,-1,0),32,.2)
    c.cylinder(name+'_face',(x,y-1.3,z),radius,.45,'Dial',(0,-1,0),32,.12)
    c.ring(name+'_rim',(x,y-1.7,z),radius+.05,.45,'Brass',(0,-1,0))
    c.pipe(name+'_needle',[(x,y-2.0,z),(x+radius*.5,y-2.0,z+radius*.35)],.25,'Iron')
    c.cylinder(name+'_pin',(x,y-2.2,z),.8,.4,'Brass',(0,-1,0),16,.08)


def front_knob(name,x,y,z,radius=2.6):
    c.cylinder(name+'_washer',(x,y,z),radius+1.0,.75,'Brass',(0,-1,0),24,.1)
    c.cylinder(name+'_body',(x,y-1.4,z),radius,3.0,'Iron',(0,-1,0),24,.3)
    c.cylinder(name+'_cap',(x,y-3.1,z),radius*.65,.4,'Brass',(0,-1,0),24,.1)


def receiver_body(name,center,width=79,depth=48,height=35):
    x,y,z=center
    c.box(name+'_oak_case',(x,y,z),(width,depth,height),'Oak',2)
    c.box(name+'_front_recess',(x,y-depth/2-.5,z),(width-6,1.5,height-5),'Iron',.8)
    c.box(name+'_brass_top',(x,y-depth/2-1.4,z+height/2-3),(width-8,.7,1.2),'Brass',.2)
    c.box(name+'_brass_bottom',(x,y-depth/2-1.4,z-height/2+3),(width-8,.7,1.2),'Brass',.2)
    for sx in (-1,1):
        for sz in (-1,1):
            c.cylinder(name+'_screw',(x+sx*(width/2-5),y-depth/2-1.5,z+sz*(height/2-4.5)),
                       .75,.6,'Brass',(0,-1,0),16,.08)


def radio_stack():
    c.begin('VintageRadioStack',(85,55,125),REF)
    c.box('plinth',(0,0,5),(85,55,10),'Oak',1.4)
    # Three individual receivers with air gaps rather than one box with decals.
    for idx,z in enumerate((29,67,105)):
        receiver_body('radio_%d'%idx,(0,0,z),81,48,34)
        for x in (-28,28):
            c.box('receiver_foot',(x,0,z-18),(8,30,4),'Iron',.5)
        c.collision((0,0,z),(81,49,34))
    # Upper unit: analog dial and mechanical tuning scale.
    radio_dial('upper_meter',25,-25.1,105,9.4)
    c.box('tuning_scale_frame',(-15,-25.8,111),(39,1.4,9),'Brass',.7)
    c.box('tuning_scale_cream',(-15,-26.7,111),(34,.5,5.5),'Cream',.15)
    for k in range(13):
        c.box('tuning_hash',(-30+k*2.5,-27.1,112),( .3,.3,3 if k%3==0 else 1.5),'Iron',.02)
    c.box('tuning_cursor',(-12,-27.4,111),(.7,.3,5),'BookRed',.05)
    for x in (-29,-12,4):front_knob('upper_tuning',x,-26.4,97,2.7)
    # Middle unit: brass slatted speaker grille, meters and luminous tell-tales.
    grille('middle_speaker',(-17,-25.8,67),36,23)
    radio_dial('middle_meter',22,-26,71,7.3)
    for x,mat in [(12,'Green'),(23,'Amber'),(34,'Green')]:
        c.cylinder('middle_socket',(x,-26.2,58),2.6,1.2,'Brass',(0,-1,0),20,.15)
        c.cylinder('middle_bulb',(x,-27.4,58),1.7,1.3,mat,(0,-1,0),20,.3)
    # Lower unit: selectable patch connections and two large rotary controls.
    for x in (-24,24):front_knob('lower_tuning',x,-26.1,30,6)
    for x in (-7,7):
        for z in (24,35):
            c.ring('patch_socket',(x,-26.4,z),2.2,.65,'Brass',(0,-1,0))
            c.cylinder('patch_dark_core',(x,-26,z),1.5,.8,'Iron',(0,-1,0),20,.1)
    c.pipe('patch_cable',[(-7,-27,24),(-8,-27,18),(6,-27,17),(7,-27,24)],.65,'Iron')
    c.collision((0,0,5),(85,55,10))
    c.end()


def shelf_receiver():
    c.begin('ShelfReceiver',(85,35,38),REF)
    receiver_body('shelf_receiver',(0,0,20),85,30,36)
    for x in (-31,31):c.box('foot',(x,0,1.5),(9,23,3),'Iron',.5)
    radio_dial('shelf_meter',26,-16.0,21,11.3)
    c.box('pilot_bank',(-10,-16,24),(39,1.4,20),'Brass',.9)
    for x,mat in [(-24,'Amber'),(-11,'Green'),(2,'Green')]:
        c.box('pilot_dark_recess',(x,-17,27),(8,1.0,11),'Iron',.5)
        c.box('pilot_bulb',(x,-17.8,27),(5.5,.8,7),mat,.6)
        front_knob('pilot_selector',x,-17.4,14,2.4)
    c.collision((0,0,19),(85,35,38))
    c.end()


def detail_previews():
    scene=bpy.context.scene
    scene.render.resolution_x=1600;scene.render.resolution_y=1100
    scene.cycles.samples=40
    for ob in c.objects:ob.hide_render=True
    for entry,ob in zip(c.entries,c.objects):
        ob.hide_render=False
        original=ob.location.copy();ob.location=(0,0,0)
        height=entry['size_cm'][2]/100
        target=Vector((0,0,height*.5))
        scene.camera.location=target+Vector((3.6,-7.5,3.0))
        scene.camera.rotation_euler=(target-scene.camera.location).to_track_quat('-Z','Y').to_euler()
        scene.camera.data.ortho_scale=max(entry['size_cm'][0]/100*1.4,height*2.10)
        scene.render.filepath=str(c.OUT/'Previews'/f"{entry['name']}.png")
        bpy.ops.render.render(write_still=True)
        ob.location=original;ob.hide_render=True
    for ob in c.objects:ob.hide_render=False


def verify():
    manifest=json.loads((c.OUT/'Manifests/Console.json').read_text(encoding='utf-8'))
    checked=[]
    for entry in manifest['assets']:
        bpy.ops.wm.read_factory_settings(use_empty=True)
        bpy.ops.import_scene.fbx(filepath=str(c.OUT/'Models'/f"{entry['name']}.fbx"))
        ob=bpy.data.objects[entry['name']]
        verts=[ob.matrix_world@v.co for v in ob.data.vertices]
        sizes=[100*(max(v[i] for v in verts)-min(v[i] for v in verts)) for i in range(3)]
        assert max(abs(sizes[i]-entry['size_cm'][i]) for i in range(3))<.02,(entry['name'],sizes)
        assert abs(min(v.z for v in verts)*100)<.02,entry['name']
        ucx=[o for o in bpy.data.objects if o.name.startswith('UCX_'+entry['name'])]
        assert len(ucx)==entry['collision_boxes'],entry['name']
        assert len(ob.data.uv_layers)==2,entry['name']
        ob.data.calc_loop_triangles()
        assert len(ob.data.loop_triangles)==entry['triangles'],entry['name']
        checked.append({'name':entry['name'],'size_cm':sizes,'triangles':len(ob.data.loop_triangles),
                        'collision_boxes':len(ucx),'uv_channels':2,'passed':True})
    (c.OUT/'Manifests/Console_validation.json').write_text(json.dumps({'passed':True,'assets':checked},indent=2),encoding='utf-8')
    print('MOLE_CONTROL_CONSOLE_VALIDATION_PASS '+str(len(checked)))


if __name__=='__main__':
    if '--verify' in sys.argv:
        verify()
    else:
        c.init('Console')
        console();radio_stack();shelf_receiver();c.complete();detail_previews()
