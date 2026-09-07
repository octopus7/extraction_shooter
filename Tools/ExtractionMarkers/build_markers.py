"""Reproducible Blender 4.5 source; meters/+X north. ImageGen supplies bitmap art."""
import bpy,bmesh,math,json,os,shutil
from pathlib import Path
from mathutils import Vector,Matrix
ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'TunaSweeper/SourceArt/Environment/ExtractionMarkers'
REF=ROOT/'TunaSweeper/SourceArt/Environment/ModularInteriorExpansion/Models'
BASE=ROOT/'TunaSweeper/SourceArt/Environment/ModularInteriorPreview/Textures'
for d in ('Models','Previews'): (OUT/d).mkdir(exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True)
s=bpy.context.scene;s.unit_settings.system='METRIC';s.render.engine='CYCLES';s.cycles.samples=24;s.cycles.use_denoising=True
s.render.threads_mode='FIXED';s.render.threads=10
s.render.resolution_x=1200;s.render.resolution_y=900;s.render.resolution_percentage=100
s.world=bpy.data.worlds.new('Day');s.world.use_nodes=True;s.world.node_tree.nodes['Background'].inputs[0].default_value=(.65,.70,.78,1);s.world.node_tree.nodes['Background'].inputs[1].default_value=.6
s.view_settings.view_transform='AgX'
atlas=bpy.data.images.load(str(OUT/'Textures/ImageGen_Atlas_Original.png'));atlas.scale(1024,1024);atlas.filepath_raw=str(OUT/'Textures/T_EM_Atlas.png');atlas.file_format='PNG';atlas.save();atlas.pack();atlas.filepath='//Textures/T_EM_Atlas.png'
mask=bpy.data.images.load(str(BASE/'T_MI_DirtMask.png'));mask.colorspace_settings.name='Non-Color';mask.pack()
steelimg=bpy.data.images.load(str(BASE/'T_MI_Atlas.png'));steelimg.pack()
def material(name,img=None,color=(.1,.8,.25,1),emission=0):
 m=bpy.data.materials.new(name);m.use_nodes=True;n=m.node_tree.nodes;l=m.node_tree.links;b=n.get('Principled BSDF');b.inputs['Base Color'].default_value=color;b.inputs['Roughness'].default_value=.58;b.inputs['Metallic'].default_value=.2
 if img:
  t=n.new('ShaderNodeTexImage');t.image=img;uv=n.new('ShaderNodeUVMap');uv.uv_map='DirtUV';d=n.new('ShaderNodeTexImage');d.image=mask;l.new(uv.outputs[0],d.inputs[0]);mult=n.new('ShaderNodeMath');mult.operation='MULTIPLY';mult.inputs[1].default_value=.25;l.new(d.outputs[0],mult.inputs[0]);mix=n.new('ShaderNodeMixRGB');mix.inputs[2].default_value=(.10,.085,.065,1);l.new(mult.outputs[0],mix.inputs[0]);l.new(t.outputs[0],mix.inputs[1]);l.new(mix.outputs[0],b.inputs['Base Color'])
 else:b.inputs['Emission Color'].default_value=color;b.inputs['Emission Strength'].default_value=emission
 return m
surface=material('M_EM_Surface',atlas);green=material('M_EM_Green',color=(.025,.65,.10,1),emission=2)
steel=material('M_MI_Steel',steelimg);amber=material('M_MI_ExpansionAmber',color=(1,.19,.025,1),emission=3)
assets={}
def reflect(o):
 o.data.transform(Matrix.Diagonal((1,-1,1,1)));bm=bmesh.new();bm.from_mesh(o.data);bmesh.ops.reverse_faces(bm,faces=list(bm.faces));bm.to_mesh(o.data);bm.free()
def load_reference(name):
 bpy.ops.import_scene.fbx(filepath=str(REF/(name+'.fbx')),use_custom_normals=True)
 o=next(o for o in bpy.context.selected_objects if o.type=='MESH' and not o.name.startswith('UCX'))
 bpy.context.view_layer.objects.active=o;bpy.ops.object.transform_apply(location=True,rotation=True,scale=True);reflect(o)
 return o
light=load_reference('SM_MIE_EmergencyLight');light.name='SM_MIE_EmergencyLight';light.data.materials.clear();light.data.materials.append(steel);light.data.materials.append(amber)
for p in light.data.polygons:p.material_index=1 if p.normal.y<-.9 or p.normal.z>.9 else 0
assets[light.name]=light
sign=load_reference('SM_MIE_ZoneSign');sign.name='SM_EM_DirectionSign';sign.data.materials.clear();sign.data.materials.append(surface)
# Reuse the existing closed 12-triangle sign body, widen its short axis, rotate to slope.
a=math.radians(20)
for v in sign.data.vertices:
 old=v.co.copy();x=(old.z/.5-.5)*.65;y=(old.x-.5)*.9;z=-old.y
 v.co=(x*math.cos(a)-z*math.sin(a),y,.16+x*math.sin(a)+z*math.cos(a))
bm=bmesh.new();bm.from_mesh(sign.data);bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces));bm.to_mesh(sign.data);bm.free();sign.data.update()
def box(name,lo,hi,mat=surface):
 bpy.ops.mesh.primitive_cube_add(size=1,location=[(x+y)/2 for x,y in zip(lo,hi)]);o=bpy.context.object;o.name=name;o.dimensions=[y-x for x,y in zip(lo,hi)];bpy.ops.object.transform_apply(location=True,rotation=True,scale=True);o.data.materials.append(mat);return o
def join(objects,name):
 bpy.ops.object.select_all(action='DESELECT')
 for o in objects:o.select_set(True)
 bpy.context.view_layer.objects.active=objects[0];bpy.ops.object.join();o=objects[0];o.name=name;return o
feet=[box('Foot',(-.34,y,0),(.34,y+.065,.055)) for y in (-.40,.335)]
# Feet have rising rear supports meeting the underside of the sloped reused panel.
supports=[box('Support',(.23,y,.055),(.28,y+.065,.2386)) for y in (-.40,.335)]
sign=join([sign]+feet+supports,'SM_EM_DirectionSign');assets[sign.name]=sign
def cylinder(name,r,z0,z1,mat):
 bpy.ops.mesh.primitive_cylinder_add(vertices=8,radius=r,depth=z1-z0,location=(0,0,(z0+z1)/2));o=bpy.context.object;o.name=name;bpy.ops.object.transform_apply(location=True,rotation=True,scale=True);o.data.materials.append(mat);return o
parts=[box('Base',(-.3,-.3,0),(.3,.3,.14)),cylinder('Collar',.215,.14,.20,surface),cylinder('Body',.19,.20,.48,surface),cylinder('Lens',.19,.48,.55,green)]
parts += [box('HandleAnchor',(-.06,y,.055),(.06,y+.035,.095)) for y in (-.37,-.335)]
parts += [box('HandleGrip',(-.13,-.385,.055),(.13,-.35,.095))]
beacon=join(parts,'SM_EM_Beacon');assets[beacon.name]=beacon
regions={'gray':(.025,.525,.475,.975),'green':(.525,.525,.975,.975),'arrow':(.012,.012,.488,.488),'vent':(.525,.025,.975,.475)}
entries=[]
for name,o in assets.items():
 if name!='SM_MIE_EmergencyLight':
  for layer in list(o.data.uv_layers):o.data.uv_layers.remove(layer)
  uv=o.data.uv_layers.new(name='UVMap');duv=o.data.uv_layers.new(name='DirtUV')
  for p in o.data.polygons:
   axis=max(range(3),key=lambda i:abs(p.normal[i]));axes=[i for i in range(3) if i!=axis]
   region='gray'
   if name=='SM_EM_DirectionSign' and p.center.z>.075 and p.normal.z>.9 and p.area>.04:region='arrow';axes=[1,0]
   if name=='SM_EM_Beacon' and p.center.z>.20:region='green'
   if name=='SM_EM_Beacon' and p.normal.x<-.9 and p.center.z<.15:region='vent'
   coords=[o.data.vertices[o.data.loops[li].vertex_index].co for li in p.loop_indices]
   # Use global panel bounds across its imported triangles; no arrow duplication per triangle.
   if region=='arrow':ranges=[(-.45,.45),(-.325*math.cos(a)-.02*math.sin(a),.325*math.cos(a)-.02*math.sin(a))]
   else:ranges=[(min(v[i] for v in coords),max(v[i] for v in coords)) for i in axes]
   r=regions[region]
   for li,v in zip(p.loop_indices,coords):
    t=[(v[i]-lo)/max(hi-lo,1e-8) for i,(lo,hi) in zip(axes,ranges)]
    uv.data[li].uv=(r[0]+t[0]*(r[2]-r[0]),r[1]+t[1]*(r[3]-r[1]));duv.data[li].uv=(v[axes[0]]/4,v[axes[1]]/4)
 bpy.ops.object.select_all(action='DESELECT');o.select_set(True);bpy.context.view_layer.objects.active=o;mod=o.modifiers.new('Triangulate','TRIANGULATE');bpy.ops.object.modifier_apply(modifier=mod.name);o.data.calc_loop_triangles()
 bounds=[min(v.co[i] for v in o.data.vertices) for i in range(3)]+[max(v.co[i] for v in o.data.vertices) for i in range(3)]
 entries.append({'name':name,'triangles':len(o.data.loop_triangles),'material_slots':len(o.data.materials),'materials':[m.name for m in o.data.materials],'bounds_m':bounds,'pivot_m':[0,0,0],'collision_boxes':[[[-.3,-.3,0],[.3,.3,.55]]] if name=='SM_EM_Beacon' else [],'reused_ue':name=='SM_MIE_EmergencyLight'})
 if name=='SM_MIE_EmergencyLight':shutil.copy2(REF/(name+'.fbx'),OUT/'Models'/(name+'.fbx'))
 else:
  o.name=name+'_source';cp=o.copy();cp.data=o.data.copy();s.collection.objects.link(cp);cp.name=name;reflect(cp);bpy.ops.object.select_all(action='DESELECT');cp.select_set(True);bpy.context.view_layer.objects.active=cp
  bpy.ops.export_scene.fbx(filepath=str(OUT/'Models'/(name+'.fbx')),use_selection=True,object_types={'MESH'},apply_unit_scale=True,apply_scale_options='FBX_SCALE_NONE',axis_forward='-Y',axis_up='Z',mesh_smooth_type='FACE',bake_anim=False,path_mode='STRIP');bpy.data.objects.remove(cp,do_unlink=True);o.name=name
 o.hide_render=True;o.hide_set(True)
placements=[{'name':'Beacon','mesh':'SM_EM_Beacon','location_m':[-3.9,0,0]}, {'name':'DirectionSign','mesh':'SM_EM_DirectionSign','location_m':[-5,0,0]}, {'name':'EmergencyLight','mesh':'SM_MIE_EmergencyLight','location_m':[-4.35,.45,0]}]
instances=[]
for p in placements:
 src=assets[p['mesh']];o=src.copy();o.data=src.data;s.collection.objects.link(o);o.name=p['name'];o.location=p['location_m'];o.hide_render=False;o.hide_set(False);instances.append(o)
groundmat=material('PreviewGround',color=(.42,.44,.40,1));floor=box('PreviewGround',(-8,-8,-.08),(8,8,-.01),groundmat)
def camera(name,loc,target,ortho=0):
 d=bpy.data.cameras.new(name);o=bpy.data.objects.new(name,d);s.collection.objects.link(o);o.location=loc;o.rotation_euler=(Vector(target)-o.location).to_track_quat('-Z','Y').to_euler();d.lens=38
 if ortho:d.type='ORTHO';d.ortho_scale=ortho
 return o
ld=bpy.data.lights.new('DaySoftbox','AREA');ld.energy=1800;ld.shape='DISK';ld.size=8;lo=bpy.data.objects.new('DaySoftbox',ld);s.collection.objects.link(lo);lo.location=(-6.9,-2.9,8)
def render(name,cam):
 if os.environ.get('EM_SKIP_RENDER'):return
 if os.environ.get('EM_RENDER_FILTER') and name not in os.environ['EM_RENDER_FILTER'].split(','):return
 s.use_nodes=True;nodes=s.node_tree.nodes
 if not nodes.get('ProjectAxisFlip'):
  nodes.clear();rl=nodes.new('CompositorNodeRLayers');fl=nodes.new('CompositorNodeFlip');fl.name='ProjectAxisFlip';fl.axis='X';co=nodes.new('CompositorNodeComposite');s.node_tree.links.new(rl.outputs['Image'],fl.inputs[0]);s.node_tree.links.new(fl.outputs[0],co.inputs[0])
 nodes['ProjectAxisFlip'].mute=cam.name!='TrueTopDown'
 s.camera=cam;s.render.filepath=str(OUT/'Previews'/(name+'.png'));bpy.ops.render.render(write_still=True)
overview=camera('Overview',(-5.9,-2.6,2.4),(-4.3,.05,.2),3.2);render('Blender_Day_Assembly',overview)
top=camera('TrueTopDown',(-4.3,.05,8),(-4.3,.05,0),3.1);top.rotation_euler=(0,0,-math.pi/2);render('Blender_TrueTopDown',top)
render('Blender_Reverse',camera('Reverse',(-1.9,2.6,2.2),(-4.3,.05,.2),3.2))
for emission,label in [(0,'Off'),(2,'On')]:
 green.node_tree.nodes['Principled BSDF'].inputs['Emission Strength'].default_value=emission;amber.node_tree.nodes['Principled BSDF'].inputs['Emission Strength'].default_value=emission*1.5;render('Blender_Day_'+label,overview)
 ld.energy=90;s.world.node_tree.nodes['Background'].inputs[1].default_value=.03;render('Blender_Dark_'+label,overview);ld.energy=1800;s.world.node_tree.nodes['Background'].inputs[1].default_value=.6
for o in instances:o.hide_render=True
for name,o in assets.items():
 o.hide_render=False;o.hide_set(False);entry=next(e for e in entries if e['name']==name);b=entry['bounds_m'];center=Vector([(b[i]+b[i+3])/2 for i in range(3)]);span=max(b[i+3]-b[i] for i in range(3));lo.location=(-2,-3,5)
 render('Blender_Sheet_'+name,camera('Sheet_'+name,center+Vector((-1,-1.5,1.2))*span,center,span*1.7));o.hide_render=True;o.hide_set(True)
for o in instances:o.hide_render=False
lo.location=(-6.9,-2.9,8);s.camera=overview
manifest={'assets':entries,'placements':placements,'sample_radius_cm':300,'source_units':'meters','axes':'+X north +Y east +Z up','unique_triangles':sum(e['triangles'] for e in entries),'total_material_slots':sum(e['material_slots'] for e in entries),'new_atlas_size':[1024,1024],'prop_light_actors':0}
(OUT/'model_manifest.json').write_text(json.dumps(manifest,indent=2));bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'ExtractionMarkers.blend'));print('EM_BUILD_PASSED',manifest['unique_triangles'])
