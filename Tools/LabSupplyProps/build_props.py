"""Reproducible Blender 4.5 source build. No UE asset writes. Meters, project axes."""
import bpy,bmesh,json,math,sys,os
from pathlib import Path
from mathutils import Vector,Matrix
ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'TunaSweeper/SourceArt/Environment/LabSupplyProps'
BASE=ROOT/'TunaSweeper/SourceArt/Environment/ModularInteriorPreview'
sys.path.insert(0,str(Path(__file__).parent/'parts'))
import furniture,lab_equipment,warehouse
specs=furniture.assets()+lab_equipment.assets()+warehouse.assets()
for d in ('Models','Previews','Textures'): (OUT/d).mkdir(parents=True,exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True)
s=bpy.context.scene;s.unit_settings.system='METRIC';s.unit_settings.scale_length=1
s.render.engine='CYCLES';s.cycles.samples=24;s.cycles.use_denoising=True
s.render.threads_mode='FIXED';s.render.threads=10
s.render.resolution_x=1600;s.render.resolution_y=1100;s.render.resolution_percentage=100
s.view_settings.view_transform='AgX'
s.world=bpy.data.worlds.new('Studio');s.world.use_nodes=True;s.world.node_tree.nodes['Background'].inputs[0].default_value=(.26,.29,.33,1);s.world.node_tree.nodes['Background'].inputs[1].default_value=.5
atlas=bpy.data.images.load(str(OUT/'Textures/ImageGen_Atlas_Final.png'));atlas.scale(2048,2048)
atlas.filepath_raw=str(OUT/'Textures/T_LSP_Atlas.png');atlas.file_format='PNG';atlas.save();atlas.pack();atlas.filepath='//Textures/T_LSP_Atlas.png'
mask=bpy.data.images.load(str(BASE/'Textures/T_MI_DirtMask.png'));mask.colorspace_settings.name='Non-Color';mask.pack()
mat=bpy.data.materials.new('M_LSP_Surface');mat.use_nodes=True
n=mat.node_tree.nodes;l=mat.node_tree.links;b=n.get('Principled BSDF')
tex=n.new('ShaderNodeTexImage');tex.image=atlas
atlasuv=n.new('ShaderNodeUVMap');atlasuv.uv_map='UVMap';l.new(atlasuv.outputs[0],tex.inputs[0])
uv=n.new('ShaderNodeUVMap');uv.uv_map='DirtUV'
info=n.new('ShaderNodeObjectInfo');sep=n.new('ShaderNodeSeparateColor');l.new(info.outputs['Color'],sep.inputs[0])
sc=n.new('ShaderNodeVectorMath');sc.operation='SCALE';sc.name='DirtScale';sc.inputs[3].default_value=1;l.new(uv.outputs[0],sc.inputs[0])
off=n.new('ShaderNodeCombineXYZ');l.new(sep.outputs[0],off.inputs[0]);l.new(sep.outputs[1],off.inputs[1])
add=n.new('ShaderNodeVectorMath');add.operation='ADD';l.new(sc.outputs[0],add.inputs[0]);l.new(off.outputs[0],add.inputs[1])
dirt=n.new('ShaderNodeTexImage');dirt.image=mask;l.new(add.outputs[0],dirt.inputs[0])
strength=n.new('ShaderNodeMath');strength.operation='MULTIPLY';strength.use_clamp=True;strength.name='DirtStrength';strength.inputs[1].default_value=.35;l.new(dirt.outputs[0],strength.inputs[0])
mix=n.new('ShaderNodeMixRGB');mix.inputs[2].default_value=(.12,.105,.075,1);l.new(strength.outputs[0],mix.inputs[0]);l.new(tex.outputs[0],mix.inputs[1]);l.new(mix.outputs[0],b.inputs['Base Color'])
attr=n.new('ShaderNodeVertexColor');attr.layer_name='SurfaceParams';surf=n.new('ShaderNodeSeparateColor');l.new(attr.outputs[0],surf.inputs[0])
for field,ch,val in [('Metallic',0,0),('Roughness',1,.94)]:
    mix=n.new('ShaderNodeMixRGB');mix.inputs[2].default_value=(val,val,val,1);l.new(strength.outputs[0],mix.inputs[0]);l.new(surf.outputs[ch],mix.inputs[1]);l.new(mix.outputs[0],b.inputs[field])
templates=bpy.data.collections.new('Source templates');s.collection.children.link(templates)
assets={};entries=[]
def part_geometry(p):
    if p['type']=='box':
        a,c=p['lo'],p['hi'];poly=[(a[0],a[1]),(c[0],a[1]),(c[0],c[1]),(a[0],c[1])];depth=[a[2],c[2]];plane='XY'
    elif p['type']=='prism':poly=p['polygon'];depth=p['depth'];plane=p['plane']
    else:
        r=p['radius'];ctr=p['center'];axis=p['axis'];axes={'X':(1,2),'Y':(0,2),'Z':(0,1)}[axis];di='XYZ'.index(axis)
        poly=[(ctr[axes[0]]+r*math.cos(i*2*math.pi/p['sides']),ctr[axes[1]]+r*math.sin(i*2*math.pi/p['sides'])) for i in range(p['sides'])]
        depth=[ctr[di]-p['depth']/2,ctr[di]+p['depth']/2];plane={'X':'YZ','Y':'XZ','Z':'XY'}[axis]
    def co(a,d):return {'XY':(a[0],a[1],d),'XZ':(a[0],d,a[1]),'YZ':(d,a[0],a[1])}[plane]
    count=len(poly);v=[co(a,d) for d in depth for a in poly]
    f=[tuple(reversed(range(count))),tuple(range(count,2*count))]+[(i,(i+1)%count,(i+1)%count+count,i+count) for i in range(count)]
    return v,f
def tile_params(tile):
    if tile in (3,15):return (.68,.43)
    if tile in (8,9,10,13,14):return (0,.82)
    return (.12,.64)
def build(e):
    verts=[];faces=[];properties=[]
    for p in e['parts']:
        v,f=part_geometry(p);offset=len(verts);verts+=v;faces +=[tuple(i+offset for i in face) for face in f];properties +=[p]*len(f)
    me=bpy.data.meshes.new(e['key']);me.from_pydata(verts,[],faces);me.update()
    bm=bmesh.new();bm.from_mesh(me);bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces));bm.to_mesh(me);bm.free()
    o=bpy.data.objects.new('SM_LSP_'+e['key'],me);templates.objects.link(o);me.materials.append(mat)
    uv=me.uv_layers.new(name='UVMap');duv=me.uv_layers.new(name='DirtUV');me.uv_layers.active_index=0;colors=me.color_attributes.new(name='SurfaceParams',type='FLOAT_COLOR',domain='CORNER');me.color_attributes.active_color=colors
    # Adding corner attributes reallocates Blender CustomData; reacquire RNA layer handles.
    uv=me.uv_layers['UVMap'];duv=me.uv_layers['DirtUV'];colors=me.color_attributes['SurfaceParams']
    for face,p in zip(me.polygons,properties):
        normal=face.normal;axis=max(range(3),key=lambda a:abs(normal[a]));side=[('left','right'),('front','back'),('bottom','top')][axis][normal[axis]>0]
        tile=p.get('face_tiles',{}).get(side,p['tile']);col=tile%4;row=tile//4
        axes=[a for a in range(3) if a!=axis];coords=[me.vertices[me.loops[i].vertex_index].co for i in face.loop_indices]
        ranges=[(min(c[a] for c in coords),max(c[a] for c in coords)) for a in axes]
        if e['key']=='Sink' and side=='front' and tile==4:ranges=[(-.775,.775),(.075,.86)]
        metal,rough=tile_params(tile)
        for i,c in zip(face.loop_indices,coords):
            u=(c[axes[0]]-ranges[0][0])/max(1e-9,ranges[0][1]-ranges[0][0]);v=(c[axes[1]]-ranges[1][0])/max(1e-9,ranges[1][1]-ranges[1][0])
            # View-facing horizontal direction and top orientation. Atlas art remains readable from front.
            if side in ('back','left'):u=1-u
            uv.data[i].uv=((col+.025+.95*u)/4,(3-row+.025+.95*v)/4)
            duv.data[i].uv=(c[axes[0]]/2,c[axes[1]]/2);colors.data[i].color=(metal,rough,0,1)
    bpy.context.view_layer.objects.active=o;o.select_set(True)
    mod=o.modifiers.new('Triangles','TRIANGULATE');bpy.ops.object.modifier_apply(modifier=mod.name);o.select_set(False)
    me.calc_loop_triangles();bounds=[min(v.co[a] for v in me.vertices) for a in range(3)]+[max(v.co[a] for v in me.vertices) for a in range(3)]
    assert max(abs(bounds[a+3]-bounds[a]-e['dimensions'][a]) for a in range(3))<1e-5,e['key']
    ent={'key':e['key'],'name':o.name,'label':e['label'],'triangles':len(me.loop_triangles),'bounds_m':bounds,'dimensions_m':e['dimensions'],'material_slots':1,'materials':[mat.name],'collision_boxes':e['collision'],'pivot_m':[0,0,0],'notes':e['notes']};entries.append(ent);assets[e['key']]=o
    saved=o.name;o.name=saved+'_source';copies=[]
    cp=o.copy();cp.data=o.data.copy();s.collection.objects.link(cp);cp.name=saved;copies.append(cp)
    cp.data.transform(Matrix.Diagonal((1,-1,1,1)));bm=bmesh.new();bm.from_mesh(cp.data);bmesh.ops.reverse_faces(bm,faces=list(bm.faces));bm.to_mesh(cp.data);bm.free()
    for idx,(lo,hi) in enumerate(e['collision']):
        bpy.ops.mesh.primitive_cube_add(size=1,location=[(a+b)/2 for a,b in zip(lo,hi)]);c=bpy.context.object;c.dimensions=[b-a for a,b in zip(lo,hi)];bpy.ops.object.transform_apply(location=True,rotation=True,scale=True)
        c.data.transform(Matrix.Diagonal((1,-1,1,1)));bm=bmesh.new();bm.from_mesh(c.data);bmesh.ops.reverse_faces(bm,faces=list(bm.faces));bm.to_mesh(c.data);bm.free();c.name=f'UCX_{saved}_{idx:02d}';copies.append(c)
    bpy.ops.object.select_all(action='DESELECT')
    for c in copies:c.select_set(True)
    bpy.context.view_layer.objects.active=cp
    bpy.ops.export_scene.fbx(filepath=str(OUT/'Models'/f'{saved}.fbx'),use_selection=True,object_types={'MESH'},apply_unit_scale=True,apply_scale_options='FBX_SCALE_NONE',axis_forward='-Y',axis_up='Z',mesh_smooth_type='FACE',add_leaf_bones=False,bake_anim=False,path_mode='STRIP',colors_type='LINEAR')
    for c in copies:bpy.data.objects.remove(c,do_unlink=True)
    o.name=saved;o.hide_render=True;o.hide_set(True)
for e in specs:build(e)

# Reuse actual completed architectural meshes/materials, never rebuild architecture.
with bpy.data.libraries.load(str(BASE/'ModularInteriorPreview.blend'),link=False) as (src,dst):
    dst.objects=[x for x in src.objects if x in ('SM_MI_Floor','SM_MI_Wall','SM_MI_Doorway','SM_MI_DoorLeaf','SM_MI_LightBar')]
for o in dst.objects:
    if o:templates.objects.link(o);o.hide_render=True;o.hide_set(True);assets[o.name[6:]]=o
base_manifest=json.loads((BASE/'model_manifest.json').read_text())
base_tris={e['key']:e['triangles'] for e in base_manifest['assets']}
placements=[];groups={}
def place(key,loc,scene='Lab',yaw=0,scale=(1,1,1),label=None):
    if scene not in groups:
        groups[scene]=bpy.data.collections.new(scene);s.collection.children.link(groups[scene])
    src=assets[key];o=src.copy();o.data=src.data;groups[scene].objects.link(o);o.hide_render=False;o.hide_set(False)
    o.name=label or f'LSP_{scene}_{key}_{len(placements):03d}';o.location=loc;o.rotation_euler.z=math.radians(yaw);o.scale=scale;o.color=((len(placements)*.371)%1,(len(placements)*.619)%1,0,1)
    placements.append({'name':o.name,'key':key,'group':scene,'location_m':list(loc),'yaw_deg':yaw,'scale':list(scale),'dirt_offset':list(o.color[:2]),'base':key in base_tris});return o
def room(group):
    for x in (-4,-2,0,2):
        for y in (-4,-2,0,2):place('Floor',(x,y,0),group)
    for x in (-4,-2,0,2):place('Wall',(x,4,0),group)
    for x in (-2,4):place('Wall',(x,-4,0),group,yaw=180)
    place('Doorway',(2,-4,0),group,yaw=180)
    for y in (-4,-2,0,2):
        place('Wall',(-4,y,0),group,yaw=90)
        place('Wall',(4,y+2,0),group,yaw=-90)
room('Lab');room('Warehouse')
place('Workbench',(-2.8,3.55,0));place('Sink',(-1.2,3.55,0));place('Analyzer',(1.4,3.45,0));place('ControlBox',(.1,3.99,1.25))
place('Cabinet',(-2.8,.8,0));place('Workbench',(-2.6,-1.8,0));place('Chair',(-2.6,-2.65,0),yaw=180);place('Cart',(.8,.6,0))
place('SampleTray',(-2.8,3.5,.9));place('SampleTray',(-2.55,-1.8,.9));place('SampleTray',(.7,.55,.735));place('Shelf',(3.6,1.2,0),yaw=90)
place('SupplyCrate',(3.6,1.2,.72),yaw=90)
for x in (-2.6,0,2.6):
    place('Shelf',(x,3.6,0),'Warehouse')
    for z in (.12,.72,1.32):
        for dx in (-.32,.32):place('SupplyCrate',(x+dx,3.6,z),'Warehouse')
for x,y in [(-2.6,-1.7),(2.6,-1.7)]:
    place('Pallet',(x,y,0),'Warehouse');place('BoxBundle',(x,y,.15),'Warehouse')
place('Pallet',(-2.6,.6,0),'Warehouse');place('SupplyCrate',(-2.85,.6,.15),'Warehouse');place('SupplyCrate',(-2.25,.6,.15),'Warehouse');place('SupplyCrate',(-2.85,.6,.55),'Warehouse')
place('Cart',(2.6,.6,0),'Warehouse');place('Cabinet',(-3.6,1.8,0),'Warehouse',yaw=-90)

# Separate actual-model contact sheet, positions are review-only.
for i,e in enumerate(entries):place(e['key'],((i%4)*2.65-3.975,3-(i//4)*3,0),'Sheet')

def aim(o,target):o.rotation_euler=(Vector(target)-o.location).to_track_quat('-Z','Y').to_euler()
bpy.ops.object.camera_add(location=(10,-14,13));cam=bpy.context.object;cam.name='ReviewCamera';cam.data.type='ORTHO';cam.data.ortho_scale=15.7;aim(cam,(0,0,.7));s.camera=cam
s.use_nodes=True;cn=s.node_tree.nodes;cn.clear();rl=cn.new('CompositorNodeRLayers');flip=cn.new('CompositorNodeFlip');flip.axis='X';output=cn.new('CompositorNodeComposite');s.node_tree.links.new(rl.outputs['Image'],flip.inputs['Image']);s.node_tree.links.new(flip.outputs['Image'],output.inputs['Image']);flip.mute=True
for pos,power,size in [((0,-2,11),2400,9),((-6,4,7),1500,6),((6,3,5),800,5)]:
    bpy.ops.object.light_add(type='AREA',location=pos);o=bpy.context.object;o.data.energy=power;o.data.shape='DISK';o.data.size=size;aim(o,(0,0,0))
# Stylized 176cm-high, 50cm-wide Blender scale proxy; exact native capsule bounds are checked in UE.
proxy=bpy.data.collections.new('Scale reference');s.collection.children.link(proxy)
pm=bpy.data.materials.new('Scale amber');pm.diffuse_color=(.95,.36,.045,1)
for center,r,depth in [((0,-.65,.75),.25,1.15),((0,-.65,1.56),.20,.40)]:
    bpy.ops.mesh.primitive_cylinder_add(vertices=8,radius=r,depth=depth,location=center);o=bpy.context.object;o.data.materials.append(pm)
    for c in list(o.users_collection):c.objects.unlink(o)
    proxy.objects.link(o)
def visibility(group):
    for name,c in groups.items():c.hide_render=name!=group;c.hide_viewport=name!=group
    proxy.hide_render=group=='Sheet';proxy.hide_viewport=group=='Sheet'
def render(name,group,view='oblique'):
    visibility(group)
    s.render.resolution_x=1600;s.render.resolution_y=1100
    if view=='top':
        # UE matching 1500cm arm, pitch -88, yaw0, target capsule center .88.
        pitch=math.radians(88);cam.location=(-15*math.cos(pitch),0,.88+15*math.sin(pitch));aim(cam,(0,0,.88));cam.data.type='PERSP';cam.data.lens_unit='FOV';cam.data.angle=math.radians(70);cam.data.sensor_fit='HORIZONTAL'
        cam.rotation_euler=Matrix(((0,-1,0),(math.sin(pitch),0,math.cos(pitch)),(-math.cos(pitch),0,math.sin(pitch)))).transposed().to_euler();flip.mute=False
    else:flip.mute=True;cam.location=(10,-14,13);aim(cam,(0,0,.6));cam.data.type='ORTHO';cam.data.ortho_scale=15.5 if group=='Sheet' else 13.9
    s.render.filepath=str(OUT/'Previews'/f'{name}.png');bpy.ops.render.render(write_still=True)
manifest={'assets':entries,'roofless':True,'materials':['M_LSP_Surface'],'textures':{'atlas':[2048,2048],'mask_reused':[1024,1024]},'placements':placements,'unique_triangles':sum(e['triangles'] for e in entries),'camera':{'arm_cm':1500,'pitch_deg':-88,'yaw_deg':0,'fov_deg':70},'sample_totals':{}}
for group in ('Lab','Warehouse','Sheet'):
    ps=[p for p in placements if p['group']==group];tri={**base_tris,**{e['key']:e['triangles'] for e in entries}}
    manifest['sample_totals'][group]={'instances':len(ps),'triangles':sum(tri[p['key']] for p in ps),'prop_instances':sum(not p['base'] for p in ps),'prop_triangles':sum(tri[p['key']] for p in ps if not p['base'])}
(OUT/'model_manifest.json').write_text(json.dumps(manifest,indent=2))
visibility('Lab');bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'LabSupplyProps.blend'))
if not os.getenv('LSP_SKIP_RENDER'):
    render('Models_12_Sheet','Sheet')
    for group in ('Lab','Warehouse'):
        render(group+'_Oblique',group);render(group+'_PlayCamera',group,'top')
    strength.inputs[1].default_value=0;render('Wear_BaseOnly','Lab','top')
    strength.inputs[1].default_value=1.8;render('Wear_AdditionalDirt','Lab','top')
    strength.inputs[1].default_value=.35
visibility('Lab');bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'LabSupplyProps.blend'))
print('LSP_BUILD_PASSED',manifest['unique_triangles'],manifest['sample_totals'])

