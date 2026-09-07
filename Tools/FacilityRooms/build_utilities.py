"""Editable, grounded industrial equipment for the underground facility.

Centimetres, X width, -Y operator side, Z up. Uses only the collection's
semantic material slots so paint, metal, rubber and indicators remain editable.
Run with Blender 4.5 --background --factory-startup --python this_file.py.
"""
import math
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import common as C

REF = 'References/Basement_Reference.png'


def bolts(prefix, x_values, y, z_values, mat='Steel'):
    for i, x in enumerate(x_values):
        for j, z in enumerate(z_values):
            C.cylinder(f'{prefix}_{i}_{j}', (x,y,z), 1.15, .9, mat,
                       axis=(0,1,0), vertices=8, bevel=0)


def gauge(prefix, x, y, z, radius=8):
    C.cylinder(prefix+'_Housing', (x,y,z), radius, 4, 'Steel', axis=(0,1,0), bevel=.3)
    C.cylinder(prefix+'_Face', (x,y-2.2,z), radius*.8, .8, 'Ivory', axis=(0,1,0), bevel=0)
    for k in range(7):
        a=math.radians(30+k*20)
        C.box(prefix+f'_Mark{k}', (x+math.cos(a)*radius*.6,y-2.7,z+math.sin(a)*radius*.6),
              (.8,.5,1.4), 'Iron', .05)
    # A raised needle survives the medium-distance room view.
    needle=C.box(prefix+'_Needle',(x-radius*.12,y-3,z+radius*.18),(.8,.6,radius*.65),'Red',.08)
    needle.rotation_euler[1]=math.radians(-30)
    C.cylinder(prefix+'_Pin',(x,y-3.4,z),1,1,'Iron',axis=(0,1,0),vertices=12,bevel=0)


def valve(prefix, loc, radius=13, axis=(0,1,0)):
    x,y,z=loc
    C.cylinder(prefix+'_Hub',(x,y,z),3.3,7,'Steel',axis=axis,vertices=16,bevel=.2)
    C.ring(prefix+'_Wheel',(x,y-4,z),radius,1.5,'Red',axis=axis)
    C.box(prefix+'_SpokeHorizontal',(x,y-4,z),(radius*1.8,1.8,1.8),'Red',.2)
    C.box(prefix+'_SpokeVertical',(x,y-4,z),(1.8,1.8,radius*1.8),'Red',.2)


def control_panel(prefix, loc, size=(44,8,55)):
    x,y,z=loc
    C.box(prefix+'_Frame',(x,y,z),size,'Iron',1)
    C.box(prefix+'_Face',(x,y-size[1]/2-.5,z),(size[0]-3,1,size[2]-3),'Panel',.5)
    C.box(prefix+'_Display',(x,y-size[1]/2-1.2,z+size[2]*.15),(size[0]*.66,1.2,size[2]*.32),'Screen',.4)
    C.box(prefix+'_DisplayLine',(x-3,y-size[1]/2-1.9,z+size[2]*.15),(size[0]*.38,.3,1.2),'Blue',.05)
    for i,mat in enumerate(('Amber','Teal','Red')):
        C.cylinder(prefix+f'_Button{i}',(x+(i-1)*size[0]*.23,y-size[1]/2-1.4,z-size[2]*.25),
                   2.4,2,mat,axis=(0,1,0),vertices=16,bevel=.15)
    C.box(prefix+'_Label',(x,y-size[1]/2-1.1,z-size[2]*.43),(size[0]*.66,.3,2),'Ivory',.05)


def boiler():
    C.begin('Boiler',(220,130,230),REF)
    C.box('Skid',(0,4,10),(211,115,20),'Iron',1.5)
    for x in (-76,76):
        C.box(f'Saddle{x}',(x,4,45),(18,96,65),'Steel',1.5)
        C.box(f'Foot{x}',(x,4,22),(38,106,6),'Iron',.5)
    C.cylinder('InsulatedPressureVessel',(0,7,123),54,179,'Teal',axis=(1,0,0),vertices=48,bevel=5)
    for x in (-85,-30,30,85):
        C.ring(f'ShellBand{x}',(x,7,123),54.1,1.25,'Steel',axis=(1,0,0))
    for x in (-92,92):
        C.cylinder(f'EndCap{x}',(x,7,123),49,9,'Steel',axis=(1,0,0),vertices=48,bevel=3)
    C.box('BurnerHousing',(-51,-43,75),(67,40,56),'Teal',4)
    C.cylinder('BurnerAirIntake',(-51,-65,75),20,7,'Iron',axis=(0,1,0),bevel=1)
    for k in range(6):
        C.box(f'BurnerGrill{k}',(-51,-69,60+k*6),(29,2,2.2),'Steel',.3)
    C.cylinder('MaintenanceHatch',(43,-45,125),27,8,'Iron',axis=(0,1,0),bevel=1)
    C.cylinder('MaintenanceHatchCenter',(43,-50,125),22,3,'Steel',axis=(0,1,0),bevel=.5)
    C.box('HatchHandle',(43,-54,125),(20,6,4),'Iron',1)
    for k in range(8):
        a=2*math.pi*k/8
        C.cylinder(f'HatchBolt{k}',(43+24*math.cos(a),-50,125+24*math.sin(a)),1.7,1.4,'Iron',axis=(0,1,0),vertices=8,bevel=0)
    C.cylinder('TopExhaust',(-63,7,188),15,69,'Steel',vertices=32,bevel=.6)
    C.ring('ExhaustCollar',(-63,7,163),17,2.4,'Iron')
    C.cylinder('ExhaustCap',(-63,7,222),21,6,'Iron',vertices=32,bevel=.8)
    C.pipe('HotWaterPipe',[(68,7,160),(68,7,188),(96,7,188),(103,7,171),(103,-43,171)],5,'Red')
    C.ring('HotWaterFlange',(103,-34,171),8.5,2,'Steel',axis=(0,1,0))
    valve('HotWaterValve',(103,-43,171),10)
    C.pipe('FeedPipe',[(-95,15,92),(-105,15,80),(-105,-30,43),(-72,-30,43)],4,'Steel')
    C.box('InstrumentMount',(-1,-45,160),(65,10,29),'Teal',1.5)
    gauge('PressureGauge',-16,-53,160,9)
    C.box('TemperatureScreen',(15,-51.5,162),(20,3,10),'Screen',.4)
    C.box('TemperatureBar',(15,-53.2,162),(12,.3,2),'Amber',.05)
    C.box('CautionPlaque',(2,-48,95),(31,2,11),'Hazard',.3)
    C.collision((0,7,114),(194,109,114))
    C.collision((0,4,28),(211,115,56))
    C.collision((-51,-48,75),(67,44,56))
    C.collision((-63,7,198),(42,42,52))
    C.collision((101,-11,171),(17,73,34))
    C.end()


def generator():
    C.begin('Generator',(250,120,160),REF)
    C.box('FoundationSkid',(0,0,9),(250,116,18),'Iron',2)
    for x in (-93,93):
        C.box(f'RailFoot{x}',(x,0,3),(20,120,6),'Steel',.5)
    C.box('FuelTank',(5,0,28),(218,101,25),'Iron',2)
    C.box('MainCanopy',(-29,0,92),(172,110,104),'Teal',4)
    C.box('CanopyRoof',(-26,0,147),(186,113,7),'Steel',2)
    C.box('EndFrame',(104,0,80),(27,106,93),'Iron',2)
    C.box('ExposedEngine',(69,1,85),(45,76,67),'Steel',3)
    C.cylinder('Alternator',(86,-3,80),31,49,'Iron',axis=(1,0,0),vertices=32,bevel=3)
    for x in (68,74,80,86,92,98):
        C.ring(f'AlternatorCoolingRing{x}',(x,-3,80),30.5,1.5,'Steel',axis=(1,0,0))
    C.box('RadiatorGrillFrame',(-72,-56,93),(70,5,77),'Iron',1)
    for i in range(10):
        C.box(f'LargeVentLouver{i}',(-72,-60,60+i*6.6),(63,5,3.4),'Steel',.5)
    C.box('AccessDoor',(0,-56,91),(67,4,82),'Teal',1.5)
    C.box('DoorInset',(0,-59,85),(57,2,47),'Panel',.6)
    C.box('DoorHandle',(23,-62,92),(3.5,5,17),'Steel',.8)
    for z in (65,117):
        C.box(f'DoorHinge{z}',(-30,-59,z),(4,4,10),'Steel',.4)
    control_panel('PowerControl',(0,-60,122),(46,5,25))
    C.box('ServiceStripe',(-22,-61,46),(172,2,7),'Ivory',.4)
    C.box('DangerPlate',(-89,-63,136),(27,1,8),'Hazard',.2)
    for x in (-119,120):
        C.box(f'CornerGuard{x}',(x,-51,47),(8,12,61),'Iron',1)
    C.cylinder('ExhaustMuffler',(48,24,140),10,59,'Steel',axis=(1,0,0),vertices=24,bevel=2)
    C.pipe('ExhaustElbow',[(47,23,111),(47,23,140),(83,23,140),(90,23,155)],4.5,'Iron')
    C.cylinder('FuelCap',(56,38,43),7,5,'Steel',vertices=20,bevel=.4)
    C.pipe('EngineCable',[(78,-37,57),(84,-46,39),(21,-46,37)],2.2,'Rubber')
    bolts('FrameBolt',(-109,37),-57,(45,138))
    C.collision((0,0,22),(250,116,44))
    C.collision((-29,0,95),(181,114,112))
    C.collision((84,0,85),(73,106,98))
    C.collision((58,24,143),(74,27,34))
    C.end()


def pump_skid():
    C.begin('PumpSkid',(200,110,130),REF)
    C.box('Skid',(0,0,7),(198,104,14),'Iron',1.3)
    for x in (-78,78):
        C.box(f'Rail{x}',(x,0,17),(15,96,11),'Steel',.7)
    for idx,x in enumerate((-48,48)):
        C.box(f'MotorFoot{idx}',(x,20,28),(52,61,14),'Steel',1)
        C.cylinder(f'Motor{idx}',(x,20,52),23,61,'Teal',axis=(0,1,0),vertices=32,bevel=2)
        for side in (-1,1):
            for k in range(4):
                C.box(f'CoolingFin{idx}_{side}_{k}',(x+side*(18+k*.9),19,44+k*5),(2,48,4.8),'Steel',.35)
        C.cylinder(f'MotorBackCap{idx}',(x,52,52),22,7,'Iron',axis=(0,1,0),vertices=24,bevel=1)
        C.box(f'MotorJunction{idx}',(x,24,79),(22,23,12),'Teal',1.5)
        C.cylinder(f'PumpBody{idx}',(x,-25,48),26,26,'Blue',axis=(0,1,0),vertices=32,bevel=3)
        C.cylinder(f'PumpFrontFlange{idx}',(x,-42,48),14,9,'Steel',axis=(0,1,0),vertices=24,bevel=.6)
        C.ring(f'FlangeRim{idx}',(x,-47,48),12,1.2,'Iron',axis=(0,1,0))
        C.pipe(f'SupplyBranch{idx}',[(x,-45,48),(x,-50,72),(x,-50,92)],6,'Blue')
        C.pipe(f'DeliveryBranch{idx}',[(x,-21,64),(x,-21,103),(x,0,114)],5.5,'Blue')
        C.ring(f'TopBranchFlange{idx}',(x,-21,89),9,2,'Steel')
        gauge(f'PressureGauge{idx}',x,-28,109,6.5)
        C.collision((x,17,48),(55,73,63))
        C.collision((x,-29,48),(55,48,53))
    C.cylinder('FrontManifold',(0,-48,93),7,187,'Blue',axis=(1,0,0),vertices=24,bevel=.7)
    C.cylinder('UpperManifold',(0,0,116),7,185,'Blue',axis=(1,0,0),vertices=24,bevel=.7)
    for x in (-89,89):
        C.cylinder(f'EndFlangeLow{x}',(x,-48,93),12,7,'Steel',axis=(1,0,0),vertices=24,bevel=.7)
        C.cylinder(f'EndFlangeHigh{x}',(x,0,116),12,7,'Steel',axis=(1,0,0),vertices=24,bevel=.7)
    valve('IsolationValve',(0,-50,93),11)
    C.box('EquipmentTag',(0,-53,23),(37,2,10),'Ivory',.3)
    C.collision((0,0,7),(198,104,14))
    C.collision((0,-48,93),(200,24,25))
    C.collision((0,0,116),(200,24,26))
    C.end()


def tank_shell(name, loc, radius, height, mat):
    x,y,z=loc
    profile=[(0,0),(.58*radius,1),(.88*radius,height*.07),(radius,height*.16),
             (radius,height*.84),(.88*radius,height*.93),(.58*radius,height*.99),(0,height)]
    verts=[];faces=[];segments=40
    # Single tip vertices avoid degenerate triangles at polar caps.
    verts.append((x,y,z))
    for r,h in profile[1:-1]:
        verts += [(x+r*math.cos(2*math.pi*k/segments),y+r*math.sin(2*math.pi*k/segments),z+h) for k in range(segments)]
    top=len(verts);verts.append((x,y,z+height))
    for k in range(segments):faces.append((0,1+(k+1)%segments,1+k))
    for j in range(len(profile)-3):
        a=1+j*segments;b=a+segments
        for k in range(segments):faces.append((a+k,a+(k+1)%segments,b+(k+1)%segments,b+k))
    a=1+(len(profile)-3)*segments
    for k in range(segments):faces.append((a+k,a+(k+1)%segments,top))
    C.surface(name,verts,faces,mat,0,True)


def pressure_tank():
    C.begin('PressureTank',(80,80,190),REF)
    for x in (-23,23):
        for y in (-23,23):
            C.box(f'Leg{x}_{y}',(x,y,20),(9,9,40),'Steel',.8)
            C.box(f'Foot{x}_{y}',(x,y,3),(19,19,6),'Iron',.5)
    tank_shell('ExpansionTank',(0,0,23),36,153,'Ivory')
    for z in (53,143):C.ring(f'ReinforcementBand{z}',(0,0,z),36.1,1.4,'Steel')
    C.cylinder('TopNeck',(0,0,178),8,15,'Steel',vertices=24,bevel=.7)
    C.cylinder('TopCap',(0,0,187),13,6,'Iron',vertices=24,bevel=.7)
    C.pipe('DrainOutlet',[(0,0,28),(0,-24,15),(0,-36,15)],4,'Blue')
    valve('DrainValve',(0,-34,21),7)
    C.box('GaugeBracket',(0,-34,125),(23,7,30),'Teal',1)
    gauge('TankGauge',0,-41,127,9)
    C.box('TankPlate',(0,-36,99),(20,1,13),'Panel',.4)
    C.box('TankPlateLine',(0,-37,99),(13,.5,2),'Ivory',.05)
    C.collision((0,0,99),(72,72,155))
    C.collision((0,0,13),(64,64,26))
    C.collision((0,0,182),(26,26,16))
    C.end()


def electrical_cabinet():
    C.begin('ElectricalCabinet',(110,45,200),REF)
    C.box('Plinth',(0,0,8),(108,44,16),'Iron',1)
    C.box('CabinetBody',(0,1,107),(108,41,182),'Ivory',2)
    C.box('TopCap',(0,1,198),(110,44,4),'Steel',.7)
    for x in (-26,26):
        C.box(f'Door{x}',(x,-21,107),(51,4,175),'Panel',.8)
        C.box(f'DoorHandle{x}',(x+18,-25,103),(3,5,19),'Iron',.6)
        for z in (35,172):C.box(f'DoorHinge{x}_{z}',(x-23,-23,z),(3,3,9),'Steel',.3)
    control_panel('ElectricalStatus',(-25,-24,155),(37,4,42))
    C.box('MainBreakerPlate',(25,-24,144),(32,3,45),'Iron',.8)
    C.cylinder('BreakerPivot',(25,-27,145),10,4,'Red',axis=(0,1,0),vertices=20,bevel=.4)
    C.box('BreakerHandle',(25,-31,150),(5,5,22),'Iron',.6)
    C.box('ElectricalHazard',(24,-25,104),(25,1,22),'Hazard',.3)
    for x in (-26,26):
        for k in range(6):C.box(f'BottomVent{x}_{k}',(x,-24,34+k*4),(36,1,1.8),'Iron',.1)
    C.collision((0,0,100),(110,45,200))
    C.end()


def crate(prefix, loc, size, mat='Teal'):
    x,y,z=loc;sx,sy,sz=size
    C.box(prefix+'_Case',(x,y,z),(sx,sy,sz),mat,1.2)
    C.box(prefix+'_Lid',(x,y,z+sz/2),(sx+1,sy+1,4),'Iron',.6)
    for xx in (x-sx*.36,x+sx*.36):
        C.box(prefix+f'_Strap{xx}',(xx,y-sy/2-.4,z),(3,1,sz-2),'Steel',.2)
    C.box(prefix+'_Label',(x,y-sy/2-.8,z+sz*.12),(sx*.45,.7,sz*.25),'Paper',.15)
    C.box(prefix+'_Handle',(x,y-sy/2-1.5,z-sz*.15),(sx*.27,2.5,3),'Iron',.5)


def supply_rack():
    C.begin('SupplyRack',(180,70,210),REF)
    for x in (-85,85):
        for y in (-29,29):
            C.box(f'Upright{x}_{y}',(x,y,104),(7,7,208),'Teal',.7)
            C.box(f'Foot{x}_{y}',(x,y,2),(13,13,4),'Iron',.4)
            C.collision((x,y,104),(7,7,208))
        C.box(f'CrossBrace{x}',(x,0,148),(5,62,5),'Steel',.5)
    for z in (13,76,139,206):
        C.box(f'Shelf{z}',(0,0,z),(175,66,5),'Steel',.6)
        C.box(f'ShelfFrontLip{z}',(0,-32,z-4),(176,3,10),'Teal',.4)
        C.collision((0,0,z-2),(180,70,11))
    for i,x in enumerate((-55,0,55)):
        crate(f'LowBin{i}',(x,0,37),(45,49,39),'Teal' if i!=1 else 'Ivory')
    for i,x in enumerate((-51,40)):
        crate(f'MidCrate{i}',(x,4,98),(69,49,35),'Iron' if i==0 else 'Teal')
    for x in (-60,-20,20):
        C.box(f'Carton{x}',(x,4,165),(33,47,44),'Wood',1)
        C.box(f'CartonTape{x}',(x,-20,165),(6,1,43),'Paper',.2)
    C.cylinder('RolledCable',(63,3,164),18,36,'Rubber',axis=(0,1,0),vertices=24,bevel=1)
    C.cylinder('CableCore',(63,-16,164),7,2,'Wood',axis=(0,1,0),vertices=20,bevel=.3)
    C.box('RackBayLabel',(0,-36,200),(45,1,8),'Paper',.2)
    C.collision((0,0,38),(158,54,42))
    C.collision((-5,4,100),(163,54,40))
    C.collision((0,4,166),(163,54,47))
    C.end()


def pallet_crates():
    C.begin('PalletCrates',(120,100,105),REF)
    for x in (-46,0,46):
        C.box(f'PalletRunner{x}',(x,0,6),(15,96,12),'Wood',.5)
    for y in (-40,-20,0,20,40):
        C.box(f'PalletDeck{y}',(0,y,14),(120,16,5),'Wood',.5)
    crate('LargeCrate',(0,5,46),(102,78,54),'Teal')
    crate('TopCrateA',(-27,2,88),(43,57,26),'Ivory')
    crate('TopCrateB',(27,10,87),(44,46,27),'Iron')
    C.collision((0,0,8),(120,100,16))
    C.collision((0,5,46),(105,82,58))
    C.collision((0,7,88),(106,65,33))
    C.end()


def pipe_bundle():
    C.begin('PipeBundle',(180,65,60),REF)
    for x in (-60,60):
        C.box(f'WoodCradle{x}',(x,0,7),(16,65,14),'Wood',.7)
        C.box(f'CradleStopA{x}',(x,-29,18),(16,8,23),'Wood',.5)
        C.box(f'CradleStopB{x}',(x,29,18),(16,8,23),'Wood',.5)
    for idx,(y,z) in enumerate(((-19,23),(0,23),(19,23),(-10,41),(10,41))):
        C.cylinder(f'Pipe{idx}',(0,y,z),9.3,180,'Steel',axis=(1,0,0),vertices=20,bevel=.4)
        for x in (-90.2,90.2):
            C.cylinder(f'OpenPipeShadow{idx}_{x}',(x,y,z),7.9,.6,'Iron',axis=(1,0,0),vertices=20,bevel=0)
            C.ring(f'OpenPipeRim{idx}_{x}',(x,y,z),8.5,.9,'Steel',axis=(1,0,0))
    for x in (-52,52):
        C.box(f'BundleBandTop{x}',(x,0,52),(4,49,3),'Iron',.3)
        for y in (-25,25):C.box(f'BundleBandSide{x}_{y}',(x,y,33),(4,3,37),'Iron',.3)
    C.collision((0,0,33),(180,59,43))
    C.collision((0,0,8),(140,65,16))
    C.end()


def pipe_run():
    C.begin('PipeRun',(200,25,90),REF)
    for z in (20,72):
        C.cylinder(f'ServicePipe{z}',(0,0,z),5.5,195,'Blue' if z==20 else 'Steel',axis=(1,0,0),vertices=24,bevel=.4)
        for x in (-70,70):
            C.ring(f'PipeClamp{x}_{z}',(x,0,z),6.2,1.1,'Iron',axis=(1,0,0))
            C.box(f'WallMount{x}_{z}',(x,8,z),(16,6,19),'Iron',.7)
            C.box(f'ClampSpacer{x}_{z}',(x,4,z),(6,9,8),'Steel',.4)
        for x in (-95,95):
            C.cylinder(f'Coupling{x}_{z}',(x,0,z),9,7,'Steel',axis=(1,0,0),vertices=24,bevel=.4)
        C.collision((0,1,z),(200,23,19))
    C.cylinder('ValveBody',(0,0,20),10,21,'Steel',axis=(1,0,0),vertices=24,bevel=.8)
    valve('SupplyShutoff',(0,-7,22),15)
    C.box('ServiceTag',(44,-6,49),(22,2,17),'Hazard',.5)
    C.pipe('GaugeStem',[(47,0,72),(47,0,81),(47,-6,83)],2,'Steel')
    gauge('LineGauge',47,-8,82,7)
    C.collision((0,-8,22),(36,18,35))
    C.collision((47,-5,80),(20,20,20))
    C.end()


def verify_fbx():
    """Read exports into clean Blender scenes; make no asset edits."""
    import bpy
    from mathutils import Vector
    manifest=json.loads((C.OUT/'Manifests/Utilities.json').read_text(encoding='utf-8'))
    results=[]
    for entry in manifest['assets']:
        bpy.ops.wm.read_factory_settings(use_empty=True)
        bpy.ops.import_scene.fbx(filepath=str(C.OUT/'Models'/(entry['name']+'.fbx')))
        meshes=[o for o in bpy.context.scene.objects if o.type=='MESH' and not o.name.startswith('UCX_')]
        collisions=[o for o in bpy.context.scene.objects if o.type=='MESH' and o.name.startswith('UCX_')]
        assert len(meshes)==1,(entry['name'],'render mesh count',len(meshes))
        mesh=meshes[0];mesh.data.calc_loop_triangles()
        size=[float(v)*100 for v in mesh.dimensions]
        assert all(abs(size[i]-entry['size_cm'][i])<.1 for i in range(3)),(entry['name'],size)
        assert len(mesh.data.uv_layers)==2,(entry['name'],'UV count')
        assert len(mesh.data.materials)==len(entry['materials']),(entry['name'],'material count')
        assert len(collisions)==entry['collision_boxes'],(entry['name'],'collision count')
        assert all(len(c.data.vertices)==8 for c in collisions),(entry['name'],'non-box collision')
        assert len(mesh.data.loop_triangles)==entry['triangles'],(entry['name'],'triangle count')
        assert all(math.isfinite(a) for v in mesh.data.vertices for a in v.co),(entry['name'],'nonfinite geometry')
        assert all(t.area>1e-13 for t in mesh.data.loop_triangles),(entry['name'],'degenerate geometry')
        coords=[mesh.matrix_world@v.co for v in mesh.data.vertices]
        bottom=min(v.z for v in coords)*100
        assert abs(bottom)<.1,(entry['name'],'floor pivot',bottom)
        results.append({'name':entry['name'],'size_cm':size,'triangles':len(mesh.data.loop_triangles),'collision_boxes':len(collisions),'uv_channels':2})
    print('FACILITY_UTILITIES_FBX_VALIDATED '+json.dumps({'passed':True,'assets':results,'total_triangles':manifest['total_triangles']}))


def build():
    C.init('Utilities')
    boiler()
    generator()
    pump_skid()
    pressure_tank()
    electrical_cabinet()
    supply_rack()
    pallet_crates()
    pipe_bundle()
    pipe_run()
    C.complete()


if __name__=='__main__':
    if '--verify' in sys.argv:
        verify_fbx()
    else:
        build()
