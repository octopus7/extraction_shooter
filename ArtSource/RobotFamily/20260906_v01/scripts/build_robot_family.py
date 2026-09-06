"""Rebuild robot family assets with Blender 4.5 LTS, no add-ons required.
blender --background --factory-startup --python build_robot_family.py -- [build|verify|render]
All authored geometry uses centimeters, +X front, +Y right, +Z up.
"""
import bpy, bmesh, math, json, sys
from pathlib import Path
from mathutils import Vector
from mathutils.bvhtree import BVHTree

OUT = Path(__file__).resolve().parents[1]
TEX = OUT / "textures/T_RobotFamily_BaseColor.png"
CONFIGS = {
 "Q1_Scout": dict(n=4, length=150, width=52, bottom=69, height=37, hipz=75, spread=33, knee_dx=23, kneez=43, footx=-3, ankle=12, foot=(28,23,10), link=10, joint=9, ring=[(.0,.82),(.22,1),(.8,.96),(1,.67)]),
 "Q2_Bulwark": dict(n=4, length=155, width=113, bottom=51, height=55, hipz=61, spread=32, knee_dx=16, kneez=33, footx=-4, ankle=13, foot=(39,31,12), link=15, joint=12, ring=[(0,.82),(.18,1),(.64,.94),(1,.70)]),
 "H1_Carrier": dict(n=6, length=220, width=72, bottom=67, height=36, hipz=74, spread=31, knee_dx=19, kneez=43, footx=-4, ankle=12, foot=(28,24,10), link=10, joint=9, ring=[(0,.83),(.18,1),(.82,1),(1,.84)]),
 "B1_Sentry": dict(n=2, length=65, width=59, bottom=94, height=100, hipz=96, spread=10, knee_dx=27, kneez=54, footx=5, ankle=14, foot=(53,30,12), link=15, joint=12, ring=[(0,.65),(.18,.95),(.53,1),(.86,.90),(1,.63)])
}
PARTS=[]; MATERIALS=[]; META={}

def active(ob):
 bpy.ops.object.select_all(action='DESELECT')
 ob.select_set(True); bpy.context.view_layer.objects.active=ob

def finish(ob,name,bone,tile,material=0,bevel=0):
 ob.name=name
 active(ob)
 bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
 if bevel:
  mod=ob.modifiers.new("Manufactured_edge","BEVEL"); mod.width=bevel; mod.segments=2; mod.limit_method='ANGLE'; mod.angle_limit=.35
  bpy.ops.object.modifier_apply(modifier=mod.name)
 bm=bmesh.new();bm.from_mesh(ob.data)
 bmesh.ops.dissolve_degenerate(bm,dist=.0001,edges=list(bm.edges))
 bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces))
 bm.to_mesh(ob.data);bm.free();ob.data.update()
 # Face-projected reusable swatch UVs; islands deliberately overlap between rigid parts.
 uv=ob.data.uv_layers.new(name="UV0") if not ob.data.uv_layers else ob.data.uv_layers[0]
 uv.name='UV0'
 xs=[v.co.x for v in ob.data.vertices]; ys=[v.co.y for v in ob.data.vertices]; zs=[v.co.z for v in ob.data.vertices]
 mins=[min(xs),min(ys),min(zs)]; maxs=[max(xs),max(ys),max(zs)]
 origin=[(.025,.525),(.525,.525),(.025,.025),(.525,.025)][tile]
 for p in ob.data.polygons:
  axis=max(range(3),key=lambda i:abs(p.normal[i])); dims=[i for i in range(3) if i!=axis]
  longest=max(maxs[d]-mins[d] for d in dims) or 1
  for li in p.loop_indices:
   v=ob.data.vertices[ob.data.loops[li].vertex_index].co
   uv.data[li].uv=[origin[k]+.45*(.5+(v[d]-(mins[d]+maxs[d])/2)/longest) for k,d in enumerate(dims)]
 ob.data.materials.append(MATERIALS[material])
 vg=ob.vertex_groups.new(name=bone); vg.add(list(range(len(ob.data.vertices))),1.0,'REPLACE')
 ob["rigid_bone"]=bone; ob["atlas_quadrant"]=tile
 PARTS.append(ob)
 return ob

def box(name,loc,size,bone,tile=0,bevel=2,material=0):
 bpy.ops.mesh.primitive_cube_add(size=1,location=loc); ob=bpy.context.object; ob.dimensions=size
 return finish(ob,name,bone,tile,material,bevel)

def cylinder(name,loc,radius,depth,axis,bone,tile=2,material=1,verts=20):
 bpy.ops.mesh.primitive_cylinder_add(vertices=verts,radius=radius,depth=depth,location=loc)
 ob=bpy.context.object; ob.rotation_mode='QUATERNION'
 ob.rotation_quaternion=Vector(axis).to_track_quat('Z','Y')
 return finish(ob,name,bone,tile,material,.65)

def hull(c):
 # Octagonal cross-section rings form a closed beveled body, tapered at front/back.
 L,W,H=c["length"],c["width"],c["height"]; b=c["bottom"]
 xy=[(-.5,-.34),(-.38,-.5),(.35,-.5),(.5,-.32),(.5,.32),(.35,.5),(-.38,.5),(-.5,.34)]
 vs=[]
 for z,s in c["ring"]:
  for x,y in xy:
   front_taper=1-.22*max(0,x*2) if c["n"]==4 and W<60 else 1
   vs.append((x*L*s,y*W*s*front_taper,b+z*H))
 fs=[tuple(reversed(range(8)))]
 for k in range(len(c["ring"])-1):
  for i in range(8):fs.append((k*8+i,k*8+(i+1)%8,(k+1)*8+(i+1)%8,(k+1)*8+i))
 fs.append(tuple(range((len(c["ring"])-1)*8,len(vs))))
 me=bpy.data.meshes.new("Shell_geometry");me.from_pydata(vs,[],fs);me.update()
 ob=bpy.data.objects.new("Shell",me);bpy.context.collection.objects.link(ob)
 return finish(ob,"Body_Armor","body",0,0,1.3)

def link(name,a,b,width,bone,r0,r1):
 a,b=Vector(a),Vector(b); delta=b-a; u=delta.normalized()
 aa=a+u*(r0*.83);bb=b-u*(r1*.83)
 ob=box(name,(aa+bb)/2,(width,width*.80,(bb-aa).length),bone,1,1.2)
 ob.rotation_mode='QUATERNION';ob.rotation_quaternion=delta.to_track_quat('Z','Y')
 return ob

def material_setup():
 global MATERIALS
 image=bpy.data.images.load(str(TEX),check_existing=True)
 MATERIALS=[]
 for name,metal,rough in [("M_Robot_Painted",.25,.57),("M_Robot_Joints",.7,.4),("M_Robot_Sensor",.15,.25)]:
  m=bpy.data.materials.new(name);m.use_nodes=True
  bs=m.node_tree.nodes.get("Principled BSDF")
  node=m.node_tree.nodes.new("ShaderNodeTexImage");node.image=image;node.interpolation="Linear";node.label="Actual ImageGen shared atlas / UV0"
  m.node_tree.links.new(node.outputs["Color"],bs.inputs["Base Color"])
  bs.inputs["Metallic"].default_value=metal;bs.inputs["Roughness"].default_value=rough
  if name.endswith("Sensor"):
   bs.inputs["Emission Color"].default_value=(1,.24,.015,1);bs.inputs["Emission Strength"].default_value=2.2
  MATERIALS.append(m)
 return image

def make_robot(name,c):
 global PARTS
 bpy.ops.wm.read_factory_settings(use_empty=True); PARTS=[]
 scene=bpy.context.scene;scene.unit_settings.system='METRIC';scene.unit_settings.scale_length=.01;scene.unit_settings.length_unit='CENTIMETERS'
 image=material_setup();hull(c)
 L,W=c["length"],c["width"];b=c["bottom"];h=c["height"]
 box("Body_Undercarriage",(0,0,b+3),(L*.76,W*.75,13),"body",2,4,1)
 # Sensor normal points exactly along +X; layered housing avoids a human face.
 sx=L*.5+1;sz=b+h*(.39 if name!="B1_Sentry" else .53)
 rad=9 if name=="Q1_Scout" else 12
 cylinder("Sensor_recess",(sx,0,sz),rad*1.38,6,(1,0,0),"body",2,1,28)
 cylinder("Sensor_rim",(sx+3.4,0,sz),rad*1.13,2.5,(1,0,0),"body",1,1,28)
 cylinder("Amber_lens",(sx+5,0,sz),rad*.84,1.8,(1,0,0),"body",3,2,28)
 if name=="B1_Sentry":
  box("Identity_stripe",(L*.466,0,b+h*.80),(1.3,10,21),"body",3,.4)
 else:
  box("Identity_stripe",(-L*.16,0,b+h+.6),(L*.19,11,1.3),"body",3,.4)
 if name=="H1_Carrier":
  box("Carrier_case_base",(-10,0,b+h+3),(L*.68,W*.79,6),"body",2,3,1)
  box("Carrier_equipment_case",(-10,0,b+h+12),(L*.68,W*.77,15),"body",0,5)
  box("Carrier_case_mark",(-L*.26,0,b+h+20),(16,11,1.2),"body",3,.4)
 if name=="Q2_Bulwark":
  box("Front_integral_bumper",(L*.435,0,b+9),(13,W*.70,22),"body",0,4)
  # Recess remains forward of bumper.
 legs={}
 positions=[("front",L*.31),("rear",-L*.31)] if c["n"]==4 else ([("front",L*.34),("middle",0),("rear",-L*.34)] if c["n"]==6 else [("main",0)])
 for row,x in positions:
  for side,sign in [("l",-1),("r",1)]:
   tag=row+"_"+side
   hip=Vector((x,sign*(W/2+11),c["hipz"]))
   knee=Vector((x+c["knee_dx"],sign*(W/2+11+c["spread"]*.5),c["kneez"]))
   ankle=Vector((x+c["footx"],sign*(W/2+11+c["spread"]),c["ankle"]))
   axis=(knee-hip).cross(ankle-knee).normalized()
   if axis.y<0:axis=-axis
   legs[tag]=dict(hip=list(hip),knee=list(knee),ankle=list(ankle),axis=list(axis))
   upper="upper_"+tag; lower="lower_"+tag; foot="foot_"+tag
   # Hips use a fixed inboard stub and a freely rotating outboard disk.
   inset,depth=(12,32) if c['n']==2 else (5,16)
   cylinder("Hip_stub_"+tag,hip-axis*(sign*inset),c["joint"]*.65,depth,axis,"body")
   cylinder("Hip_motor_"+tag,hip,c["joint"],c["link"]*.97,axis,upper)
   cylinder("Hip_cap_"+tag,hip+axis*(c["link"]*.59),c["joint"]*.76,2,axis,upper,2,1)
   link("Upper_"+tag,hip,knee,c["link"],upper,c["joint"],c["joint"]*.86)
   cylinder("Knee_motor_"+tag,knee,c["joint"]*.86,c["link"]*.99,axis,lower)
   cylinder("Knee_cap_"+tag,knee+axis*(c["link"]*.61),c["joint"]*.64,2,axis,lower,2,1)
   link("Lower_"+tag,knee,ankle,c["link"]*.84,lower,c["joint"]*.86,c["joint"]*.65)
   cylinder("Ankle_motor_"+tag,ankle,c["joint"]*.65,c["link"],axis,foot)
   fx,fy,fz=c["foot"];fc=ankle+Vector((7 if c["n"]==2 else 0,0,-c["ankle"]+fz/2))
   if name in ("Q1_Scout","H1_Carrier"):
    cylinder("Foot_sole_"+tag,(fc.x,fc.y,2),fx*.52,4,(0,0,1),foot,2,1,12)
    cylinder("Foot_armor_"+tag,(fc.x,fc.y,6.6),fx*.46,5,(0,0,1),foot,0,0,12)
   else:
    box("Foot_sole_"+tag,(fc.x,fc.y,2),(fx,fy,4),foot,2,2,1)
    box("Foot_armor_"+tag,(fc.x,fc.y,7),(fx*.93,fy*.93,7),foot,0,2)
   box("Foot_mark_"+tag,(fc.x+fx*.27,fc.y,10.8),(fx*.18,fy*.38,1.2),foot,3,.4)
 # Make a real armature, direct Upper -> Lower -> Foot per leg.
 ar=bpy.data.armatures.new(name+"_Skeleton");rig=bpy.data.objects.new("Armature",ar);bpy.context.collection.objects.link(rig)
 active(rig);bpy.ops.object.mode_set(mode='EDIT')
 root=ar.edit_bones.new("root");root.head=(0,0,0);root.tail=(0,0,12)
 body=ar.edit_bones.new("body");body.head=(0,0,b);body.tail=(0,0,b+20);body.parent=root
 for tag,d in legs.items():
  u=ar.edit_bones.new("upper_"+tag);u.head=d["hip"];u.tail=d["knee"];u.parent=body;u.align_roll(Vector(d["axis"]))
  lo=ar.edit_bones.new("lower_"+tag);lo.head=d["knee"];lo.tail=d["ankle"];lo.parent=u;lo.use_connect=True;lo.align_roll(Vector(d["axis"]))
  f=ar.edit_bones.new("foot_"+tag);f.head=d["ankle"];f.tail=Vector(d["ankle"])+Vector((18,0,0));f.parent=lo;f.use_connect=True;f.align_roll(Vector(d["axis"]))
 bpy.ops.object.mode_set(mode='OBJECT');rig.show_in_front=True
 # Preserve disjoint islands, consolidate draw object and exactly 3 shared slots.
 bpy.ops.object.select_all(action='DESELECT')
 for ob in PARTS:ob.select_set(True)
 bpy.context.view_layer.objects.active=PARTS[0];bpy.ops.object.join();mesh=bpy.context.object;mesh.name="SK_"+name
 bpy.ops.object.transform_apply(location=True,rotation=True,scale=True)
 mesh.parent=rig
 mod=mesh.modifiers.new("Rigid_bone_deformation","ARMATURE");mod.object=rig;mod.use_deform_preserve_volume=False
 rig["coordinates"]="+X front / +Y right / +Z up; centimeters";rig["leg_count"]=c["n"]
 rig["rig_type"]="Quadruped Upper-Lower-Foot candidate" if c["n"]==4 else "Separate "+str(c["n"])+"-leg rig; existing quadruped solver is not compatible"
 # Pose study, NOT a shipped gait. Every angle rotates around explicit local Z hinge axes.
 scene.frame_start=1;scene.frame_end=36;scene.render.fps=24
 for frame,group in [(1,-1),(12,0),(24,1),(36,-1)]:
  for i,(tag,d) in enumerate(legs.items()):
   raised=group>=0 and i%2==group
   for prefix,angle in [("upper", -12 if raised else 0),("lower",32 if raised else 0),("foot",-20 if raised else 0)]:
    pb=rig.pose.bones[prefix+"_"+tag];pb.rotation_mode='XYZ';pb.rotation_euler=(0,0,math.radians(angle))
    pb.keyframe_insert("rotation_euler",frame=frame,group=tag)
 if rig.animation_data:rig.animation_data.action.name=name+"_JointPoseStudy"
 scene.frame_set(1);bpy.context.view_layer.update()
 image.filepath=str(TEX);image.pack();image.filepath="//textures/T_RobotFamily_BaseColor.png"
 scene.world=bpy.data.worlds.new("StudioWorld");scene.world.use_nodes=True
 scene.world.node_tree.nodes["Background"].inputs[0].default_value=(.18,.21,.25,1)
 scene.world.node_tree.nodes["Background"].inputs[1].default_value=.45
 scene.render.engine='CYCLES';scene.cycles.samples=24;scene.cycles.use_denoising=True
 scene.render.resolution_x=960;scene.render.resolution_y=800;scene.render.resolution_percentage=100
 scene.view_settings.view_transform='AgX'
 scene.render.image_settings.file_format='PNG';scene.render.film_transparent=False
 META[name]={"config":c,"legs":legs,"texture_size":list(image.size),"texture_source":"ImageGen atlas copied byte-for-byte","parts_before_join":len(PARTS)}
 studio_camera(name,(1.1,-1.4,1.05),(0,0,(b+h)*.48),max(L,W+2*c['spread']+50,b+h)*1.37)
 active(mesh)
 bpy.context.preferences.filepaths.save_version=0
 bpy.ops.wm.save_as_mainfile(filepath=str(OUT/(name+".blend")))
 active(mesh);rig.select_set(True)
 bpy.ops.export_scene.fbx(filepath=str(OUT/(name+".fbx")),use_selection=True,object_types={'ARMATURE','MESH'},axis_forward='X',axis_up='Z',apply_unit_scale=True,apply_scale_options='FBX_SCALE_UNITS',add_leaf_bones=False,use_armature_deform_only=False,bake_anim=False,path_mode='COPY',embed_textures=True,use_mesh_modifiers=True)
 return mesh,rig

def inspect(mesh,rig):
 mesh.data.calc_loop_triangles()
 weights=[];missing=[]
 for v in mesh.data.vertices:
  g=[x for x in v.groups if x.weight>1e-6]
  if len(g)!=1 or abs(sum(x.weight for x in g)-1)>1e-5:weights.append(v.index)
  if any(mesh.vertex_groups[x.group].name not in rig.data.bones for x in g):missing.append(v.index)
 uv=mesh.data.uv_layers.active
 uv_bad=sum(not all(math.isfinite(q) and -.0001<=q<=1.0001 for q in x.uv) for x in uv.data) if uv else -1
 bad_faces=sum(p.area<1e-7 for p in mesh.data.polygons)
 # Watertight per rigid primitive; all original component islands should be closed.
 edge_usage={}
 for p in mesh.data.polygons:
  for key in p.edge_keys:edge_usage[key]=edge_usage.get(key,0)+1
 boundary=sum(n!=2 for n in edge_usage.values())
 bounds=[mesh.matrix_world@Vector(v) for v in mesh.bound_box]
 dim=[max(v[i] for v in bounds)-min(v[i] for v in bounds) for i in range(3)]
 out={"vertices":len(mesh.data.vertices),"polygons":len(mesh.data.polygons),"triangles":len(mesh.data.loop_triangles),"bones":len(rig.data.bones),"material_slots":len(mesh.material_slots),"uv_layers":len(mesh.data.uv_layers),"invalid_uv_loops":uv_bad,"invalid_weight_vertices":len(weights),"missing_bone_vertices":len(missing),"degenerate_faces":bad_faces,"nonmanifold_component_edges":boundary,"dimensions_cm":dim,"mesh_scale":list(mesh.scale),"rig_scale":list(rig.scale)}
 assert not weights and not missing and uv and not uv_bad and not bad_faces and not boundary,out
 return out

def pose_collision(mesh,rig):
 # BVH surface crossing between NON-adjacent bones only. Adjacent motor/axle fits are intentional.
 deps=bpy.context.evaluated_depsgraph_get();ev=mesh.evaluated_get(deps);me=ev.to_mesh()
 groups={}; verts=[mesh.matrix_world@v.co for v in me.vertices]
 for p in me.polygons:
  g=mesh.vertex_groups[mesh.data.vertices[p.vertices[0]].groups[0].group].name
  groups.setdefault(g,[]).append(tuple(p.vertices))
 trees={k:BVHTree.FromPolygons(verts,v,all_triangles=False) for k,v in groups.items()}
 hits=[]
 names=list(trees)
 for i,a in enumerate(names):
  for b in names[i+1:]:
   pa=rig.data.bones[a].parent;pb=rig.data.bones[b].parent
   if (pa and pa.name==b) or (pb and pb.name==a):continue
   count=len(trees[a].overlap(trees[b]))
   if count:hits.append({"a":a,"b":b,"surface_triangle_pairs":count})
 ev.to_mesh_clear()
 return hits

def studio_camera(name,angle,target,size):
 scene=bpy.context.scene
 for ob in list(scene.objects):
  if ob.type in {'CAMERA','LIGHT'}:bpy.data.objects.remove(ob,do_unlink=True)
 def aim(ob,point):ob.rotation_euler=(Vector(point)-ob.location).to_track_quat('-Z','Y').to_euler()
 bpy.ops.object.camera_add(location=Vector(target)+Vector(angle).normalized()*650)
 cam=bpy.context.object;cam.name="Preview_Camera";cam.data.type='ORTHO';cam.data.ortho_scale=size;cam.data.lens=45;aim(cam,target);scene.camera=cam
 for label,loc,power,area in [("Key",(220,-300,420),2400000,300),("Fill",(80,250,220),1300000,240),("Rim",(-250,80,340),2100000,220)]:
  bpy.ops.object.light_add(type='AREA',location=loc);ob=bpy.context.object;ob.name=label;ob.data.energy=power;ob.data.shape='DISK';ob.data.size=area;aim(ob,target)

def render_all():
 for name,c in CONFIGS.items():
  if TARGET and name!=TARGET:continue
  bpy.ops.wm.open_mainfile(filepath=str(OUT/(name+".blend")))
  target=(0,0,(c["bottom"]+c["height"])*.48)
  span=max(c["length"],c["width"]+2*c["spread"]+50,c["bottom"]+c["height"])*1.37
  for label,angle,frame in [("hero",(1.1,-1.4,1.05),1),("rear",(-1.1,1.4,.75),1),("front",(1,0,.05),1),("side",(0,-1,.03),1),("top",(0,0,1),1),("pose",(1.1,-1.4,1.05),12)]:
   bpy.context.scene.frame_set(frame);studio_camera(name,angle,target,span)
   bpy.context.scene.render.filepath=str(OUT/"previews"/(name+"_"+label+".png"))
   bpy.ops.render.render(write_still=True)
  print("RENDERED",name,flush=True)

def verify_all():
 report={}
 for name,c in CONFIGS.items():
  bpy.ops.wm.open_mainfile(filepath=str(OUT/(name+".blend")))
  mesh=bpy.data.objects["SK_"+name];rig=bpy.data.objects["Armature"]
  record={"blend":inspect(mesh,rig),"pose_collision_nonadjacent":{}}
  for f in range(1,37):
   bpy.context.scene.frame_set(f);bpy.context.view_layer.update()
   record["pose_collision_nonadjacent"][str(f)]=pose_collision(mesh,rig)
  missing=[]
  for im in bpy.data.images:
   if im.source=='FILE' and not im.packed_file and not Path(bpy.path.abspath(im.filepath)).exists():missing.append(im.filepath)
  record["missing_textures"]=missing;assert not missing
  for tag in META.get(name,{}).get("legs",{}):
   assert rig.data.bones["lower_"+tag].parent.name=="upper_"+tag
   assert rig.data.bones["foot_"+tag].parent.name=="lower_"+tag
  bpy.ops.wm.read_factory_settings(use_empty=True);bpy.context.scene.unit_settings.system='METRIC';bpy.context.scene.unit_settings.scale_length=.01
  bpy.ops.import_scene.fbx(filepath=str(OUT/(name+".fbx")),use_anim=False,automatic_bone_orientation=False)
  mesh=next(o for o in bpy.context.scene.objects if o.type=='MESH');rig=next(o for o in bpy.context.scene.objects if o.type=='ARMATURE')
  record["fbx_reload"]=inspect(mesh,rig)
  record['fbx_texture_nodes']=sum(1 for m in mesh.data.materials for n in m.node_tree.nodes if n.type=='TEX_IMAGE' and n.image and (n.image.packed_file or Path(bpy.path.abspath(n.image.filepath)).exists()))
  assert record['fbx_texture_nodes']>=3
  record["fbx_dimensions_delta_cm"]=[abs(a-b) for a,b in zip(record["blend"]["dimensions_cm"],record["fbx_reload"]["dimensions_cm"])]
  assert max(record["fbx_dimensions_delta_cm"])<.02,record
  assert record["fbx_reload"]["bones"]==2+3*c["n"]
  assert record["fbx_reload"]["triangles"]==record["blend"]["triangles"]
  report[name]=record
  print("VERIFIED",name,json.dumps(record),flush=True)
 (OUT/"validation/report.json").write_text(json.dumps(report,indent=2),encoding="utf-8")

mode=sys.argv[sys.argv.index("--")+1] if "--" in sys.argv else "build"
TARGET=sys.argv[sys.argv.index("--")+2] if "--" in sys.argv and len(sys.argv)>sys.argv.index("--")+2 else None
if mode=="build":
 for name,c in CONFIGS.items():
  mesh,rig=make_robot(name,c);print("BUILT",name,json.dumps(inspect(mesh,rig)),flush=True)
 (OUT/"validation/design_parameters.json").write_text(json.dumps(META,indent=2),encoding="utf-8")
elif mode=="verify":
 META=json.loads((OUT/"validation/design_parameters.json").read_text(encoding="utf-8"));verify_all()
elif mode=="render":render_all()
else:raise ValueError(mode)
