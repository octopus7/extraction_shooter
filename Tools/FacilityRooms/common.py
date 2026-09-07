"""Reusable Blender authoring primitives for the additive interior collection.

Authoring coordinates are centimetres, X width, -Y front, Z up. Geometry is
stored in metres and exported with FBX unit metadata. No existing file is opened.
"""
import bpy, bmesh, json, math
from pathlib import Path
from mathutils import Vector, Matrix

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / 'TunaSweeper/SourceArt/Environment/FacilityRooms'
TILES = {
    'Concrete':(0,0,0,.85),'Floor':(1,0,.05,.72),'Steel':(2,0,.8,.3),'Iron':(3,0,.65,.48),
    'Teal':(0,1,.2,.48),'Ivory':(1,1,.1,.45),'Rust':(2,1,.3,.82),'Rubber':(3,1,0,.9),
    'Hazard':(0,2,.1,.55),'Wood':(1,2,0,.8),'Screen':(2,2,.05,.3),'Amber':(3,2,.0,.4),
    'Blue':(0,3,.25,.4),'Red':(1,3,.2,.4),'Panel':(2,3,.3,.5),'Paper':(3,3,0,.9),
}
mats={}; parts=[]; colls=[]; objects=[]; entries=[]; current={}; category=''

def init(label):
    global mats, parts, colls, objects, entries, category
    category=label; mats={}; parts=[]; colls=[]; objects=[]; entries=[]
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.context.preferences.filepaths.save_version=0
    scene=bpy.context.scene; scene.unit_settings.system='METRIC'; scene.unit_settings.scale_length=1
    for sub in ['Models','Previews','Manifests']:(OUT/sub).mkdir(parents=True,exist_ok=True)
    atlas=bpy.data.images.load(str(OUT/'Textures/T_FacilityRooms_Atlas.png'))
    atlas.pack();atlas.filepath='//Textures/T_FacilityRooms_Atlas.png'
    for key,(_,_,metal,rough) in TILES.items():
        m=bpy.data.materials.new('M_FacilityRooms_'+key);m.use_nodes=True
        bs=m.node_tree.nodes.get('Principled BSDF');bs.inputs['Metallic'].default_value=metal;bs.inputs['Roughness'].default_value=rough
        tex=m.node_tree.nodes.new('ShaderNodeTexImage');tex.image=atlas
        m.node_tree.links.new(tex.outputs['Color'],bs.inputs['Base Color'])
        if key in {'Screen','Amber'}:
            m.node_tree.links.new(tex.outputs['Color'],bs.inputs['Emission Color'])
            bs.inputs['Emission Strength'].default_value=1.5 if key=='Screen' else 2.0
        mats[key]=m

def begin(short_name,target_size_cm,reference=''):
    global current,parts,colls
    parts=[];colls=[]
    current={'name':'SM_FacilityRooms_'+short_name,'target_size_cm':list(target_size_cm),'category':category,'reference':reference}

def finish(o,name,mat='Teal',bevel=1,smooth=False):
    bpy.ops.object.select_all(action='DESELECT');o.select_set(True);bpy.context.view_layer.objects.active=o
    if o.type!='MESH':bpy.ops.object.convert(target='MESH');o=bpy.context.object
    o.name=current['name']+'_'+name
    bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
    if bevel:
        mod=o.modifiers.new('Soft crafted edge','BEVEL');mod.width=bevel/100;mod.segments=2
        bpy.ops.object.modifier_apply(modifier=mod.name)
    bm=bmesh.new();bm.from_mesh(o.data)
    bmesh.ops.remove_doubles(bm,verts=list(bm.verts),dist=1e-7)
    bmesh.ops.dissolve_degenerate(bm,dist=1e-7,edges=list(bm.edges))
    bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces));bm.to_mesh(o.data);bm.free()
    for p in o.data.polygons:p.use_smooth=smooth
    if smooth:
        mod=o.modifiers.new('Stable corner normals','WEIGHTED_NORMAL');mod.keep_sharp=True
        bpy.ops.object.modifier_apply(modifier=mod.name)
    # Project each face in its dominant plane. Insets prevent neighbouring atlas
    # tiles bleeding into the asset, including the generated image's native size.
    uv=o.data.uv_layers.new(name='UVMap') if not o.data.uv_layers else o.data.uv_layers.active
    coords=[v.co for v in o.data.vertices]
    mins=[min(v[i] for v in coords) for i in range(3)];maxs=[max(v[i] for v in coords) for i in range(3)]
    col,row,_,_=TILES[mat]
    for p in o.data.polygons:
        axis=max(range(3),key=lambda a:abs(p.normal[a]));axes=[a for a in range(3) if a!=axis]
        for li in p.loop_indices:
            co=o.data.vertices[o.data.loops[li].vertex_index].co
            val=[(co[a]-mins[a])/max(maxs[a]-mins[a],1e-8) for a in axes]
            uv.data[li].uv=((col+.055+.89*val[0])/4,(3-row+.055+.89*val[1])/4)
    o.data.materials.clear();o.data.materials.append(mats[mat]);parts.append(o);o.select_set(False)
    return o

def box(name,loc,size,mat='Teal',bevel=1):
    bpy.ops.mesh.primitive_cube_add(size=1,location=Vector(loc)/100);o=bpy.context.object;o.dimensions=Vector(size)/100
    return finish(o,name,mat,bevel,True)

def cylinder(name,loc,radius,depth,mat='Ivory',axis=(0,0,1),vertices=32,bevel=.3):
    bpy.ops.mesh.primitive_cylinder_add(vertices=vertices,radius=radius/100,depth=depth/100,location=Vector(loc)/100)
    o=bpy.context.object;o.rotation_euler=Vector(axis).to_track_quat('Z','Y').to_euler()
    return finish(o,name,mat,bevel,True)

def sphere(name,loc,size,mat='Ivory'):
    bpy.ops.mesh.primitive_uv_sphere_add(segments=24,ring_count=12,radius=1,location=Vector(loc)/100)
    o=bpy.context.object;o.dimensions=Vector(size)/100
    return finish(o,name,mat,0,True)

def ring(name,loc,major,minor,mat='Steel',axis=(0,0,1),scale=(1,1,1)):
    bpy.ops.mesh.primitive_torus_add(major_segments=40,minor_segments=8,major_radius=major/100,minor_radius=minor/100,location=Vector(loc)/100)
    o=bpy.context.object;o.scale=scale;o.rotation_euler=Vector(axis).to_track_quat('Z','Y').to_euler()
    return finish(o,name,mat,0,True)

def pipe(name,points,radius,mat='Steel'):
    data=bpy.data.curves.new(name,'CURVE');data.dimensions='3D';data.resolution_u=8;data.bevel_depth=radius/100;data.bevel_resolution=2;data.use_fill_caps=True
    spl=data.splines.new('BEZIER');spl.bezier_points.add(len(points)-1)
    for p,co in zip(spl.bezier_points,points):p.co=Vector(co)/100;p.handle_left_type='AUTO';p.handle_right_type='AUTO'
    o=bpy.data.objects.new(name,data);bpy.context.collection.objects.link(o)
    return finish(o,name,mat,0,True)

def surface(name,verts_cm,faces,mat='Ivory',bevel=0,smooth=False):
    me=bpy.data.meshes.new(name);me.from_pydata([Vector(v)/100 for v in verts_cm],[],faces);me.update()
    o=bpy.data.objects.new(name,me);bpy.context.collection.objects.link(o)
    return finish(o,name,mat,bevel,smooth)

def lathe(name,profile,loc=(0,0,0),mat='Ivory',scale=(1,1,1),segments=48):
    """Closed radial cross-section (radius,z) swept about local Z, cm."""
    verts=[];faces=[]
    for radius,z in profile:
        for k in range(segments):
            a=2*math.pi*k/segments
            verts.append((loc[0]+math.cos(a)*radius*scale[0],loc[1]+math.sin(a)*radius*scale[1],loc[2]+z*scale[2]))
    for j in range(len(profile)):
        nxt=(j+1)%len(profile)
        for k in range(segments):faces.append((j*segments+k,j*segments+(k+1)%segments,nxt*segments+(k+1)%segments,nxt*segments+k))
    return surface(name,verts,faces,mat,0,True)

def collision(loc,size):
    colls.append((list(loc),list(size)))

def end():
    assert parts,current
    bpy.ops.object.select_all(action='DESELECT')
    for o in parts:o.select_set(True)
    bpy.context.view_layer.objects.active=parts[0];bpy.ops.object.join();o=bpy.context.object;o.name=current['name']
    bpy.ops.object.transform_apply(location=False,rotation=True,scale=True)
    bpy.context.scene.cursor.location=(0,0,0);bpy.ops.object.origin_set(type='ORIGIN_CURSOR')
    co=[v.co.copy() for v in o.data.vertices];mins=Vector([min(v[i] for v in co) for i in range(3)]);maxs=Vector([max(v[i] for v in co) for i in range(3)])
    center=Vector(((mins.x+maxs.x)/2,(mins.y+maxs.y)/2,mins.z));scale=Vector([current['target_size_cm'][i]/100/(maxs[i]-mins[i]) for i in range(3)])
    for v in o.data.vertices:v.co=Vector([(v.co[i]-center[i])*scale[i] for i in range(3)])
    bm=bmesh.new();bm.from_mesh(o.data);bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces));bm.to_mesh(o.data);bm.free()
    o.data.update();o.data.calc_loop_triangles()
    assert all(math.isfinite(x) for v in o.data.vertices for x in v.co),o.name
    assert all(t.area>1e-13 for t in o.data.loop_triangles),(o.name,'degenerate triangle')
    # Consolidate same-name material slots after joining independent parts.
    old=list(o.data.materials);unique=[];remap={}
    for i,m in enumerate(old):
        if m not in unique:unique.append(m)
        remap[i]=unique.index(m)
    indices=[remap[p.material_index] for p in o.data.polygons];o.data.materials.clear()
    for m in unique:o.data.materials.append(m)
    for p,i in zip(o.data.polygons,indices):p.material_index=i
    # Author a non-overlapping lightmap UV1; keep atlas UV0 active for material sampling.
    uv0=o.data.uv_layers.active
    uv1=o.data.uv_layers.new(name='LightmapUV');o.data.uv_layers.active=uv1
    bpy.ops.object.mode_set(mode='EDIT');bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.uv.smart_project(angle_limit=math.radians(66),island_margin=.015)
    bpy.ops.object.mode_set(mode='OBJECT');o.data.uv_layers.active=uv0
    sx,sy,sz=current['target_size_cm'];entry=dict(current)
    entry.update(size_cm=[sx,sy,sz],bounds_cm=[-sx/2,-sy/2,0,sx/2,sy/2,sz],triangles=len(o.data.loop_triangles),materials=[m.name for m in unique])
    export_coll=[]
    for idx,(loc,size) in enumerate(colls):
        loc=Vector([(loc[i]/100-center[i])*scale[i] for i in range(3)]);size=Vector([size[i]/100*scale[i] for i in range(3)])
        bpy.ops.mesh.primitive_cube_add(size=1,location=loc);c=bpy.context.object;c.name='UCX_'+o.name+'_%02d'%idx;c.dimensions=size
        bpy.ops.object.transform_apply(location=False,rotation=False,scale=True);export_coll.append(c)
    entry['collision_boxes']=len(export_coll)
    entry['collision_specs_cm']=[{'center':[(loc[i]/100-center[i])*scale[i]*100 for i in range(3)],'size':[size[i]*scale[i] for i in range(3)]} for loc,size in colls]
    entry['uv_channels']=len(o.data.uv_layers)
    if not export_coll:entry.pop('collision_boxes')
    bpy.ops.object.select_all(action='DESELECT');o.select_set(True)
    for c in export_coll:c.select_set(True)
    bpy.context.view_layer.objects.active=o
    bpy.ops.export_scene.fbx(filepath=str(OUT/'Models'/f'{o.name}.fbx'),use_selection=True,object_types={'MESH'},apply_unit_scale=True,apply_scale_options='FBX_SCALE_NONE',axis_forward='-Y',axis_up='Z',mesh_smooth_type='FACE',add_leaf_bones=False,bake_anim=False,path_mode='STRIP')
    for c in export_coll:bpy.data.objects.remove(c,do_unlink=True)
    o.select_set(False);objects.append(o);entries.append(entry)
    return o

def complete():
    manifest={'category':category,'axis_contract':'Blender X width/-Y front/Z up -> UE X width/+Y front/Z up; floor-centred pivots, cm', 'materials':{k:{'name':mats[k].name,'metallic':v[2],'roughness':v[3],'emission':(1.5 if k=='Screen' else 2.0 if k=='Amber' else 0.0)} for k,v in TILES.items()},'assets':entries,'total_triangles':sum(e['triangles'] for e in entries)}
    (OUT/'Manifests'/f'{category}.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
    # Display copies are in a single reusable source scene; exports keep local pivots.
    columns=4;spacing=3.5
    for idx,o in enumerate(objects):o.location=((idx%columns-(columns-1)/2)*spacing,(idx//columns)*spacing,0)
    scene=bpy.context.scene;scene.render.engine='CYCLES';scene.cycles.samples=24;scene.cycles.use_denoising=True
    scene.render.resolution_x=1800;scene.render.resolution_y=max(900,500*math.ceil(len(objects)/columns));scene.render.resolution_percentage=100
    scene.view_settings.view_transform='AgX';scene.world=bpy.data.worlds.new('WarmStudio');scene.world.use_nodes=True;scene.world.node_tree.nodes['Background'].inputs['Strength'].default_value=.6
    bpy.ops.mesh.primitive_plane_add(size=200,location=(0,0,-.015));floor=bpy.context.object;floor.name='PREVIEW_Floor'
    m=bpy.data.materials.new('PREVIEW_MatteCream');m.diffuse_color=(.32,.28,.23,1);m.use_nodes=True;m.node_tree.nodes['Principled BSDF'].inputs['Base Color'].default_value=(.32,.28,.23,1);m.node_tree.nodes['Principled BSDF'].inputs['Roughness'].default_value=.9;floor.data.materials.append(m)
    target=Vector((0,(math.ceil(len(objects)/columns)-1)*spacing/2,.25))
    for name,loc,power,size in [('Key',(-7,-6,12),2600,9),('Fill',(8,-1,8),1800,8),('Rim',(0,10,12),3000,8)]:
        d=bpy.data.lights.new(name,'AREA');d.energy=power;d.shape='DISK';d.size=size;o=bpy.data.objects.new('PREVIEW_'+name,d);scene.collection.objects.link(o);o.location=loc;o.rotation_euler=(target-o.location).to_track_quat('-Z','Y').to_euler()
    d=bpy.data.cameras.new('ReviewCamera');cam=bpy.data.objects.new('PREVIEW_Camera',d);scene.collection.objects.link(cam);cam.location=target+Vector((6,-12,15));cam.rotation_euler=(target-cam.location).to_track_quat('-Z','Y').to_euler();d.type='ORTHO';d.ortho_scale=16.5;scene.camera=cam
    bpy.ops.wm.save_as_mainfile(filepath=str(OUT/f'FacilityRooms_{category}.blend'))
    scene.render.filepath=str(OUT/'Previews'/f'{category}.png');bpy.ops.render.render(write_still=True)
    print('INTERIOR_CATEGORY_COMPLETE '+category+' '+str(len(entries))+' meshes '+str(manifest['total_triangles'])+' triangles')
