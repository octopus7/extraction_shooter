"""Blender 4.5 expansion. Reuses the committed six-module source as the assembly SSOT."""
import bpy,bmesh,json,math,os,ast
from pathlib import Path
from mathutils import Vector,Matrix
from collections import Counter
ROOT=Path(__file__).resolve().parents[2]
BASE=ROOT/'TunaSweeper/SourceArt/Environment/ModularInteriorPreview'
OUT=ROOT/'TunaSweeper/SourceArt/Environment/ModularInteriorExpansion'
source=(ROOT/'Tools/ModularInteriorPreview/build_interior.py').read_text()
bpy.ops.wm.open_mainfile(filepath=str(BASE/'ModularInteriorPreview.blend'))
scene=bpy.context.scene
scene.cycles.samples=24
base_manifest=json.loads((BASE/'model_manifest.json').read_text())
base_assets={e['key']:bpy.data.objects[e['name']] for e in base_manifest['assets']}
mats={k:bpy.data.materials['M_MI_'+k] for k in ('Concrete','Floor','Steel','Door','LED')}
image=bpy.data.images.load(str(OUT/'Textures/Expansion_Typography.png'))
image.scale(2048,2048);image.filepath_raw=str(OUT/'Textures/T_MIE_ServiceAtlas.png');image.file_format='PNG';image.save();image.pack();image.filepath='//Textures/T_MIE_ServiceAtlas.png'
service=mats['Steel'].copy();service.name='M_MI_ExpansionSurface'
for n in service.node_tree.nodes:
    if n.type=='TEX_IMAGE' and n.image and n.image.colorspace_settings.name!='Non-Color':n.image=image
service.node_tree.nodes['Tint'].inputs[2].default_value=(.9,.95,1,1)
mats['ExpansionSurface']=service
glass=bpy.data.materials.new('M_MI_ExpansionGlass');glass.use_nodes=True
b=glass.node_tree.nodes.get('Principled BSDF');b.inputs['Base Color'].default_value=(.15,.32,.36,1);b.inputs['Roughness'].default_value=.22;b.inputs['Alpha'].default_value=.22
glass.surface_render_method='DITHERED';mats['ExpansionGlass']=glass
amber=mats['LED'].copy();amber.name='M_MI_ExpansionAmber'
b=amber.node_tree.nodes.get('Principled BSDF');b.inputs['Base Color'].default_value=(1,.19,.025,1);b.inputs['Emission Color'].default_value=(1,.19,.025,1);b.inputs['Emission Strength'].default_value=3
mats['ExpansionAmber']=amber
templates=bpy.data.collections.new('Expansion 18 source modules');scene.collection.children.link(templates)
sample=bpy.data.collections['Sample assembly']
assets={};collision={};entries=[]
# Load only the established closed-prism/box functions; never execute the base generator.
tree=ast.parse(source)
for n in tree.body:
    if isinstance(n,ast.FunctionDef) and n.name in ('mesh','prism','box','area','camera'):
        exec(compile(ast.Module(body=[n],type_ignores=[]),str(ROOT/'Tools/ModularInteriorPreview/build_interior.py'),'exec'))

def grid(name,xs,zs,cells,depth,keys):
    verts=[];faces=[];indices=[];lookup={}
    def vi(p):
        if p not in lookup:lookup[p]=len(verts);verts.append(p)
        return lookup[p]
    for (i,j),mi in cells.items():
        x0,x1=xs[i:i+2];z0,z1=zs[j:j+2]
        for y in depth:
            faces.append(tuple(vi((x,y,z)) for x,z in [(x0,z0),(x1,z0),(x1,z1),(x0,z1)]));indices.append(mi)
        for neighbor,ends in [((i-1,j),[(x0,z0),(x0,z1)]),((i+1,j),[(x1,z0),(x1,z1)]),((i,j-1),[(x0,z0),(x1,z0)]),((i,j+1),[(x0,z1),(x1,z1)])]:
            if neighbor not in cells:
                (a,b),(c,d)=ends;faces.append(tuple(vi(p) for p in [(a,depth[0],b),(c,depth[0],d),(c,depth[1],d),(a,depth[1],b)]));indices.append(mi)
    return mesh(name,verts,faces,keys,indices)

box('FloorHalf',(0,0,-.2),(1,2,0),'Floor')
o=box('DrainFloor',(0,0,-.2),(2,2,0),'ExpansionSurface')
box('FloorEdge',(0,-.05,-.2),(2,0,0),'Steel')
box('WallHalf',(0,0,0),(1,.2,3),'Concrete')
grid('WindowWall',[0,.5,1.5,2],[0,1,2.5,3],{(i,j):0 for i in range(3) for j in range(3) if (i,j)!=(1,1)},(0,.2),('Concrete',))
collision['WindowWall']=[((0,0,0),(.5,.2,3)),((1.5,0,0),(2,.2,3)),((.5,0,0),(1.5,.2,1)),((.5,0,2.5),(1.5,.2,3))]
box('LowPartition',(0,0,0),(2,.2,1),'Concrete')
prism('OutsideCorner',[(0,0),(1.2,0),(1.2,.2),(.2,.2),(.2,1.2),(0,1.2)],(0,3))
collision['OutsideCorner']=[((0,0,0),(1.2,.2,3)),((0,.2,0),(.2,1.2,3))]
box('WallEnd',(0,0,0),(.05,.2,3),'Steel')
# Exact integrated-frame cells from the base Doorway (10cm wide, flush 20cm deep).
grid('DoorFrame',[.9,1,3,3.1],[0,2.4,2.5],{(i,j):0 for i in range(3) for j in range(2) if (i,j)!=(1,0)},(0,.2),('Steel',))
collision['DoorFrame']=[((.9,0,0),(1,.2,2.4)),((3,0,0),(3.1,.2,2.4)),((.9,0,2.4),(3.1,.2,2.5))]
# Inset frame is 2mm clear of the window aperture; a thin pane has its own slot.
o=grid('ObservationWindow',[.502,.552,1.448,1.498],[1.002,1.052,2.448,2.498],{(i,j):0 for i in range(3) for j in range(3) if (i,j)!=(1,1)},(.02,.18),('Steel','ExpansionGlass'))
pane=box('_Pane',(.552,.09,1.052),(1.448,.11,2.448),'ExpansionGlass')
bm=bmesh.new();bm.from_mesh(o.data);tmp=bmesh.new();tmp.from_mesh(pane.data)
# Join using Blender to keep the two closed components and material assignment.
bpy.ops.object.select_all(action='DESELECT');o.select_set(True);pane.select_set(True);bpy.context.view_layer.objects.active=o;bpy.ops.object.join();bm.free();tmp.free()
assets.pop('_Pane');collision.pop('_Pane');collision['ObservationWindow']=[((.502,.02,1.002),(1.498,.18,2.498))]

def pipe(name,centers,tangents):
    verts=[];faces=[];radius=.1;N=8
    for c,t in zip(centers,tangents):
        side=Vector((0,1,0));up=Vector(t).cross(side).normalized()
        for i in range(N):verts.append(Vector(c)+radius*(math.cos(i*math.tau/N)*side+math.sin(i*math.tau/N)*up))
    faces.append(tuple(reversed(range(N))));faces.append(tuple(range((len(centers)-1)*N,len(centers)*N)))
    for j in range(len(centers)-1):
        for i in range(N):faces.append((j*N+i,j*N+(i+1)%N,(j+1)*N+(i+1)%N,(j+1)*N+i))
    mesh(name,verts,faces,('Steel',));collision[name]=[]
pipe('PipeStraight',[(0,-.1,0),(2,-.1,0)],[(1,0,0)]*2)
ts=[i*math.pi/8 for i in range(5)]
pipe('PipeElbow',[(.5*math.sin(t),-.1,.5*(1-math.cos(t))) for t in ts],[(math.cos(t),0,math.sin(t)) for t in ts])
pipe('PipeEnd',[(0,-.1,0),(.05,-.1,0)],[(1,0,0)]*2)
box('DuctStraight',(0,-.4,0),(2,0,.4),'Steel');collision['DuctStraight']=[]
prism('DuctCorner',[(0,0),(.5,0),(.5,.5),(.1,.5),(.1,.4),(0,.4)],(-.4,0),plane='XZ',keys=('Steel',));collision['DuctCorner']=[]
box('Vent',(0,-.08,0),(1,0,.5),'ExpansionSurface');collision['Vent']=[]
o=box('EmergencyLight',(0,-.12,0),(.4,0,.2),'Steel');o.data.materials.append(amber)
for p in o.data.polygons:
    if p.normal.y<-.9 or p.normal.z>.9:p.material_index=1
collision['EmergencyLight']=[]
box('ZoneSign',(0,-.02,0),(1,0,.5),'ExpansionSurface');collision['ZoneSign']=[]

# Match the base planar UV0 and 4m DirtUV contract and identical handedness export.
regions={'Concrete':(.012,.512,.488,.988),'Floor':(.008,.008,.492,.492),'Steel':(.508,.508,.992,.992),'Door':(.508,.008,.992,.492),'LED':(.65,.65,.8,.8),'ExpansionSurface':(.508,.008,.992,.492),'ExpansionGlass':(.1,.1,.9,.9),'ExpansionAmber':(.1,.1,.9,.9)}
for name,o in assets.items():o.name='SM_MIE_'+name
export=source[source.index('for name,o in assets.items():'):source.index('\nplacements=[]')]
# Service UV regions are module/face specific but remain one material and one atlas.
export=export.replace('u0,v0,u1,v1=regions[key]',"u0,v0,u1,v1=regions[key]\n        if key=='ExpansionSurface':\n            if name=='DrainFloor' and p.normal.z>.9:u0,v0,u1,v1=(.012,.512,.488,.988)\n            elif name=='Vent' and abs(p.normal.y)>.9:u0,v0,u1,v1=(.512,.512,.988,.988)\n            elif name=='ZoneSign' and abs(p.normal.y)>.9:u0,v0,u1,v1=(.012,.012,.488,.488)")
exec(compile(export,__file__,'exec'))
assert len(assets)==18
assets.update(base_assets)
placements=list(base_manifest['placements'])
place_node=next(n for n in tree.body if isinstance(n,ast.FunctionDef) and n.name=='place')
exec(compile(ast.Module(body=[place_node],type_ignores=[]),__file__,'exec'))
# 6x6 annex: structural surfaces meet, decorative equipment attaches to finish faces.
for x in (12,14,16):
    for y in (0,2,4):
        if (x,y)==(14,2):place('DrainFloor',(x,y,0))
        elif (x,y)==(12,0):
            place('FloorHalf',(x,y,0));place('FloorHalf',(x+1,y,0))
        else:place('Floor',(x,y,0))
for x in (12,13,16,17):place('WallHalf',(x,6,0))
place('WindowWall',(14,6,0));place('ObservationWindow',(14,6,0))
for y in (0,2,4):place('Wall',(18,y+2,0),-math.pi/2)
# Replacement frame sample: surrounding concrete stops at frame's OUTER boundary.
place('Wall',(12,0,0),math.pi,scale=(.45,1,1),label='DoorAdapter_Left')
# Door coordinates run +X; wall thickness +Y. South indoor is +Y, so place the portal at y=-.2.
bpy.data.objects['DoorAdapter_Left'].location=(12.9,0,0)
placements[-1]['location_m']=[12.9,0,0]
place('Wall',(18,0,0),math.pi,scale=(1.45,1,1),label='DoorAdapter_Right')
place('Wall',(12.9,-.2,2.5),scale=(1.1,1,1/6),label='DoorAdapter_Header')
place('DoorFrame',(12,-.2,0))
place('DoorLeaf',(13.02,-.14,.02),math.radians(-68),label='ExpansionDoorLeaf_Open')
place('LowPartition',(12,2,0));place('WallEnd',(14,2,0),scale=(1,1,1/3))
place('OutsideCorner',(12,4.8,0));place('WallHalf',(12.2,3.8,0),math.pi/2)
place('WallEnd',(12.2,3.75,0),math.pi/2)
for x in (12,14,16):place('FloorEdge',(x,0,0))
place('PipeStraight',(13.5,6,.5),label='Pipe_Run')
# Local pipe runs +X then turns +Z. Rear surface touches the wall finish plane.
place('PipeElbow',(15.5,6,.5),label='Pipe_Turn')
place('PipeEnd',(13.45,6,.5),label='Pipe_Cap')
place('DuctStraight',(14,6,2.5),label='Duct_Run')
place('DuctCorner',(16,6,2.5),label='Duct_Turn')
place('Vent',(16.8,6,1.7));place('EmergencyLight',(12.3,6,2.5));place('ZoneSign',(16.7,6,1))
place('LightBar',(18,4.5,2.4),-math.pi/2)
# Typography is separate review text; baked accurately into the atlas by prepare_typography.ps1.
fixtures=list(base_manifest['fixtures'])
fixtures.append({'location_m':[17.9,3.75,2.46],'target_m':[15,3.75,.8]})
area('ExpansionWallFixture',fixtures[-1]['location_m'],160,1.1,target=fixtures[-1]['target_m'])
area('ExpansionOverviewSoftbox',(14,1,10),1800,8,target=(15,3,0))
overview=camera('ExpansionOverview',(-13,-23,27),(9,6,.6),ortho=31)
annex=camera('ExpansionAnnex',(7,-7,9),(15,3,1),ortho=11)
play=bpy.data.objects['PlayCamera_NativeTopDown'].copy();play.data=play.data.copy();scene.collection.objects.link(play);play.name='ExpansionPlayCamera';play.location+=Vector((12,-.3,0))
close=camera('ExpansionWindowServices',(12,1,3.9),(15.7,6,1.85),lens=42)
doorcam=camera('ExpansionDoorClose',(16,3.5,2.5),(13.8,0,1.4),lens=35)
cornercam=camera('ExpansionCornerClose',(15.5,2.5,5.5),(12.55,4.8,1.5),lens=42)
allentries=base_manifest['assets']+entries
counts=Counter(p['key'] for p in placements)
manifest={**base_manifest,'assets':entries,'base_assets':base_manifest['assets'],'placements':placements,'counts':dict(counts),'fixtures':fixtures,'unique_triangles':sum(e['triangles'] for e in entries),'combined_unique_triangles':sum(e['triangles'] for e in allentries),'sample_instances':len(placements),'sample_triangles':sum(counts[e['key']]*e['triangles'] for e in allentries),'real_lights':len(fixtures),'preview_only_softboxes':2,'texture':{'base_atlas':[2048,2048],'shared_dirt':[1024,1024],'service_atlas':[2048,2048],'new_runtime_textures':1},'cameras':[]}
for cam in (overview,annex,play,close,doorcam,cornercam):
    manifest['cameras'].append({'name':cam.name,'location_m':list(cam.location),'rotation_rad':list(cam.rotation_euler),'fov':math.degrees(cam.data.angle_x),'ortho_m':cam.data.ortho_scale if cam.data.type=='ORTHO' else None})
(OUT/'model_manifest.json').write_text(json.dumps(manifest,indent=2))
flip=next(n for n in scene.node_tree.nodes if n.type=='FLIP')
def render(name,cam):
    if os.environ.get('MIE_RENDER_FILTER') and name not in os.environ['MIE_RENDER_FILTER'].split(','):return
    scene.camera=cam;flip.mute=cam!=play;scene.render.filepath=str(OUT/'Previews'/f'{name}.png');bpy.ops.render.render(write_still=True)
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'ModularInteriorExpansion.blend'))
if not os.environ.get('MIE_SKIP_RENDER'):
    for name,cam in [('Overview',overview),('Annex',annex),('PlayCamera',play),('Window_Services',close),('Door_Frame',doorcam),('Outside_Corner',cornercam)]:render(name,cam)
    for label,strength in [('Clean',0),('Weak',.65),('Strong',1.8)]:
        for mat in mats.values():
            n=mat.node_tree.nodes.get('DirtStrength') if mat.use_nodes else None
            if n:n.inputs[1].default_value=strength
        render('Dirt_'+label,annex)
    for mat in mats.values():
        n=mat.node_tree.nodes.get('DirtStrength') if mat.use_nodes else None
        if n:n.inputs[1].default_value=.65
    # Individual module renders with common neutral studio; labels are in the HTML contact sheet.
    visible=[o for o in scene.objects if o.type=='MESH' and not o.hide_render]
    for o in visible:o.hide_render=True
    for e in entries:
        o=assets[e['key']];o.hide_render=False;o.hide_set(False)
        center=Vector([(e['bounds_m'][i]+e['bounds_m'][i+3])/2 for i in range(3)])
        span=max(e['bounds_m'][i+3]-e['bounds_m'][i] for i in range(3))
        cam=camera('Sheet_'+e['key'],center+Vector((span*.7,-span*1.4,span*.9)),center,ortho=max(.6,span*1.45))
        lamp=area('SheetSoftbox',center+Vector((-2,-4,6)),600,5,target=center)
        render('Module_'+e['key'],cam);o.hide_render=True;o.hide_set(True);bpy.data.objects.remove(lamp,do_unlink=True)
    for o in visible:o.hide_render=False
scene.camera=play;flip.mute=False
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'ModularInteriorExpansion.blend'))
print('EXPANSION_BUILD_PASSED',manifest['unique_triangles'],manifest['sample_triangles'])


