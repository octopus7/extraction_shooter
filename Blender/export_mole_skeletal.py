"""One-off: author subtle idle and stationary turn steps, export UE skeletal assets."""
import bpy, math, json
from mathutils import Vector
from pathlib import Path
BASE=Path('D:/github/extraction_shooter')
bpy.ops.wm.open_mainfile(filepath=str(BASE/'Blender/SKM_MoleDummy.blend'))
rig=next(o for o in bpy.context.scene.objects if o.type=='ARMATURE'); mesh=bpy.data.objects['SKM_MoleDummy']
mesh.name='SKM_MoleDummy'; mesh.data.name='SKM_MoleDummy_Mesh'; rig.name='Armature'
scene=bpy.context.scene; scene.render.fps=30
def reset():
    for b in rig.pose.bones: b.location=(0,0,0); b.rotation_euler=(0,0,0); b.scale=(1,1,1)
def world_offset(name,offset):
    b=rig.pose.bones[name]; b.location=b.bone.matrix_local.to_3x3().inverted()@Vector(offset)
for name,end in [('Idle_Breathe',121),('Turn_InPlace',33)]:
    if name in bpy.data.actions: bpy.data.actions.remove(bpy.data.actions[name])
    a=bpy.data.actions.new(name); a.use_fake_user=True; a.asset_mark(); rig.animation_data.action=a
    for f in range(1,end+1):
        reset(); phase=2*math.pi*(f-1)/(end-1)
        if name=='Idle_Breathe':
            breath=(1-math.cos(phase))/2
            # Four-second breath: chest expands <0.5%, head rises <1mm; feet stay anchored.
            rig.pose.bones['spine'].scale=(1+.004*breath,1+.002*breath,1+.004*breath)
            rig.pose.bones['head'].rotation_euler.x=.002*math.sin(phase)
            for side in ['L','R']: rig.pose.bones['upper_arm.'+side].rotation_euler.x=.003*breath
        else:
            world_offset('pelvis',(0,0,-.008+.003*math.cos(2*phase)))
            rig.pose.bones['spine'].rotation_euler.y=.008*math.sin(phase)
            for side,shift,sign in [('L',0,1),('R',math.pi,-1)]:
                p=phase+shift; lift=max(0,math.sin(p))**2
                world_offset('CTRL_foot.'+side,(sign*.006*lift,0,.023*lift))
                rig.pose.bones['upper_arm.'+side].rotation_euler.x=sign*.025*math.sin(phase)
        for b in rig.pose.bones:
            for prop in ['location','rotation_euler','scale']: b.keyframe_insert(prop,frame=f,group=b.name)
    for layer in a.layers:
        for strip in layer.strips:
            for bag in strip.channelbags:
                for fc in bag.fcurves:
                    for k in fc.keyframe_points: k.interpolation='LINEAR'
    a['Usage']='Runtime subtle breathing loop' if name=='Idle_Breathe' else 'Runtime stationary stepping loop; actor owns yaw, root stays fixed'
rig['Usage']='Blender 4.5 / 30 fps. Idle_Breathe and Turn_InPlace are runtime clips. Walking and 90-degree root turns are standalone animation assets only.'
rig.animation_data.action=bpy.data.actions['Idle_Breathe']; scene.frame_start=1; scene.frame_end=120; scene.frame_set(1)
if 'README_MoleRig' in bpy.data.texts:
    bpy.data.texts['README_MoleRig'].clear()
    bpy.data.texts['README_MoleRig'].write(rig['Usage']+'\nSelect Armature and use Action Editor. Idle: 1-120, seam121. Turn_InPlace and walks: 1-32, seam33. 90-degree turns:1-65. CTRL_foot.L/R are IK controls.\n')
bpy.ops.object.select_all(action='DESELECT'); rig.select_set(True); mesh.select_set(True); bpy.context.view_layer.objects.active=rig
bpy.ops.wm.save_as_mainfile(filepath=str(BASE/'Blender/SKM_MoleDummy.blend'))
out=BASE/'TunaSweeper/SourceArt/Characters/Mole'; out.mkdir(parents=True,exist_ok=True)
options=dict(use_selection=True,object_types={'ARMATURE','MESH'},add_leaf_bones=False,use_armature_deform_only=True,axis_forward='-Y',axis_up='Z',apply_unit_scale=True,bake_anim_use_nla_strips=False,bake_anim_use_all_actions=False,bake_anim_simplify_factor=0.0,bake_anim_force_startend_keying=True)
rig.animation_data.action=None; reset(); bpy.context.view_layer.update()
bpy.ops.export_scene.fbx(filepath=str(out/'SKM_MoleDummy.fbx'),bake_anim=False,**options)
for name in ['Idle_Breathe','Turn_InPlace','Walk_InPlace','Walk_Forward','Turn_Left_90','Turn_Right_90']:
    a=bpy.data.actions[name]; rig.animation_data.action=a; scene.frame_start=1; scene.frame_end=int(a.frame_range[1]); scene.frame_set(1)
    bpy.ops.export_scene.fbx(filepath=str(out/('A_Mole_'+name+'.fbx')),bake_anim=True,**options)
print('EXPORTED_MOLE',list(str(p) for p in out.glob('*.fbx')))
