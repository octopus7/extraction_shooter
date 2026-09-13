"""One-off Blender 4.5 rig and locomotion authoring tool."""
import bpy, math, json
from mathutils import Vector, Matrix
from pathlib import Path

BASE=Path('D:/github/extraction_shooter')
OUT=BASE/'Blender/SM_MoleDummy_Rigged.blend'
bpy.ops.wm.open_mainfile(filepath=str(BASE/'Blender/SM_MoleDummy.blend'))
mesh=bpy.data.objects['SM_MoleDummy']
bpy.ops.object.select_all(action='DESELECT')
mesh.select_set(True); bpy.context.view_layer.objects.active=mesh
bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
arm=bpy.data.armatures.new('MoleSkeleton')
rig=bpy.data.objects.new('RIG_MoleDummy',arm); bpy.context.collection.objects.link(rig)
bpy.context.view_layer.objects.active=rig; mesh.select_set(False); rig.select_set(True)
bpy.ops.object.mode_set(mode='EDIT')
def bone(n,h,t,p=None,deform=True):
    b=arm.edit_bones.new(n); b.head=h; b.tail=t; b.use_deform=deform
    if p: b.parent=arm.edit_bones[p]
bone('root',(0,0,0),(0,.12,0),deform=False)
bone('pelvis',(0,.055,.22),(0,.055,.39),'root')
bone('spine',(0,.055,.39),(0,.055,.62),'pelvis')
bone('head',(0,.055,.62),(0,.055,.96),'spine')
bone('tail',(0,.22,.23),(0,.41,.055),'pelvis')
for s,sg in [('L',1),('R',-1)]:
    bone('upper_arm.'+s,(sg*.19,.035,.61),(sg*.31,.015,.46),'spine')
    bone('forearm.'+s,(sg*.31,.015,.46),(sg*.385,-.02,.355),'upper_arm.'+s)
    bone('hand.'+s,(sg*.385,-.02,.355),(sg*.39,-.04,.305),'forearm.'+s)
    bone('thigh.'+s,(sg*.14,.045,.24),(sg*.15,-.02,.135),'pelvis')
    bone('shin.'+s,(sg*.15,-.02,.135),(sg*.15,.015,.055),'thigh.'+s)
    bone('foot.'+s,(sg*.15,.015,.055),(sg*.15,-.17,.04),'shin.'+s)
    bone('CTRL_foot.'+s,(sg*.15,.015,.055),(sg*.15,-.17,.04),'root',False)
bpy.ops.object.mode_set(mode='OBJECT')
bpy.ops.object.select_all(action='DESELECT'); mesh.select_set(True); rig.select_set(True)
bpy.context.view_layer.objects.active=rig
bpy.ops.object.parent_set(type='ARMATURE_AUTO')
assert mesh.modifiers and any(m.type=='ARMATURE' for m in mesh.modifiers)
missing=[v.index for v in mesh.data.vertices if not v.groups]
if missing:
    from mathutils.kdtree import KDTree
    good=[v for v in mesh.data.vertices if v.groups]
    tree=KDTree(len(good))
    for v in good: tree.insert(v.co,v.index)
    tree.balance()
    for idx in missing:
        _,near,_=tree.find(mesh.data.vertices[idx].co)
        for g in mesh.data.vertices[near].groups: mesh.vertex_groups[g.group].add([idx],g.weight,'REPLACE')
missing=[v.index for v in mesh.data.vertices if not v.groups]
assert not missing, f'Unweighted vertices: {len(missing)}'
# Weld weight values across coincident split-normal/UV vertices without changing geometry.
from mathutils.kdtree import KDTree
tree=KDTree(len(mesh.data.vertices))
for v in mesh.data.vertices: tree.insert(v.co,v.index)
tree.balance(); visited=set()
for v in mesh.data.vertices:
    if v.index in visited: continue
    ids=[idx for _,idx,_ in tree.find_range(v.co,.0001)]
    visited.update(ids)
    if len(ids)<2: continue
    weights={}
    for idx in ids:
        for g in mesh.data.vertices[idx].groups: weights[g.group]=weights.get(g.group,0)+g.weight/len(ids)
    for vg in mesh.vertex_groups: vg.remove(ids)
    for idx,w in weights.items(): mesh.vertex_groups[idx].add(ids,w,'REPLACE')
for v in mesh.data.vertices:
    weights=[(g.group,g.weight) for g in v.groups]; total=sum(w for _,w in weights)
    assert total>0
    for idx,w in weights: mesh.vertex_groups[idx].add([v.index],w/total,'REPLACE')
for s in ['L','R']:
    ik=rig.pose.bones['shin.'+s].constraints.new('IK'); ik.target=rig; ik.subtarget='CTRL_foot.'+s; ik.chain_count=2
    con=rig.pose.bones['foot.'+s].constraints.new('COPY_ROTATION'); con.target=rig; con.subtarget='CTRL_foot.'+s; con.target_space='POSE'; con.owner_space='POSE'
for b in rig.pose.bones: b.rotation_mode='XYZ'
rig.show_in_front=True; arm.display_type='OCTAHEDRAL'
rig['Usage']='Select rig; Action Editor: Walk_InPlace, Walk_Forward, Turn_Left_90, Turn_Right_90. Feet: CTRL_foot.L/R. Forward: -Y. 30 fps.'
scene=bpy.context.scene; scene.render.fps=30
def curves(a):
    return [fc for layer in a.layers for st in layer.strips for bag in st.channelbags for fc in bag.fcurves]
def reset():
    for b in rig.pose.bones: b.location=(0,0,0); b.rotation_euler=(0,0,0); b.scale=(1,1,1)
def foot_target(s,offset):
    b=rig.pose.bones['CTRL_foot.'+s]
    b.location=b.bone.matrix_local.to_3x3().inverted()@Vector(offset)
def yaw_at(f,sign):
    t=max(0,min(1,f/48)); return sign*math.pi/2*(t*t*(3-2*t))
for name,end in [('Walk_InPlace',33),('Walk_Forward',33),('Turn_Left_90',65),('Turn_Right_90',65)]:
    reset(); rig.animation_data_create(); a=bpy.data.actions.new(name); a.use_fake_user=True; rig.animation_data.action=a
    turn=name.startswith('Turn'); sign=1 if 'Left' in name else -1
    for f in range(1,end+1):
        reset(); t=f-1; phase=2*math.pi*t/32
        root=rig.pose.bones['root']; pelvis=rig.pose.bones['pelvis']
        yaw=yaw_at(t,sign) if turn else 0
        root.rotation_euler.z=yaw
        if name=='Walk_Forward': root.location.y=-.32*t/32
        fade=math.sin(math.pi*t/64)**2 if turn else 1
        pelvis.location=pelvis.bone.matrix_local.to_3x3().inverted()@Vector((0,0,-.012+.006*math.cos(2*phase)*fade))
        pelvis.rotation_euler.y=.025*math.sin(phase)*fade
        rig.pose.bones['spine'].rotation_euler.y=-.018*math.sin(phase)*fade
        rig.pose.bones['head'].rotation_euler.y=-.012*math.sin(phase)*fade
        rig.pose.bones['tail'].rotation_euler.y=.06*math.sin(phase)*fade
        for s,sg,shift in [('L',1,0),('R',-1,16)]:
            q=((t+shift)%32)/32
            if not turn:
                y=-.08+.32*q if q<.5 else .08-.16*((q-.5)*2)**2*(3-2*(q-.5)*2)
                z=0 if q<.5 else .04*math.sin(math.pi*(q-.5)*2)**2
                foot_target(s,(0,y,z))
            else:
                cycle=math.floor((t+shift)/32); start=cycle*32-shift
                rest=Vector((sg*.15,.015,.055))
                old=Matrix.Rotation(yaw_at(start,sign),3,'Z')@rest
                new=Matrix.Rotation(yaw_at(start+32,sign),3,'Z')@rest
                u=max(0,(q-.5)*2); ease=u*u*(3-2*u)
                target=old.lerp(new,ease); target.z+=.035*math.sin(math.pi*u)**2
                local=Matrix.Rotation(-yaw,3,'Z')@target
                foot_target(s,local-rest)
                ctl=rig.pose.bones['CTRL_foot.'+s]; basis=ctl.bone.matrix_local.to_3x3()
                angle=(yaw_at(start,sign)*(1-ease)+yaw_at(start+32,sign)*ease)-yaw
                ctl.rotation_euler=(basis.inverted()@Matrix.Rotation(angle,3,'Z')@basis).to_euler()
            rig.pose.bones['upper_arm.'+s].rotation_euler.x=sg*.20*math.sin(phase)*fade
            rig.pose.bones['forearm.'+s].rotation_euler.x=.045*math.cos(phase+shift*math.pi/16)*fade
        for b in rig.pose.bones:
            b.keyframe_insert('location',frame=f,group=b.name); b.keyframe_insert('rotation_euler',frame=f,group=b.name)
    for fc in curves(a):
        for k in fc.keyframe_points: k.interpolation='LINEAR'
    a['Playback']='Frames 1-32 loop (33 matches start)' if not turn else 'Frames 1-65; root rotates 90 degrees, feet step in place'
    a.asset_mark()
rig.animation_data.action=bpy.data.actions['Walk_InPlace']
scene.frame_start=1; scene.frame_end=32; scene.frame_set(1)
notes=bpy.data.texts.new('README_MoleRig')
notes.write('Blender 4.5 / 30 fps / forward -Y / height about 1.04 m.\nOriginal mesh retained; automatic heat weights.\nWalk_InPlace: 1-32 loop, 33 is duplicate seam.\nWalk_Forward: 1-33, root advances 0.32 m.\nTurn_Left_90 / Turn_Right_90: 1-65; stepped pivot around root.\nUse Action Editor to switch actions; set scene frame end to 65 for turns.\nCTRL_foot.L/R drive two-bone leg IK; root controls locomotion.\nActions are saved as assets with fake users. For export bake evaluated pose/constraints.\n')
for area in bpy.context.screen.areas:
    if area.type=='VIEW_3D':
        area.spaces.active.region_3d.view_distance=2.3
        area.spaces.active.region_3d.view_location=(0,0,.5)
bpy.ops.wm.save_as_mainfile(filepath=str(OUT))
# Render previews after saving so staging does not enter the rig file.
bpy.ops.object.camera_add(location=(2,-3,1.6)); cam=bpy.context.object
cam.rotation_euler=(Vector((0,0,.5))-cam.location).to_track_quat('-Z','Y').to_euler(); cam.data.type='ORTHO'; cam.data.ortho_scale=1.45; scene.camera=cam
scene.render.engine='BLENDER_WORKBENCH'; scene.display.shading.light='STUDIO'; scene.display.shading.color_type='MATERIAL'
scene.render.resolution_x=480; scene.render.resolution_y=480; scene.render.resolution_percentage=100
preview=BASE/'Saved/MoleRig'; preview.mkdir(parents=True,exist_ok=True)
for action in ['Walk_InPlace','Turn_Left_90']:
    rig.animation_data.action=bpy.data.actions[action]
    for frame in ([1,9,17,25] if action=='Walk_InPlace' else [1,17,33,49,65]):
        scene.frame_set(frame); scene.render.filepath=str(preview/f'{action}_{frame:02}.png'); bpy.ops.render.render(write_still=True)
print('RIG_RESULT',json.dumps({'file':str(OUT),'bones':len(arm.bones),'vertices':len(mesh.data.vertices),'unweighted':len(missing),'actions':[(a.name,list(a.frame_range)) for a in bpy.data.actions]}))
for action in ['Walk_InPlace','Turn_Left_90']:
    rig.animation_data.action=bpy.data.actions[action]
    scene.frame_start=1; scene.frame_end=32 if action=='Walk_InPlace' else 65
    scene.render.image_settings.file_format='FFMPEG'; scene.render.ffmpeg.format='MPEG4'; scene.render.ffmpeg.codec='H264'
    scene.render.filepath=str(preview/(action+'.mp4')); bpy.ops.render.render(animation=True)
