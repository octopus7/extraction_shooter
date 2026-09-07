"""Deterministic placements for the two independent facility levels (UE cm)."""
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / 'TunaSweeper/SourceArt/Environment/FacilityRooms'
levels = []

def room(name, size, floor_z):
    level = {'name': name, 'size_cm': size, 'floor_z_cm': floor_z,
             'placements': [], 'player_start': [0, 0, floor_z + 100],
             'walkway_checks': [], 'no_ceiling': True}
    levels.append(level)
    return level

def add(level, mesh, xyz, yaw=0, zone='Architecture'):
    index = len(level['placements'])
    level['placements'].append({'label': f'{zone}_{mesh}_{index:03d}',
        'mesh': 'SM_FacilityRooms_' + mesh, 'location_cm': list(xyz),
        'yaw_deg': yaw, 'zone': zone})

def perimeter(level, width, depth, z, wall='Wall200'):
    for x in range(-width//2+100, width//2, 200):
        for y in (-depth/2, depth/2):
            add(level, wall, (x,y,z))
    for y in range(-depth//2+100, depth//2, 200):
        for x in (-width/2, width/2):
            add(level, wall, (x,y,z),90)

basement=room('L_Facility_Basement', [1200,1000], 0)
for x in range(-500,600,200):
    for y in range(-400,500,200):
        add(basement,'Floor200',(x,y,-20))
perimeter(basement,1200,1000,0)
# 4 x 4 m southwest ladder room; open doorway to the utility hall.
add(basement,'Doorway200',(-300,-100,0),zone='LadderRoom')
add(basement,'Wall200',(-500,-100,0),zone='LadderRoom')
for y in (-400,-200):add(basement,'Wall200',(-200,y,0),90,'LadderRoom')
add(basement,'Threshold120',(-300,-100,0),zone='LadderRoom')
add(basement,'Ladder300',(-470,-355,0),zone='LadderRoom')
add(basement,'HatchLanding200',(-470,-330,300),zone='LadderRoom')
add(basement,'Guardrail200',(-470,-425,320),zone='LadderRoom')
add(basement,'Boiler',(-385,290,0),0,'BoilerBay')
add(basement,'Generator',(-375,130,0),0,'GeneratorBay')
add(basement,'ElectricalCabinet',(-525,105,0),90,'GeneratorBay')
add(basement,'PumpSkid',(380,300,0),0,'PumpBay')
add(basement,'PressureTank',(500,110,0),0,'PumpBay')
add(basement,'PipeRun',(360,460,30),0,'PumpBay')
add(basement,'SupplyRack',(390,-425,0),0,'MaterialsStorage')
add(basement,'SupplyRack',(545,-160,0),-90,'MaterialsStorage')
add(basement,'PalletCrates',(285,-220,0),0,'MaterialsStorage')
add(basement,'PipeBundle',(390,-70,0),0,'MaterialsStorage')
for x,y,yaw in [(-590,300,90),(590,300,-90),(0,490,0),(-470,-490,180)]:
    add(basement,'WallLamp',(x,y,240),yaw,'Lighting')
add(basement,'WallVent',(-380,484,170),0,'BoilerBay')
basement['walkway_checks']=[[-300,-180,-300,-20],[0,-350,0,350],[-100,170,100,170]]
basement['camera']={'location_cm':[1700,-2200,2000],'target_cm':[0,20,40]}

control=room('L_Facility_ControlFloor02',[600,600],320)
for x in (-200,0,200):
    for y in (-200,0,200):
        add(control,'WoodHatchLanding200' if (x,y)==(-200,-200) else 'WoodFloor200',(x,y,300))
perimeter(control,600,600,320,'WoodWall200')
for placement in control['placements']:
    if placement['location_cm']==[-300,0,320]:
        placement['mesh']='SM_FacilityRooms_WoodWindowWall200'
        placement['label']='AtticTimber_WoodWindowWall200'
add(control,'WoodGable600',(0,-300,540),zone='AtticTimber')
add(control,'WoodGable600',(0,300,540),zone='AtticTimber')
add(control,'WoodRoof600',(0,0,540),zone='AtticRoof')
control['placements'][-1]['review_hidden']=True
control['no_ceiling']=False
add(control,'Ladder300',(-200,-225,0),zone='LadderLanding')
add(control,'Guardrail200',(-295,-200,320),90,'LadderLanding')
add(control,'Guardrail200',(-200,-295,320),0,'LadderLanding')
add(control,'OperatorDesk',(-120,110,320),180,'Operations')
add(control,'OperatorDesk',(100,110,320),180,'Operations')
add(control,'DeskMonitor',(-120,125,398),180,'Operations')
add(control,'DeskMonitor',(65,135,398),180,'Operations')
add(control,'KeyboardConsole',(-120,80,399),180,'Operations')
add(control,'KeyboardConsole',(65,80,399),180,'Operations')
add(control,'OperatorChair',(-120,10,320),180,'Operations')
add(control,'OperatorChair',(100,10,320),180,'Operations')
add(control,'MonitorWall',(0,265,420),180,'SituationDisplays')
add(control,'RadioRack',(250,-160,320),-90,'Communications')
add(control,'RadioConsole',(150,115,398),180,'Communications')
add(control,'FileCabinet',(-265,130,320),90,'Operations')
add(control,'AtticLamp',(-185,130,398),zone='WarmLighting')
add(control,'AtticLamp',(235,185,450),zone='WarmLighting')
add(control,'BookStack',(-170,85,398),zone='MoleLiving')
add(control,'DeskMug',(-65,85,398),zone='MoleLiving')
control['player_start']=[0,-140,420]
control['walkway_checks']=[[-100,-200,100,-200],[0,-220,0,30]]
control['camera']={'location_cm':[1050,-1350,2350],'target_cm':[0,0,370]}
control['interior_camera']={'location_cm':[210,-215,465],'target_cm':[-190,100,475],'fov':82}
control['forest']={'label':'Exterior_ForestImpostor','location_cm':[-550,0,490],
                   'rotation_deg':[0,90,-90],'scale':[8,6,1]}
control['window_light']={'location_cm':[-470,-70,540],'target_cm':[170,190,340],
                         'intensity_candela':1800,'inner_cone_deg':15,'outer_cone_deg':21,
                         'volumetric_scattering_intensity':4,'fog_density':.02,
                         'fog_extinction_scale':1.5,'fog_view_distance_cm':2000}

if __name__=='__main__':
    SOURCE.mkdir(parents=True,exist_ok=True)
    (SOURCE/'level_layout.json').write_text(json.dumps({'levels':levels},indent=2),encoding='utf-8')
    print('FACILITY_LAYOUT', [(l['name'],len(l['placements'])) for l in levels])
