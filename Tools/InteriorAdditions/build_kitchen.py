"""Script-mode original kitchen siblings, using the generated reference board."""
import sys, math
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent))
from common import *

init('Kitchen')

def flower(x,y,z,r=3.3):
    for k in range(5):
        a=2*math.pi*k/5
        sphere('MagnetPetal',(x+r*math.sin(a),y,z+r*math.cos(a)),(r*1.35,1.8,r*1.35),'Ivory' if k%2 else 'Pink')
    sphere('MagnetHeart',(x,y-.9,z),(r,2,r),'Brass')

def handle(x,y,z,length,vertical=True,mat='Brass'):
    if vertical:
        for h in [-length/2,length/2]:sphere('HandleMount',(x,y+1,z+h),(4,3,4),mat)
        pipe('Handle',[(x,y,z-length/2),(x,y-3,z-length*.3),(x,y-3,z+length*.3),(x,y,z+length/2)],1.65,mat)
    else:
        pipe('Handle',[(x-length/2,y+2,z),(x-length/2+3,y-2,z),(x+length/2-3,y-2,z),(x+length/2,y+2,z)],1.6,mat)

begin('Refrigerator',(102.128,95.468,173.174),'/Game/Environment/Bunker/Agit/GroupE/SM_Agit_E_006')
box('Cabinet',(0,0,83),(82,72,158),'Pink',6)
box('RearCoilPanel',(0,35.5,82),(67,2,129),'Iron',1)
for z in range(25,140,8):box('RearCondenser',(0,37,z),(60,1,1),'Iron',.35)
for x in (-28,28):
    for y in (-23,23):cylinder('Foot',(x,y,3.5),5,7,'Walnut')
for z,h in [(47,78),(125,70)]:
    box('DoorGasket',(0,-36,z),(78,2.8,h),'Walnut',4)
    box('RoundedDoor',(0,-39,z),(80,9,h-2),'Pink',5)
    handle(-25,-46,z,24)
    for zz in [-h*.32,h*.32]:box('Hinge',(39,-34,z+zz),(2.5,5,8),'Brass',.8)
flower(15,-45.4,139,4);flower(-3,-45.4,111,3.7);flower(24,-45.4,65,3.9);flower(3,-45.4,40,3)
box('Note',(9,-45,121),(12,1,15),'Linen',.6)
box('NoteClip',(9,-45.9,128),(3,1.4,3),'Pink',.6)
for z in [119,122,125]:box('NoteRule',(9,-45.7,z),(6,.2,.4),'Oak',.1)
collision((0,0,82),(80,76,164));end()

# Double sink: the worktop is built around genuine openings; basin bowls have
# inset walls and a bottom, rather than a solid block or painted black patch.
begin('SinkCabinet',(204.257,100.648,137.651),'/Game/Environment/Bunker/Agit/GroupE/SM_Agit_E')
box('CarcassBottom',(0,0,9),(177,64,8),'Oak',2)
box('CarcassBack',(0,30,44),(177,4,70),'Oak',1)
for x in [-86.5,86.5]:box('CarcassSide',(x,0,44),(4,64,70),'Oak',1)
box('CarcassFrontRail',(0,-30,72),(177,4,12),'Oak',1)
box('ToeKick',(0,0,5),(169,60,10),'Walnut',1.5)
box('CounterFront',(0,-29,85),(184,12,7),'Oak',2.2)
box('CounterBack',(0,27,85),(184,16,7),'Oak',2.2)
box('CounterLeft',(-85,-2,85),(14,43,7),'Oak',2.2)
box('CounterRight',(54.5,-2,85),(75,43,7),'Oak',2.2)
box('CounterBridge',(-31,-2,85),(6,43,7),'Oak',1.2)
for x in [-56,-6]:
    # Rounded rectangular basin cross section, open at top.
    n=40;verts=[];faces=[]
    for w,d,z in [(46,42,88),(43,39,86),(34,29,67),(31,26,66)]:
        for k in range(n):
            a=2*math.pi*k/n;cx=math.copysign(abs(math.cos(a))**.42,math.cos(a));cy=math.copysign(abs(math.sin(a))**.42,math.sin(a))
            verts.append((x+w/2*cx,-2+d/2*cy,z))
    for j in range(3):
        for k in range(n):faces.append((j*n+k,j*n+(k+1)%n,(j+1)*n+(k+1)%n,(j+1)*n+k))
    faces.append(tuple(range(3*n,4*n)))
    surface('InsetSteelBasin',verts,faces,'Steel',0,True)
    cylinder('Drain',(x,-2,66.5),3,1,'Iron')
    for dx in [-1,1]:box('DrainSlit',(x+dx,-2,67.1),(.5,3,.1),'Steel',0)
pipe('SwanNeckFaucet',[(-31,27,88),(-31,27,113),(-31,21,120),(-31,6,117),(-31,5,110)],2.2,'Steel')
for x in [-44,-18]:
    cylinder('TapBase',(x,26,89),4.6,3,'Steel')
    cylinder('TapStem',(x,26,94),2.5,8,'Steel')
    box('TapHandle',(x,26,98),(10,3,3),'Steel',1)
for x in [-55,2]:
    box('PinkCupboardDoor',(x,-33,44),(54,4,67),'Pink',2.5)
    box('InsetPanel',(x,-35.3,44),(43,1.2,53),'Pink',3)
    for xx in [-22,22]:box('DoorRail',(x+xx,-36,44),(2,1,58),'Pink',.8)
    handle(x+(-1 if x>0 else 1)*18,-37,49,9)
for z in [22,45,68]:
    box('DrawerFront',(62,-33,z),(46,4,20),'Oak',1.8)
    box('DrawerInset',(62,-35.2,z),(38,.8,13),'Floral',1)
    sphere('DrawerPull',(62,-38,z),(5,4,5),'Brass')
box('SideTrim',(89,0,44),(3,63,76),'Oak',1)
collision((0,0,42),(177,64,84));end()

begin('StoveOven',(97.688,79.186,120.630),'/Game/Environment/Bunker/Agit/GroupE/SM_Agit_E_001')
box('Body',(0,0,45),(78,64,84),'Ivory',4)
box('Cooktop',(0,0,91),(80,68,7),'Ivory',2)
box('Backsplash',(0,29,99),(78,5,13),'Ivory',2)
for x in [-20,20]:
    for y in [-16,15]:
        cylinder('BurnerInset',(x,y,95),14,1.4,'Iron',vertices=48)
        for rad in [3,6,9,12]:ring('Coil',(x,y,96),rad,.65,'Iron')
        cylinder('BurnerCore',(x,y,95.8),2,1.5,'Iron')
box('ControlPanel',(0,-34,83),(77,6,15),'Ivory',2)
for x in [-27,-9,9,27]:
    cylinder('DialBezel',(x,-38,84),5.4,1.5,'Brass',(0,-1,0))
    cylinder('ControlDial',(x,-40,84),4.2,4,'Ivory',(0,-1,0))
    box('DialPointer',(x,-42.2,85),(1,.6,4),'Iron',.3)
box('DoorSeal',(0,-33,42),(69,3,63),'Walnut',3)
box('OvenDoor',(0,-35,42),(69,5,63),'Ivory',4)
box('GlassWindow',(0,-38,43),(53,1.4,38),'Glass',4)
for z in [33,49]:box('VisibleRack',(0,-39, z),(43,.2,.8),'Steel',.3)
handle(0,-40,70,56,False,'Steel')
for x in [-30,30]:
    for y in [-22,22]:cylinder('Foot',(x,y,3),4,6,'Walnut')
collision((0,0,50),(78,70,100));end()

def canister(x,y,z,color='Floral',r=10,h=20):
    lathe('CeramicJar',[(0.1,0),(r*.8,0),(r,3),(r,h-4),(r*.85,h),(r*.74,h),(r*.8,3),(.1,3)],(x,y,z),color)
    cylinder('Lid',(x,y,z+h+1),r+1,3,'Walnut')
    ring('LidRim',(x,y,z+h+1.5),r,.8,'Brass')
    sphere('LidKnob',(x,y,z+h+5),(6,6,6),'Walnut')

begin('PantryShelf',(121.370,79.926,89.547),'/Game/Environment/Bunker/Agit/GroupE/SM_Agit_E_002')
for x in [-48,48]:
    for y in [-20,20]:box('CornerLeg',(x,y,31),(5,5,62),'Oak',1.6)
for z in [8,47]:box('Shelf',(0,0,z),(105,51,5),'Oak',2)
for x in [-51,51]:box('EndRail',(x,0,54),(5,46,7),'Oak',2)
box('BackRail',(0,24,54),(103,4,10),'Oak',1)
for x,color in [(-33,'Pink'),(0,'Sage'),(33,'Floral')]:canister(x,0,50,color,11,21)
for z in [12,15,18]:box('LowerLinen',(22,-4,z),(37,29,3),'Linen' if z==15 else 'Gingham',1)
for z in [8,47]:collision((0,0,z),(105,51,5))
for x in [-48,48]:
    for y in [-20,20]:collision((x,y,30),(5,5,60))
end()

begin('CanisterSet',(58,22,29),'Kitchen pantry shelf decorative ceramics')
for x,color in [(-20,'Pink'),(0,'Sage'),(20,'Floral')]:canister(x,0,0,color,8.7,21)
collision((0,0,14),(58,22,28));end()

begin('ElectricKettle',(28,24,37),'Existing sink countertop ivory kettle')
lathe('CeramicBody',[(.1,1),(10,1),(11,4),(10,19),(7.2,25),(6.7,26),(6,24),(8,5),(.1,5)],mat='Floral')
cylinder('Base',(0,0,2),11.6,3,'Ivory')
ring('BaseTrim',(0,0,3.2),10.8,.55,'Brass')
lathe('Lid',[(.1,26),(6.8,26),(7.5,27),(5,29),(.1,29)],mat='Ivory')
sphere('TopKnob',(0,0,30),(5,5,4),'Pink')
pipe('Handle',[(8,0,24),(15,0,24),(17,0,17),(15,0,8),(10,0,6)],1.8,'Ivory')
pipe('Spout',[(-7,0,19),(-11,0,24),(-13,0,26)],2.3,'Ivory')
box('WaterGauge',(0,-10,16),(2.6,.8,10),'Glass',1)
collision((1,0,15),(28,22,30));end()

begin('CookingPot',(39,30,27),'Existing stovetop pot with lid')
lathe('PotWall',[(.1,1),(12,1),(14,4),(15,19),(13.8,19),(12.8,5),(.1,4)],mat='Iron')
ring('SteelLip',(0,0,19),14.5,.6,'Steel')
lathe('Lid',[(.1,19),(14.8,19),(14.8,20),(11,22),(4,24),(.1,24)],mat='Steel')
cylinder('LidKnobStem',(0,0,25),2,3,'Iron')
sphere('LidKnob',(0,0,27),(6,6,3),'Iron')
for sign in [-1,1]:pipe('SideHandle',[(sign*14,-5,16),(sign*19,-5,16),(sign*20,0,16),(sign*19,5,16),(sign*14,5,16)],1.3,'Iron')
collision((0,0,13),(39,28,26));end()

begin('FoldedNapkin',(32,26,3),'Existing kitchen countertop pink cloth')
for z,mat in [(1,'Pink'),(2,'Gingham')]:box('Fold',(0,0,z),(32,26,1.4),mat,.7)
pipe('Seam',[(-15,-11,2.7),(15,-11,2.7),(15,11,2.7)],.18,'Ivory')
collision((0,0,1.5),(32,26,3));end()

begin('WindowCurtains',(138,23,98),'/Game/Environment/Bunker/Agit/GroupE/SM_Agit_E_003')
box('WindowBacking',(0,4,48),(118,3,86),'Turquoise',1)
for x in [-59,0,59]:box('WindowMullion',(x,0,48),(5,7,88),'Oak',1)
for z in [5,48,91]:box('WindowRail',(0,0,z),(123,7,5),'Oak',1)
pipe('CurtainRod',[(-68,-4,99),(68,-4,99)],1.4,'Brass')
for side in [-1,1]:
    verts=[];faces=[];cols=20;rows=16
    for j in range(rows+1):
        t=j/rows;z=94-86*t
        for i in range(cols+1):
            s=i/cols;inner=20+14*math.sin(t*math.pi);x=side*(inner+(66-inner)*s)
            y=-6-2.2*math.cos(s*8*math.pi)*(1-.3*math.sin(t*math.pi))
            verts.append((x,y,z+1.2*math.sin(s*8*math.pi)*t))
    for j in range(rows):
        for i in range(cols):
            a=j*(cols+1)+i;faces.append((a,a+1,a+cols+2,a+cols+1))
    o=surface('DrapedCurtain',verts,faces,'Pink',0,True)
    mod=o.modifiers.new('ClothThickness','SOLIDIFY');mod.thickness=.007
    bpy.context.view_layer.objects.active=o;bpy.ops.object.modifier_apply(modifier=mod.name)
    pipe('CurtainTie',[(side*36,-9,40),(side*52,-10,38),(side*66,-9,40)],1.5,'Linen')
collision((0,3,48),(123,6,91));end()

begin('MilkCan',(36.263,28.122,59.205),'/Game/Environment/Bunker/Agit/GroupE/SM_Agit_E_004')
lathe('SteelChurn',[(.1,1),(12,1),(14,5),(14,30),(12,35),(7,39),(7,49),(6,49),(6,40),(11,34),(12,29),(12,5),(.1,4)],mat='Steel')
for z,rad in [(4,14),(31,13.5),(48,7.2)]:ring('ChurnBand',(0,0,z),rad,.8,'Steel')
cylinder('Lid',(0,0,50),8,2,'Steel')
pipe('LidGrip',[(-4,0,51),(-4,0,55),(4,0,55),(4,0,51)],1.2,'Steel')
for sign in [-1,1]:pipe('SideGrip',[(sign*11,0,35),(sign*17,0,37),(sign*17,0,28),(sign*14,0,26)],1.6,'Steel')
collision((0,0,26),(28,28,52));end()

complete()
