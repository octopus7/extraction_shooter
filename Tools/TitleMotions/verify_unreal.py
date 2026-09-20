"""Read-only validation of standalone title motions, evaluated on the production skeleton."""
import unreal, json, math
from pathlib import Path
BASE = '/Game/Characters/Player/LunaMk2/Animations/Title/'
NAMES = ['AS_LunaMk2_Title_A', 'AS_LunaMk2_Title_B', 'AS_LunaMk2_Title_C', 'AS_LunaMk2_Title_AtoB', 'AS_LunaMk2_Title_BtoA']
clips = {n: unreal.load_asset(BASE+n) for n in NAMES}
assert all(clips.values()), 'Missing standalone title animation assets'
sk = unreal.load_asset('/Game/Characters/Player/LunaMk2/SKM_LunaMk2_Skeleton')
opts = unreal.AnimPoseEvaluationOptions()
space = unreal.AnimPoseSpaces.WORLD
def pose(clip, t):
    return unreal.AnimPoseExtensions.get_anim_pose_at_time(clip, t, opts)
def bone(p, n):
    return unreal.AnimPoseExtensions.get_bone_pose(p,n,space)
def dist(a,b):
    return math.sqrt(sum((x-y)**2 for x,y in zip(a.to_tuple(),b.to_tuple())))
def compare(a,b):
    error=0.0
    for n in unreal.AnimPoseExtensions.get_bone_names(a):
        x,y=bone(a,n),bone(b,n)
        error=max(error,dist(x.translation,y.translation))
        q,r=x.rotation,y.rotation
        dot=abs(q.x*r.x+q.y*r.y+q.z*r.z+q.w*r.w)
        assert dot>.99999,(n,'rotation seam',dot)
    assert error<.03,('position seam cm',error)
    return error
report={'assets':{}, 'seams':{}}
for n,c in clips.items():
    assert isinstance(c,unreal.AnimSequence),n
    assert c.get_editor_property('skeleton')==sk,n
    assert not c.get_editor_property('enable_root_motion'),n
    length=c.get_play_length()
    assert length>0,n
    samples=[pose(c,length*i/60) for i in range(61)]
    for p in samples:
        assert dist(bone(p,'root').translation,unreal.Vector(0,0,0))<.001,(n,'root translation')
        # Hands stay on their own sides even while C turns; no crossed wrists behind the back.
        root=bone(p,'root')
        left=bone(p,'hand_l').translation-root.translation
        right=bone(p,'hand_r').translation-root.translation
        yaw=math.atan2(2*(root.rotation.w*root.rotation.z+root.rotation.x*root.rotation.y),1-2*(root.rotation.y**2+root.rotation.z**2))
        lx=math.cos(yaw)*left.x+math.sin(yaw)*left.y
        rx=math.cos(yaw)*right.x+math.sin(yaw)*right.y
        assert lx>20 and rx<-20,(n,'arms must remain open on separate sides',lx,rx)
        assert dist(bone(p,'hand_l').translation,bone(p,'hand_r').translation)>45,(n,'wrists must not cross')
        for name in unreal.AnimPoseExtensions.get_bone_names(p):
            tr=bone(p,name)
            assert all(math.isfinite(v) for v in tr.translation.to_tuple()),(n,name)
            assert max(abs(v-1) for v in tr.scale3d.to_tuple())<.005,(n,name,'scale')
            q=tr.rotation
            assert abs(q.x*q.x+q.y*q.y+q.z*q.z+q.w*q.w-1)<.001,(n,name,'unit rotation')
    report['assets'][n]={'seconds':length,'skeleton':sk.get_path_name()}
    if n.endswith(('_A','_B')):
        report['seams'][n]=compare(samples[0],samples[-1])
        for foot in ['foot_l','foot_r']:
            drift=max(dist(bone(samples[0],foot).translation,bone(p,foot).translation) for p in samples)
            assert drift<.08,(n,foot,'idle foot drift cm',drift)
            report['assets'][n][foot+'_drift_cm']=drift
        breath=max(dist(bone(samples[0],'spine_05').translation,bone(p,'spine_05').translation) for p in samples)
        assert .01<breath<1.5,(n,'breath amplitude',breath)
        report['assets'][n]['breath_cm']=breath
a,b,c,ab,ba=[clips[n] for n in NAMES]
for label,left,lt,right,rt in [('C_A',c,c.get_play_length(),a,0),('A_AB',a,0,ab,0),('AB_B',ab,ab.get_play_length(),b,0),('B_BA',b,0,ba,0),('BA_A',ba,ba.get_play_length(),a,0)]:
    report['seams'][label]=compare(pose(left,lt),pose(right,rt))
ap,bp=pose(a,0),pose(b,0)
assert dist(bone(ap,'foot_r').translation,bone(bp,'foot_r').translation)>5,'B must change leg stance'
cp=pose(c,0)
for foot in ['foot_l','foot_r']:
    floor=bone(ap,foot).translation.z
    heights=[bone(pose(c,c.get_play_length()*i/180),foot).translation.z for i in range(181)]
    assert min(heights)>=floor-.05,(foot,'C below reference floor')
    assert max(heights)>floor+2,(foot,'C needs a lifted turning step')
    report['assets'][NAMES[2]][foot+'_lift_cm']=max(heights)-floor
def root_yaw(p):
    q=bone(p,'root').rotation
    return math.degrees(math.atan2(2*(q.w*q.z+q.x*q.y),1-2*(q.y*q.y+q.z*q.z)))
# UE yaw +90 corresponds to the mirrored reference: screen-left profile.
# Follow the short path to front, opposite to the original -140-degree entrance.
yaws=[root_yaw(pose(c,c.get_play_length()*i/180)) for i in range(181)]
assert abs(yaws[0]-90)<.1,('C must begin at mirrored side profile',yaws[0])
assert abs(yaws[-1])<.1,('C must finish facing front',yaws[-1])
assert all(-.01<=y<=90.01 for y in yaws),'C must use the short side-to-front arc'
assert all(b<=a+.01 for a,b in zip(yaws,yaws[1:])),'C turn direction must be reversed'
report['assets'][NAMES[2]]['root_yaw_start_end_degrees']=[yaws[0],yaws[-1]]
out=Path('D:/github/extraction_shooter/TunaSweeper/SourceArt/Characters/LunaMk2/TitleMotions')
report['passed']=True
(out/'unreal_validation.json').write_text(json.dumps(report,indent=2))
unreal.log('TITLE_MOTIONS_VALIDATION_PASSED '+json.dumps(report))
