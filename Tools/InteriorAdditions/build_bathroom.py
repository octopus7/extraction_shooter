"""Additive bathroom and laundry props, modelled from inspected Agit originals."""
import sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent))
from common import *
import math


def towel(name, x, y, z, width=36, depth=25, mat='Linen'):
    box(name,(x,y,z),(width,depth,5),mat,2)
    pipe(name+'Piping',[(x-width*.44,y-depth*.46,z+1.5),(x,y-depth*.48,z+1.5),(x+width*.44,y-depth*.46,z+1.5)],.32,'Ivory')


def bottle(name,x,y,z,height=29,mat='Pink'):
    # Oval detergent bottle with moulded shoulder, screw cap, and label plaque.
    lathe(name+'Body',[(.25,0),(7,0),(8,2),(8,height-9),(7,height-6),(4,height-4),(3.4,height-2),(.25,height-2)],(x,y,z),mat,scale=(1,.7,1),segments=32)
    cylinder(name+'Cap',(x,y,z+height-1),3.6,3,'Ivory',vertices=24)
    box(name+'Label',(x,y-5.45,z+height*.45),(10,.6,10),'Floral',.7)
    ring(name+'Handle',(x+7,y,z+height*.65),4.5,1.3,mat,axis=(0,1,0),scale=(.65,1,1.1))


def washing_machine():
    begin('WashingMachine',(90.727,79.510,110.522),'/Game/Environment/Bunker/Agit/SM_Agit_WashingMachineA')
    box('EnamelCabinet',(0,0,50),(70,67,96),'Ivory',3)
    box('TopCap',(0,0,99),(73,70,5),'Ivory',1.8)
    box('RoseFrontInset',(0,-34,48),(65,2,80),'Pink',2)
    box('ControlFascia',(0,-35.5,87),(66,3,16),'Ivory',1.8)
    box('DrawerGap',(-22,-37.3,87),(20,.6,10),'Iron',.4)
    box('SoapDrawer',(-22,-37.9,87),(18,2,8),'Pink',.9)
    box('DrawerPull',(-22,-39.2,89),(12,1.7,1.7),'Brass',.6)
    cylinder('ProgramDial',(2,-39,88),5.3,3,'Brass',axis=(0,1,0))
    cylinder('ProgramDialFace',(2,-41,88),4.2,1,'Ivory',axis=(0,1,0))
    box('DialPointer',(2,-41.6,91),(1,.8,3),'Iron',.2)
    box('DisplayHousing',(23,-38.2,89),(14,1.8,6),'Iron',.6)
    box('DisplayGlass',(23,-39.3,89),(11,.5,3.5),'Glass',.4)
    for i in range(3):cylinder('Button'+str(i),(17+i*5,-39,81),1.4,1.5,'Brass',axis=(0,1,0),vertices=20)
    cylinder('DoorDarkWell',(0,-36,48),24,3,'Iron',axis=(0,1,0))
    ring('DoorOuterBezel',(0,-38.5,48),23,3.8,'Ivory',axis=(0,1,0))
    ring('DoorGasket',(0,-40,48),18.8,1.3,'Iron',axis=(0,1,0))
    cylinder('DoorGlass',(0,-39.8,48),18,1.7,'Glass',axis=(0,1,0),vertices=48)
    ring('DrumRing',(0,-41,48),13.6,1,'Steel',axis=(0,1,0))
    for i in range(8):
        a=i*math.pi/4;cylinder('DrumDimple'+str(i),(9*math.sin(a),-40.8,48+9*math.cos(a)),.65,.25,'Steel',axis=(0,1,0),vertices=12,bevel=.08)
    pipe('DoorHandle',[(20,-42,54),(23,-43,49),(20,-42,42)],1.2,'Brass')
    box('ServiceHatch',(-21,-36,13),(17,2,10),'Ivory',1.5)
    for i in range(5):box('Vent'+str(i),(14,-35.5,9+i*2),(22,.6,.75),'Iron',.2)
    for x in [-25,25]:
        for y in [-23,23]:cylinder('AdjustableFoot',(x,y,2),4,4,'Brass',vertices=20)
    collision((0,0,50),(70,67,100));end()


def laundry_shelf():
    begin('LaundryShelf',(140,92,184),'Screenshot laundry alcove; wooden surround sized to the existing washer')
    for x in [-54,54]:
        box('SidePost',(x,0,88),(8,73,172),'Oak',1.5)
        box('Foot',(x,0,5),(11,77,10),'Walnut',1)
    box('Crown',(0,0,175),(118,82,8),'Oak',2)
    box('UpperShelf',(0,0,125),(108,76,6),'Oak',1.2)
    box('TopBack',(0,35,150),(106,4,46),'Oak',.7)
    box('UpperRail',(0,34,176),(112,5,8),'Walnut',.7)
    # A narrow towel cubby creates the asymmetry visible beside the original washer.
    box('CubbyDivider',(-31,0,61),(4,73,116),'Oak',.7)
    for z in [16,49,82,111]:box('CubbyShelf',(-43,0,z),(22,73,3),'Oak',.6)
    for z in [23,28,56,61,89,94]:towel('CubbyTowel',-43,-9,z,18,36,'Gingham' if z%2 else 'Linen')
    towel('TopTowelA',22,-10,132,40,31,'Linen');towel('TopTowelB',24,-10,138,34,30,'Gingham')
    bottle('ShelfPinkBottle',-36,0,129,30,'Pink');bottle('ShelfSageBottle',-14,2,129,27,'Sage')
    collision((-54,0,89),(8,73,178));collision((54,0,89),(8,73,178));collision((0,0,175),(118,82,8));collision((0,0,125),(108,76,6));end()


def hamper():
    begin('LaundryHamper',(48,46,63),'Screenshot woven laundry basket beside washer')
    lathe('WickerBody',[(19,4),(22,6),(24,51),(23,54),(20.5,54),(20.8,50),(18,7),(17,4)],mat='Oak',segments=48)
    for z in range(8,54,4):ring('HorizontalWeave'+str(z),(0,0,z),19.6+z*.073,.5,'Walnut')
    for i in range(24):
        a=i*math.pi/12
        pipe('UprightWeave'+str(i),[(19.8*math.cos(a),19.8*math.sin(a),6),(23.7*math.cos(a),23.7*math.sin(a),51)],.55,'Oak')
    ring('RolledRim',(0,0,54),22.5,1.6,'Walnut')
    for x in [-23,23]:ring('BasketHandle',(x,0,48),7,1.2,'Walnut',axis=(1,0,0),scale=(1,1,.75))
    towel('LaundrySpill',0,0,53,34,32,'Gingham');towel('SmallTowel',-3,1,58,26,28,'Linen')
    collision((0,0,29),(42,42,56));end()


def toilet():
    begin('Toilet',(52.626,71.214,77.249),'/Game/Environment/Bunker/Agit/SM_Agit_Toilet; original X-facing mesh rotated to new -Y front')
    box('Foot',(0,1,6),(36,51,12),'Ivory',4)
    lathe('Pedestal',[(.3,7),(15,7),(15,22),(19,28),(19,30),(.3,30)],mat='Ivory',scale=(1,1.25,1))
    lathe('Bowl',[(1,23),(20,27),(28,39),(29,47),(27,51),(23,51),(22,46),(17,36),(1,32)],(0,-14,0),'Ivory',scale=(1,1.22,1))
    cylinder('BowlWater',(0,-14,35),13,1,'Turquoise',vertices=40)
    ring('RoseSeat',(0,-14,51),25,2.8,'Pink',scale=(1,1.22,.68))
    box('Tank',(0,27,72),(51,22,43),'Ivory',4)
    box('TankLid',(0,27,95),(54,25,6),'Ivory',2)
    cylinder('FlushButton',(0,27,98.5),3.7,1.7,'Brass')
    box('RaisedLid',(0,7,67),(47,4,32),'Pink',6)
    for x in [-12,12]:cylinder('SeatHinge',(x,6,50),2,5,'Brass',axis=(1,0,0))
    collision((0,1,24),(44,62,48));collision((0,27,73),(51,22,48));end()


def bathtub():
    begin('Bathtub',(155.726,226.280,116.876),'/Game/Environment/Bunker/Agit/SM_Agit_Bath; native bounds multiplied by existing BunkerMap actor scale 1.3449263')
    # A real hollow shell gives the rim and basin their own geometry and shadows.
    profile=[(18,13),(32,16),(40,31),(44,54),(44,62),(41,65),(38.5,62),(37,51),(32,27),(23,20),(18,20)]
    lathe('RoseTubShell',profile,mat='Pink',scale=(1,1.75,1),segments=64)
    ring('ThickIvoryRim',(0,0,63),41,3.2,'Ivory',scale=(1,1.75,.85))
    # Flat water surface stays below the rim; atlas turquoise carries hand-painted variation.
    water=cylinder('BathWater',(0,0,35),34.3,.9,'Turquoise',vertices=64,bevel=.15);water.scale.y=1.75
    ring('WaterRippleA',(0,0,35.6),17,.18,'Turquoise',scale=(1,1.5,.35))
    ring('WaterRippleB',(0,0,35.6),26,.14,'Turquoise',scale=(1,1.55,.35))
    for x in [-27,27]:
        for y in [-45,45]:
            sphere('ClawFoot',(x,y,6),(12,16,10),'Brass')
            pipe('FootStem',[(x,y,8),(x*1.05,y,13),(x*.9,y,21)],2.3,'Brass')
    pipe('GooseneckTap',[(0,55,49),(0,56,74),(0,46,82),(0,35,78),(0,34,69)],2.3,'Brass')
    pipe('TapBridge',[(-15,54,57),(0,54,57),(15,54,57)],1.7,'Brass')
    for x in [-15,15]:
        cylinder('TapRosette',(x,54,57),4,3,'Brass')
        pipe('TapCrossA',[(x-4,54,61),(x+4,54,61)],.85,'Brass')
        pipe('TapCrossB',[(x,50,61),(x,58,61)],.85,'Brass')
        sphere('PorcelainTapCap',(x,54,62),(3,3,2),'Ivory')
    collision((0,0,25),(78,130,45));end()


def washbasin():
    begin('Washbasin',(52.706,56.126,27.560),'/Game/Environment/Bunker/Agit/SM_Agit_Washstand')
    # Closed rounded rectangular profile: outside, rolled lip, inner bowl, drain floor.
    profiles=[(29,23,3),(36,28,9),(38,30,21),(37,29,24),(32,25,24),(30,23,18),(18,13,6),(8,6,4),(8,6,3)]
    verts=[];faces=[];n=64
    for hx,hy,z in profiles:
        for i in range(n):
            a=2*math.pi*i/n;c=math.cos(a);s=math.sin(a)
            verts.append((hx*math.copysign(abs(c)**.55,c),hy*math.copysign(abs(s)**.55,s),z))
    for j in range(len(profiles)):
        for i in range(n):faces.append((j*n+i,j*n+(i+1)%n,((j+1)%len(profiles))*n+(i+1)%n,((j+1)%len(profiles))*n+i))
    surface('CeramicBowl',verts,faces,'Ivory',smooth=True)
    cylinder('Drain',(0,0,4.5),4.2,.8,'Steel')
    for i in range(5):
        a=i*math.pi*2/5;cylinder('DrainSlot'+str(i),(2*math.cos(a),2*math.sin(a),5),.5,.2,'Iron',vertices=12,bevel=.05)
    box('BackRim',(0,29,22),(74,12,6),'Pink',2)
    pipe('BrassFaucet',[(0,27,25),(0,27,39),(0,17,44),(0,8,39)],1.7,'Brass')
    for x in [-14,14]:
        cylinder('TapBase',(x,27,26),3.4,3,'Brass')
        pipe('TapHandleA',[(x-4,27,30),(x+4,27,30)],.85,'Brass')
        pipe('TapHandleB',[(x,23,30),(x,31,30)],.85,'Brass')
        sphere('TapCap',(x,27,31),(3,3,2),'Ivory')
    for x in [-24,24]:box('WallBracket',(x,22,7),(6,19,11),'Brass',1)
    collision((0,0,12),(74,60,24));end()


def towel_stack():
    begin('TowelStack',(44,34,25),'Screenshot folded gingham towels on laundry/bedroom cabinet')
    for i in range(4):towel('FoldedTowel'+str(i),(-1)**i,0,3+i*5.5,42-i*2,32,'Gingham' if i%2 else 'Linen')
    box('TopBand',(0,-1,24),(5,30,1),'Lace',.3)
    collision((0,0,12),(44,34,24));end()


def detergent_set():
    begin('DetergentSet',(53,31,39),'Screenshot pink detergent bottles above laundry washer')
    box('WoodTray',(0,0,2),(53,31,4),'Oak',1.2)
    for x in [-25,25]:box('TraySide',(x,0,5),(2,31,6),'Walnut',.5)
    bottle('LargeRose',-13,1,4,34,'Pink');bottle('Sage',6,1,4,27,'Sage')
    lathe('PowderJar',[(.2,0),(7,0),(7,16),(6,18),(.2,18)],(20,0,4),'Ivory',segments=32)
    cylinder('PowderJarLid',(20,0,23),7.3,3,'Pink');box('JarLabel',(20,-7,13),(8,.7,8),'Floral',.7)
    collision((0,0,19),(53,31,38));end()


if __name__=='__main__':
    init('BathroomLaundry')
    washing_machine();laundry_shelf();hamper();toilet();bathtub();washbasin();towel_stack();detergent_set()
    complete()
