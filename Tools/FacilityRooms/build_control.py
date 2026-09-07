"""Script-authored warm wooden attic control-room furnishings, dimensions in cm.

Run with Blender 4.5 in a dedicated background process.  All visible components
are real mesh geometry with shared semantic atlas materials; the separate UCX
boxes are intentionally coarse for predictable gameplay collision.
"""
import sys
import math
import json
from pathlib import Path

import bpy
from mathutils import Vector

sys.path.insert(0, str(Path(__file__).resolve().parent))
import common as c

REFERENCE = 'References/ControlRoom_Attic_Reference.png'


def bolt(name, x, y, z, r=.65):
    return c.cylinder(name, (x,y,z), r, .6, 'Steel', axis=(0,-1,0), vertices=12, bevel=.08)


def front_bar(name, x, y, z, width, height, mat='Blue'):
    return c.box(name, (x,y,z), (width,.015,height), mat, bevel=0)


def screen_graph(name, x, y, z, width, height, variant=0):
    """Readable, quiet non-text tactical interface geometry over an atlas screen."""
    front_bar(name+'_Header',x,y,z+height*.39,width*.88,height*.035,'Teal')
    front_bar(name+'_HeaderStatus',x+width*.33,y-.002,z+height*.39,width*.17,height*.035,'Amber')
    if variant % 3 == 0:
        for idx,(px,pz,wx,hz) in enumerate([(-.20,.13,.29,.025),(-.05,-.07,.43,.022),(.18,-.25,.31,.025)]):
            front_bar(name+f'_RouteH{idx}',x+width*px,y,z+height*pz,width*wx,height*hz)
        for idx,(px,pz,wx,hz) in enumerate([(-.34,.025,.015,.25),(-.04,-.045,.015,.34),(.32,-.18,.015,.18)]):
            front_bar(name+f'_RouteV{idx}',x+width*px,y-.002,z+height*pz,width*wx,height*hz)
        for idx,(px,pz) in enumerate([(-.34,.14),(-.04,-.07),(.32,-.25)]):
            front_bar(name+f'_Point{idx}',x+width*px,y-.004,z+height*pz,width*.045,height*.08,'Amber')
        front_bar(name+'_SideKey',x+width*.37,y,z+height*.10,width*.07,height*.28,'Teal')
    elif variant % 3 == 1:
        for idx in range(5):
            pz=z+height*(.23-idx*.115)
            front_bar(name+f'_Label{idx}',x-width*.29,y,pz,width*.12,height*.036,'Teal')
            front_bar(name+f'_Track{idx}',x+width*.09,y,pz,width*.49,height*.030,'Iron')
            front_bar(name+f'_Meter{idx}',x-width*.07+width*.032*idx,y-.002,pz,width*(.16+.064*idx),height*.035,'Amber' if idx==3 else 'Blue')
    else:
        for idx in range(6):
            px=x+width*(-.34+idx*.133)
            val=[.18,.36,.25,.45,.31,.52][idx]*height
            front_bar(name+f'_DataBar{idx}',px,y,z-height*.30+val/2,width*.054,val,'Blue' if idx!=4 else 'Amber')
        front_bar(name+'_Baseline',x,y,z-height*.34,width*.83,height*.022,'Teal')
    front_bar(name+'_Footer',x-width*.19,y,z-height*.42,width*.45,height*.02,'Teal')


def operator_desk():
    c.begin('OperatorDesk',(180,80,78),REFERENCE)
    c.box('Worktop',(0,0,74.5),(180,80,7),'Wood',1.2)
    c.box('FrontApron',(0,-37.5,68),(177,3,6),'Wood',.5)
    c.box('RearCableTray',(0,31,60),(145,13,12),'Steel',.7)
    c.box('LeftPedestal',(-62,3,35),(43,63,68),'Wood',1)
    c.box('LeftFoot',(-62,4,2),(44,62,4),'Wood',.5)
    for idx,z in enumerate([17,41,61]):
        height=28 if idx==0 else 18
        c.box(f'Drawer{idx}',(-62,-29.3,z),(38,2.5,height),'Wood',.4)
        c.box(f'DrawerPull{idx}',(-62,-32,z+4),(16,3,2),'Iron',.5)
    for y in [-28,28]:
        c.box('RightLeg'+str(y),(72,y,35),(7,7,68),'Wood',.6)
        c.box('RightFoot'+str(y),(72,y,2),(9,10,4),'Rubber',.5)
    c.box('RightCrossbar',(72,0,10),(5,60,5),'Wood',.5)
    c.box('ModestyPanel',(14,26,45),(104,2,33),'Wood',.5)
    c.cylinder('CableGrommet',(-57,27,78),4,.35,'Rubber',vertices=24,bevel=.1)
    c.ring('CableGrommetRim',(-57,27,78.05),3.8,.4,'Steel')
    c.collision((0,0,74.5),(180,80,7))
    c.collision((-62,3,35),(44,64,70))
    c.collision((72,-28,35),(8,8,70));c.collision((72,28,35),(8,8,70))
    c.collision((14,26,45),(104,5,33))
    c.end()


def monitor_wall():
    c.begin('MonitorWall',(240,35,170),REFERENCE)
    for x in [-78,78]:
        c.box('Base'+str(x),(x,0,3),(55,35,6),'Wood',1)
        c.box('Column'+str(x),(x,10,83),(9,9,160),'Wood',.6)
        c.box('FootPad'+str(x),(x,0,.9),(49,30,1.8),'Rubber',.3)
    c.box('BackCrossMember',(0,10,66),(220,8,8),'Steel',.6)
    c.box('BackCrossMemberUpper',(0,10,138),(220,8,8),'Steel',.6)
    for row,z in enumerate([87,142]):
        for col,x in enumerate([-80,0,80]):
            n=f'Panel_{row}_{col}'
            c.box(n+'_Back',(x,-1,z),(79,12,54),'Wood',1.2)
            c.box(n+'_Bezel',(x,-7.4,z),(76,2,51),'Iron',.5)
            c.box(n+'_Glass',(x,-8.6,z),(73.5,.9,47.5),'Screen',.45)
            screen_graph(n,x,-9.06,z,70,44,row*3+col)
            front_bar(n+'_Power',x+34,-9.4,z-25,2,.6,'Amber')
    c.collision((0,-1,114.5),(240,15,111))
    for x in [-78,78]:
        c.collision((x,0,3),(55,35,6));c.collision((x,10,31),(10,10,62))
    c.end()


def desk_monitor():
    c.begin('DeskMonitor',(65,25,50),REFERENCE)
    c.box('Base',(0,0,2),(39,25,4),'Iron',1.5)
    c.box('BaseInset',(0,-2,4),(25,16,1),'Wood',.5)
    c.box('Stem',(0,6,13),(7,7,23),'Steel',.6)
    c.cylinder('TiltJoint',(0,3,28),5,15,'Iron',axis=(1,0,0),vertices=20,bevel=.5)
    c.box('Housing',(0,0,31.5),(65,7,37),'Wood',1.2)
    c.box('Bezel',(0,-4,31.5),(62,2,34),'Iron',.5)
    c.box('Screen',(0,-5.2,32),(60,.6,31.8),'Screen',.4)
    screen_graph('DeskStatus',0,-5.51,32,56,28,0)
    front_bar('PowerLED',28,-5.5,15,1,.5,'Amber')
    c.box('RearVent',(0,4,37),(35,1,9),'Rubber',.2)
    for x in range(-15,16,5):c.box('RearFin'+str(x),(x,4.7,37),(1,1.4,7),'Iron',.1)
    c.collision((0,0,31.5),(65,10,37));c.collision((0,0,2),(39,25,4));c.collision((0,6,10),(10,10,20))
    c.end()


def radio_rack():
    c.begin('RadioRack',(90,65,185),REFERENCE)
    c.box('Cabinet',(0,0,94),(90,61,182),'Wood',1.3)
    c.box('InnerBlackFace',(0,-31.5,94),(82,2,173),'Rubber',.3)
    for x in [-42,42]:c.box('Rail'+str(x),(x,-33,95),(3,3,174),'Steel',.35)
    for x in [-35,35]:
        for y in [-24,24]:c.box('Foot',(x,y,3),(9,11,6),'Rubber',.5)
    for i,z in enumerate([23,55,87,119,151]):
        c.box(f'Module{i}',(0,-33,z),(78,3,28),'Ivory' if i in [1,3] else 'Panel',.55)
        for x in [-35.5,35.5]:
            bolt(f'ModuleBolt{i}',x,-35,z-10);bolt(f'ModuleBoltUpper{i}',x,-35,z+10)
        for x in [-29,29]:c.box(f'Handle{i}_{x}',(x,-36,z),(2,4,15),'Steel',.35)
        c.box(f'Display{i}',(-12,-35.2,z+3),(22,.7,10),'Screen',.3)
        front_bar(f'Channel{i}',-13,-35.7,z+4,15,1,'Blue')
        front_bar(f'Readout{i}',-16,-35.7,z+1,9,.7,'Amber')
        c.cylinder(f'Tuning{i}',(14,-36,z+2),5,4,'Iron',axis=(0,-1,0),vertices=24,bevel=.4)
        front_bar(f'TuningMark{i}',14,-38.2,z+5,.6,2,'Ivory')
        for n in range(3):
            c.cylinder(f'Button{i}_{n}',(-9+n*7,-35.5,z-8),1.3,.8,'Amber' if n==0 else 'Iron',axis=(0,-1,0),vertices=12,bevel=.2)
    c.box('BottomVent',(0,-35,9),(64,2,6),'Iron',.25)
    for x in range(-29,30,5):c.box('BottomVentSlat'+str(x),(x,-36.3,9),(1.2,1,5),'Steel',.12)
    c.box('Header',(0,-32,176),(77,3,10),'Wood',.4)
    front_bar('HeaderStatus',28,-34,176,8,2,'Amber')
    c.collision((0,0,93),(90,65,184))
    c.end()


def radio_console():
    c.begin('RadioConsole',(70,45,25),REFERENCE)
    c.surface('Housing',[(-35,-22.5,2),(35,-22.5,2),(35,22.5,2),(-35,22.5,2),(-35,-22.5,15),(35,-22.5,15),(35,22.5,25),(-35,22.5,25)],[(0,3,2,1),(0,1,5,4),(1,2,6,5),(2,3,7,6),(3,0,4,7),(4,5,6,7)],'Wood',.8,True)
    c.box('FrontPanel',(5,-22.5,8.3),(56,1.2,11),'Ivory',.5)
    c.box('Readout',(2,-23.2,9.2),(24,.8,7),'Screen',.3)
    front_bar('ReadoutBar',0,-23.7,10,16,1,'Blue')
    front_bar('ReadoutBar2',-4,-23.7,7.5,8,.7,'Amber')
    for x in [21,30]:c.cylinder('Dial'+str(x),(x,-23.7,8.5),2.5,2,'Iron',axis=(0,-1,0),vertices=20,bevel=.2)
    # Low handset silhouette with two distinct earpieces and a bridge.
    c.box('HandsetCradle',(-19,4,21),(22,28,2),'Iron',.6)
    c.box('HandsetFront',(-19,-8,24),(23,9,6),'Rubber',1.5)
    c.box('HandsetRear',(-19,13,28),(23,9,6),'Rubber',1.5)
    c.box('HandsetGrip',(-19,2.5,28),(8,24,5),'Iron',1.2)
    for x in [-25,-22,-19,-16,-13]:c.box('SpeakerSlot'+str(x),(x,13,31.2),(1,5,.5),'Panel',.15)
    c.box('ButtonBed',(14,5,23.5),(23,25,1),'Panel',.6)
    for row in range(3):
        for col in range(3):c.box(f'Key_{row}_{col}',(7+col*7,-3+row*7,24.5),(5,5,1.5),'Ivory',.4)
    c.pipe('HandsetCord',[(-30,-8,19),(-34,-4,8),(-31,13,9),(-25,18,18)],1,'Rubber')
    c.collision((0,0,13.5),(70,45,25))
    c.end()


def operator_chair():
    c.begin('OperatorChair',(65,65,115),REFERENCE)
    c.cylinder('BaseHub',(0,0,15),7,14,'Steel',vertices=24,bevel=.6)
    for i in range(5):
        a=2*math.pi*i/5
        x,y=math.cos(a)*17,math.sin(a)*17
        leg=c.box('BaseSpoke'+str(i),(x,y,10),(31,6,6),'Iron',.8);leg.rotation_euler.z=a
        wx,wy=math.cos(a)*29,math.sin(a)*29
        c.cylinder('Wheel'+str(i),(wx,wy,5),5,6,'Rubber',axis=(-math.sin(a),math.cos(a),0),vertices=20,bevel=.5)
    c.cylinder('GasLift',(0,0,29),3,26,'Steel',vertices=24,bevel=.4)
    c.cylinder('GasLiftBoot',(0,0,21),4.3,12,'Rubber',vertices=24,bevel=.4)
    c.box('SeatShell',(0,-1,46),(53,51,7),'Iron',2)
    c.box('SeatCushion',(0,-2,51),(52,49,9),'Ivory',3)
    c.box('BackSpine',(0,21,78),(8,7,60),'Steel',1)
    c.box('BackShell',(0,24,86),(48,8,58),'Wood',3)
    c.box('BackCushion',(0,18.5,86),(44,6,51),'Ivory',3)
    c.box('LumbarPad',(0,14.8,72),(39,3.5,13),'Rubber',2)
    for x in [-29,29]:
        c.box('ArmSupport'+str(x),(x,10,57),(4,5,26),'Steel',.7)
        c.box('ArmRest'+str(x),(x,-3,70),(7,36,5),'Wood',1.5)
    c.pipe('AdjustmentLever',[(13,0,44),(23,0,43),(23,-14,43)],.85,'Steel')
    c.box('LeverGrip',(23,-14,43),(4,10,3),'Rubber',.6)
    c.collision((0,0,10),(57,57,18));c.collision((0,-1,49),(53,51,14));c.collision((0,22,86),(49,13,58))
    c.end()


def keyboard_console():
    c.begin('KeyboardConsole',(75,35,9),REFERENCE)
    c.box('KeyboardTray',(0,0,3.5),(75,35,7),'Wood',1.5)
    c.box('KeyboardInset',(-10,3,7),(48,22,1),'Iron',.5)
    c.box('WristRest',(-10,-12,7),(49,8,3),'Ivory',1)
    for row in range(4):
        for col in range(11):
            c.box(f'Key_{row}_{col}',(-31+col*4.05,-5+row*4.8,8.5),(3.35,3.7,1.6),'Ivory' if row<3 else 'Panel',.35)
    c.box('SpaceBar',(-11,-7.3,8.5),(20,3.5,1.6),'Ivory',.35)
    c.box('TrackballBed',(25,2,7.7),(20,24,1.5),'Panel',.6)
    c.sphere('Trackball',(25,4,9),(11,11,7),'Iron')
    c.box('TrackballClickLeft',(20,-7,8.6),(8,5,1.5),'Ivory',.4)
    c.box('TrackballClickRight',(30,-7,8.6),(8,5,1.5),'Ivory',.4)
    c.collision((0,0,4.5),(75,35,9))
    c.end()


def file_cabinet():
    c.begin('FileCabinet',(90,45,130),REFERENCE)
    # The cap owns the visible z=130 face; overlapping coplanar upper faces
    # previously caused a black surface in the path-traced inspection render.
    c.box('Case',(0,0,64),(90,43,128),'Wood',1.2)
    c.box('Plinth',(0,0,3),(87,41,6),'Wood',.7)
    c.box('TopLip',(0,0,129),(90,45,2),'Wood',.4)
    for idx,z in enumerate([22,61,100]):
        c.box(f'Drawer{idx}',(0,-22,z),(84,2,36),'Wood',.6)
        c.box(f'Pull{idx}',(0,-24.5,z+5),(24,4,3),'Iron',.6)
        c.box(f'LabelFrame{idx}',(-24,-23.2,z+5),(15,.5,7),'Iron',.15)
        c.box(f'LabelCard{idx}',(-24,-23.6,z+5),(12,.3,4.5),'Paper',.1)
        c.cylinder(f'Lock{idx}',(28,-23.6,z+7),1.4,.6,'Steel',axis=(0,-1,0),vertices=16,bevel=.15)
    c.collision((0,0,65),(90,45,130));c.end()


def book_stack():
    c.begin('BookStack',(45,30,22),REFERENCE)
    for idx,(width,depth,z,angle,cover) in enumerate([(45,30,3.4,0,'Red'),(41,28,10.3,-.08,'Wood'),(43,27,17.2,.06,'Teal')]):
        for label,dz,height,mat in [('Pages',0,5.8,'Paper'),('LowerCover',-3.25,.7,cover),('UpperCover',3.25,.7,cover)]:
            o=c.box(f'Book{idx}_{label}',(0,0,z+dz),(width-(.7 if label=='Pages' else 0),depth-(.8 if label=='Pages' else 0),height),mat,.25)
            o.rotation_euler.z=angle
        o=c.box(f'Book{idx}_Spine',(0,-depth/2+.1,z),(width,.8,6.7),cover,.2);o.rotation_euler.z=angle
        for line in [-1.5,0,1.5]:
            o=c.box(f'Book{idx}_PageEdge{line}',(width/2-.4,0,z+line),(.15,depth-2,.1),'Ivory',0);o.rotation_euler.z=angle
    c.collision((0,0,11),(45,30,22));c.end()


def desk_mug():
    c.begin('DeskMug',(16,13,18),REFERENCE)
    c.lathe('CeramicWall',[(4.55,1),(5.25,1),(6,15.5),(5.25,15.5)],mat='Ivory',segments=40)
    c.cylinder('CeramicBase',(0,0,1),5.2,2,'Ivory',vertices=40,bevel=.3)
    c.ring('CeramicLip',(0,0,15.5),5.62,.4,'Ivory')
    c.ring('Handle',(7.5,0,9),3.4,1,'Ivory',axis=(0,1,0),scale=(1,1,1.35))
    c.cylinder('TeaSurface',(0,0,12.8),5.3,.12,'Wood',vertices=40,bevel=0)
    c.ring('FootRing',(0,0,.5),4.6,.5,'Ivory')
    c.collision((2.94,0,7.95),(17.92,12.04,15.9));c.end()


def verify_exports():
    """Fresh-import QA path; does not regenerate or mutate exported meshes."""
    source=Path(__file__).resolve().parents[2]/'TunaSweeper/SourceArt/Environment/FacilityRooms'
    manifest=json.loads((source/'Manifests/Control.json').read_text(encoding='utf-8'))
    checks=[]
    for entry in manifest['assets']:
        bpy.ops.wm.read_factory_settings(use_empty=True)
        bpy.ops.import_scene.fbx(filepath=str(source/'Models'/(entry['name']+'.fbx')))
        mesh=bpy.data.objects.get(entry['name']);assert mesh and mesh.type=='MESH',entry['name']
        mesh.data.calc_loop_triangles()
        coll=[o for o in bpy.data.objects if o.name.startswith('UCX_'+entry['name']+'_')]
        assert len(coll)==entry['collision_boxes'],(entry['name'],'collision')
        assert len(mesh.data.uv_layers)>=2,(entry['name'],'UV')
        assert len(mesh.data.materials)==len(entry['materials']),(entry['name'],'materials')
        assert len(mesh.data.loop_triangles)==entry['triangles'],(entry['name'],'triangles')
        dims=[float(v)*100 for v in mesh.dimensions]
        assert max(abs(a-b) for a,b in zip(dims,entry['size_cm']))<.02,(entry['name'],dims)
        assert all(t.area>1e-13 for t in mesh.data.loop_triangles),(entry['name'],'degenerate')
        checks.append({'name':entry['name'],'triangles':len(mesh.data.loop_triangles),'material_slots':len(mesh.data.materials),'collision_boxes':len(coll),'uv_channels':len(mesh.data.uv_layers),'size_cm':dims})
    print('CONTROL_FBX_REIMPORT_PASSED '+json.dumps(checks))


def render_details(only=None):
    """Front-biased inspection renders show the small physical interface parts."""
    scene=bpy.context.scene;cam=scene.camera
    scene.render.resolution_x=1100;scene.render.resolution_y=900
    scene.cycles.samples=24
    original=[(o,o.location.copy()) for o in c.objects]
    for o in c.objects:o.hide_render=True
    for o,loc in original:
        if only and o.name not in only:continue
        o.hide_render=False;o.location=(0,0,0)
        height=o.dimensions.z;span=max(o.dimensions)
        target=Vector((0,0,height*.48))
        cam.location=target+Vector((4,-8,4.5));cam.rotation_euler=(target-cam.location).to_track_quat('-Z','Y').to_euler()
        cam.data.ortho_scale=span*1.45
        scene.render.filepath=str(c.OUT/'Previews'/('Control_'+o.name.removeprefix('SM_FacilityRooms_')+'.png'))
        bpy.ops.render.render(write_still=True)
        o.location=loc;o.hide_render=True
    for o in c.objects:o.hide_render=False


if __name__=='__main__':
    if '--verify' in sys.argv:verify_exports()
    else:
        c.init('Control')
        for build in [operator_desk,monitor_wall,desk_monitor,radio_rack,radio_console,operator_chair,keyboard_console,file_cabinet,book_stack,desk_mug]:build()
        c.complete()
        if '--cabinet-details' in sys.argv:render_details({'SM_FacilityRooms_FileCabinet'})
        elif '--desk-details' in sys.argv:render_details({'SM_FacilityRooms_OperatorDesk'})
        elif '--no-details' not in sys.argv:render_details()
