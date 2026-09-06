"""Blender 4.5: concrete replacement kit preserving the existing door's motion contract."""
import bpy,bmesh,json,math
from pathlib import Path
from mathutils import Vector,Matrix
ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'TunaSweeper/SourceArt/Environment/GarageDoorConcrete'
bpy.ops.wm.read_factory_settings(use_empty=True)
scene=bpy.context.scene
scene.unit_settings.system='METRIC'
image=bpy.data.images.load(str(OUT/'Textures/T_GarageConcrete_Atlas.png'))
image.pack();image.filepath='//Textures/T_GarageConcrete_Atlas.png'
specs={'Concrete':(0,0,0,.9),'Steel':(1,0,.6,.5),'Panel':(0,1,.3,.65),'Rubber':(1,1,0,.85)}
mats={}
for key,(c,r,metal,rough) in specs.items():
    m=bpy.data.materials.new('M_GarageConcrete_'+key);m.use_nodes=True
    bsdf=m.node_tree.nodes['Principled BSDF'];bsdf.inputs['Metallic'].default_value=metal;bsdf.inputs['Roughness'].default_value=rough
    tex=m.node_tree.nodes.new('ShaderNodeTexImage');tex.image=image
    m.node_tree.links.new(tex.outputs['Color'],bsdf.inputs['Base Color']);mats[key]=m
for key,color,strength in [('LED',(.65,.85,1,1),5),('Signal',(.02,.25,.04,1),0)]:
    m=bpy.data.materials.new('M_GarageConcrete_'+key);m.use_nodes=True
    b=m.node_tree.nodes['Principled BSDF'];b.inputs['Base Color'].default_value=color
    b.inputs['Emission Color'].default_value=color;b.inputs['Emission Strength'].default_value=strength
    b.inputs['Roughness'].default_value=.28;mats[key]=m
groups={};active='';expected={};placements={}
def group(name,size,loc=(0,0,0)):
    global active
    active='SM_GarageConcrete_'+name;groups[active]=[];expected[active]=size;placements[active]=loc
def finish(o,name,mat='Steel',bevel=.35,smooth=False):
    o.name=name;bpy.context.view_layer.objects.active=o;o.select_set(True)
    bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
    if bevel:
        mod=o.modifiers.new('Edge bevel','BEVEL');mod.width=bevel/100;mod.segments=2
        bpy.ops.object.modifier_apply(modifier=mod.name)
    bm=bmesh.new();bm.from_mesh(o.data);bmesh.ops.dissolve_degenerate(bm,dist=1e-7,edges=list(bm.edges));bm.to_mesh(o.data);bm.free()
    for f in o.data.polygons:f.use_smooth=smooth
    if smooth:
        mod=o.modifiers.new('Weighted normals','WEIGHTED_NORMAL');mod.keep_sharp=True;bpy.ops.object.modifier_apply(modifier=mod.name)
    bpy.ops.object.mode_set(mode='EDIT');bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.uv.smart_project(angle_limit=math.radians(66),island_margin=.02)
    bpy.ops.object.mode_set(mode='OBJECT')
    if mat in specs:
        c,r,*_=specs[mat]
        for uv in o.data.uv_layers.active.data:
            uv.uv.x=(c+.07+.86*uv.uv.x)/2;uv.uv.y=(1-r+.07+.86*uv.uv.y)/2
    o.data.materials.append(mats[mat]);groups[active].append(o);o.select_set(False);return o
def cube_raw(loc,size):
    bpy.ops.mesh.primitive_cube_add(size=1,location=Vector(loc)/100)
    o=bpy.context.object;o.dimensions=Vector(size)/100
    bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
    return o
def box(name,loc,size,mat='Steel',bevel=.35):return finish(cube_raw(loc,size),name,mat,bevel)
def cylinder(name,loc,radius,depth,mat='Steel',vertices=24):
    bpy.ops.mesh.primitive_cylinder_add(vertices=vertices,radius=radius/100,depth=depth/100,location=Vector(loc)/100)
    o=bpy.context.object;o.rotation_euler.x=math.pi/2
    return finish(o,name,mat,.15,True)
def bolt(loc,r=.7):return cylinder('Bolt',loc,r,.7,'Steel',6)

# True runtime dimensions, unlike the original 600 cm prototype OBJ source.
for side,label in [(-1,'FrameLeft'),(1,'FrameRight')]:
    group(label,(35,45,195),(side*117.5,0,97.5))
    base=cube_raw((0,0,0),(35,45,195))
    # Rail center is inward of the jamb center; its pocket must follow that offset.
    cut=cube_raw((-side*7.875,-16.1,62.0),(20.25,13.8,76.0))
    bpy.context.view_layer.objects.active=base
    mod=base.modifiers.new('Rail pocket aligned with actual rail','BOOLEAN');mod.operation='DIFFERENCE';mod.object=cut
    bpy.ops.object.modifier_apply(modifier=mod.name);bpy.data.objects.remove(cut,do_unlink=True)
    finish(base,'Concrete_jamb','Concrete',.65)
    for z in (-68,-20,20):
        # Recessed formwork plugs, within the contracted bounds.
        cylinder('Form_tie',(side*10,-22.15,z),1.0,.5,'Concrete',12)
    box('Jamb_foot_armor',(0,-21.5,-88),(32,1.1,13),'Steel',.4)
group('FrameTop',(270,45,35),(0,0,212.5))
box('Concrete_lintel',(0,0,0),(270,45,35),'Concrete',.8)
box('Lintel_inset_rail',(0,-22,0),(192,.7,14),'Steel',.5)
for x in (-90,90):bolt((x,-22.1,0),1)
for side,label in [(-1,'CanopyRailLeft'),(1,'CanopyRailRight')]:
    group(label,(19.25,74,12.3125))
    box('Rail_inner_web',(0,0,0),(4,74,12.3125),'Steel',.3)
    for z in (-5.35,5.35):box('Rail_flange',(0,0,z),(19.25,74,1.6125),'Steel',.28)
    box('Rail_runner',(0,0,5.98),(11,68,.35),'Rubber',.1)
group('UpperPanel',(200,16,45))
# Four copies of this centered slab retain independent native hinges.
for x in (-97,97):box('Panel_end_cap',(x,0,0),(6,16,45),'Steel',.4)
for z in (-20,20):box('Panel_edge_rail',(0,0,z),(190,16,5),'Steel',.35)
box('Panel_structural_skin',(0,1,0),(190,11,36),'Panel',.6)
for x in (-62,0,62):
    box('Inset_plate',(x,-5.2,0),(58,1.6,29),'Panel',.7)
    for z in (-14,14):bolt((x,-6.25,z),.65)
for x in (-92,92):
    box('Seam_gasket',(x,-7.5,0),(.8,.8,35),'Rubber',.1)
group('LowerEmbeddedPanel',(200,16,25),(0,0,2.5))
box('Lower_plinth',(0,0,0),(200,16,25),'Steel',.5)
box('Lower_inset',(0,-7.5,0),(183,.8,14),'Panel',.4)
for x in (-85,85):bolt((x,-7.6,0),.8)
group('LEDBar',(110,6,10))
box('LED_housing',(0,0,0),(110,6,10),'Steel',.55)
box('LED_gasket',(0,-2.7,0),(105,.35,7.8),'Rubber',.3)
box('LED_diffuser',(0,-2.93,0),(102,.14,6),'LED',.055)
group('MotionIndicator',(12,4,12))
cylinder('Signal_bezel',(0,.5,0),6,3,'Steel',32)
cylinder('Signal_seal',(0,-1.48,0),5.05,.95,'Rubber',32)
cylinder('Signal_lens',(0,-1.89,0),4.5,.22,'Signal',32)
for x in (-5.45,5.45):bolt((x,-1.65,0),.35)
# Optional shell pieces retain the existing authored component slots.
for side,label in [(-1,'TemporaryWallLeft'),(1,'TemporaryWallRight')]:
    group(label,(200,45,230),(side*235,0,115))
    box('Concrete_side_shell',(0,0,0),(200,45,230),'Concrete',.8)
group('TemporaryRoof',(670,200,45),(0,100,252.5))
box('Concrete_roof',(0,0,0),(670,200,45),'Concrete',.9)

assets={};manifest={'assets':[],'materials':{key:{'name':mats[key].name,'metallic':v[2],'roughness':v[3]} for key,v in specs.items()},
                   'axis_contract':'X width, -Y exterior, Z up; meshes centered. FBX pre-mirrors Y to compensate Unreal conversion.',
                   'indicator_cpd':{'0':'emissive strength','1..3':'linear RGB'},'total_triangles':0}
for name,parts in groups.items():
    bpy.ops.object.select_all(action='DESELECT')
    for p in parts:p.select_set(True)
    bpy.context.view_layer.objects.active=parts[0];bpy.ops.object.join();o=bpy.context.object;o.name=name
    bpy.ops.object.transform_apply(location=False,rotation=True,scale=True)
    scene.cursor.location=(0,0,0);bpy.ops.object.origin_set(type='ORIGIN_CURSOR')
    bpy.ops.object.mode_set(mode='EDIT');bpy.ops.mesh.select_all(action='SELECT');bpy.ops.mesh.normals_make_consistent(inside=False);bpy.ops.object.mode_set(mode='OBJECT')
    o.data.calc_loop_triangles()
    actual=[d*100 for d in o.dimensions]
    assert max(abs(a-b) for a,b in zip(actual,expected[name]))<.1,(name,actual,expected[name])
    assert not any(t.area<1e-12 for t in o.data.loop_triangles),name
    assert o.data.uv_layers.active
    entry={'name':name,'size_cm':actual,'triangles':len(o.data.loop_triangles),'materials':[m.name for m in o.data.materials],
           'closed_location_cm':placements[name]}
    manifest['assets'].append(entry);manifest['total_triangles']+=entry['triangles'];assets[name]=o
    # Export mirrored copies only; Blender review geometry remains in project axes.
    copy=o.copy();copy.data=o.data.copy();scene.collection.objects.link(copy)
    o.select_set(False);copy.select_set(True);bpy.context.view_layer.objects.active=copy
    original_name=o.name;o.name=original_name+'_source';copy.name=original_name
    copy.data.transform(Matrix.Diagonal((1,-1,1,1)))
    bm=bmesh.new();bm.from_mesh(copy.data);bmesh.ops.reverse_faces(bm,faces=list(bm.faces));bm.to_mesh(copy.data);bm.free()
    bpy.ops.export_scene.fbx(filepath=str(OUT/'Models'/f'{original_name}.fbx'),use_selection=True,object_types={'MESH'},
        apply_unit_scale=True,apply_scale_options='FBX_SCALE_NONE',axis_forward='-Y',axis_up='Z',
        mesh_smooth_type='FACE',add_leaf_bones=False,bake_anim=False,path_mode='STRIP')
    bpy.data.objects.remove(copy,do_unlink=True);o.name=original_name
(OUT/'model_manifest.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')

# Assembly preview uses the exact native door-pose equations.
upper=[]
for i in range(4):
    o=assets['SM_GarageConcrete_UpperPanel'] if i==0 else assets['SM_GarageConcrete_UpperPanel'].copy()
    if i:scene.collection.objects.link(o);o.name='Preview_UpperPanel_%d'%i
    upper.append(o)
for n,o in assets.items():
    o.location=Vector(placements[n])/100
    if 'Temporary' in n:o.hide_render=True;o.hide_set(True)
signal=assets['SM_GarageConcrete_MotionIndicator']
red=signal;green=signal.copy();green.data=signal.data.copy();scene.collection.objects.link(green);green.name='Preview_GreenSignal'
def signal_material(o,label,color):
    o.data=o.data.copy()
    for i,m in enumerate(o.data.materials):
        if m==mats['Signal']:
            m=m.copy();m.name='Preview_'+label;o.data.materials[i]=m
            b=m.node_tree.nodes['Principled BSDF'];b.inputs['Base Color'].default_value=(*[x*.12 for x in color],1)
            b.inputs['Emission Color'].default_value=(*color,1)
            return b
red_bsdf=signal_material(red,'Red',(1,.015,.01));green_bsdf=signal_material(green,'Green',(.015,1,.06))
red.location=(1.28,-.245,1.3575);green.location=(1.28,-.245,1.1775)
def smooth(v):v=max(0,min(1,v));return v*v*(3-2*v)
def remap(v,a,b):return smooth((v-a)/(b-a))
def pose(alpha):
    top=1.95;total=1.8;h=.45;reveal=.0675
    for i,o in enumerate(upper):
        hinge_z=top-i*h;initial=(hinge_z-h-.15)/total;progress=initial+alpha;bounded=min(progress,1)
        lift=total*(min(bounded,.75)-min(initial,.75))
        relturn=max(0,min(1,remap(bounded,.5,.75)-remap(initial,.5,.75)))
        stack=remap(bounded,.75,1);slide=remap(progress,1,1.25) if i<3 else 0
        roll=(45*relturn)*(1-stack)+90*stack
        hinge=Vector((0,0,hinge_z+lift)).lerp(Vector((0,i*reveal,top-i*.1)),stack)+Vector((0,-.29*slide,0))
        rot=Matrix.Rotation(math.radians(-roll),4,'X')
        o.location=hinge+rot@Vector((0,0,-h/2));o.rotation_euler.x=math.radians(-roll)
        if i==0:
            led=assets['SM_GarageConcrete_LEDBar'];led.location=hinge+rot@Vector((0,-.11,-h));led.rotation_euler.x=math.radians(-roll)
            for side,label in [(-1,'CanopyRailLeft'),(1,'CanopyRailRight')]:
                rail=assets['SM_GarageConcrete_'+label]
                a=math.radians(90+roll);rotation=Matrix.Rotation(a,4,'X')
                rail.location=Vector((side*1.09625,-.1634375,top-.74+.74*roll/90))+rotation@Vector((0,.37,0))
                rail.rotation_euler.x=a
    assets['SM_GarageConcrete_LowerEmbeddedPanel'].location.z=.025-.175*remap(alpha,0,.12)

scene.render.engine='CYCLES';scene.cycles.samples=40;scene.cycles.use_denoising=True
scene.render.resolution_x=1400;scene.render.resolution_y=1400;scene.render.resolution_percentage=100
scene.view_settings.view_transform='AgX';scene.world=bpy.data.worlds.new('Studio');scene.world.use_nodes=True
scene.world.node_tree.nodes['Background'].inputs['Strength'].default_value=.4
preview=bpy.data.collections.new('PREVIEW_ONLY');scene.collection.children.link(preview)
def preview_link(o):
    for c in list(o.users_collection):c.objects.unlink(o)
    preview.objects.link(o)
bpy.ops.mesh.primitive_plane_add(size=200,location=(0,0,-.002));floor=bpy.context.object;preview_link(floor)
floor.name='PreviewFloor';m=bpy.data.materials.new('PreviewFloor');m.diffuse_color=(.16,.19,.21,1);floor.data.materials.append(m)
for name,loc,energy,size in [('Key',(-3,-5,6),1200,5),('Fill',(4,-2,4),900,4),('Rim',(0,4,5),1400,4)]:
    data=bpy.data.lights.new(name,'AREA');data.energy=energy;data.shape='DISK';data.size=size
    o=bpy.data.objects.new(name,data);preview.objects.link(o);o.location=loc;o.rotation_euler=(Vector((0,0,1))-o.location).to_track_quat('-Z','Y').to_euler()
data=bpy.data.cameras.new('ReviewCamera');cam=bpy.data.objects.new('ReviewCamera',data);preview.objects.link(cam)
cam.location=(4,-7,3.4);cam.rotation_euler=(Vector((0,-.1,1.05))-cam.location).to_track_quat('-Z','Y').to_euler()
data.type='ORTHO';data.ortho_scale=3.7;scene.camera=cam
pose(0);red_bsdf.inputs['Emission Strength'].default_value=0;green_bsdf.inputs['Emission Strength'].default_value=0
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'GarageDoorConcrete.blend'))
for name,alpha,red_on,green_on in [('Closed',0,0,0),('Opening',.46,0,8),('Open',1,0,0),('Closing',.46,8,0)]:
    pose(alpha);red_bsdf.inputs['Emission Strength'].default_value=red_on;green_bsdf.inputs['Emission Strength'].default_value=green_on
    scene.render.filepath=str(OUT/'Previews'/f'GarageConcrete_{name}.png');bpy.ops.render.render(write_still=True)
print('GARAGE_CONCRETE_BUILD_COMPLETE '+str(manifest['total_triangles']))
