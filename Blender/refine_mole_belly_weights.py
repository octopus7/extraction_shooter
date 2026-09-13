"""One-off Blender 4.5 belly skin-weight correction; no mesh/animation changes."""
import bpy, json
from pathlib import Path
from mathutils import Vector
BASE=Path('D:/github/extraction_shooter')
path=BASE/'Blender/SM_MoleDummy_Rigged.blend'
bpy.ops.wm.open_mainfile(filepath=str(path))
mesh=bpy.data.objects['SM_MoleDummy']; rig=bpy.data.objects['RIG_MoleDummy']; scene=bpy.context.scene
def smooth(a,b,x):
    t=max(0,min(1,(x-a)/(b-a))); return t*t*(3-2*t)
old=[{g.group:g.weight for g in v.groups} for v in mesh.data.vertices]
pelvis=mesh.vertex_groups['pelvis'].index; spine=mesh.vertex_groups['spine'].index
changed=0; central=[]
for v in mesh.data.vertices:
    x,y,z=v.co
    mask=(1-smooth(.21,.30,abs(x)))*(1-smooth(-.03,.15,y))*smooth(.07,.14,z)*(1-smooth(.58,.72,z))
    if mask<1e-8: continue
    sw=.12+.88*smooth(.14,.57,z)
    weights={i:w*(1-mask) for i,w in old[v.index].items()}
    weights[pelvis]=weights.get(pelvis,0)+mask*(1-sw)
    weights[spine]=weights.get(spine,0)+mask*sw
    for vg in mesh.vertex_groups: vg.remove([v.index])
    total=sum(weights.values())
    for i,w in weights.items():
        if w>1e-9: mesh.vertex_groups[i].add([v.index],w/total,'REPLACE')
    changed+=1
    if abs(x)<.05 and y<-.15 and .14<z<.58:
        assert all(mesh.vertex_groups[g.group].name in ('pelvis','spine') for g in v.groups)
        central.append(v.index)
assert central
assert all(abs(sum(g.weight for g in v.groups)-1)<1e-5 for v in mesh.data.vertices)
rig.animation_data.action=bpy.data.actions['Walk_InPlace']; scene.frame_start=1; scene.frame_end=32; scene.frame_set(1)
rig['BellyWeighting']='Front belly uses smooth pelvis-to-spine blend; leg weights restricted near attachment. Geometry and animation unchanged.'
bpy.ops.wm.save_as_mainfile(filepath=str(path))
print('WEIGHT_RESULT',json.dumps({'changed_vertices':changed,'center_without_limb_influence':len(central)}))
new=[{g.group:g.weight for g in v.groups} for v in mesh.data.vertices]
# Temporary render staging is added after saving the production file.
bpy.ops.object.camera_add(location=(2,-3,1.6)); cam=bpy.context.object
cam.rotation_euler=(Vector((0,0,.5))-cam.location).to_track_quat('-Z','Y').to_euler(); cam.data.type='ORTHO'; cam.data.ortho_scale=1.45; scene.camera=cam
scene.render.engine='BLENDER_WORKBENCH'; scene.display.shading.light='STUDIO'; scene.display.shading.color_type='MATERIAL'
scene.render.resolution_x=640; scene.render.resolution_y=640; scene.render.resolution_percentage=100
out=BASE/'Saved/MoleRig/BellyFix'; out.mkdir(parents=True,exist_ok=True)
def assign(weights):
    for vg in mesh.vertex_groups: vg.remove(list(range(len(mesh.data.vertices))))
    for i,ws in enumerate(weights):
        for g,w in ws.items(): mesh.vertex_groups[g].add([i],w,'REPLACE')
    mesh.data.update(); bpy.context.view_layer.update()
for label,weights in [('before',old),('after',new)]:
    assign(weights)
    for action,frames in [('Walk_InPlace',[9,25,29]),('Turn_Left_90',[33]),('Turn_Right_90',[33])]:
        rig.animation_data.action=bpy.data.actions[action]
        for f in frames:
            scene.frame_set(f); scene.render.filepath=str(out/f'{label}_{action}_{f:02}.png'); bpy.ops.render.render(write_still=True)
for action in ['Walk_InPlace','Turn_Left_90']:
    rig.animation_data.action=bpy.data.actions[action]; scene.frame_start=1; scene.frame_end=32 if action=='Walk_InPlace' else 65
    scene.render.image_settings.file_format='FFMPEG'; scene.render.ffmpeg.format='MPEG4'; scene.render.ffmpeg.codec='H264'
    scene.render.filepath=str(out/(action+'.mp4')); bpy.ops.render.render(animation=True)
