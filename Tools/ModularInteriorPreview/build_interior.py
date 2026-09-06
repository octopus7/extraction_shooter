"""Blender 4.5, meters in source / cm in UE. Rebuild all eight assets and review views."""
import bpy, bmesh, json, math, os
from pathlib import Path
from mathutils import Vector, Matrix
from collections import Counter

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / 'TunaSweeper/SourceArt/Environment/ModularInteriorPreview'
bpy.ops.wm.read_factory_settings(use_empty=True)
scene = bpy.context.scene
scene.unit_settings.system = 'METRIC'
scene.render.engine = 'CYCLES'
scene.cycles.samples = 32
scene.cycles.use_denoising = True
scene.render.threads_mode='FIXED';scene.render.threads=12
scene.render.resolution_x = 1400
scene.render.resolution_y = 1000
scene.render.resolution_percentage = 100
scene.world = bpy.data.worlds.new('PreviewWorld')
scene.world.color = (.18,.18,.18)
scene.view_settings.view_transform = 'AgX'

# Mechanical resolution conversion only; no image filtering, repainting or generated PBR maps.
image = bpy.data.images.load(str(OUT/'Textures/ImageGen_Original.png'))
original_size = list(image.size)
image.scale(2048,2048)
image.filepath_raw = str(OUT/'Textures/T_MI_Atlas.png')
image.file_format = 'PNG'
image.save()
image.pack()
image.filepath = '//Textures/T_MI_Atlas.png'
dirt = bpy.data.images.load(str(OUT/'Textures/ImageGen_Dirt_Original.png'))
dirt.colorspace_settings.name='Non-Color'
dirt.scale(1024,1024);dirt.filepath_raw=str(OUT/'Textures/T_MI_DirtMask.png')
dirt.file_format='PNG';dirt.save();dirt.pack();dirt.filepath='//Textures/T_MI_DirtMask.png'

specs = {
 'Concrete': {'metallic':0.,'roughness':.85,'tint':[1,1,1]},
 'Floor': {'metallic':0.,'roughness':.78,'tint':[.72,.76,.79]},
 'Steel': {'metallic':.25,'roughness':.52,'tint':[.62,.70,.73]},
 'Door': {'metallic':.20,'roughness':.58,'tint':[.83,.89,.91]},
 'LED': {'metallic':0.,'roughness':.35,'tint':[.68,.86,1.], 'emission':5.},
}
mats = {}
for key,spec in specs.items():
    m = bpy.data.materials.new('M_MI_'+key); m.use_nodes = True
    b = m.node_tree.nodes.get('Principled BSDF')
    b.inputs['Metallic'].default_value = spec['metallic']
    b.inputs['Roughness'].default_value = spec['roughness']
    if key == 'LED':
        b.inputs['Base Color'].default_value = (*spec['tint'],1)
        b.inputs['Emission Color'].default_value = (*spec['tint'],1)
        b.inputs['Emission Strength'].default_value = 5
    else:
        tex = m.node_tree.nodes.new('ShaderNodeTexImage'); tex.image = image
        mix = m.node_tree.nodes.new('ShaderNodeMixRGB'); mix.blend_type='MULTIPLY'
        mix.name='Tint'; mix.inputs[0].default_value=1
        mix.inputs[2].default_value=(*spec['tint'],1)
        m.node_tree.links.new(tex.outputs['Color'],mix.inputs[1])
        if key == 'Concrete':
            geo=m.node_tree.nodes.new('ShaderNodeNewGeometry')
            sep=m.node_tree.nodes.new('ShaderNodeSeparateXYZ')
            m.node_tree.links.new(geo.outputs['Position'],sep.inputs[0])
            low=m.node_tree.nodes.new('ShaderNodeMath');low.operation='GREATER_THAN';low.inputs[1].default_value=1.05
            high=m.node_tree.nodes.new('ShaderNodeMath');high.operation='LESS_THAN';high.inputs[1].default_value=1.30
            m.node_tree.links.new(sep.outputs['Z'],low.inputs[0]);m.node_tree.links.new(sep.outputs['Z'],high.inputs[0])
            mask=m.node_tree.nodes.new('ShaderNodeMath');mask.operation='MULTIPLY'
            m.node_tree.links.new(low.outputs[0],mask.inputs[0]);m.node_tree.links.new(high.outputs[0],mask.inputs[1])
            band=m.node_tree.nodes.new('ShaderNodeMixRGB');band.name='Band';band.blend_type='MULTIPLY'
            band.inputs[2].default_value=(1,1,1,1)
            m.node_tree.links.new(mask.outputs[0],band.inputs[0]);m.node_tree.links.new(mix.outputs[0],band.inputs[1])
            m.node_tree.links.new(band.outputs[0],b.inputs['Base Color'])
        else:m.node_tree.links.new(mix.outputs[0],b.inputs['Base Color'])
    if key!='LED':
        nodes=m.node_tree.nodes;links=m.node_tree.links
        base=b.inputs['Base Color'].links[0].from_socket
        uvnode=nodes.new('ShaderNodeUVMap');uvnode.uv_map='DirtUV'
        info=nodes.new('ShaderNodeObjectInfo')
        sep=nodes.new('ShaderNodeSeparateColor');links.new(info.outputs['Color'],sep.inputs[0])
        offset=nodes.new('ShaderNodeCombineXYZ');links.new(sep.outputs[0],offset.inputs[0]);links.new(sep.outputs[1],offset.inputs[1])
        scale=nodes.new('ShaderNodeVectorMath');scale.operation='SCALE';scale.name='DirtScale';scale.inputs[3].default_value=1
        links.new(uvnode.outputs[0],scale.inputs[0])
        add=nodes.new('ShaderNodeVectorMath');add.operation='ADD';links.new(scale.outputs[0],add.inputs[0]);links.new(offset.outputs[0],add.inputs[1])
        masktex=nodes.new('ShaderNodeTexImage');masktex.image=dirt;links.new(add.outputs[0],masktex.inputs[0])
        strength=nodes.new('ShaderNodeMath');strength.operation='MULTIPLY';strength.name='DirtStrength';strength.inputs[1].default_value=.65;strength.use_clamp=True
        links.new(masktex.outputs[0],strength.inputs[0])
        dirty=nodes.new('ShaderNodeMixRGB');dirty.inputs[2].default_value=(.12,.105,.075,1)
        links.new(base,dirty.inputs[1]);links.new(strength.outputs[0],dirty.inputs[0]);links.new(dirty.outputs[0],b.inputs['Base Color'])
        for field,target in [('Roughness',.94),('Metallic',0.)]:
            mixprop=nodes.new('ShaderNodeMixRGB');mixprop.inputs[1].default_value=(spec[field.lower()],)*3+(1,)
            mixprop.inputs[2].default_value=(target,)*3+(1,)
            links.new(strength.outputs[0],mixprop.inputs[0]);links.new(mixprop.outputs[0],b.inputs[field])
    mats[key]=m

templates=bpy.data.collections.new('Eight source modules - hidden from sample')
scene.collection.children.link(templates)
sample=bpy.data.collections.new('Sample assembly')
scene.collection.children.link(sample)
assets={}; collision={}; entries=[]

def prism(name, poly, depth, plane='XY', keys=('Concrete',)):
    """Extrude simple CCW outline, producing a closed mesh without internal faces."""
    n=len(poly)
    def co(p,d):return (p[0],p[1],d) if plane=='XY' else (p[0],d,p[1])
    verts=[co(p,d) for d in depth for p in poly]
    faces=[tuple(reversed(range(n))),tuple(range(n,2*n))]
    faces += [(i,(i+1)%n,(i+1)%n+n,i+n) for i in range(n)]
    return mesh(name,verts,faces,keys)

def mesh(name,verts,faces,keys,indices=None):
    me=bpy.data.meshes.new(name);me.from_pydata(verts,[],faces);me.update()
    o=bpy.data.objects.new('SM_MI_'+name,me);templates.objects.link(o)
    for k in keys:me.materials.append(mats[k])
    if indices:
        for p,i in zip(me.polygons,indices):p.material_index=i
    bm=bmesh.new();bm.from_mesh(me);bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces));bm.to_mesh(me);bm.free()
    assets[name]=o
    return o

def box(name,lo,hi,key):
    o=prism(name,[(lo[0],lo[1]),(hi[0],lo[1]),(hi[0],hi[1]),(lo[0],hi[1])],(lo[2],hi[2]),keys=(key,))
    collision[name]=[(lo,hi)]
    return o

box('Floor',(0,0,-.2),(2,2,0),'Floor')
box('Wall',(0,0,0),(2,.2,3),'Concrete')
prism('InsideCorner',[(-.2,-.2),(1,-.2),(1,0),(0,0),(0,1),(-.2,1)],(0,3))
collision['InsideCorner']=[((-.2,-.2,0),(1,0,3)),((-.2,0,0),(0,1,3))]

# Watertight gridded U extrusion. Frame is flush topology/material, no separate overlapping trim.
xs=[0,.9,1,3,3.1,4]; zs=[0,2.4,2.5,3]
cells={}
for i in range(5):
    for j in range(3):
        if i==2 and j==0:continue
        cells[i,j]=1 if (i in (1,3) and j<2) or (i==2 and j==1) else 0
verts=[];faces=[];indices=[];lookup={}
def vid(x,y,z):
    p=(x,y,z)
    if p not in lookup:lookup[p]=len(verts);verts.append(p)
    return lookup[p]
for (i,j),material in cells.items():
    x0,x1=xs[i:i+2];z0,z1=zs[j:j+2]
    for y in (0,.2):
        faces.append(tuple(vid(x,y,z) for x,z in [(x0,z0),(x1,z0),(x1,z1),(x0,z1)]));indices.append(material)
    for neighbor,ends in [((i-1,j),[(x0,z0),(x0,z1)]),((i+1,j),[(x1,z0),(x1,z1)]),((i,j-1),[(x0,z0),(x1,z0)]),((i,j+1),[(x0,z1),(x1,z1)])]:
        if neighbor not in cells:
            (a,b),(c,d)=ends
            faces.append((vid(a,0,b),vid(c,0,d),vid(c,.2,d),vid(a,.2,b)));indices.append(material)
mesh('Doorway',verts,faces,('Concrete','Steel'),indices)
collision['Doorway']=[((0,0,0),(1,.2,2.4)),((3,0,0),(4,.2,2.4)),((0,0,2.4),(4,.2,3))]
box('DoorLeaf',(0,0,0),(1.96,.08,2.36),'Door')
box('Ceiling',(0,0,0),(2,2,.2),'Concrete')
box('Beam',(0,-.10,-.20),(2,.10,0),'Steel')
o=box('LightBar',(0,-.06,-.08),(1.5,.06,0),'Steel')
o.data.materials.append(mats['LED'])
# The bottom face is the diffuser. No second coplanar plane or transparent material.
for p in o.data.polygons:
    if p.normal.z<-.9:p.material_index=1
collision['LightBar']=[]

regions={'Concrete':(.012,.512,.488,.988),'Floor':(.008,.008,.492,.492),
         'Steel':(.508,.508,.992,.992),'Door':(.508,.008,.992,.492),'LED':(.65,.65,.8,.8)}
for name,o in assets.items():
    # UVs are planar and world-meter proportional for concrete; panel faces use full atlas region.
    uv=o.data.uv_layers.new(name='UVMap')
    dirtuv=o.data.uv_layers.new(name='DirtUV')
    for p in o.data.polygons:
        key=o.data.materials[p.material_index].name[5:]
        axis=max(range(3),key=lambda a:abs(p.normal[a]))
        axes=[a for a in range(3) if a!=axis]
        coords=[o.data.vertices[o.data.loops[li].vertex_index].co for li in p.loop_indices]
        ranges=[(min(v[a] for v in coords),max(v[a] for v in coords)) for a in axes]
        u0,v0,u1,v1=regions[key]
        for li,v in zip(p.loop_indices,coords):
            if key=='Concrete':
                a=((v[axes[0]]+10)%2)/2
                b=((v[axes[1]]+12)%3)/3
                # Endpoints stay continuous within each planar polygon.
                a=(v[axes[0]]-ranges[0][0])/max(2,ranges[0][1]-ranges[0][0])
                b=(v[axes[1]]-ranges[1][0])/max(3 if axes[1]==2 else 2,ranges[1][1]-ranges[1][0])
            else:
                a=(v[axes[0]]-ranges[0][0])/max(1e-8,ranges[0][1]-ranges[0][0])
                b=(v[axes[1]]-ranges[1][0])/max(1e-8,ranges[1][1]-ranges[1][0])
            uv.data[li].uv=(u0+(u1-u0)*a,v0+(v1-v0)*b)
            dirtuv.data[li].uv=(v[axes[0]]/4,v[axes[1]]/4)
    # Triangulate deterministically once; FBX/UE are checked against these exact counts.
    bpy.context.view_layer.objects.active=o;o.select_set(True)
    mod=o.modifiers.new('Final low-poly triangles','TRIANGULATE')
    bpy.ops.object.modifier_apply(modifier=mod.name)
    o.select_set(False);o.data.calc_loop_triangles()
    bm=bmesh.new();bm.from_mesh(o.data)
    assert all(e.is_manifold for e in bm.edges),name
    assert bm.calc_volume(signed=True)>0,name
    bm.free()
    assert all(t.area>1e-10 for t in o.data.loop_triangles),name
    assert all(math.isfinite(x) and -.001<=x<=1.001 for d in o.data.uv_layers['UVMap'].data for x in d.uv),name
    bounds=[min(v.co[i] for v in o.data.vertices) for i in range(3)]+[max(v.co[i] for v in o.data.vertices) for i in range(3)]
    entry={'name':o.name,'key':name,'triangles':len(o.data.loop_triangles),'material_slots':len(o.data.materials),
           'materials':[m.name for m in o.data.materials],'bounds_m':bounds,'collision_boxes':collision[name],
           'pivot_m':[0,0,0]}
    entries.append(entry)
    # UE handedness compensation; render scene retains project axes.
    copies=[]
    def export_copy(src,newname):
        cp=src.copy();cp.data=src.data.copy();scene.collection.objects.link(cp);cp.name=newname
        cp.data.transform(Matrix.Diagonal((1,-1,1,1)))
        bm=bmesh.new();bm.from_mesh(cp.data);bmesh.ops.reverse_faces(bm,faces=list(bm.faces));bm.to_mesh(cp.data);bm.free()
        cp.select_set(True);copies.append(cp)
        return cp
    saved_name=o.name;o.name=saved_name+'_source'
    cp=export_copy(o,saved_name)
    for ci,(lo,hi) in enumerate(collision[name]):
        bpy.ops.mesh.primitive_cube_add(size=1,location=[(a+b)/2 for a,b in zip(lo,hi)])
        c=bpy.context.object;c.dimensions=[b-a for a,b in zip(lo,hi)]
        bpy.ops.object.transform_apply(location=True,rotation=True,scale=True)
        c.data.transform(Matrix.Diagonal((1,-1,1,1)))
        bm=bmesh.new();bm.from_mesh(c.data);bmesh.ops.reverse_faces(bm,faces=list(bm.faces));bm.to_mesh(c.data);bm.free()
        c.name=f'UCX_{saved_name}_{ci:02d}';copies.append(c)
    bpy.ops.object.select_all(action='DESELECT')
    for c in copies:c.select_set(True)
    bpy.context.view_layer.objects.active=cp
    bpy.ops.export_scene.fbx(filepath=str(OUT/'Models'/f'{saved_name}.fbx'),use_selection=True,object_types={'MESH'},
        apply_unit_scale=True,apply_scale_options='FBX_SCALE_NONE',axis_forward='-Y',axis_up='Z',
        mesh_smooth_type='FACE',add_leaf_bones=False,bake_anim=False,path_mode='STRIP')
    for c in copies:bpy.data.objects.remove(c,do_unlink=True)
    o.name=saved_name;o.hide_render=True;o.hide_set(True)

placements=[];ceilings=[]
def place(key,loc,angle=0,scale=(1,1,1),label=None):
    src=assets[key];o=src.copy();o.data=src.data;sample.objects.link(o)
    o.name=label or f'{key}_{len(placements):03d}';o.location=loc;o.rotation_euler.z=angle
    o.scale=scale;o.hide_render=False;o.hide_set(False)
    offset=[round((len(placements)*.371)%1,4),round((len(placements)*.619)%1,4)]
    o.color=(*offset,.65,1)
    placements.append({'name':o.name,'key':key,'location_m':list(loc),'yaw_deg':math.degrees(angle),'scale':list(scale),'dirt_offset':offset})
    if key=='Ceiling':ceilings.append(o)
    return o

# 6x6 clear room, threshold through 20cm wall, 4m-wide L corridor.
for x in (0,2,4):
    for y in (0,2,4):
        place('Floor',(x,y,0));place('Ceiling',(x,y,3))
for x in (1,3):place('Floor',(x,6,0),scale=(1,.1,1));place('Ceiling',(x,6,3),scale=(1,.1,1))
for x in (1,3,5,7):
    for y in (6.2,8.2,10.2,12.2):
        if x>=5 and y<10:continue
        place('Floor',(x,y,0));place('Ceiling',(x,y,3))

edge_checks=[]
def perimeter(poly,open_edges=(),open_vertices=(),door_edge=None):
    n=len(poly);consumed=[]
    for i,point in enumerate(poly):
        p=Vector(point);prev=Vector(poly[(i-1)%n]);nxt=Vector(poly[(i+1)%n])
        incoming=(p-prev).normalized();outgoing=(nxt-p).normalized()
        convex=incoming.x*outgoing.y-incoming.y*outgoing.x>0
        arms=[-incoming,outgoing]
        if arms[0].x*arms[1].y-arms[0].y*arms[1].x<0:arms.reverse()
        if i in open_vertices:consumed.append(0);continue
        origin=p if convex else p+.2*(arms[0]+arms[1])
        angle=math.atan2(arms[0].y,arms[0].x)
        place('InsideCorner',(*origin,0),angle)
        consumed.append(1 if convex else 1.2)
    for i in range(n):
        if i in open_edges:continue
        p=Vector(poly[i]);q=Vector(poly[(i+1)%n]);d=(q-p).normalized()
        a=p+d*consumed[i];b=q-d*consumed[(i+1)%n]
        length=(b-a).dot(d);assert length>=-1e-5
        if i==door_edge:
            assert abs(length-4)<1e-5
            # Doorway's x runs opposite CCW boundary so its +Y is outward.
            place('Doorway',(*b,0),math.atan2(-d.y,-d.x))
            edge_checks.append({'length':length,'filled':4,'kind':'doorway'});continue
        remaining=length;total=0
        while remaining>1e-5:
            span=min(2,remaining)
            end=a+d*(total+span)
            place('Wall',(*end,0),math.atan2(-d.y,-d.x),(span/2,1,1))
            total+=span;remaining-=span
        edge_checks.append({'length':length,'filled':total,'kind':'wall'})
perimeter([(0,0),(6,0),(6,6),(0,6)],door_edge=2)
perimeter([(1,6.2),(5,6.2),(5,10.2),(9,10.2),(9,14.2),(1,14.2)],open_edges=(0,),open_vertices=(0,1))
# Side-hinged leaf opened into room. 2cm clearances on all four edges when closed.
place('DoorLeaf',(2.02,6.06,.02),math.radians(-68),label='DoorLeaf_open_68deg')
for x,y in [(0,2),(2,2),(4,2),(0,4),(2,4),(4,4),(1,8.2),(3,8.2),(1,12.2),(3,12.2),(5,12.2),(7,12.2)]:
    place('Beam',(x,y,3))
for x,y in [(1.25,2),(3.25,4),(2.25,8.2),(2.25,12.2),(6.25,12.2)]:
    place('LightBar',(x,y,2.79))

# Temporary project capsule-sized scale mannequin, excluded from eight exported models.
def plainmat(name,color):
    m=bpy.data.materials.new(name);m.diffuse_color=(*color,1);return m
refmat=plainmat('Preview_ScaleMarker',(.67,.28,.09))
def refpart(loc,scale):
    bpy.ops.mesh.primitive_uv_sphere_add(segments=12,ring_count=6,radius=1,location=loc)
    o=bpy.context.object;o.name='PreviewOnly_176cm_reference';o.scale=scale;o.data.materials.append(refmat)
    return o
refpart((3,3.3,1.56),(.18,.18,.20))
refpart((3,3.3,1.13),(.28,.18,.30))
for dx in (-.15,.15):
    refpart((3+dx,3.3,.46),(.12,.12,.46))
    refpart((3+dx*1.8,3.3,.99),(.07,.09,.31))

def area(name,loc,energy,size,color=(.83,.90,1),target=None):
    d=bpy.data.lights.new(name,'AREA');d.energy=energy;d.shape='DISK';d.size=size;d.color=color
    o=bpy.data.objects.new(name,d);scene.collection.objects.link(o);o.location=loc
    if target:o.rotation_euler=(Vector(target)-o.location).to_track_quat('-Z','Y').to_euler()
    return o
# Five actual fixtures. Preview illumination is explicitly separate from emissive mesh cost.
for i,(x,y) in enumerate([(2,2),(4,4),(3,8.2),(3,12.2),(7,12.2)]):area('FixtureLight_'+str(i),(x,y,2.70),110,1.1)
studio=area('PreviewOnly_OverviewSoftbox',(-1,3,10),1800,9,target=(3,6,0))
def camera(name,loc,target,lens=35,ortho=None):
    d=bpy.data.cameras.new(name);o=bpy.data.objects.new(name,d);scene.collection.objects.link(o)
    o.location=loc;o.rotation_euler=(Vector(target)-o.location).to_track_quat('-Z','Y').to_euler();d.lens=lens
    if ortho:d.type='ORTHO';d.ortho_scale=ortho
    return o
overview=camera('Overview',(-14,-16,20),(4.0,7.0,.4),ortho=23)
eye=camera('Eye_160cm',(1.0,.9,1.6),(3.4,6.4,1.5),lens=20)
# Runtime native top-down mode: arm=1500cm, pitch=-88deg, FOV=70, capsule center z=88cm.
pitch=math.radians(88);target=Vector((3,3.3,.88))
play=camera('PlayCamera_NativeTopDown',target+Vector((-15*math.cos(pitch),0,15*math.sin(pitch))),target)
play.data.lens=36/(2*math.tan(math.radians(70)/2));play.data.sensor_width=36
play.rotation_euler=Matrix(((0,-1,0),(math.sin(pitch),0,math.cos(pitch)),(-math.cos(pitch),0,math.sin(pitch)))).transposed().to_euler()
# Blender is right-handed, UE is left-handed. Horizontal film flip makes +Y screen-right.
scene.use_nodes=True
nodes=scene.node_tree.nodes;nodes.clear()
rl=nodes.new('CompositorNodeRLayers');flip=nodes.new('CompositorNodeFlip');flip.axis='X'
output=nodes.new('CompositorNodeComposite')
scene.node_tree.links.new(rl.outputs['Image'],flip.inputs['Image'])
scene.node_tree.links.new(flip.outputs['Image'],output.inputs['Image']);flip.mute=True

counts=Counter(p['key'] for p in placements)
manifest={'assets':entries,'materials':{'M_MI_'+k:v for k,v in specs.items()},'texture':{'runtime_size':[2048,2048],
 'imagegen_original_size':original_size,'runtime_count':2,'dirt_size':[1024,1024],'normal_map':None,'roughness':'artist-selected scalar, not measured PBR'},
 'placements':placements,'counts':dict(counts),'unique_triangles':sum(e['triangles'] for e in entries),
 'sample_triangles':sum(counts[e['key']]*e['triangles'] for e in entries),'sample_instances':len(placements),
 'real_lights':5,'preview_only_softboxes':1,'room_clear_m':[6,6,3],'corridor_clear_width_m':4,
 'door_clear_m':[2,2.4],'edge_checks':edge_checks,'axis_contract':'Blender source X=UE X north, Y=UE Y east, Z=up; export copies mirror Y',
 'play_camera':{'arm_cm':1500,'pitch_deg':-88,'yaw_deg':0,'horizontal_fov_deg':70,'target_m':list(target)}}
assert all(abs(e['length']-e['filled'])<1e-5 for e in edge_checks)
(OUT/'model_manifest.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')

def variant(kind):
    tint={'Light':(1.2,1.2,1.16,1),'Dark':(.29,.35,.40,1),'Managed':(.97,1.05,1.03,1)}[kind]
    mats['Concrete'].node_tree.nodes['Tint'].inputs[2].default_value=tint
    mats['Concrete'].node_tree.nodes['Band'].inputs[2].default_value=(.17,.48,.50,1) if kind=='Managed' else (1,1,1,1)

def render(name,cam,roof=False):
    scene.camera=cam
    flip.mute=cam!=play
    for o in ceilings:o.hide_render=not roof
    studio.hide_render=roof
    scene.render.filepath=str(OUT/'Previews'/f'{name}.png');bpy.ops.render.render(write_still=True)

variant('Light')
scene.camera=overview
for o in ceilings:o.hide_render=True
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'ModularInteriorPreview.blend'))
if not os.environ.get('MI_SKIP_RENDER'):
    for kind in ('Light','Dark','Managed'):
        variant(kind);render(f'{kind}_Overview',overview)
        render(f'{kind}_Eye',eye,True)
    variant('Light');render('Light_PlayCamera',play)
    for label,strength in [('Clean',0),('Weak',.65),('Strong',1.8)]:
        for key,m in mats.items():
            if key!='LED':m.node_tree.nodes['DirtStrength'].inputs[1].default_value=strength
        render('Dirt_'+label+'_Eye',eye,True)
        render('Dirt_'+label+'_Overview',overview)
    for key,m in mats.items():
        if key!='LED':m.node_tree.nodes['DirtStrength'].inputs[1].default_value=.65
variant('Light')
for o in ceilings:o.hide_render=True
studio.hide_render=False;scene.camera=overview
flip.mute=True
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'ModularInteriorPreview.blend'))
print('MI_BUILD_PASSED',manifest['unique_triangles'],manifest['sample_triangles'])
