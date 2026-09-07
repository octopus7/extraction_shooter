"""Reference-led V2 layout in UE centimetres, independent from previous maps."""
import json
from pathlib import Path
SOURCE=Path(__file__).resolve().parents[2]/'TunaSweeper/SourceArt/Environment/MoleControlV2'
level={'name':'L_MoleControlV2_StoneVault','size_cm':[600,600],'floor_z_cm':320,'no_ceiling':False,
       'placements':[],'player_start':[0,-210,420],
       'walkway_checks':[[0,-210,0,85],[-160,-180,120,-180]],
       'camera':{'location_cm':[1150,-1400,1750],'target_cm':[0,0,405]},
       'interior_camera':{'location_cm':[240,-245,475],'target_cm':[-35,120,470],'fov':86},
       'forest':{'label':'Exterior_ForestImpostor','location_cm':[-550,0,480],'rotation_deg':[0,90,-90],'scale':[8,6,1]},
       'window_light':{'location_cm':[-470,-50,565],'target_cm':[100,160,370],'intensity_candela':350,
                       'inner_cone_deg':15,'outer_cone_deg':22,'volumetric_scattering_intensity':8,
                       'fog_density':.03,'fog_extinction_scale':1,'fog_view_distance_cm':2000}}
def add(mesh,xyz,yaw=0,zone='Architecture',**extra):
    p={'label':f'{zone}_{mesh}_{len(level["placements"]):03d}','mesh':'SM_MoleControlV2_'+mesh,
       'location_cm':list(xyz),'yaw_deg':yaw,'zone':zone,**extra};level['placements'].append(p);return p
for x in (-200,0,200):
    for y in (-200,0,200):add('OakHatch200' if (x,y)==(200,-200) else 'OakFloor200',(x,y,300))
for x in (-200,0,200):
    for y in (-300,300):add('StoneWall200',(x,y,320))
for y in (-200,0,200):
    add('StoneWindowWall200' if y==0 else 'ArchedDoorway200' if y==-200 else 'StoneWall200',(-300,y,320),90)
    add('StoneWall200',(300,y,320),90)
add('ArchedWoodDoor',(-300,-200,320),-90,'Entry')
add('CurtainFrame',(-280,0,420),-90,'Window')
add('StoneVault600',(0,0,560),zone='VaultRoof',review_hidden=True)
for y in (-300,300):add('VaultEnd600',(0,y,560),zone='VaultEnd')
add('GrandMapConsole',(0,185,320),180,'Operations')
add('VintageRadioStack',(-215,30,320),180,'Communications')
add('ShelfReceiver',(85,224,529.44),180,'Communications')
for x in (-235,235):add('SideCabinet',(x,185,320),180,'Living')
add('RoseTableLamp',(245,190,405),zone='Practical')
add('RoseTableLamp',(-120,224,529.44),zone='Practical')
add('BookSet',(-235,183,405),zone='Living')
add('FlowerPot',(-235,183,437),zone='Living')
add('BookSet',(-40,225,529.44),zone='Living')
add('CeramicMug',(215,164,405),zone='Living')
add('FloralRug240',(0,-40,320.05),zone='Living')
for x in (-95,110):add('SwivelStool',(x,70,320),zone='Operations')
p=add('Ladder300',(200,-225,0),zone='Entry',asset_path='/Game/Environment/FacilityRooms/Meshes/SM_FacilityRooms_Ladder300')
p['mesh']='SM_FacilityRooms_Ladder300'
if __name__=='__main__':
    (SOURCE/'level_layout.json').write_text(json.dumps({'levels':[level]},indent=2),encoding='utf-8')
    print('MOLE_V2_LAYOUT',len(level['placements']))
