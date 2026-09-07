"""Blender 4.5 reproducible loot set. Meters, +Z up; front +Y, hinge rear -Y.

Reuses the ImageGen LabSupplyProps atlas and supply-crate proportions/caps.
Only source artifacts are generated; no Unreal editor startup hooks.
"""
from pathlib import Path
import bpy, bmesh, math, json, os
from mathutils import Vector, Matrix
ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'TunaSweeper/SourceArt/Environment/LootContainerSet'
for d in ['Models','Previews']: (OUT/d).mkdir(parents=True,exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True)
s=bpy.context.scene;s.unit_settings.system='METRIC';s.unit_settings.scale_length=1
s.render.engine='CYCLES';s.cycles.samples=32;s.cycles.use_denoising=True
s.render.threads_mode='FIXED';s.render.threads=10
s.render.resolution_x=1440;s.render.resolution_y=1000;s.render.resolution_percentage=100
s.view_settings.view_transform='AgX'
s.world=bpy.data.worlds.new('Bright daylight');s.world.use_nodes=True
s.world.node_tree.nodes['Background'].inputs[0].default_value=(.65,.70,.78,1)
s.world.node_tree.nodes['Background'].inputs[1].default_value=.7
mat=bpy.data.materials.new('M_LSP_Surface');mat.use_nodes=True
n=mat.node_tree.nodes;l=mat.node_tree.links;b=n.get('Principled BSDF')
tex=n.new('ShaderNodeTexImage');tex.image=bpy.data.images.load(str(OUT/'Textures/T_LSP_Atlas.png'));tex.image.pack();tex.image.filepath='//Textures/T_LSP_Atlas.png'
uv=n.new('ShaderNodeUVMap');uv.uv_map='UVMap';l.new(uv.outputs[0],tex.inputs[0])
duv=n.new('ShaderNodeUVMap');duv.uv_map='DirtUV'
dirt=n.new('ShaderNodeTexImage');dirt.image=bpy.data.images.load(str(OUT/'Textures/T_MI_DirtMask.png'));dirt.image.colorspace_settings.name='Non-Color';dirt.image.pack();dirt.image.filepath='//Textures/T_MI_DirtMask.png';l.new(duv.outputs[0],dirt.inputs[0])
strength=n.new('ShaderNodeMath');strength.operation='MULTIPLY';strength.name='DirtStrength';strength.inputs[1].default_value=.18;l.new(dirt.outputs[0],strength.inputs[0])
mix=n.new('ShaderNodeMixRGB');mix.inputs[2].default_value=(.12,.105,.075,1);l.new(strength.outputs[0],mix.inputs[0]);l.new(tex.outputs[0],mix.inputs[1]);l.new(mix.outputs[0],b.inputs['Base Color'])
vc=n.new('ShaderNodeVertexColor');vc.layer_name='SurfaceParams';sep=n.new('ShaderNodeSeparateColor');l.new(vc.outputs[0],sep.inputs[0]);l.new(sep.outputs[0],b.inputs['Metallic']);l.new(sep.outputs[1],b.inputs['Roughness'])
sources=bpy.data.collections.new('SOURCE - six export meshes');s.collection.children.link(sources)
def box(lo,hi,tile,**kw): return dict(lo=list(lo),hi=list(hi),tile=tile,**kw)
def shell(w,d,z0,z1,t,tile,front=None):
    return [box((-w/2,-d/2,z0),(w/2,d/2,z0+t),tile),
      box((-w/2,-d/2,z0+t),(-w/2+t,d/2,z1),tile),box((w/2-t,-d/2,z0+t),(w/2,d/2,z1),tile),
      box((-w/2+t,-d/2,z0+t),(w/2-t,-d/2+t,z1),tile),box((-w/2+t,d/2-t,z0+t),(w/2-t,d/2,z1),tile,face_tiles={'front':front if front is not None else tile})]
specs=[];sets=[]
def add_set(key,w,d,h,seam,body,lid):
    hinge=[0,-d/2,seam]
    sets.append(dict(key=key,dimensions_m=[w,d,h],hinge_m=hinge,closed_roll=0,open_roll=-105,body_location_cm=[0,0,0],lid_location_cm=[0,0,0],scale=[1,1,1]))
    for part,ps,origin in [('Body',body,[0,0,0]),('Lid',lid,hinge)]:
        specs.append(dict(name=f'SM_LC_{key}_{part}',key=key,part=part,parts=ps,origin=origin,dimensions=[w,d,h],seam=seam))
# Wood: five solid shell slabs, silhouette battens; plank lines are atlas-only.
w,d,h=1.1,.8,.55;seam=.46
body=shell(1.04,.74,0,seam-.002,.027,8)
for x in [-.40,.40]:
    for y in [-.40,.37]:body.append(box((x-.035,y,0),(x+.035,y+.03,seam-.002),8,grain_vertical=True))
for x in [-.55,.52]:body.append(box((x,-.37,0),(x+.03,.37,.06),8))
for x in [-.35,.35]:body.append(box((x-.035,-.4,.425),(x+.035,-.365,seam-.002),1))
lid=[box((-.55,-.4,seam),(.55,.4,.515),8)]
for x in [-.4,.4]:lid.append(box((x-.04,-.4,.515),(x+.04,.4,.55),8,grain_vertical=True))
add_set('Wood',w,d,h,seam,body,lid)
# Metal: chamfered eight-sided tray rim, with a flat chamfered lid and two ribs.
w,d,h=1.35,.95,.60;seam=.50;ch=.07
def octagon(w,d,c):return [(-w/2+c,-d/2),(w/2-c,-d/2),(w/2,-d/2+c),(w/2,d/2-c),(w/2-c,d/2),(-w/2+c,d/2),(-w/2,d/2-c),(-w/2,-d/2+c)]
outer=octagon(w,d,ch);inner=octagon(w-.06,d-.06,ch-.012)
body=[dict(polygon=outer,depth=[0,.035],tile=1)]
for i in range(8):body.append(dict(polygon=[outer[i],outer[(i+1)%8],inner[(i+1)%8],inner[i]],depth=[.035,seam-.002],tile=1))
lid=[dict(polygon=outer,depth=[seam,.573],tile=1)]
for x in [-.40,.40]:lid.append(box((x-.035,-.405,.573),(x+.035,.405,.60),3))
# Broad carry handle is essential silhouette. Two supports and one grip at front.
for x in [-.13,.10]:body.append(box((x,.44,.28),(x+.03,.475,.35),1))
body.append(box((-.13,.45,.35),(.13,.475,.38),1))
add_set('Metal',w,d,h,seam,body,lid)
# Existing LSP closed supply crate: 2.75x XY and 1.75x Z, now hollow/split.
w,d,h=1.65,1.1,.70;seam=.6125
body=shell(.58*2.75,.38*2.75,.025*1.75,seam-.002,.03,0,11)
body.append(box((-.295*2.75,-.195*2.75,0),(.295*2.75,.195*2.75,.025*1.75),2))
lid=[box((-.295*2.75,-.195*2.75,seam),(.295*2.75,.195*2.75,.393*1.75),1,face_tiles={'top':12,'bottom':0,'front':11})]
for xa,xb in [(-.30,-.235),(.235,.30)]:
    for ya,yb in [(-.20,-.14),(.14,.20)]:
        body.append(box((xa*2.75,ya*2.75,0),(xb*2.75,yb*2.75,.055*1.75),1))
        body.append(box((xa*2.75,ya*2.75,.305*1.75),(xb*2.75,yb*2.75,seam-.001),1))
        lid.append(box((xa*2.75,ya*2.75,seam),(xb*2.75,yb*2.75,h),1))
add_set('Supply',w,d,h,seam,body,lid)
def geometry(p):
    if 'polygon' in p:poly=p['polygon'];a,b=p['depth']
    else:
        lo,hi=p['lo'],p['hi'];poly=[(lo[0],lo[1]),(hi[0],lo[1]),(hi[0],hi[1]),(lo[0],hi[1])];a,b=lo[2],hi[2]
    k=len(poly);vs=[(x,y,z) for z in [a,b] for x,y in poly]
    fs=[tuple(reversed(range(k))),tuple(range(k,2*k))]+[(i,(i+1)%k,(i+1)%k+k,i+k) for i in range(k)]
    return vs,fs
entries=[];objects={}
def build(e):
    vs=[];fs=[];ps=[];origin=Vector(e['origin'])
    for p in e['parts']:
        v,f=geometry(p);o=len(vs);vs +=[Vector(c)-origin for c in v];fs +=[tuple(i+o for i in face) for face in f];ps +=[p]*len(f)
    me=bpy.data.meshes.new(e['name']);me.from_pydata(vs,[],fs);me.update()
    bm=bmesh.new();bm.from_mesh(me);bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces));bm.to_mesh(me);bm.free()
    ob=bpy.data.objects.new(e['name'],me);sources.objects.link(ob);me.materials.append(mat)
    me.uv_layers.new(name='UVMap');me.uv_layers.new(name='DirtUV');me.color_attributes.new(name='SurfaceParams',type='FLOAT_COLOR',domain='CORNER')
    uv=me.uv_layers['UVMap'];duv=me.uv_layers['DirtUV'];colors=me.color_attributes['SurfaceParams'];me.color_attributes.active_color=colors;me.uv_layers.active_index=0
    for f,p in zip(me.polygons,ps):
        axis=max(range(3),key=lambda a:abs(f.normal[a]));side=[('left','right'),('back','front'),('bottom','top')][axis][f.normal[axis]>0]
        tile=p.get('face_tiles',{}).get(side,p['tile']);axes=[a for a in range(3) if a!=axis]
        cs=[me.vertices[me.loops[i].vertex_index].co+origin for i in f.loop_indices]
        ranges=[(min(c[a] for c in cs),max(c[a] for c in cs)) for a in axes]
        # LSP crate front artwork extends across the entire shell/front seam region.
        if tile==11:ranges=[(-.81125,.81125),(.04375,.04375+(.6125-.04375)/.82)]
        for i,c in zip(f.loop_indices,cs):
            u=(c[axes[0]]-ranges[0][0])/max(1e-9,ranges[0][1]-ranges[0][0]);v=(c[axes[1]]-ranges[1][0])/max(1e-9,ranges[1][1]-ranges[1][0])
            if side in ['front','left']:u=1-u
            if p.get('grain_vertical'):u,v=v,u
            uv.data[i].uv=((tile%4+.025+.95*u)/4,(3-tile//4+.025+.95*v)/4)
            # Stable assembly coordinates: no jump from the lid's hinge-local origin.
            duv.data[i].uv=((c.x+.731*c.y)*.5,c.z*.5) if axis!=2 else (c.x*.5,c.y*.5)
            metal,rough=(0,.82) if tile==8 else (.68,.43) if tile==3 else (.12,.64)
            colors.data[i].color=(metal,rough,0,1)
    bpy.context.view_layer.objects.active=ob;ob.select_set(True)
    mod=ob.modifiers.new('Explicit triangles','TRIANGULATE');bpy.ops.object.modifier_apply(modifier=mod.name);ob.select_set(False)
    me.calc_loop_triangles();bounds=[min(v.co[a] for v in me.vertices) for a in range(3)]+[max(v.co[a] for v in me.vertices) for a in range(3)]
    # Collision follows tray bottom and four walls; no hull spanning the open cavity.
    if e['part']=='Body':
        w,d,_=e['dimensions'];z=e['seam']-.002;t=.03
        collisions=[[[ -w/2,-d/2,0],[w/2,d/2,.035]], [[-w/2,-d/2,.035],[-w/2+t,d/2,z]],[[w/2-t,-d/2,.035],[w/2,d/2,z]],[[-w/2+t,-d/2,.035],[w/2-t,-d/2+t,z]],[[-w/2+t,d/2-t,.035],[w/2-t,d/2,z]]]
    else:collisions=[[bounds[:3],bounds[3:]]]
    ent=dict(name=e['name'],triangles=len(me.loop_triangles),bounds_m=bounds,collision_boxes=collisions,material_slots=1,materials=[mat.name],pivot_m=[0,0,0],assembly_origin_m=e['origin'])
    entries.append(ent);objects[e['name']]=ob
    saved=ob.name;ob.name=saved+'_source';copies=[]
    cp=ob.copy();cp.data=ob.data.copy();s.collection.objects.link(cp);cp.name=saved;copies.append(cp)
    for j,(lo,hi) in enumerate(collisions):
        bpy.ops.mesh.primitive_cube_add(size=1,location=[(a+b)/2 for a,b in zip(lo,hi)]);c=bpy.context.object;c.dimensions=[b-a for a,b in zip(lo,hi)];bpy.ops.object.transform_apply(location=True,rotation=True,scale=True);c.name=f'UCX_{saved}_{j:02}';copies.append(c)
    for c in copies:
        c.data.transform(Matrix.Diagonal((1,-1,1,1)));bm=bmesh.new();bm.from_mesh(c.data);bmesh.ops.reverse_faces(bm,faces=list(bm.faces));bm.to_mesh(c.data);bm.free()
    bpy.ops.object.select_all(action='DESELECT')
    for c in copies:c.select_set(True)
    bpy.context.view_layer.objects.active=cp
    bpy.ops.export_scene.fbx(filepath=str(OUT/'Models'/f'{saved}.fbx'),use_selection=True,object_types={'MESH'},apply_unit_scale=True,apply_scale_options='FBX_SCALE_NONE',axis_forward='-Y',axis_up='Z',mesh_smooth_type='FACE',add_leaf_bones=False,bake_anim=False,path_mode='STRIP',colors_type='LINEAR')
    for c in copies:bpy.data.objects.remove(c,do_unlink=True)
    ob.name=saved;ob.hide_render=True;ob.hide_set(True)
for e in specs:build(e)
manifest=dict(assets=entries,sets=sets,textures={'atlas':[2048,2048],'dirt_mask':[1024,1024]},texture_samples=2,material='M_LSP_Surface',unique_triangles=sum(e['triangles'] for e in entries),fbx_export='Y reflection plus reversed winding; -Y forward, Z up, meters to UE cm')
(OUT/'model_manifest.json').write_text(json.dumps(manifest,indent=2))
review=bpy.data.collections.new('REVIEW - actual models');s.collection.children.link(review)
def place(key,part,loc,open=False):
    ob=objects[f'SM_LC_{key}_{part}'].copy();ob.data=ob.data;review.objects.link(ob);ob.hide_render=False;ob.hide_set(False);ob.location=loc
    if part=='Lid':
        spec=next(e for e in sets if e['key']==key);ob.location+=Vector(spec['hinge_m']);ob.rotation_euler.x=math.radians(105 if open else 0)
    return ob
def clear():
    for o in list(review.objects):bpy.data.objects.remove(o,do_unlink=True)
def aim(o,t):o.rotation_euler=(Vector(t)-o.location).to_track_quat('-Z','Y').to_euler()
bpy.ops.mesh.primitive_plane_add(size=200);floor=bpy.context.object;floor.name='Review ground';floor.location.z=-.018
fm=bpy.data.materials.new('Review ground warm neutral');fm.diffuse_color=(.56,.55,.50,1);floor.data.materials.append(fm)
for loc,power,size in [((-3,3,8),2000,7),((4,-2,6),1000,5)]:
    bpy.ops.object.light_add(type='AREA',location=loc);bpy.context.object.data.energy=power;bpy.context.object.data.size=size;aim(bpy.context.object,(0,0,0))
bpy.ops.object.camera_add();cam=bpy.context.object;s.camera=cam;cam.data.type='ORTHO'
def render(name,view='angle',scale=6.5,target=(0,0,.2)):
    if os.getenv('LC_RENDER_FILTER') and not any(word in name for word in os.environ['LC_RENDER_FILTER'].split(',')):return
    cam.data.ortho_scale=scale
    if view=='top':cam.data.ortho_scale=8.5;cam.location=(0,0,10);cam.rotation_euler=(0,0,math.pi/2)
    elif view=='rear':cam.location=(-4,-7,6);aim(cam,target)
    elif view=='front':cam.location=(0,8,2.1);aim(cam,target)
    else:cam.location=(4,7,7);aim(cam,target)
    # Fit actual projected vertices so open lids and corner caps never crop.
    bpy.context.view_layer.update();inv=cam.matrix_world.inverted()
    points=[inv @ (o.matrix_world @ v.co) for o in review.objects for v in o.data.vertices]
    xmin,xmax=min(p.x for p in points),max(p.x for p in points)
    ymin,ymax=min(p.y for p in points),max(p.y for p in points)
    cam.data.ortho_scale=max(cam.data.ortho_scale,1.16*max(xmax-xmin,(ymax-ymin)*s.render.resolution_x/s.render.resolution_y))
    cam.location+=cam.rotation_euler.to_matrix() @ Vector(((xmin+xmax)/2,(ymin+ymax)/2,0))
    s.render.filepath=str(OUT/'Previews'/f'{name}.png');bpy.ops.render.render(write_still=True)
if not os.getenv('LC_SKIP_RENDER'):
    for isopen in [False,True]:
        clear()
        for x,e in zip([-1.75,-.2,1.6],sets):
            for p in ['Body','Lid']:place(e['key'],p,(x,0,0),isopen)
        state='Open' if isopen else 'Closed'
        render(f'Actual_{state}_Comparison');render(f'Actual_{state}_Topdown','top')
    for e in sets:
        for isopen in [False,True]:
            clear()
            for p in ['Body','Lid']:place(e['key'],p,(0,0,0),isopen)
            render(f"Actual_{e['key']}_{'Open' if isopen else 'Closed'}",scale=(4.2 if e['key']=='Supply' else 3.5) if isopen else 2.9,target=(0,0,.5))
        render(f"Actual_{e['key']}_Rear",'rear',4.2 if e['key']=='Supply' else 3.5,target=(0,0,.5))
    clear()
    for x,e in zip([-1.75,-.2,1.6],sets):
        place(e['key'],'Body',(x,.6,0));place(e['key'],'Lid',(x,-1.0,0))
    render('Actual_Separated_Parts',scale=6.5)
clear()
for x,e in zip([-1.75,-.2,1.6],sets):
    for p in ['Body','Lid']:place(e['key'],p,(x,0,0),False)
cam.location=(4,7,7);aim(cam,(0,0,.2));cam.data.ortho_scale=6.5
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'LootContainerSet.blend'))
print('LOOT_BUILD_PASSED',manifest['unique_triangles'])
