"""Deterministic mesh-only foliage, Blender 4.5 LTS. No external packages.
blender -b --factory-startup -t 4 --python-exit-code 1 --python <this file>
"""
from pathlib import Path
import bpy, bmesh, math, random, json, struct, zlib
from mathutils import Vector
ROOT=Path(__file__).resolve().parents[3]
OUT=ROOT/'TunaSweeper/SourceArt/ForestProps/BushSpreading'
NAME='SM_BushSpreading'
random.seed(90637)
bpy.ops.wm.read_factory_settings(use_empty=True)
scene=bpy.context.scene
scene.unit_settings.system='METRIC';scene.unit_settings.scale_length=1
scene.render.engine='CYCLES';scene.cycles.samples=24;scene.cycles.use_denoising=True
scene.render.resolution_x=1280;scene.render.resolution_y=960;scene.render.resolution_percentage=100
scene.view_settings.view_transform='AgX'
scene.world=bpy.data.worlds.new('Soft studio');scene.world.use_nodes=True
scene.world.node_tree.nodes['Background'].inputs[0].default_value=(.60,.68,.72,1)
scene.world.node_tree.nodes['Background'].inputs[1].default_value=.5
for folder in ['Textures','Previews']: (OUT/folder).mkdir(parents=True,exist_ok=True)
# Calm sRGB color fields; no alpha, normal map, veins, speckles or microtexture.
palette=['48613D','587447','6A8052','506C40','627449','405536','806344','927450']
pixels=bytearray()
for y in range(128):
    for x in range(128):
        hx=palette[(y//64)*4+x//32]
        pixels.extend([int(hx[i:i+2],16) for i in (0,2,4)])
# Write explicit sRGB bytes, avoiding generated-image linear/sRGB ambiguity.
def chunk(kind,data):return struct.pack('>I',len(data))+kind+data+struct.pack('>I',zlib.crc32(kind+data)&0xffffffff)
rows=b''.join(b'\0'+pixels[y*384:(y+1)*384] for y in reversed(range(128)))
png=b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',128,128,8,2,0,0,0))+chunk(b'sRGB',b'\0')+chunk(b'IDAT',zlib.compress(rows))+chunk(b'IEND',b'')
texpath=OUT/'Textures/T_BushSpreading_Palette.png';texpath.write_bytes(png)
atlas=bpy.data.images.load(str(texpath));atlas.name='T_BushSpreading_Palette';atlas.pack()
atlas.filepath='//Textures/T_BushSpreading_Palette.png'
mat=bpy.data.materials.new('M_BushSpreading');mat.use_nodes=True
bsdf=mat.node_tree.nodes.get('Principled BSDF');bsdf.inputs['Roughness'].default_value=.86
bsdf.inputs['Specular IOR Level'].default_value=.25
tex=mat.node_tree.nodes.new('ShaderNodeTexImage');tex.image=atlas
mat.node_tree.links.new(tex.outputs['Color'],bsdf.inputs['Base Color'])
verts=[];faces=[];tiles=[];leaf_count=0;branch_count=0
def geometry(points,polys,tile):
    offset=len(verts);verts.extend([tuple(p) for p in points])
    faces.extend([tuple(offset+i for i in f) for f in polys]);tiles.extend([tile]*len(polys))
def branch(points,radii):
    global branch_count
    branch_count+=1
    p=[Vector(x) for x in points];v=[];f=[];sides=5
    for j,point in enumerate(p):
        direction=(p[min(j+1,len(p)-1)]-p[max(0,j-1)]).normalized()
        u=direction.cross(Vector((0,1,0))).normalized();w=direction.cross(u).normalized()
        for i in range(sides):
            a=i*2*math.pi/sides
            v.append(point+radii[j]*(u*math.cos(a)+w*math.sin(a)))
    f.append(tuple(reversed(range(sides))))
    for j in range(len(p)-1):
        for i in range(sides): f.append((j*sides+i,j*sides+(i+1)%sides,(j+1)*sides+(i+1)%sides,(j+1)*sides+i))
    f.append(tuple((len(p)-1)*sides+i for i in range(sides)))
    geometry(v,f,6)
def leaf(base,direction,length,width,tile,roll=0):
    global leaf_count
    leaf_count+=1
    d=Vector(direction).normalized();u=d.cross(Vector((0,0,1))).normalized()
    n=u.cross(d).normalized();u2=u*math.cos(roll)+n*math.sin(roll);n=d.cross(u2)
    if n.z<0:n=-n
    b=Vector(base)
    coords=[(0,0),(.26,-.40),(.66,-.48),(1,0),(.66,.48),(.26,.40)]
    p=[b+d*(x*length)+u2*(y*width) for x,y in coords]
    p.extend([b+d*(.49*length)+n*(length*.095),b+d*(.49*length)-n*(length*.022)])
    f=[]
    for i in range(6):f.extend([(6,i,(i+1)%6),(7,(i+1)%6,i)])
    geometry(p,f,tile)
# Unequal arm lengths and offset crown create a low, laterally spreading bush.
arms=[(8,.91,.37),(48,.65,.49),(93,.48,.41),(141,.67,.43),(179,.79,.31),(220,.72,.34),(266,.49,.46),(314,.80,.32)]
base=Vector((-.11,.035,.018))
for index,(deg,reach,height) in enumerate(arms):
    a=math.radians(deg);radial=Vector((math.cos(a),math.sin(a)*.82,0));side=Vector((-math.sin(a),math.cos(a)*.82,0))
    def point(t):return base+radial*(reach*t)+side*(.065*math.sin(t*math.pi)*(1 if index%2 else -1))+Vector((0,0,height*math.sin(t*1.7)))
    branch([point(t) for t in [0,.25,.55,.8,1]],[.031,.025,.017,.012,.004])
    tile=[1,2,0,1,3,0,1,4][index]
    for j,t in enumerate([.32,.52,.72,.91]):
        for sign in [-1,1]:
            start=point(t)
            heading=radial*.50+side*(sign*.85)+Vector((0,0,random.uniform(.10,.48)))
            length=random.uniform(.25,.33)*(1-.16*t)
            leaf(start,heading,length,length*random.uniform(.56,.69),tile,random.uniform(-.26,.26))
    leaf(point(.96),radial+Vector((0,0,.24)),.24,.15,tile)
    # Secondary leafy fans attach to the woody framework, not floating clumps.
    for sign in [-1,1]:
        start=point(.53)
        heading=(radial*.55+side*(sign*.74)).normalized()
        tip=start+heading*.24+Vector((0,0,.075))
        branch([start,(start+tip)*.5+Vector((0,0,.012)),tip],[.012,.008,.003])
        for t in [.5,1]:
            for s in [-1,1]:
                loc=start.lerp(tip,t)
                direction=heading*.7+Vector((-heading.y,heading.x,0))*(s*.6)+Vector((0,0,.2))
                leaf(loc,direction,.23,.145,tile,random.uniform(-.2,.2))
# Three offset upright shoots close the central gap without making a round ball.
for index,(offset,tip) in enumerate([((-.10,.04,.02),(-.25,.12,.52)),((-.08,.02,.02),(.11,-.11,.46)),((-.10,.04,.02),(-.06,.31,.49))]):
    start=Vector(offset);end=Vector(tip)
    branch([start,start.lerp(end,.48),end],[.026,.016,.004])
    for j,t in enumerate([.45,.68,.91]):
        a=index*2.3+j*.7
        for s in [-1,1]:
            d=Vector((math.cos(a)*s,math.sin(a)*s,.38))
            leaf(start.lerp(end,t),d,.29,.18,[1,3,2][index],.15*s)
    leaf(end,Vector((.3,-.2,.7)),.15,.095,2)
mesh=bpy.data.meshes.new(NAME);mesh.from_pydata(verts,[],faces);mesh.update()
obj=bpy.data.objects.new(NAME,mesh);scene.collection.objects.link(obj)
bpy.context.view_layer.objects.active=obj;obj.select_set(True);mesh.materials.append(mat)
# Each face maps to the interior of a solid palette cell with finite UV area.
uv=mesh.uv_layers.new(name='PaletteUV')
for poly,tile in zip(mesh.polygons,tiles):
    for j,li in enumerate(poly.loop_indices):
        theta=2*math.pi*j/len(poly.loop_indices)
        uv.data[li].uv=((tile%4+.5+.20*math.cos(theta))/4,(tile//4+.5+.20*math.sin(theta))/2)
bm=bmesh.new();bm.from_mesh(mesh);bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces));bmesh.ops.triangulate(bm,faces=list(bm.faces));bm.to_mesh(mesh);bm.free();mesh.update()
# Put the very bottom of the trunk at Z=0; object origin remains at ground.
minimum=min(v.co.z for v in mesh.vertices)
for v in mesh.vertices:v.co.z-=minimum
mesh.uv_layers.new(name='LightmapUV');mesh.uv_layers.active_index=1
bpy.ops.object.mode_set(mode='EDIT');bpy.ops.mesh.select_all(action='SELECT');bpy.ops.uv.smart_project(angle_limit=math.radians(66),island_margin=.02);bpy.ops.object.mode_set(mode='OBJECT')
mesh.uv_layers.active_index=0;mesh.uv_layers[0].active_render=True
def audit(o):
    m=o.data;m.calc_loop_triangles()
    bm=bmesh.new();bm.from_mesh(m)
    bad=sum(1 for e in bm.edges if not e.is_manifold);bm.free()
    bounds=[min((o.matrix_world@v.co)[i] for v in m.vertices) for i in range(3)]+[max((o.matrix_world@v.co)[i] for v in m.vertices) for i in range(3)]
    result={'vertices':len(m.vertices),'triangles':len(m.loop_triangles),'bounds_m':bounds,'dimensions_m':[bounds[i+3]-bounds[i] for i in range(3)],'material_slots':len(m.materials),'uv_channels':len(m.uv_layers),'nonmanifold_edges':bad,'degenerate_faces':sum(p.area<1e-10 for p in m.polygons),'invalid_normals':sum(abs(p.normal.length-1)>1e-4 for p in m.polygons),'uv_nonfinite':sum(not math.isfinite(c) for layer in m.uv_layers for item in layer.data for c in item.uv)}
    assert result['nonmanifold_edges']==result['degenerate_faces']==result['invalid_normals']==result['uv_nonfinite']==0,result
    assert result['material_slots']==1 and result['uv_channels']==2
    return result
bpy.context.view_layer.update()
report={'asset':NAME,'blender':bpy.app.version_string,'seed':90637,'leaves':leaf_count,'branches':branch_count,'palette_srgb':palette,'source':audit(obj),'pivot':[0,0,0],'units':'meters; UE centimeters x100','opaque_closed_leaves':True,'collision':False,'fbx_export':{'axis_forward':'-Y','axis_up':'Z','apply_unit_scale':True,'apply_scale_options':'FBX_SCALE_UNITS','global_scale':1}}
fbx=OUT/(NAME+'.fbx')
bpy.ops.export_scene.fbx(filepath=str(fbx),use_selection=True,object_types={'MESH'},axis_forward='-Y',axis_up='Z',apply_unit_scale=True,apply_scale_options='FBX_SCALE_UNITS',use_mesh_modifiers=True,mesh_smooth_type='FACE',use_tspace=True,bake_anim=False,path_mode='RELATIVE')
obj.select_set(False)
bpy.ops.import_scene.fbx(filepath=str(fbx),use_custom_normals=True)
reload=[o for o in bpy.context.selected_objects if o.type=='MESH'][0]
report['fbx_reload']=audit(reload)
assert report['source']['triangles']==report['fbx_reload']['triangles']
assert max(abs(a-b) for a,b in zip(report['source']['bounds_m'],report['fbx_reload']['bounds_m']))<1e-5
for o in list(bpy.context.selected_objects):bpy.data.objects.remove(o,do_unlink=True)
report['passed']=True
(OUT/'model_validation.json').write_text(json.dumps(report,indent=2))
# Studio helpers are not exported. Repetition is rendered only, never saved as a map.
stage=bpy.data.collections.new('PREVIEW_ONLY');scene.collection.children.link(stage)
def stage_object(o):
    for c in list(o.users_collection):c.objects.unlink(o)
    stage.objects.link(o)
def material(name,color):
    m=bpy.data.materials.new(name);m.diffuse_color=(*color,1);m.use_nodes=True
    m.node_tree.nodes['Principled BSDF'].inputs['Base Color'].default_value=(*color,1)
    m.node_tree.nodes['Principled BSDF'].inputs['Roughness'].default_value=.95
    return m
bpy.ops.mesh.primitive_plane_add(size=200,location=(0,0,-.008));floor=bpy.context.object;floor.name='PREVIEW_Ground';floor.data.materials.append(material('PREVIEW_Ground',(.16,.19,.155)));stage_object(floor)
for pos,power,size in [((-3,-4,6),650,4),((4,2,5),850,5)]:
    bpy.ops.object.light_add(type='AREA',location=pos);light=bpy.context.object;light.data.energy=power;light.data.shape='DISK';light.data.size=size;stage_object(light)
bpy.ops.object.camera_add();cam=bpy.context.object;stage_object(cam);scene.camera=cam;cam.data.type='ORTHO'
def render(name,position,target,scale):
    cam.location=position;cam.rotation_euler=(Vector(target)-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.ortho_scale=scale
    scene.render.filepath=str(OUT/'Previews'/(name+'.png'));bpy.ops.render.render(write_still=True)
render('hero',(2.6,-3.5,2.35),(0,0,.26),2.55)
for name,pos in [('front',(0,-4,1)),('back',(0,4,1)),('side',(4,0,1)),('top',(0,-.001,5))]:render(name,pos,(0,0,.25),2.7)
obj.select_set(True);bpy.context.view_layer.objects.active=obj
cam.location=(2.6,-3.5,2.35);cam.rotation_euler=(Vector((0,0,.26))-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.ortho_scale=2.55
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/(NAME+'.blend')))
instances=[]
for x,y,yaw,s in [(-2,-1.6,.5,.86),(0,-1.8,2.4,1.04),(2,-1.7,5.2,.93),(-2.3,.0,1.8,1.12),(2.1,.2,4.1,.87),(-2,1.6,3.3,.92),(.1,1.7,6.1,1.06),(2.2,1.8,.8,1.0)]:
    dup=bpy.data.objects.new('PREVIEW_Instance',mesh);stage.objects.link(dup);dup.location=(x,y,0);dup.rotation_euler.z=yaw;dup.scale=(s,s,s);instances.append(dup)
render('repeated',(6,-8,8),(0,0,.1),8.9)
print('BUSH_SPREADING_VALIDATION_PASSED',json.dumps(report))
