"""One-off authoring of the existing ATV as a rigid mechanical skeletal mesh."""
import bpy, bmesh, json, math, hashlib
from pathlib import Path
from mathutils import Vector, Matrix, Quaternion

ROOT=Path('D:/github/extraction_shooter')
OUT=ROOT/'TunaSweeper/SourceArt/Vehicles/ATV'
OUT.mkdir(parents=True,exist_ok=True)
(OUT/'Previews').mkdir(exist_ok=True)
SOURCE=ROOT/'Blender/SM_ATV.blend'
SOURCE_HASH=hashlib.sha256(SOURCE.read_bytes()).hexdigest()
bpy.ops.wm.open_mainfile(filepath=str(SOURCE))
src=bpy.data.objects['SM_ATV']; mesh=src.data
source_report=json.loads((ROOT/'TunaSweeper/Saved/ATVRigWork/source_unreal.json').read_text())
# Match the existing UE mesh dimensions; canonical front is the round headlights.
SCALE=(source_report['box_extent'][1]*2)/(max(v.co.z for v in mesh.vertices)-min(v.co.z for v in mesh.vertices))
CENTERS={'FL':(-.133,.105,-.154),'FR':(.15,.105,-.139),'RL':(-.126,.107,.184),'RR':(.148,.106,.192)}
def convert(raw):
 x,y,z=raw
 return Vector((-z,-x,y-.0003049965))*SCALE

def classify(c):
 x,h,z=c
 if h>.333:return 'handlebar'
 for key,(sx,sh,sz) in CENTERS.items():
  side=-1 if sx<0 else 1
  radial=((h-sh)/.111)**2+((z-sz)/(.115 if key in ['FL','RL','RR'] else .108))**2
  inner=(.045 if side<0 else .057)+.037*min(radial,1.0)**.6
  if x*side>inner and h<.216 and radial<1.09:
   if key.startswith('F') and z<-.244 and h>.105 and x*side<.11:continue
   if key.startswith('R') and z<.089 and h>.09 and x*side<.127:continue
   return 'wheel_'+key
 # The original one-piece scan fused the shocks/links into the wheel bays.
 # Rebuild only these occluded mechanical connections with articulating parts.
 for key,(sx,sh,sz) in CENTERS.items():
  if .026<abs(x)<.101 and .061<h<.228 and abs(z-sz)<.073:
   if x*sx>0:return 'replace_mechanics'
 return 'root'

labels={p.index:classify(p.center) for p in mesh.polygons}
edge_faces={}
for p in mesh.polygons:
 for edge in p.edge_keys:edge_faces.setdefault(edge,[]).append(p.index)
adj={p.index:set() for p in mesh.polygons}
for ids in edge_faces.values():
 for i in ids:adj[i].update(set(ids)-{i})
for key,center in CENTERS.items():
 label='wheel_'+key;remaining={i for i in labels if labels[i]==label};groups=[]
 while remaining:
  stack=[remaining.pop()];group=set(stack)
  while stack:
   for i in adj[stack.pop()]&remaining:
    remaining.remove(i);group.add(i);stack.append(i)
  groups.append(group)
 biggest=max(groups,key=len)
 for group in groups:
  mean=sum((mesh.polygons[i].center for i in group),Vector())/len(group)
  near_hub=((mean.y-center[1])/.08)**2+((mean.z-center[2])/.08)**2<1
  if group is not biggest and not near_hub:
   for i in group:labels[i]='root'

def material(name,color,metallic,roughness):
 m=bpy.data.materials.get(name) or bpy.data.materials.new(name)
 m.use_nodes=True;m.diffuse_color=(*color,1)
 p=m.node_tree.nodes.get('Principled BSDF');p.inputs['Base Color'].default_value=(*color,1);p.inputs['Metallic'].default_value=metallic;p.inputs['Roughness'].default_value=roughness
 return m
DARK=material('M_ATV_DarkSteel',(.055,.061,.067),.65,.45)
GOLD=material('M_ATV_ShockGold',(.32,.15,.045),.65,.34)
CHROME=material('M_ATV_Chrome',(.33,.37,.40),.82,.25)
parts=[];rigspec={};part_report=[]
def add_spec(name,head,tail=None,parent='root',deform=True):
 h=Vector(head);t=Vector(tail) if tail is not None else h+Vector((8,0,0))
 rigspec[name]={'head':h,'tail':t,'parent':parent,'deform':deform}
add_spec('root',(0,0,0),parent=None)
add_spec('handlebar',convert((.008,.332,-.075)))

def register(obj,bone,mat=None):
 obj.name='ATV_'+bone+'_'+str(len(parts))
 if mat:
  obj.data.materials.clear();obj.data.materials.append(mat)
 vg=obj.vertex_groups.new(name=bone);vg.add(list(range(len(obj.data.vertices))),1.0,'REPLACE')
 obj['rig_part']=bone
 parts.append(obj)
 return obj

for name in ['root','handlebar']+['wheel_'+k for k in CENTERS]:
 obj=src.copy();obj.data=mesh.copy();bpy.context.scene.collection.objects.link(obj);obj.matrix_world=Matrix.Identity(4)
 bm=bmesh.new();bm.from_mesh(obj.data);bm.faces.ensure_lookup_table()
 oldboundary=bm.edges.layers.int.new('old_boundary')
 for edge in bm.edges:edge[oldboundary]=int(edge.is_boundary)
 bmesh.ops.delete(bm,geom=[f for f in bm.faces if labels[f.index]!=name],context='FACES')
 bmesh.ops.delete(bm,geom=[v for v in bm.verts if not v.link_faces],context='VERTS')
 cap_edges=[e for e in bm.edges if e.is_boundary and not e[oldboundary]]
 newfaces=bmesh.ops.holes_fill(bm,edges=cap_edges,sides=0).get('faces',[])
 cap_index=len(obj.data.materials);obj.data.materials.append(DARK)
 for face in newfaces:
  face.material_index=cap_index;face.smooth=False;face.normal_update()
  skip=max(range(3),key=lambda a:abs(face.normal[a]));axes=[a for a in range(3) if a!=skip]
  for layer in bm.loops.layers.uv.values():
   for loop in face.loops:loop[layer].uv=(loop.vert.co[axes[0]]*2+.5,loop.vert.co[axes[1]]*2+.5)
 bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces))
 for v in bm.verts:v.co=convert(v.co)
 bm.to_mesh(obj.data);bm.free();obj.data.update()
 register(obj,name)
 part_report.append({'part':name,'source_faces':sum(v==name for v in labels.values()),'cap_faces':len(newfaces),'vertices':len(obj.data.vertices)})
bpy.data.objects.remove(src,do_unlink=True)

def cylinder(name,a,b,r,mat,bone,vertices=12):
 a=Vector(a);b=Vector(b);delta=b-a
 bpy.ops.mesh.primitive_cylinder_add(vertices=vertices,radius=r,depth=delta.length,end_fill_type='NGON',location=(a+b)/2)
 obj=bpy.context.object;obj.name=name;obj.rotation_euler=delta.to_track_quat('Z','Y').to_euler()
 bpy.ops.object.transform_apply(location=True,rotation=True,scale=True)
 for p in obj.data.polygons:p.use_smooth=len(p.vertices)==4
 register(obj,bone,mat)
 return obj

mechanics={}
for key,rawcenter in CENTERS.items():
 hub=convert(rawcenter);side=1 if hub.y>0 else -1
 add_spec('wheel_'+key,hub)
 lower_root=Vector((hub.x,side*13,hub.z-7))
 upper_root=Vector((hub.x,side*15,hub.z+15))
 lower_end=Vector((hub.x,side*(abs(hub.y)-11),hub.z-7))
 upper_end=Vector((hub.x,side*(abs(hub.y)-11),hub.z+11))
 shocktop=Vector((hub.x-4,side*19,hub.z+34))
 shockbottom=lower_root.lerp(lower_end,.76)+Vector((0,0,3))
 add_spec('lower_arm_'+key,lower_root,lower_end)
 add_spec('upper_arm_'+key,upper_root,upper_end)
 add_spec('knuckle_'+key,hub)
 add_spec('shock_upper_'+key,shocktop,shockbottom)
 add_spec('shock_lower_'+key,shockbottom,shocktop)
 add_spec('spring_'+key,shocktop,shockbottom)
 for level,start,end in [('lower',lower_root,lower_end),('upper',upper_root,upper_end)]:
  for offset in [-8,8]:cylinder(level+'_'+key,start+Vector((offset,0,0)),end,1.15,DARK,level+'_arm_'+key)
 cylinder('knuckle_'+key,lower_end,upper_end,2.0,DARK,'knuckle_'+key)
 cylinder('hub_'+key,hub-Vector((0,side*13,0)),hub,2.6,DARK,'knuckle_'+key)
 cylinder('shock_body_'+key,shocktop,shocktop.lerp(shockbottom,.64),3.0,GOLD,'shock_upper_'+key)
 cylinder('shock_rod_'+key,shockbottom,shockbottom.lerp(shocktop,.68),1.15,CHROME,'shock_lower_'+key)
 for point,bone in [(shocktop,'shock_upper_'+key),(shockbottom,'shock_lower_'+key)]:
  cylinder('eye_'+key,point-Vector((2,0,0)),point+Vector((2,0,0)),2.2,DARK,bone)
 # Visible coil follows suspension length; upper cylinder and lower piston stay rigid.
 axis=(shockbottom-shocktop).normalized();u=axis.cross(Vector((1,0,0))).normalized();v=axis.cross(u).normalized()
 verts=[];faces=[];steps=112;ring=6;turns=7
 for i in range(steps+1):
  t=i/steps;angle=t*turns*math.tau
  radial=u*math.cos(angle)+v*math.sin(angle)
  center=shocktop.lerp(shockbottom,.10+.80*t)+radial*4.4
  for j in range(ring):
   q=j/ring*math.tau;verts.append(center+radial*(.65*math.cos(q))+axis*(.65*math.sin(q)))
 for i in range(steps):
  for j in range(ring):faces.append((i*ring+j,i*ring+(j+1)%ring,(i+1)*ring+(j+1)%ring,(i+1)*ring+j))
 faces.extend([tuple(reversed(range(ring))),tuple(steps*ring+j for j in range(ring))])
 data=bpy.data.meshes.new('Coil_'+key);data.from_pydata(verts,[],faces);data.update();obj=bpy.data.objects.new('Coil_'+key,data);bpy.context.scene.collection.objects.link(obj)
 uv=data.uv_layers.new(name='UVMap')
 for p in data.polygons:
  p.use_smooth=True
  if p.index<steps*ring:
   i,j=divmod(p.index,ring);coords=[(j/ring,i/steps),((j+1)/ring,i/steps),((j+1)/ring,(i+1)/steps),(j/ring,(i+1)/steps)]
   for loop,coord in zip(p.loop_indices,coords):uv.data[loop].uv=coord
  else:
   for loop in p.loop_indices:
    j=data.loops[loop].vertex_index%ring;q=j/ring*math.tau;uv.data[loop].uv=(.5+.5*math.cos(q),.5+.5*math.sin(q))
 register(obj,'spring_'+key,DARK)
 mechanics[key]={'hub':hub,'lower_root':lower_root,'lower_end':lower_end,'upper_root':upper_root,'upper_end':upper_end,'shocktop':shocktop,'shockbottom':shockbottom}

add_spec('seat',convert((.009,.310,.065)),parent='root',deform=False)
add_spec('grip_l',convert((-.130,.382,-.080)),parent='handlebar',deform=False)
add_spec('grip_r',convert((.155,.382,-.080)),parent='handlebar',deform=False)
add_spec('foot_l',convert((-.09,.088,.006)),parent='root',deform=False)
add_spec('foot_r',convert((.11,.088,.006)),parent='root',deform=False)

bpy.ops.object.select_all(action='DESELECT')
armdata=bpy.data.armatures.new('SK_ATV');arm=bpy.data.objects.new('Armature',armdata);bpy.context.scene.collection.objects.link(arm);arm.select_set(True);bpy.context.view_layer.objects.active=arm
bpy.ops.object.mode_set(mode='EDIT')
for name,s in rigspec.items():
 bone=armdata.edit_bones.new(name);bone.head=s['head'];bone.tail=s['tail'];bone.roll=0;bone.use_deform=s['deform']
 if s['parent']:bone.parent=armdata.edit_bones[s['parent']]
bpy.ops.object.mode_set(mode='OBJECT');arm.show_in_front=True;armdata.display_type='STICK'
for obj in parts:
 obj.parent=arm;mod=obj.modifiers.new('ATV_RigidSkin','ARMATURE');mod.object=arm

scene=bpy.context.scene;scene.unit_settings.system='METRIC';scene.unit_settings.scale_length=.01;scene.render.fps=30;scene.frame_start=1;scene.frame_end=121
def set_segment(name,a,b,stretch=False):
 rest=rigspec[name];ra=rest['head'];rb=rest['tail'];delta=b-a;rotation=(rb-ra).rotation_difference(delta).to_matrix().to_4x4()
 pose=arm.pose.bones[name];pose.matrix=Matrix.Translation(a)@rotation@Matrix.Translation(-ra)@armdata.bones[name].matrix_local
 if stretch:pose.scale.y=delta.length/(rb-ra).length

def set_pose(steer=0,spin=0,travel=None):
 for p in arm.pose.bones:p.matrix_basis=Matrix.Identity(4)
 arm.pose.bones['handlebar'].matrix=Matrix.Translation(rigspec['handlebar']['head'])@Matrix.Rotation(steer,4,'Z')@Matrix.Translation(-rigspec['handlebar']['head'])@armdata.bones['handlebar'].matrix_local
 for key,m in mechanics.items():
  dz=(travel or {}).get(key,0);shift=Vector((0,0,dz));hub=m['hub']+shift;angle=steer if key.startswith('F') else 0
  orient=Matrix.Rotation(angle,4,'Z')@Matrix.Rotation(spin,4,'Y')
  arm.pose.bones['wheel_'+key].matrix=Matrix.Translation(hub)@orient@Matrix.Translation(-m['hub'])@armdata.bones['wheel_'+key].matrix_local
  arm.pose.bones['knuckle_'+key].matrix=Matrix.Translation(hub)@Matrix.Rotation(angle,4,'Z')@Matrix.Translation(-m['hub'])@armdata.bones['knuckle_'+key].matrix_local
  lower=m['lower_end']+shift;upper=m['upper_end']+shift
  set_segment('lower_arm_'+key,m['lower_root'],lower,True);set_segment('upper_arm_'+key,m['upper_root'],upper,True)
  bottom=m['lower_root'].lerp(lower,.76)+Vector((0,0,3))
  set_segment('shock_upper_'+key,m['shocktop'],bottom)
  set_segment('shock_lower_'+key,bottom,m['shocktop'])
  set_segment('spring_'+key,m['shocktop'],bottom,True)
 bpy.context.view_layer.update()

# A baked inspection action, not runtime suspension logic.
for frame in range(1,122):
 scene.frame_set(frame);phase=(frame-1)/120
 steer=math.radians(24)*math.sin(phase*math.tau)
 travel={k:7*math.sin(phase*math.tau+(0 if k in ['FL','RR'] else math.pi)) for k in CENTERS}
 set_pose(steer,phase*math.tau*2,travel)
 for p in arm.pose.bones:
  p.rotation_mode='QUATERNION'
  p.keyframe_insert('location',frame=frame);p.keyframe_insert('rotation_quaternion',frame=frame);p.keyframe_insert('scale',frame=frame)
arm.animation_data.action.name='ATV_RigCheck'
action=arm.animation_data.action
arm.animation_data.action=None;scene.frame_set(1);set_pose()
for obj in parts:
 obj.data.calc_loop_triangles()
 # All generated mechanical UVs are deliberately independent of the source atlas.
 if not obj.data.uv_layers:obj.data.uv_layers.new(name='UVMap')

# Export centimeters, identity object scale, +X bone forward and +Z up.
bpy.ops.object.select_all(action='DESELECT')
for obj in parts+[arm]:obj.select_set(True)
bpy.context.view_layer.objects.active=arm
export=dict(use_selection=True,object_types={'MESH','ARMATURE'},axis_forward='-Y',axis_up='Z',apply_unit_scale=True,apply_scale_options='FBX_SCALE_NONE',add_leaf_bones=False,primary_bone_axis='X',secondary_bone_axis='-Y',use_armature_deform_only=False,bake_space_transform=False,mesh_smooth_type='FACE',use_mesh_modifiers=True,path_mode='COPY',embed_textures=True)
bpy.ops.export_scene.fbx(filepath=str(OUT/'SKM_ATV.fbx'),bake_anim=False,**export)
arm.animation_data.action=action
bpy.ops.export_scene.fbx(filepath=str(OUT/'ATV_RigCheck.fbx'),bake_anim=True,bake_anim_use_all_actions=False,bake_anim_use_nla_strips=False,bake_anim_simplify_factor=0.0,**export)
scene.frame_set(1)

# Store packed authoring scene with editable separate parts and the inspection action.
for img in bpy.data.images:
 if img.source=='FILE' and img.has_data and not img.packed_file:img.pack()
arm['axis_contract']='UE +X forward, +Y right, +Z up; centimeters; wheels are direct root children.'
arm['animation_note']='ATV_RigCheck is a baked inspection action; runtime Chaos/Control Rig integration is separate.'
manifest={'source':'Blender/SM_ATV.blend','source_sha256':SOURCE_HASH,'source_polygons':len(labels),'discarded_fused_mechanics_faces':sum(v=='replace_mechanics' for v in labels.values()),'source_to_cm_scale':SCALE,'axis_contract':'raw source (-Z,-X,+Y) -> Blender (+X forward,+Y left,+Z up); FBX -> UE (+X forward,+Y right,+Z up)','parts':part_report,'bones':[{ 'name':n,'parent':s['parent'],'head_cm':list(s['head']),'tail_cm':list(s['tail']),'deform':s['deform']} for n,s in rigspec.items()],'wheel_order':['FL','FR','RL','RR'],'wheel_centers_blender_cm':{k:list(m['hub']) for k,m in mechanics.items()},'triangles':sum(len(o.data.loop_triangles) for o in parts),'mesh_objects':len(parts),'inspection_animation':{'frames':121,'fps':30,'steering_degrees':24,'suspension_travel_cm':7},'materials':[{'name':m.name,'base_color':list(m.node_tree.nodes.get('Principled BSDF').inputs['Base Color'].default_value),'metallic':m.node_tree.nodes.get('Principled BSDF').inputs['Metallic'].default_value,'roughness':m.node_tree.nodes.get('Principled BSDF').inputs['Roughness'].default_value} for m in [DARK,GOLD,CHROME]]}
(OUT/'rig_manifest.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
bpy.ops.wm.save_as_mainfile(filepath=str(ROOT/'Blender/SKM_ATV.blend'))
assert hashlib.sha256(SOURCE.read_bytes()).hexdigest()==SOURCE_HASH
print('ATV_RIG_BUILT',json.dumps({'bones':len(rigspec),'parts':len(parts),'triangles':manifest['triangles']}))
