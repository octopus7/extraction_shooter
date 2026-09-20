"""One-off authoring: poses solved in the original Blender rig; UE tracks use its exact bind rotations."""
import bpy, math, json, sys
from pathlib import Path
from mathutils import Vector, Quaternion, Matrix
ROOT=Path('D:/github/extraction_shooter')
OUT=ROOT/'TunaSweeper/SourceArt/Characters/LunaMk2/TitleMotions'
OUT.mkdir(parents=True,exist_ok=True)
(OUT/'Previews').mkdir(exist_ok=True)
bpy.ops.wm.open_mainfile(filepath=str(OUT/'LunaMk2_TitleMotions.blend'))
scene=bpy.context.scene
rig=bpy.data.objects['SK_LunaMk2']
rig.animation_data_clear()
rig.animation_data_create()
REST={b.name:b.matrix_local.copy() for b in rig.data.bones}
PB=rig.pose.bones
def update(): bpy.context.view_layer.update()
def rotate_world(name,axis,degrees):
    p=PB[name];m=p.matrix.copy();q=Quaternion(axis,math.radians(degrees))
    p.matrix=Matrix.LocRotScale(m.translation,q@m.to_quaternion(),Vector((1,1,1)));update()
def aim(name,child,target):
    p=PB[name];m=p.matrix.copy()
    before=PB[child].head-p.head
    q=before.rotation_difference(Vector(target)-p.head)
    p.matrix=Matrix.LocRotScale(m.translation,q@m.to_quaternion(),Vector((1,1,1)));update()
def ik(a,b,c,target,pole):
    start=PB[a].head.copy();target=Vector(target)
    l1=(REST[b].translation-REST[a].translation).length
    l2=(REST[c].translation-REST[b].translation).length
    ray=target-start;d=ray.length
    assert d<l1+l2-1e-5,(a,'unreachable',d,l1+l2)
    u=ray.normalized();v=Vector(pole)-start;v=(v-u*v.dot(u)).normalized()
    along=(l1*l1-l2*l2+d*d)/(2*d)
    joint=start+u*along+v*math.sqrt(max(0,l1*l1-along*along))
    aim(a,b,joint);aim(b,c,target)
    assert (PB[c].head-target).length<.0001,(c,PB[c].head,target)
def smooth(x):
    x=max(0,min(1,x));return x*x*x*(x*(6*x-15)+10)
def pose(weight=0,breath=0,yaw=0,turn=0,step=0,entrance=None):
    for p in PB:p.matrix_basis=Matrix.Identity(4)
    update()
    # Small hip hinge plus a distributed spinal lean; face counters the torso.
    p=PB['pelvis'];m=p.matrix.copy();m.translation+=Vector((.026*weight,.025,-.017-.006*weight))
    p.matrix=m;update()
    rotate_world('pelvis',(1,0,0),6)
    rotate_world('pelvis',(0,1,0),-2*weight)
    rotate_world('spine_01',(1,0,0),5+.22*breath)
    rotate_world('spine_03',(1,0,0),5+.35*breath)
    rotate_world('spine_05',(1,0,0),2+.18*breath)
    rotate_world('neck_01',(1,0,0),-10-.3*breath)
    rotate_world('neck_01',(0,1,0),-6+1.5*weight)
    rotate_world('head',(1,0,0),-3)
    rotate_world('head',(0,0,1),4+turn)
    for side,sign in [('l',1),('r',-1)]:
        # Grounded idle feet. The free right foot steps forward/outward in B.
        foot=Vector((sign*.063,-.005,REST['foot_'+side].translation.z))
        if side=='r':foot+=Vector((-.066*weight,-.065*weight,step))
        footYaw=0
        if entrance is not None:
            initial=Vector((sign*.063,-.005,REST['foot_'+side].translation.z))
            if side=='r':
                first=smooth((entrance-.06)/.36);second=smooth((entrance-.70)/.30)
                footYaw=-90+45*first+45*second
                lift=.035*math.sin(math.pi*first)**2+.025*math.sin(math.pi*second)**2
            else:
                progress=smooth((entrance-.34)/.42)
                footYaw=-90*(1-progress);lift=.038*math.sin(math.pi*progress)**2
            worldFoot=Quaternion((0,0,1),math.radians(footYaw))@initial
            worldFoot.z+=lift
            foot=Quaternion((0,0,1),math.radians(-yaw))@worldFoot
            footYaw-=yaw
        ik('thigh_'+side,'calf_'+side,'foot_'+side,foot,(sign*.055,-.45,.45))
        p=PB['foot_'+side];p.matrix=Matrix.LocRotScale(p.head,Quaternion((0,0,1),math.radians(footYaw))@REST[p.name].to_quaternion(),Vector((1,1,1)));update()
        # Gently clasped hands behind the waist, with elbows out of the torso.
        wrist=Vector((sign*.023+.026*weight,.145,.943+(0.008 if side=='l' else 0)))
        ik('upperarm_'+side,'lowerarm_'+side,'hand_'+side,wrist,(sign*.24,.16,.96))
        # Fingers point across the back toward the opposite wrist.
        hand=PB['hand_'+side];middle=PB['middle_01_'+side]
        aim(hand.name,middle.name,hand.head+Vector((-sign*.06,.005,-.024)))
        for finger in ['index','middle','ring','pinky']:
            for part,ang in [('01',14),('02',25),('03',14)]:
                bone=PB.get(f'{finger}_{part}_{side}')
                if bone:
                    bone.rotation_mode='QUATERNION';bone.rotation_quaternion=Quaternion((0,0,1),math.radians(sign*ang))
        update()
    # Root orientation is baked for the standalone entrance. Translation stays zero.
    if yaw:rotate_world('root',(0,0,1),yaw)
    update()


scene.render.image_settings.file_format='PNG'
def render(name):
    scene.render.filepath=str(OUT/'Previews'/f'{name}.png');bpy.ops.render.render(write_still=True)
uref=json.loads((ROOT/'TunaSweeper/Saved/TitleMotions/ue_rigs.json').read_text())['SKM_LunaMk2']['bones']
def uname(n):return n.replace('.','_')
S=Matrix.Diagonal(Vector((1,-1,1)))
clip_specs=[('C',3)]
reports={}
for suffix,duration in clip_specs:
    name='AS_LunaMk2_Title_'+suffix;end=round(duration*30)
    old=bpy.data.actions.get(name)
    if old:bpy.data.actions.remove(old)
    action=bpy.data.actions.new(name);action.use_fake_user=True;rig.animation_data.action=action
    tracks={n:{'p':[],'q':[]} for n in uref}
    for f in range(end+1):
        scene.frame_set(f+1);t=f/end
        if suffix in ['A','B']:pose(weight=int(suffix=='B'),breath=(1-math.cos(t*2*math.pi))*.5)
        elif suffix=='C':
            u=smooth((t-.15)/.72)
            pose(yaw=-90*(1-u),turn=12*math.sin(math.pi*u),entrance=u)
        else:
            w=smooth(t) if suffix=='AtoB' else 1-smooth(t)
            pose(weight=w,step=.022*math.sin(math.pi*t)**2)
        for p in PB:
            p.rotation_mode='QUATERNION'
            p.keyframe_insert('location',frame=f+1,group=p.name)
            p.keyframe_insert('rotation_quaternion',frame=f+1,group=p.name)
        worlds={}
        for p in PB:
            n=uname(p.name)
            if n not in uref:continue
            ref=uref[n];rr=ref['rotation'];rue=Quaternion((rr[3],rr[0],rr[1],rr[2]))
            delta=p.matrix.to_quaternion()@REST[p.name].to_quaternion().inverted()
            q=(S@delta.to_matrix()@S).to_quaternion()@rue
            off=p.matrix.translation-REST[p.name].translation
            loc=Vector(ref['location'])+Vector((off.x,-off.y,off.z))*100
            worlds[n]=(loc,q)
            parent=uname(p.parent.name) if p.parent else None
            if parent:
                pl,pq=worlds[parent];loc=pq.inverted()@(loc-pl);q=pq.inverted()@q
            q.normalize()
            prev=tracks[n]['q'][-1] if tracks[n]['q'] else None
            values=[q.x,q.y,q.z,q.w]
            if prev and sum(x*y for x,y in zip(prev,values))<0:values=[-x for x in values]
            tracks[n]['p'].append(list(loc));tracks[n]['q'].append(values)
    assert all(len(v['p'])==end+1 for v in tracks.values()),'Rig bone coverage'
    (OUT/(name+'.json')).write_text(json.dumps({'name':name,'fps':30,'frames':end,'tracks':tracks},separators=(',',':')))
    scene.frame_start=1;scene.frame_end=end+1
    scene.frame_set(1)
    bpy.ops.object.select_all(action='DESELECT');rig.select_set(True);bpy.context.view_layer.objects.active=rig
    bpy.ops.export_scene.fbx(filepath=str(OUT/(name+'.fbx')),use_selection=True,object_types={'ARMATURE'},add_leaf_bones=False,bake_anim=True,bake_anim_use_all_actions=False,bake_anim_use_nla_strips=False,bake_anim_simplify_factor=0,axis_forward='-Y',axis_up='Z')
    reports[name]={'seconds':duration,'fps':30,'keys':end+1,'bones':len(tracks)}
    for frame in ([1,31,61,91] if suffix=='C' else [1,31] if suffix in ['A','B'] else [25]):
        scene.frame_set(frame);render(f'{suffix}_{frame:03d}')
scene.frame_start=1;scene.frame_end=121;rig.animation_data.action=bpy.data.actions['AS_LunaMk2_Title_A'];scene.frame_set(1)
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'LunaMk2_TitleMotions.blend'))

print('TITLE_MOTION_SOURCE_COMPLETE')
